#pragma once

#ifdef ENABLE_API_VULKAN

#include "VulkanBackendTypes.h"

namespace RHINO::APIVulkan {
    class VulkanSwapchain : public Swapchain {
    public:
        void Initialize(const VulkanObjectContext& context, const SwapchainDesc& desc, uint32_t queueFamilyIndex) noexcept;
        void Present(VkQueue queue, VulkanTexture2D* toPresent, size_t width, size_t height) noexcept;

    public:
        void Release() noexcept final;

    private:
        VulkanObjectContext m_Context = {};
        VkSwapchainKHR m_Swapchain = VK_NULL_HANDLE;
        VkSurfaceKHR m_Surface = VK_NULL_HANDLE;

        std::vector<VkImage> m_SwapchainImages = {};
        std::vector<VkSemaphore> m_SwapchainSyncs = {};
        VkFence m_AcquireFence = VK_NULL_HANDLE;

        VkCommandPool m_CMDPool = VK_NULL_HANDLE;
    };
} // namespace RHINO::APIVulkan

#endif // ENABLE_API_VULKAN
