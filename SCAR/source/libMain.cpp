#include <SCAR.h>
#include <archiver/PSOArchiver.h>
#include <random>

#include "CompilationChain.h"
#include "ICompilationPipeline.h"
#include "pipelines/LibILCompilationPipeline.h"
#include "pipelines/ComputeILCompilationPipeline.h"
#include "steps/HlslToDxilStep.h"
#include "steps/HlslToSpirvStep.h"

namespace SCAR {
    inline const char* MergeErrorsAndAllocateStr(const std::vector<std::string>& input, const std::string& delimiter = "\n") noexcept {
        size_t totalSize = 0;
        for (const auto& item : input) {
            totalSize += item.length() + delimiter.size();
        }
        totalSize -= delimiter.length();
        char* result = new char[totalSize + 1];

        char* cursor = result;
        for (size_t i = 0; i < input.size(); ++i) {
            memcpy(cursor, input[i].data(), input[i].length());
            cursor += input[i].length();
            if (i < input.size() - 1) {
                memcpy(cursor, delimiter.data(), delimiter.length());
                cursor += input[i].length();
            }
        }
        result[totalSize] = '\0';
        return result;
    }

    CompilationChain* ConstructCompilationChain(ArchivePSOLang lang) noexcept {
        switch (lang) {
            case ArchivePSOLang::SPIRV: {
                return new HlslToSpirvStep{};
            }
            case ArchivePSOLang::DXIL: {
                return new HlslToDxilStep{};
            }
#ifdef __clang__
            case ArchivePSOLang::MetalLib: {
                auto* chainHead = new HlslToDxilStep{};
                return chainHead;
            }
#endif // __clang__
            default:
                return nullptr;
        }
    }

    ICompilationPipeline* ConstructCompilationPipeline(CompilationChain* chain, ArchivePSOType psoType) {
        assert(chain);
        switch (psoType) {
            case ArchivePSOType::Compute:
                return new ComputeILCompilationPipeline{chain};
            case ArchivePSOType::Library:
                return new LibILCompilationPipeline{chain};
            case ArchivePSOType::Graphics:
                return nullptr;
            default:
                return nullptr;
        }
    }

    CompilationResult Compile(const CompileSettings* settings) noexcept {
        if (!settings)
            return {nullptr, 0};

        CompilationChain* chain = ConstructCompilationChain(settings->psoLang);
        if (!chain) {
            //TODO: error: unsupported lang.
            return {nullptr, 0};
        }
        ICompilationPipeline* pipeline = ConstructCompilationPipeline(chain, settings->psoType);

        std::vector<std::string> errors;
        std::vector<std::string> warnings;
        ArchiveBinary archive = pipeline->Execute(*settings, errors, warnings);

        delete pipeline;
        delete chain;

        CompilationResult result{};
        result.success = static_cast<bool>(archive.archiveSizeInBytes);
        result.archive = archive;
        result.warningsStr = errors.empty() ? nullptr : MergeErrorsAndAllocateStr(warnings);
        result.errorsStr = errors.empty() ? nullptr : MergeErrorsAndAllocateStr(errors);

        return result;
    }

    void ReleaseCompilationResult(CompilationResult result) noexcept {
        free(result.archive.data);
        delete result.errorsStr;
        delete result.warningsStr;
    }
} // namespace SCAR
