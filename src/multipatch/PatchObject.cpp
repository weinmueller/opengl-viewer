#include "PatchObject.h"
#include <iostream>

PatchObject::PatchObject(const std::string& name, int patchIndex)
    : SceneObject(name)
    , m_patchIndex(patchIndex)
{
}

void PatchObject::setTessellationCallback(TessellationCallback callback) {
    m_tessCallback = std::move(callback);
}

void PatchObject::setTessellationLevel(int level) {
    m_tessellationLevel = level;
    m_pendingTessLevel = level;
}

void PatchObject::requestTessellation(int newLevel) {
    if (newLevel != m_tessellationLevel && !m_isRetessellating) {
        m_pendingTessLevel = newLevel;
    }
}

void PatchObject::applyRetessellatedMesh(MeshData&& data, int newLevel) {
    // Apply the new mesh data
    applySubdividedMesh(std::move(data));
    m_tessellationLevel = newLevel;
    m_pendingTessLevel = newLevel;
    m_isRetessellating = false;
}

void PatchObject::tessellateSync(int level) {
    if (!m_tessCallback) {
        std::cerr << "PatchObject: No tessellation callback set for " << getName() << std::endl;
        return;
    }

    MeshData meshData = m_tessCallback(level, level);
    setMeshData(meshData);
    m_tessellationLevel = level;
    m_pendingTessLevel = level;
}
