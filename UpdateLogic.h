#pragma once
#include "MathHelper.h"
#include "ECS.h"

using namespace DirectX;

class logic
{
public:
    bool ScreenPointToWorldRay(
        int clientWidth, int clientHeight,
        int mouseX, int mouseY,
        XMFLOAT4X4& viewMatrix,
        XMFLOAT4X4& projMatrix,
        XMVECTOR& outRayOrigin,
        XMVECTOR& outRayDirection);

    // 반환값을 Object 포인터에서 Entity ID로 변경하고 Registry를 참조합니다.
    // 선택된 엔티티가 없으면 UINT32_MAX 를 반환한다고 가정합니다.
    Entity PickObject(Registry& registry, XMVECTOR rayOrigin, XMVECTOR rayDir);
    Entity PickUI(Registry& registry, int mouseX, int mouseY);

    bool RayIntersectsAABB(const XMVECTOR& rayOrigin, const XMVECTOR& rayDir, const XMFLOAT3& aabbMin, const XMFLOAT3& aabbMax, float& tminOut, float& tmaxOut);
};

