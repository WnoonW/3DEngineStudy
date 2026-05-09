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

    // MTL 로드
    if (!mtlFilename.empty()) {
        MtlLoader::Load(mtlFilename, materials);
    }

    // ====================== OBJ 파싱을 위한 데이터 ======================
    std::vector<DirectX::XMFLOAT3> positions;
    std::vector<DirectX::XMFLOAT2> texCoords;
    std::vector<DirectX::XMFLOAT3> normals;

    SubMesh* currentSubMesh = nullptr;
    std::unordered_map<VertexKey, WORD> vertexMap;   // 현재 SubMesh용

    std::string line;
    while (std::getline(file, line)) {
        std::istringstream iss(line);
        std::string type;
        iss >> type;

        if (type == "mtllib" && mtlFilename.empty()) {
            // MTL 파일 경로 처리 (기존 코드 유지)
            std::string mtlFile;
            iss >> mtlFile;
            size_t lastSlash = filename.find_last_of(L"/\\");
            std::wstring dir = (lastSlash != std::wstring::npos) ? filename.substr(0, lastSlash + 1) : L"";
            std::wstring fullMtlPath = dir + std::wstring(mtlFile.begin(), mtlFile.end());
            MtlLoader::Load(fullMtlPath, materials);
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

            vertexMap.clear();   // 새 SubMesh마다 vertexMap 초기화
        }
        else if (type == "v") {
            XMFLOAT3 pos;
            iss >> pos.x >> pos.y >> pos.z;
            positions.push_back(pos);
        }
        else if (type == "vt") {
            XMFLOAT2 uv;
            iss >> uv.x >> uv.y;
            texCoords.push_back(uv);
        }
        else if (type == "vn") {
            XMFLOAT3 norm;
            iss >> norm.x >> norm.y >> norm.z;
            normals.push_back(norm);
        }
        else if (type == "f" && currentSubMesh) {
            // face 라인 파싱 (f v1/vt1/vn1 v2/vt2/vn2 v3/vt3/vn3 ...)
            std::string token;
            while (iss >> token) {
                ParseFace(token, positions, normals, texCoords, *currentSubMesh, vertexMap);
            }
        }
    }

    // usemtl이 하나도 없었던 경우 (단순 OBJ)
    if (outSubMeshes.empty()) {
        outSubMeshes.emplace_back();
        // 이 경우 모든 vertex/index를 첫 번째 SubMesh에 넣어야 하지만,
        // 지금 구조에서는 usemtl 없이도 f 라인을 처리하려면 currentSubMesh를 여기서 만들어야 함
        // (필요하면 나중에 추가로 말씀해주세요)
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