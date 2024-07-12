#include "MetalDescriptorHeap.h"
#include "MetalBackendTypes.h"

#import <metal_irconverter_runtime/metal_irconverter_runtime.h>

namespace RHINO::APIMetal {
    void MetalDescriptorHeap::WriteSRV(const WriteBufferDescriptorDesc& desc) noexcept {
        auto* metalBuffer = static_cast<MetalBuffer*>(desc.buffer);
        auto* entry = static_cast<IRDescriptorTableEntry*>([m_ArgBuf contents]);
        size_t bufferGPUAddress = [metalBuffer->buffer gpuAddress] + desc.bufferOffset;
        IRDescriptorTableSetBuffer(entry + desc.offsetInHeap, bufferGPUAddress, 0);
        resources[desc.offsetInHeap] = metalBuffer->buffer;
    }

    void MetalDescriptorHeap::WriteUAV(const WriteBufferDescriptorDesc& desc) noexcept {
        auto* metalBuffer = static_cast<MetalBuffer*>(desc.buffer);
        auto* entry = static_cast<IRDescriptorTableEntry*>([m_ArgBuf contents]);
        size_t bufferGPUAddress = [metalBuffer->buffer gpuAddress] + desc.bufferOffset;
        IRDescriptorTableSetBuffer(entry + desc.offsetInHeap, bufferGPUAddress, 0);
        resources[desc.offsetInHeap] = metalBuffer->buffer;
    }

    void MetalDescriptorHeap::WriteCBV(const WriteBufferDescriptorDesc& desc) noexcept {
        auto* metalBuffer = static_cast<MetalBuffer*>(desc.buffer);
        auto* entry = static_cast<IRDescriptorTableEntry*>([m_ArgBuf contents]);
        size_t bufferGPUAddress = [metalBuffer->buffer gpuAddress] + desc.bufferOffset;
        IRDescriptorTableSetBuffer(entry + desc.offsetInHeap, bufferGPUAddress, 0);
        resources[desc.offsetInHeap] = metalBuffer->buffer;
    }

    void MetalDescriptorHeap::WriteSRV(const WriteTexture2DDescriptorDesc& desc) noexcept {
        auto* metalTexture2D = static_cast<MetalTexture2D*>(desc.texture);
        auto* entry = static_cast<IRDescriptorTableEntry*>([m_ArgBuf contents]);
        IRDescriptorTableSetTexture(entry + desc.offsetInHeap, metalTexture2D->texture, 0, 0);
        resources[desc.offsetInHeap] = metalTexture2D->texture;
    }

    void MetalDescriptorHeap::WriteUAV(const WriteTexture2DDescriptorDesc& desc) noexcept {
        auto* metalTexture2D = static_cast<MetalTexture2D*>(desc.texture);
        auto* entry = static_cast<IRDescriptorTableEntry*>([m_ArgBuf contents]);
        IRDescriptorTableSetTexture(entry + desc.offsetInHeap, metalTexture2D->texture, 0, 0);
        resources[desc.offsetInHeap] = metalTexture2D->texture;
    }

    void MetalDescriptorHeap::WriteSRV(const WriteTexture3DDescriptorDesc& desc) noexcept {
        auto* metalTexture3D = static_cast<MetalTexture2D*>(desc.texture);
        auto* entry = static_cast<IRDescriptorTableEntry*>([m_ArgBuf contents]);
        IRDescriptorTableSetTexture(entry + desc.offsetInHeap, metalTexture3D->texture, 0, 0);
        resources[desc.offsetInHeap] = metalTexture3D->texture;
    }

    void MetalDescriptorHeap::WriteUAV(const WriteTexture3DDescriptorDesc& desc) noexcept {
        auto* metalTexture3D = static_cast<MetalTexture2D*>(desc.texture);
        auto* entry = static_cast<IRDescriptorTableEntry*>([m_ArgBuf contents]);
        IRDescriptorTableSetTexture(entry + desc.offsetInHeap, metalTexture3D->texture, 0, 0);
        resources[desc.offsetInHeap] = metalTexture3D->texture;
    }

    void MetalDescriptorHeap::WriteSRV(const WriteTLASDescriptorDesc& desc) noexcept {
        //TODO: implement
    }
} // namespace RHINO::APIMetal
