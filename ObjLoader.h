#pragma once
#include <vector>
#include <string>
#include <sstream>
#include <fstream>
#include <DirectXMath.h>
#include "Object.h"

using namespace DirectX;

struct MeshData
{
    std::vector<ObjectVertex> vertices;
    std::vector<WORD> indices;
    std::string name;
};

class ObjLoader
{
public:
    static bool Load(const std::string& filename, MeshData& outMesh);
};
