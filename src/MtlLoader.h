#pragma once
#include "Material.h"
#include <unordered_map>
#include <string>
#include <vector>

class MtlLoader
{
public:
    static bool Load(const std::wstring& mtlFilePath,
        std::unordered_map<std::string, MaterialData>& outMaterials);

private:
    static void ParseLine(const std::string& line,
        std::unordered_map<std::string, MaterialData>& materials,
        MaterialData*& currentMaterial);
};