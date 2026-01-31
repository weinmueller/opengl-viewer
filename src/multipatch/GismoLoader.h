#pragma once

#include "mesh/MeshLoader.h"
#include "mesh/MeshData.h"
#include <string>
#include <vector>
#include <memory>

#ifdef GISMO_AVAILABLE
// Forward declarations for G+Smo types (avoid including gismo.h in header)
namespace gismo {
    template<class T> class gsMultiPatch;
    template<class T> class gsGeometry;
}
#endif

// Forward declaration for Poisson solution
struct PoissonSolution;

// Result of loading a multipatch - contains mesh data for each patch
struct MultiPatchData {
    std::vector<MeshData> patches;
    std::vector<std::string> patchNames;
    std::string name;

    size_t totalVertices() const {
        size_t total = 0;
        for (const auto& p : patches) total += p.vertices.size();
        return total;
    }

    size_t totalTriangles() const {
        size_t total = 0;
        for (const auto& p : patches) total += p.indices.size() / 3;
        return total;
    }
};

// Loader for G+Smo multipatch XML files
class GismoLoader : public MeshLoader {
public:
    GismoLoader() = default;
    ~GismoLoader() override = default;

    // MeshLoader interface - loads all patches merged into one mesh
    bool load(const std::string& path, MeshData& outData) override;
    bool canLoad(const std::string& extension) const override;

    // Load as separate patches (preferred for view-dependent refinement)
    bool loadMultiPatch(const std::string& path, MultiPatchData& outData, int tessellationLevel = 16);

#ifdef GISMO_AVAILABLE
    // Tessellate a single patch at given resolution
    static MeshData tessellatePatch(const gismo::gsGeometry<double>& patch,
                                     int uSamples, int vSamples);

    // Tessellate a patch with solution values evaluated at each vertex
    static MeshData tessellatePatchWithSolution(const gismo::gsGeometry<double>& patch,
                                                 int uSamples, int vSamples,
                                                 const PoissonSolution* solution,
                                                 int patchIndex);
#endif

    // Set default tessellation level
    void setTessellationLevel(int level) { m_tessellationLevel = level; }
    int getTessellationLevel() const { return m_tessellationLevel; }

private:
    int m_tessellationLevel = 16;  // Default: 16x16 grid per patch
};
