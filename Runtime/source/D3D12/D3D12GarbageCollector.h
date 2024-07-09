#pragma once


namespace RHINO::APID3D12 {
    class D3D12GarbageCollector {
    private:
        struct Garbage {
            ID3D12Resource* resource;
            HANDLE event;
        };

    public:
        void AddGarbage(ID3D12Resource* resource, ID3D12Fence* fence, size_t completionValue) noexcept;
        void CollectGarbage() noexcept;
        void Release() noexcept;
    private:
        std::list<Garbage> m_TrackedItems{};
    };
} // namespace RHINO::APID3D12
