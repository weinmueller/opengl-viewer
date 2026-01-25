#pragma once

#include "mesh/MeshData.h"
#include "async/SubdivisionTask.h"
#include <unordered_map>
#include <vector>
#include <tuple>

class Subdivision {
public:
    // Loop subdivision - smooths and subdivides triangle mesh
    // Each triangle becomes 4 triangles
    // creaseAngleThreshold: edges with dihedral angle > threshold (in degrees) are kept sharp
    // Default 180 degrees refines everything (no crease preservation)
    static MeshData loopSubdivide(const MeshData& input, float creaseAngleThreshold = 180.0f);

    // Simple midpoint subdivision - splits without smoothing
    static MeshData midpointSubdivide(const MeshData& input);

    // Progress-aware versions for background threading
    static MeshData loopSubdivideWithProgress(const MeshData& input, float creaseAngleThreshold,
                                              SubdivisionProgress& progress);
    static MeshData midpointSubdivideWithProgress(const MeshData& input,
                                                   SubdivisionProgress& progress);

    // Weld vertices that share the same position (within epsilon)
    // This is needed before subdivision when meshes have split vertices for per-face normals
    static MeshData weldVertices(const MeshData& input, float epsilon = 1e-6f);

private:
    // Edge key for hash map (ordered pair of vertex indices)
    struct EdgeKey {
        uint32_t v0, v1;

        EdgeKey(uint32_t a, uint32_t b) {
            v0 = std::min(a, b);
            v1 = std::max(a, b);
        }

        bool operator==(const EdgeKey& other) const {
            return v0 == other.v0 && v1 == other.v1;
        }
    };

    struct EdgeKeyHash {
        size_t operator()(const EdgeKey& k) const {
            return std::hash<uint64_t>()(
                (static_cast<uint64_t>(k.v0) << 32) | k.v1
            );
        }
    };

    // Thread-local storage for parallel adjacency building
    struct ThreadLocalAdjacency {
        std::vector<std::pair<uint32_t, uint32_t>> neighborPairs;
        std::vector<std::tuple<EdgeKey, uint32_t>> edgeOppositePairs;
        std::vector<std::pair<EdgeKey, size_t>> edgeFacePairs;

        void reserve(size_t numFaces) {
            neighborPairs.reserve(numFaces * 6);
            edgeOppositePairs.reserve(numFaces * 3);
            edgeFacePairs.reserve(numFaces * 3);
        }

        void clear() {
            neighborPairs.clear();
            edgeOppositePairs.clear();
            edgeFacePairs.clear();
        }
    };

    // Get or create edge midpoint vertex
    static uint32_t getEdgeMidpoint(
        const MeshData& input,
        MeshData& output,
        std::unordered_map<EdgeKey, uint32_t, EdgeKeyHash>& edgeVertices,
        uint32_t v0, uint32_t v1
    );
};
