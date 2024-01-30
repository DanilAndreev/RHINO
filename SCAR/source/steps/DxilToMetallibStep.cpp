#include "DxilToMetallibStep.h"

#include <metal_irconverter/metal_irconverter.h>

namespace SCAR {
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


        IRCompiler* pCompiler = IRCompilerCreate();
        IRCompilerSetEntryPointName(pCompiler, "main");

        IRObject* pDXIL = IRObjectCreateFromDXIL(context.data.get(), context.dataLength, IRBytecodeOwnershipNone);

        // Compile DXIL to Metal IR:
        IRError* pError = nullptr;
        IRObject* pOutIR = IRCompilerAllocCompileAndLink(pCompiler, NULL, pDXIL, &pError);

        if (!pOutIR) {
            // Inspect pError to determine cause.
            IRErrorDestroy(pError);
        }

        // Retrieve Metallib:
        IRMetalLibBinary* pMetallib = IRMetalLibBinaryCreate();
        IRObjectGetMetalLibBinary(pOutIR, shaderStage, pMetallib);

        context.dataLength = IRMetalLibGetBytecodeSize(pMetallib);
        context.data.reset(new uint8_t[context.dataLength]);
        IRMetalLibGetBytecode(pMetallib, context.data.get());

        IRMetalLibBinaryDestroy(pMetallib);
        IRObjectDestroy(pDXIL);
        IRObjectDestroy(pOutIR);
        IRCompilerDestroy(pCompiler);
    }
} // namespace SCAR
