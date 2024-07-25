#ifdef ENABLE_API_VULKAN

#include "VulkanDescriptorHeap.h"
#include "VulkanAPI.h"
#include "VulkanUtils.h"

namespace RHINO::APIVulkan {
    void VulkanDescriptorHeap::Initialize(const char* name, DescriptorHeapType type, size_t descriptorsCount,
                                          VulkanObjectContext context) noexcept {
        m_Context = context;

        VkPhysicalDeviceDescriptorBufferPropertiesEXT descriptorProps{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_BUFFER_PROPERTIES_EXT};
        VkPhysicalDeviceProperties2 props{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2};
        props.pNext = &descriptorProps;
        vkGetPhysicalDeviceProperties2(m_Context.physicalDevice, &props);

        m_DescriptorProps = descriptorProps;

        m_DescriptorHandleIncrementSize = CalculateDescriptorHandleIncrementSize(type, descriptorProps);

        // TODO: 256 is just a magic number for RTX 3080TI.
        //  For some reason descriptorProps.descriptorBufferOffsetAlignment != 256.
        //  Research valid parameter for this one.
        const VkDeviceSize dbAlignment = descriptorProps.descriptorBufferOffsetAlignment;
        m_HeapSize = RHINO_CEIL_TO_MULTIPLE_OF(m_DescriptorHandleIncrementSize * descriptorsCount + 256, dbAlignment);

        VkBufferCreateInfo heapCreateInfo{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
        heapCreateInfo.flags = 0;
        heapCreateInfo.usage = VK_BUFFER_USAGE_RESOURCE_DESCRIPTOR_BUFFER_BIT_EXT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
        if (type == DescriptorHeapType::Sampler) {
            heapCreateInfo.usage |= VK_BUFFER_USAGE_SAMPLER_DESCRIPTOR_BUFFER_BIT_EXT;
        }
        heapCreateInfo.size = m_HeapSize;
        heapCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        RHINO_VKS(vkCreateBuffer(m_Context.device, &heapCreateInfo, m_Context.allocator, &m_Heap));

        // Create the memory backing up the buffer handle
        VkMemoryRequirements memReqs;
        vkGetBufferMemoryRequirements(m_Context.device, m_Heap, &memReqs);
        VkPhysicalDeviceMemoryProperties memoryProps;
        vkGetPhysicalDeviceMemoryProperties(m_Context.physicalDevice, &memoryProps);

        VkMemoryAllocateFlagsInfo allocateFlagsInfo{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO};
        allocateFlagsInfo.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT;

        constexpr auto memoryFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
        VkMemoryAllocateInfo alloc{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
        alloc.pNext = &allocateFlagsInfo;
        alloc.allocationSize = memReqs.size;
        alloc.memoryTypeIndex = SelectMemoryType(0xffffff, memoryFlags, m_Context);
        RHINO_VKS(vkAllocateMemory(m_Context.device, &alloc, m_Context.allocator, &m_Memory));

        RHINO_VKS(vkBindBufferMemory(m_Context.device, m_Heap, m_Memory, 0));

        VkBufferDeviceAddressInfo bufferInfo{VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO};
        bufferInfo.buffer = m_Heap;
        m_HeapGPUStartHandle = vkGetBufferDeviceAddress(m_Context.device, &bufferInfo);

        RHINO_VKS(vkMapMemory(m_Context.device, m_Memory, 0, VK_WHOLE_SIZE, 0, &m_Mapped));

        m_ImageViewPerDescriptor.resize(descriptorsCount);
    }
    VkDeviceAddress VulkanDescriptorHeap::GetHeapGPUStartHandle() noexcept {
        return m_HeapGPUStartHandle;
    }

    void VulkanDescriptorHeap::WriteSRV(const WriteBufferDescriptorDesc& desc) noexcept {
        InvalidateSlot(desc.offsetInHeap);
        auto* vulkanBuffer = INTERPRET_AS<VulkanBuffer*>(desc.buffer);

        VkDescriptorAddressInfoEXT bufferInfo{VK_STRUCTURE_TYPE_DESCRIPTOR_ADDRESS_INFO_EXT};
        bufferInfo.address = vulkanBuffer->deviceAddress + desc.bufferOffset;
        bufferInfo.range = vulkanBuffer->size - desc.bufferOffset;
        bufferInfo.format = VK_FORMAT_UNDEFINED;

        VkDescriptorGetInfoEXT info{VK_STRUCTURE_TYPE_DESCRIPTOR_GET_INFO_EXT};
        info.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        info.data.pStorageBuffer = &bufferInfo;

        auto* mem = static_cast<uint8_t*>(m_Mapped);
        EXT::vkGetDescriptorEXT(m_Context.device, &info, m_DescriptorProps.storageBufferDescriptorSize,
                                mem + desc.offsetInHeap * m_DescriptorHandleIncrementSize);
    }

    void VulkanDescriptorHeap::WriteUAV(const WriteBufferDescriptorDesc& desc) noexcept {
        InvalidateSlot(desc.offsetInHeap);
        auto* vulkanBuffer = INTERPRET_AS<VulkanBuffer*>(desc.buffer);

        VkDescriptorAddressInfoEXT bufferInfo{VK_STRUCTURE_TYPE_DESCRIPTOR_ADDRESS_INFO_EXT};
        bufferInfo.address = vulkanBuffer->deviceAddress + desc.bufferOffset;
        bufferInfo.range = vulkanBuffer->size - desc.bufferOffset;
        bufferInfo.format = VK_FORMAT_UNDEFINED;

        VkDescriptorGetInfoEXT info{VK_STRUCTURE_TYPE_DESCRIPTOR_GET_INFO_EXT};
        info.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        info.data.pStorageBuffer = &bufferInfo;

        auto* mem = static_cast<uint8_t*>(m_Mapped);
        EXT::vkGetDescriptorEXT(m_Context.device, &info, m_DescriptorProps.storageBufferDescriptorSize,
                                mem + desc.offsetInHeap * m_DescriptorHandleIncrementSize);
    }

    void VulkanDescriptorHeap::WriteCBV(const WriteBufferDescriptorDesc& desc) noexcept {
        InvalidateSlot(desc.offsetInHeap);
        auto* vulkanBuffer = INTERPRET_AS<VulkanBuffer*>(desc.buffer);

        VkDescriptorAddressInfoEXT bufferInfo{VK_STRUCTURE_TYPE_DESCRIPTOR_ADDRESS_INFO_EXT};
        bufferInfo.address = vulkanBuffer->deviceAddress + desc.bufferOffset;
        bufferInfo.range = vulkanBuffer->size - desc.bufferOffset;
        bufferInfo.format = VK_FORMAT_UNDEFINED;

        VkDescriptorGetInfoEXT info{VK_STRUCTURE_TYPE_DESCRIPTOR_GET_INFO_EXT};
        info.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        info.data.pUniformBuffer = &bufferInfo;

        auto* mem = static_cast<uint8_t*>(m_Mapped);
        EXT::vkGetDescriptorEXT(m_Context.device, &info, m_DescriptorProps.uniformBufferDescriptorSize,
                                mem + desc.offsetInHeap * m_DescriptorHandleIncrementSize);
    }

    void VulkanDescriptorHeap::WriteSRV(const WriteTexture2DDescriptorDesc& desc) noexcept {
        VkImageView& view = InvalidateSlot(desc.offsetInHeap);
        auto* vulkanTexture = INTERPRET_AS<VulkanTexture2D*>(desc.texture);

        VkImageViewCreateInfo viewInfo{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
        viewInfo.flags = 0;
        viewInfo.format = vulkanTexture->origimalFormat;
        viewInfo.image = vulkanTexture->texture;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = VK_WHOLE_SIZE;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = VK_WHOLE_SIZE;
        RHINO_VKS(vkCreateImageView(m_Context.device, &viewInfo, m_Context.allocator, &view));

        VkDescriptorImageInfo textureInfo{};
        textureInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
        textureInfo.imageView = view;

        VkDescriptorGetInfoEXT info{VK_STRUCTURE_TYPE_DESCRIPTOR_GET_INFO_EXT};
        info.type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        info.data.pSampledImage = &textureInfo;

        auto* mem = static_cast<uint8_t*>(m_Mapped);
        EXT::vkGetDescriptorEXT(m_Context.device, &info, m_DescriptorProps.sampledImageDescriptorSize,
                                mem + desc.offsetInHeap * m_DescriptorHandleIncrementSize);
    }

    void VulkanDescriptorHeap::WriteUAV(const WriteTexture2DDescriptorDesc& desc) noexcept {
        VkImageView& view = InvalidateSlot(desc.offsetInHeap);
        auto* vulkanTexture = INTERPRET_AS<VulkanTexture2D*>(desc.texture);

        VkImageViewCreateInfo viewInfo{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
        viewInfo.flags = 0;
        viewInfo.format = vulkanTexture->origimalFormat;
        viewInfo.image = vulkanTexture->texture;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = VK_WHOLE_SIZE;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = VK_WHOLE_SIZE;
        RHINO_VKS(vkCreateImageView(m_Context.device, &viewInfo, m_Context.allocator, &view));

        VkDescriptorImageInfo textureInfo{};
        textureInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
        textureInfo.imageView = view;

        VkDescriptorGetInfoEXT info{VK_STRUCTURE_TYPE_DESCRIPTOR_GET_INFO_EXT};
        info.type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        info.data.pStorageImage = &textureInfo;

        auto* mem = static_cast<uint8_t*>(m_Mapped);
        EXT::vkGetDescriptorEXT(m_Context.device, &info, m_DescriptorProps.storageImageDescriptorSize,
                                mem + desc.offsetInHeap * m_DescriptorHandleIncrementSize);
    }

    void VulkanDescriptorHeap::WriteSRV(const WriteTexture3DDescriptorDesc& desc) noexcept {
        InvalidateSlot(desc.offsetInHeap);
        //TODO: implement
    }

    void VulkanDescriptorHeap::WriteUAV(const WriteTexture3DDescriptorDesc& desc) noexcept {
        InvalidateSlot(desc.offsetInHeap);
        // TODO: implement
    }

    void VulkanDescriptorHeap::WriteSRV(const WriteTLASDescriptorDesc& desc) noexcept {
        InvalidateSlot(desc.offsetInHeap);
        // TODO: implement
    }

    void VulkanDescriptorHeap::WriteSMP(Sampler* sampler, size_t offsetInHeap) noexcept {
        auto* vulkanSampler = INTERPRET_AS<VulkanSampler*>(sampler);

        VkDescriptorGetInfoEXT info{VK_STRUCTURE_TYPE_DESCRIPTOR_GET_INFO_EXT};
        info.type = VK_DESCRIPTOR_TYPE_SAMPLER;
        info.data.pSampler = &vulkanSampler->sampler;

        auto* mem = static_cast<uint8_t*>(m_Mapped);
        EXT::vkGetDescriptorEXT(m_Context.device, &info, m_DescriptorProps.samplerDescriptorSize,
                                mem + offsetInHeap * m_DescriptorHandleIncrementSize);
    }

    void VulkanDescriptorHeap::Release() noexcept {
        for (VkImageView view : m_ImageViewPerDescriptor) {
            vkDestroyImageView(m_Context.device, view, m_Context.allocator);
        }
        vkUnmapMemory(m_Context.device, this->m_Memory);
        vkDestroyBuffer(m_Context.device, this->m_Heap, m_Context.allocator);
        vkFreeMemory(m_Context.device, this->m_Memory, m_Context.allocator);
        delete this;
    }

    VkImageView& VulkanDescriptorHeap::InvalidateSlot(uint32_t descriptorSlot) noexcept {
        if (m_ImageViewPerDescriptor[descriptorSlot]) {
            vkDestroyImageView(m_Context.device, m_ImageViewPerDescriptor[descriptorSlot], m_Context.allocator);
        }
        m_ImageViewPerDescriptor[descriptorSlot] = VK_NULL_HANDLE;
        return m_ImageViewPerDescriptor[descriptorSlot];
    }
} // namespace RHINO::APIVulkan

#endif// ENABLE_API_VULKAN
