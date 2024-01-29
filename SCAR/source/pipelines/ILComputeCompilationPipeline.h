#pragma once

#include "CompilationChain.h"
#include "ICompilationPipeline.h"

namespace SCAR {
    class ILComputeCompilationPipeline final : public ICompilationPipeline {
    public:
        explicit ILComputeCompilationPipeline(CompilationChain* chain) noexcept;

    public:
        ArchiveBinary Execute(const CompileSettings& settings) noexcept final;

    private:
        CompilationChain* m_CompilationChain = nullptr;
    };
} // namespace SCAR
