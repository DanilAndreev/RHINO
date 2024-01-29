#include <SCARUnarchiver.h>
#include <CLI11.hpp>
#include <fstream>

int main(int argc, char* argv[]) {
    CLI::App app{"RHINO Shader Compiler-Archiver Reflection utility.", "SCARReflect"};
    try {
        app.positionals_at_end();
        app.parse(argc, argv);
    } catch (std::exception& error) {
        std::cerr << "RHIPSOCompiler CLI usage error:\n" << error.what() << std::endl;
    }

    std::ifstream inFile{"result.scar", std::ios::binary | std::ios::ate};
    if (!inFile.is_open()) {
        std::cerr << "Failed to open file: " << "" << std::endl;
        return 1;
    }
    std::streamsize size = inFile.tellg();
    inFile.seekg(0, std::ios::beg);
    std::vector<uint8_t> archive(size);
    if (!size || !inFile.read(reinterpret_cast<char*>(archive.data()), size))
    {
        std::cerr << "Failed to read file content: " << "" << std::endl;
    }

    SCAR::ArchiveReader reader{archive.data(), static_cast<uint32_t>(archive.size())};
    reader.Read();
    auto a = reader.HasRecord(SCAR::RecordType::PSOAssembly);
    auto b = reader.GetRecord(SCAR::RecordType::PSOAssembly);
    std::cout << reinterpret_cast<const char*>(b.data) << std::endl;
}
