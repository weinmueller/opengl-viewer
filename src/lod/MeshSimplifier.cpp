#include "MeshSimplifier.h"
#include <glm/glm.hpp>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <queue>
#include <algorithm>
#include <cmath>
#include <limits>

// 4x4 symmetric matrix for quadric error metric
struct Quadric {
    // Stored as upper triangular (10 unique values)
    // a = m[0][0], b = m[0][1], c = m[0][2], d = m[0][3]
    //              e = m[1][1], f = m[1][2], g = m[1][3]
    //                           h = m[2][2], i = m[2][3]
    //                                        j = m[3][3]
    double a, b, c, d;
    double    e, f, g;
    double       h, i;
    double          j;

    Quadric() : a(0), b(0), c(0), d(0), e(0), f(0), g(0), h(0), i(0), j(0) {}

    // Create from plane equation ax + by + cz + d = 0
    Quadric(double aa, double bb, double cc, double dd) {
        a = aa * aa; b = aa * bb; c = aa * cc; d = aa * dd;
                     e = bb * bb; f = bb * cc; g = bb * dd;
                                  h = cc * cc; i = cc * dd;
                                               j = dd * dd;
    }

    Quadric& operator+=(const Quadric& other) {
        a += other.a; b += other.b; c += other.c; d += other.d;
        e += other.e; f += other.f; g += other.g;
        h += other.h; i += other.i;
        j += other.j;
        return *this;
    }

    // Evaluate quadric error for vertex position
    double evaluate(const glm::vec3& v) const {
        double x = v.x, y = v.y, z = v.z;
        return a*x*x + 2*b*x*y + 2*c*x*z + 2*d*x
             + e*y*y + 2*f*y*z + 2*g*y
             + h*z*z + 2*i*z
             + j;
    }

    // Find optimal position that minimizes error
    // Returns true if successful, false if matrix is singular
    bool findOptimal(glm::vec3& result) const {
        // Solve Ax = b where A is the 3x3 upper-left block and b is negative of last column
        // Use Cramer's rule for 3x3 system
        double det = a * (e * h - f * f)
                   - b * (b * h - c * f)
                   + c * (b * f - c * e);

        if (std::abs(det) < 1e-10) {
            return false;
        }

        double invDet = 1.0 / det;

        // Right-hand side: -[d, g, i]
        double rx = -d, ry = -g, rz = -i;

        // Cramers rule
        result.x = static_cast<float>(invDet * (rx * (e * h - f * f) - ry * (b * h - c * f) + rz * (b * f - c * e)));
        result.y = static_cast<float>(invDet * (a * (ry * h - rz * f) - b * (rx * h - rz * c) + c * (rx * f - ry * c)));
        result.z = static_cast<float>(invDet * (a * (e * rz - f * ry) - b * (b * rz - f * rx) + c * (b * ry - e * rx)));

        return true;
    }
};

// Edge structure for the priority queue
struct Edge {
    uint32_t v0, v1;
    float cost;
    glm::vec3 optimalPos;

    bool operator>(const Edge& other) const {
        return cost > other.cost;
    }
};

// Hash function for edge pairs
struct EdgeHash {
    size_t operator()(const std::pair<uint32_t, uint32_t>& e) const {
        return std::hash<uint64_t>()(static_cast<uint64_t>(e.first) << 32 | e.second);
    }
};

// Make ordered edge pair (smaller index first)
inline std::pair<uint32_t, uint32_t> makeEdge(uint32_t a, uint32_t b) {
    return a < b ? std::make_pair(a, b) : std::make_pair(b, a);
}

MeshData MeshSimplifier::simplify(const MeshData& input, uint32_t targetTriangles) {
    SimplificationProgress progress;
    return simplifyWithProgress(input, targetTriangles, progress);
}

MeshData MeshSimplifier::simplifyRatio(const MeshData& input, float ratio) {
    SimplificationProgress progress;
    return simplifyRatioWithProgress(input, ratio, progress);
}

MeshData MeshSimplifier::simplifyRatioWithProgress(
    const MeshData& input,
    float ratio,
    SimplificationProgress& progress)
{
    uint32_t currentTriangles = static_cast<uint32_t>(input.indices.size() / 3);
    uint32_t targetTriangles = static_cast<uint32_t>(currentTriangles * ratio);
    targetTriangles = std::max(targetTriangles, 4u);  // Keep at least 4 triangles
    return simplifyWithProgress(input, targetTriangles, progress);
}

MeshData MeshSimplifier::simplifyWithProgress(
    const MeshData& input,
    uint32_t targetTriangles,
    SimplificationProgress& progress)
{
    progress.reset();

    uint32_t numVertices = static_cast<uint32_t>(input.vertices.size());
    uint32_t numTriangles = static_cast<uint32_t>(input.indices.size() / 3);

    // If already at or below target, return copy
    if (numTriangles <= targetTriangles) {
        progress.progress.store(1.0f, std::memory_order_relaxed);
        progress.completed.store(true, std::memory_order_relaxed);
        return input;
    }

    // Copy vertex positions for manipulation
    std::vector<glm::vec3> positions(numVertices);
    std::vector<glm::vec3> normals(numVertices);
    std::vector<glm::vec2> texCoords(numVertices);
    for (uint32_t i = 0; i < numVertices; i++) {
        positions[i] = input.vertices[i].position;
        normals[i] = input.vertices[i].normal;
        texCoords[i] = input.vertices[i].texCoord;
    }

    // Copy triangles
    std::vector<glm::uvec3> triangles(numTriangles);
    for (uint32_t i = 0; i < numTriangles; i++) {
        triangles[i] = glm::uvec3(
            input.indices[i * 3],
            input.indices[i * 3 + 1],
            input.indices[i * 3 + 2]
        );
    }

    // Track which triangles use each vertex
    std::vector<std::unordered_set<uint32_t>> vertexTriangles(numVertices);
    for (uint32_t ti = 0; ti < numTriangles; ti++) {
        vertexTriangles[triangles[ti].x].insert(ti);
        vertexTriangles[triangles[ti].y].insert(ti);
        vertexTriangles[triangles[ti].z].insert(ti);
    }

    // Compute initial quadrics for each vertex
    std::vector<Quadric> quadrics(numVertices);

    for (uint32_t ti = 0; ti < numTriangles; ti++) {
        const auto& tri = triangles[ti];
        glm::vec3 v0 = positions[tri.x];
        glm::vec3 v1 = positions[tri.y];
        glm::vec3 v2 = positions[tri.z];

        // Compute plane equation
        glm::vec3 e1 = v1 - v0;
        glm::vec3 e2 = v2 - v0;
        glm::vec3 normal = glm::cross(e1, e2);
        float len = glm::length(normal);
        if (len > 1e-10f) {
            normal /= len;
            double d = -glm::dot(normal, v0);
            Quadric q(normal.x, normal.y, normal.z, d);

            // Weight by triangle area
            float area = len * 0.5f;
            q.a *= area; q.b *= area; q.c *= area; q.d *= area;
            q.e *= area; q.f *= area; q.g *= area;
            q.h *= area; q.i *= area;
            q.j *= area;

            quadrics[tri.x] += q;
            quadrics[tri.y] += q;
            quadrics[tri.z] += q;
        }
    }

    // Build edge set and priority queue
    std::unordered_map<std::pair<uint32_t, uint32_t>, size_t, EdgeHash> edgeIndices;
    std::vector<Edge> edges;

    for (uint32_t ti = 0; ti < numTriangles; ti++) {
        const auto& tri = triangles[ti];
        uint32_t v[3] = {tri.x, tri.y, tri.z};

        for (int i = 0; i < 3; i++) {
            auto edge = makeEdge(v[i], v[(i + 1) % 3]);
            if (edgeIndices.find(edge) == edgeIndices.end()) {
                edgeIndices[edge] = edges.size();

                Edge e;
                e.v0 = edge.first;
                e.v1 = edge.second;

                // Compute collapse cost
                Quadric combined = quadrics[e.v0];
                combined += quadrics[e.v1];

                // Try to find optimal position
                if (!combined.findOptimal(e.optimalPos)) {
                    // Fall back to midpoint
                    e.optimalPos = (positions[e.v0] + positions[e.v1]) * 0.5f;
                }

                // Clamp optimal position to be near the edge
                glm::vec3 mid = (positions[e.v0] + positions[e.v1]) * 0.5f;
                float edgeLen = glm::length(positions[e.v1] - positions[e.v0]);
                float dist = glm::length(e.optimalPos - mid);
                if (dist > edgeLen * 2.0f) {
                    e.optimalPos = mid;
                }

                e.cost = static_cast<float>(combined.evaluate(e.optimalPos));
                edges.push_back(e);
            }
        }
    }

    // Build priority queue
    auto cmp = [](const size_t a, const size_t b, const std::vector<Edge>& edges) {
        return edges[a].cost > edges[b].cost;
    };

    std::vector<size_t> heap;
    std::vector<bool> edgeValid(edges.size(), true);
    heap.reserve(edges.size());
    for (size_t i = 0; i < edges.size(); i++) {
        heap.push_back(i);
    }
    std::make_heap(heap.begin(), heap.end(), [&](size_t a, size_t b) {
        return edges[a].cost > edges[b].cost;
    });

    // Track removed vertices
    std::vector<uint32_t> vertexRemap(numVertices);
    for (uint32_t i = 0; i < numVertices; i++) {
        vertexRemap[i] = i;
    }

    auto findRoot = [&](uint32_t v) -> uint32_t {
        while (vertexRemap[v] != v) {
            vertexRemap[v] = vertexRemap[vertexRemap[v]];  // Path compression
            v = vertexRemap[v];
        }
        return v;
    };

    // Track valid triangles
    std::vector<bool> triangleValid(numTriangles, true);
    uint32_t currentTriangleCount = numTriangles;
    uint32_t trianglesToRemove = numTriangles - targetTriangles;
    uint32_t trianglesRemoved = 0;

    // Main simplification loop
    while (currentTriangleCount > targetTriangles && !heap.empty()) {
        if (progress.isCancelled()) {
            return input;  // Return original on cancellation
        }

        // Update progress
        float prog = static_cast<float>(trianglesRemoved) / static_cast<float>(trianglesToRemove);
        progress.progress.store(prog, std::memory_order_relaxed);

        // Get minimum cost edge
        std::pop_heap(heap.begin(), heap.end(), [&](size_t a, size_t b) {
            return edges[a].cost > edges[b].cost;
        });
        size_t edgeIdx = heap.back();
        heap.pop_back();

        if (!edgeValid[edgeIdx]) {
            continue;
        }

        Edge& e = edges[edgeIdx];
        uint32_t v0 = findRoot(e.v0);
        uint32_t v1 = findRoot(e.v1);

        if (v0 == v1) {
            edgeValid[edgeIdx] = false;
            continue;
        }

        // Check if collapse would cause mesh inversion
        bool wouldInvert = false;
        for (uint32_t ti : vertexTriangles[v0]) {
            if (!triangleValid[ti]) continue;

            const auto& tri = triangles[ti];
            uint32_t tv0 = findRoot(tri.x);
            uint32_t tv1 = findRoot(tri.y);
            uint32_t tv2 = findRoot(tri.z);

            // Skip triangles that will be removed
            if ((tv0 == v0 && tv1 == v1) || (tv0 == v1 && tv1 == v0) ||
                (tv1 == v0 && tv2 == v1) || (tv1 == v1 && tv2 == v0) ||
                (tv2 == v0 && tv0 == v1) || (tv2 == v1 && tv0 == v0)) {
                continue;
            }

            // Check normal flip
            glm::vec3 oldP[3] = {positions[tv0], positions[tv1], positions[tv2]};
            glm::vec3 newP[3] = {
                tv0 == v0 || tv0 == v1 ? e.optimalPos : positions[tv0],
                tv1 == v0 || tv1 == v1 ? e.optimalPos : positions[tv1],
                tv2 == v0 || tv2 == v1 ? e.optimalPos : positions[tv2]
            };

            glm::vec3 oldN = glm::cross(oldP[1] - oldP[0], oldP[2] - oldP[0]);
            glm::vec3 newN = glm::cross(newP[1] - newP[0], newP[2] - newP[0]);

            if (glm::dot(oldN, newN) <= 0.0f) {
                wouldInvert = true;
                break;
            }
        }

        for (uint32_t ti : vertexTriangles[v1]) {
            if (wouldInvert) break;
            if (!triangleValid[ti]) continue;

            const auto& tri = triangles[ti];
            uint32_t tv0 = findRoot(tri.x);
            uint32_t tv1 = findRoot(tri.y);
            uint32_t tv2 = findRoot(tri.z);

            // Skip triangles that will be removed
            if ((tv0 == v0 && tv1 == v1) || (tv0 == v1 && tv1 == v0) ||
                (tv1 == v0 && tv2 == v1) || (tv1 == v1 && tv2 == v0) ||
                (tv2 == v0 && tv0 == v1) || (tv2 == v1 && tv0 == v0)) {
                continue;
            }

            // Check normal flip
            glm::vec3 oldP[3] = {positions[tv0], positions[tv1], positions[tv2]};
            glm::vec3 newP[3] = {
                tv0 == v0 || tv0 == v1 ? e.optimalPos : positions[tv0],
                tv1 == v0 || tv1 == v1 ? e.optimalPos : positions[tv1],
                tv2 == v0 || tv2 == v1 ? e.optimalPos : positions[tv2]
            };

            glm::vec3 oldN = glm::cross(oldP[1] - oldP[0], oldP[2] - oldP[0]);
            glm::vec3 newN = glm::cross(newP[1] - newP[0], newP[2] - newP[0]);

            if (glm::dot(oldN, newN) <= 0.0f) {
                wouldInvert = true;
                break;
            }
        }

        if (wouldInvert) {
            edgeValid[edgeIdx] = false;
            continue;
        }

        // Perform collapse: merge v1 into v0
        positions[v0] = e.optimalPos;

        // Blend normals
        normals[v0] = glm::normalize(normals[v0] + normals[v1]);

        // Average texture coordinates
        texCoords[v0] = (texCoords[v0] + texCoords[v1]) * 0.5f;

        // Update quadric
        quadrics[v0] += quadrics[v1];

        // Remap v1 to v0
        vertexRemap[v1] = v0;

        // Update triangle references and remove degenerate triangles
        for (uint32_t ti : vertexTriangles[v1]) {
            if (!triangleValid[ti]) continue;
            vertexTriangles[v0].insert(ti);
        }
        vertexTriangles[v1].clear();

        // Mark degenerate triangles as invalid
        for (uint32_t ti : vertexTriangles[v0]) {
            if (!triangleValid[ti]) continue;

            auto& tri = triangles[ti];
            uint32_t tv0 = findRoot(tri.x);
            uint32_t tv1 = findRoot(tri.y);
            uint32_t tv2 = findRoot(tri.z);

            if (tv0 == tv1 || tv1 == tv2 || tv2 == tv0) {
                triangleValid[ti] = false;
                currentTriangleCount--;
                trianglesRemoved++;
            }
        }

        edgeValid[edgeIdx] = false;
    }

    // Build output mesh
    MeshData result;

    // Compact vertices
    std::vector<uint32_t> newVertexIndex(numVertices, UINT32_MAX);
    for (uint32_t ti = 0; ti < numTriangles; ti++) {
        if (!triangleValid[ti]) continue;

        const auto& tri = triangles[ti];
        uint32_t v[3] = {findRoot(tri.x), findRoot(tri.y), findRoot(tri.z)};

        for (int i = 0; i < 3; i++) {
            if (newVertexIndex[v[i]] == UINT32_MAX) {
                newVertexIndex[v[i]] = static_cast<uint32_t>(result.vertices.size());
                Vertex vertex;
                vertex.position = positions[v[i]];
                vertex.normal = normals[v[i]];
                vertex.texCoord = texCoords[v[i]];
                result.vertices.push_back(vertex);
            }
        }
    }

    // Build indices
    for (uint32_t ti = 0; ti < numTriangles; ti++) {
        if (!triangleValid[ti]) continue;

        const auto& tri = triangles[ti];
        uint32_t v[3] = {findRoot(tri.x), findRoot(tri.y), findRoot(tri.z)};

        result.indices.push_back(newVertexIndex[v[0]]);
        result.indices.push_back(newVertexIndex[v[1]]);
        result.indices.push_back(newVertexIndex[v[2]]);
    }

    // Recalculate normals for better quality
    std::vector<glm::vec3> newNormals(result.vertices.size(), glm::vec3(0.0f));
    for (size_t i = 0; i < result.indices.size(); i += 3) {
        uint32_t i0 = result.indices[i];
        uint32_t i1 = result.indices[i + 1];
        uint32_t i2 = result.indices[i + 2];

        glm::vec3 v0 = result.vertices[i0].position;
        glm::vec3 v1 = result.vertices[i1].position;
        glm::vec3 v2 = result.vertices[i2].position;

        glm::vec3 normal = glm::cross(v1 - v0, v2 - v0);
        newNormals[i0] += normal;
        newNormals[i1] += normal;
        newNormals[i2] += normal;
    }

    for (size_t i = 0; i < result.vertices.size(); i++) {
        float len = glm::length(newNormals[i]);
        if (len > 1e-10f) {
            result.vertices[i].normal = newNormals[i] / len;
        }
    }

    result.calculateBounds();

    progress.progress.store(1.0f, std::memory_order_relaxed);
    progress.completed.store(true, std::memory_order_relaxed);

    return result;
}
