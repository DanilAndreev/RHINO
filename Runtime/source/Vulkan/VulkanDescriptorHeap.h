#pragma once

#ifdef ENABLE_API_VULKAN

#include "VulkanBackendTypes.h"

namespace RHINO::APIVulkan {
    class VulkanDescriptorHeap final : public DescriptorHeap {
    public:
        static constexpr VkDescriptorType CDBSRVUAVTypes[6] = {
                VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,        VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
                VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER,
                VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,       VK_DESCRIPTOR_TYPE_STORAGE_BUFFER};
        static constexpr VkDescriptorType CBVTypes[1] = {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER};
        static constexpr VkDescriptorType SRVTypes[3] = {VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER,
                                                                   VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
                                                                   VK_DESCRIPTOR_TYPE_STORAGE_BUFFER};
        static constexpr VkDescriptorType UAVTypes[3] = {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
                                                                   VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER,
                                                                   VK_DESCRIPTOR_TYPE_STORAGE_BUFFER};

    public:
        void WriteSRV(const WriteBufferDescriptorDesc& desc) noexcept final;
        void WriteUAV(const WriteBufferDescriptorDesc& desc) noexcept final;
        void WriteCBV(const WriteBufferDescriptorDesc& desc) noexcept final;
        void WriteSRV(const WriteTexture2DSRVDesc& desc) noexcept final;
        void WriteUAV(const WriteTexture2DSRVDesc& desc) noexcept final;
        void WriteSRV(const WriteTexture3DSRVDesc& desc) noexcept final;
        void WriteUAV(const WriteTexture3DSRVDesc& desc) noexcept final;

    public:
        uint32_t heapSize = 0;
        VkBuffer heap = VK_NULL_HANDLE;
        VkDeviceMemory memory = VK_NULL_HANDLE;
        void* mapped = nullptr;
        VkDeviceAddress heapGPUStartHandle = 0;
        size_t descriptorHandleIncrementSize = 0;

        VkPhysicalDeviceDescriptorBufferPropertiesEXT descriptorProps{};

        VkDevice device = VK_NULL_HANDLE;
    };

} // namespace RHINO::APIVulkan

#endif // ENABLE_API_VULKAN
