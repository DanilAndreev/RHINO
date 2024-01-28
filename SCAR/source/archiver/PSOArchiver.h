#pragma once

namespace SCAR {
    class PSOArchiver {
    public:
        std::unique_ptr<uint8_t> Archive() noexcept;
    };
}// namespace SCAR
