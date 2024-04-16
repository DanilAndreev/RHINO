#pragma once

#ifdef ENABLE_API_VULKAN

#include "VulkanBackendTypes.h"

namespace RHINO::APIVulkan {
    class VulkanDescriptorHeap;
    class VulkanCommandList;

    class VulkanBackend final : public RHINOInterface {
    public:
        explicit VulkanBackend() noexcept = default;

    public:
        void Initialize() noexcept final;
        void Release() noexcept final;

    public:
        RTPSO* CompileRTPSO(const RTPSODesc& desc) noexcept final;
        void ReleaseRTPSO(RTPSO* pso) noexcept final;
        ComputePSO* CompileComputePSO(const ComputePSODesc& desc) noexcept final;
        ComputePSO* CompileSCARComputePSO(const void* scar, uint32_t sizeInBytes,
                                          const char* debugName) noexcept final;
        void ReleaseComputePSO(ComputePSO* pso) noexcept final;

        Buffer* CreateBuffer(size_t size, ResourceHeapType heapType, ResourceUsage usage, size_t structuredStride, const char* name) noexcept final;
        void ReleaseBuffer(Buffer* buffer) noexcept final;
        void* MapMemory(Buffer* buffer, size_t offset, size_t size) noexcept final;
        void UnmapMemory(Buffer* buffer) noexcept final;
        Texture2D* CreateTexture2D(const Dim3D& dimensions, size_t mips, TextureFormat format, ResourceUsage usage,
                           const char* name) noexcept final;
        void ReleaseTexture2D(Texture2D* texture) noexcept final;
        DescriptorHeap* CreateDescriptorHeap(DescriptorHeapType type, size_t descriptorsCount, const char* name) noexcept final;
        void ReleaseDescriptorHeap(DescriptorHeap* heap) noexcept final;

        CommandList* AllocateCommandList(const char* name) noexcept final;
        void ReleaseCommandList(CommandList* commandList) noexcept final;

    public:
        void SubmitCommandList(CommandList* cmd) noexcept override;

    private:
        uint32_t SelectMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) noexcept;
        void SelectQueues(VkDeviceQueueCreateInfo queueInfos[3], uint32_t* infosCount) noexcept;
    private:
        static VkDescriptorType ToDescriptorType(DescriptorType type) noexcept;
        static VkBufferUsageFlags ToBufferUsage(ResourceUsage usage) noexcept;
    private:
        VkInstance m_Instance = VK_NULL_HANDLE;
        VkDevice m_Device = VK_NULL_HANDLE;
        VkPhysicalDevice m_PhysicalDevice = VK_NULL_HANDLE;

        VkQueue m_DefaultQueue = VK_NULL_HANDLE;
        uint32_t m_DefaultQueueFamIndex = 0;
        VkQueue m_AsyncComputeQueue = VK_NULL_HANDLE;
        uint32_t m_AsyncComputeQueueFamIndex = 0;
        VkQueue m_CopyQueue = VK_NULL_HANDLE;
        uint32_t m_CopyQueueFamIndex = 0;

        VkAllocationCallbacks* m_Alloc = nullptr;

        // DEBUG
        VkSwapchainKHR m_Swapchain = VK_NULL_HANDLE;
        // std::vector<VkSemaphore> m_SwapchainSemaphores{};
        uint32_t m_SwapchainCurrentImage = 0;
    };
}// namespace RHINO::APIVulkan

#endif//  ENABLE_API_VULKAN