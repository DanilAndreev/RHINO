#include "RHINOInterfaceImplBase.h"
#include "SCARTools/SCARComputePSOArchiveView.h"
#include "SCARTools/SCARRTPSOArchiveView.h"

namespace RHINO {
    ComputePSO* RHINOInterfaceImplBase::CompileSCARComputePSO(const void* scar, uint32_t sizeInBytes, RootSignature* rootSignature,
                                                              const char* debugName) noexcept {
        const SCARTools::SCARComputePSOArchiveView view{scar, sizeInBytes, debugName};
        if (!view.IsValid()) {
            return nullptr;
        }
        ComputePSODesc desc = view.GetDesc();
        desc.rootSignature = rootSignature;
        return CompileComputePSO(desc);
    }

    RTPSO* RHINOInterfaceImplBase::CreateSCARRTPSO(const void* scar, uint32_t sizeInBytes, const RTPSODesc& desc) noexcept {
        const SCARTools::SCARRTPSOArchiveView view{scar, sizeInBytes, desc};
        if (!view.IsValid()) {
            return nullptr;
        }
        return CreateRTPSO(view.GetPatchedDesc());
    }
} // RHINO
