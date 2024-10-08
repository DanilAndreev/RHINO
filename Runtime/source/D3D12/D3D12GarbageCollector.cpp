#ifdef ENABLE_API_D3D12

#include "D3D12GarbageCollector.h"

namespace RHINO::APID3D12 {

    void D3D12GarbageCollector::AddGarbage(ID3D12Resource* resource, ID3D12Fence* fence, size_t completionValue) noexcept {
        HANDLE cpuEvent = ::CreateEventEx(NULL, FALSE, FALSE, EVENT_ALL_ACCESS);
        fence->SetEventOnCompletion(completionValue, cpuEvent);
        m_TrackedItems.emplace_back(resource, cpuEvent);
    }

    void D3D12GarbageCollector::CollectGarbage() noexcept {
        auto i = m_TrackedItems.begin();
        while (i != m_TrackedItems.end()) {
            if (WaitForSingleObject(i->event, 0) == WAIT_OBJECT_0) {
                i->resource->Release();
                i = m_TrackedItems.erase(i);
                CloseHandle(i->event);
            }
        }
    }

    void D3D12GarbageCollector::Release() noexcept {
        for (auto& item: m_TrackedItems) {
            WaitForSingleObject(item.event, INFINITE);
            CloseHandle(item.event);
            item.resource->Release();
        }
        m_TrackedItems.clear();
    }
} // namespace RHINO::APID3D12

#endif // ENABLE_API_D3D12
