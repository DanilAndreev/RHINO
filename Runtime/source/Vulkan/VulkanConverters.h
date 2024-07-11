#pragma once

#ifdef ENABLE_API_VULKAN

#include "VulkanBackendTypes.h"

namespace RHINO::APIVulkan::Convert {
    inline VkDescriptorType ToVkDescriptorType(DescriptorType type) noexcept {
        switch (type) {
            case DescriptorType::BufferCBV:
                return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            case DescriptorType::BufferSRV:
            case DescriptorType::BufferUAV:
                return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            case DescriptorType::Texture2DSRV:
                return VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
            case DescriptorType::Texture2DUAV:
                return VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
            case DescriptorType::Texture3DSRV:
                return VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
            case DescriptorType::Texture3DUAV:
                return VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
            case DescriptorType::Sampler:
                return VK_DESCRIPTOR_TYPE_SAMPLER;
            default:
                assert(0);
                return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        }
    }

    inline VkBufferUsageFlags ToVkBufferUsage(ResourceUsage usage) noexcept {
        VkBufferUsageFlags result = 0;
        if (bool(usage & ResourceUsage::VertexBuffer)) {
            result |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
        }
        if (bool(usage & ResourceUsage::IndexBuffer)) {
            result |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
        }
        if (bool(usage & ResourceUsage::ConstantBuffer)) {
            result |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
        }
        if (bool(usage & ResourceUsage::ShaderResource)) {
            result |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
        }
        if (bool(usage & ResourceUsage::UnorderedAccess)) {
            result |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
        }
        if (bool(usage & ResourceUsage::Indirect)) {
            result |= VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT;
        }
        if (bool(usage & ResourceUsage::CopySource)) {
            result |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        }
        if (bool(usage & ResourceUsage::CopyDest)) {
            result |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        }
        return result;
    }

    inline VkFormat ToVkFormat(TextureFormat format) noexcept {
        switch (format) {
            case TextureFormat::R32G32B32A32_FLOAT:
                return VK_FORMAT_R32G32B32A32_SFLOAT;
            case TextureFormat::R32G32B32A32_UINT:
                return VK_FORMAT_R32G32B32A32_UINT;
            case TextureFormat::R32G32B32A32_SINT:
                return VK_FORMAT_R32G32B32A32_SINT;
            case TextureFormat::R32G32B32_FLOAT:
                return VK_FORMAT_R32G32B32_SFLOAT;
            case TextureFormat::R32G32B32_UINT:
                return VK_FORMAT_R32G32B32_UINT;
            case TextureFormat::R32G32B32_SINT:
                return VK_FORMAT_R32G32B32_SINT;
            case TextureFormat::R32_FLOAT:
                return VK_FORMAT_R32_SFLOAT;
            case TextureFormat::R32_UINT:
                return VK_FORMAT_R32_UINT;
            case TextureFormat::R32_SINT:
                return VK_FORMAT_R32_SINT;
            default:
                assert(0);
                return VK_FORMAT_R32G32B32A32_SFLOAT;
        }
    }

    inline VkIndexType ToVkIndexType(IndexFormat format) noexcept {
        switch (format) {
            case IndexFormat::R32_UINT:
                return VK_INDEX_TYPE_UINT32;
            case IndexFormat::R16_UINT:
                return VK_INDEX_TYPE_UINT16;
            default:
                assert(0);
                return VK_INDEX_TYPE_UINT32;
        }
    }
} // namespace RHINO::APIVulkan::Convert

#endif // ENABLE_API_VULKAN
