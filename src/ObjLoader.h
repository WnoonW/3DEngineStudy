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

// Vertex deduplication을 위한 키 (header로 이동)
struct VertexKey {
    int posIdx = 0;
    int texIdx = 0;
    int norIdx = 0;

    bool operator==(const VertexKey& other) const {
        return posIdx == other.posIdx && texIdx == other.texIdx && norIdx == other.norIdx;
    }
};

namespace std {
    template<> struct hash<VertexKey> {
        size_t operator()(const VertexKey& k) const {
            return ((size_t)k.posIdx << 32) ^ ((size_t)k.texIdx << 16) ^ (size_t)k.norIdx;
        }
    };
}

struct SubMesh {
    std::vector<Vertex> vertices;
    std::vector<WORD> indices;
    MaterialData material;
};

class ObjLoader {
public:
    // 기존 API (backward compatibility)
    static bool Load(const std::wstring& filename,
        std::vector<Vertex>& outVertices,
        std::vector<WORD>& outIndices);

    // MTL 지원 API (추천)
    static bool LoadWithMaterials(
        const std::wstring& objFilename,
        std::vector<SubMesh>& outSubMeshes,
        const std::wstring& mtlFilename
    );

private:
    static void ParseFace(const std::string& token,
        const std::vector<DirectX::XMFLOAT3>& positions,
        const std::vector<DirectX::XMFLOAT3>& normals,
        const std::vector<DirectX::XMFLOAT2>& texCoords,
        SubMesh& currentSubMesh,
        std::unordered_map<VertexKey, WORD>& vertexMap);
};