#pragma once
#include "ECS.h"
#include "ResourceManager.h"
#include "AppUtill.h"
#include "Systems.h"
#include "src/ObjLoader.h"

// 정점 구조체들
struct ObjectVertex {
    DirectX::XMFLOAT3 position;
    DirectX::XMFLOAT2 textcoord;
};

struct UIVertex {
    DirectX::XMFLOAT2 position;
    DirectX::XMFLOAT2 uv;
    DirectX::XMFLOAT4 color;
};

struct MeshData {
	std::vector<Vertex> vertices;
	std::vector<WORD> indices;
};

class EntityFactory {
public:
    static Entity CreateCube(Registry& registry, ResourceManager* rm, DirectX::XMFLOAT3 pos, DirectX::XMFLOAT3 scale);
    static Entity CreateUI(Registry& registry, ResourceManager* rm, DirectX::XMFLOAT3 pos, DirectX::XMFLOAT3 scale);
    static Entity CreateMesh(const std::string& filename, Registry& registry, ResourceManager* rm, DirectX::XMFLOAT3 pos, DirectX::XMFLOAT3 scale);

private:
    // ==================== 중복 제거용 Helper ====================
    static void SetupTransformComponent(Registry& registry, Entity entity,
        DirectX::XMFLOAT3 pos, DirectX::XMFLOAT3 scale);

    static void SetupPerEntityRenderResources(RenderComponent& render, ResourceManager* rm,
        ID3D12Device14* device, ID3D12Resource* sharedTexture);
};


