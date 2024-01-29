#pragma once

#include <cstdint>
#include "SCARUnarchiver.h"

namespace SCAR {
    struct ArchiveBinary {
        void* data;
        uint32_t archiveSizeInBytes;
    };

    struct CompileSettings {
        size_t optimizationLevel = 3;
        ArchivePSOLang psoLang = {};
        ArchivePSOType psoType = {};
        size_t globalDefinesCount = 0;
        const char** globalDefines = nullptr;
        size_t includeDirectoriesCount = 0;
        const char** includeDirectories = nullptr;
    };

    ArchiveBinary Compile(const CompileSettings* settings) noexcept;
}
