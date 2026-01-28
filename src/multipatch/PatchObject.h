#pragma once

#include "scene/SceneObject.h"
#include "mesh/MeshData.h"
#include <memory>
#include <string>
#include <functional>

#ifdef GISMO_AVAILABLE
namespace gismo {
    template<class T> class gsGeometry;
}
#endif

// Callback type for re-tessellation
using TessellationCallback = std::function<MeshData(int uSamples, int vSamples)>;

// SceneObject with dynamic tessellation support for NURBS/B-spline patches
class PatchObject : public SceneObject {
public:
    PatchObject(const std::string& name, int patchIndex);

    // Set the tessellation callback (captures the G+Smo patch reference)
    void setTessellationCallback(TessellationCallback callback);

    // Current tessellation level (samples per direction)
    int getTessellationLevel() const { return m_tessellationLevel; }
    void setTessellationLevel(int level);

    // Request re-tessellation at a new level (non-blocking check)
    bool needsRetessellation() const { return m_pendingTessLevel != m_tessellationLevel; }
    void requestTessellation(int newLevel);

    // Apply re-tessellated mesh (called after background tessellation completes)
    void applyRetessellatedMesh(MeshData&& data, int newLevel);

    // Get patch index within the multipatch
    int getPatchIndex() const { return m_patchIndex; }

    // Get pending tessellation level
    int getPendingTessellationLevel() const { return m_pendingTessLevel; }

    // Check if patch is being retessellated
    bool isRetessellating() const { return m_isRetessellating; }
    void setRetessellating(bool value) { m_isRetessellating = value; }

    // Tessellate synchronously (for initial load)
    void tessellateSync(int level);

private:
    int m_patchIndex{0};
    int m_tessellationLevel{16};   // Current tessellation level
    int m_pendingTessLevel{16};    // Requested level (may differ during async retess)
    bool m_isRetessellating{false};
    TessellationCallback m_tessCallback;
};
