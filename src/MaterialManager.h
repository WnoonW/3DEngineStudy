#pragma once
#include "Material.h"
#include "..\ResourceManager.h"
#include <unordered_map>
#include <string>

class MaterialManager
{
public:
    static MaterialManager& GetInstance();

    // Cube용 Material (기존 SharedRenderResources 대체)
    Material* GetCubeMaterial(ResourceManager* rm);

    // UI용 Material
    Material* GetUIMaterial(ResourceManager* rm);

    // Mesh용 Material (OBJ용, color_v2.hlsl 기반)
    Material* GetMeshMaterial(ResourceManager* rm);

private:
    MaterialManager() = default;

    Material mCubeMaterial;
    Material mUIMaterial;
    Material mMeshMaterial;

    bool mCubeInitialized = false;
    bool mUIInitialized = false;
    bool mMeshInitialized = false;
};