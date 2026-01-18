#pragma once

#include "MeshData.h"
#include <string>
#include <memory>

class MeshLoader {
public:
    virtual ~MeshLoader() = default;

    virtual bool load(const std::string& path, MeshData& outData) = 0;
    virtual bool canLoad(const std::string& extension) const = 0;

    static std::unique_ptr<MeshLoader> createForFile(const std::string& path);
};
