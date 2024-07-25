#ifdef ENABLE_API_VULKAN

#include "VulkanSwapchain.h"
#include <RHINOSwapchainPlatform.h>

namespace RHINO::APIVulkan {
    void VulkanSwapchain::Initialize(const VulkanObjectContext& context, const SwapchainDesc& desc, uint32_t queueFamilyIndex) noexcept {
        m_Context = context;

#ifdef RHINO_WIN32_SURFACE
        {
            auto* surfaceDesc = static_cast<RHINOWin32SurfaceDesc*>(desc.surfaceDesc);
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
        RHINO_VKS(vkGetSwapchainImagesKHR(m_Context.device, m_Swapchain, &swapchainImagesCount, nullptr));
        m_SwapchainImages.resize(swapchainImagesCount);
        RHINO_VKS(vkGetSwapchainImagesKHR(m_Context.device, m_Swapchain, &swapchainImagesCount, m_SwapchainImages.data()));

        m_SwapchainSyncs.resize(swapchainImagesCount);
        for (size_t i = 0; i < swapchainImagesCount; ++i) {
            VkSemaphoreCreateInfo semaphoreCreateInfo{VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
            semaphoreCreateInfo.flags = 0;
            RHINO_VKS(vkCreateSemaphore(m_Context.device, &semaphoreCreateInfo, m_Context.allocator, &m_SwapchainSyncs[i]));
        }

        VkCommandPoolCreateInfo cmdPoolInfo{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
        cmdPoolInfo.queueFamilyIndex = queueFamilyIndex;
        cmdPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        RHINO_VKS(vkCreateCommandPool(m_Context.device, &cmdPoolInfo, m_Context.allocator, &m_CMDPool));
    }

    void VulkanSwapchain::Present(VkQueue queue, VulkanTexture2D* toPresent, size_t width, size_t height) noexcept {
        uint32_t index = 0;
        RHINO_VKS(vkAcquireNextImageKHR(m_Context.device, m_Swapchain, ~0, VK_NULL_HANDLE, VK_NULL_HANDLE, &index));
        VkImage backbuffer = m_SwapchainImages[index];

        VkCommandBuffer cmd = VK_NULL_HANDLE;
        VkCommandBufferAllocateInfo cmdAllocInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
        cmdAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        cmdAllocInfo.commandPool = m_CMDPool;
        cmdAllocInfo.commandBufferCount = 1;
        RHINO_VKS(vkAllocateCommandBuffers(m_Context.device, &cmdAllocInfo, &cmd));

        VkCommandBufferBeginInfo cmdBegin{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
        cmdBegin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        cmdBegin.pInheritanceInfo = nullptr;
        RHINO_VKS(vkBeginCommandBuffer(cmd, &cmdBegin));

        VkImageCopy region{};
        region.srcOffset = {0, 0, 0};
        region.dstOffset = {0, 0, 0};
        region.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.srcSubresource.mipLevel = 0;
        region.srcSubresource.baseArrayLayer = 0;
        region.srcSubresource.layerCount = 1;
        region.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.dstSubresource.mipLevel = 0;
        region.dstSubresource.baseArrayLayer = 0;
        region.dstSubresource.layerCount = 1;
        region.extent = VkExtent3D{static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1};
        vkCmdCopyImage(cmd, toPresent->texture, VK_IMAGE_LAYOUT_GENERAL, backbuffer, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, 1, &region);

        RHINO_VKS(vkEndCommandBuffer(cmd));
        VkSubmitInfo submit{VK_STRUCTURE_TYPE_SUBMIT_INFO};
        submit.commandBufferCount = 1;
        submit.pCommandBuffers = &cmd;
        submit.signalSemaphoreCount = 1;
        submit.pSignalSemaphores = &m_SwapchainSyncs[index];
        RHINO_VKS(vkQueueSubmit(queue, 1, &submit, VK_NULL_HANDLE));

        VkResult swapchainStatus = VK_SUCCESS;
        VkPresentInfoKHR presentInfo{VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = &m_SwapchainSyncs[index];
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = &m_Swapchain;
        presentInfo.pImageIndices = &index;
        presentInfo.pResults = &swapchainStatus;

        RHINO_VKS(vkQueuePresentKHR(queue, &presentInfo));
        RHINO_VKS(swapchainStatus);
    }

    void VulkanSwapchain::Release() noexcept {
        for (VkSemaphore semaphore : m_SwapchainSyncs) {
            vkDestroySemaphore(m_Context.device, semaphore, m_Context.allocator);
        }
        vkDestroyCommandPool(m_Context.device, m_CMDPool, m_Context.allocator);
        vkDestroySwapchainKHR(m_Context.device, m_Swapchain, m_Context.allocator);
        vkDestroySurfaceKHR(m_Context.instance, m_Surface, m_Context.allocator);
        delete this;
    }
} // namespace RHINO::APIVulkan

#endif // ENABLE_API_VULKAN
