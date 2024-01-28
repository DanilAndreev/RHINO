#ifdef ENABLE_API_D3D12
#include "D3D12/D3D12Backend.h"
#endif // ENABLE_API_D3D12

#ifdef ENABLE_API_VULKAN
#include "Vulkan/VulkanBackend.h"
#endif // ENABLE_API_VULKAN

#ifdef ENABLE_API_METAL
#include "Metal/AllocateMetalBackend.h"
#endif // ENABLE_API_METAL

#include "DebugLayer/DebugLayer.h"
#include <cstdlib>


namespace RHINO {
    RHINOInterface* CreateRHINO(BackendAPI backendApi) noexcept {
        RHINOInterface* result = nullptr;
        switch (backendApi) {
#ifdef ENABLE_API_D3D12
            case BackendAPI::D3D12:
                result = new APID3D12::D3D12Backend{};
                break;
#endif // ENABLE_API_D3D12
#ifdef ENABLE_API_VULKAN
            case BackendAPI::Vulkan:
                result = new APIVulkan::VulkanBackend{};
                break;
#endif // ENABLE_API_VULKAN
#ifdef ENABLE_API_METAL
            case BackendAPI::Metal:
                result = APIMetal::AllocateMetalBackend();
                break;
#endif // ENABLE_API_METAL
            default:
                assert(0 && "Invalid API or selected API is not supported on this platform.");
                return nullptr;
        }
        const char* validationEnv = std::getenv("RHINO_ENABLE_VALIDATION");
        if (validationEnv != nullptr && std::string{validationEnv} == "1") {
            result = new DebugLayer::DebugLayer{result};
        }
        return result;
    }
}// namespace RHINO
