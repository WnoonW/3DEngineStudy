#include "ObjLoader.h"
#include <windows.h>
#include <fstream>
#include <sstream>
#include <vector>
#include <unordered_map>
#include <string>
#include <algorithm>

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

bool ObjLoader::Load(const std::string& filename, std::vector<Vertex>& outVertices, std::vector<WORD>& outIndices) {
    outVertices.clear();
    outIndices.clear();

    std::ifstream file(filename);
    if (!file.is_open()) {
        OutputDebugStringA("Failed to open OBJ file!\n");
        return false;
    }

    std::vector<DirectX::XMFLOAT3> positions;
    std::vector<DirectX::XMFLOAT3> normals;
    std::vector<DirectX::XMFLOAT2> texCoords;

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
        else if (type == "f") {
            std::vector<int> faceVertices;
            std::string token;
            while (iss >> token) {
                if (token.empty()) continue;

                std::replace(token.begin(), token.end(), '/', ' ');
                std::istringstream faceIss(token);

                int vIdx = 0, tIdx = 0, nIdx = 0;
                faceIss >> vIdx >> tIdx >> nIdx;

                VertexKey key{ vIdx, tIdx, nIdx };

                if (vertexMap.find(key) == vertexMap.end()) {
                    Vertex vertex{};
                    if (vIdx > 0 && vIdx <= (int)positions.size())
                        vertex.Position = positions[vIdx - 1];
                    if (nIdx > 0 && nIdx <= (int)normals.size())
                        vertex.Normal = normals[nIdx - 1];
                    if (tIdx > 0 && tIdx <= (int)texCoords.size())
                        vertex.TexCoord = texCoords[tIdx - 1];

                    WORD newIndex = (WORD)outVertices.size();
                    outVertices.push_back(vertex);
                    vertexMap[key] = newIndex;
                }

                outIndices.push_back(vertexMap[key]);
            }
        }
    }

    OutputDebugStringA(("OBJ Loaded: " + std::to_string(outVertices.size()) + " vertices, "
        + std::to_string(outIndices.size()) + " indices\n").c_str());

    return true;
}