#include "Subdivision.h"
#include <cmath>
#include <omp.h>
#include <algorithm>

uint32_t Subdivision::getEdgeMidpoint(
    const MeshData& input,
    MeshData& output,
    std::unordered_map<EdgeKey, uint32_t, EdgeKeyHash>& edgeVertices,
    uint32_t v0, uint32_t v1)
{
    EdgeKey key(v0, v1);

    auto it = edgeVertices.find(key);
    if (it != edgeVertices.end()) {
        return it->second;
    }

    // Create new vertex at midpoint
    const Vertex& vert0 = input.vertices[v0];
    const Vertex& vert1 = input.vertices[v1];

    Vertex newVert;
    newVert.position = (vert0.position + vert1.position) * 0.5f;
    newVert.normal = glm::normalize((vert0.normal + vert1.normal) * 0.5f);
    newVert.texCoord = (vert0.texCoord + vert1.texCoord) * 0.5f;

    uint32_t newIndex = static_cast<uint32_t>(output.vertices.size());
    output.vertices.push_back(newVert);
    edgeVertices[key] = newIndex;

    return newIndex;
}

MeshData Subdivision::midpointSubdivide(const MeshData& input) {
    MeshData output;

    // Copy original vertices
    output.vertices = input.vertices;

    // Map to track edge midpoints
    std::unordered_map<EdgeKey, uint32_t, EdgeKeyHash> edgeVertices;

    // Process each triangle
    for (size_t i = 0; i < input.indices.size(); i += 3) {
        uint32_t i0 = input.indices[i];
        uint32_t i1 = input.indices[i + 1];
        uint32_t i2 = input.indices[i + 2];

        // Get midpoint vertices for each edge
        uint32_t m01 = getEdgeMidpoint(input, output, edgeVertices, i0, i1);
        uint32_t m12 = getEdgeMidpoint(input, output, edgeVertices, i1, i2);
        uint32_t m20 = getEdgeMidpoint(input, output, edgeVertices, i2, i0);

        // Create 4 new triangles
        // Triangle 0: corner 0
        output.indices.push_back(i0);
        output.indices.push_back(m01);
        output.indices.push_back(m20);

        // Triangle 1: corner 1
        output.indices.push_back(m01);
        output.indices.push_back(i1);
        output.indices.push_back(m12);

        // Triangle 2: corner 2
        output.indices.push_back(m20);
        output.indices.push_back(m12);
        output.indices.push_back(i2);

        // Triangle 3: center
        output.indices.push_back(m01);
        output.indices.push_back(m12);
        output.indices.push_back(m20);
    }

    // Recalculate normals from actual geometry for correct lighting
    output.recalculateNormals();
    output.calculateBounds();
    return output;
}

MeshData Subdivision::loopSubdivide(const MeshData& input, float creaseAngleThreshold) {
    // First, weld vertices by position to ensure proper adjacency
    MeshData welded = weldVertices(input);

    const size_t numVertices = welded.vertices.size();
    const size_t numFaces = welded.indices.size() / 3;
    const float cosThreshold = std::cos(creaseAngleThreshold * 3.14159265f / 180.0f);

    MeshData output;

    // ========== PHASE 1: Parallel face normal computation ==========
    std::vector<glm::vec3> faceNormals(numFaces);

    #pragma omp parallel for schedule(static)
    for (size_t f = 0; f < numFaces; ++f) {
        size_t i = f * 3;
        uint32_t i0 = welded.indices[i];
        uint32_t i1 = welded.indices[i + 1];
        uint32_t i2 = welded.indices[i + 2];

        glm::vec3 v0 = welded.vertices[i0].position;
        glm::vec3 v1 = welded.vertices[i1].position;
        glm::vec3 v2 = welded.vertices[i2].position;
        faceNormals[f] = glm::normalize(glm::cross(v1 - v0, v2 - v0));
    }

    // ========== PHASE 2: Parallel adjacency collection ==========
    int numThreads = omp_get_max_threads();
    std::vector<ThreadLocalAdjacency> threadLocalData(numThreads);

    // Pre-allocate thread-local storage
    size_t facesPerThread = (numFaces + numThreads - 1) / numThreads;
    for (auto& tld : threadLocalData) {
        tld.reserve(facesPerThread);
    }

    #pragma omp parallel
    {
        int tid = omp_get_thread_num();
        ThreadLocalAdjacency& local = threadLocalData[tid];

        #pragma omp for schedule(static)
        for (size_t f = 0; f < numFaces; ++f) {
            size_t i = f * 3;
            uint32_t i0 = welded.indices[i];
            uint32_t i1 = welded.indices[i + 1];
            uint32_t i2 = welded.indices[i + 2];

            // Neighbor pairs (vertex, neighbor)
            local.neighborPairs.emplace_back(i0, i1);
            local.neighborPairs.emplace_back(i0, i2);
            local.neighborPairs.emplace_back(i1, i0);
            local.neighborPairs.emplace_back(i1, i2);
            local.neighborPairs.emplace_back(i2, i0);
            local.neighborPairs.emplace_back(i2, i1);

            // Edge opposite pairs (edge, opposite vertex)
            local.edgeOppositePairs.emplace_back(EdgeKey(i0, i1), i2);
            local.edgeOppositePairs.emplace_back(EdgeKey(i1, i2), i0);
            local.edgeOppositePairs.emplace_back(EdgeKey(i2, i0), i1);

            // Edge face pairs (edge, face index)
            local.edgeFacePairs.emplace_back(EdgeKey(i0, i1), f);
            local.edgeFacePairs.emplace_back(EdgeKey(i1, i2), f);
            local.edgeFacePairs.emplace_back(EdgeKey(i2, i0), f);
        }
    }

    // ========== PHASE 3: Merge thread-local data into global structures ==========
    std::vector<std::vector<uint32_t>> vertexNeighbors(numVertices);
    std::unordered_map<EdgeKey, std::vector<uint32_t>, EdgeKeyHash> edgeOpposites;
    std::unordered_map<EdgeKey, std::vector<size_t>, EdgeKeyHash> edgeFaces;

    // Reserve space in maps based on expected edges (~1.5 * numFaces for manifold mesh)
    edgeOpposites.reserve(numFaces * 2);
    edgeFaces.reserve(numFaces * 2);

    // Merge neighbor pairs
    for (const auto& tld : threadLocalData) {
        for (const auto& [vertex, neighbor] : tld.neighborPairs) {
            auto& neighbors = vertexNeighbors[vertex];
            if (std::find(neighbors.begin(), neighbors.end(), neighbor) == neighbors.end()) {
                neighbors.push_back(neighbor);
            }
        }
    }

    // Merge edge opposite pairs
    for (const auto& tld : threadLocalData) {
        for (const auto& [edge, opposite] : tld.edgeOppositePairs) {
            edgeOpposites[edge].push_back(opposite);
        }
    }

    // Merge edge face pairs
    for (const auto& tld : threadLocalData) {
        for (const auto& [edge, faceIdx] : tld.edgeFacePairs) {
            edgeFaces[edge].push_back(faceIdx);
        }
    }

    // Clear thread-local data to free memory
    threadLocalData.clear();
    threadLocalData.shrink_to_fit();

    // ========== PHASE 4: Build unique edge list for parallel processing ==========
    std::vector<EdgeKey> uniqueEdges;
    uniqueEdges.reserve(edgeFaces.size());
    for (const auto& [edge, faces] : edgeFaces) {
        uniqueEdges.push_back(edge);
    }

    // ========== PHASE 5: Sharp edge detection (sequential for map safety) ==========
    std::vector<bool> edgeIsSharp(uniqueEdges.size(), false);

    for (size_t e = 0; e < uniqueEdges.size(); ++e) {
        const EdgeKey& edge = uniqueEdges[e];
        const auto& faces = edgeFaces.at(edge);

        if (faces.size() == 1) {
            // Boundary edge - always sharp
            edgeIsSharp[e] = true;
        } else if (faces.size() == 2) {
            // Compute dihedral angle
            glm::vec3 n0 = faceNormals[faces[0]];
            glm::vec3 n1 = faceNormals[faces[1]];
            float cosAngle = glm::dot(n0, n1);
            edgeIsSharp[e] = (cosAngle < cosThreshold);
        }
    }

    // Build map for fast sharp edge lookup
    std::unordered_map<EdgeKey, bool, EdgeKeyHash> isSharpEdge;
    isSharpEdge.reserve(uniqueEdges.size());
    for (size_t e = 0; e < uniqueEdges.size(); ++e) {
        isSharpEdge[uniqueEdges[e]] = edgeIsSharp[e];
    }

    // Identify crease vertices and their crease neighbors
    std::vector<bool> isCreaseVertex(numVertices, false);
    std::vector<std::vector<uint32_t>> creaseNeighbors(numVertices);

    for (size_t e = 0; e < uniqueEdges.size(); ++e) {
        if (edgeIsSharp[e]) {
            const EdgeKey& edge = uniqueEdges[e];
            isCreaseVertex[edge.v0] = true;
            isCreaseVertex[edge.v1] = true;

            auto addCreaseNeighbor = [&](uint32_t v, uint32_t n) {
                auto& neighbors = creaseNeighbors[v];
                if (std::find(neighbors.begin(), neighbors.end(), n) == neighbors.end()) {
                    neighbors.push_back(n);
                }
            };
            addCreaseNeighbor(edge.v0, edge.v1);
            addCreaseNeighbor(edge.v1, edge.v0);
        }
    }

    // ========== PHASE 6: Parallel vertex repositioning ==========
    output.vertices.resize(numVertices);

    #pragma omp parallel for schedule(static)
    for (size_t i = 0; i < numVertices; ++i) {
        const auto& neighbors = vertexNeighbors[i];
        size_t n = neighbors.size();

        if (n == 0) {
            output.vertices[i] = welded.vertices[i];
            continue;
        }

        Vertex& outVert = output.vertices[i];

        if (isCreaseVertex[i]) {
            const auto& cNeighbors = creaseNeighbors[i];

            if (cNeighbors.size() == 2) {
                const Vertex& c1 = welded.vertices[cNeighbors[0]];
                const Vertex& c2 = welded.vertices[cNeighbors[1]];

                outVert.position = 0.75f * welded.vertices[i].position
                                 + 0.125f * (c1.position + c2.position);
                outVert.normal = glm::normalize(
                    0.75f * welded.vertices[i].normal
                    + 0.125f * (c1.normal + c2.normal)
                );
                outVert.texCoord = 0.75f * welded.vertices[i].texCoord
                                 + 0.125f * (c1.texCoord + c2.texCoord);
            } else {
                outVert = welded.vertices[i];
            }
        } else {
            float beta;
            if (n == 3) {
                beta = 3.0f / 16.0f;
            } else {
                beta = 3.0f / (8.0f * static_cast<float>(n));
            }

            glm::vec3 neighborSum(0.0f);
            glm::vec3 normalSum(0.0f);
            glm::vec2 texSum(0.0f);

            for (uint32_t ni : neighbors) {
                neighborSum += welded.vertices[ni].position;
                normalSum += welded.vertices[ni].normal;
                texSum += welded.vertices[ni].texCoord;
            }

            outVert.position = (1.0f - static_cast<float>(n) * beta) * welded.vertices[i].position
                             + beta * neighborSum;
            outVert.normal = glm::normalize(
                (1.0f - static_cast<float>(n) * beta) * welded.vertices[i].normal
                + beta * normalSum
            );
            outVert.texCoord = (1.0f - static_cast<float>(n) * beta) * welded.vertices[i].texCoord
                             + beta * texSum;
        }
    }

    // ========== PHASE 7: Parallel edge vertex creation ==========
    // Pre-allocate space for edge vertices (one per unique edge)
    size_t numEdges = uniqueEdges.size();
    size_t edgeVertexStartIndex = output.vertices.size();
    output.vertices.resize(edgeVertexStartIndex + numEdges);

    // Create map from edge to its vertex index
    std::unordered_map<EdgeKey, uint32_t, EdgeKeyHash> edgeVertexMap;
    edgeVertexMap.reserve(numEdges);
    for (size_t e = 0; e < numEdges; ++e) {
        edgeVertexMap[uniqueEdges[e]] = static_cast<uint32_t>(edgeVertexStartIndex + e);
    }

    // Pre-extract data for parallel-safe access
    std::vector<std::vector<uint32_t>> edgeOppositesVec(numEdges);
    for (size_t e = 0; e < numEdges; ++e) {
        edgeOppositesVec[e] = edgeOpposites.at(uniqueEdges[e]);
    }

    #pragma omp parallel for schedule(static)
    for (size_t e = 0; e < numEdges; ++e) {
        const EdgeKey& edge = uniqueEdges[e];
        const Vertex& vert0 = welded.vertices[edge.v0];
        const Vertex& vert1 = welded.vertices[edge.v1];

        Vertex newVert;

        bool sharp = edgeIsSharp[e];
        const auto& opposites = edgeOppositesVec[e];

        if (!sharp && opposites.size() == 2) {
            const Vertex& opp0 = welded.vertices[opposites[0]];
            const Vertex& opp1 = welded.vertices[opposites[1]];

            newVert.position = 0.375f * (vert0.position + vert1.position)
                             + 0.125f * (opp0.position + opp1.position);
            newVert.normal = glm::normalize(
                0.375f * (vert0.normal + vert1.normal)
                + 0.125f * (opp0.normal + opp1.normal)
            );
            newVert.texCoord = 0.375f * (vert0.texCoord + vert1.texCoord)
                             + 0.125f * (opp0.texCoord + opp1.texCoord);
        } else {
            newVert.position = 0.5f * (vert0.position + vert1.position);
            newVert.normal = glm::normalize(0.5f * (vert0.normal + vert1.normal));
            newVert.texCoord = 0.5f * (vert0.texCoord + vert1.texCoord);
        }

        output.vertices[edgeVertexStartIndex + e] = newVert;
    }

    // ========== PHASE 8: Parallel triangle generation ==========
    // Pre-compute edge midpoint indices for each face (for parallel-safe access)
    struct FaceEdges {
        uint32_t m01, m12, m20;
    };
    std::vector<FaceEdges> faceEdgeVertices(numFaces);

    for (size_t f = 0; f < numFaces; ++f) {
        size_t i = f * 3;
        uint32_t i0 = welded.indices[i];
        uint32_t i1 = welded.indices[i + 1];
        uint32_t i2 = welded.indices[i + 2];

        faceEdgeVertices[f].m01 = edgeVertexMap.at(EdgeKey(i0, i1));
        faceEdgeVertices[f].m12 = edgeVertexMap.at(EdgeKey(i1, i2));
        faceEdgeVertices[f].m20 = edgeVertexMap.at(EdgeKey(i2, i0));
    }

    // Each original triangle produces 4 new triangles (12 indices)
    output.indices.resize(numFaces * 12);

    #pragma omp parallel for schedule(static)
    for (size_t f = 0; f < numFaces; ++f) {
        size_t i = f * 3;
        uint32_t i0 = welded.indices[i];
        uint32_t i1 = welded.indices[i + 1];
        uint32_t i2 = welded.indices[i + 2];

        uint32_t m01 = faceEdgeVertices[f].m01;
        uint32_t m12 = faceEdgeVertices[f].m12;
        uint32_t m20 = faceEdgeVertices[f].m20;

        size_t outIdx = f * 12;

        // Triangle 0: corner 0
        output.indices[outIdx + 0] = i0;
        output.indices[outIdx + 1] = m01;
        output.indices[outIdx + 2] = m20;

        // Triangle 1: corner 1
        output.indices[outIdx + 3] = m01;
        output.indices[outIdx + 4] = i1;
        output.indices[outIdx + 5] = m12;

        // Triangle 2: corner 2
        output.indices[outIdx + 6] = m20;
        output.indices[outIdx + 7] = m12;
        output.indices[outIdx + 8] = i2;

        // Triangle 3: center
        output.indices[outIdx + 9] = m01;
        output.indices[outIdx + 10] = m12;
        output.indices[outIdx + 11] = m20;
    }

    // Recalculate normals from actual geometry for correct lighting
    output.recalculateNormals();
    output.calculateBounds();
    return output;
}

MeshData Subdivision::weldVertices(const MeshData& input, float epsilon) {
    MeshData output;

    // Map from old vertex index to new (welded) vertex index
    std::vector<uint32_t> vertexRemap(input.vertices.size());

    // For each unique position, store the first vertex index that had it
    struct PositionKey {
        int x, y, z;

        bool operator==(const PositionKey& other) const {
            return x == other.x && y == other.y && z == other.z;
        }
    };

    struct PositionHash {
        size_t operator()(const PositionKey& k) const {
            return std::hash<int>()(k.x) ^
                   (std::hash<int>()(k.y) << 1) ^
                   (std::hash<int>()(k.z) << 2);
        }
    };

    // Quantize positions to grid cells for fast lookup
    float invEpsilon = 1.0f / epsilon;
    std::unordered_map<PositionKey, uint32_t, PositionHash> positionMap;

    for (size_t i = 0; i < input.vertices.size(); ++i) {
        const glm::vec3& pos = input.vertices[i].position;

        float fx = pos.x * invEpsilon;
        float fy = pos.y * invEpsilon;
        float fz = pos.z * invEpsilon;

        PositionKey key{
            static_cast<int>(std::round(fx)),
            static_cast<int>(std::round(fy)),
            static_cast<int>(std::round(fz))
        };

        // Check primary cell first
        auto it = positionMap.find(key);
        if (it != positionMap.end()) {
            vertexRemap[i] = it->second;
            continue;
        }

        // Check neighboring cells for vertices near grid boundaries.
        // A vertex near a cell boundary (fractional part close to 0.5) may have
        // been quantized to an adjacent cell by a nearby vertex.
        bool found = false;
        int altX[] = { key.x, key.x };
        int altY[] = { key.y, key.y };
        int altZ[] = { key.z, key.z };
        float fracX = fx - std::floor(fx);
        float fracY = fy - std::floor(fy);
        float fracZ = fz - std::floor(fz);
        // If near the 0.5 boundary, also check the other side
        if (fracX > 0.4f && fracX < 0.6f) { altX[1] = (fracX >= 0.5f) ? key.x - 1 : key.x + 1; }
        if (fracY > 0.4f && fracY < 0.6f) { altY[1] = (fracY >= 0.5f) ? key.y - 1 : key.y + 1; }
        if (fracZ > 0.4f && fracZ < 0.6f) { altZ[1] = (fracZ >= 0.5f) ? key.z - 1 : key.z + 1; }

        for (int xi = 0; xi < 2 && !found; ++xi) {
            for (int yi = 0; yi < 2 && !found; ++yi) {
                for (int zi = 0; zi < 2 && !found; ++zi) {
                    if (xi == 0 && yi == 0 && zi == 0) continue; // Already checked primary
                    PositionKey altKey{ altX[xi], altY[yi], altZ[zi] };
                    auto ait = positionMap.find(altKey);
                    if (ait != positionMap.end()) {
                        vertexRemap[i] = ait->second;
                        found = true;
                    }
                }
            }
        }

        if (!found) {
            // New unique position
            uint32_t newIndex = static_cast<uint32_t>(output.vertices.size());
            output.vertices.push_back(input.vertices[i]);
            positionMap[key] = newIndex;
            vertexRemap[i] = newIndex;
        }
    }

    // Remap all indices
    output.indices.reserve(input.indices.size());
    for (uint32_t idx : input.indices) {
        output.indices.push_back(vertexRemap[idx]);
    }

    output.calculateBounds();
    return output;
}

// Progress-aware midpoint subdivision
MeshData Subdivision::midpointSubdivideWithProgress(const MeshData& input,
                                                     SubdivisionProgress& progress) {
    MeshData output;

    // Midpoint subdivision is simpler - use direct progress updates
    // Phase 7: "Creating edge vertices" (main work)
    progress.phase.store(7, std::memory_order_relaxed);
    progress.totalProgress.store(0.0f, std::memory_order_relaxed);
    if (progress.isCancelled()) return output;

    // Copy original vertices (small portion of work)
    output.vertices = input.vertices;
    progress.totalProgress.store(0.05f, std::memory_order_relaxed);

    std::unordered_map<EdgeKey, uint32_t, EdgeKeyHash> edgeVertices;
    size_t numFaces = input.indices.size() / 3;

    for (size_t f = 0; f < numFaces; ++f) {
        if (progress.isCancelled()) return output;

        size_t i = f * 3;
        uint32_t i0 = input.indices[i];
        uint32_t i1 = input.indices[i + 1];
        uint32_t i2 = input.indices[i + 2];

        // Get midpoint vertices for each edge
        uint32_t m01 = getEdgeMidpoint(input, output, edgeVertices, i0, i1);
        uint32_t m12 = getEdgeMidpoint(input, output, edgeVertices, i1, i2);
        uint32_t m20 = getEdgeMidpoint(input, output, edgeVertices, i2, i0);

        // Create 4 new triangles
        output.indices.push_back(i0);
        output.indices.push_back(m01);
        output.indices.push_back(m20);

        output.indices.push_back(m01);
        output.indices.push_back(i1);
        output.indices.push_back(m12);

        output.indices.push_back(m20);
        output.indices.push_back(m12);
        output.indices.push_back(i2);

        output.indices.push_back(m01);
        output.indices.push_back(m12);
        output.indices.push_back(m20);

        // Update progress periodically (5% to 95% range for main loop)
        if (f % 500 == 0 || f == numFaces - 1) {
            float loopProgress = static_cast<float>(f + 1) / numFaces;
            progress.totalProgress.store(0.05f + loopProgress * 0.90f, std::memory_order_relaxed);
        }
    }

    // Finalize
    progress.phase.store(8, std::memory_order_relaxed);
    progress.totalProgress.store(0.98f, std::memory_order_relaxed);

    // Recalculate normals from actual geometry for correct lighting
    output.recalculateNormals();
    output.calculateBounds();

    progress.totalProgress.store(1.0f, std::memory_order_relaxed);
    return output;
}

// Progress-aware Loop subdivision
MeshData Subdivision::loopSubdivideWithProgress(const MeshData& input, float creaseAngleThreshold,
                                                 SubdivisionProgress& progress) {
    // Phase 1: Weld vertices and compute face normals
    progress.setPhase(1);
    if (progress.isCancelled()) return MeshData();

    MeshData welded = weldVertices(input);

    const size_t numVertices = welded.vertices.size();
    const size_t numFaces = welded.indices.size() / 3;
    const float cosThreshold = std::cos(creaseAngleThreshold * 3.14159265f / 180.0f);

    MeshData output;

    // Compute face normals
    std::vector<glm::vec3> faceNormals(numFaces);

    #pragma omp parallel for schedule(static)
    for (size_t f = 0; f < numFaces; ++f) {
        size_t i = f * 3;
        uint32_t i0 = welded.indices[i];
        uint32_t i1 = welded.indices[i + 1];
        uint32_t i2 = welded.indices[i + 2];

        glm::vec3 v0 = welded.vertices[i0].position;
        glm::vec3 v1 = welded.vertices[i1].position;
        glm::vec3 v2 = welded.vertices[i2].position;
        faceNormals[f] = glm::normalize(glm::cross(v1 - v0, v2 - v0));
    }
    progress.updatePhaseProgress(1.0f);

    // Phase 2: Build adjacency
    progress.setPhase(2);
    if (progress.isCancelled()) return MeshData();

    int numThreads = omp_get_max_threads();
    std::vector<ThreadLocalAdjacency> threadLocalData(numThreads);

    size_t facesPerThread = (numFaces + numThreads - 1) / numThreads;
    for (auto& tld : threadLocalData) {
        tld.reserve(facesPerThread);
    }

    #pragma omp parallel
    {
        int tid = omp_get_thread_num();
        ThreadLocalAdjacency& local = threadLocalData[tid];

        #pragma omp for schedule(static)
        for (size_t f = 0; f < numFaces; ++f) {
            size_t i = f * 3;
            uint32_t i0 = welded.indices[i];
            uint32_t i1 = welded.indices[i + 1];
            uint32_t i2 = welded.indices[i + 2];

            local.neighborPairs.emplace_back(i0, i1);
            local.neighborPairs.emplace_back(i0, i2);
            local.neighborPairs.emplace_back(i1, i0);
            local.neighborPairs.emplace_back(i1, i2);
            local.neighborPairs.emplace_back(i2, i0);
            local.neighborPairs.emplace_back(i2, i1);

            local.edgeOppositePairs.emplace_back(EdgeKey(i0, i1), i2);
            local.edgeOppositePairs.emplace_back(EdgeKey(i1, i2), i0);
            local.edgeOppositePairs.emplace_back(EdgeKey(i2, i0), i1);

            local.edgeFacePairs.emplace_back(EdgeKey(i0, i1), f);
            local.edgeFacePairs.emplace_back(EdgeKey(i1, i2), f);
            local.edgeFacePairs.emplace_back(EdgeKey(i2, i0), f);
        }
    }
    progress.updatePhaseProgress(1.0f);

    // Phase 3: Merge thread-local data
    progress.setPhase(3);
    if (progress.isCancelled()) return MeshData();

    std::vector<std::vector<uint32_t>> vertexNeighbors(numVertices);
    std::unordered_map<EdgeKey, std::vector<uint32_t>, EdgeKeyHash> edgeOpposites;
    std::unordered_map<EdgeKey, std::vector<size_t>, EdgeKeyHash> edgeFaces;

    edgeOpposites.reserve(numFaces * 2);
    edgeFaces.reserve(numFaces * 2);

    for (const auto& tld : threadLocalData) {
        for (const auto& [vertex, neighbor] : tld.neighborPairs) {
            auto& neighbors = vertexNeighbors[vertex];
            if (std::find(neighbors.begin(), neighbors.end(), neighbor) == neighbors.end()) {
                neighbors.push_back(neighbor);
            }
        }
    }

    for (const auto& tld : threadLocalData) {
        for (const auto& [edge, opposite] : tld.edgeOppositePairs) {
            edgeOpposites[edge].push_back(opposite);
        }
    }

    for (const auto& tld : threadLocalData) {
        for (const auto& [edge, faceIdx] : tld.edgeFacePairs) {
            edgeFaces[edge].push_back(faceIdx);
        }
    }

    threadLocalData.clear();
    threadLocalData.shrink_to_fit();
    progress.updatePhaseProgress(1.0f);

    // Phase 4: Build unique edge list
    progress.setPhase(4);
    if (progress.isCancelled()) return MeshData();

    std::vector<EdgeKey> uniqueEdges;
    uniqueEdges.reserve(edgeFaces.size());
    for (const auto& [edge, faces] : edgeFaces) {
        uniqueEdges.push_back(edge);
    }
    progress.updatePhaseProgress(1.0f);

    // Phase 5: Sharp edge detection
    progress.setPhase(5);
    if (progress.isCancelled()) return MeshData();

    std::vector<bool> edgeIsSharp(uniqueEdges.size(), false);

    for (size_t e = 0; e < uniqueEdges.size(); ++e) {
        const EdgeKey& edge = uniqueEdges[e];
        const auto& faces = edgeFaces.at(edge);

        if (faces.size() == 1) {
            edgeIsSharp[e] = true;
        } else if (faces.size() == 2) {
            glm::vec3 n0 = faceNormals[faces[0]];
            glm::vec3 n1 = faceNormals[faces[1]];
            float cosAngle = glm::dot(n0, n1);
            edgeIsSharp[e] = (cosAngle < cosThreshold);
        }

        if (e % 1000 == 0) {
            progress.updatePhaseProgress(static_cast<float>(e) / uniqueEdges.size());
        }
    }

    std::unordered_map<EdgeKey, bool, EdgeKeyHash> isSharpEdge;
    isSharpEdge.reserve(uniqueEdges.size());
    for (size_t e = 0; e < uniqueEdges.size(); ++e) {
        isSharpEdge[uniqueEdges[e]] = edgeIsSharp[e];
    }

    std::vector<bool> isCreaseVertex(numVertices, false);
    std::vector<std::vector<uint32_t>> creaseNeighbors(numVertices);

    for (size_t e = 0; e < uniqueEdges.size(); ++e) {
        if (edgeIsSharp[e]) {
            const EdgeKey& edge = uniqueEdges[e];
            isCreaseVertex[edge.v0] = true;
            isCreaseVertex[edge.v1] = true;

            auto addCreaseNeighbor = [&](uint32_t v, uint32_t n) {
                auto& neighbors = creaseNeighbors[v];
                if (std::find(neighbors.begin(), neighbors.end(), n) == neighbors.end()) {
                    neighbors.push_back(n);
                }
            };
            addCreaseNeighbor(edge.v0, edge.v1);
            addCreaseNeighbor(edge.v1, edge.v0);
        }
    }
    progress.updatePhaseProgress(1.0f);

    // Phase 6: Vertex repositioning
    progress.setPhase(6);
    if (progress.isCancelled()) return MeshData();

    output.vertices.resize(numVertices);

    #pragma omp parallel for schedule(static)
    for (size_t i = 0; i < numVertices; ++i) {
        const auto& neighbors = vertexNeighbors[i];
        size_t n = neighbors.size();

        if (n == 0) {
            output.vertices[i] = welded.vertices[i];
            continue;
        }

        Vertex& outVert = output.vertices[i];

        if (isCreaseVertex[i]) {
            const auto& cNeighbors = creaseNeighbors[i];

            if (cNeighbors.size() == 2) {
                const Vertex& c1 = welded.vertices[cNeighbors[0]];
                const Vertex& c2 = welded.vertices[cNeighbors[1]];

                outVert.position = 0.75f * welded.vertices[i].position
                                 + 0.125f * (c1.position + c2.position);
                outVert.normal = glm::normalize(
                    0.75f * welded.vertices[i].normal
                    + 0.125f * (c1.normal + c2.normal)
                );
                outVert.texCoord = 0.75f * welded.vertices[i].texCoord
                                 + 0.125f * (c1.texCoord + c2.texCoord);
            } else {
                outVert = welded.vertices[i];
            }
        } else {
            float beta;
            if (n == 3) {
                beta = 3.0f / 16.0f;
            } else {
                beta = 3.0f / (8.0f * static_cast<float>(n));
            }

            glm::vec3 neighborSum(0.0f);
            glm::vec3 normalSum(0.0f);
            glm::vec2 texSum(0.0f);

            for (uint32_t ni : neighbors) {
                neighborSum += welded.vertices[ni].position;
                normalSum += welded.vertices[ni].normal;
                texSum += welded.vertices[ni].texCoord;
            }

            outVert.position = (1.0f - static_cast<float>(n) * beta) * welded.vertices[i].position
                             + beta * neighborSum;
            outVert.normal = glm::normalize(
                (1.0f - static_cast<float>(n) * beta) * welded.vertices[i].normal
                + beta * normalSum
            );
            outVert.texCoord = (1.0f - static_cast<float>(n) * beta) * welded.vertices[i].texCoord
                             + beta * texSum;
        }
    }
    progress.updatePhaseProgress(1.0f);

    // Phase 7: Edge vertex creation
    progress.setPhase(7);
    if (progress.isCancelled()) return MeshData();

    size_t numEdges = uniqueEdges.size();
    size_t edgeVertexStartIndex = output.vertices.size();
    output.vertices.resize(edgeVertexStartIndex + numEdges);

    std::unordered_map<EdgeKey, uint32_t, EdgeKeyHash> edgeVertexMap;
    edgeVertexMap.reserve(numEdges);
    for (size_t e = 0; e < numEdges; ++e) {
        edgeVertexMap[uniqueEdges[e]] = static_cast<uint32_t>(edgeVertexStartIndex + e);
    }

    std::vector<std::vector<uint32_t>> edgeOppositesVec(numEdges);
    for (size_t e = 0; e < numEdges; ++e) {
        edgeOppositesVec[e] = edgeOpposites.at(uniqueEdges[e]);
    }

    #pragma omp parallel for schedule(static)
    for (size_t e = 0; e < numEdges; ++e) {
        const EdgeKey& edge = uniqueEdges[e];
        const Vertex& vert0 = welded.vertices[edge.v0];
        const Vertex& vert1 = welded.vertices[edge.v1];

        Vertex newVert;

        bool sharp = edgeIsSharp[e];
        const auto& opposites = edgeOppositesVec[e];

        if (!sharp && opposites.size() == 2) {
            const Vertex& opp0 = welded.vertices[opposites[0]];
            const Vertex& opp1 = welded.vertices[opposites[1]];

            newVert.position = 0.375f * (vert0.position + vert1.position)
                             + 0.125f * (opp0.position + opp1.position);
            newVert.normal = glm::normalize(
                0.375f * (vert0.normal + vert1.normal)
                + 0.125f * (opp0.normal + opp1.normal)
            );
            newVert.texCoord = 0.375f * (vert0.texCoord + vert1.texCoord)
                             + 0.125f * (opp0.texCoord + opp1.texCoord);
        } else {
            newVert.position = 0.5f * (vert0.position + vert1.position);
            newVert.normal = glm::normalize(0.5f * (vert0.normal + vert1.normal));
            newVert.texCoord = 0.5f * (vert0.texCoord + vert1.texCoord);
        }

        output.vertices[edgeVertexStartIndex + e] = newVert;
    }
    progress.updatePhaseProgress(1.0f);

    // Phase 8: Triangle generation
    progress.setPhase(8);
    if (progress.isCancelled()) return MeshData();

    struct FaceEdges {
        uint32_t m01, m12, m20;
    };
    std::vector<FaceEdges> faceEdgeVertices(numFaces);

    for (size_t f = 0; f < numFaces; ++f) {
        size_t i = f * 3;
        uint32_t i0 = welded.indices[i];
        uint32_t i1 = welded.indices[i + 1];
        uint32_t i2 = welded.indices[i + 2];

        faceEdgeVertices[f].m01 = edgeVertexMap.at(EdgeKey(i0, i1));
        faceEdgeVertices[f].m12 = edgeVertexMap.at(EdgeKey(i1, i2));
        faceEdgeVertices[f].m20 = edgeVertexMap.at(EdgeKey(i2, i0));
    }

    output.indices.resize(numFaces * 12);

    #pragma omp parallel for schedule(static)
    for (size_t f = 0; f < numFaces; ++f) {
        size_t i = f * 3;
        uint32_t i0 = welded.indices[i];
        uint32_t i1 = welded.indices[i + 1];
        uint32_t i2 = welded.indices[i + 2];

        uint32_t m01 = faceEdgeVertices[f].m01;
        uint32_t m12 = faceEdgeVertices[f].m12;
        uint32_t m20 = faceEdgeVertices[f].m20;

        size_t outIdx = f * 12;

        output.indices[outIdx + 0] = i0;
        output.indices[outIdx + 1] = m01;
        output.indices[outIdx + 2] = m20;

        output.indices[outIdx + 3] = m01;
        output.indices[outIdx + 4] = i1;
        output.indices[outIdx + 5] = m12;

        output.indices[outIdx + 6] = m20;
        output.indices[outIdx + 7] = m12;
        output.indices[outIdx + 8] = i2;

        output.indices[outIdx + 9] = m01;
        output.indices[outIdx + 10] = m12;
        output.indices[outIdx + 11] = m20;
    }
    progress.updatePhaseProgress(1.0f);

    // Recalculate normals from actual geometry for correct lighting
    output.recalculateNormals();
    output.calculateBounds();
    return output;
}
