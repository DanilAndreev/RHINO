#ifdef __clang__

#include "DxilToMetallibStep.h"

#include <metal_irconverter/metal_irconverter.h>

namespace SCAR {
    static IRRootSignature* CreateIRRootSignature(size_t spacesCount,
                                                  const RHINO::DescriptorSpaceDesc* spaces) noexcept {
        IRError* pError = nullptr;

        // TODO: if root constants defined: add root constants root param

        std::vector<IRDescriptorRange1> rangeDescsStorage{};
        std::vector<IRRootParameter1> rootParamsDescs{};
        rootParamsDescs.reserve(spacesCount + 1);
        std::vector<size_t> offsetsInRangeDescsPerSpaceIdx{};
        offsetsInRangeDescsPerSpaceIdx.resize(desc.spacesCount);

        for (size_t spaceIdx = 0; spaceIdx < spacesCount; ++spaceIdx) {
            IRRootParameter1& rootParamDesc = rootParamsDescs.emplace_back();
            rootParamDesc.ParameterType = IRRootParameterTypeDescriptorTable;
            rootParamDesc.DescriptorTable.NumDescriptorRanges = spaces[spaceIdx].rangeDescCount;
            offsetsInRangeDescsPerSpaceIdx[spaceIdx] = rangeDescsStorage.size();

            //TODO: assert that ranges in one space exclusively CBVSRVUAV or SMP.

            for (size_t i = 0; i < spaces[spaceIdx].rangeDescCount; ++i) {
                IRDescriptorRange1& rangeDesc = rangeDescsStorage.emplace_back();
                rangeDesc.NumDescriptors = spaces[spaceIdx].rangeDescs[i].descriptorsCount;
                rangeDesc.RegisterSpace = spaces[spaceIdx].space;
                rangeDesc.OffsetInDescriptorsFromTableStart = spaces[spaceIdx].offsetInDescriptorsFromTableStart +
                                                              spaces[spaceIdx].rangeDescs[i].baseRegisterSlot;
                rangeDesc.BaseShaderRegister = spaces[spaceIdx].rangeDescs[i].baseRegisterSlot;
                switch (spaces[spaceIdx].rangeDescs[i].rangeType) {
                    case RHINO::DescriptorRangeType::SRV:
                        rangeDesc.RangeType = IRDescriptorRangeTypeSRV;
                        break;
                    case RHINO::DescriptorRangeType::UAV:
                        rangeDesc.RangeType = IRDescriptorRangeTypeUAV;
                        break;
                    case RHINO::DescriptorRangeType::CBV:
                        rangeDesc.RangeType = IRDescriptorRangeTypeCBV;
                        break;
                    case RHINO::DescriptorRangeType::Sampler:
                        rangeDesc.RangeType = IRDescriptorRangeTypeSampler;
                        break;
                }
            }
        }
        for (size_t spaceIdx = 0; spaceIdx < spacesCount; ++spaceIdx) {
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
        auto result = IRRootSignatureCreateFromDescriptor(&rsVersionedDesc, &pError);
        assert(!pError);
        std::cout << IRVersionedRootSignatureDescriptorAllocStringAndSerialize(&rsVersionedDesc) << std::endl;
        return result;
    }

    bool DxilToMetallibStep::Execute(const CompileSettings& settings, const ChainSettings& chSettings,
                                     ChainContext& context) noexcept {
        IRShaderStage shaderStage = {};
        switch (chSettings.stage) {
            case ChainStageTarget::Vertex:
                shaderStage = IRShaderStageVertex;
                break;
            case ChainStageTarget::Pixel:
                shaderStage = IRShaderStageFragment;
                break;
            case ChainStageTarget::Geometry:
                shaderStage = IRShaderStageGeometry;
                break;
            case ChainStageTarget::Compute:
                shaderStage = IRShaderStageCompute;
                break;
        }


        IRError* pError = nullptr;
        IRCompiler* pCompiler = IRCompilerCreate();
        IRCompilerSetEntryPointName(pCompiler, "main");

        const PSORootSignatureDesc* rsDesc = settings.rootSignature;
        IRRootSignature* rootSignature = CreateIRRootSignature(rsDesc->spacesCount, rsDesc->spacesDescs);
        IRCompilerSetGlobalRootSignature(pCompiler, rootSignature);

        IRObject* pDXIL = IRObjectCreateFromDXIL(context.data.get(), context.dataLength, IRBytecodeOwnershipNone);

        // Compile DXIL to Metal IR:
        IRObject* pOutIR = IRCompilerAllocCompileAndLink(pCompiler, NULL, pDXIL, &pError);

        if (!pOutIR) {
            // Inspect pError to determine cause.
            IRErrorDestroy(pError);
        }

        // Retrieve Metallib:
        IRMetalLibBinary* pMetallib = IRMetalLibBinaryCreate();
        IRObjectGetMetalLibBinary(pOutIR, shaderStage, pMetallib);

        const auto rsResCount = IRRootSignatureGetResourceCount(rootSignature);
        std::vector<IRResourceLocation> rsLocs{rsResCount};
        IRRootSignatureGetResourceLocations(rootSignature, rsLocs.data());

        context.dataLength = IRMetalLibGetBytecodeSize(pMetallib);
        context.data.reset(new uint8_t[context.dataLength]);
        IRMetalLibGetBytecode(pMetallib, context.data.get());

        IRRootSignatureDestroy(rootSignature);
        IRMetalLibBinaryDestroy(pMetallib);
        IRObjectDestroy(pDXIL);
        IRObjectDestroy(pOutIR);
        IRCompilerDestroy(pCompiler);
        return true;
    }
} // namespace SCAR

#endif // __clang__
