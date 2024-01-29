#pragma once

#include <cstdint>

namespace SCAR {
    struct ArchiveBinary {
        void* data;
        uint32_t archiveSizeInBytes;
    };

    ArchiveBinary Compile() noexcept;
}
