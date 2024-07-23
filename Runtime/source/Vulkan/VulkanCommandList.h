#pragma once

#ifdef ENABLE_API_VULKAN

#include "VulkanBackendTypes.h"

namespace RHINO::APIVulkan {
    class VulkanCommandList : public CommandList {
    public:
        void Initialize(const char* name, VulkanObjectContext context, VkCommandPool pool) noexcept;
        void SubmitToQueue(VkQueue queue) noexcept;

    public:
        void SetRootSignature(RootSignature* rootSignature) noexcept final;
        void CopyBuffer(Buffer* src, Buffer* dst, size_t srcOffset, size_t dstOffset, size_t size) noexcept final;
        void SetComputePSO(ComputePSO* pso) noexcept final;
        void SetHeap(DescriptorHeap* CBVSRVUAVHeap, DescriptorHeap* SamplerHeap) noexcept final;
        void Dispatch(const DispatchDesc& desc) noexcept final;
        void DispatchRays(const DispatchRaysDesc& desc) noexcept final;
        void Draw() noexcept final;
        void ResourceBarrier(const ResourceBarrierDesc& desc) noexcept final;

    public:
        void Release() noexcept final;

    public:
        void BuildRTPSO(RTPSO* pso) noexcept final;
        BLAS* BuildBLAS(const BLASDesc& desc, Buffer* scratchBuffer, size_t scratchBufferStartOffset, const char* name) noexcept final;
        TLAS* BuildTLAS(const TLASDesc& desc, Buffer* scratchBuffer, size_t scratchBufferStartOffset, const char* name) noexcept final;

    private:
        VulkanObjectContext m_Context = {};
        VkCommandBuffer m_Cmd = VK_NULL_HANDLE;
        VkCommandPool m_Pool = VK_NULL_HANDLE;

        VkPhysicalDeviceDescriptorBufferPropertiesEXT m_DescriptorProps = {};
    };
}// namespace RHINO::APIVulkan

#endif// ENABLE_API_VULKAN
