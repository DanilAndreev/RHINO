#include <RHINO.h>

#include <CLI11.hpp>
#define SCAR_RHINO_ADDONS
#include <SCARUnarchiver.h>
#include <fstream>
#include <filesystem>
#include <iostream>
#include <ranges>

static std::string MakeClosingFileExtension(const SCAR::ArchiveReader& reader) noexcept {
    switch (reader.GetPSOLang()) {
        case SCAR::ArchivePSOLang::SPIRV:
            return ".spirv";
        case SCAR::ArchivePSOLang::DXIL:
            return ".dxil";
        case SCAR::ArchivePSOLang::MetalLib:
            return ".metallib";
        default:
            throw std::runtime_error{"Unexpected error. Invalid PSOLang record..."};
    }
}

int main(int argc, char* argv[]) {
    std::filesystem::path archiveFilepath;
    std::filesystem::path outDir;

    CLI::App app{"RHINO Shader Compiler-Archiver Reflection utility.", "SCARReflect"};
    try {
        app.add_option("filepath", archiveFilepath, "SCAR archive filepath")->required();
        app.add_option("-o, --out-dir", outDir, "Dump output directory")->default_str("./");
        app.parse(argc, argv);
    }
    catch (std::exception& error) {
        std::cerr << "RHIPSOCompiler CLI usage error:\n" << error.what() << std::endl;
        return 1;
    }

    std::ifstream inFile{archiveFilepath, std::ios::binary | std::ifstream::ate};
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

    std::string inputFilename = archiveFilepath.filename().string();

    for (const SCAR::Record& r: reader.GetTable() | std::views::values) {
        std::filesystem::path outFilepath = outDir;
        std::string outFilename = inputFilename;

        switch (r.type) {
            case SCAR::RecordType::PSOAssembly:
                outFilename += ".pso";
                break;
            case SCAR::RecordType::VSAssembly:
                outFilename += ".vs";
                break;
            case SCAR::RecordType::PSAssembly:
                outFilename += ".ps";
                break;
            case SCAR::RecordType::CSAssembly:
                outFilename += ".cs";
                break;
            case SCAR::RecordType::RootSignature:
            case SCAR::RecordType::TableEnd:
            default:
                continue;
        }
        outFilename += MakeClosingFileExtension(reader);
        outFilepath.concat(outFilename);
        std::ofstream outFile{outFilepath, std::ios::binary};
        outFile.write(reinterpret_cast<const char*>(r.data), r.dataSize);
        outFile.close();
    }
}
