#include <CLI11.hpp>
#include <json.hpp>
#include <SCAR.h>
#include <fstream>
#include <string>
#include <vector>
#include <filesystem>

class JSONDescLoader {
public:
    explicit JSONDescLoader(const std::filesystem::path& inputFilepath) {
        std::filesystem::path filepath = std::filesystem::absolute(inputFilepath);
        //TODO: add json schema validator pass before deserializing
        std::ifstream psoDescFile{filepath};
        if (!psoDescFile.is_open()) {
            throw std::runtime_error{"Failed to open PSO desc file."};
        }
        nlohmann::json inputDesc = nlohmann::json::parse(psoDescFile);
        psoDescFile.close();

        std::string psoType = inputDesc["psoType"];
        if (psoType == "Library") {
            m_Settings.psoType = SCAR::ArchivePSOType::Library;
        } else if (psoType == "Compute") {
            m_Settings.psoType = SCAR::ArchivePSOType::Compute;
        } else {
            throw std::runtime_error("Invalid psoType param");
        }

        switch (m_Settings.psoType) {
            case SCAR::ArchivePSOType::Compute: {
                const nlohmann::json& computeSettings = inputDesc["computeSettings"];
                m_EntrypointsStorage.emplace_back(std::string{computeSettings["entrypoint"]});
                m_Settings.computeSettings.entrypoint = m_EntrypointsStorage[0].c_str();
                std::filesystem::path shaderFilepath = std::string{computeSettings["shaderSourceFilepath"]};
                if (!shaderFilepath.is_absolute()) {
                    shaderFilepath = filepath.parent_path() / shaderFilepath;
                }
                m_InputFilepath = shaderFilepath.string();
                m_Settings.computeSettings.inputFilepath = m_InputFilepath.c_str();
                break;
            }
            case SCAR::ArchivePSOType::Library: {
                const nlohmann::json& libSettings = inputDesc["librarySettings"];
                m_EntrypointsStorage = libSettings["entrypoints"];
                for (const auto& item : m_EntrypointsStorage) {
                    m_EntrypointsRefs.emplace_back(item.c_str());
                }
                m_Settings.librarySettings.entrypointsCount = m_EntrypointsRefs.size();
                m_Settings.librarySettings.entrypoints = m_EntrypointsRefs.data();
                std::filesystem::path shaderFilepath = std::string{libSettings["shaderSourceFilepath"]};
                if (!shaderFilepath.is_absolute()) {
                    shaderFilepath = filepath.parent_path() / shaderFilepath;
                }
                m_InputFilepath = shaderFilepath.string();
                m_Settings.librarySettings.inputFilepath = m_InputFilepath.c_str();
                m_Settings.librarySettings.maxAttributeSizeInBytes = libSettings["maxAttributeSizeInBytes"];
                m_Settings.librarySettings.maxPayloadSizeInBytes = libSettings["maxPayloadSizeInBytes"];
                break;
            }
            case SCAR::ArchivePSOType::Graphics:
                // TODO: implement.
                break;
        }
    }
    [[nodiscard]] const SCAR::CompileSettings& GetCompileSettingsView() const noexcept { return m_Settings; }
private:
    SCAR::CompileSettings m_Settings = {};
    std::vector<std::string> m_EntrypointsStorage{};
    std::vector<const char*> m_EntrypointsRefs{};
    std::string m_InputFilepath{};
};

int main(int argc, char* argv[]) {
    std::filesystem::path outFilepath{};
    std::vector<std::string> globalDefinesS;
    std::vector<std::string> includeDirectoriesS;
    SCAR::CompileSettings settings{};
    std::filesystem::path psoDescFilepath = {};
    try {
        CLI::App app{"RHINO Shader Compiler-Archiver", "SCAR"};
        app.add_option("-o,--out", outFilepath, "Out object filepath. If not set - result will be printed to STDOUT.");
        app.add_flag("--O0{0},--O1{1},--O2{2},--O3{3}", settings.optimizationLevel, "Optimization level.");
        app.add_option("-t,--target", settings.psoLang, "Destination language")
                ->required()
                ->transform(CLI::CheckedTransformer(
                        std::map<std::string, SCAR::ArchivePSOLang>{
                                {"DXIL", SCAR::ArchivePSOLang::DXIL},
                                {"SPIRV", SCAR::ArchivePSOLang::SPIRV},
                                {"MetalLib", SCAR::ArchivePSOLang::MetalLib},
                        },
                        CLI::ignore_case));
        app.add_option("-D", "Define shader macro.")->take_all()->each([&globalDefinesS](const std::string& val) {
            globalDefinesS.push_back(val);
        });
        app.add_option("-I", "Define shader macro.")->take_all()->each([&includeDirectoriesS](const std::string& val) {
            includeDirectoriesS.push_back(val);
        });
        app.add_option("desc", psoDescFilepath, "PSO Desc json filepath.");
        app.parse(argc, argv);
    }
    catch (const std::exception& e) {
        std::cerr << "SCAR Failed: " << e.what() << std::endl;
    }


    JSONDescLoader descView{psoDescFilepath};
    settings.psoType = descView.GetCompileSettingsView().psoType;
    switch (settings.psoType) {
        case SCAR::ArchivePSOType::Compute:
            settings.computeSettings = descView.GetCompileSettingsView().computeSettings;
            break;
        case SCAR::ArchivePSOType::Graphics:
            settings.graphicsSetting = descView.GetCompileSettingsView().graphicsSetting;
            break;
        case SCAR::ArchivePSOType::Library:
            settings.librarySettings = descView.GetCompileSettingsView().librarySettings;
            break;
    }

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

    // // -------------------------- START TESTS
    // const RHINO::DescriptorRangeDesc space0rd[] = {
    //     RHINO::DescriptorRangeDesc{RHINO::DescriptorRangeType::SRV, 0, 1},
    //     RHINO::DescriptorRangeDesc{RHINO::DescriptorRangeType::UAV, 1, 1},
    //     RHINO::DescriptorRangeDesc{RHINO::DescriptorRangeType::CBV, 2, 1},
    // };
    //
    // const RHINO::DescriptorSpaceDesc spaces[] = {
    //     RHINO::DescriptorSpaceDesc{0, 0, std::size(space0rd), space0rd},
    // };
    //
    // SCAR::PSORootSignatureDesc rootSignature{};
    // rootSignature.spacesCount = std::size(spaces);
    // rootSignature.spacesDescs = spaces;
    //
    // const char* entrypoints[] = {"MyRaygenShader", "MyMissShader", "MyClosestHitShader"};
    //
    // settings.rootSignature = &rootSignature;
    // settings.psoLang = SCAR::ArchivePSOLang::DXIL;
    // settings.psoType = SCAR::ArchivePSOType::Library;
    // settings.librarySettings.entrypointsCount = std::size(entrypoints);
    // settings.librarySettings.entrypoints = entrypoints;
    // settings.librarySettings.inputFilepath = "rt.hlsl";
    // settings.librarySettings.maxAttributeSizeInBytes = 32;
    // settings.librarySettings.maxPayloadSizeInBytes = 32;
    //
    //
    // outFilepath = "out.scar";
    //
    // //----------------------------END TESTS

    const SCAR::CompilationResult res = SCAR::Compile(&settings);

    if (res.success) {
        if (!outFilepath.empty()) {
            std::ofstream out{outFilepath, std::ios::binary};
            out.write(static_cast<const char*>(res.archive.data), res.archive.archiveSizeInBytes);
            out.close();
        } else {
            assert(0);
            //TODO: print to STDOUT in b64 format
        }
    }
    if (res.warningsStr) {
        std::cout << res.warningsStr << std::endl;
    }
    if (res.errorsStr) {
        std::cerr << res.errorsStr << std::endl;
    }
    SCAR::ReleaseCompilationResult(res);
    return res.success ? 0 : -1;
}
