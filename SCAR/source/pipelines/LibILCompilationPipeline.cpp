#include "LibILCompilationPipeline.h"

#include "archiver/PSOArchiver.h"
#include "serializers/Serializers.h"

namespace SCAR {
    LibILCompilationPipeline::LibILCompilationPipeline(CompilationChain* chain) noexcept :
        m_CompilationChain(chain) {}

    ArchiveBinary LibILCompilationPipeline::Execute(const CompileSettings& settings, std::vector<std::string>& errors,
                                                        std::vector<std::string>& warnings) noexcept {
        assert(m_CompilationChain != nullptr);

        LibraryCompileSettings librarySettings = settings.librarySettings;

        ChainSettings chSettings{};
        chSettings.shaderFilepath = librarySettings.inputFilepath;
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
        std::vector<std::string> entrypoints{librarySettings.entrypoints, librarySettings.entrypoints + librarySettings.entrypointsCount};
        archiver.AddRecord(RecordType::ShadersEntrypoints, RecordFlags::MultipleValues, SerializeEntrypoints(entrypoints));
        return archiver.Archive();
    }
} // namespace SCAR
