#pragma once

#include "VulkanAPI.h"

namespace RHINO {
    inline size_t CalculateDescriptorHandleIncrementSize(DescriptorHeapType heapType, const VkPhysicalDeviceDescriptorBufferPropertiesEXT& descriptorProps) noexcept {
        switch (heapType) {
            case DescriptorHeapType::SRV_CBV_UAV:
                return std::max(descriptorProps.uniformBufferDescriptorSize, std::max(descriptorProps.storageBufferDescriptorSize, std::max(descriptorProps.storageImageDescriptorSize, descriptorProps.sampledImageDescriptorSize)));
            case DescriptorHeapType::Sampler:
                return descriptorProps.samplerDescriptorSize;
            default:
                return 0;
        }
    }
}
