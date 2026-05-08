#include "ObjLoader.h"
#include "MtlLoader.h"
#include <fstream>
#include <sstream>
#include <vector>
#include <unordered_map>
#include <string>
#include <algorithm>
#include <iostream>
#include <DirectXMath.h>

using namespace DirectX;


bool ObjLoader::Load(const std::wstring& filename, 
                     std::vector<Vertex>& outVertices, 
                     std::vector<WORD>& outIndices)
{
    std::vector<SubMesh> subMeshes;
    if (!LoadWithMaterials(filename + L".obj", subMeshes, filename + L".mtl")) return false;

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

bool ObjLoader::LoadWithMaterials(const std::wstring& filename,
    std::vector<SubMesh>& outSubMeshes,
    const std::wstring& mtlFilename)
{
    std::ifstream file(filename);
    if (!file.is_open()) {
        OutputDebugStringA("Failed to open OBJ file\n");
        return false;
    }

    std::unordered_map<std::string, MaterialData> materials;
    std::string currentMtlName;
    SubMesh* currentSubMesh = nullptr;

    // ====================== MTL 로드 (별도 파일 우선) ======================
    bool mtlLoaded = false;
    if (!mtlFilename.empty()) {
        if (MtlLoader::Load(mtlFilename, materials)) {
            mtlLoaded = true;
            OutputDebugStringA("Loaded external MTL file successfully.\n");
        }
    }

    // ====================== OBJ 파싱 ======================
    std::string line;
    while (std::getline(file, line)) {
        std::istringstream iss(line);
        std::string type;
        iss >> type;

        if (type == "mtllib" && mtlFilename.empty()) {
            // ★★★ GetDirectory 대신 직접 경로 계산 ★★★
            std::string mtlFile;
            iss >> mtlFile;

            size_t lastSlash = filename.find_last_of(L"/\\");
            std::wstring dir = (lastSlash != std::wstring::npos)
                ? filename.substr(0, lastSlash + 1)
                : L"";

            std::wstring fullMtlPath = dir + std::wstring(mtlFile.begin(), mtlFile.end());

            if (MtlLoader::Load(fullMtlPath, materials)) {
                mtlLoaded = true;
            }
        }
        else if (type == "usemtl") {
            iss >> currentMtlName;

            outSubMeshes.emplace_back();
            currentSubMesh = &outSubMeshes.back();
            currentSubMesh->material.name = currentMtlName;

            auto it = materials.find(currentMtlName);
            if (it != materials.end()) {
                currentSubMesh->material = it->second;
            }
        }
        else if (type == "v") {
            XMFLOAT3 pos;
            iss >> pos.x >> pos.y >> pos.z;
            // positions.push_back(pos);   ← 기존에 positions 벡터가 있었다면 유지
        }
        else if (type == "vt") {
            XMFLOAT2 uv;
            iss >> uv.x >> uv.y;
            // texCoords.push_back(uv);
        }
        else if (type == "vn") {
            XMFLOAT3 norm;
            iss >> norm.x >> norm.y >> norm.z;
            // normals.push_back(norm);
        }
        else if (type == "f" && currentSubMesh) {
            // ← 기존 face 파싱 로직 그대로 복사해서 넣으세요 (ParseFace 호출 등)
        }
    }

    // usemtl이 하나도 없던 단순 OBJ 처리
    if (outSubMeshes.empty()) {
        outSubMeshes.emplace_back();
    }

    file.close();
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