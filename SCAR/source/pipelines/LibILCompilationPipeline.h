#pragma once

#include "CompilationChain.h"
#include "ICompilationPipeline.h"

namespace SCAR {
    class LibILCompilationPipeline final : public ICompilationPipeline {
    public:
        explicit LibILCompilationPipeline(CompilationChain* chain) noexcept;

    public:
        ArchiveBinary Execute(const CompileSettings& settings, std::vector<std::string>& errors,
                              std::vector<std::string>& warnings) noexcept final;

    private:
        CompilationChain* m_CompilationChain = nullptr;
    };
} // namespace SCAR
