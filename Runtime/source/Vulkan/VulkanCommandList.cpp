#ifdef ENABLE_API_VULKAN

#include "VulkanCommandList.h"
#include "VulkanAPI.h"
#include "VulkanConverters.h"
#include "VulkanDescriptorHeap.h"
#include "VulkanUtils.h"

namespace RHINO::APIVulkan {
    void VulkanCommandList::Initialize(const char* name, VulkanObjectContext context,
                                       VkCommandPool pool) noexcept {
        m_Context = context;

        VkPhysicalDeviceDescriptorBufferPropertiesEXT descriptorProps{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_BUFFER_PROPERTIES_EXT};
        VkPhysicalDeviceProperties2 props{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2};
        props.pNext = &descriptorProps;
        vkGetPhysicalDeviceProperties2(m_Context.physicalDevice, &props);
        m_DescriptorProps = descriptorProps;

        VkCommandBufferAllocateInfo cmdAlloc{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
        cmdAlloc.commandPool = pool;
        cmdAlloc.commandBufferCount = 1;
        cmdAlloc.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        vkAllocateCommandBuffers(m_Context.device, &cmdAlloc, &m_Cmd);

        VkCommandBufferBeginInfo beginInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
        beginInfo.pInheritanceInfo = nullptr;
        vkBeginCommandBuffer(m_Cmd, &beginInfo);
    }

    void VulkanCommandList::Release() noexcept {
        vkFreeCommandBuffers(m_Context.device, m_Pool, 1, &m_Cmd);
        vkDestroyCommandPool(m_Context.device, m_Pool, m_Context.allocator);
        // Command pool is managed by VulkanBackend instance and should be released by it.
        delete this;
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
        bindingCBVSRVUAV.address = vulkanCBVSRVUAVHeap->GetHeapGPUStartHandle();
        bindingCBVSRVUAV.usage = VK_BUFFER_USAGE_RESOURCE_DESCRIPTOR_BUFFER_BIT_EXT;
        bindings[0] = bindingCBVSRVUAV;
        if (SamplerHeap) {
            auto* vulkanSamplerHeap = static_cast<VulkanDescriptorHeap*>(SamplerHeap);
            VkDescriptorBufferBindingInfoEXT bindingSampler{VK_STRUCTURE_TYPE_DESCRIPTOR_BUFFER_BINDING_INFO_EXT};
            bindingSampler.address = vulkanSamplerHeap->GetHeapGPUStartHandle();
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

    void VulkanCommandList::Draw() noexcept {}

    void VulkanCommandList::ResourceBarrier(const ResourceBarrierDesc& desc) noexcept {
        constexpr auto srcStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
        constexpr auto dstStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;

        VkBufferMemoryBarrier bufferBarrier{};
        VkImageMemoryBarrier imageBarrier{};




        switch (desc.type) {
            case ResourceBarrierType::UAV:
                 = desc.UAV.resource;
            break;
            case ResourceBarrierType::Transition:
                barrier.Transition.pResource = desc.transition.resource;
            barrier.Transition.Subresource = 0;
            barrier.Transition.StateBefore = Convert::ToVulkanResourceState(desc.transition.stateBefore);
            barrier.Transition.StateAfter = Convert::ToVulkanResourceState(desc.transition.stateAfter);
            break;
        }

        uint32_t bufferCount = 0;
        VkBufferMemoryBarrier* pBufferBarrier = nullptr;
        uint32_t imageCount = 0;
        VkImageMemoryBarrier* pImageBarrier = nullptr;

        vkCmdPipelineBarrier(m_Cmd, srcStage, dstStage, 0, 0, nullptr, bufferCount, pBufferBarrier, imageCount, pImageBarrier);
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

        VkAccelerationStructureBuildGeometryInfoKHR buildInfo{};
        buildInfo.dstAccelerationStructure = VK_NULL_HANDLE;
        buildInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR |
                             VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_COMPACTION_BIT_KHR;
        buildInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
        buildInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
        buildInfo.geometryCount = 1;
        buildInfo.pGeometries = &asGeom;
        buildInfo.ppGeometries = nullptr;

        VkAccelerationStructureBuildSizesInfoKHR outSizesInfo{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR};
        vkGetAccelerationStructureBuildSizesKHR(m_Context.device, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &buildInfo, nullptr,
                                                &outSizesInfo);


        VkBufferCreateInfo bufferInfo{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
        bufferInfo.size = outSizesInfo.accelerationStructureSize;
        bufferInfo.usage = VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        vkCreateBuffer(m_Context.device, &bufferInfo, m_Context.allocator, &result->buffer);

        // TODO: allocate buffer memory

        VkAccelerationStructureCreateInfoKHR createInfo{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR};
        createInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
        createInfo.buffer = result->buffer;
        createInfo.deviceAddress = NULL;
        createInfo.offset = 0;
        createInfo.size = outSizesInfo.accelerationStructureSize;
        createInfo.createFlags = 0;

        vkCreateAccelerationStructureKHR(m_Context.device, &createInfo, m_Context.allocator, &result->accelerationStructure);

        buildInfo.scratchData = {scratch->deviceAddress};
        buildInfo.dstAccelerationStructure = result->accelerationStructure;
        vkCmdBuildAccelerationStructuresKHR(m_Cmd, 1, &buildInfo, nullptr);
    }

    TLAS* VulkanCommandList::BuildTLAS(const TLASDesc& desc, Buffer* scratchBuffer, size_t scratchBufferStartOffset,
                                       const char* name) noexcept {
        auto* scratch = static_cast<VulkanBuffer*>(scratchBuffer);

        auto result = new VulkanTLAS{};

        for (size_t i = 0; i < desc.blasInstancesCount; ++i) {
            const BLASInstanceDesc& instanceDesc = desc.blasInstances[i];
            VkAccelerationStructureInstanceKHR nativeInstanceDesc{};
            nativeInstanceDesc.flags = 0;
            nativeInstanceDesc.mask = instanceDesc.instanceMask;
            nativeInstanceDesc.instanceCustomIndex = instanceDesc.instanceID;
            nativeInstanceDesc.instanceShaderBindingTableRecordOffset = 0;
            nativeInstanceDesc.accelerationStructureReference = ;
            memcpy(nativeInstanceDesc.transform.matrix, instanceDesc.transform, sizeof(instanceDesc.transform));

            //TODO: copy it to mapped buffer and upload
        }

        VkAccelerationStructureGeometryInstancesDataKHR instancesDesc{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR};
        instancesDesc.data.deviceAddress = instBufferAddr;
        instancesDesc.arrayOfPointers = VK_FALSE;

        VkAccelerationStructureGeometryKHR geoDesc{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR};
        geoDesc.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
        geoDesc.geometry.instances = instancesDesc;

        VkAccelerationStructureBuildGeometryInfoKHR buildInfo{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR};
        buildInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
        buildInfo.geometryCount = 1;
        buildInfo.pGeometries = &geoDesc;
        buildInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
        buildInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
        buildInfo.srcAccelerationStructure = VK_NULL_HANDLE;

        VkAccelerationStructureBuildSizesInfoKHR sizeInfo{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR};
        vkGetAccelerationStructureBuildSizesKHR(m_Context.device, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &buildInfo, nullptr, &sizeInfo);

        VkAccelerationStructureCreateInfoKHR createInfo{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR};
        createInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
        createInfo.size = sizeInfo.accelerationStructureSize;
        createInfo.createFlags = 0;
        createInfo.offset = 0;
        vkCreateAccelerationStructureKHR(m_Context.device, &createInfo, m_Context.allocator, &result->accelerationStructure);

        buildInfo.scratchData = {scratch->deviceAddress};
        buildInfo.dstAccelerationStructure = result->accelerationStructure;
        vkCmdBuildAccelerationStructuresKHR(m_Cmd, 1, &buildInfo, nullptr);
    }
} // namespace RHINO::APIVulkan

#endif// ENABLE_API_VULKAN
