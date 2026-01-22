#include "LODMesh.h"

void LODMesh::addLevel(LODLevel&& level) {
    m_levels.push_back(std::move(level));
}

void LODMesh::setLevels(std::vector<LODLevel>&& levels) {
    m_levels = std::move(levels);
    m_currentLOD = 0;
}

void LODMesh::clear() {
    m_levels.clear();
    m_currentLOD = 0;
    m_forcedLOD = -1;
    m_generating = false;
}

LODLevel* LODMesh::getLevel(size_t index) {
    if (index < m_levels.size()) {
        return &m_levels[index];
    }
    return nullptr;
}

const LODLevel* LODMesh::getLevel(size_t index) const {
    if (index < m_levels.size()) {
        return &m_levels[index];
    }
    return nullptr;
}

Mesh* LODMesh::selectLOD(float screenSize) {
    if (m_levels.empty()) {
        return nullptr;
    }

    int lodIndex;
    if (m_forcedLOD >= 0 && m_forcedLOD < static_cast<int>(m_levels.size())) {
        lodIndex = m_forcedLOD;
    } else {
        lodIndex = LODSelector::selectLOD(screenSize, m_currentLOD, static_cast<int>(m_levels.size()));
    }

    m_currentLOD = lodIndex;
    return m_levels[lodIndex].getMesh();
}

void LODMesh::forceLOD(int level) {
    m_forcedLOD = level;
}

void LODMesh::clearForcedLOD() {
    m_forcedLOD = -1;
}

uint32_t LODMesh::getCurrentTriangleCount() const {
    if (m_currentLOD >= 0 && m_currentLOD < static_cast<int>(m_levels.size())) {
        return m_levels[m_currentLOD].triangleCount;
    }
    return 0;
}

uint32_t LODMesh::getTotalTriangleCount() const {
    uint32_t total = 0;
    for (const auto& level : m_levels) {
        total += level.triangleCount;
    }
    return total;
}
