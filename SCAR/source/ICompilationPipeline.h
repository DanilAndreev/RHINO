#pragma once

namespace SCAR {
    class ICompilationPipeline {
    public:
        virtual ~ICompilationPipeline() = default;
        virtual ArchiveBinary Execute(const CompileSettings& settings, std::vector<std::string>& errors,
                                      std::vector<std::string>& warnings) noexcept = 0;
    };
} // namespace SCAR
