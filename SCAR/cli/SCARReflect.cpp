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
        case SCAR::RecordType::RootSignature:
            return "RootSignature";
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
        default:
            return "Unknown";
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
        size_t offsetFromStart = reinterpret_cast<size_t>(r.data) - reinterpret_cast<size_t>(reader.GetArchiveBinary());
        std::cout << IS << "Record: " << str(r.type) << " | OffsetFromArchiveStart: " << offsetFromStart
                  << " Size: " << r.dataSize << " Flags: " << std::bitset<16>(static_cast<uint16_t>(r.flags))
                  << std::endl;
    }
}

void PrintRootSignature(const SCAR::ArchiveReader& reader, size_t indent = 0) noexcept {
    std::string IS{};
    IS.resize(indent, ' ');

    std::cout << IS << "RootSignarute Record Deserialization:\n";
    for (const RHINO::DescriptorSpaceDesc& s: reader.CreateRootSignatureView()) {
        std::cout << IS << "  Space" << s.space << "\n";
        std::cout << IS << "    OffsetInDescriptorsFromTableStart: " << s.offsetInDescriptorsFromTableStart << "\n";
        if (s.rangeDescCount) {
            std::cout << "    Ranges:\n";
            for (size_t i = 0; i < s.rangeDescCount; ++i) {
                const RHINO::DescriptorRangeDesc& r = s.rangeDescs[i];
                std::cout << IS << "      Range" << i << " " << str(r.rangeType)
                          << " | BaseRegisterSlot: " << r.baseRegisterSlot
                          << " DescriptorsCount: " << r.descriptorsCount << "\n";
            }
        }
        else {
            std::cout << IS << "  Ranges: Empty\n";
        }
    }
}

int main(int argc, char* argv[]) {
    CLI::App app{"RHINO Shader Compiler-Archiver Reflection utility.", "SCARReflect"};
    try {
        app.positionals_at_end();
        app.parse(argc, argv);
    }
    catch (std::exception& error) {
        std::cerr << "RHIPSOCompiler CLI usage error:\n" << error.what() << std::endl;
    }

    std::ifstream inFile{"out.scar", std::ios::binary | std::ios::ate};
    if (!inFile.is_open()) {
        std::cerr << "Failed to open file: "
                  << "" << std::endl;
        return 1;
    }
    std::streamsize size = inFile.tellg();
    inFile.seekg(0, std::ios::beg);
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
    if (reader.HasRecord(SCAR::RecordType::RootSignature)) {
        PrintRootSignature(reader, 0);
    }
}
