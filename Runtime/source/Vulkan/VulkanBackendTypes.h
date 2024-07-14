#pragma once
#include "RHINOTypesImpl.h"

#ifdef ENABLE_API_VULKAN

namespace RHINO::APIVulkan {
    class VulkanDescriptorHeap;

    struct VulkanObjectContext {
        VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
        VkDevice device = VK_NULL_HANDLE;
        VkAllocationCallbacks* allocator = nullptr;
    };

    class VulkanBuffer : public BufferBase {
    public:
        VkBuffer buffer = VK_NULL_HANDLE;
        uint32_t size = 0;
        VkDeviceMemory memory = VK_NULL_HANDLE;
        void* mapped = nullptr;
        VkDeviceAddress deviceAddress = 0;
        VulkanObjectContext context = {};

    public:
        void Release() noexcept final {
            vkDestroyBuffer(this->context.device, this->buffer, this->context.allocator);
            vkFreeMemory(this->context.device, this->memory, this->context.allocator);
            delete this;
        }
    };

    class VulkanTexture2D : public Texture2DBase {
    public:
        VkImage texture = VK_NULL_HANDLE;
        VkImageView view = VK_NULL_HANDLE;
        VkImageLayout layout = VK_IMAGE_LAYOUT_GENERAL;
        VulkanObjectContext context = {};

    public:
        void Release() noexcept final {
            vkDestroyImageView(this->context.device, this->view, this->context.allocator);
            vkDestroyImage(this->context.device, this->texture, this->context.allocator);
            delete this;
        }
    };

    class VulkanTexture3D : public Texture2DBase {
    public:
        VkImage texture = VK_NULL_HANDLE;
        VkImageView view = VK_NULL_HANDLE;
        VkImageLayout layout = VK_IMAGE_LAYOUT_GENERAL;
        VulkanObjectContext context = {};

    public:
        void Release() noexcept final {
            vkDestroyImageView(this->context.device, this->view, this->context.allocator);
            vkDestroyImage(this->context.device, this->texture, this->context.allocator);
            delete this;
        }
    };

    class VulkanRTPSO : public RTPSO {
    public:
        VulkanObjectContext context = {};

    public:
        void Release() noexcept final {
            //TODO: implement
        }
    };

    class VulkanComputePSO : public ComputePSO {
    public:
        VkPipeline PSO = VK_NULL_HANDLE;
        VkShaderModule shaderModule = VK_NULL_HANDLE;
        VkPipelineLayout layout = VK_NULL_HANDLE;
        std::map<size_t, std::pair<DescriptorRangeType, size_t>> heapOffsetsInDescriptorsBySpaces{};
        VulkanObjectContext context = {};

    public:
        void Release() noexcept final {
            vkDestroyPipelineLayout(this->context.device, this->layout, this->context.allocator);
            vkDestroyPipeline(this->context.device, this->PSO, this->context.allocator);
            vkDestroyShaderModule(this->context.device, this->shaderModule, this->context.allocator);
            delete this;
        }
    };

    class VulkanBLAS : public BLASBase {
    public:
        VkAccelerationStructureKHR accelerationStructure = VK_NULL_HANDLE;
        VkBuffer buffer = VK_NULL_HANDLE;
        VulkanObjectContext context = {};

    public:
        void Release() noexcept final {
            vkDestroyAccelerationStructureKHR(this->context.device, this->accelerationStructure, this->context.allocator);
            vkDestroyBuffer(this->context.device, this->buffer, this->context.allocator);
            delete this;
        }
    };

    class VulkanTLAS : public TLASBase {
    public:
        VkAccelerationStructureKHR accelerationStructure = VK_NULL_HANDLE;
        VkBuffer buffer;
        VulkanObjectContext context = {};

    public:
        void Release() noexcept final {
            vkDestroyAccelerationStructureKHR(this->context.device, this->accelerationStructure, this->context.allocator);
            vkDestroyBuffer(this->context.device, this->buffer, this->context.allocator);
            delete this;
        }
    };

    class VulkanSemaphore : public Semaphore {
    public:
        VkSemaphore semaphore = VK_NULL_HANDLE;
        VulkanObjectContext context = {};

    public:
        void Release() noexcept final {
            vkDestroySemaphore(this->context.device, this->semaphore, this->context.allocator);
            delete this;
        }
    };
}// namespace RHINO::APIVulkan

#endif// ENABLE_API_VULKAN