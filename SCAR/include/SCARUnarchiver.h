#pragma once

#include <cstdint>
#include <map>
#include <cassert>

namespace SCAR {
    enum class ArchivePSOType : uint16_t {
        Compute,
        Graphics,
    };

    enum class ArchivePSOLang : uint16_t {
        SPIRV = 0x1,
        DXIL = 0x2,
        MetalLib = 0x3,
    };

    struct SerializedVersion {
        uint8_t major = 0;
        uint8_t minor = 0;
        uint8_t patch = 0;
        uint8_t _empty = 0;
    };

    enum class RecordFlags : uint16_t {
        None = 0x0,
    };

    enum class RecordType : uint16_t {
        TableEnd = 0x0,
        PSOAssembly,
        VSAssembly,
        PSAssembly,

        CSAssembly,

        RootSignature,
    };

    struct Record {
        RecordType type;
        RecordFlags flags;
        const uint8_t* data;
        uint32_t dataSize;
    };

    class ArchiveReader {
    public:
        explicit ArchiveReader(const void* archive, uint32_t sizeInBytes) noexcept :
            m_Archive(static_cast<const uint8_t*>(archive)), m_Size(sizeInBytes) {}

    public:
        void Read() noexcept {
            const uint8_t* cursor = m_Archive;
            m_Version = ReadItem<SerializedVersion>(cursor);
            m_PSOType = ReadItem<ArchivePSOType>(cursor);
            m_PSOLang = ReadItem<ArchivePSOLang>(cursor);
            const auto* headerBinarySizeMarker = ReadItem<uint32_t>(cursor);
            assert(*headerBinarySizeMarker == m_Size);

            const auto* recordType = ReadItem<RecordType>(cursor);
            uint32_t totalStorageSize = 0;
            while (*recordType != RecordType::TableEnd) {
                Record record{*recordType};
                switch (*recordType) {
                    case RecordType::PSOAssembly:
                    case RecordType::VSAssembly:
                    case RecordType::PSAssembly:
                        record.flags = *ReadItem<RecordFlags>(cursor);
                        record.data = m_Archive + *ReadItem<uint32_t>(cursor);
                        record.dataSize = *ReadItem<uint32_t>(cursor);
                        totalStorageSize += record.dataSize;
                        m_Table[*recordType] = record;
                        break;
                    default:
                        assert(0);
                }
                recordType = ReadItem<RecordType>(cursor);
            }
            // Trailing RecordType item is 4 bytes aligned.
            cursor += 2;

            const auto* trailBinarySizeMarker = reinterpret_cast<const uint32_t*>(cursor + totalStorageSize);
            if (*trailBinarySizeMarker != *headerBinarySizeMarker) {
                assert(0);
            }
        }
        const bool HasRecord(RecordType recordType) noexcept { return m_Table.contains(recordType); };
        const Record& GetRecord(RecordType recordType) const noexcept { return m_Table.at(recordType); }
        [[nodiscard]] const void* GetArchiveBinary() const noexcept { return m_Archive; }

    private:
        template<class T>
        static inline const T* ReadItem(const uint8_t*& cursor) noexcept {
            const T* result = reinterpret_cast<const T*>(cursor);
            cursor += sizeof(T);
            return result;
        }

    private:
        const uint8_t* m_Archive;
        uint32_t m_Size;

        const SerializedVersion* m_Version = nullptr;
        const ArchivePSOType* m_PSOType = nullptr;
        const ArchivePSOLang* m_PSOLang = nullptr;
        std::map<RecordType, Record> m_Table{};
    };
} // namespace SCAR
