#pragma once

#ifdef ENABLE_API_VULKAN

#include "RHINOInterfaceImplBase.h"
#include "VulkanBackendTypes.h"

namespace RHINO::APIVulkan {
    class VulkanDescriptorHeap;
    class VulkanCommandList;

    class VulkanBackend : public RHINOInterfaceImplBase {
    public:
        explicit VulkanBackend() noexcept = default;

    public:
        void Initialize() noexcept final;
        void Release() noexcept final;

    public:
        RTPSO* CreateRTPSO(const RTPSODesc& desc) noexcept final;
        ComputePSO* CompileComputePSO(const ComputePSODesc& desc) noexcept final;

        Buffer* CreateBuffer(size_t size, ResourceHeapType heapType, ResourceUsage usage, size_t structuredStride, const char* name) noexcept final;
        void* MapMemory(Buffer* buffer, size_t offset, size_t size) noexcept final;
        void UnmapMemory(Buffer* buffer) noexcept final;
        Texture2D* CreateTexture2D(const Dim3D& dimensions, size_t mips, TextureFormat format, ResourceUsage usage,
                           const char* name) noexcept final;
        DescriptorHeap* CreateDescriptorHeap(DescriptorHeapType type, size_t descriptorsCount, const char* name) noexcept final;
        Swapchain* CreateSwapchain(const SwapchainDesc& desc) noexcept final;

        CommandList* AllocateCommandList(const char* name) noexcept final;

        ASPrebuildInfo GetBLASPrebuildInfo(const BLASDesc& desc) noexcept final;
        ASPrebuildInfo GetTLASPrebuildInfo(const TLASDesc& desc) noexcept final;

    public:
        void SubmitCommandList(CommandList* cmd) noexcept final;
        void SwapchainPresent(Swapchain* swapchain) noexcept final;

    public:
        Semaphore* CreateSyncSemaphore(uint64_t initialValue) noexcept final;

        void SignalFromQueue(Semaphore* semaphore, uint64_t value) noexcept final;
        void SignalFromHost(Semaphore* semaphore, uint64_t value) noexcept final;
        bool SemaphoreWaitFromHost(const Semaphore* semaphore, uint64_t value, size_t timeout) noexcept final;
        void SemaphoreWaitFromQueue(const Semaphore* semaphore, uint64_t value) noexcept final;
        uint64_t GetSemaphoreCompletedValue(const Semaphore* semaphore) noexcept final;

    private:
        void SelectQueues(VkDeviceQueueCreateInfo queueInfos[3], uint32_t* infosCount) noexcept;
        VulkanObjectContext CreateVulkanObjectContext() const noexcept;
    private:
        VkInstance m_Instance = VK_NULL_HANDLE;
        VkDevice m_Device = VK_NULL_HANDLE;
        VkPhysicalDevice m_PhysicalDevice = VK_NULL_HANDLE;

        VkQueue m_DefaultQueue = VK_NULL_HANDLE;
        uint32_t m_DefaultQueueFamIndex = 0;
        VkCommandPool m_DefaultQueueCMDPool = VK_NULL_HANDLE;
        VkQueue m_AsyncComputeQueue = VK_NULL_HANDLE;
        uint32_t m_AsyncComputeQueueFamIndex = 0;
        VkCommandPool m_AsyncComputeQueueCMDPool = VK_NULL_HANDLE;
        VkQueue m_CopyQueue = VK_NULL_HANDLE;
        uint32_t m_CopyQueueFamIndex = 0;
        VkCommandPool m_CopyQueueCMDPool = VK_NULL_HANDLE;

        VkAllocationCallbacks* m_Alloc = nullptr;
    };
}// namespace RHINO::APIVulkan

#endif//  ENABLE_API_VULKAN