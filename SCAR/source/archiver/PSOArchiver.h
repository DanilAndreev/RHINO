#pragma once

namespace SCAR {
    struct SerializedVersion {
        uint8_t major = 0;
        uint8_t minor = 0;
        uint8_t patch = 0;
        uint8_t _empty = 0;
    };

    namespace ArchiveTable {
        enum class Flags : uint16_t {
            None = 0x0,
        };

        enum class RecordType : uint16_t {
            TableEnd = 0x0,
            PSOAssembly,
            VSAssembly,
            PSAssembly,
        };

        struct Record {
            RecordType type;
            Flags flags;

            uint32_t dataStartOffset;
            uint32_t dataSize;
        };
    } // namespace ArchiveTable

    class PSOArchiver {
        //                                              VERSION                BINARY SIZE
        static constexpr uint32_t ARCHIVE_HEADER_SIZE = sizeof(SerializedVersion) + sizeof(uint32_t);
        //                                         BINARY SIZE
        static constexpr uint32_t ARCHIVE_FOOTER_SIZE = sizeof(uint32_t);
    public:
        void AddRecord(ArchiveTable::RecordType t, ArchiveTable::Flags flags, const void* data, uint32_t dataSize) noexcept;
        template<class T>
        void AddRecord(ArchiveTable::RecordType t, ArchiveTable::Flags flags, const std::vector<T>& data) noexcept {
            AddRecord(t, flags, data.data(), data.size() * sizeof(T));
        }
        ArchiveBinary Archive() noexcept;
    private:
        [[nodiscard]] uint32_t CalculateArchiveTableSize() const noexcept;
        [[nodiscard]] uint32_t CalculateArchiveBinarySize() const noexcept;
    private:
        std::map<ArchiveTable::RecordType, ArchiveTable::Record> m_Table{};
        std::vector<char> m_Storage{};
    };
} // namespace SCAR
