#ifdef ENABLE_API_METAL

#include "MetalDescriptorHeap.h"
#include "MetalBackendTypes.h"

#import <metal_irconverter_runtime/metal_irconverter_runtime.h>

namespace RHINO::APIMetal {
    void MetalDescriptorHeap::WriteSRV(const WriteBufferDescriptorDesc& desc) noexcept {
        auto* metalBuffer = INTERPRET_AS<MetalBuffer*>(desc.buffer);
        auto* entry = static_cast<IRDescriptorTableEntry*>([m_DescriptorHeap contents]);
        size_t bufferGPUAddress = [metalBuffer->buffer gpuAddress] + desc.bufferOffset;
        IRDescriptorTableSetBuffer(entry + desc.offsetInHeap, bufferGPUAddress, 0);
        m_Resources[desc.offsetInHeap] = metalBuffer->buffer;
    }

    void MetalDescriptorHeap::WriteUAV(const WriteBufferDescriptorDesc& desc) noexcept {
        auto* metalBuffer = INTERPRET_AS<MetalBuffer*>(desc.buffer);
        auto* entry = static_cast<IRDescriptorTableEntry*>([m_DescriptorHeap contents]);
        size_t bufferGPUAddress = [metalBuffer->buffer gpuAddress] + desc.bufferOffset;
        IRDescriptorTableSetBuffer(entry + desc.offsetInHeap, bufferGPUAddress, 0);
        m_Resources[desc.offsetInHeap] = metalBuffer->buffer;
    }

    void MetalDescriptorHeap::WriteCBV(const WriteBufferDescriptorDesc& desc) noexcept {
        auto* metalBuffer = INTERPRET_AS<MetalBuffer*>(desc.buffer);
        auto* entry = static_cast<IRDescriptorTableEntry*>([m_DescriptorHeap contents]);
        size_t bufferGPUAddress = [metalBuffer->buffer gpuAddress] + desc.bufferOffset;
        IRDescriptorTableSetBuffer(entry + desc.offsetInHeap, bufferGPUAddress, 0);
        m_Resources[desc.offsetInHeap] = metalBuffer->buffer;
    }

    void MetalDescriptorHeap::WriteSRV(const WriteTexture2DDescriptorDesc& desc) noexcept {
        auto* metalTexture2D = INTERPRET_AS<MetalTexture2D*>(desc.texture);
        auto* entry = static_cast<IRDescriptorTableEntry*>([m_DescriptorHeap contents]);
        IRDescriptorTableSetTexture(entry + desc.offsetInHeap, metalTexture2D->texture, 0, 0);
        m_Resources[desc.offsetInHeap] = metalTexture2D->texture;
    }

    void MetalDescriptorHeap::WriteUAV(const WriteTexture2DDescriptorDesc& desc) noexcept {
        auto* metalTexture2D = INTERPRET_AS<MetalTexture2D*>(desc.texture);
        auto* entry = static_cast<IRDescriptorTableEntry*>([m_DescriptorHeap contents]);
        IRDescriptorTableSetTexture(entry + desc.offsetInHeap, metalTexture2D->texture, 0, 0);
        m_Resources[desc.offsetInHeap] = metalTexture2D->texture;
    }

    void MetalDescriptorHeap::WriteSRV(const WriteTexture3DDescriptorDesc& desc) noexcept {
        auto* metalTexture3D = INTERPRET_AS<MetalTexture2D*>(desc.texture);
        auto* entry = static_cast<IRDescriptorTableEntry*>([m_DescriptorHeap contents]);
        IRDescriptorTableSetTexture(entry + desc.offsetInHeap, metalTexture3D->texture, 0, 0);
        m_Resources[desc.offsetInHeap] = metalTexture3D->texture;
    }

    void MetalDescriptorHeap::WriteUAV(const WriteTexture3DDescriptorDesc& desc) noexcept {
        auto* metalTexture3D = INTERPRET_AS<MetalTexture2D*>(desc.texture);
        auto* entry = static_cast<IRDescriptorTableEntry*>([m_DescriptorHeap contents]);
        IRDescriptorTableSetTexture(entry + desc.offsetInHeap, metalTexture3D->texture, 0, 0);
        m_Resources[desc.offsetInHeap] = metalTexture3D->texture;
    }

    void MetalDescriptorHeap::WriteSRV(const WriteTLASDescriptorDesc& desc) noexcept {
        auto* metalTLAS = INTERPRET_AS<MetalTLAS*>(desc.tlas);
        auto* entry = static_cast<IRDescriptorTableEntry*>([m_DescriptorHeap contents]);
        // IRDescriptorTableSetAccelerationStructure(entry + desc.offsetInHeap, metalTLAS->accelerationStructure);
        // m_Resources[desc.offsetInHeap] = metalTLAS->accelerationStructure;
        // TODO: implement
    }

    void MetalDescriptorHeap::Release() noexcept {
        delete this;
    }

    id<MTLBuffer> MetalDescriptorHeap::GetHeapBuffer() noexcept {
        return m_DescriptorHeap;
    }

    size_t MetalDescriptorHeap::GetDescriptorStride() const noexcept {
        return sizeof(IRDescriptorTableEntry);
    }

    const std::vector<id<MTLResource>>& MetalDescriptorHeap::GetBoundResources() const noexcept {
        return m_Resources;
    }
} // namespace RHINO::APIMetal

#endif // ENABLE_API_METAL
