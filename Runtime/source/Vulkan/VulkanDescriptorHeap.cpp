#ifdef ENABLE_API_VULKAN

#include "VulkanDescriptorHeap.h"
#include "VulkanAPI.h"

namespace RHINO::APIVulkan {
    void VulkanDescriptorHeap::WriteSRV(const RHINO::WriteBufferDescriptorDesc& desc) noexcept {
        auto* vulkanBuffer = static_cast<VulkanBuffer*>(desc.buffer);

        VkDescriptorAddressInfoEXT bufferInfo{VK_STRUCTURE_TYPE_DESCRIPTOR_ADDRESS_INFO_EXT};
        bufferInfo.address = vulkanBuffer->deviceAddress + desc.bufferOffset;
        bufferInfo.range = vulkanBuffer->size - desc.bufferOffset;
        VkDescriptorGetInfoEXT info{VK_STRUCTURE_TYPE_DESCRIPTOR_GET_INFO_EXT};
        info.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        info.data.pStorageBuffer = &bufferInfo;

        uint8_t* mem = static_cast<uint8_t*>(mapped);
        EXT::vkGetDescriptorEXT(device, &info, descriptorProps.storageBufferDescriptorSize, mem + desc.offsetInHeap * descriptorHandleIncrementSize);
    }
    void VulkanDescriptorHeap::WriteUAV(const WriteBufferDescriptorDesc& desc) noexcept {
        auto* vulkanBuffer = static_cast<VulkanBuffer*>(desc.buffer);

        VkDescriptorAddressInfoEXT bufferInfo{VK_STRUCTURE_TYPE_DESCRIPTOR_ADDRESS_INFO_EXT};
        bufferInfo.address = vulkanBuffer->deviceAddress + desc.bufferOffset;
        bufferInfo.range = vulkanBuffer->size - desc.bufferOffset;
        VkDescriptorGetInfoEXT info{VK_STRUCTURE_TYPE_DESCRIPTOR_GET_INFO_EXT};
        info.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        info.data.pStorageBuffer = &bufferInfo;

        uint8_t* mem = static_cast<uint8_t*>(mapped);
        EXT::vkGetDescriptorEXT(device, &info, descriptorProps.storageBufferDescriptorSize, mem + desc.offsetInHeap * descriptorHandleIncrementSize);
    }
    void VulkanDescriptorHeap::WriteCBV(const WriteBufferDescriptorDesc& desc) noexcept {
        auto* vulkanBuffer = static_cast<VulkanBuffer*>(desc.buffer);

        VkDescriptorAddressInfoEXT bufferInfo{VK_STRUCTURE_TYPE_DESCRIPTOR_ADDRESS_INFO_EXT};
        bufferInfo.address = vulkanBuffer->deviceAddress + desc.bufferOffset;
        bufferInfo.range = vulkanBuffer->size - desc.bufferOffset;
        VkDescriptorGetInfoEXT info{VK_STRUCTURE_TYPE_DESCRIPTOR_GET_INFO_EXT};
        info.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        info.data.pUniformBuffer = &bufferInfo;

        uint8_t* mem = static_cast<uint8_t*>(mapped);
        EXT::vkGetDescriptorEXT(device, &info, descriptorProps.uniformBufferDescriptorSize, mem + desc.offsetInHeap * descriptorHandleIncrementSize);
    }
    void VulkanDescriptorHeap::WriteSRV(const WriteTexture2DSRVDesc& desc) noexcept {
        auto* vulkanTexture = static_cast<VulkanTexture2D*>(desc.texture);

        VkDescriptorImageInfo textureInfo{};
        textureInfo.imageLayout = vulkanTexture->layout;
        textureInfo.imageView = vulkanTexture->view;
        VkDescriptorGetInfoEXT info{VK_STRUCTURE_TYPE_DESCRIPTOR_GET_INFO_EXT};
        info.type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        info.data.pSampledImage = &textureInfo;

        uint8_t* mem = static_cast<uint8_t*>(mapped);
        EXT::vkGetDescriptorEXT(device, &info, descriptorProps.sampledImageDescriptorSize, mem + desc.offsetInHeap * descriptorHandleIncrementSize);
    }
    void VulkanDescriptorHeap::WriteUAV(const WriteTexture2DSRVDesc& desc) noexcept {
        auto* vulkanTexture = static_cast<VulkanTexture2D*>(desc.texture);

        VkDescriptorImageInfo textureInfo{};
        textureInfo.imageLayout = vulkanTexture->layout;
        textureInfo.imageView = vulkanTexture->view;
        VkDescriptorGetInfoEXT info{VK_STRUCTURE_TYPE_DESCRIPTOR_GET_INFO_EXT};
        info.type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        info.data.pStorageImage = &textureInfo;

        uint8_t* mem = static_cast<uint8_t*>(mapped);
        EXT::vkGetDescriptorEXT(device, &info, descriptorProps.storageImageDescriptorSize, mem + desc.offsetInHeap * descriptorHandleIncrementSize);
    }
    void VulkanDescriptorHeap::WriteSRV(const WriteTexture3DSRVDesc& desc) noexcept {
    }
    void VulkanDescriptorHeap::WriteUAV(const WriteTexture3DSRVDesc& desc) noexcept {
    }
}// namespace RHINO::APIVulkan

#endif// ENABLE_API_VULKAN
