#include "PSOArchiver.h"


#include <ranges>

namespace SCAR {
    /**
     * \brief Writes typed data item to address.
     * \tparam T - Item type.
     * \param[in] cursor Memory cursor with destination write address.
     * \param[in] data Typed data item to write.
     * \return Number of bytes written.
     */
    template<class T>
    SCAR_FORCEINLINE size_t WriteItem(uint8_t* cursor, const T& data) noexcept {
        memcpy(cursor, &data, sizeof(T));
        return sizeof(data);
    }

    SCAR_FORCEINLINE size_t WriteChunk(uint8_t* cursor, void* data, size_t sizeInBytes) noexcept {
        memcpy(cursor, data, sizeInBytes);
        return sizeInBytes;
    }

    void PSOArchiver::AddRecord(ArchiveTable::RecordType type, ArchiveTable::Flags flags, const void* data,
                                uint32_t dataSize) noexcept {
        if (m_Table.contains(type)) {
            assert(0);
        }
        const uint32_t offset = m_Storage.size();
        m_Table[type] = ArchiveTable::Record{type, flags, offset, dataSize};

        m_Storage.resize(m_Storage.size() + dataSize);
        void* cursor = m_Storage.data() + offset;
        memcpy(cursor, data, dataSize);
    }

    ArchiveBinary PSOArchiver::Archive() noexcept {
        const uint32_t binarySize = CalculateArchiveBinarySize();
        auto result = std::make_unique<uint8_t[]>(binarySize);
        uint8_t* cursor = result.get();

        constexpr SerializedVersion version{SCAR_VERSION_MAJOR, SCAR_VERSION_MINOR, SCAR_VERSION_PATCH};
        cursor += WriteItem(cursor, version);
        cursor += WriteItem(cursor, binarySize);

        assert(cursor == result.get() + ARCHIVE_HEADER_SIZE);

        for (const auto& record: m_Table | std::views::values) {
            ArchiveTable::Record resolvedRecord = record;
            resolvedRecord.dataStartOffset += ARCHIVE_HEADER_SIZE + CalculateArchiveTableSize();
            cursor += WriteItem(cursor, resolvedRecord);
        }
        auto tableTrail = static_cast<uint32_t>(ArchiveTable::RecordType::TableEnd);
        cursor += WriteItem(cursor, tableTrail);
        assert(cursor == result.get() + ARCHIVE_HEADER_SIZE + CalculateArchiveTableSize());

        cursor += WriteChunk(cursor, m_Storage.data(), m_Storage.size());

        cursor += WriteItem(cursor, binarySize);

        assert(cursor == result.get() + binarySize);
        return {result.release(), binarySize};
    }

    uint32_t PSOArchiver::CalculateArchiveTableSize() const noexcept {
        return m_Table.size() * sizeof(ArchiveTable::Record) + sizeof(uint32_t);
    }

    uint32_t PSOArchiver::CalculateArchiveBinarySize() const noexcept {
        return ARCHIVE_HEADER_SIZE + CalculateArchiveTableSize() + m_Storage.size() + ARCHIVE_FOOTER_SIZE;
    }
} // namespace SCAR
