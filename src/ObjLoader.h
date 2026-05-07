#pragma once

#include <vector>
#include <string>
#include <DirectXMath.h>

struct Vertex {
    DirectX::XMFLOAT3 Position;
    DirectX::XMFLOAT3 Normal;
    DirectX::XMFLOAT2 TexCoord;
};

class ObjLoader {
public:
    static bool Load(const std::string& filename, std::vector<Vertex>& outVertices, std::vector<uint32_t>& outIndices);
};