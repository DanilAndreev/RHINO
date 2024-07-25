#pragma once

#ifdef ENABLE_API_VULKAN

#include "VulkanBackendTypes.h"

namespace RHINO::APIVulkan {
    class VulkanDescriptorHeap : public DescriptorHeap {
    public:
        static constexpr VkDescriptorType CDBSRVUAVTypes[6] = {
                VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,        VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,  VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER,
                VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER};
        static constexpr VkDescriptorType CBVTypes[1] = {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER};
        static constexpr VkDescriptorType SRVTypes[3] = {VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
                                                         VK_DESCRIPTOR_TYPE_STORAGE_BUFFER};
        static constexpr VkDescriptorType UAVTypes[3] = {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER,
                                                         VK_DESCRIPTOR_TYPE_STORAGE_BUFFER};

    public:
        void Initialize(const char* name, DescriptorHeapType type, size_t descriptorsCount, VulkanObjectContext context) noexcept;
        VkDeviceAddress GetHeapGPUStartHandle() noexcept;

    public:
        void WriteSRV(const WriteBufferDescriptorDesc& desc) noexcept final;
        void WriteUAV(const WriteBufferDescriptorDesc& desc) noexcept final;
        void WriteCBV(const WriteBufferDescriptorDesc& desc) noexcept final;
        void WriteSRV(const WriteTexture2DDescriptorDesc& desc) noexcept final;
        void WriteUAV(const WriteTexture2DDescriptorDesc& desc) noexcept final;
        void WriteSRV(const WriteTexture3DDescriptorDesc& desc) noexcept final;
        void WriteUAV(const WriteTexture3DDescriptorDesc& desc) noexcept final;
        void WriteSRV(const WriteTLASDescriptorDesc& desc) noexcept final;
        void WriteSMP(Sampler* sampler, size_t offsetInHeap) noexcept final;

    public:
        void Release() noexcept final;

    private:
        VkImageView& InvalidateSlot(uint32_t descriptorSlot) noexcept;

    private:
        uint32_t m_HeapSize = 0;
        VkBuffer m_Heap = VK_NULL_HANDLE;
        VkDeviceMemory m_Memory = VK_NULL_HANDLE;
        void* m_Mapped = nullptr;
        VkDeviceAddress m_HeapGPUStartHandle = 0;
        size_t m_DescriptorHandleIncrementSize = 0;

        VkPhysicalDeviceDescriptorBufferPropertiesEXT m_DescriptorProps{};
        VulkanObjectContext m_Context = {};

        std::vector<VkImageView> m_ImageViewPerDescriptor{};
    };

} // namespace RHINO::APIVulkan

#endif // ENABLE_API_VULKAN
