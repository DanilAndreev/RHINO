#ifdef ENABLE_API_D3D12

#include "D3D12DescriptorHeap.h"
#include "D3D12BackendTypes.h"

namespace RHINO::APID3D12 {
    void D3D12DescriptorHeap::WriteSRV(const WriteBufferDescriptorDesc& desc) noexcept {
        //TODO: check that desc.size %  desc.bufferStructuredStride = 0 and desc.bufferOffset %  desc.bufferStructuredStride
        auto* d3d12Buffer = static_cast<D3D12Buffer*>(desc.buffer);

        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.Format = DXGI_FORMAT_UNKNOWN;
        srvDesc.Buffer.FirstElement = desc.bufferOffset / desc.bufferStructuredStride;
        srvDesc.Buffer.NumElements = (desc.size / desc.bufferStructuredStride) - srvDesc.Buffer.FirstElement;
        srvDesc.Buffer.StructureByteStride = desc.bufferStructuredStride;

        D3D12_CPU_DESCRIPTOR_HANDLE CPUHeapCPUHandle = GetCPUHeapCPUHandle(desc.offsetInHeap);
        D3D12_CPU_DESCRIPTOR_HANDLE GPUHeapCPUHandle = GetGPUHeapCPUHandle(desc.offsetInHeap);
        device->CreateShaderResourceView(d3d12Buffer->buffer, &srvDesc, CPUHeapCPUHandle);
        device->CopyDescriptorsSimple(1, GPUHeapCPUHandle, CPUHeapCPUHandle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    }
    void D3D12DescriptorHeap::WriteUAV(const WriteBufferDescriptorDesc& desc) noexcept {
        auto* d3d12Buffer = static_cast<D3D12Buffer*>(desc.buffer);

        D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc{};
        uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
        uavDesc.Format = DXGI_FORMAT_UNKNOWN;
        uavDesc.Buffer.FirstElement = desc.bufferOffset / desc.bufferStructuredStride;
        uavDesc.Buffer.NumElements = (desc.size / desc.bufferStructuredStride) - uavDesc.Buffer.FirstElement;
        uavDesc.Buffer.StructureByteStride = desc.bufferStructuredStride;
        uavDesc.Buffer.CounterOffsetInBytes = 0;

        D3D12_CPU_DESCRIPTOR_HANDLE CPUHeapCPUHandle = GetCPUHeapCPUHandle(desc.offsetInHeap);
        D3D12_CPU_DESCRIPTOR_HANDLE GPUHeapCPUHandle = GetGPUHeapCPUHandle(desc.offsetInHeap);
        device->CreateUnorderedAccessView(d3d12Buffer->buffer, nullptr, &uavDesc, CPUHeapCPUHandle);
        device->CopyDescriptorsSimple(1, GPUHeapCPUHandle, CPUHeapCPUHandle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    }
    void D3D12DescriptorHeap::WriteCBV(const WriteBufferDescriptorDesc& desc) noexcept {
        auto* d3d12Buffer = static_cast<D3D12Buffer*>(desc.buffer);

        D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc{};
        cbvDesc.SizeInBytes = desc.size;
        cbvDesc.BufferLocation = d3d12Buffer->buffer->GetGPUVirtualAddress() + desc.bufferOffset;

        D3D12_CPU_DESCRIPTOR_HANDLE CPUHeapCPUHandle = GetCPUHeapCPUHandle(desc.offsetInHeap);
        D3D12_CPU_DESCRIPTOR_HANDLE GPUHeapCPUHandle = GetGPUHeapCPUHandle(desc.offsetInHeap);
        device->CreateConstantBufferView(&cbvDesc, CPUHeapCPUHandle);
        device->CopyDescriptorsSimple(1, GPUHeapCPUHandle, CPUHeapCPUHandle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    }
    void D3D12DescriptorHeap::WriteSRV(const WriteTexture2DSRVDesc& desc) noexcept {
        auto* d3d12Texture = static_cast<D3D12Texture2D*>(desc.texture);

        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        // srvDesc.Format = CalculateSRVFormat(d3d12Texture->desc.Format);
        srvDesc.Texture2D.MipLevels = -1;

        D3D12_CPU_DESCRIPTOR_HANDLE CPUHeapCPUHandle = GetCPUHeapCPUHandle(desc.offsetInHeap);
        D3D12_CPU_DESCRIPTOR_HANDLE GPUHeapCPUHandle = GetGPUHeapCPUHandle(desc.offsetInHeap);
        device->CreateShaderResourceView(d3d12Texture->texture, &srvDesc, CPUHeapCPUHandle);
        device->CopyDescriptorsSimple(1, GPUHeapCPUHandle, CPUHeapCPUHandle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    }
    void D3D12DescriptorHeap::WriteUAV(const WriteTexture2DSRVDesc& desc) noexcept {
        auto* d3d12Texture = static_cast<D3D12Texture2D*>(desc.texture);

        D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc{};
        uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
        // uavDesc.Format = CalculateUAVFormat(d3d12Texture->desc.Format);
        uavDesc.Texture2D.MipSlice = 0;

        D3D12_CPU_DESCRIPTOR_HANDLE CPUHeapCPUHandle = GetCPUHeapCPUHandle(desc.offsetInHeap);
        D3D12_CPU_DESCRIPTOR_HANDLE GPUHeapCPUHandle = GetGPUHeapCPUHandle(desc.offsetInHeap);
        device->CreateUnorderedAccessView(d3d12Texture->texture, nullptr, &uavDesc, CPUHeapCPUHandle);
        device->CopyDescriptorsSimple(1, GPUHeapCPUHandle, CPUHeapCPUHandle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    }
    void D3D12DescriptorHeap::WriteSRV(const WriteTexture3DSRVDesc& desc) noexcept {
        auto* d3d12Texture = static_cast<D3D12Texture2D*>(desc.texture);

        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE3D;
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        // srvDesc.Format = CalculateSRVFormat(d3d12Texture->desc.Format);
        srvDesc.Texture2D.MipLevels = -1;

        D3D12_CPU_DESCRIPTOR_HANDLE CPUHeapCPUHandle = GetCPUHeapCPUHandle(desc.offsetInHeap);
        D3D12_CPU_DESCRIPTOR_HANDLE GPUHeapCPUHandle = GetGPUHeapCPUHandle(desc.offsetInHeap);
        device->CreateShaderResourceView(d3d12Texture->texture, &srvDesc, CPUHeapCPUHandle);
        device->CopyDescriptorsSimple(1, GPUHeapCPUHandle, CPUHeapCPUHandle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    }
    void D3D12DescriptorHeap::WriteUAV(const WriteTexture3DSRVDesc& desc) noexcept {
        auto* d3d12Texture = static_cast<D3D12Texture2D*>(desc.texture);

        D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc{};
        uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE3D;
        // uavDesc.Format = CalculateUAVFormat(d3d12Texture->desc.Format);
        uavDesc.Texture2D.MipSlice = 0;

        D3D12_CPU_DESCRIPTOR_HANDLE CPUHeapCPUHandle = GetCPUHeapCPUHandle(desc.offsetInHeap);
        D3D12_CPU_DESCRIPTOR_HANDLE GPUHeapCPUHandle = GetGPUHeapCPUHandle(desc.offsetInHeap);
        device->CreateUnorderedAccessView(d3d12Texture->texture, nullptr, &uavDesc, CPUHeapCPUHandle);
        device->CopyDescriptorsSimple(1, GPUHeapCPUHandle, CPUHeapCPUHandle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    }

    D3D12_CPU_DESCRIPTOR_HANDLE D3D12DescriptorHeap::GetCPUHeapCPUHandle(UINT index) const noexcept {
        assert(CPUHeapCPUStartHandle.ptr != 0);
        D3D12_CPU_DESCRIPTOR_HANDLE result = CPUHeapCPUStartHandle;
        result.ptr += static_cast<SIZE_T>(index) * descriptorHandleIncrementSize;
        return result;
    }
    D3D12_CPU_DESCRIPTOR_HANDLE D3D12DescriptorHeap::GetGPUHeapCPUHandle(UINT index) const noexcept {
        assert(GPUHeapCPUStartHandle.ptr != 0);
        D3D12_CPU_DESCRIPTOR_HANDLE result = GPUHeapCPUStartHandle;
        result.ptr += static_cast<SIZE_T>(index) * descriptorHandleIncrementSize;
        return result;
    }
    D3D12_GPU_DESCRIPTOR_HANDLE D3D12DescriptorHeap::GetGPUHeapGPUHandle(UINT index) const noexcept {
        assert(GPUHeapGPUStartHandle.ptr != 0);
        D3D12_GPU_DESCRIPTOR_HANDLE result = GPUHeapGPUStartHandle;
        result.ptr += static_cast<UINT64>(index) * descriptorHandleIncrementSize;
        return result;
    }
}// namespace RHINO::APID3D12

#endif// ENABLE_API_D3D12
