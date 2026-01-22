#pragma once

#include "mesh/MeshData.h"
#include "mesh/Mesh.h"
#include <memory>

// Represents a single LOD level
struct LODLevel {
    MeshData meshData;                    // CPU-side mesh data
    std::shared_ptr<Mesh> gpuMesh;        // Lazy GPU upload
    float screenSizeThreshold{0.0f};      // Min screen pixels for this LOD
    uint32_t triangleCount{0};            // Number of triangles in this LOD

    LODLevel() = default;

    LODLevel(MeshData&& data, float threshold)
        : meshData(std::move(data))
        , screenSizeThreshold(threshold)
        , triangleCount(static_cast<uint32_t>(meshData.indices.size() / 3))
    {
    }

    // Ensure GPU mesh is uploaded (call from main thread only)
    void ensureGPUMesh() {
        if (!gpuMesh && !meshData.empty()) {
            gpuMesh = std::make_shared<Mesh>();
            gpuMesh->upload(meshData);
        }
    }

    Mesh* getMesh() {
        ensureGPUMesh();
        return gpuMesh.get();
    }

    bool isValid() const {
        return !meshData.empty() || (gpuMesh && gpuMesh->isValid());
    }
};
