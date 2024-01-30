#include "HlslToDxilStep.h"

#ifdef __clang__
#include <Support/WinAdapter.h>
#else
#include <Windows.h>
#endif
#include <dxcapi.h>


class CustomIncludeHandler : public IDxcIncludeHandler {
public:
    HRESULT STDMETHODCALLTYPE LoadSource(_In_ LPCWSTR pFilename,
                                         _COM_Outptr_result_maybenull_ IDxcBlob** ppIncludeSource) override {
        static IDxcUtils* pUtils = nullptr;
        if (!pUtils) {
            DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&pUtils));
        }

        IDxcBlobEncoding* pEncoding;
        std::wstring wPath = pFilename;
        std::string path{wPath.begin(), wPath.end()};
        if (IncludedFiles.find(path) != IncludedFiles.end()) {
            // Return empty string blob if this file has been included before
            static const char nullStr[] = " ";
            pUtils->CreateBlobFromPinned(nullStr, ARRAYSIZE(nullStr), DXC_CP_ACP, &pEncoding);
            *ppIncludeSource = pEncoding;
            return S_OK;
        }

        HRESULT hr = pUtils->LoadFile(pFilename, nullptr, &pEncoding);
        if (SUCCEEDED(hr)) {
            IncludedFiles.insert(path);
            *ppIncludeSource = pEncoding;
        }
        return hr;
    }

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, _COM_Outptr_ void** ppvObject) override {
        return E_NOINTERFACE;
    }
    ULONG STDMETHODCALLTYPE AddRef() override { return 0; }
    ULONG STDMETHODCALLTYPE Release() override { return 0; }

    std::unordered_set<std::string> IncludedFiles;
};

namespace SCAR {
    bool HlslToDxilStep::Execute(const CompileSettings& settings, const ChainSettings& chSettings,
                                 ChainContext& context) noexcept {
        std::ifstream shaderStream{chSettings.shaderFilepath};
        if (!shaderStream.is_open()) {
            using namespace std::string_literals;
            context.errors.emplace_back("Failed to open file: "s + chSettings.shaderFilepath.string());
            return false;
        }
        std::string shaderText((std::istreambuf_iterator<char>(shaderStream)), std::istreambuf_iterator<char>());

        IDxcUtils* dxcUtils;
        DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&dxcUtils));
        IDxcBlobEncoding* sourceBlob;
        dxcUtils->CreateBlob(shaderText.data(), shaderText.length(), CP_UTF8, &sourceBlob);
        IDxcCompiler3* compiler;
        DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&compiler));
        auto includeHandler = std::make_shared<CustomIncludeHandler>();

        std::vector<const wchar_t*> args{};
        args.push_back(L"-all_resources_bound");

        args.push_back(L"-T");
        switch (chSettings.stage) {
            case ChainStageTarget::Vertex:
                args.push_back(L"vs_6_0");
                break;
            case ChainStageTarget::Pixel:
                args.push_back(L"ps_6_0");
                break;
            case ChainStageTarget::Compute:
                args.push_back(L"cs_6_0");
                break;
        }

        std::vector<std::wstring> includePathsArgs{};
        for (size_t i = 0; i < settings.includeDirectoriesCount; ++i) {
            std::filesystem::path dir = settings.includeDirectories[i];
            args.push_back(L"-I");
            auto& it = includePathsArgs.emplace_back(dir.wstring());
            args.push_back(it.c_str());
        }
        args.push_back(L"-I");
        auto& it = includePathsArgs.emplace_back(
                std::filesystem::path{chSettings.shaderFilepath}.remove_filename().wstring());
        args.push_back(it.c_str());

        auto entrypoint = std::wstring{chSettings.entrypoint.begin(), chSettings.entrypoint.end()};
        args.push_back(L"-E");
        args.push_back(entrypoint.c_str());

        std::wstring optLevel = L"-O";
        optLevel += std::to_wstring(settings.optimizationLevel);
        args.push_back(optLevel.c_str());

        std::vector<const wchar_t*> defineMacros{};
        for (const std::string& define: chSettings.defines) {
            using namespace std::literals;
            std::wstring temp = L"-D"s + std::wstring{define.begin(), define.end()};
            auto* text = new wchar_t[temp.length() + 1];
            memcpy(text, temp.data(), temp.size() * sizeof(wchar_t));
            text[temp.length()] = L'\0';
            defineMacros.push_back(text);
            args.push_back(text);
        }

        const wchar_t* bName = L"result.dxil";
        args.push_back(L"-Fo");
        args.push_back(bName);

        args.push_back(L"shader.hlsl");

        DxcBuffer sourceBuffer{};
        sourceBuffer.Ptr = sourceBlob->GetBufferPointer();
        sourceBuffer.Size = sourceBlob->GetBufferSize();
        sourceBuffer.Encoding = 0;

        IDxcResult* result;
        compiler->Compile(&sourceBuffer, args.data(), args.size(), includeHandler.get(), IID_PPV_ARGS(&result));

        for (const wchar_t* item: defineMacros) {
            delete item;
        }

        IDxcBlobUtf8* errors{};
        result->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&errors), nullptr);
        if (errors && errors->GetStringLength() > 0) {
            using namespace std::string_literals;
            if (result->HasOutput(DXC_OUT_OBJECT)) {
                context.warnings.push_back((char*)errors->GetBufferPointer());
            }
            else {
                context.errors.push_back("DXC failed with message:\n"s + (char*)errors->GetBufferPointer());
                return false;
            }
        }

        IDxcBlobEncoding* outName{};
        dxcUtils->CreateBlob(bName, wcslen(bName) * sizeof(wchar_t), 0, &outName);

        IDxcBlobWide* outNameWide{};
        dxcUtils->GetBlobAsWide(outName, &outNameWide);

        if (!result->HasOutput(DXC_OUT_OBJECT)) {
            context.errors.push_back("DXC exited with no output object:\n");
            return false;
        }
        IDxcBlob* outObject{};
        result->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&outObject), &outNameWide);

        context.dataLength = outObject->GetBufferSize();
        context.data.reset(new uint8_t[context.dataLength]);
        memcpy(context.data.get(), outObject->GetBufferPointer(), context.dataLength * sizeof(uint8_t));
        return true;
    }
} // namespace SCAR
