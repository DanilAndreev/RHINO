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

    inline VkImageUsageFlags ToVkImageUsage(ResourceUsage usage) noexcept {
        VkImageUsageFlags result = 0;
        if (bool(usage & ResourceUsage::ShaderResource)) {
            result |= VK_IMAGE_USAGE_SAMPLED_BIT;
        }
        if (bool(usage & ResourceUsage::UnorderedAccess)) {
            result |= VK_IMAGE_USAGE_STORAGE_BIT;
        }
        if (bool(usage & ResourceUsage::CopySource)) {
            result |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
        }
        if (bool(usage & ResourceUsage::CopyDest)) {
            result |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        }
        return result;
    }

    inline VkFormat ToVkFormat(TextureFormat format) noexcept {
        switch (format) {
            case TextureFormat::R8G8B8A8_UNORM:
                return VK_FORMAT_R8G8B8A8_UNORM;
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
                return VK_FORMAT_R8G8B8A8_UNORM;
        }
    }

    inline VkAccessFlags ToVulkanResourceState(ResourceState state) noexcept {
        // TODO: possibly refactor to use common read/write flags VK_ACCESS_MEMORY_READ_BIT and VK_ACCESS_MEMORY_WRITE_BIT

        switch (state) {
            case ResourceState::ConstantBuffer:
                return VK_ACCESS_UNIFORM_READ_BIT;
            case ResourceState::UnorderedAccess:
                return VK_ACCESS_SHADER_WRITE_BIT;
            case ResourceState::ShaderResource:
                return VK_ACCESS_SHADER_READ_BIT;
            case ResourceState::IndirectArgument:
                return VK_ACCESS_INDIRECT_COMMAND_READ_BIT;
            case ResourceState::CopyDest:
                return VK_ACCESS_TRANSFER_WRITE_BIT;
            case ResourceState::CopySource:
                return VK_ACCESS_TRANSFER_READ_BIT;
            case ResourceState::HostWrite:
                return VK_ACCESS_HOST_WRITE_BIT;
            case ResourceState::HostRead:
                return VK_ACCESS_HOST_READ_BIT;
            case ResourceState::RTAccelerationStructure:
                return VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR;

            default:
                assert(0);
                return VK_ACCESS_NONE;
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

    inline VkCompareOp ToVkCompareOp(ComparisonFunction func) noexcept {
        switch (func) {
            case ComparisonFunction::Never:
                return VK_COMPARE_OP_NEVER;
            case ComparisonFunction::Less:
                return VK_COMPARE_OP_LESS;
            case ComparisonFunction::Equal:
                return VK_COMPARE_OP_EQUAL;
            case ComparisonFunction::LessEqual:
                return VK_COMPARE_OP_LESS_OR_EQUAL;
            case ComparisonFunction::Greater:
                return VK_COMPARE_OP_GREATER;
            case ComparisonFunction::NotEqual:
                return VK_COMPARE_OP_NOT_EQUAL;
            case ComparisonFunction::GreaterEqual:
                return VK_COMPARE_OP_GREATER_OR_EQUAL;
            case ComparisonFunction::Always:
                return VK_COMPARE_OP_ALWAYS;
            default:
                assert(0);
                return VK_COMPARE_OP_NEVER;
        }
    }

    inline VkSamplerAddressMode ToVkSamplerAddressMode(TextureAddressMode mode) noexcept {
        switch (mode) {
            case TextureAddressMode::Wrap:
                return VK_SAMPLER_ADDRESS_MODE_REPEAT;
            case TextureAddressMode::Mirror:
                return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
            case TextureAddressMode::Clamp:
                return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            case TextureAddressMode::Border:
                return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
            case TextureAddressMode::MirrorOnce:
                return VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE;
            default:
                assert(0);
                return VK_SAMPLER_ADDRESS_MODE_REPEAT;
        }
    }

    inline VkBorderColor ToVkBorderColor(BorderColor color) noexcept {
        switch (color) {
            case BorderColor::TransparentBlack:
                return VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
            case BorderColor::OpaqueBlack:
                return VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
            case BorderColor::OpaqueWhite:
                return VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
            default:
                assert(0);
                return VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
        }
    }

    struct VulkanMinMagMipFilters {
        VkFilter min;
        VkFilter mag;
        VkSamplerMipmapMode mip;
    };

    inline VulkanMinMagMipFilters ToMTLMinMagMipFilter(TextureFilter filter) noexcept {
        switch (filter) {
            case TextureFilter::MinMagMipPoint:
            case TextureFilter::ComparisonMinMagMipPoint:
                return {VK_FILTER_NEAREST, VK_FILTER_NEAREST, VK_SAMPLER_MIPMAP_MODE_NEAREST};
            case TextureFilter::MinMagPointMipLinear:
            case TextureFilter::ComparisonMinMagPointMipLinear:
                return {VK_FILTER_NEAREST, VK_FILTER_NEAREST, VK_SAMPLER_MIPMAP_MODE_LINEAR};
            case TextureFilter::MinPointMagLinearMipPoint:
            case TextureFilter::ComparisonMinPointMagLinearMipPoint:
                return {VK_FILTER_NEAREST, VK_FILTER_LINEAR, VK_SAMPLER_MIPMAP_MODE_NEAREST};
            case TextureFilter::MinPointMagMipLinear:
            case TextureFilter::ComparisonMinPointMagMipLinear:
                return {VK_FILTER_NEAREST, VK_FILTER_LINEAR, VK_SAMPLER_MIPMAP_MODE_LINEAR};
            case TextureFilter::MinLinearMagMipPoint:
            case TextureFilter::ComparisonMinLinearMagMipPoint:
                return {VK_FILTER_LINEAR, VK_FILTER_NEAREST, VK_SAMPLER_MIPMAP_MODE_NEAREST};
            case TextureFilter::MinLinearMagPointMipLinear:
            case TextureFilter::ComparisonMinLinearMagPointMipLinear:
                return {VK_FILTER_LINEAR, VK_FILTER_NEAREST, VK_SAMPLER_MIPMAP_MODE_LINEAR};
            case TextureFilter::MinMagLinearMipPoint:
            case TextureFilter::ComparisonMinMagLinearMipPoint:
                return {VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_MIPMAP_MODE_NEAREST};
            case TextureFilter::MinMagMipLinear:
            case TextureFilter::ComparisonMinMagMipLinear:
                return {VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_MIPMAP_MODE_LINEAR};

            case TextureFilter::Anisotrophic:
            case TextureFilter::ComparisonAnisotrophic:
                return {VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_MIPMAP_MODE_LINEAR};
            default:
                assert(0);
                return {VK_FILTER_NEAREST, VK_FILTER_NEAREST, VK_SAMPLER_MIPMAP_MODE_NEAREST};
        }
    }
} // namespace RHINO::APIVulkan::Convert

#endif // ENABLE_API_VULKAN
