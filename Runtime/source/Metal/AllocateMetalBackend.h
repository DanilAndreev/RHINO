#pragma once

#ifdef ENABLE_API_METAL

namespace RHINO::APIMetal {
    RHINOInterface* AllocateMetalBackend() noexcept;
}

#endif // ENABLE_API_METAL
