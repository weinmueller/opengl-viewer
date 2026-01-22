#pragma once

#include "LODLevel.h"
#include "LODSelector.h"
#include <vector>
#include <memory>

// Container managing multiple LOD levels per object
class LODMesh {
public:
    LODMesh() = default;

    // Add a LOD level (must be added in order from highest to lowest detail)
    void addLevel(LODLevel&& level);

    // Set all LOD levels at once
    void setLevels(std::vector<LODLevel>&& levels);

    // Clear all LOD levels
    void clear();

    // Get number of LOD levels
    size_t getLevelCount() const { return m_levels.size(); }

    // Check if LOD levels exist
    bool hasLOD() const { return !m_levels.empty(); }

    // Get specific LOD level
    LODLevel* getLevel(size_t index);
    const LODLevel* getLevel(size_t index) const;

    // Select appropriate LOD based on screen size
    // Returns the mesh to render (or nullptr if no valid LOD)
    Mesh* selectLOD(float screenSize);

    // Force a specific LOD level (for debugging)
    void forceLOD(int level);

    // Clear forced LOD (return to automatic selection)
    void clearForcedLOD();

    // Get current LOD index (for debug display)
    int getCurrentLODIndex() const { return m_currentLOD; }

    // Get current LOD triangle count
    uint32_t getCurrentTriangleCount() const;

    // Get total triangles across all LOD levels (for stats)
    uint32_t getTotalTriangleCount() const;

    // Check if LOD generation is in progress
    bool isGenerating() const { return m_generating; }
    void setGenerating(bool generating) { m_generating = generating; }

private:
    std::vector<LODLevel> m_levels;
    int m_currentLOD{0};
    int m_forcedLOD{-1};  // -1 = automatic selection
    bool m_generating{false};
};
