#include <RHINO.h>

#include <CLI11.hpp>
#define SCAR_RHINO_ADDONS
#include <SCARUnarchiver.h>
#include <bitset>
#include <fstream>
#include <iostream>
#include <ranges>

static const char* str(SCAR::RecordType recordType) noexcept {
    switch (recordType) {
        case SCAR::RecordType::TableEnd:
            return "TableEnd";
        case SCAR::RecordType::PSOAssembly:
            return "PSOAssembly";
        case SCAR::RecordType::VSAssembly:
            return "VSAssembly";
        case SCAR::RecordType::PSAssembly:
            return "PSAssembly";
        case SCAR::RecordType::CSAssembly:
            return "CSAssembly";
        case SCAR::RecordType::LibAssembly:
            return "LibAssembly";
        case SCAR::RecordType::ShadersEntrypoints:
            return "ShadersEntrypoints";
        case SCAR::RecordType::RTAttributes:
            return "RTAttributes";
        default:
            return "Unknown";
    }
}

static const char* str(SCAR::ArchivePSOLang lang) noexcept {
    switch (lang) {
        case SCAR::ArchivePSOLang::SPIRV:
            return "SPIRV";
        case SCAR::ArchivePSOLang::DXIL:
            return "DXIL";
        case SCAR::ArchivePSOLang::MetalLib:
            return "MetalLib";
        default:
            return "Unknown";
    }
}

static const char* str(SCAR::ArchivePSOType type) noexcept {
    switch (type) {
        case SCAR::ArchivePSOType::Compute:
            return "Compute";
        case SCAR::ArchivePSOType::Graphics:
            return "Graphics";
        case SCAR::ArchivePSOType::Library:
            return "Library";
        default:
            return "Unknown";
    }
}

static const char* str(SCAR::RecordFlags flag) noexcept {
    switch (flag) {
        case SCAR::RecordFlags::MultipleValues:
            return "MultipleValues";
        default:
            return "ERROR_FLAG_VALUE";
    }
}

static const char* str(RHINO::DescriptorRangeType type) noexcept {
    switch (type) {
        case RHINO::DescriptorRangeType::CBV:
            return "CBV";
        case RHINO::DescriptorRangeType::SRV:
            return "SRV";
        case RHINO::DescriptorRangeType::UAV:
            return "UAV";
        case RHINO::DescriptorRangeType::Sampler:
            return "Sampler";
        default:
            return "Unknown";
    }
}

void PrintTable(const SCAR::ArchiveReader& reader, size_t indent = 0) noexcept {
    std::string IS{};
    IS.resize(indent, ' ');

    std::cout << "Table content:" << std::endl;
    for (const SCAR::Record& r: reader.GetTable() | std::views::values) {
        std::vector<std::string> flagsStr{};
        for (size_t i = 0; i < sizeof(SCAR::RecordFlags); ++i) {
            auto flag = static_cast<SCAR::RecordFlags>(1 << i);
            if (bool(size_t(flag) & size_t(SCAR::RecordFlags::VALID_MASK)) && bool(size_t(flag) & size_t(r.flags))) {
                flagsStr.emplace_back(str(flag));
            }
        }

        size_t offsetFromStart = reinterpret_cast<size_t>(r.data) - reinterpret_cast<size_t>(reader.GetArchiveBinary());
        std::cout << IS << "Record: " << str(r.type) << " | OffsetFromArchiveStart: " << offsetFromStart
                  << " Size: " << r.dataSize << " Flags: " << std::bitset<16>(static_cast<uint16_t>(r.flags));
        if (!flagsStr.empty()) {
            std::cout << " (";
        }
        for (size_t i = 0; i < flagsStr.size(); ++i) {
            std::cout << flagsStr[i];
            if (i + 1 < flagsStr.size()) {
                std::cout << " | ";
            } else {
                std::cout << ")";
            }
        }
        std::cout << std::endl;
    }
}

int main(int argc, char* argv[]) {
    std::filesystem::path archiveFilepath;

    CLI::App app{"RHINO Shader Compiler-Archiver Reflection utility.", "SCARReflect"};
    try {
        app.add_option("filepath", archiveFilepath, "SCAR archive filepath")->required();
        app.parse(argc, argv);
    }
    catch (std::exception& error) {
        std::cerr << "RHIPSOCompiler CLI usage error:\n" << error.what() << std::endl;
        return 1;
    }

    std::ifstream inFile{archiveFilepath, std::ifstream::binary | std::ifstream::ate};
    if (!inFile.is_open()) {
        std::cerr << "Failed to open file: "
                  << "" << std::endl;
        return 1;
    }
    std::streamsize size = inFile.tellg();
    inFile.seekg(0, std::ifstream::beg);
    std::vector<uint8_t> archive(size);
    if (!size || !inFile.read(reinterpret_cast<char*>(archive.data()), size)) {
        std::cerr << "Failed to read file content: "
                  << "" << std::endl;
    }

    SCAR::ArchiveReader reader{archive.data(), static_cast<uint32_t>(archive.size())};
    reader.Read();

    std::cout << "Summary:" << std::endl;
    const SCAR::SerializedVersion& v = reader.GetVersion();
    std::cout << "  Compiler version: " << +v.major << "." << +v.minor << "." << +v.patch << std::endl;
    std::cout << "  PSO Language/Technology: " << str(reader.GetPSOLang()) << std::endl;
    std::cout << "  PSO Type: " << str(reader.GetPSOType()) << std::endl;

    PrintTable(reader, 2);
    std::cout << "\n";
}
