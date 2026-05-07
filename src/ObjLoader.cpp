#include "ObjLoader.h"
#include <windows.h>
#include <fstream>
#include <sstream>
#include <algorithm>

bool ObjLoader::Load(const std::string& filename, std::vector<Vertex>& outVertices, std::vector<WORD>& outIndices) {
    outVertices.clear();
    outIndices.clear();

    std::ifstream file(filename);
    if (!file.is_open()) {
        return false;
    }

    std::vector<DirectX::XMFLOAT3> positions;
    std::vector<DirectX::XMFLOAT3> normals;
    std::vector<DirectX::XMFLOAT2> texCoords;

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
            DirectX::XMFLOAT3 normal{};
            iss >> normal.x >> normal.y >> normal.z;
            normals.push_back(normal);
        }
        else if (type == "vt") {
            DirectX::XMFLOAT2 tex{};
            iss >> tex.x >> tex.y;
            texCoords.push_back(tex);
        }
        else if (type == "f") {
            // Blender exported OBJ face parsing (v/t/n format)
            for (int i = 0; i < 3; ++i) {  // Assume triangulated
                std::string faceStr;
                iss >> faceStr;
                if (faceStr.empty()) continue;

                std::replace(faceStr.begin(), faceStr.end(), '/', ' ');
                std::istringstream faceIss(faceStr);

                int vIdx = 0, tIdx = 0, nIdx = 0;
                faceIss >> vIdx >> tIdx >> nIdx;

                Vertex vertex{};
                if (vIdx > 0 && vIdx <= positions.size()) vertex.Position = positions[vIdx - 1];
                if (nIdx > 0 && nIdx <= normals.size()) vertex.Normal = normals[nIdx - 1];
                if (tIdx > 0 && tIdx <= texCoords.size()) vertex.TexCoord = texCoords[tIdx - 1];

                outVertices.push_back(vertex);
                outIndices.push_back(static_cast<WORD>(outVertices.size() - 1));
            }
        }
    }
    return true;
}