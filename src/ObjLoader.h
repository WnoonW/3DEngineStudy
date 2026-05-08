#pragma once

#include <windows.h>
#include <vector>
#include <string>
#include <unordered_map>
#include <DirectXMath.h>
#include "Material.h"

struct Vertex {
    DirectX::XMFLOAT3 Position;
    DirectX::XMFLOAT3 Normal;
    DirectX::XMFLOAT2 TexCoord;
};

struct SubMesh {
    std::string materialName = "default";
    UINT indexStart = 0;
    UINT indexCount = 0;
};

struct LoadedObj {
    std::vector<Vertex> vertices;
    std::vector<WORD> indices;
    std::unordered_map<std::string, MaterialData> materials;
    std::vector<SubMesh> subMeshes;
};

class ObjLoader {
public:
    // Recommended new interface with material support
    static bool Load(const std::string& filename, LoadedObj& outMesh);
    static bool Load(const std::wstring& filename, LoadedObj& outMesh);

    // Old interface (kept for backward compatibility)
    static bool Load(const std::string& filename, std::vector<Vertex>& outVertices, std::vector<WORD>& outIndices);
};