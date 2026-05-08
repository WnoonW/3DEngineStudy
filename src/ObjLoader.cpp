#include "ObjLoader.h"
#include "MtlLoader.h"
#include <fstream>
#include <sstream>
#include <vector>
#include <unordered_map>
#include <string>
#include <algorithm>
#include <iostream>

struct VertexKey {
    int posIdx, texIdx, norIdx;
    bool operator==(const VertexKey& other) const {
        return posIdx == other.posIdx && texIdx == other.texIdx && norIdx == other.norIdx;
    }
};

namespace std {
    template<> struct hash<VertexKey> {
        size_t operator()(const VertexKey& k) const {
            return ((size_t)k.posIdx << 32) ^ (k.texIdx << 16) ^ k.norIdx;
        }
    };
}

bool ObjLoader::Load(const std::string& filename, 
                     std::vector<Vertex>& outVertices, 
                     std::vector<WORD>& outIndices)
{
    std::vector<SubMesh> subMeshes;
    if (!LoadWithMaterials(filename, subMeshes)) return false;

    outVertices.clear();
    outIndices.clear();
    WORD baseIndex = 0;

    for (const auto& sub : subMeshes) {
        outVertices.insert(outVertices.end(), sub.vertices.begin(), sub.vertices.end());
        for (WORD idx : sub.indices) {
            outIndices.push_back(baseIndex + idx);
        }
        baseIndex += (WORD)sub.vertices.size();
    }
    return true;
}

bool ObjLoader::LoadWithMaterials(const std::string& filename, 
                                  std::vector<SubMesh>& outSubMeshes)
{
    outSubMeshes.clear();

    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "[ObjLoader] Failed to open: " << filename << std::endl;
        return false;
    }

    std::vector<DirectX::XMFLOAT3> positions;
    std::vector<DirectX::XMFLOAT3> normals;
    std::vector<DirectX::XMFLOAT2> texCoords;

    std::unordered_map<std::string, MaterialData> materials;
    std::string mtlLibPath;

    SubMesh* currentSubMesh = nullptr;
    std::unordered_map<VertexKey, WORD> vertexMap;

    std::string line;
    while (std::getline(file, line)) {
        std::istringstream iss(line);
        std::string type;
        iss >> type;

        if (type == "v") {
            DirectX::XMFLOAT3 pos{};
            iss >> pos.x >> pos.y >> pos.z;
            positions.push_back(pos);
        }
        else if (type == "vn") {
            DirectX::XMFLOAT3 n{};
            iss >> n.x >> n.y >> n.z;
            normals.push_back(n);
        }
        else if (type == "vt") {
            DirectX::XMFLOAT2 t{};
            iss >> t.x >> t.y;
            texCoords.push_back(t);
        }
        else if (type == "mtllib") {
            std::string mtlFile;
            iss >> mtlFile;

            size_t lastSlash = filename.find_last_of("/\\");
            std::string basePath = (lastSlash != std::string::npos) ? filename.substr(0, lastSlash + 1) : "";
            std::wstring fullMtlPath = std::wstring(basePath.begin(), basePath.end()) + std::wstring(mtlFile.begin(), mtlFile.end());

            MtlLoader::Load(fullMtlPath, materials);
        }
        else if (type == "usemtl") {
            std::string matName;
            iss >> matName;

            outSubMeshes.emplace_back();
            currentSubMesh = &outSubMeshes.back();
            vertexMap.clear();

            auto it = materials.find(matName);
            if (it != materials.end()) {
                currentSubMesh->material = it->second;
            } else {
                currentSubMesh->material.name = matName;
            }
        }
        else if (type == "f" && currentSubMesh) {
            std::string token;
            while (iss >> token) {
                if (token.empty()) continue;
                ParseFace(token, positions, normals, texCoords, *currentSubMesh, vertexMap);
            }
        }
    }

    std::cout << "[ObjLoader] Loaded " << outSubMeshes.size() << " SubMesh(es) with MTL from " << filename << std::endl;
    return true;
}

void ObjLoader::ParseFace(const std::string& token,
                          const std::vector<DirectX::XMFLOAT3>& positions,
                          const std::vector<DirectX::XMFLOAT3>& normals,
                          const std::vector<DirectX::XMFLOAT2>& texCoords,
                          SubMesh& currentSubMesh,
                          std::unordered_map<VertexKey, WORD>& vertexMap)
{
    std::string t = token;
    std::replace(t.begin(), t.end(), '/', ' ');
    std::istringstream faceIss(t);

    int vIdx = 0, tIdx = 0, nIdx = 0;
    faceIss >> vIdx >> tIdx >> nIdx;

    VertexKey key{ vIdx, tIdx, nIdx };

    if (vertexMap.find(key) == vertexMap.end()) {
        Vertex vertex{};
        if (vIdx > 0 && vIdx <= (int)positions.size()) vertex.Position = positions[vIdx - 1];
        if (nIdx > 0 && nIdx <= (int)normals.size()) vertex.Normal = normals[nIdx - 1];
        if (tIdx > 0 && tIdx <= (int)texCoords.size()) vertex.TexCoord = texCoords[tIdx - 1];

        WORD newIndex = (WORD)currentSubMesh.vertices.size();
        currentSubMesh.vertices.push_back(vertex);
        vertexMap[key] = newIndex;
    }

    currentSubMesh.indices.push_back(vertexMap[key]);
}