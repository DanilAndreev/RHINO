#pragma once

#include <RHINO.h>
#include <cstdint>
#include "SCARUnarchiver.h"

namespace SCAR {
    struct ArchiveBinary {
        void* data = nullptr;
        uint32_t archiveSizeInBytes = 0;
    };

    struct CompilationResult {
        ArchiveBinary archive;
        bool success = false;
        const char* warningsStr = nullptr;
        const char* errorsStr = nullptr;
    };

    struct GraphicsCompileSettings {
        const char* vertexShaderInputFilepath = nullptr;
        const char* vertexShaderEntrypoint = nullptr;
        const char* pixelShaderInputFilepath = nullptr;
        const char* pixelShaderEntrypoint = nullptr;
    };

    struct ComputeCompileSettings {
        const char* inputFilepath = nullptr;
        const char* entrypoint = nullptr;
    };

    struct LibraryCompileSettings {
        const char* inputFilepath = nullptr;
        size_t entrypointsCount = 0;
        const char** entrypoints = nullptr;

        uint32_t maxPayloadSizeInBytes = 0;
        uint32_t maxAttributeSizeInBytes = 0;
    };

    struct CompileSettings {
        size_t optimizationLevel = 3;
        ArchivePSOLang psoLang = {};
        ArchivePSOType psoType = {};
        size_t globalDefinesCount = 0;
        const char** globalDefines = nullptr;
        size_t includeDirectoriesCount = 0;
        const char** includeDirectories = nullptr;

        union {
            GraphicsCompileSettings graphicsSetting;
            ComputeCompileSettings computeSettings;
            LibraryCompileSettings librarySettings;
        };

    };

    CompilationResult Compile(const CompileSettings* settings) noexcept;
    void ReleaseCompilationResult(CompilationResult result) noexcept;
} // namespace SCAR
