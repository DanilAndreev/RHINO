#include "LibILCompilationPipeline.h"

#include "archiver/PSOArchiver.h"
#include "serializers/SerializeRootSignature.h"

namespace SCAR {
    LibILCompilationPipeline::LibILCompilationPipeline(CompilationChain* chain) noexcept :
        m_CompilationChain(chain) {}

    ArchiveBinary LibILCompilationPipeline::Execute(const CompileSettings& settings, std::vector<std::string>& errors,
                                                        std::vector<std::string>& warnings) noexcept {
        assert(m_CompilationChain != nullptr);

        ChainSettings chSettings{};
        chSettings.shaderFilepath = settings.libratySettings.inputFilepath;
        chSettings.entrypoint = std::nullopt;
        chSettings.stage = ChainStageTarget::Lib;

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
        archiver.AddRecord(RecordType::LibAssembly, RecordFlags::None, shaderModuleAssembly, context.dataLength);
        delete shaderModuleAssembly;
        archiver.AddRecord(RecordType::RootSignature, RecordFlags::None,
                           SerializeRootSignature(*settings.rootSignature));
        return archiver.Archive();
    }
} // namespace SCAR
