#pragma once

#ifdef ENABLE_API_VULKAN

#include "VulkanBackendTypes.h"

namespace RHINO::APIVulkan {
    class VulkanSwapchain : public Swapchain {
    public:
        void Initialize(const VulkanObjectContext& context, const SwapchainDesc& desc) noexcept;
        void Present(VkQueue queue) noexcept;

    public:
        void GetTexture() noexcept final;
        void Release() noexcept final;

    private:
        VulkanObjectContext m_Context = {};
        VkSwapchainKHR m_Swapchain = VK_NULL_HANDLE;
        VkSurfaceKHR m_Surface = VK_NULL_HANDLE;

        std::vector<VkImage> m_SwapchainImages = {};
        std::vector<VkFence> m_SwapchainSyncs = {};
        uint32_t m_CurrentImageIndex = 0;
    };
} // namespace RHINO::APIVulkan

#endif // ENABLE_API_VULKAN
