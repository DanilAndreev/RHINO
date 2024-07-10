#pragma once

#ifdef __clang__
#include <Support/WinAdapter.h>
#else
#include <Windows.h>
#endif
#include <dxcapi.h>

namespace SCAR {
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
            const std::string path{wPath.begin(), wPath.end()};
            if (IncludedFiles.contains(path)) {
                // Return empty string blob if this file has been included before
                constexpr char nullStr[] = " ";
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

    static std::vector<std::wstring> GenerateBasicDXCArgs(const CompileSettings& settings,
                                                          const ChainSettings& chSettings, ChainContext& context,
                                                          const std::wstring& blobName) noexcept {
        std::vector<std::wstring> args{};

        args.emplace_back(L"-T");
        switch (chSettings.stage) {
            case ChainStageTarget::Vertex:
                args.emplace_back(L"vs_6_0");
                break;
            case ChainStageTarget::Pixel:
                args.emplace_back(L"ps_6_0");
                break;
            case ChainStageTarget::Compute:
                args.emplace_back(L"cs_6_0");
                break;
            case ChainStageTarget::Lib:
                args.emplace_back(L"lib_6_3");
                break;
            default:;
        }

        std::vector<std::wstring> includePathsArgs{};
        for (size_t i = 0; i < settings.includeDirectoriesCount; ++i) {
            std::filesystem::path dir = settings.includeDirectories[i];
            args.emplace_back(L"-I");
            args.emplace_back(dir.wstring());
        }
        args.emplace_back(L"-I");
        args.emplace_back(std::filesystem::path{chSettings.shaderFilepath}.remove_filename().wstring());

        if (chSettings.stage != ChainStageTarget::Lib) {
            args.emplace_back(L"-E");
            args.emplace_back(chSettings.entrypoint->begin(), chSettings.entrypoint->end());
        }

        std::wstring optLevel = L"-O";
        optLevel += std::to_wstring(settings.optimizationLevel);
        args.emplace_back(optLevel.c_str());

        for (const std::string& define: chSettings.defines) {
            using namespace std::literals;
            args.emplace_back(L"-D"s + std::wstring{define.begin(), define.end()});
        }

        args.emplace_back(L"-Fo");
        args.push_back(blobName);

        args.emplace_back(L"source.hlsl"); // TODO: use shader source filename
        return args;
    }

    static std::vector<const wchar_t*> ArgsWCHARView(const std::vector<std::wstring>& src) noexcept {
        std::vector<const wchar_t*> result;
        result.reserve(src.size());
        for (const std::wstring& item: src) {
            result.emplace_back(item.c_str());
        }
        return result;
    }
} // namespace SCAR
