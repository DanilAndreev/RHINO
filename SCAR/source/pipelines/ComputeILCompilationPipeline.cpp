#include "ComputeILCompilationPipeline.h"

#include "archiver/PSOArchiver.h"
#include "serializers/Serializers.h"

namespace SCAR {
    ComputeILCompilationPipeline::ComputeILCompilationPipeline(CompilationChain* chain) noexcept :
        m_CompilationChain(chain) {}

    ArchiveBinary ComputeILCompilationPipeline::Execute(const CompileSettings& settings, std::vector<std::string>& errors,
                                                        std::vector<std::string>& warnings) noexcept {
        assert(m_CompilationChain != nullptr);

        ChainSettings chSettings{};
        chSettings.shaderFilepath = settings.computeSettings.inputFilepath;
        chSettings.entrypoint = settings.computeSettings.entrypoint;
        chSettings.stage = ChainStageTarget::Compute;

        ChainContext context{};
        bool status = m_CompilationChain->Run(settings, chSettings, context);
        uint8_t* shaderModuleAssembly = context.data.release();

        errors.insert(errors.begin(), context.errors.cbegin(), context.errors.cend());
        warnings.insert(warnings.begin(), context.warnings.cbegin(), context.warnings.cend());

        if (!status) {
            delete shaderModuleAssembly;
            return {nullptr, 0};
        }

        PSOArchiver archiver{settings.psoType, settings.psoLang};
        archiver.AddRecord(RecordType::CSAssembly, RecordFlags::None, shaderModuleAssembly, context.dataLength);
        delete shaderModuleAssembly;
        archiver.AddRecord(RecordType::RootSignature, RecordFlags::None,
                           SerializeRootSignature(*settings.rootSignature));
        archiver.AddRecord(RecordType::ShadersEntrypoints, RecordFlags::None, SerializeEntrypoints({settings.computeSettings.entrypoint}));
        return archiver.Archive();
    }
} // namespace SCAR
