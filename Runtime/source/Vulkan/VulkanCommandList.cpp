#ifdef ENABLE_API_VULKAN

#include "VulkanCommandList.h"
#include "VulkanAPI.h"
#include "VulkanConverters.h"
#include "VulkanDescriptorHeap.h"
#include "VulkanUtils.h"

namespace RHINO::APIVulkan {
    void VulkanCommandList::Initialize(const char* name, VkPhysicalDevice physicalDevice, VkDevice device, VkCommandPool pool,
                                       VkAllocationCallbacks* alloc) noexcept {
        m_Device = device;
        m_Alloc = alloc;

        VkPhysicalDeviceDescriptorBufferPropertiesEXT descriptorProps{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_BUFFER_PROPERTIES_EXT};
        VkPhysicalDeviceProperties2 props{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2};
        props.pNext = &descriptorProps;
        vkGetPhysicalDeviceProperties2(physicalDevice, &props);
        m_DescriptorProps = descriptorProps;

        VkCommandBufferAllocateInfo cmdAlloc{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
        cmdAlloc.commandPool = pool;
        cmdAlloc.commandBufferCount = 1;
        cmdAlloc.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        vkAllocateCommandBuffers(m_Device, &cmdAlloc, &m_Cmd);

        VkCommandBufferBeginInfo beginInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
        beginInfo.pInheritanceInfo = nullptr;
        vkBeginCommandBuffer(m_Cmd, &beginInfo);
    }

    void VulkanCommandList::Release() noexcept {
        vkFreeCommandBuffers(m_Device, m_Pool, 1, &m_Cmd);
        vkDestroyCommandPool(m_Device, m_Pool, m_Alloc);
        // Command pool is managed by VulkanBackend instance and should be released by it.
    }

    void VulkanCommandList::SubmitToQueue(VkQueue queue) noexcept {
        vkEndCommandBuffer(m_Cmd);

        VkSubmitInfo submitInfo{VK_STRUCTURE_TYPE_SUBMIT_INFO};
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &m_Cmd;
        vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
    }

    void VulkanCommandList::CopyBuffer(Buffer* src, Buffer* dst, size_t srcOffset, size_t dstOffset, size_t size) noexcept {
        auto* vulkanSrc = static_cast<VulkanBuffer*>(src);
        auto* vulkanDst = static_cast<VulkanBuffer*>(dst);
        VkBufferCopy region{};
        region.size = size;
        region.srcOffset = srcOffset;
        region.dstOffset = dstOffset;
        vkCmdCopyBuffer(m_Cmd, vulkanSrc->buffer, vulkanDst->buffer, 1, &region);
    }

    void VulkanCommandList::SetComputePSO(ComputePSO* pso) noexcept {
        auto* vulkanPSO = static_cast<VulkanComputePSO*>(pso);
        vkCmdBindPipeline(m_Cmd, VK_PIPELINE_BIND_POINT_COMPUTE, vulkanPSO->PSO);
        for (auto [space, spaceInfo] : vulkanPSO->heapOffsetsInDescriptorsBySpaces) {
            uint32_t bufferIndex = spaceInfo.first == DescriptorRangeType::Sampler ? 1 : 0;
            VkDeviceSize offset = spaceInfo.second;
            switch (spaceInfo.first) {
                case DescriptorRangeType::Sampler:
                    offset *= CalculateDescriptorHandleIncrementSize(DescriptorHeapType::Sampler, m_DescriptorProps);
                    break;
                default:
                    offset *= CalculateDescriptorHandleIncrementSize(DescriptorHeapType::SRV_CBV_UAV, m_DescriptorProps);
            }
            EXT::vkCmdSetDescriptorBufferOffsetsEXT(m_Cmd, VK_PIPELINE_BIND_POINT_COMPUTE, vulkanPSO->layout,
                                                    space, 1, &bufferIndex, &offset);
        }
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
            bindingSampler.usage = VK_BUFFER_USAGE_SAMPLER_DESCRIPTOR_BUFFER_BIT_EXT | VK_BUFFER_USAGE_RESOURCE_DESCRIPTOR_BUFFER_BIT_EXT;
            bindings[1] = bindingSampler;
        }
        EXT::vkCmdBindDescriptorBuffersEXT(m_Cmd, SamplerHeap ? 2 : 1, bindings);
    }

    void VulkanCommandList::Dispatch(const DispatchDesc& desc) noexcept {
        vkCmdDispatch(m_Cmd, desc.dimensionsX, desc.dimensionsY, desc.dimensionsZ);
    }

    void VulkanCommandList::DispatchRays(const DispatchRaysDesc& desc) noexcept {
        VkStridedDeviceAddressRegionKHR rayGenTable = {};
        VkStridedDeviceAddressRegionKHR missTable = {};
        VkStridedDeviceAddressRegionKHR higGroupTable = {};
        VkStridedDeviceAddressRegionKHR callableTable = {NULL};

        //TODO: calculate addresses;

        vkCmdTraceRaysKHR(m_Cmd, &rayGenTable, &missTable, &higGroupTable, &callableTable, desc.width, desc.height, 1);
    }

    void VulkanCommandList::Draw() noexcept {
    }

    void VulkanCommandList::BuildRTPSO(RTPSO* pso) noexcept {
        //TODO: impl
    }

    BLAS* VulkanCommandList::BuildBLAS(const BLASDesc& desc, Buffer* scratchBuffer, size_t scratchBufferStartOffset,
                                       const char* name) noexcept {
        auto* indexBuffer = static_cast<VulkanBuffer*>(desc.indexBuffer);
        auto* vertexBuffer = static_cast<VulkanBuffer*>(desc.vertexBuffer);
        auto* transform = static_cast<VulkanBuffer*>(desc.transformBuffer);
        auto* scratch = static_cast<VulkanBuffer*>(scratchBuffer);

        auto result = new VulkanBLAS{};

        VkAccelerationStructureGeometryKHR asGeom{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR};
        asGeom.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
        asGeom.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
        asGeom.geometry.triangles.indexType = Convert::ToVkIndexType(desc.indexFormat);
        asGeom.geometry.triangles.indexData = {indexBuffer->deviceAddress + desc.indexBufferStartOffset};
        asGeom.geometry.triangles.vertexStride = desc.vertexStride;
        asGeom.geometry.triangles.vertexFormat = Convert::ToVkFormat(desc.vertexFormat);
        asGeom.geometry.triangles.vertexData = {vertexBuffer->deviceAddress + desc.vertexBufferStartOffset};
        asGeom.geometry.triangles.maxVertex = desc.vertexCount;
        asGeom.geometry.triangles.transformData = {transform->deviceAddress + desc.transformBufferStartOffset};

        VkAccelerationStructureBuildGeometryInfoKHR buildGeoInfo{};
        buildGeoInfo.dstAccelerationStructure = VK_NULL_HANDLE;
        buildGeoInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
        buildGeoInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
        buildGeoInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
        buildGeoInfo.geometryCount = 1;
        buildGeoInfo.pGeometries = &asGeom;
        buildGeoInfo.ppGeometries = nullptr;
        buildGeoInfo.scratchData = {scratch->deviceAddress};

        VkAccelerationStructureBuildSizesInfoKHR outSizesInfo{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR};
        vkGetAccelerationStructureBuildSizesKHR(m_Device, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &buildGeoInfo, nullptr,
                                                &outSizesInfo);


        VkBufferCreateInfo bufferInfo{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
        bufferInfo.size = outSizesInfo.accelerationStructureSize;
        bufferInfo.usage = VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        vkCreateBuffer(m_Device, &bufferInfo, m_Alloc, &result->buffer);

        // TODO: allocate buffer memory

        VkAccelerationStructureCreateInfoKHR asInfo{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR};
        asInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
        asInfo.buffer = result->buffer;
        asInfo.deviceAddress = NULL;
        asInfo.offset = 0;
        asInfo.size = outSizesInfo.accelerationStructureSize;
        asInfo.createFlags = 0;

        VkAccelerationStructureKHR accelerationStructure;
        vkCreateAccelerationStructureKHR(m_Device, &asInfo, m_Alloc, &accelerationStructure);

        vkCmdBuildAccelerationStructuresKHR(m_Cmd, 1, &buildGeoInfo, nullptr);
    }

    TLAS* VulkanCommandList::BuildTLAS(const TLASDesc& desc, Buffer* scratchBuffer, size_t scratchBufferStartOffset,
                                       const char* name) noexcept {

    }
} // namespace RHINO::APIVulkan

#endif// ENABLE_API_VULKAN
