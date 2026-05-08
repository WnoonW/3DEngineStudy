#pragma once

#include <windows.h>
#include <vector>
#include <string>
#include <DirectXMath.h>
#include "Material.h"

struct Vertex {
    DirectX::XMFLOAT3 Position;
    DirectX::XMFLOAT3 Normal;
    DirectX::XMFLOAT2 TexCoord;
};

struct SubMesh {
    std::vector<Vertex> vertices;
    std::vector<WORD> indices;
    MaterialData material;
};

class ObjLoader {
public:
    static bool Load(const std::string& filename, 
                     std::vector<Vertex>& outVertices, 
                     std::vector<WORD>& outIndices);

    static bool LoadWithMaterials(const std::string& filename, 
                                  std::vector<SubMesh>& outSubMeshes);

private:
    static void ParseFace(const std::string& token,
                          const std::vector<DirectX::XMFLOAT3>& positions,
                          const std::vector<DirectX::XMFLOAT3>& normals,
                          const std::vector<DirectX::XMFLOAT2>& texCoords,
                          SubMesh& currentSubMesh,
                          std::unordered_map<VertexKey, WORD>& vertexMap);
};