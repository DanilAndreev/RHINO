#include "PSOArchiver.h"

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

    PSOArchiver::PSOArchiver(ArchivePSOType type, ArchivePSOLang lang) noexcept : m_PSOInfo(type, lang) {}

    void PSOArchiver::AddRecord(RecordType type, RecordFlags flags, const void* data,
                                uint32_t dataSize) noexcept {
        if (m_Table.contains(type)) {
            assert(0);
        }
        const uint32_t offset = m_Storage.size();
        m_Table[type] = SerializedRecord{type, flags, offset, dataSize};

        m_Storage.resize(m_Storage.size() + dataSize);
        void* cursor = m_Storage.data() + offset;
        memcpy(cursor, data, dataSize);
    }

    ArchiveBinary PSOArchiver::Archive() noexcept {
        const uint32_t binarySize = CalculateArchiveBinarySize();
        auto result = std::make_unique<uint8_t[]>(binarySize);
        uint8_t* cursor = result.get();

        // ----------------- HEADER ------------------
        constexpr SerializedVersion version{SCAR_VERSION_MAJOR, SCAR_VERSION_MINOR, SCAR_VERSION_PATCH};
        cursor += WriteItem(cursor, version);
        cursor += WriteItem(cursor, m_PSOInfo);
        cursor += WriteItem(cursor, binarySize);

        assert(cursor == result.get() + ARCHIVE_HEADER_SIZE);

        // ------------------ TABLE ------------------
        for (const auto& record: m_Table | std::views::values) {
            SerializedRecord resolvedRecord = record;
            resolvedRecord.dataStartOffset += ARCHIVE_HEADER_SIZE + CalculateArchiveTableSize();
            cursor += WriteItem(cursor, resolvedRecord);
        }
        auto tableTrail = static_cast<uint32_t>(RecordType::TableEnd);
        cursor += WriteItem(cursor, tableTrail);
        assert(cursor == result.get() + ARCHIVE_HEADER_SIZE + CalculateArchiveTableSize());

        // ----------------- STORAGE -----------------
        cursor += WriteChunk(cursor, m_Storage.data(), m_Storage.size());

        // ----------------- FOOTER ------------------
        cursor += WriteItem(cursor, binarySize);

        assert(cursor == result.get() + binarySize);
        return {result.release(), binarySize};
    }

    uint32_t PSOArchiver::CalculateArchiveTableSize() const noexcept {
        return m_Table.size() * sizeof(SerializedRecord) + sizeof(uint32_t);
    }

    uint32_t PSOArchiver::CalculateArchiveBinarySize() const noexcept {
        return ARCHIVE_HEADER_SIZE + CalculateArchiveTableSize() + m_Storage.size() + ARCHIVE_FOOTER_SIZE;
    }
} // namespace SCAR
