#include <SCAR.h>
#include <fstream>

int main() {
    const SCAR::ArchiveBinary res = SCAR::Compile();
    std::ofstream out{"result.scar", std::ios::binary};
    out.write(static_cast<const char*>(res.data), res.archiveSizeInBytes);
    out.close();
    free(res.data);
}
