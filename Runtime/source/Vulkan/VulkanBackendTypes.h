#pragma once

#ifdef ENABLE_API_VULKAN

namespace RHINO::APIVulkan {
    class VulkanDescriptorHeap;

    class VulkanBuffer : public Buffer {
    public:
        VkBuffer buffer = VK_NULL_HANDLE;
        uint32_t size = 0;
        VkDeviceMemory memory = VK_NULL_HANDLE;
        void* mapped = nullptr;
        VkDeviceAddress deviceAddress = 0;
    };

    class VulkanTexture2D : public Texture2D {
    public:
        VkImage texture = VK_NULL_HANDLE;
        VkImageView view = VK_NULL_HANDLE;
        VkImageLayout layout = VK_IMAGE_LAYOUT_GENERAL;
    };

    class VulkanRTPSO : public RTPSO {
    public:
    };

    class VulkanComputePSO : public ComputePSO {
    public:
        VkPipeline PSO = VK_NULL_HANDLE;
        VkShaderModule shaderModule = VK_NULL_HANDLE;
        VkPipelineLayout layout = VK_NULL_HANDLE;
        std::map<size_t, std::pair<DescriptorRangeType, size_t>> heapOffsetsBySpaces{};
    };
}// namespace RHINO::APIVulkan

#endif// ENABLE_API_VULKAN