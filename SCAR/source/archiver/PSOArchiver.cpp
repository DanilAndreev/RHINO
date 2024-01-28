#include "PSOArchiver.h"
#include "SCARVersion.h"

namespace SCAR {
    struct SerializedVersion {
        uint8_t major = 0;
        uint8_t minor = 0;
        uint8_t patch = 0;
        uint8_t _empty = 0;
    };

    std::unique_ptr<uint8_t> PSOArchiver::Archive() noexcept {


        uint32_t binarySize = 0;
        auto result = std::make_unique<uint8_t[]>(binarySize);
        uint8_t* cursor = result.get();

        constexpr SerializedVersion version{SCAR_VERSION_MAJOR, SCAR_VERSION_MINOR, SCAR_VERSION_PATCH};
        memcpy(cursor, &version, sizeof(version));
        cursor += sizeof(version);

        memcpy(cursor, &binarySize, sizeof(binarySize));
        cursor += binarySize;



        memcpy(cursor, &binarySize, sizeof(binarySize));
        cursor += binarySize;

        if (result.get() + binarySize != cursor) {
            // TODO: ERROR;
        }
        return result;
    }
}// namespace SCAR
