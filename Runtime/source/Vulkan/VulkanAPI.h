#pragma once

#ifdef ENABLE_API_VULKAN

#ifdef _DEBUG
#define RHINO_VKS(result) assert(result == VK_SUCCESS)
#else
#define RHINO_VKS(result) result
#endif

#define RHINO_VULKAN_API_FUNCS()                                                                                                           \
    RHINO_APPLY(vkGetDescriptorEXT)                                                                                                        \
    RHINO_APPLY(vkCmdBindDescriptorBuffersEXT)                                                                                             \
    RHINO_APPLY(vkCmdSetDescriptorBufferOffsetsEXT)                                                                                        \
    RHINO_APPLY(vkGetDescriptorSetLayoutBindingOffsetEXT)                                                                                  \
    RHINO_APPLY(vkGetAccelerationStructureBuildSizesKHR)                                                                                   \
    RHINO_APPLY(vkCreateAccelerationStructureKHR)                                                                                          \
    RHINO_APPLY(vkDestroyAccelerationStructureKHR)                                                                                         \
    RHINO_APPLY(vkCmdBuildAccelerationStructuresKHR)                                                                                       \
    RHINO_APPLY(vkCmdTraceRaysKHR)                                                                                                         \
    RHINO_APPLY(vkGetDescriptorSetLayoutSizeEXT)


namespace RHINO::APIVulkan {
    namespace EXT {
#define RHINO_APPLY(f) extern ::PFN_##f f;
        RHINO_VULKAN_API_FUNCS()
#undef RHINO_APPLY
    } // namespace EXT

    void LoadVulkanAPI(VkInstance instance, PFN_vkGetInstanceProcAddr getProcAddr) noexcept;

    std::string VkResultToStr(VkResult s) noexcept;
} // namespace RHINO::APIVulkan

#endif // ENABLE_API_VULKAN
