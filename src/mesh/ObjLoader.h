#pragma once

#include "MeshLoader.h"

class ObjLoader : public MeshLoader {
public:
    bool load(const std::string& path, MeshData& outData) override;
    bool canLoad(const std::string& extension) const override;
};
