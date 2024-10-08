#ifdef ENABLE_API_VULKAN

#include "VulkanBackend.h"
#include "VulkanAPI.h"
#include "VulkanCommandList.h"
#include "VulkanDescriptorHeap.h"
#include "VulkanSwapchain.h"
#include "VulkanUtils.h"
#include "VulkanConverters.h"

namespace RHINO::APIVulkan {
    void VulkanBackend::Initialize() noexcept {
        VkApplicationInfo appInfo{VK_STRUCTURE_TYPE_APPLICATION_INFO};
        appInfo.apiVersion = VK_API_VERSION_1_3;
        appInfo.engineVersion = VK_MAKE_VERSION(0, 1, 0);
        appInfo.applicationVersion = VK_MAKE_VERSION(0, 1, 0);
        appInfo.pApplicationName = "RHINO";
        appInfo.pEngineName = "RHINO";

        const char* instanceExts[] = {
            VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
            VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME,
            VK_KHR_SURFACE_EXTENSION_NAME,
            VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
        };

        VkInstanceCreateInfo instanceInfo{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
        instanceInfo.pApplicationInfo = &appInfo;
        instanceInfo.enabledExtensionCount = RHINO_ARR_SIZE(instanceExts);
        instanceInfo.ppEnabledExtensionNames = instanceExts;
        instanceInfo.enabledLayerCount = 0;
        RHINO_VKS(vkCreateInstance(&instanceInfo, m_Context.allocator, &m_Context.instance));
        LoadVulkanAPI(m_Context.instance, vkGetInstanceProcAddr);

        std::vector<VkPhysicalDevice> physicalDevices{};
        uint32_t physicalDevicesCount = 0;
        RHINO_VKS(vkEnumeratePhysicalDevices(m_Context.instance, &physicalDevicesCount, nullptr));
        physicalDevices.resize(physicalDevicesCount);
        RHINO_VKS(vkEnumeratePhysicalDevices(m_Context.instance, &physicalDevicesCount, physicalDevices.data()));

        for (size_t i = 0; i < physicalDevices.size(); ++i) {
            VkPhysicalDeviceProperties props;
            vkGetPhysicalDeviceProperties(physicalDevices[i], &props);
            if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
                m_Context.physicalDevice = physicalDevices[i];
            }
        }
        m_Context.physicalDevice = m_Context.physicalDevice ? m_Context.physicalDevice : physicalDevices[0];

        VkDeviceQueueCreateInfo queueInfos[3] = {};
        uint32_t queueInfosCount = 0;
        SelectQueues(queueInfos, &queueInfosCount);

        // PHYSICAL DEVICE FEATURES
        VkPhysicalDeviceVulkan12Features physicalDeviceVulkan12Features{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES};
        physicalDeviceVulkan12Features.runtimeDescriptorArray = VK_TRUE;
        physicalDeviceVulkan12Features.bufferDeviceAddress = VK_TRUE;
        physicalDeviceVulkan12Features.descriptorIndexing = VK_TRUE;
        physicalDeviceVulkan12Features.descriptorBindingVariableDescriptorCount = VK_TRUE;
        physicalDeviceVulkan12Features.timelineSemaphore = VK_TRUE;

        VkPhysicalDeviceMutableDescriptorTypeFeaturesEXT deviceMutableDescriptorTypeFeaturesEXT{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MUTABLE_DESCRIPTOR_TYPE_FEATURES_EXT};
        deviceMutableDescriptorTypeFeaturesEXT.pNext = &physicalDeviceVulkan12Features;
        deviceMutableDescriptorTypeFeaturesEXT.mutableDescriptorType = VK_TRUE;

        VkPhysicalDeviceDescriptorBufferFeaturesEXT deviceDescriptorBufferFeaturesExt{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_BUFFER_FEATURES_EXT};
        deviceDescriptorBufferFeaturesExt.pNext = &deviceMutableDescriptorTypeFeaturesEXT;
        deviceDescriptorBufferFeaturesExt.descriptorBuffer = VK_TRUE;
        deviceDescriptorBufferFeaturesExt.descriptorBufferPushDescriptors = VK_TRUE;

        VkPhysicalDeviceFeatures2 deviceFeatures2{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2};
        deviceFeatures2.pNext = &deviceDescriptorBufferFeaturesExt;
        deviceFeatures2.features = {};

        VkDeviceCreateInfo deviceInfo{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
        deviceInfo.pNext = &deviceFeatures2;
        const char* deviceExtensions[] = {
            VK_KHR_TIMELINE_SEMAPHORE_EXTENSION_NAME,
            VK_EXT_DESCRIPTOR_BUFFER_EXTENSION_NAME,
            VK_EXT_MUTABLE_DESCRIPTOR_TYPE_EXTENSION_NAME,
            VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        };
        deviceInfo.enabledExtensionCount = RHINO_ARR_SIZE(deviceExtensions);
        deviceInfo.ppEnabledExtensionNames = deviceExtensions;
        deviceInfo.queueCreateInfoCount = queueInfosCount;
        deviceInfo.pQueueCreateInfos = queueInfos;
        RHINO_VKS(vkCreateDevice(m_Context.physicalDevice, &deviceInfo, m_Context.allocator, &m_Context.device));

        m_DefaultQueueFamIndex = queueInfos[0].queueFamilyIndex;
        m_AsyncComputeQueueFamIndex = queueInfos[1].queueFamilyIndex;
        m_CopyQueueFamIndex = queueInfos[2].queueFamilyIndex;

        vkGetDeviceQueue(m_Context.device, m_DefaultQueueFamIndex, 0, &m_DefaultQueue);
        //TODO: fix (get real mapping)
        vkGetDeviceQueue(m_Context.device, m_AsyncComputeQueueFamIndex, 0, &m_AsyncComputeQueue);
        vkGetDeviceQueue(m_Context.device, m_CopyQueueFamIndex, 0, &m_CopyQueue);
    }

    void VulkanBackend::Release() noexcept {
        vkDestroyDevice(m_Context.device, m_Context.allocator);
        vkDestroyInstance(m_Context.instance, m_Context.allocator);
    }

    RootSignature* VulkanBackend::SerializeRootSignature(const RootSignatureDesc& desc) noexcept {
        auto result = new VulkanRootSignature{};
        result->context = m_Context;

        std::map<size_t, size_t> highestBindingPerSpace{};
        for (size_t spaceIdx = 0; spaceIdx < desc.spacesCount; ++spaceIdx) {
            for (size_t i = 0; i < desc.spacesDescs[spaceIdx].rangeDescCount; ++i) {
                const DescriptorRangeDesc& range = desc.spacesDescs[spaceIdx].rangeDescs[i];
                const size_t binding = range.baseRegisterSlot + range.descriptorsCount;
                if (highestBindingPerSpace.contains(spaceIdx)) {
                    highestBindingPerSpace[spaceIdx] = std::max(highestBindingPerSpace[spaceIdx], binding);
                } else {
                    highestBindingPerSpace[spaceIdx] = binding;
                }
            }
        }

        std::vector<VkDescriptorSetLayout> spaceLayouts{};
        spaceLayouts.resize(desc.spacesCount);
        for (size_t spaceIdx = 0; spaceIdx < desc.spacesCount; ++spaceIdx) {
            const DescriptorSpaceDesc& spaceDesc = desc.spacesDescs[spaceIdx];

            const auto heapOffsetInDescriptors = std::make_pair(spaceDesc.spaceType, spaceDesc.offsetInDescriptorsFromTableStart);
            result->heapOffsetsInDescriptorsBySpace[spaceIdx] = heapOffsetInDescriptors;

            std::vector<VkDescriptorSetLayoutBinding> bindings{};
            bindings.resize(highestBindingPerSpace[spaceIdx] + 1);

            // Fill binding to highest bind slot with sampler / mutable descriptor desc.
            VkMutableDescriptorTypeListEXT fillMutTypeList;
            if (spaceDesc.spaceType == DescriptorHeapType::Sampler) {
                for (uint32_t i = 0; i < bindings.size(); ++i) {
                    bindings[i] = VkDescriptorSetLayoutBinding{i, VK_DESCRIPTOR_TYPE_SAMPLER, 1,
                                                               VK_SHADER_STAGE_COMPUTE_BIT, nullptr};
                }
                fillMutTypeList = VkMutableDescriptorTypeListEXT{0, nullptr};
            }
            else {
                for (uint32_t i = 0; i < bindings.size(); ++i) {
                    bindings[i] = VkDescriptorSetLayoutBinding{i, VK_DESCRIPTOR_TYPE_MUTABLE_EXT, 1,
                                                               VK_SHADER_STAGE_COMPUTE_BIT, nullptr};
                }
                fillMutTypeList = VkMutableDescriptorTypeListEXT{RHINO_ARR_SIZE(VulkanDescriptorHeap::CDBSRVUAVTypes),
                                                                 VulkanDescriptorHeap::CDBSRVUAVTypes};
            }
            std::vector<VkMutableDescriptorTypeListEXT> mutableDescriptorTypeLists{};
            mutableDescriptorTypeLists.resize(bindings.size(), fillMutTypeList);


            VkMutableDescriptorTypeCreateInfoEXT mutableDescriptorTypeCreateInfoExt{
                VK_STRUCTURE_TYPE_MUTABLE_DESCRIPTOR_TYPE_CREATE_INFO_EXT};
            mutableDescriptorTypeCreateInfoExt.mutableDescriptorTypeListCount = mutableDescriptorTypeLists.size();
            mutableDescriptorTypeCreateInfoExt.pMutableDescriptorTypeLists = mutableDescriptorTypeLists.data();

            VkDescriptorSetLayoutCreateInfo setLayoutCreateInfo{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
            setLayoutCreateInfo.pNext = spaceDesc.spaceType == DescriptorHeapType::Sampler ? nullptr : &mutableDescriptorTypeCreateInfoExt;
            setLayoutCreateInfo.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_DESCRIPTOR_BUFFER_BIT_EXT;
            // setLayoutCreateInfo.flags = isSamplerSpace ? 0 : VK_DESCRIPTOR_SET_LAYOUT_CREATE_DESCRIPTOR_BUFFER_BIT_EXT;
            setLayoutCreateInfo.bindingCount = bindings.size();
            setLayoutCreateInfo.pBindings = bindings.data();

            vkCreateDescriptorSetLayout(m_Context.device, &setLayoutCreateInfo, m_Context.allocator, &spaceLayouts[spaceIdx]);

            VkDeviceSize debugSize = 0;
            EXT::vkGetDescriptorSetLayoutSizeEXT(m_Context.device, spaceLayouts[spaceIdx], &debugSize);
            std::cout << "Set " << spaceIdx << " size: " << debugSize << "\n";
            for (size_t bnd = 0; bnd < bindings.size(); ++bnd) {
                VkDeviceSize debugOffset = 0;
                EXT::vkGetDescriptorSetLayoutBindingOffsetEXT(m_Context.device, spaceLayouts[spaceIdx], bnd, &debugOffset);
                std::cout << "  Binding " << bnd << " off: " << debugOffset << std::endl;
            }
        }

        VkPipelineLayoutCreateInfo layoutInfo{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
        layoutInfo.pushConstantRangeCount = 0;
        layoutInfo.setLayoutCount = spaceLayouts.size();
        layoutInfo.pSetLayouts = spaceLayouts.data();
        vkCreatePipelineLayout(m_Context.device, &layoutInfo, m_Context.allocator, &result->layout);

        for (VkDescriptorSetLayout layout: spaceLayouts) {
            vkDestroyDescriptorSetLayout(m_Context.device, layout, m_Context.allocator);
        }
        return result;
    }

    RTPSO* VulkanBackend::CreateRTPSO(const RTPSODesc& desc) noexcept {


        return nullptr;
    }

    ComputePSO* VulkanBackend::CompileComputePSO(const ComputePSODesc& desc) noexcept {
        auto* vulkanRootSignature = INTERPRET_AS<VulkanRootSignature*>(desc.rootSignature);
        auto* result = new VulkanComputePSO{};
        result->context = m_Context;

        VkShaderModuleCreateInfo shaderModuleInfo{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
        shaderModuleInfo.codeSize = desc.CS.bytecodeSize;
        shaderModuleInfo.pCode = reinterpret_cast<const uint32_t*>(desc.CS.bytecode);
        vkCreateShaderModule(m_Context.device, &shaderModuleInfo, m_Context.allocator, &result->shaderModule);

        VkPipelineShaderStageCreateInfo stageInfo{VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO};
        stageInfo.flags = 0;
        stageInfo.module = result->shaderModule;
        stageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
        stageInfo.pName = desc.CS.entrypoint;
        stageInfo.pSpecializationInfo = nullptr;

        VkComputePipelineCreateInfo createInfo{VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO};
        createInfo.flags = VK_PIPELINE_CREATE_DESCRIPTOR_BUFFER_BIT_EXT;
        createInfo.stage = stageInfo;
        createInfo.layout = vulkanRootSignature->layout;
        vkCreateComputePipelines(m_Context.device, VK_NULL_HANDLE, 1, &createInfo, m_Context.allocator, &result->PSO);
        return result;
    }

    Buffer* VulkanBackend::CreateBuffer(size_t size, ResourceHeapType heapType, ResourceUsage usage, size_t structuredStride, const char* name) noexcept {
        auto* result = new VulkanBuffer{};
        result->context = m_Context;
        result->size = size;

        VkBufferCreateInfo createInfo{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
        createInfo.flags = 0;
        createInfo.usage = Convert::ToVkBufferUsage(usage);
        createInfo.usage |= VK_BUFFER_USAGE_RESOURCE_DESCRIPTOR_BUFFER_BIT_EXT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
        createInfo.size = size;
        createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        vkCreateBuffer(m_Context.device, &createInfo, m_Context.allocator, &result->buffer);

        VkMemoryRequirements memReqs;
        vkGetBufferMemoryRequirements(m_Context.device, result->buffer, &memReqs);
        // VkPhysicalDeviceMemoryProperties memoryProps;
        // vkGetPhysicalDeviceMemoryProperties(m_Context.physicalDevice, &memoryProps);

        VkMemoryAllocateFlagsInfo allocateFlagsInfo{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO};
        allocateFlagsInfo.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT;

        VkMemoryAllocateInfo alloc{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
        alloc.pNext = &allocateFlagsInfo;
        alloc.allocationSize = memReqs.size;
        switch (heapType) {
            case ResourceHeapType::Default:
                alloc.memoryTypeIndex = SelectMemoryType(0xffffff, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_Context);
                break;
            case ResourceHeapType::Upload:
            case ResourceHeapType::Readback:
                alloc.memoryTypeIndex = SelectMemoryType(0xffffff, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, m_Context);
                break;
        }
        vkAllocateMemory(m_Context.device, &alloc, m_Context.allocator, &result->memory);

        vkBindBufferMemory(m_Context.device, result->buffer, result->memory, 0);

        VkBufferDeviceAddressInfo bufferInfo{VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO};
        bufferInfo.buffer = result->buffer;
        result->deviceAddress = vkGetBufferDeviceAddress(m_Context.device, &bufferInfo);

        RHINO_GPU_DEBUG(SetDebugName(m_Context.device, result->buffer, VK_OBJECT_TYPE_BUFFER, name));
        return result;
    }

    void* VulkanBackend::MapMemory(Buffer* buffer, size_t offset, size_t size) noexcept {
        auto* vulkanBuffer = INTERPRET_AS<VulkanBuffer*>(buffer);
        void* result;
        vkMapMemory(m_Context.device, vulkanBuffer->memory, offset, size, 0, &result);
        return result;
    }

    void VulkanBackend::UnmapMemory(Buffer* buffer) noexcept {
        auto* vulkanBuffer = INTERPRET_AS<VulkanBuffer*>(buffer);
        vkUnmapMemory(m_Context.device, vulkanBuffer->memory);
    }

    Texture2D* VulkanBackend::CreateTexture2D(const Dim3D& dimensions, size_t mips, TextureFormat format, ResourceUsage usage,
                                              const char* name) noexcept {
        auto* result = new VulkanTexture2D{};
        result->context = m_Context;
        result->origimalFormat = Convert::ToVkFormat(format);

        VkImageCreateInfo imageInfo{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
        imageInfo.flags = VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent.width = dimensions.width;
        imageInfo.extent.height = dimensions.height;
        imageInfo.extent.depth = 1;
        imageInfo.format = result->origimalFormat;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.tiling = VK_IMAGE_TILING_LINEAR;
        imageInfo.usage = Convert::ToVkImageUsage(usage);
        imageInfo.arrayLayers = 1;
        imageInfo.mipLevels = mips;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        RHINO_VKS(vkCreateImage(m_Context.device, &imageInfo, m_Context.allocator, &result->texture));

        VkMemoryRequirements memReqs;
        vkGetImageMemoryRequirements(m_Context.device, result->texture, &memReqs);

        VkMemoryAllocateInfo allocInfo{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
        allocInfo.allocationSize = memReqs.size;
        allocInfo.memoryTypeIndex = SelectMemoryType(0xffffff, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_Context);
        RHINO_VKS(vkAllocateMemory(m_Context.device, &allocInfo, m_Context.allocator, &result->memory));
        RHINO_VKS(vkBindImageMemory(m_Context.device, result->texture, result->memory, 0));

        RHINO_GPU_DEBUG(SetDebugName(m_Context.device, result->texture, VK_OBJECT_TYPE_IMAGE, name));
        return result;
    }

    Sampler* VulkanBackend::CreateSampler(const SamplerDesc& desc) noexcept {
        auto* result = new VulkanSampler{};
        result->context = m_Context;

        Convert::VulkanMinMagMipFilters filters = Convert::ToMTLMinMagMipFilter(desc.textureFilter);
        VkSamplerCreateInfo samplerInfo{VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
        samplerInfo.flags = 0;
        samplerInfo.minFilter = filters.min;
        samplerInfo.magFilter = filters.mag;
        samplerInfo.mipmapMode = filters.mip;
        samplerInfo.addressModeU = Convert::ToVkSamplerAddressMode(desc.addresU);
        samplerInfo.addressModeV = Convert::ToVkSamplerAddressMode(desc.addresV);
        samplerInfo.addressModeW = Convert::ToVkSamplerAddressMode(desc.addresW);
        samplerInfo.borderColor = Convert::ToVkBorderColor(desc.borderColor);
        samplerInfo.compareEnable = desc.comparisonFunc != ComparisonFunction::Never;
        samplerInfo.compareOp = Convert::ToVkCompareOp(desc.comparisonFunc);
        samplerInfo.anisotropyEnable = desc.maxAnisotropy > 1;
        samplerInfo.maxAnisotropy = static_cast<float>(desc.maxAnisotropy);
        samplerInfo.minLod = desc.minLOD;
        samplerInfo.maxLod = desc.maxLOD;
        samplerInfo.mipLodBias = 0;
        samplerInfo.unnormalizedCoordinates = false;
        vkCreateSampler(m_Context.device, &samplerInfo, m_Context.allocator, &result->sampler);
        return result;
    }

    DescriptorHeap* VulkanBackend::CreateDescriptorHeap(DescriptorHeapType type, size_t descriptorsCount, const char* name) noexcept {
        auto* result = new VulkanDescriptorHeap{};
        result->Initialize(name, type, descriptorsCount, m_Context);
        return result;
    }

    Swapchain* VulkanBackend::CreateSwapchain(const SwapchainDesc& desc) noexcept {
        auto* result = new VulkanSwapchain{};
        result->Initialize(m_Context, desc, m_DefaultQueueFamIndex);
        return result;
    }

    CommandList* VulkanBackend::AllocateCommandList(const char* name) noexcept {
        auto* result = new VulkanCommandList{};
        result->Initialize(name, m_Context, m_DefaultQueueFamIndex);
        return result;
    }

    void VulkanBackend::SubmitCommandList(CommandList* cmd) noexcept {
        auto* vulkanCMD = INTERPRET_AS<VulkanCommandList*>(cmd);
        vulkanCMD->SubmitToQueue(m_DefaultQueue);
    }

    void VulkanBackend::SwapchainPresent(Swapchain* swapchain, Texture2D* toPresent, size_t width, size_t height) noexcept {
        auto* vulkanSwapchain = INTERPRET_AS<VulkanSwapchain*>(swapchain);
        auto* vulkanTexture = INTERPRET_AS<VulkanTexture2D*>(toPresent);
        vulkanSwapchain->Present(m_DefaultQueue, vulkanTexture, width, height);
    }

    Semaphore* VulkanBackend::CreateSyncSemaphore(uint64_t initialValue) noexcept {
        auto* result = new VulkanSemaphore{};
        result->context = m_Context;

        VkSemaphoreTypeCreateInfo timelineCreateInfo{VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO};
        timelineCreateInfo.pNext = nullptr;
        timelineCreateInfo.semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE;
        timelineCreateInfo.initialValue = initialValue;

        VkSemaphoreCreateInfo createInfo{VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
        createInfo.pNext = &timelineCreateInfo;
        createInfo.flags = 0;
        RHINO_VKS(vkCreateSemaphore(m_Context.device, &createInfo, m_Context.allocator, &result->semaphore));

        return result;
    }

    void VulkanBackend::SignalFromQueue(Semaphore* semaphore, uint64_t value) noexcept {
        auto* vulkanSemaphore = INTERPRET_AS<VulkanSemaphore*>(semaphore);

        VkTimelineSemaphoreSubmitInfo timelineInfo{VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO};
        timelineInfo.signalSemaphoreValueCount = 1;
        timelineInfo.pSignalSemaphoreValues = &value;

        VkSubmitInfo submitInfo{VK_STRUCTURE_TYPE_SUBMIT_INFO};
        submitInfo.pNext = &timelineInfo;
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = &vulkanSemaphore->semaphore;

        vkQueueSubmit(m_DefaultQueue, 1, &submitInfo, VK_NULL_HANDLE);
    }

    void VulkanBackend::SignalFromHost(Semaphore* semaphore, uint64_t value) noexcept {
        auto* vulkanSemaphore = INTERPRET_AS<VulkanSemaphore*>(semaphore);

        VkSemaphoreSignalInfo signalInfo{VK_STRUCTURE_TYPE_SEMAPHORE_SIGNAL_INFO};
        signalInfo.semaphore = vulkanSemaphore->semaphore;
        signalInfo.value = value;

        vkSignalSemaphore(m_Context.device, &signalInfo);
    }

    bool VulkanBackend::SemaphoreWaitFromHost(const Semaphore* semaphore, uint64_t value, size_t timeout) noexcept {
        const auto* vulkanSemaphore = INTERPRET_AS<const VulkanSemaphore*>(semaphore);

        VkSemaphoreWaitInfo waitInfo{VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO};
        waitInfo.flags = 0;
        waitInfo.semaphoreCount = 1;
        waitInfo.pSemaphores = &vulkanSemaphore->semaphore;
        waitInfo.pValues = &value;

        const VkResult status = vkWaitSemaphores(m_Context.device, &waitInfo, timeout);
        return status == VK_SUCCESS;
    }

    void VulkanBackend::SemaphoreWaitFromQueue(const Semaphore* semaphore, uint64_t value) noexcept {
        const auto* vulkanSemaphore = INTERPRET_AS<const VulkanSemaphore*>(semaphore);

        VkTimelineSemaphoreSubmitInfo timelineInfo{VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO};
        timelineInfo.waitSemaphoreValueCount = 1;
        timelineInfo.pWaitSemaphoreValues = &value;

        VkPipelineStageFlags stage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;

        VkSubmitInfo submitInfo{VK_STRUCTURE_TYPE_SUBMIT_INFO};
        submitInfo.pNext = &timelineInfo;
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = &vulkanSemaphore->semaphore;
        submitInfo.pWaitDstStageMask = &stage;
        vkQueueSubmit(m_DefaultQueue, 1, &submitInfo, VK_NULL_HANDLE);
    }

    uint64_t VulkanBackend::GetSemaphoreCompletedValue(const Semaphore* semaphore) noexcept {
        const auto* vulkanSemaphore = INTERPRET_AS<const VulkanSemaphore*>(semaphore);
        uint64_t result;
        vkGetSemaphoreCounterValue(m_Context.device, vulkanSemaphore->semaphore, &result);
        return result;
    }

    void VulkanBackend::SelectQueues(VkDeviceQueueCreateInfo queueInfos[3], uint32_t* infosCount) noexcept {
        uint32_t queuesCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(m_Context.physicalDevice, &queuesCount, nullptr);
        std::vector<VkQueueFamilyProperties> queues;
        queues.resize(queuesCount);
        vkGetPhysicalDeviceQueueFamilyProperties(m_Context.physicalDevice, &queuesCount, queues.data());

        uint32_t graphicsQueueIndex = ~0;
        uint32_t computeQueueIndex = ~0;
        uint32_t copyQueueIndex = ~0;

        for (size_t i = 0; i < queuesCount; ++i) {
            if (queues[i].queueFlags & (VK_QUEUE_COMPUTE_BIT | VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_TRANSFER_BIT) &&
                queues[i].queueCount >= 3 && graphicsQueueIndex == ~0 && computeQueueIndex == ~0 && copyQueueIndex == ~0) {
                graphicsQueueIndex = i;
                computeQueueIndex = i;
                copyQueueIndex = i;
                break;
            }
            else if (queues[i].queueFlags & (VK_QUEUE_COMPUTE_BIT | VK_QUEUE_GRAPHICS_BIT) && queues[i].queueCount >= 2 &&
                     graphicsQueueIndex == ~0 && computeQueueIndex == ~0) {
                graphicsQueueIndex = i;
                computeQueueIndex = i;
            }
            else if (queues[i].queueFlags & (VK_QUEUE_COMPUTE_BIT | VK_QUEUE_TRANSFER_BIT) && queues[i].queueCount >= 2 &&
                     computeQueueIndex == ~0 && copyQueueIndex == ~0) {
                computeQueueIndex = i;
                copyQueueIndex = i;
            }
            else if (queues[i].queueFlags & (VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_TRANSFER_BIT) && queues[i].queueCount >= 2 &&
                     graphicsQueueIndex == ~0 && copyQueueIndex == ~0) {
                graphicsQueueIndex = i;
                copyQueueIndex = i;
            }
            else if (queues[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                graphicsQueueIndex = i;
            }
            else if (queues[i].queueFlags & VK_QUEUE_COMPUTE_BIT) {
                computeQueueIndex = i;
            }
            else if (queues[i].queueFlags & VK_QUEUE_TRANSFER_BIT) {
                copyQueueIndex = i;
            }
        }

        static const float priorities[3] = {0.5, 0.5, 0.5};
        if (graphicsQueueIndex == computeQueueIndex && computeQueueIndex == copyQueueIndex) {
            queueInfos[0].queueCount = 3;
            queueInfos[0].queueFamilyIndex = graphicsQueueIndex;
            queueInfos[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueInfos[0].pQueuePriorities = priorities;
            *infosCount = 1;
        }
        else if (graphicsQueueIndex == computeQueueIndex) {
            *infosCount = 2;

            queueInfos[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueInfos[0].queueCount = 2;
            queueInfos[0].queueFamilyIndex = graphicsQueueIndex;
            queueInfos[0].pQueuePriorities = priorities;

            queueInfos[1].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueInfos[1].queueCount = 1;
            queueInfos[1].queueFamilyIndex = copyQueueIndex;
            queueInfos[1].pQueuePriorities = priorities;
        }
        else if (graphicsQueueIndex == copyQueueIndex) {
            *infosCount = 2;

            queueInfos[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueInfos[0].queueCount = 2;
            queueInfos[0].queueFamilyIndex = graphicsQueueIndex;
            queueInfos[0].pQueuePriorities = priorities;

            queueInfos[1].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueInfos[1].queueCount = 1;
            queueInfos[1].queueFamilyIndex = computeQueueIndex;
            queueInfos[1].pQueuePriorities = priorities;
        }
        else if (computeQueueIndex == copyQueueIndex) {
            *infosCount = 2;

            queueInfos[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueInfos[0].queueCount = 2;
            queueInfos[0].queueFamilyIndex = computeQueueIndex;
            queueInfos[0].pQueuePriorities = priorities;

            queueInfos[1].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueInfos[1].queueCount = 1;
            queueInfos[1].queueFamilyIndex = graphicsQueueIndex;
            queueInfos[1].pQueuePriorities = priorities;
        }
        else {
            *infosCount = 3;

            queueInfos[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueInfos[0].queueCount = 1;
            queueInfos[0].queueFamilyIndex = graphicsQueueIndex;
            queueInfos[0].pQueuePriorities = priorities;

            queueInfos[1].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueInfos[1].queueCount = 1;
            queueInfos[1].queueFamilyIndex = computeQueueIndex;
            queueInfos[1].pQueuePriorities = priorities;

            queueInfos[2].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueInfos[2].queueCount = 1;
            queueInfos[2].queueFamilyIndex = computeQueueIndex;
            queueInfos[2].pQueuePriorities = priorities;
        }
    }

    ASPrebuildInfo VulkanBackend::GetBLASPrebuildInfo(const BLASDesc& desc) noexcept {
        VkDeviceOrHostAddressConstKHR constDummyAddr{VkDeviceAddress{NULL}};
        VkDeviceOrHostAddressKHR dummyAddr{VkDeviceAddress{NULL}};

        VkAccelerationStructureGeometryKHR asGeom{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR};
        asGeom.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
        asGeom.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
        asGeom.geometry.triangles.indexType = Convert::ToVkIndexType(desc.indexFormat);
        asGeom.geometry.triangles.indexData = constDummyAddr;
        asGeom.geometry.triangles.vertexStride = desc.vertexStride;
        asGeom.geometry.triangles.vertexFormat = Convert::ToVkFormat(desc.vertexFormat);
        asGeom.geometry.triangles.vertexData = constDummyAddr;
        asGeom.geometry.triangles.maxVertex = desc.vertexCount;
        asGeom.geometry.triangles.transformData = constDummyAddr;

        VkAccelerationStructureBuildGeometryInfoKHR buildInfo{};
        buildInfo.dstAccelerationStructure = VK_NULL_HANDLE;
        buildInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR |
                          VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_COMPACTION_BIT_KHR;
        buildInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
        buildInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
        buildInfo.geometryCount = 1;
        buildInfo.pGeometries = &asGeom;
        buildInfo.ppGeometries = nullptr;
        buildInfo.scratchData = dummyAddr;

        VkAccelerationStructureBuildSizesInfoKHR outSizesInfo{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR};

        EXT::vkGetAccelerationStructureBuildSizesKHR(m_Context.device, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &buildInfo, nullptr,
                                                &outSizesInfo);
        ASPrebuildInfo result{};
        result.scratchBufferSizeInBytes = outSizesInfo.buildScratchSize;
        result.MaxASSizeInBytes = outSizesInfo.accelerationStructureSize;
        return result;
    }

    ASPrebuildInfo VulkanBackend::GetTLASPrebuildInfo(const TLASDesc& desc) noexcept {
        ASPrebuildInfo result{};
        return result;
    }
} // namespace RHINO::APIVulkan

#endif// ENABLE_API_VULKAN
