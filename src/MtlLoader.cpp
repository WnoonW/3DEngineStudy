#include "MtlLoader.h"
#include <fstream>
#include <sstream>
#include <iostream>

bool MtlLoader::Load(const std::wstring& mtlFilePath,
    std::unordered_map<std::string, MaterialData>& outMaterials)
{
    std::ifstream file(mtlFilePath);
    if (!file.is_open())
    {
        std::wcerr << L"[MtlLoader] Failed to open: " << mtlFilePath << std::endl;
        return false;
    }

    outMaterials.clear();
    MaterialData* currentMat = nullptr;

    std::string line;
    while (std::getline(file, line))
    {
        ParseLine(line, outMaterials, currentMat);
    }

    std::cout << "[MtlLoader] Loaded " << outMaterials.size() << " materials from "
        << std::string(mtlFilePath.begin(), mtlFilePath.end()) << std::endl;
    return true;
}

void MtlLoader::ParseLine(const std::string& line,
    std::unordered_map<std::string, MaterialData>& materials,
    MaterialData*& currentMaterial)
{
    std::istringstream iss(line);
    std::string prefix;
    iss >> prefix;

    if (prefix == "newmtl")
    {
        std::string name;
        iss >> name;
        currentMaterial = &materials[name];
        currentMaterial->name = name;
    }
    else if (prefix == "Ka" && currentMaterial)
    {
        iss >> currentMaterial->ambient.x >> currentMaterial->ambient.y >> currentMaterial->ambient.z;
    }
    else if (prefix == "Kd" && currentMaterial)
    {
        iss >> currentMaterial->diffuse.x >> currentMaterial->diffuse.y >> currentMaterial->diffuse.z;
    }
    else if (prefix == "Ks" && currentMaterial)
    {
        iss >> currentMaterial->specular.x >> currentMaterial->specular.y >> currentMaterial->specular.z;
    }
    else if (prefix == "Ns" && currentMaterial)
    {
        iss >> currentMaterial->shininess;
    }
    else if (prefix == "d" && currentMaterial)
    {
        iss >> currentMaterial->transparency;
    }
    else if (prefix == "map_Kd" && currentMaterial)
    {
        std::string tex;
        std::getline(iss, tex); // 파일명 전체 (공백 포함 가능)
        tex = tex.substr(tex.find_first_not_of(" \t"));
        currentMaterial->diffuseMap = std::wstring(tex.begin(), tex.end());
    }
    // map_Bump, map_normal 등 필요하면 나중에 추가
}