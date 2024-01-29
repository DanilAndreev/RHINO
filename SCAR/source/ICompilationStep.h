#pragma once

namespace SCAR {
    struct ChainContext {
        std::vector<std::string> warnings;
        std::vector<std::string> errors;

        size_t dataLength = 0;
        std::unique_ptr<uint8_t> data;
    };

    enum class ChainStageTarget {
        Vertex,
        Pixel,
        Geometry,

        Compute,
    };

    struct ChainSettings {
        ChainStageTarget stage;
        std::filesystem::path shaderFilepath;
        std::string entrypoint;
        std::set<std::string> defines;
    };

    class ICompilationStep {
    public:
        virtual ~ICompilationStep() noexcept = default;

    public:
        virtual bool Execute(const CompileSettings& settings, const ChainSettings& chSettings,
                             ChainContext& context) noexcept = 0;
    };
} // namespace SCAR
