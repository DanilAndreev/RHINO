#pragma once

#include <RHINO.h>
#include <cstdint>
#include "SCARUnarchiver.h"

namespace SCAR {
    struct ArchiveBinary {
        void* data = nullptr;
        uint32_t archiveSizeInBytes = 0;
    };

    struct PSORootSignatureDesc {
        size_t spacesCount = 0;
        const RHINO::DescriptorSpaceDesc* spacesDescs = nullptr;
    };

    struct CompileSettings {
        size_t optimizationLevel = 3;
        ArchivePSOLang psoLang = {};
        ArchivePSOType psoType = {};
        size_t globalDefinesCount = 0;
        const char** globalDefines = nullptr;
        size_t includeDirectoriesCount = 0;
        const char** includeDirectories = nullptr;

        PSORootSignatureDesc* rootSignature = nullptr;
    };

    ArchiveBinary Compile(const CompileSettings* settings) noexcept;
} // namespace SCAR
