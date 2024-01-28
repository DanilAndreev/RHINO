#ifdef ENABLE_API_VULKAN

#include "VulkanCommandList.h"
#include "VulkanAPI.h"
#include "VulkanDescriptorHeap.h"

namespace RHINO::APIVulkan {
    void VulkanCommandList::CopyBuffer(Buffer* src, Buffer* dst, size_t srcOffset, size_t dstOffset, size_t size) noexcept {
        auto* vulkanSrc = static_cast<VulkanBuffer*>(src);
        auto* vulkanDst = static_cast<VulkanBuffer*>(dst);
        VkBufferCopy region{};
        region.size = size;
        region.srcOffset = srcOffset;
        region.dstOffset = dstOffset;
        vkCmdCopyBuffer(cmd, vulkanSrc->buffer, vulkanDst->buffer, 1, &region);
    }

    void VulkanCommandList::Dispatch(const DispatchDesc& desc) noexcept {
        vkCmdDispatch(cmd, desc.dimensionsX, desc.dimensionsY, desc.dimensionsZ);
    }

    void VulkanCommandList::Draw() noexcept {
    }

    void VulkanCommandList::SetComputePSO(ComputePSO* pso) noexcept {
        auto* vulkanPSO = static_cast<VulkanComputePSO*>(pso);
        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, vulkanPSO->PSO);
        for (auto [space, spaceInfo] : vulkanPSO->heapOffsetsBySpaces) {
            uint32_t bufferIndex = spaceInfo.first == DescriptorRangeType::Sampler ? 1 : 0;
            VkDeviceSize offset = spaceInfo.second;
            EXT::vkCmdSetDescriptorBufferOffsetsEXT(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, vulkanPSO->layout,
                                                    space, 1, &bufferIndex, &offset);
        }
    }

    void VulkanCommandList::SetRTPSO(RTPSO* pso) noexcept {

    }

    void VulkanCommandList::SetHeap(DescriptorHeap* CBVSRVUAVHeap, DescriptorHeap* SamplerHeap) noexcept {
        VkDescriptorBufferBindingInfoEXT bindings[2] = {};
        auto* vulkanCBVSRVUAVHeap = static_cast<VulkanDescriptorHeap*>(CBVSRVUAVHeap);
        VkDescriptorBufferBindingInfoEXT bindingCBVSRVUAV{VK_STRUCTURE_TYPE_DESCRIPTOR_BUFFER_BINDING_INFO_EXT};
        bindingCBVSRVUAV.address = vulkanCBVSRVUAVHeap->heapGPUStartHandle;
        bindingCBVSRVUAV.usage = VK_BUFFER_USAGE_RESOURCE_DESCRIPTOR_BUFFER_BIT_EXT;
        bindings[0] = bindingCBVSRVUAV;
        if (SamplerHeap) {
            auto* vulkanSamplerHeap = static_cast<VulkanDescriptorHeap*>(SamplerHeap);
            VkDescriptorBufferBindingInfoEXT bindingSampler{VK_STRUCTURE_TYPE_DESCRIPTOR_BUFFER_BINDING_INFO_EXT};
            bindingSampler.address = vulkanSamplerHeap->heapGPUStartHandle;
            bindingSampler.usage = VK_BUFFER_USAGE_RESOURCE_DESCRIPTOR_BUFFER_BIT_EXT;
            bindings[1] = bindingSampler;
        }
        EXT::vkCmdBindDescriptorBuffersEXT(cmd, SamplerHeap ? 2 : 1, bindings);
    }
}// namespace RHINO::APIVulkan

#endif// ENABLE_API_VULKAN
