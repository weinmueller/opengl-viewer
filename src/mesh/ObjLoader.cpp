#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

#include "ObjLoader.h"
#include <iostream>
#include <unordered_map>
#include <filesystem>

namespace {
    struct VertexHash {
        size_t operator()(const Vertex& v) const {
            size_t h1 = std::hash<float>()(v.position.x);
            size_t h2 = std::hash<float>()(v.position.y);
            size_t h3 = std::hash<float>()(v.position.z);
            size_t h4 = std::hash<float>()(v.normal.x);
            size_t h5 = std::hash<float>()(v.normal.y);
            size_t h6 = std::hash<float>()(v.normal.z);
            return h1 ^ (h2 << 1) ^ (h3 << 2) ^ (h4 << 3) ^ (h5 << 4) ^ (h6 << 5);
        }
    };

    struct VertexEqual {
        bool operator()(const Vertex& a, const Vertex& b) const {
            return a.position == b.position &&
                   a.normal == b.normal &&
                   a.texCoord == b.texCoord;
        }
    };
}

bool ObjLoader::load(const std::string& path, MeshData& outData) {
    outData.clear();

    // Get directory of OBJ file for MTL search
    std::filesystem::path objPath(path);
    std::string objDir = objPath.parent_path().string();
    if (objDir.empty()) {
        objDir = ".";
    }

    tinyobj::ObjReaderConfig reader_config;
    reader_config.mtl_search_path = objDir;
    reader_config.triangulate = true;

    tinyobj::ObjReader reader;

    if (!reader.ParseFromFile(path, reader_config)) {
        if (!reader.Error().empty()) {
            std::cerr << "ObjLoader error: " << reader.Error() << std::endl;
        }
        return false;
    }

    if (!reader.Warning().empty()) {
        std::cout << "ObjLoader warning: " << reader.Warning() << std::endl;
    }

    const auto& attrib = reader.GetAttrib();
    const auto& shapes = reader.GetShapes();
    const auto& materials = reader.GetMaterials();

    // Extract diffuse texture from first material with a texture
    for (const auto& mat : materials) {
        if (!mat.diffuse_texname.empty()) {
            // Resolve texture path relative to OBJ directory
            std::filesystem::path texPath(mat.diffuse_texname);
            if (texPath.is_relative()) {
                texPath = std::filesystem::path(objDir) / texPath;
            }
            outData.texturePath = texPath.string();
            std::cout << "Found diffuse texture: " << outData.texturePath << std::endl;
            break;
        }
    }

    std::unordered_map<Vertex, uint32_t, VertexHash, VertexEqual> uniqueVertices;

    for (const auto& shape : shapes) {
        for (const auto& index : shape.mesh.indices) {
            Vertex vertex{};

            vertex.position = {
                attrib.vertices[3 * index.vertex_index + 0],
                attrib.vertices[3 * index.vertex_index + 1],
                attrib.vertices[3 * index.vertex_index + 2]
            };

            if (index.normal_index >= 0) {
                vertex.normal = {
                    attrib.normals[3 * index.normal_index + 0],
                    attrib.normals[3 * index.normal_index + 1],
                    attrib.normals[3 * index.normal_index + 2]
                };
            } else {
                vertex.normal = glm::vec3(0.0f, 1.0f, 0.0f);
            }

            if (index.texcoord_index >= 0) {
                vertex.texCoord = {
                    attrib.texcoords[2 * index.texcoord_index + 0],
                    1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
                };
            } else {
                vertex.texCoord = glm::vec2(0.0f);
            }

            if (uniqueVertices.count(vertex) == 0) {
                uniqueVertices[vertex] = static_cast<uint32_t>(outData.vertices.size());
                outData.vertices.push_back(vertex);
            }

            outData.indices.push_back(uniqueVertices[vertex]);
        }
    }

    outData.calculateBounds();

    return true;
}

bool ObjLoader::canLoad(const std::string& extension) const {
    return extension == ".obj" || extension == "obj";
}
