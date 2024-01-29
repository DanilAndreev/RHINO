#pragma once

namespace SCAR {
    class ICompilationPipeline {
    public:
        virtual ~ICompilationPipeline() = default;
        virtual ArchiveBinary Execute(const CompileSettings& settings) noexcept = 0;
    };
} // namespace SCAR
