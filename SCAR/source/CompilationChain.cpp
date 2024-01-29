#include "CompilationChain.h"

namespace SCAR {

    CompilationChain::~CompilationChain() noexcept {
        delete m_Next;
    }

    bool CompilationChain::Run(const CompileSettings& settings, const ChainSettings& chSettings,
                               ChainContext& context) noexcept {
        bool status = Execute(settings, chSettings, context);
        if (!status) {
            return status;
        }
        if (m_Next) {
            status = m_Next->Run(settings, chSettings, context);
        }
        return status;
    }
    CompilationChain* CompilationChain::SetNext(CompilationChain* next) noexcept {
        m_Next = next;
        return m_Next;
    }
    CompilationChain* CompilationChain::GetNext() const noexcept { return m_Next; }
} // namespace SCAR
