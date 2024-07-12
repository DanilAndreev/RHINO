#pragma once

#ifdef ENABLE_API_VULKAN

#include "VulkanAPI.h"
#include "VulkanDescriptorHeap.h"

namespace RHINO::APIVulkan {
    inline size_t CalculateDescriptorHandleIncrementSize(DescriptorHeapType heapType, const VkPhysicalDeviceDescriptorBufferPropertiesEXT& descriptorProps) noexcept {
        size_t typesSize = 0;
        const VkDescriptorType* types;

        static constexpr VkDescriptorType SamplerTypes[1] = {VK_DESCRIPTOR_TYPE_SAMPLER};
        switch (heapType) {
            case DescriptorHeapType::SRV_CBV_UAV: {
                typesSize = RHINO_ARR_SIZE(VulkanDescriptorHeap::CDBSRVUAVTypes);
                types = VulkanDescriptorHeap::CDBSRVUAVTypes;
                break;
            }
            case DescriptorHeapType::Sampler:
                typesSize = RHINO_ARR_SIZE(SamplerTypes);
                types = SamplerTypes;
                break;
            default:
                assert(0);
                return 0;
        }

        size_t maxDescriptorSize = 0;
        for (size_t i = 0; i < typesSize; ++i) {
            switch (types[i]) {
                case VK_DESCRIPTOR_TYPE_SAMPLER:
                    assert(heapType == DescriptorHeapType::Sampler);
                    maxDescriptorSize = std::max(maxDescriptorSize, descriptorProps.samplerDescriptorSize);
                    break;
                case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
                    assert(heapType == DescriptorHeapType::SRV_CBV_UAV);
                    maxDescriptorSize = std::max(maxDescriptorSize, descriptorProps.combinedImageSamplerDescriptorSize);
                    break;
                case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
                    assert(heapType == DescriptorHeapType::SRV_CBV_UAV);
                    maxDescriptorSize = std::max(maxDescriptorSize, descriptorProps.sampledImageDescriptorSize);
                    break;
                case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
                    assert(heapType == DescriptorHeapType::SRV_CBV_UAV);
                    maxDescriptorSize = std::max(maxDescriptorSize, descriptorProps.storageImageDescriptorSize);
                    break;
                case VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
                    assert(heapType == DescriptorHeapType::SRV_CBV_UAV);
                    maxDescriptorSize = std::max(maxDescriptorSize, descriptorProps.uniformTexelBufferDescriptorSize);
                    break;
                case VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:
                    assert(heapType == DescriptorHeapType::SRV_CBV_UAV);
                    maxDescriptorSize = std::max(maxDescriptorSize, descriptorProps.storageTexelBufferDescriptorSize);
                    break;
                case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
                    assert(heapType == DescriptorHeapType::SRV_CBV_UAV);
                    maxDescriptorSize = std::max(maxDescriptorSize, descriptorProps.uniformBufferDescriptorSize);
                    break;
                case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
                    assert(heapType == DescriptorHeapType::SRV_CBV_UAV);
                    maxDescriptorSize = std::max(maxDescriptorSize, descriptorProps.storageBufferDescriptorSize);
                    break;
                case VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR:
                    assert(heapType == DescriptorHeapType::SRV_CBV_UAV);
                    maxDescriptorSize = std::max(maxDescriptorSize, descriptorProps.accelerationStructureDescriptorSize);
                    break;
                default:
                    assert(0 && "Unsupported descriptor type.");
            }
        }
        return maxDescriptorSize;
    }
}

#endif // ENABLE_API_VULKAN
