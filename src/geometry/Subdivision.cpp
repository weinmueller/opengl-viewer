#include "Subdivision.h"
#include <cmath>

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

    output.calculateBounds();
    return output;
}

MeshData Subdivision::loopSubdivide(const MeshData& input, float creaseAngleThreshold) {
    // First, weld vertices by position to ensure proper adjacency
    // This handles meshes where vertices are split for per-face normals
    MeshData welded = weldVertices(input);

    MeshData output;

    // Build adjacency information
    // For each vertex, store its neighbors
    std::vector<std::vector<uint32_t>> vertexNeighbors(welded.vertices.size());

    // For each edge, store the opposite vertices of adjacent triangles
    std::unordered_map<EdgeKey, std::vector<uint32_t>, EdgeKeyHash> edgeOpposites;

    // Store face normals for each triangle (indexed by triangle index / 3)
    std::vector<glm::vec3> faceNormals;
    faceNormals.reserve(welded.indices.size() / 3);

    // Map edge to the face indices it belongs to
    std::unordered_map<EdgeKey, std::vector<size_t>, EdgeKeyHash> edgeFaces;

    // Build adjacency from triangles
    for (size_t i = 0; i < welded.indices.size(); i += 3) {
        uint32_t i0 = welded.indices[i];
        uint32_t i1 = welded.indices[i + 1];
        uint32_t i2 = welded.indices[i + 2];

        size_t faceIdx = i / 3;

        // Compute face normal
        glm::vec3 v0 = welded.vertices[i0].position;
        glm::vec3 v1 = welded.vertices[i1].position;
        glm::vec3 v2 = welded.vertices[i2].position;
        glm::vec3 faceNormal = glm::normalize(glm::cross(v1 - v0, v2 - v0));
        faceNormals.push_back(faceNormal);

        // Add neighbors
        auto addNeighbor = [&](uint32_t v, uint32_t n) {
            auto& neighbors = vertexNeighbors[v];
            if (std::find(neighbors.begin(), neighbors.end(), n) == neighbors.end()) {
                neighbors.push_back(n);
            }
        };

        addNeighbor(i0, i1); addNeighbor(i0, i2);
        addNeighbor(i1, i0); addNeighbor(i1, i2);
        addNeighbor(i2, i0); addNeighbor(i2, i1);

        // Store opposite vertices for edges
        edgeOpposites[EdgeKey(i0, i1)].push_back(i2);
        edgeOpposites[EdgeKey(i1, i2)].push_back(i0);
        edgeOpposites[EdgeKey(i2, i0)].push_back(i1);

        // Store face index for each edge
        edgeFaces[EdgeKey(i0, i1)].push_back(faceIdx);
        edgeFaces[EdgeKey(i1, i2)].push_back(faceIdx);
        edgeFaces[EdgeKey(i2, i0)].push_back(faceIdx);
    }

    // Identify sharp/crease edges based on dihedral angle
    std::unordered_map<EdgeKey, bool, EdgeKeyHash> isSharpEdge;
    float cosThreshold = std::cos(creaseAngleThreshold * 3.14159265f / 180.0f);

    for (const auto& [edge, faces] : edgeFaces) {
        if (faces.size() == 1) {
            // Boundary edge - always sharp
            isSharpEdge[edge] = true;
        } else if (faces.size() == 2) {
            // Compute dihedral angle between the two faces
            glm::vec3 n0 = faceNormals[faces[0]];
            glm::vec3 n1 = faceNormals[faces[1]];
            float cosAngle = glm::dot(n0, n1);

            // If angle between normals > threshold, it's a sharp edge
            // cos(angle) < cos(threshold) means angle > threshold
            isSharpEdge[edge] = (cosAngle < cosThreshold);
        }
    }

    // Identify crease/boundary vertices and their crease neighbors
    std::vector<bool> isCreaseVertex(welded.vertices.size(), false);
    std::vector<std::vector<uint32_t>> creaseNeighbors(welded.vertices.size());

    for (const auto& [edge, isSharp] : isSharpEdge) {
        if (isSharp) {
            isCreaseVertex[edge.v0] = true;
            isCreaseVertex[edge.v1] = true;

            // Store crease neighbors
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

    // Step 1: Compute new positions for original vertices
    output.vertices.resize(welded.vertices.size());

    for (size_t i = 0; i < welded.vertices.size(); ++i) {
        const auto& neighbors = vertexNeighbors[i];
        size_t n = neighbors.size();

        if (n == 0) {
            output.vertices[i] = welded.vertices[i];
            continue;
        }

        Vertex& outVert = output.vertices[i];

        if (isCreaseVertex[i]) {
            // Crease/boundary vertex handling
            const auto& cNeighbors = creaseNeighbors[i];

            if (cNeighbors.size() == 2) {
                // Regular crease vertex: 3/4 * v + 1/8 * (c1 + c2)
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
                // Corner vertex (more than 2 crease edges meet here)
                // Keep original position to preserve sharp corners
                outVert = welded.vertices[i];
            }
        } else {
            // Interior vertex - standard Loop subdivision rule
            float beta;
            if (n == 3) {
                beta = 3.0f / 16.0f;
            } else {
                beta = 3.0f / (8.0f * static_cast<float>(n));
            }

            // New position = (1 - n*beta) * v + beta * sum(neighbors)
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

    // Step 2: Create edge vertices with Loop subdivision weights
    std::unordered_map<EdgeKey, uint32_t, EdgeKeyHash> edgeVertexMap;

    auto getLoopEdgeVertex = [&](uint32_t v0, uint32_t v1) -> uint32_t {
        EdgeKey key(v0, v1);

        auto it = edgeVertexMap.find(key);
        if (it != edgeVertexMap.end()) {
            return it->second;
        }

        const Vertex& vert0 = welded.vertices[v0];
        const Vertex& vert1 = welded.vertices[v1];

        Vertex newVert;

        // Check if this is a sharp/crease edge
        auto sharpIt = isSharpEdge.find(key);
        bool edgeIsSharp = (sharpIt != isSharpEdge.end() && sharpIt->second);

        const auto& opposites = edgeOpposites[key];

        if (!edgeIsSharp && opposites.size() == 2) {
            // Interior smooth edge: 3/8 * (v0 + v1) + 1/8 * (opposite0 + opposite1)
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
            // Sharp/crease/boundary edge: simple midpoint to preserve sharpness
            newVert.position = 0.5f * (vert0.position + vert1.position);
            newVert.normal = glm::normalize(0.5f * (vert0.normal + vert1.normal));
            newVert.texCoord = 0.5f * (vert0.texCoord + vert1.texCoord);
        }

        uint32_t newIndex = static_cast<uint32_t>(output.vertices.size());
        output.vertices.push_back(newVert);
        edgeVertexMap[key] = newIndex;

        return newIndex;
    };

    // Step 3: Create new triangles
    for (size_t i = 0; i < welded.indices.size(); i += 3) {
        uint32_t i0 = welded.indices[i];
        uint32_t i1 = welded.indices[i + 1];
        uint32_t i2 = welded.indices[i + 2];

        uint32_t m01 = getLoopEdgeVertex(i0, i1);
        uint32_t m12 = getLoopEdgeVertex(i1, i2);
        uint32_t m20 = getLoopEdgeVertex(i2, i0);

        // 4 new triangles
        output.indices.push_back(i0);  output.indices.push_back(m01); output.indices.push_back(m20);
        output.indices.push_back(m01); output.indices.push_back(i1);  output.indices.push_back(m12);
        output.indices.push_back(m20); output.indices.push_back(m12); output.indices.push_back(i2);
        output.indices.push_back(m01); output.indices.push_back(m12); output.indices.push_back(m20);
    }

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

        PositionKey key{
            static_cast<int>(std::round(pos.x * invEpsilon)),
            static_cast<int>(std::round(pos.y * invEpsilon)),
            static_cast<int>(std::round(pos.z * invEpsilon))
        };

        auto it = positionMap.find(key);
        if (it != positionMap.end()) {
            // Vertex at this position already exists, map to it
            vertexRemap[i] = it->second;
        } else {
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
