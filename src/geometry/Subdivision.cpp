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

MeshData Subdivision::loopSubdivide(const MeshData& input) {
    MeshData output;

    // Build adjacency information
    // For each vertex, store its neighbors
    std::vector<std::vector<uint32_t>> vertexNeighbors(input.vertices.size());

    // For each edge, store the opposite vertices of adjacent triangles
    std::unordered_map<EdgeKey, std::vector<uint32_t>, EdgeKeyHash> edgeOpposites;

    // Build adjacency from triangles
    for (size_t i = 0; i < input.indices.size(); i += 3) {
        uint32_t i0 = input.indices[i];
        uint32_t i1 = input.indices[i + 1];
        uint32_t i2 = input.indices[i + 2];

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
    }

    // Identify boundary vertices (vertices that have at least one boundary edge)
    std::vector<bool> isBoundaryVertex(input.vertices.size(), false);
    std::vector<std::vector<uint32_t>> boundaryNeighbors(input.vertices.size());

    for (const auto& [edge, opposites] : edgeOpposites) {
        if (opposites.size() == 1) {
            // This is a boundary edge
            isBoundaryVertex[edge.v0] = true;
            isBoundaryVertex[edge.v1] = true;

            // Store boundary neighbors
            auto addBoundaryNeighbor = [&](uint32_t v, uint32_t n) {
                auto& neighbors = boundaryNeighbors[v];
                if (std::find(neighbors.begin(), neighbors.end(), n) == neighbors.end()) {
                    neighbors.push_back(n);
                }
            };
            addBoundaryNeighbor(edge.v0, edge.v1);
            addBoundaryNeighbor(edge.v1, edge.v0);
        }
    }

    // Step 1: Compute new positions for original vertices
    output.vertices.resize(input.vertices.size());

    for (size_t i = 0; i < input.vertices.size(); ++i) {
        const auto& neighbors = vertexNeighbors[i];
        size_t n = neighbors.size();

        if (n == 0) {
            output.vertices[i] = input.vertices[i];
            continue;
        }

        Vertex& outVert = output.vertices[i];

        if (isBoundaryVertex[i]) {
            // Boundary vertex rule: 3/4 * v + 1/8 * (b1 + b2)
            // where b1 and b2 are the two boundary neighbors
            const auto& bNeighbors = boundaryNeighbors[i];

            if (bNeighbors.size() == 2) {
                const Vertex& b1 = input.vertices[bNeighbors[0]];
                const Vertex& b2 = input.vertices[bNeighbors[1]];

                outVert.position = 0.75f * input.vertices[i].position
                                 + 0.125f * (b1.position + b2.position);
                outVert.normal = glm::normalize(
                    0.75f * input.vertices[i].normal
                    + 0.125f * (b1.normal + b2.normal)
                );
                outVert.texCoord = 0.75f * input.vertices[i].texCoord
                                 + 0.125f * (b1.texCoord + b2.texCoord);
            } else {
                // Corner vertex (only one boundary neighbor or more than 2)
                // Keep original position
                outVert = input.vertices[i];
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
                neighborSum += input.vertices[ni].position;
                normalSum += input.vertices[ni].normal;
                texSum += input.vertices[ni].texCoord;
            }

            outVert.position = (1.0f - static_cast<float>(n) * beta) * input.vertices[i].position
                             + beta * neighborSum;
            outVert.normal = glm::normalize(
                (1.0f - static_cast<float>(n) * beta) * input.vertices[i].normal
                + beta * normalSum
            );
            outVert.texCoord = (1.0f - static_cast<float>(n) * beta) * input.vertices[i].texCoord
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

        const Vertex& vert0 = input.vertices[v0];
        const Vertex& vert1 = input.vertices[v1];

        Vertex newVert;

        // Check if edge has two adjacent triangles (interior) or one (boundary)
        const auto& opposites = edgeOpposites[key];

        if (opposites.size() == 2) {
            // Interior edge: 3/8 * (v0 + v1) + 1/8 * (opposite0 + opposite1)
            const Vertex& opp0 = input.vertices[opposites[0]];
            const Vertex& opp1 = input.vertices[opposites[1]];

            newVert.position = 0.375f * (vert0.position + vert1.position)
                             + 0.125f * (opp0.position + opp1.position);
            newVert.normal = glm::normalize(
                0.375f * (vert0.normal + vert1.normal)
                + 0.125f * (opp0.normal + opp1.normal)
            );
            newVert.texCoord = 0.375f * (vert0.texCoord + vert1.texCoord)
                             + 0.125f * (opp0.texCoord + opp1.texCoord);
        } else {
            // Boundary edge: simple midpoint
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
    for (size_t i = 0; i < input.indices.size(); i += 3) {
        uint32_t i0 = input.indices[i];
        uint32_t i1 = input.indices[i + 1];
        uint32_t i2 = input.indices[i + 2];

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
