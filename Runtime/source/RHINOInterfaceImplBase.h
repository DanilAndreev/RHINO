#pragma once

#include <RHINO.h>

namespace RHINO {

class RHINOInterfaceImplBase : public RHINOInterface {
public:
    ComputePSO* CompileSCARComputePSO(const void* scar, uint32_t sizeInBytes, RootSignature* rootSignature,
                                      const char* debugName) noexcept final;
    RTPSO* CreateSCARRTPSO(const void* scar, uint32_t sizeInBytes, const RTPSODesc& desc) noexcept final;
};

} // RHINO
