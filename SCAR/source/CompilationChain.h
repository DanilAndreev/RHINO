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
        Lib,
    };

    struct ChainSettings {
        ChainStageTarget stage;
        std::filesystem::path shaderFilepath;
        std::optional<std::string> entrypoint;
        std::set<std::string> defines;
    };

    class CompilationChain {
    public:
        virtual ~CompilationChain() noexcept;

    public:
        bool Run(const CompileSettings& settings, const ChainSettings& chSettings, ChainContext& context) noexcept;
        CompilationChain* SetNext(CompilationChain* next) noexcept;
        [[nodiscard]] CompilationChain* GetNext() const noexcept;

    public:
        virtual bool Execute(const CompileSettings& settings, const ChainSettings& chSettings,
                             ChainContext& context) noexcept = 0;
    private:
        CompilationChain* m_Next = nullptr;
    };
} // namespace SCAR
