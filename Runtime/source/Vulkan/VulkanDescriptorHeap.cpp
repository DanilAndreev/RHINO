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
        m_HeapSize = RHINO_CEIL_TO_MULTIPLE_OF(m_DescriptorHandleIncrementSize * descriptorsCount, 256);

        VkBufferCreateInfo heapCreateInfo{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
        heapCreateInfo.flags = 0;
        heapCreateInfo.usage = VK_BUFFER_USAGE_RESOURCE_DESCRIPTOR_BUFFER_BIT_EXT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
        if (type == DescriptorHeapType::Sampler) {
            heapCreateInfo.usage |= VK_BUFFER_USAGE_SAMPLER_DESCRIPTOR_BUFFER_BIT_EXT;
        }
        heapCreateInfo.size = m_HeapSize;
        heapCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        vkCreateBuffer(m_Context.device, &heapCreateInfo, m_Context.allocator, &m_Heap);

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
        vkAllocateMemory(m_Context.device, &alloc, m_Context.allocator, &m_Memory);

        vkBindBufferMemory(m_Context.device, m_Heap, m_Memory, 0);

        VkBufferDeviceAddressInfo bufferInfo{VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO};
        bufferInfo.buffer = m_Heap;
        m_HeapGPUStartHandle = vkGetBufferDeviceAddress(m_Context.device, &bufferInfo);

        vkMapMemory(m_Context.device, m_Memory, 0, VK_WHOLE_SIZE, 0, &m_Mapped);
    }
    VkDeviceAddress VulkanDescriptorHeap::GetHeapGPUStartHandle() noexcept {
        return m_HeapGPUStartHandle;
    }

    void VulkanDescriptorHeap::WriteSRV(const RHINO::WriteBufferDescriptorDesc& desc) noexcept {
        auto* vulkanBuffer = static_cast<VulkanBuffer*>(desc.buffer);

        VkDescriptorAddressInfoEXT bufferInfo{VK_STRUCTURE_TYPE_DESCRIPTOR_ADDRESS_INFO_EXT};
        bufferInfo.address = vulkanBuffer->deviceAddress + desc.bufferOffset;
        bufferInfo.range = vulkanBuffer->size - desc.bufferOffset;
        bufferInfo.format = VK_FORMAT_UNDEFINED;

        VkDescriptorGetInfoEXT info{VK_STRUCTURE_TYPE_DESCRIPTOR_GET_INFO_EXT};
        info.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        info.data.pStorageBuffer = &bufferInfo;

        uint8_t* mem = static_cast<uint8_t*>(m_Mapped);
        EXT::vkGetDescriptorEXT(m_Context.device, &info, m_DescriptorProps.storageBufferDescriptorSize,
                                mem + desc.offsetInHeap * m_DescriptorHandleIncrementSize);
    }

    void VulkanDescriptorHeap::WriteUAV(const WriteBufferDescriptorDesc& desc) noexcept {
        auto* vulkanBuffer = static_cast<VulkanBuffer*>(desc.buffer);

        VkDescriptorAddressInfoEXT bufferInfo{VK_STRUCTURE_TYPE_DESCRIPTOR_ADDRESS_INFO_EXT};
        bufferInfo.address = vulkanBuffer->deviceAddress + desc.bufferOffset;
        bufferInfo.range = vulkanBuffer->size - desc.bufferOffset;
        bufferInfo.format = VK_FORMAT_UNDEFINED;

        VkDescriptorGetInfoEXT info{VK_STRUCTURE_TYPE_DESCRIPTOR_GET_INFO_EXT};
        info.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        info.data.pStorageBuffer = &bufferInfo;

        uint8_t* mem = static_cast<uint8_t*>(m_Mapped);
        EXT::vkGetDescriptorEXT(m_Context.device, &info, m_DescriptorProps.storageBufferDescriptorSize,
                                mem + desc.offsetInHeap * m_DescriptorHandleIncrementSize);
    }

    void VulkanDescriptorHeap::WriteCBV(const WriteBufferDescriptorDesc& desc) noexcept {
        auto* vulkanBuffer = static_cast<VulkanBuffer*>(desc.buffer);

        VkDescriptorAddressInfoEXT bufferInfo{VK_STRUCTURE_TYPE_DESCRIPTOR_ADDRESS_INFO_EXT};
        bufferInfo.address = vulkanBuffer->deviceAddress + desc.bufferOffset;
        bufferInfo.range = vulkanBuffer->size - desc.bufferOffset;
        bufferInfo.format = VK_FORMAT_UNDEFINED;

        VkDescriptorGetInfoEXT info{VK_STRUCTURE_TYPE_DESCRIPTOR_GET_INFO_EXT};
        info.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        info.data.pUniformBuffer = &bufferInfo;

        uint8_t* mem = static_cast<uint8_t*>(m_Mapped);
        EXT::vkGetDescriptorEXT(m_Context.device, &info, m_DescriptorProps.uniformBufferDescriptorSize,
                                mem + desc.offsetInHeap * m_DescriptorHandleIncrementSize);
    }

    void VulkanDescriptorHeap::WriteSRV(const WriteTexture2DDescriptorDesc& desc) noexcept {
        //TODO: validate if it is possible to create view, write this data to heap and delete vkView object;
        auto* vulkanTexture = static_cast<VulkanTexture2D*>(desc.texture);

        VkDescriptorImageInfo textureInfo{};
        textureInfo.imageLayout = vulkanTexture->layout;
        textureInfo.imageView = vulkanTexture->view;

        VkDescriptorGetInfoEXT info{VK_STRUCTURE_TYPE_DESCRIPTOR_GET_INFO_EXT};
        info.type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        info.data.pSampledImage = &textureInfo;

        uint8_t* mem = static_cast<uint8_t*>(m_Mapped);
        EXT::vkGetDescriptorEXT(m_Context.device, &info, m_DescriptorProps.sampledImageDescriptorSize,
                                mem + desc.offsetInHeap * m_DescriptorHandleIncrementSize);
    }

    void VulkanDescriptorHeap::WriteUAV(const WriteTexture2DDescriptorDesc& desc) noexcept {
        //TODO: validate if it is possible to create view, write this data to heap and delete vkView object;
        auto* vulkanTexture = static_cast<VulkanTexture2D*>(desc.texture);

        VkDescriptorImageInfo textureInfo{};
        textureInfo.imageLayout = vulkanTexture->layout;
        textureInfo.imageView = vulkanTexture->view;

        VkDescriptorGetInfoEXT info{VK_STRUCTURE_TYPE_DESCRIPTOR_GET_INFO_EXT};
        info.type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        info.data.pStorageImage = &textureInfo;

        uint8_t* mem = static_cast<uint8_t*>(m_Mapped);
        EXT::vkGetDescriptorEXT(m_Context.device, &info, m_DescriptorProps.storageImageDescriptorSize,
                                mem + desc.offsetInHeap * m_DescriptorHandleIncrementSize);
    }

    void VulkanDescriptorHeap::WriteSRV(const WriteTexture3DDescriptorDesc& desc) noexcept {
        //TODO: implement
    }

    void VulkanDescriptorHeap::WriteUAV(const WriteTexture3DDescriptorDesc& desc) noexcept {
        //TODO: implement
    }

    inline void VulkanDescriptorHeap::Release() noexcept {
        vkUnmapMemory(m_Context.device, this->m_Memory);
        vkDestroyBuffer(m_Context.device, this->m_Heap, m_Context.allocator);
        vkFreeMemory(m_Context.device, this->m_Memory, m_Context.allocator);
        delete this;
    }
}// namespace RHINO::APIVulkan

#endif// ENABLE_API_VULKAN
