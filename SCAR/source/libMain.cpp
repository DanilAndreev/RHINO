#include <SCAR.h>
#include <archiver/PSOArchiver.h>
#include <random>

namespace SCAR {
    ArchiveBinary Compile(const CompileSettings* settings) noexcept {
        PSOArchiver archiver{ArchivePSOType::Compute, ArchivePSOLang::DXIL};

        std::string psoData = "Hello darkness my old friend. I've come to talk with you again.";

        archiver.AddRecord(RecordType::PSOAssembly, RecordFlags::None, psoData.data(), psoData.size());
        return archiver.Archive();
    }
} // namespace SCAR
