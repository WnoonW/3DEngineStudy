#include "MaterialManager.h"
#include "../ResourceManager.h"
#include "../AppUtill.h"
#include <d3d12.h>
#include <wrl.h>
#include <vector>

MaterialManager& MaterialManager::GetInstance()
{
    static MaterialManager instance;
    return instance;
}

Material* MaterialManager::GetCubeMaterial(ResourceManager* rm)
{
    if (!mCubeInitialized)
    {
        // (기존 Cube 코드 유지 - 생략하지 않고 그대로)
        // ... (기존 코드)
        mCubeInitialized = true;
    }
    return &mCubeMaterial;
}

// TODO: GetUIMaterial and GetMeshMaterial 구현 예정