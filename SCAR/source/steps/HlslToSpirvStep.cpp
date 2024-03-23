#include "HlslToSpirvStep.h"

#include <system_error>
#include "DXCCommon.hpp"

namespace SCAR {
    bool HlslToSpirvStep::Execute(const CompileSettings& settings, const ChainSettings& chSettings,
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

        const wchar_t* blobName = L"result.spirv";
        std::vector<std::wstring> args = GenerateBasicDXCArgs(settings, chSettings, context, blobName);
        args.emplace_back(L"-spirv");
        args.emplace_back(L"-no-warnings");

        DxcBuffer sourceBuffer{};
        sourceBuffer.Ptr = sourceBlob->GetBufferPointer();
        sourceBuffer.Size = sourceBlob->GetBufferSize();
        sourceBuffer.Encoding = 0;

        IDxcResult* result;
        HRESULT hr = compiler->Compile(&sourceBuffer, ArgsWCHARView(args).data(), args.size(), includeHandler.get(),
                                       IID_PPV_ARGS(&result));
        if (hr != S_OK) {
            context.errors.emplace_back(std::system_category().message(hr));
            return false;
        }

        IDxcBlobUtf8* errors{};
        result->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&errors), nullptr);
        if (errors && errors->GetStringLength() > 0) {
            using namespace std::string_literals;
            if (result->HasOutput(DXC_OUT_OBJECT)) {
                context.warnings.emplace_back(static_cast<char*>(errors->GetBufferPointer()));
            }
            else {
                auto errMsg = "DXC failed with message:\n"s + static_cast<char*>(errors->GetBufferPointer());
                context.errors.emplace_back(errMsg);
                return false;
            }
        }

        IDxcBlobEncoding* outName{};
        dxcUtils->CreateBlob(blobName, wcslen(blobName) * sizeof(wchar_t), 0, &outName);

        IDxcBlobWide* outNameWide{};
        dxcUtils->GetBlobAsWide(outName, &outNameWide);

        if (!result->HasOutput(DXC_OUT_OBJECT)) {
            context.errors.emplace_back("DXC exited with no output object:\n");
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
