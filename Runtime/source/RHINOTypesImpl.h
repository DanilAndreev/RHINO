#pragma once

namespace RHINO {
    class BufferBase : public Buffer {
    public:
        ResourceType GetResourceType() final { return ResourceType::Buffer; }
    };

    class Texture2DBase : public Texture2D {
        ResourceType GetResourceType() final { return ResourceType::Texture2D; }
    };

    class Texture3DBase : public Texture2D {
        ResourceType GetResourceType() final { return ResourceType::Texture3D; }
    };

    class BLASBase : public BLAS {
        ResourceType GetResourceType() final { return ResourceType::BLAS; }
    };

    class TLASBase : public TLAS {
        ResourceType GetResourceType() final { return ResourceType::TLAS; }
    };
} // namespace RHINO
