#ifdef ENABLE_API_METAL

#include "MetalBackend.h"
#import "MetalBackendTypes.h"
#include "MetalCommandList.h"
#include "MetalConverters.h"
#include "MetalDescriptorHeap.h"
#import "MetalSwapchain.h"
#include "MetalUtils.h"
#include "MetalConstants.h"

#import <metal_irconverter_runtime/metal_irconverter_runtime.h>

namespace RHINO::APIMetal {
    void MetalBackend::Initialize() noexcept {
        m_Device = MTLCopyAllDevices()[0];
        m_DefaultQueue = [m_Device newCommandQueue];
        m_AsyncComputeQueue = [m_Device newCommandQueue];
        m_CopyQueue = [m_Device newCommandQueue];

        {
            NSError* error = nil;
            auto manager = [MTLCaptureManager sharedCaptureManager];
            MTLCaptureDescriptor* captureDescriptor = [[MTLCaptureDescriptor alloc] init];
            captureDescriptor.destination = MTLCaptureDestinationDeveloperTools;
            captureDescriptor.captureObject = m_Device;
            [manager startCaptureWithDescriptor:captureDescriptor error:&error];
        }
    }

    void MetalBackend::Release() noexcept {
        auto manager= [MTLCaptureManager sharedCaptureManager];
        [manager stopCapture];
    }

    RootSignature* MetalBackend::SerializeRootSignature(const RootSignatureDesc& desc) noexcept {
        IRError* pError = nullptr;
        auto* result = new MetalRootSignature{};

        // TODO: if root constants defined: add root constants root param

        std::vector<IRDescriptorRange1> rangeDescsStorage{};
        std::vector<IRRootParameter1> rootParamsDescs{};
        rootParamsDescs.reserve(desc.spacesCount + 1);
        std::vector<size_t> offsetsInRangeDescsPerSpaceIdx{};
        offsetsInRangeDescsPerSpaceIdx.resize(desc.spacesCount);

        for (size_t spaceIdx = 0; spaceIdx < desc.spacesCount; ++spaceIdx) {
            IRRootParameter1& rootParamDesc = rootParamsDescs.emplace_back();
            rootParamDesc.ParameterType = IRRootParameterTypeDescriptorTable;
            rootParamDesc.DescriptorTable.NumDescriptorRanges = desc.spacesDescs[spaceIdx].rangeDescCount;
            offsetsInRangeDescsPerSpaceIdx[spaceIdx] = rangeDescsStorage.size();

            //TODO: assert that ranges in one space exclusively CBVSRVUAV or SMP.

            for (size_t i = 0; i < desc.spacesDescs[spaceIdx].rangeDescCount; ++i) {
                IRDescriptorRange1& rangeDesc = rangeDescsStorage.emplace_back();
                rangeDesc.NumDescriptors = desc.spacesDescs[spaceIdx].rangeDescs[i].descriptorsCount;
                rangeDesc.RegisterSpace = desc.spacesDescs[spaceIdx].space;
                rangeDesc.OffsetInDescriptorsFromTableStart = desc.spacesDescs[spaceIdx].offsetInDescriptorsFromTableStart +
                                                              desc.spacesDescs[spaceIdx].rangeDescs[i].baseRegisterSlot;
                rangeDesc.BaseShaderRegister = desc.spacesDescs[spaceIdx].rangeDescs[i].baseRegisterSlot;
                switch (desc.spacesDescs[spaceIdx].rangeDescs[i].rangeType) {
                    case RHINO::DescriptorRangeType::SRV:
                        rangeDesc.RangeType = IRDescriptorRangeTypeSRV;
                        break;
                    case RHINO::DescriptorRangeType::UAV:
                        rangeDesc.RangeType = IRDescriptorRangeTypeUAV;
                        break;
                    case RHINO::DescriptorRangeType::CBV:
                        rangeDesc.RangeType = IRDescriptorRangeTypeCBV;
                        break;
                    case RHINO::DescriptorRangeType::SMP:
                        rangeDesc.RangeType = IRDescriptorRangeTypeSampler;
                        break;
                }
            }
        }
        for (size_t spaceIdx = 0; spaceIdx < desc.spacesCount; ++spaceIdx) {
            auto* rangesPtr = rangeDescsStorage.data() + offsetsInRangeDescsPerSpaceIdx[spaceIdx];
            rootParamsDescs[spaceIdx].DescriptorTable.pDescriptorRanges = rangesPtr;
        }

        IRRootSignatureDescriptor1 rootSignatureDesc{};
        rootSignatureDesc.NumParameters = rootParamsDescs.size();
        rootSignatureDesc.pParameters = rootParamsDescs.data();
        rootSignatureDesc.NumStaticSamplers = 0;
        rootSignatureDesc.pStaticSamplers = nullptr;
        rootSignatureDesc.Flags = IRRootSignatureFlags(IRRootSignatureFlagDenyHullShaderRootAccess |
                                                       IRRootSignatureFlagDenyDomainShaderRootAccess |
                                                       IRRootSignatureFlagDenyGeometryShaderRootAccess);

        IRVersionedRootSignatureDescriptor rsVersionedDesc{};
        rsVersionedDesc.version = IRRootSignatureVersion_1_1;
        rsVersionedDesc.desc_1_1 = rootSignatureDesc;
        result->rootSignature = IRRootSignatureCreateFromDescriptor(&rsVersionedDesc, &pError);
        if (pError) {
            return nullptr;
        }

        result->spaceDescs.resize(desc.spacesCount);
        std::vector<size_t> offsetInRangeDescsPerDescriptorSpace{};
        for (size_t spaceID = 0; spaceID < desc.spacesCount; ++spaceID) {
            result->spaceDescs[spaceID] = desc.spacesDescs[spaceID];
            offsetInRangeDescsPerDescriptorSpace.push_back(result->rangeDescsStorage.size());
            for (size_t i = 0; i < desc.spacesDescs[spaceID].rangeDescCount; ++i) {
                result->rangeDescsStorage.push_back(desc.spacesDescs[spaceID].rangeDescs[i]);
            }
        }
        for (size_t spaceID = 0; spaceID < desc.spacesCount; ++spaceID) {
            auto addr = result->rangeDescsStorage.data() + offsetInRangeDescsPerDescriptorSpace[spaceID];
            result->spaceDescs[spaceID].rangeDescs = addr;
        }

        return result;
    }

    static id<MTLLibrary> NewLibraryFromDXILUsingCompiler(const IRObject* pDXIL, IRShaderStage shaderStage, const char* entryPointName,
                                                          IRCompiler* pCompiler, id<MTLDevice> device,
                                                          const char* fuseAnyHitEntryPointName) noexcept {
        assert(pDXIL);
        assert(pCompiler);

        IRError* pError = nullptr;
        IRObject* pAIR = nullptr;
        if (fuseAnyHitEntryPointName) {
            pAIR = IRCompilerAllocCombineCompileAndLink(pCompiler, entryPointName, pDXIL, fuseAnyHitEntryPointName, pDXIL, &pError);
        }
        else {
            pAIR = IRCompilerAllocCompileAndLink(pCompiler, entryPointName, pDXIL, &pError);
        }

        if (!pAIR) {
            __builtin_printf("Error compiling shader \"%s\" to AIR: %s\n", entryPointName, (const char*)IRErrorGetPayload(pError));
            IRErrorDestroy(pError);
            __builtin_trap();
            return nil;
        }
        assert(pAIR);

        IRMetalLibBinary* pMetalLib = IRMetalLibBinaryCreate();
        if (!IRObjectGetMetalLibBinary(pAIR, shaderStage, pMetalLib)) {
            __builtin_printf("Error getting metallib binary\n");
            IRObjectDestroy(pAIR);
            IRCompilerDestroy(pCompiler);
            __builtin_trap();
            return nil;
        }
        size_t metallibSize = IRMetalLibGetBytecodeSize(pMetalLib);

        uint8_t* metallibBytecode = new uint8_t[metallibSize];
        IRMetalLibGetBytecode(pMetalLib, metallibBytecode);

        // dispatch_data_t dispatchData = IRMetalLibGetBytecodeData(pMetalLib);

        dispatch_data_t metallib = dispatch_data_create(metallibBytecode, metallibSize, dispatch_get_main_queue(),
                                                        DISPATCH_DATA_DESTRUCTOR_DEFAULT);
        id<MTLLibrary> pLib = [device newLibraryWithData:metallib error:nullptr];


        // CFRelease(metallib);
        delete[] metallibBytecode;
        IRMetalLibBinaryDestroy(pMetalLib);
        IRObjectDestroy(pAIR);

        return pLib;
    }

    id<MTLFunction> MetalBackend::CompileSingleRTPSOFunction(const ShaderModule& sm, IRObject* smIR, IRShaderStage stage, IRCompiler* compiler) noexcept {
        IRCompilerSetEntryPointName(compiler, sm.entrypoint);
        id<MTLLibrary> lib = NewLibraryFromDXILUsingCompiler(smIR, stage, sm.entrypoint,
                                                             compiler, m_Device, nullptr);
        NSString* functionName = [NSString stringWithUTF8String:sm.entrypoint];
        return [lib newFunctionWithName:functionName];
    }

    RTPSO* APIMetal::MetalBackend::CreateRTPSO(const RHINO::RTPSODesc& desc) noexcept {
        auto* metalRootSignature = INTERPRET_AS<MetalRootSignature*>(desc.rootSignature);

        static constexpr size_t VFT_START_IDX = 1;

        auto result = new MetalRTPSO{};
        IRCompiler* compiler = IRCompilerCreate();

        std::vector<id<MTLFunction>> compiledSMs{};
        compiledSMs.resize(desc.shaderModulesCount);

        std::vector<IRObject*> smIRs{};
        smIRs.resize(desc.shaderModulesCount);
        for (size_t i = 0; i < desc.shaderModulesCount; ++i) {
            const ShaderModule& sm = desc.shaderModules[i];
            smIRs[i] = IRObjectCreateFromDXIL(sm.bytecode, sm.bytecodeSize, IRBytecodeOwnershipNone);
        }

        uint64_t closestHitMask = 0x0;
        uint64_t missMask = 0x0;
        uint64_t anyHitMask = 0x0;

        result->shaderTableRecordStride = sizeof(IRShaderIdentifier);
        result->shaderTable = [m_Device newBufferWithLength:result->shaderTableRecordStride * desc.recordsCount
                                                    options:MTLResourceStorageModeShared];
        if (desc.debugName) {
            std::string debugName = std::string{desc.debugName} + ".ShaderTable";
            [result->shaderTable setLabel: [NSString stringWithUTF8String:debugName.c_str()]];
        }
        auto* shaderRecords = static_cast<IRShaderIdentifier*>([result->shaderTable contents]);

        for (size_t i = 0; i < desc.recordsCount; ++i) {
            const RTShaderTableRecord& record = desc.records[i];
            switch (record.recordType) {
                case RTShaderTableRecordType::HitGroup: {
                    if (record.hitGroup.clothestHitShaderEnabled) {
                        const ShaderModule& sm = desc.shaderModules[record.hitGroup.closestHitShaderIndex];
                        IRObject* smIR = smIRs[record.hitGroup.closestHitShaderIndex];
                        closestHitMask |= IRObjectGatherRaytracingIntrinsics(smIR, sm.entrypoint);
                    }
                    if (record.hitGroup.anyHitShaderEnabled) {
                        const ShaderModule& sm = desc.shaderModules[record.hitGroup.anyHitShaderIndex];
                        IRObject* smIR = smIRs[record.hitGroup.anyHitShaderIndex];
                        anyHitMask |= IRObjectGatherRaytracingIntrinsics(smIR, sm.entrypoint);
                    }
                    break;
                }
                case RTShaderTableRecordType::Miss: {
                    const ShaderModule& sm = desc.shaderModules[record.miss.missShaderIndex];
                    IRObject* smIR = smIRs[record.miss.missShaderIndex];
                    missMask |= IRObjectGatherRaytracingIntrinsics(smIR, sm.entrypoint);
                    break;
                }
                default:
                    break;
            }
        }

        //TODO: maybe remove
        IRCompilerSetMinimumDeploymentTarget(compiler, IROperatingSystem_macOS, "14.0.0");
        IRCompilerSetGlobalRootSignature(compiler, metalRootSignature->rootSignature);
        // IRCompilerSetLocalRootSignature(pCompiler, pLocalRootSignature);

        IRError* pError = nullptr;
        for (size_t i = 0; i < desc.recordsCount; ++i) {
            const RTShaderTableRecord& record = desc.records[i];

            IRCompilerSetRayTracingPipelineArguments(compiler, desc.maxAttributeSizeInBytes, IRRaytracingPipelineFlagNone,
                                                     closestHitMask, missMask, anyHitMask, ~0, IRRayTracingUnlimitedRecursionDepth,
                                                     IRRayGenerationCompilationVisibleFunction,
                                                     IRIntersectionFunctionCompilationVisibleFunction);

            switch (record.recordType) {
                case RTShaderTableRecordType::RayGeneration: {
                    const ShaderModule& sm = desc.shaderModules[record.rayGeneration.rayGenerationShaderIndex];
                    IRObject* smIR = smIRs[record.rayGeneration.rayGenerationShaderIndex];
                    compiledSMs[record.rayGeneration.rayGenerationShaderIndex] = CompileSingleRTPSOFunction(sm, smIR, IRShaderStageRayGeneration, compiler);
                    IRShaderIdentifierInit(&shaderRecords[i], record.rayGeneration.rayGenerationShaderIndex + VFT_START_IDX);
                    break;
                }
                case RTShaderTableRecordType::HitGroup: {
                    IRCompilerSetHitgroupType(compiler, IRHitGroupTypeTriangles);
                    if (record.hitGroup.clothestHitShaderEnabled) {
                        const ShaderModule& sm = desc.shaderModules[record.hitGroup.closestHitShaderIndex];
                        IRObject* smIR = smIRs[record.hitGroup.closestHitShaderIndex];
                        compiledSMs[record.hitGroup.closestHitShaderIndex] = CompileSingleRTPSOFunction(sm, smIR, IRShaderStageClosestHit, compiler);
                    }
                    if (record.hitGroup.anyHitShaderEnabled) {
                        const ShaderModule& sm = desc.shaderModules[record.hitGroup.anyHitShaderIndex];
                        IRObject* smIR = smIRs[record.hitGroup.anyHitShaderIndex];
                        compiledSMs[record.hitGroup.anyHitShaderIndex] = CompileSingleRTPSOFunction(sm, smIR, IRShaderStageAnyHit, compiler);
                    }
                    if (record.hitGroup.intersectionShaderEnabled) {
                        const ShaderModule& sm = desc.shaderModules[record.hitGroup.intersectionShaderIndex];
                        IRObject* smIR = smIRs[record.hitGroup.intersectionShaderIndex];
                        compiledSMs[record.hitGroup.intersectionShaderIndex] = CompileSingleRTPSOFunction(sm, smIR, IRShaderStageIntersection, compiler);
                    }
                    IRShaderIdentifierInit(&shaderRecords[i], record.hitGroup.closestHitShaderIndex + VFT_START_IDX);
//                    IRShaderIdentifierInitWithCustomIntersection(&shaderRecords[i], record.hitGroup.closestHitShaderIndex + VFT_START_IDX,
//                                                                 record.hitGroup.intersectionShaderIndex + VFT_START_IDX);
                    break;
                }
                case RTShaderTableRecordType::Miss: {
                    const ShaderModule& sm = desc.shaderModules[record.miss.missShaderIndex];
                    IRObject* smIR = smIRs[record.miss.missShaderIndex];
                    compiledSMs[record.miss.missShaderIndex] = CompileSingleRTPSOFunction(sm, smIR, IRShaderStageMiss, compiler);
                    IRShaderIdentifierInit(&shaderRecords[i], record.miss.missShaderIndex + VFT_START_IDX);
                    break;
                }
            }
        }

        NSError* error;

        // Synthesizing indirect intersection functions for AABB and Triangles
        id<MTLFunction> synthTriangleIndirectIntersectionFn = nil;
        id<MTLFunction> synthAABBIndirectIntersectionFn = nil;
        {
            bool status = true;

            // Triangle intersection
            {
                IRCompilerSetHitgroupType(compiler, IRHitGroupTypeTriangles);
                IRMetalLibBinary* indirectIntersectLibBin = IRMetalLibBinaryCreate();
                status = IRMetalLibSynthesizeIndirectIntersectionFunction(compiler, indirectIntersectLibBin);
                assert(status);

                id<MTLLibrary> indirectIntersectLib = [m_Device newLibraryWithData:IRMetalLibGetBytecodeData(indirectIntersectLibBin)
                                                                             error:&error];
                assert(indirectIntersectLib);
                NSString* indirectIntersectFnName = [NSString stringWithUTF8String:kIRIndirectTriangleIntersectionFunctionName];
                synthTriangleIndirectIntersectionFn = [indirectIntersectLib newFunctionWithName:indirectIntersectFnName];
                assert(synthTriangleIndirectIntersectionFn);
            }

            // AABB intersection
            {
                IRCompilerSetHitgroupType(compiler, IRHitGroupTypeProceduralPrimitive);
                IRMetalLibBinary* indirectIntersectLibBin = IRMetalLibBinaryCreate();
                status = IRMetalLibSynthesizeIndirectIntersectionFunction(compiler, indirectIntersectLibBin);
                assert(status);

                id<MTLLibrary> indirectIntersectLib = [m_Device newLibraryWithData:IRMetalLibGetBytecodeData(indirectIntersectLibBin)
                                                                             error:&error];
                assert(indirectIntersectLib);
                NSString* indirectIntersectFnName = [NSString stringWithUTF8String:kIRIndirectProceduralIntersectionFunctionName];
                synthAABBIndirectIntersectionFn = [indirectIntersectLib newFunctionWithName:indirectIntersectFnName];
                assert(synthAABBIndirectIntersectionFn);
            }
        }

        // Synthesizing dispatch ray function
        id<MTLFunction> dispatchSynthFn = nil;
        {
            IRMetalLibBinary* libBin = IRMetalLibBinaryCreate();
            IRMetalLibSynthesizeIndirectRayDispatchFunction(compiler, libBin);
            id<MTLLibrary> lib = [m_Device newLibraryWithData:IRMetalLibGetBytecodeData(libBin) error:&error];
            assert(lib);
            IRMetalLibBinaryDestroy(libBin);
            NSString* entrypoint = [NSString stringWithUTF8String:kIRRayDispatchIndirectionKernelName];
            dispatchSynthFn = [lib newFunctionWithName: entrypoint];
        }

        // Gathering all PSO linked functions
        std::vector<id<MTLFunction>> psoLinkedFunctions{compiledSMs.begin(), compiledSMs.end()};
        psoLinkedFunctions.emplace_back(synthTriangleIndirectIntersectionFn);
        psoLinkedFunctions.emplace_back(synthAABBIndirectIntersectionFn);
        NSArray *nsPSOLinkedFNs = [NSArray arrayWithObjects:psoLinkedFunctions.data() count:psoLinkedFunctions.size()];
        MTLLinkedFunctions* linkedFn = [[MTLLinkedFunctions alloc] init];
        [linkedFn setFunctions:nsPSOLinkedFNs];

        // Creating RT PSO
        MTLComputePipelineDescriptor* descriptor = [[MTLComputePipelineDescriptor alloc] init];
        [descriptor setComputeFunction:dispatchSynthFn];
        [descriptor setLinkedFunctions:linkedFn];

        //TODO: fix stack overflow and remove this statement.
        [descriptor setMaxCallStackDepth:20];
        if (desc.debugName) {
            [descriptor setLabel:[NSString stringWithUTF8String:desc.debugName]];
        }
        result->pso = [m_Device newComputePipelineStateWithDescriptor:descriptor options:0 reflection:nil error:&error];

        // Setup Intersection Function Table
        MTLIntersectionFunctionTableDescriptor* iftDesc = [[MTLIntersectionFunctionTableDescriptor alloc] init];
        [iftDesc setFunctionCount: IFT_TOTAL_FUNCTIONS_COUNT];
        result->ift = [result->pso newIntersectionFunctionTableWithDescriptor:iftDesc];
        if (desc.debugName) {
            std::string debugName = std::string{desc.debugName} + ".IFT";
            [result->ift setLabel:[NSString stringWithUTF8String:debugName.c_str()]];
        }
        [result->ift setFunction:[result->pso functionHandleWithFunction:synthTriangleIndirectIntersectionFn]
                         atIndex:IFT_SYNTH_TRIANGLE_INTERSECTION_IDX];
        [result->ift setFunction:[result->pso functionHandleWithFunction:synthAABBIndirectIntersectionFn]
                         atIndex:IFT_SYNTH_AABB_INTERSECTION_IDX];

        // Setup Visible Function Table
        MTLVisibleFunctionTableDescriptor* vftDesc = [[MTLVisibleFunctionTableDescriptor alloc] init];
        [vftDesc setFunctionCount: VFT_START_IDX + compiledSMs.size()];
        result->vft = [result->pso newVisibleFunctionTableWithDescriptor:vftDesc];
        if (desc.debugName) {
            std::string debugName = std::string{desc.debugName} + ".VFT";
            [result->vft setLabel:[NSString stringWithUTF8String:debugName.c_str()]];
        }
        for (size_t i = 0; i < compiledSMs.size(); ++i) {
            [result->vft setFunction:[result->pso functionHandleWithFunction:compiledSMs[i]] atIndex:i + VFT_START_IDX];
        }

        for (auto obj : smIRs) {
            IRObjectDestroy(obj);
        }
        IRCompilerDestroy(compiler);

        return result;
    }

    ComputePSO* MetalBackend::CompileComputePSO(const ComputePSODesc& desc) noexcept {
        auto* metalRootSignature = INTERPRET_AS<MetalRootSignature*>(desc.rootSignature);

        IRError* pError = nullptr;
        auto* result = new MetalComputePSO{};
        IRCompiler* compiler = IRCompilerCreate();

        IRObject* pDXIL = IRObjectCreateFromDXIL(desc.CS.bytecode, desc.CS.bytecodeSize, IRBytecodeOwnershipNone);

        IRCompilerSetGlobalRootSignature(compiler, metalRootSignature->rootSignature);
        IRCompilerSetEntryPointName(compiler, desc.CS.entrypoint);
        IRObject* outIR = IRCompilerAllocCompileAndLink(compiler, desc.CS.entrypoint, pDXIL, &pError);

        if (!outIR) {
            // Inspect pError to determine cause.
            IRErrorCode code = static_cast<IRErrorCode>(IRErrorGetCode(pError));

            const void* payload = IRErrorGetPayload(pError);
            assert(0);
            IRErrorDestroy(pError);
            return nullptr;
        }

        // Retrieve Metallib:
        IRMetalLibBinary* pMetallib = IRMetalLibBinaryCreate();
        IRObjectGetMetalLibBinary(outIR, IRShaderStageCompute, pMetallib);

        IRShaderReflection* reflection = IRShaderReflectionCreate();
        IRObjectGetReflection(outIR, IRShaderStageCompute, reflection);

        IRVersionedCSInfo csInfo{};
        IRShaderReflectionCopyComputeInfo(reflection, IRReflectionVersion_1_0, &csInfo);
        result->localWorkgroupSize[0] = csInfo.info_1_0.tg_size[0];
        result->localWorkgroupSize[1] = csInfo.info_1_0.tg_size[1];
        result->localWorkgroupSize[2] = csInfo.info_1_0.tg_size[2];

        IRShaderReflectionDestroy(reflection);
        IRObjectDestroy(pDXIL);
        IRObjectDestroy(outIR);

        NSError* error = nil;
        auto emptyHandler = ^{};

        dispatch_data_t dispatchData = IRMetalLibGetBytecodeData(pMetallib);

        id<MTLLibrary> lib = [m_Device newLibraryWithData:dispatchData error:&error];
        if (!lib || error) {
            assert(0);
            //TODO: show error.
            return nullptr;
        }

        NSString* functionName = [NSString stringWithUTF8String:desc.CS.entrypoint];
        id<MTLFunction> shaderModule = [lib newFunctionWithName:functionName];

        MTLComputePipelineDescriptor* descriptor = [[MTLComputePipelineDescriptor alloc] init];
        descriptor.computeFunction = shaderModule;
        result->pso = [m_Device newComputePipelineStateWithDescriptor:descriptor options:0 reflection:nil error:&error];

        IRMetalLibBinaryDestroy(pMetallib);
        IRCompilerDestroy(compiler);
        return result;
    }

    Buffer* MetalBackend::CreateBuffer(size_t size, ResourceHeapType heapType, ResourceUsage usage,
                                            size_t structuredStride, const char* name) noexcept {
        auto* result = new MetalBuffer{};
        result->buffer = [m_Device newBufferWithLength:size options:0];
        [result->buffer setLabel:[NSString stringWithUTF8String:name]];
        return result;
    }

    Texture2D* MetalBackend::CreateTexture2D(const Dim3D& dimensions, size_t mips, TextureFormat format,
                                             ResourceUsage usage, const char* name) noexcept {
        auto* result = new MetalTexture2D{};
        MTLTextureDescriptor* descriptor = [[MTLTextureDescriptor alloc] init];
        descriptor.arrayLength = 1;
        descriptor.mipmapLevelCount = mips;
        descriptor.width = dimensions.width;
        descriptor.height = dimensions.height;
        descriptor.depth = 1;
        descriptor.cpuCacheMode = MTLCPUCacheModeDefaultCache;
        descriptor.pixelFormat = Convert::ToMTLPixelFormat(format);
        descriptor.resourceOptions = 0;
        descriptor.sampleCount = 1;
        descriptor.textureType = MTLTextureType2DArray;
        descriptor.storageMode = MTLStorageModePrivate;
        descriptor.usage = Convert::ToMTLResourceUsage(usage);

        result->texture = [m_Device newTextureWithDescriptor:descriptor];

        [result->texture setLabel:[NSString stringWithUTF8String:name]];
        return result;
    }

    Sampler* MetalBackend::CreateSampler(const SamplerDesc& desc) noexcept {
        auto* result = new MetalSampler{};
        Convert::MetalMinMagMipFilters filters = Convert::ToMTLMinMagMipFilter(desc.textureFilter);
        MTLSamplerDescriptor* samplerDescriptor = [[MTLSamplerDescriptor alloc] init];
        samplerDescriptor.minFilter = filters.min;
        samplerDescriptor.magFilter = filters.mag;
        samplerDescriptor.mipFilter = filters.mip;
        samplerDescriptor.sAddressMode = Convert::ToMTLSamplerAddressMode(desc.addresU);
        samplerDescriptor.tAddressMode = Convert::ToMTLSamplerAddressMode(desc.addresV);
        samplerDescriptor.rAddressMode = Convert::ToMTLSamplerAddressMode(desc.addresW);
        samplerDescriptor.borderColor = Convert::ToMTLSamplerBorderColor(desc.borderColor);
        samplerDescriptor.compareFunction = Convert::ToMTLCompareFunction(desc.comparisonFunc);
        samplerDescriptor.maxAnisotropy = Convert::IsFilterAnisotrophic(desc.textureFilter) ? desc.maxAnisotropy : 1;
        samplerDescriptor.lodMinClamp = desc.minLOD;
        samplerDescriptor.lodMaxClamp = desc.maxLOD;
        samplerDescriptor.supportArgumentBuffers = true;
        samplerDescriptor.label = [NSString stringWithUTF8String:desc.name];

        result->sampler = [m_Device newSamplerStateWithDescriptor:samplerDescriptor];
        return result;
    }

    DescriptorHeap* MetalBackend::CreateDescriptorHeap(DescriptorHeapType type, size_t descriptorsCount,
                                                       const char* name) noexcept {
        auto* result = new MetalDescriptorHeap{};

        result->m_Resources.resize(descriptorsCount);
        result->m_DescriptorHeap = [m_Device newBufferWithLength:sizeof(IRDescriptorTableEntry) * descriptorsCount
                                                 options:MTLResourceStorageModeShared];
        [result->m_DescriptorHeap setLabel:[NSString stringWithUTF8String:name]];
        return result;
    }

    Swapchain* MetalBackend::CreateSwapchain(const SwapchainDesc& desc) noexcept {
        auto* result = new MetalSwapchain{};
        result->Initialize(m_Device, m_DefaultQueue, desc);
        return result;
    }

    CommandList* MetalBackend::AllocateCommandList(const char* name) noexcept {
        auto* result = new MetalCommandList{};
        result->Initialize(m_Device, m_DefaultQueue, name);
        return result;
    }

    void MetalBackend::SubmitCommandList(CommandList* cmd) noexcept {
        auto* metalCmd = INTERPRET_AS<MetalCommandList*>(cmd);
        metalCmd->SubmitToQueue();
    }

    void MetalBackend::SwapchainPresent(Swapchain* swapchain, Texture2D* toPresent, size_t width, size_t height) noexcept {
        auto* metalSwapchain = INTERPRET_AS<MetalSwapchain*>(swapchain);
        auto* metalTexture = INTERPRET_AS<MetalTexture2D*>(toPresent);
        metalSwapchain->Present(metalTexture, width, height);
    }

    void* MetalBackend::MapMemory(Buffer* buffer, size_t offset, size_t size) noexcept {
        auto* metalBuffer = INTERPRET_AS<MetalBuffer*>(buffer);
        return metalBuffer->buffer.contents;
    }

    void MetalBackend::UnmapMemory(Buffer* buffer) noexcept {
        // NOOP
    }

    ASPrebuildInfo MetalBackend::GetBLASPrebuildInfo(const BLASDesc& desc) noexcept {

        auto geometryDescriptors = [NSMutableArray array];
        if (desc.type == BLASPrimitiveType::Procedural) {
            auto aabbGeoDesc = [MTLAccelerationStructureBoundingBoxGeometryDescriptor descriptor];
            aabbGeoDesc.boundingBoxBuffer = nil;
            aabbGeoDesc.boundingBoxBufferOffset = desc.procedural.AABBsBufferOffset;
            aabbGeoDesc.boundingBoxCount = desc.procedural.AABBCount;
            aabbGeoDesc.boundingBoxStride = desc.procedural.AABBStrideInBytes;
            [geometryDescriptors addObject:aabbGeoDesc];
        } else {
            const auto& tDesc = desc.triangles;

            auto triangleGeoDesc = [MTLAccelerationStructureTriangleGeometryDescriptor descriptor];
            triangleGeoDesc.vertexBuffer = nil;
            triangleGeoDesc.vertexBufferOffset = 0;
            triangleGeoDesc.vertexFormat = Convert::ToMTLMTLAttributeFormat(tDesc.vertexFormat);
            triangleGeoDesc.vertexStride = tDesc.vertexStride;
            triangleGeoDesc.indexBuffer = nil;
            triangleGeoDesc.indexBufferOffset = 0;
            triangleGeoDesc.indexType = Convert::ToMTLIndexType(tDesc.indexFormat);
            triangleGeoDesc.triangleCount = tDesc.indexCount / 3;
            triangleGeoDesc.primitiveDataBuffer = nil;
            triangleGeoDesc.primitiveDataStride = 0;
            triangleGeoDesc.primitiveDataElementSize = 0;
            triangleGeoDesc.transformationMatrixBuffer = nil;
            triangleGeoDesc.transformationMatrixBufferOffset = 0;
            [geometryDescriptors addObject:triangleGeoDesc];
        }

        auto accelerationStructureDescriptor = [MTLPrimitiveAccelerationStructureDescriptor descriptor];

        accelerationStructureDescriptor.geometryDescriptors = geometryDescriptors;
        MTLAccelerationStructureSizes sizes = [m_Device accelerationStructureSizesWithDescriptor:accelerationStructureDescriptor];

        ASPrebuildInfo result{};
        result.MaxASSizeInBytes = sizes.accelerationStructureSize;
        result.scratchBufferSizeInBytes = sizes.buildScratchBufferSize;
        return result;
    }

    ASPrebuildInfo MetalBackend::GetTLASPrebuildInfo(const TLASDesc& desc) noexcept {
        MTLAccelerationStructureInstanceDescriptor descr{};
        MTLInstanceAccelerationStructureDescriptor* descriptor = [[MTLInstanceAccelerationStructureDescriptor alloc] init];
        [descriptor setInstanceCount:desc.blasInstancesCount];
        [descriptor setInstanceDescriptorBuffer:nil];
        [descriptor setInstanceDescriptorBufferOffset:0];
        [descriptor setInstanceDescriptorType:MTLAccelerationStructureInstanceDescriptorTypeDefault];
        [descriptor setInstanceDescriptorStride:sizeof(MTLAccelerationStructureInstanceDescriptor)];
        [descriptor setInstancedAccelerationStructures:nil];

        MTLAccelerationStructureSizes sizes = [m_Device accelerationStructureSizesWithDescriptor:descriptor];

        ASPrebuildInfo result{};
        result.MaxASSizeInBytes = sizes.accelerationStructureSize;
        result.scratchBufferSizeInBytes = sizes.buildScratchBufferSize;
        return result;
    }

    Semaphore* MetalBackend::CreateSyncSemaphore(uint64_t initialValue) noexcept {
        auto* result = new MetalSemaphore{};
        result->event = [m_Device newSharedEvent];
        [result->event setSignaledValue: initialValue];
        return result;
    }

    void MetalBackend::SignalFromQueue(Semaphore* semaphore, uint64_t value) noexcept {
        auto* metalSemaphore = INTERPRET_AS<MetalSemaphore*>(semaphore);
        id<MTLCommandBuffer> cmd = [m_DefaultQueue commandBuffer];
        [cmd encodeSignalEvent: metalSemaphore->event value: value];
        [cmd commit];
    }

    void MetalBackend::SignalFromHost(Semaphore* semaphore, uint64_t value) noexcept {
        auto* metalSemaphore = INTERPRET_AS<MetalSemaphore*>(semaphore);
        [metalSemaphore->event setSignaledValue: value];
    }

    bool MetalBackend::SemaphoreWaitFromHost(const Semaphore* semaphore, uint64_t value, size_t timeout) noexcept {
        const auto* metalSemaphore = INTERPRET_AS<const MetalSemaphore*>(semaphore);
        return WaitForMTLSharedEventValue(metalSemaphore->event, value, timeout);
    }

    void MetalBackend::SemaphoreWaitFromQueue(const Semaphore* semaphore, uint64_t value) noexcept {
        const auto* metalSemaphore = INTERPRET_AS<const MetalSemaphore*>(semaphore);
        id<MTLCommandBuffer> cmd = [m_DefaultQueue commandBuffer];
        [cmd encodeWaitForEvent: metalSemaphore->event value: value];
        [cmd commit];
    }

    uint64_t MetalBackend::GetSemaphoreCompletedValue(const Semaphore* semaphore) noexcept {
        const auto* metalSemaphore = INTERPRET_AS<const MetalSemaphore*>(semaphore);
        [metalSemaphore->event signaledValue];
        return true;
    }
} // namespace RHINO::APIMetal

#endif // ENABLE_API_METAL
