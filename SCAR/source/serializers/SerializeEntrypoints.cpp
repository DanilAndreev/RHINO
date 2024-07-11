#include "Serializers.h"

namespace SCAR {
    std::vector<uint8_t> SerializeEntrypoints(const std::vector<std::string>& entrypoints) noexcept {
        std::vector<uint8_t> result{};
        for (const auto& entry : entrypoints) {
            size_t start = result.size();
            size_t size = entry.length();
            result.resize(start + size + 1);
            memcpy(result.data() + start, entry.c_str(), size * sizeof(char));
            result[start + size] = '\0';
        }
        return result;
    }
}