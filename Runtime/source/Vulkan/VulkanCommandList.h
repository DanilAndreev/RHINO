#pragma once

#ifdef ENABLE_API_VULKAN

#include "VulkanBackendTypes.h"

namespace RHINO::APIVulkan {
    class VulkanCommandList : public CommandList {
    public:
        void CopyBuffer(Buffer* src, Buffer* dst, size_t srcOffset, size_t dstOffset, size_t size) noexcept final;
        void Dispatch(const DispatchDesc& desc) noexcept final;
        void Draw() noexcept final;
        void SetComputePSO(ComputePSO* pso) noexcept final;
        void SetRTPSO(RTPSO* pso) noexcept final;
        void SetHeap(DescriptorHeap* CBVSRVUAVHeap, DescriptorHeap* SamplerHeap) noexcept final;

    public:
        VkCommandBuffer cmd = VK_NULL_HANDLE;
        VkCommandPool pool = VK_NULL_HANDLE; //TODO: move to rhi.
    };
}// namespace RHINO::APIVulkan

#endif// ENABLE_API_VULKAN
