#pragma once

#include <SCARUnarchiver.h>

namespace SCAR {
    struct PSOInfo {
        ArchivePSOType type;
        ArchivePSOLang lang;
    };

    struct SerializedRecord {
        RecordType type;
        RecordFlags flags;

        uint32_t dataStartOffset;
        uint32_t dataSize;
    };

    class PSOArchiver {
        //                                                       VERSION                PSO INFO         BINARY SIZE
        static constexpr uint32_t ARCHIVE_HEADER_SIZE = sizeof(SerializedVersion) + sizeof(PSOInfo) + sizeof(uint32_t);
        //                                                BINARY SIZE
        static constexpr uint32_t ARCHIVE_FOOTER_SIZE = sizeof(uint32_t);

    public:
        explicit PSOArchiver(ArchivePSOType type, ArchivePSOLang lang) noexcept;

    public:
        void AddRecord(RecordType t, RecordFlags flags, const void* data,
                       uint32_t dataSize) noexcept;
        template<class T>
        void AddRecord(RecordType t, RecordFlags flags, const std::vector<T>& data) noexcept {
            AddRecord(t, flags, data.data(), data.size() * sizeof(T));
        }
        ArchiveBinary Archive() noexcept;

    private:
        [[nodiscard]] uint32_t CalculateArchiveTableSize() const noexcept;
        [[nodiscard]] uint32_t CalculateArchiveBinarySize() const noexcept;

    private:
        PSOInfo m_PSOInfo;
        std::map<RecordType, SerializedRecord> m_Table{};
        std::vector<uint8_t> m_Storage{};
    };
} // namespace SCAR
