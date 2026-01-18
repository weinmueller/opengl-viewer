#pragma once

#include "mesh/MeshData.h"
#include <unordered_map>

class Subdivision {
public:
    // Loop subdivision - smooths and subdivides triangle mesh
    // Each triangle becomes 4 triangles
    static MeshData loopSubdivide(const MeshData& input);

    // Simple midpoint subdivision - splits without smoothing
    static MeshData midpointSubdivide(const MeshData& input);

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

    // Get or create edge midpoint vertex
    static uint32_t getEdgeMidpoint(
        const MeshData& input,
        MeshData& output,
        std::unordered_map<EdgeKey, uint32_t, EdgeKeyHash>& edgeVertices,
        uint32_t v0, uint32_t v1
    );
};
