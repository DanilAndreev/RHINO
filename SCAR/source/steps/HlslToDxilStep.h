#pragma once

#include "ICompilationStep.h"

namespace SCAR {
    class HlslToDxilStep final : public ICompilationStep {
    public:
        bool Execute(const CompileSettings& settings, const ChainSettings& chSettings,
                     ChainContext& context) noexcept final;
    };
} // namespace SCAR
