#include "ILComputeCompilationPipeline.h"

#include "archiver/PSOArchiver.h"
#include "serializers/SerializeRootSignature.h"

namespace SCAR {
    ILComputeCompilationPipeline::ILComputeCompilationPipeline(CompilationChain* chain) noexcept :
        m_CompilationChain(chain) {}

    ArchiveBinary ILComputeCompilationPipeline::Execute(const CompileSettings& settings) noexcept {
        assert(m_CompilationChain != nullptr);

        ChainSettings chSettings{};
        chSettings.shaderFilepath = "layouts.compute.hlsl";
        chSettings.entrypoint = "main";
        chSettings.stage = ChainStageTarget::Compute;

        ChainContext context{};
        bool status = m_CompilationChain->Run(settings, chSettings, context);
        uint8_t* shaderModuleAssembly = context.data.release();
        if (!status) {
            delete shaderModuleAssembly;
            return {nullptr, 0};
        }

        PSOArchiver archiver{settings.psoType, settings.psoLang};
        archiver.AddRecord(RecordType::CSAssembly, RecordFlags::None, shaderModuleAssembly, context.dataLength);
        delete shaderModuleAssembly;
        archiver.AddRecord(RecordType::RootSignature, RecordFlags::None,
                           SerializeRootSignature(*settings.rootSignature));
        return archiver.Archive();
    }
} // namespace SCAR
