#include "GismoLoader.h"
#include <iostream>
#include <algorithm>

#ifdef GISMO_AVAILABLE
#include <gismo.h>
#endif

bool GismoLoader::canLoad(const std::string& extension) const {
#ifdef GISMO_AVAILABLE
    std::string ext = extension;
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    return ext == ".xml" || ext == ".gz";
#else
    return false;
#endif
}

bool GismoLoader::load(const std::string& path, MeshData& outData) {
#ifdef GISMO_AVAILABLE
    MultiPatchData mpData;
    if (!loadMultiPatch(path, mpData, m_tessellationLevel)) {
        return false;
    }

    // Merge all patches into a single mesh
    outData.clear();

    uint32_t vertexOffset = 0;
    for (const auto& patch : mpData.patches) {
        // Add vertices
        for (const auto& v : patch.vertices) {
            outData.vertices.push_back(v);
        }

        // Add indices with offset
        for (uint32_t idx : patch.indices) {
            outData.indices.push_back(idx + vertexOffset);
        }

        vertexOffset += static_cast<uint32_t>(patch.vertices.size());
    }

    outData.calculateBounds();
    return true;
#else
    std::cerr << "G+Smo support not available. Rebuild with GISMO_AVAILABLE." << std::endl;
    return false;
#endif
}

bool GismoLoader::loadMultiPatch(const std::string& path, MultiPatchData& outData, int tessellationLevel) {
#ifdef GISMO_AVAILABLE
    std::cout << "Loading G+Smo multipatch: " << path << std::endl;

    // Load the file using G+Smo
    gismo::gsFileData<> fileData(path);

    // Try to get as multipatch first
    if (fileData.has<gismo::gsMultiPatch<>>()) {
        gismo::gsMultiPatch<> mp;
        fileData.getFirst(mp);

        std::cout << "  Found MultiPatch with " << mp.nPatches() << " patches" << std::endl;

        outData.patches.reserve(mp.nPatches());
        outData.patchNames.reserve(mp.nPatches());
        outData.name = path;

        // Extract filename for naming
        size_t lastSlash = path.find_last_of("/\\");
        if (lastSlash != std::string::npos) {
            outData.name = path.substr(lastSlash + 1);
        }

        // Tessellate each patch
        for (size_t i = 0; i < mp.nPatches(); ++i) {
            const gismo::gsGeometry<>& patch = mp.patch(i);

            MeshData meshData = tessellatePatch(patch, tessellationLevel, tessellationLevel);
            outData.patches.push_back(std::move(meshData));
            outData.patchNames.push_back(outData.name + "_patch" + std::to_string(i));

            std::cout << "  Patch " << i << ": " << outData.patches.back().vertices.size()
                      << " vertices, " << outData.patches.back().indices.size() / 3 << " triangles" << std::endl;
        }
    }
    // Try single geometry
    else if (fileData.has<gismo::gsGeometry<>>()) {
        gismo::gsGeometry<>::uPtr geom = fileData.getFirst<gismo::gsGeometry<>>();

        std::cout << "  Found single Geometry" << std::endl;

        outData.name = path;
        size_t lastSlash = path.find_last_of("/\\");
        if (lastSlash != std::string::npos) {
            outData.name = path.substr(lastSlash + 1);
        }

        MeshData meshData = tessellatePatch(*geom, tessellationLevel, tessellationLevel);
        outData.patches.push_back(std::move(meshData));
        outData.patchNames.push_back(outData.name);
    }
    else {
        std::cerr << "Error: No geometry found in " << path << std::endl;
        return false;
    }

    std::cout << "  Total: " << outData.totalVertices() << " vertices, "
              << outData.totalTriangles() << " triangles" << std::endl;

    return true;
#else
    std::cerr << "G+Smo support not available." << std::endl;
    return false;
#endif
}

#ifdef GISMO_AVAILABLE
MeshData GismoLoader::tessellatePatch(const gismo::gsGeometry<double>& patch,
                                       int uSamples, int vSamples) {
    MeshData data;

    // Get parameter domain
    gismo::gsMatrix<> support = patch.support();  // 2x2: [[uMin,uMax],[vMin,vMax]]
    gismo::gsVector<> lower(2), upper(2);
    lower << support(0, 0), support(1, 0);
    upper << support(0, 1), support(1, 1);

    // Generate uniform parameter grid
    gismo::gsVector<unsigned> numPts(2);
    numPts << uSamples, vSamples;
    gismo::gsMatrix<> uv = gismo::gsPointGrid(lower, upper, numPts);

    // Evaluate positions at grid points
    gismo::gsMatrix<> positions = patch.eval(uv);  // geoDim x N

    // Check geometry dimension
    int geoDim = patch.geoDim();
    int parDim = patch.parDim();

    // Evaluate derivatives for normal computation (only for surfaces in 3D)
    gismo::gsMatrix<> derivs;
    bool hasDerivatives = (parDim == 2 && geoDim == 3);
    if (hasDerivatives) {
        derivs = patch.deriv(uv);  // (geoDim * parDim) x N = 6 x N for 3D surface
    }

    // Build vertices
    data.vertices.reserve(uv.cols());
    for (int i = 0; i < uv.cols(); ++i) {
        Vertex v;

        // Position (handle 2D and 3D cases)
        if (geoDim >= 3) {
            v.position = glm::vec3(positions(0, i), positions(1, i), positions(2, i));
        } else if (geoDim == 2) {
            v.position = glm::vec3(positions(0, i), positions(1, i), 0.0f);
        } else {
            v.position = glm::vec3(positions(0, i), 0.0f, 0.0f);
        }

        // Texture coordinates from parameters
        v.texCoord = glm::vec2(
            (uv(0, i) - lower(0)) / (upper(0) - lower(0)),
            (uv(1, i) - lower(1)) / (upper(1) - lower(1))
        );

        // Normal from cross product of partial derivatives
        if (hasDerivatives) {
            // G+Smo deriv layout for 3D surface (geoDim=3, parDim=2):
            // Row 0: dx/du, Row 1: dx/dv
            // Row 2: dy/du, Row 3: dy/dv
            // Row 4: dz/du, Row 5: dz/dv
            glm::vec3 dPdu(derivs(0, i), derivs(2, i), derivs(4, i));
            glm::vec3 dPdv(derivs(1, i), derivs(3, i), derivs(5, i));
            glm::vec3 normal = glm::cross(dPdu, dPdv);
            float len = glm::length(normal);
            if (len > 1e-10f) {
                v.normal = normal / len;
            } else {
                v.normal = glm::vec3(0.0f, 0.0f, 1.0f);
            }
        } else {
            // Default normal for 2D geometry
            v.normal = glm::vec3(0.0f, 0.0f, 1.0f);
        }

        data.vertices.push_back(v);
    }

    // Build triangle indices (structured grid topology)
    // Two triangles per quad cell
    data.indices.reserve((uSamples - 1) * (vSamples - 1) * 6);
    for (int j = 0; j < vSamples - 1; ++j) {
        for (int i = 0; i < uSamples - 1; ++i) {
            uint32_t idx00 = j * uSamples + i;
            uint32_t idx10 = idx00 + 1;
            uint32_t idx01 = idx00 + uSamples;
            uint32_t idx11 = idx01 + 1;

            // First triangle (lower-left)
            data.indices.push_back(idx00);
            data.indices.push_back(idx10);
            data.indices.push_back(idx11);

            // Second triangle (upper-right)
            data.indices.push_back(idx11);
            data.indices.push_back(idx01);
            data.indices.push_back(idx00);
        }
    }

    data.calculateBounds();
    return data;
}
#else
MeshData GismoLoader::tessellatePatch(const gismo::gsGeometry<double>& /*patch*/,
                                       int /*uSamples*/, int /*vSamples*/) {
    return MeshData();
}
#endif
