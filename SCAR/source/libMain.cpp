#include <SCAR.h>
#include <archiver/PSOArchiver.h>
#include <random>

#include "CompilationChain.h"
#include "ICompilationPipeline.h"
#include "pipelines/ILComputeCompilationPipeline.h"
#include "steps/DxilToMetallibStep.h"
#include "steps/HlslToDxilStep.h"
#include "steps/HlslToSpirvStep.h"

namespace SCAR {
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
                chainHead->SetNext(new DxilToMetallibStep{});
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
                return new ILComputeCompilationPipeline{chain};
            case ArchivePSOType::Graphics:
                return nullptr;
            default:
                return nullptr;
        }
    }

    ArchiveBinary Compile(const CompileSettings* settings) noexcept {
        if (!settings)
            return {nullptr, 0};

        CompilationChain* chain = ConstructCompilationChain(settings->psoLang);
        if (!chain) {
            //TODO: error: unsupported lang.
            return {nullptr, 0};
        }
        ICompilationPipeline* pipeline = ConstructCompilationPipeline(chain, settings->psoType);

        ArchiveBinary result = pipeline->Execute(*settings);

        delete pipeline;
        delete chain;

        return result;
    }
} // namespace SCAR
