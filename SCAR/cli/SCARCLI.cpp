#include <CLI11.hpp>
#include <SCAR.h>
#include <fstream>
#include <string>
#include <vector>
#include <filesystem>

int main(int argc, char* argv[]) {
    std::filesystem::path outFilepath{};
    std::vector<std::string> globalDefinesS;
    std::vector<std::string> includeDirectoriesS;
    SCAR::CompileSettings settings{};
    // try {
    //     CLI::App app{"RHINO Shader Compiler-Archiver", "SCAR"};
    //     app.add_option("-o,--out", outFilepath, "Out object filepath. If not set - result will be printed to STDOUT.");
    //     app.add_flag("--O0{0},--O1{1},--O2{2},--O3{3}", settings.optimizationLevel, "Optimization level.");
    //     app.add_option("-t,--target", settings.psoLang, "Destination language")
    //             ->required()
    //             ->transform(CLI::CheckedTransformer(
    //                     std::map<std::string, SCAR::ArchivePSOLang>{
    //                             {"DXIL", SCAR::ArchivePSOLang::DXIL},
    //                             {"SPIRV", SCAR::ArchivePSOLang::SPIRV},
    //                             {"MetalLib", SCAR::ArchivePSOLang::MetalLib},
    //                     },
    //                     CLI::ignore_case));
    //     app.add_option("-D", "Define shader macro.")->take_all()->each([&globalDefinesS](const std::string& val) {
    //         globalDefinesS.push_back(val);
    //     });
    //     app.add_option("-I", "Define shader macro.")->take_all()->each([&includeDirectoriesS](const std::string& val) {
    //         includeDirectoriesS.push_back(val);
    //     });
    //     app.parse(argc, argv);
    // }
    // catch (const std::exception& e) {
    //     std::cerr << "SCAR Failed: " << e.what() << std::endl;
    // }

    std::vector<const char*> globalDefines{};
    globalDefines.resize(globalDefinesS.size());
    for (size_t i = 0; i < globalDefinesS.size(); ++i) {
        globalDefines[i] = globalDefinesS[i].c_str();
    }
    std::vector<const char*> includeDirectories{};
    includeDirectories.resize(includeDirectoriesS.size());
    for (size_t i = 0; i < includeDirectoriesS.size(); ++i) {
        includeDirectories[i] = includeDirectoriesS[i].c_str();
    }

    settings.globalDefinesCount = globalDefines.size();
    settings.globalDefines = globalDefines.data();
    settings.includeDirectoriesCount = includeDirectories.size();
    settings.includeDirectories = includeDirectories.data();

    // -------------------------- START TESTS
    const RHINO::DescriptorRangeDesc space0rd[] = {
        RHINO::DescriptorRangeDesc{RHINO::DescriptorRangeType::UAV, 0, 1},
    };

    const RHINO::DescriptorRangeDesc space1rd[] = {
        RHINO::DescriptorRangeDesc{RHINO::DescriptorRangeType::UAV, 0, 1},
        RHINO::DescriptorRangeDesc{RHINO::DescriptorRangeType::UAV, 1, 1},
    };

    const RHINO::DescriptorSpaceDesc spaces[] = {
        RHINO::DescriptorSpaceDesc{0, 0, std::size(space0rd), space0rd},
        RHINO::DescriptorSpaceDesc{1, 4, std::size(space1rd), space1rd},
    };
    SCAR::PSORootSignatureDesc rootSignature{};
    rootSignature.spacesCount = std::size(spaces);
    rootSignature.spacesDescs = spaces;

    settings.rootSignature = &rootSignature;
    settings.psoLang = SCAR::ArchivePSOLang::SPIRV;
    settings.psoType = SCAR::ArchivePSOType::Compute;

    outFilepath = "out.scar";

    //----------------------------END TESTS

    const SCAR::ArchiveBinary res = SCAR::Compile(&settings);

    if (!outFilepath.empty()) {
        std::ofstream out{outFilepath, std::ios::binary};
        out.write(static_cast<const char*>(res.data), res.archiveSizeInBytes);
        out.close();
    } else {
        //TODO: print to STDOUT in b64 format
    }
    free(res.data);
    return 0;
}
