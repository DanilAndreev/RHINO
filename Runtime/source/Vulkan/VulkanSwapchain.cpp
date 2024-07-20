#ifdef ENABLE_API_VULKAN

#include "VulkanSwapchain.h"

namespace RHINO::APIVulkan {
    void VulkanSwapchain::Initialize(const VulkanObjectContext& context, const SwapchainDesc& desc, uint32_t queueFamilyIndex) noexcept {
        m_Context = context;

#ifdef RHINO_WIN32_SURFACE
        {
            auto* surfaceDesc = static_cast<Win32SurfaceDesc*>(desc.surfaceDesc);
            VkWin32SurfaceCreateInfoKHR surfaceCreateInfoKhr{VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR};
            surfaceCreateInfoKhr.hwnd = surfaceDesc->hWnd;
            surfaceCreateInfoKhr.hinstance = surfaceDesc->hInstance;
            RHINO_VKS(vkCreateWin32SurfaceKHR(m_Context.instance, &surfaceCreateInfoKhr, m_Context.allocator, &m_Surface));
        }
#else
#error "Unsupported surface"
#endif // RHINO_WIN32_SURFACE

        VkBool32 surfaceIsSupported;
        vkGetPhysicalDeviceSurfaceSupportKHR(m_Context.physicalDevice, queueFamilyIndex, m_Surface, &surfaceIsSupported);
        assert(surfaceIsSupported && "Surface is not supported by device.");
        VkSurfaceCapabilitiesKHR surfaceCapabilities;
        RHINO_VKS(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_Context.physicalDevice, m_Surface, &surfaceCapabilities));

        uint32_t formatsCount;
        std::vector<VkSurfaceFormatKHR> availableFormats;
        RHINO_VKS(vkGetPhysicalDeviceSurfaceFormatsKHR(m_Context.physicalDevice, m_Surface, &formatsCount, nullptr));
        availableFormats.resize(formatsCount);
        RHINO_VKS(vkGetPhysicalDeviceSurfaceFormatsKHR(m_Context.physicalDevice, m_Surface, &formatsCount, availableFormats.data()));
        VkSurfaceFormatKHR surfaceFormat = availableFormats.front();

        VkSwapchainCreateInfoKHR swapchainCreateInfoKhr{VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR};
        swapchainCreateInfoKhr.surface = m_Surface;
        swapchainCreateInfoKhr.minImageCount = surfaceCapabilities.minImageCount;
        swapchainCreateInfoKhr.imageFormat = surfaceFormat.format;
        swapchainCreateInfoKhr.imageColorSpace = surfaceFormat.colorSpace;
        swapchainCreateInfoKhr.imageExtent = surfaceCapabilities.currentExtent;
        swapchainCreateInfoKhr.imageArrayLayers = 1;
        swapchainCreateInfoKhr.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        swapchainCreateInfoKhr.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        swapchainCreateInfoKhr.queueFamilyIndexCount = 0;
        swapchainCreateInfoKhr.pQueueFamilyIndices = nullptr;
        swapchainCreateInfoKhr.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
        swapchainCreateInfoKhr.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        swapchainCreateInfoKhr.presentMode = VK_PRESENT_MODE_FIFO_KHR;
        swapchainCreateInfoKhr.clipped = VK_TRUE;
        swapchainCreateInfoKhr.oldSwapchain = VK_NULL_HANDLE;
        RHINO_VKS(vkCreateSwapchainKHR(m_Context.device, &swapchainCreateInfoKhr, m_Context.allocator, &m_Swapchain));

        uint32_t swapchainImagesCount = 0;
        vkGetSwapchainImagesKHR(m_Context.device, m_Swapchain, &swapchainImagesCount, nullptr);
        m_SwapchainImages.resize(swapchainImagesCount);
        vkGetSwapchainImagesKHR(m_Context.device, m_Swapchain, &swapchainImagesCount, m_SwapchainImages.data());

        m_SwapchainSyncs.resize(swapchainImagesCount);
        for (size_t i = 0; i < swapchainImagesCount; ++i) {
            VkFenceCreateInfo fenceCreateInfo{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
            fenceCreateInfo.flags = 0;
            vkCreateFence(m_Context.device, &fenceCreateInfo, m_Context.allocator, &m_SwapchainSyncs[i]);
        }
    }

    void VulkanSwapchain::Present(VkQueue queue) noexcept {
        VkResult swapchainStatus = VK_SUCCESS;
        VkPresentInfoKHR presentInfo{VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};
        presentInfo.waitSemaphoreCount = 0;
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = &m_Swapchain;
        presentInfo.pImageIndices = &m_CurrentImageIndex;
        presentInfo.pResults = &swapchainStatus;

        RHINO_VKS(vkQueuePresentKHR(queue, &presentInfo));
        RHINO_VKS(swapchainStatus);
    }

    void VulkanSwapchain::Release() noexcept {
        vkWaitForFences(m_Context.device, m_SwapchainSyncs.size(), m_SwapchainSyncs.data(), VK_TRUE, ~0);
        for (VkFence fence : m_SwapchainSyncs) {
            vkDestroyFence(m_Context.device, fence, m_Context.allocator);
        }
        vkDestroySwapchainKHR(m_Context.device, m_Swapchain, m_Context.allocator);
        vkDestroySurfaceKHR(m_Context.instance, m_Surface, m_Context.allocator);
        delete this;
    }

    void VulkanSwapchain::GetTexture() noexcept {
        RHINO_VKS(vkAcquireNextImageKHR(m_Context.device, m_Swapchain, ~0, VK_NULL_HANDLE, VK_NULL_HANDLE, &m_CurrentImageIndex));
        VkImage image = m_SwapchainImages[m_CurrentImageIndex];

        return image;
    }
} // namespace RHINO::APIVulkan

#endif // ENABLE_API_VULKAN
