#pragma once

#ifdef ENABLE_API_D3D12

#ifdef _DEBUG
#define RHINO_D3DS(expr) assert(expr == S_OK)
#else
#define RHINO_D3DS(expr) expr
#endif

inline void SetDebugName(ID3D12DeviceChild* resource, const std::string& name) noexcept {
    resource->SetPrivateData(WKPDID_D3DDebugObjectName, name.length(), name.c_str());
}

#endif // ENABLE_API_D3D12
