#include "MeshLoader.h"
#include "ObjLoader.h"
#include <algorithm>

std::unique_ptr<MeshLoader> MeshLoader::createForFile(const std::string& path) {
    size_t dotPos = path.rfind('.');
    if (dotPos == std::string::npos) {
        return nullptr;
    }

    std::string ext = path.substr(dotPos);
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    if (ext == ".obj") {
        return std::make_unique<ObjLoader>();
    }

    return nullptr;
}
