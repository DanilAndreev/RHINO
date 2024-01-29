#include <SCAR.h>
#include <archiver/PSOArchiver.h>
#include <random>

namespace SCAR {
    ArchiveBinary Compile() noexcept {
        using namespace ArchiveTable;
        PSOArchiver archiver{};

        std::string psoData = "Hello darkness my old friend. I've come to talk with you again.";

        archiver.AddRecord(RecordType::PSOAssembly, Flags::None, psoData.data(), psoData.size());
        return archiver.Archive();
    }
} // namespace SCAR
