#pragma once

#ifdef ENABLE_API_D3D12

namespace RHINO::APID3D12 {
    class D3D12DescriptorHeap : public DescriptorHeap {
    public:
        void WriteSRV(const WriteBufferDescriptorDesc& desc) noexcept final;
        void WriteUAV(const WriteBufferDescriptorDesc& desc) noexcept final;
        void WriteCBV(const WriteBufferDescriptorDesc& desc) noexcept final;
        void WriteSRV(const WriteTexture2DSRVDesc& desc) noexcept final;
        void WriteUAV(const WriteTexture2DSRVDesc& desc) noexcept final;
        void WriteSRV(const WriteTexture3DSRVDesc& desc) noexcept final;
        void WriteUAV(const WriteTexture3DSRVDesc& desc) noexcept final;

    public:
        [[nodiscard]] D3D12_CPU_DESCRIPTOR_HANDLE GetCPUHeapCPUHandle(UINT index) const noexcept;
        [[nodiscard]] D3D12_CPU_DESCRIPTOR_HANDLE GetGPUHeapCPUHandle(UINT index) const noexcept;
        [[nodiscard]] D3D12_GPU_DESCRIPTOR_HANDLE GetGPUHeapGPUHandle(UINT index) const noexcept;

        ID3D12DescriptorHeap* CPUDescriptorHeap = nullptr;
        ID3D12DescriptorHeap* GPUDescriptorHeap = nullptr;
        UINT descriptorHandleIncrementSize = 0;
        D3D12_CPU_DESCRIPTOR_HANDLE CPUHeapCPUStartHandle = {};
        D3D12_CPU_DESCRIPTOR_HANDLE GPUHeapCPUStartHandle = {};
        D3D12_GPU_DESCRIPTOR_HANDLE GPUHeapGPUStartHandle = {};

        ID3D12Device* device;
    };
}// namespace RHINO::APID3D12

#endif// ENABLE_API_D3D12
