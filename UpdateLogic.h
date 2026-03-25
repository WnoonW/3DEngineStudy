#pragma once
#include "MathHelper.h"
#include "Object.h"

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

    Object* PickObject(const std::vector<std::unique_ptr<Object>>& objects, XMVECTOR rayOrigin, XMVECTOR rayDir);

	bool RayIntersectsAABB(const XMVECTOR& rayOrigin, const XMVECTOR& rayDir, const XMFLOAT3& aabbMin, const XMFLOAT3& aabbMax, float& tminOut, float& tmaxOut);
};
