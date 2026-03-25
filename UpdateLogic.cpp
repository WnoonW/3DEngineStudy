#include "UpdateLogic.h"

bool logic::ScreenPointToWorldRay(
    int clientWidth, int clientHeight,
    int mouseX, int mouseY,
    XMFLOAT4X4& viewMatrix,
    XMFLOAT4X4& projMatrix,
    XMVECTOR& outRayOrigin,
    XMVECTOR& outRayDirection)
{
    // 1. Viewport 공간으로 변환 (0~1 범위)
    float viewportX = static_cast<float>(mouseX) / clientWidth;
    float viewportY = static_cast<float>(mouseY) / clientHeight;

    // 2. NDC 좌표 (-1 ~ +1)
    float ndcX = viewportX * 2.0f - 1.0f;
    float ndcY = 1.0f - viewportY * 2.0f;   // Y축이 위쪽이 +1

    // 3. 근평면(Near plane)에서의 3D 점 (z=0)
    XMVECTOR nearPoint = XMVectorSet(ndcX, ndcY, 0.0f, 1.0f);

    // 4. 원근 투영 역행렬 → View 공간으로
    XMMATRIX invProj = XMMatrixInverse(nullptr, XMLoadFloat4x4(&projMatrix));

    // 5. View 공간 → World 공간으로
    XMMATRIX invView = XMMatrixInverse(nullptr, XMLoadFloat4x4(&viewMatrix));

    // 근평면 점을 월드 공간으로 변환
    XMVECTOR nearWorld = XMVector3TransformCoord(nearPoint, invProj);
    nearWorld = XMVector3TransformCoord(nearWorld, invView);

    // 카메라(눈) 위치 = view 행렬의 역행렬에서 추출
    XMVECTOR eyePos = XMVector3TransformCoord(XMVectorSet(0, 0, 0, 1), invView);

    // 방향 벡터 = nearWorld - eyePos
    outRayOrigin = eyePos;
    outRayDirection = XMVector3Normalize(XMVectorSubtract(nearWorld, eyePos));

    return true;
}

Object* logic::PickObject(const std::vector<std::unique_ptr<Object>>& objects, XMVECTOR rayOrigin, XMVECTOR rayDir)
{
    Object* closest = nullptr;
    float minDistance = FLT_MAX;

    for (auto& obj : objects)
    {
        if (!obj) continue;
        

        AABB aabb = obj->GetWorldAABB();

        // 레이-AABB 충돌 검사
        float tmin, tmax;
        if (RayIntersectsAABB(rayOrigin, rayDir, aabb.min, aabb.max, tmin, tmax))
        {
            if (tmin > 0.0f && tmin < minDistance)
            {
                char buf[256];
                sprintf_s(buf, sizeof(buf),
                    "AABB: min(%.3f, %.3f, %.3f)  max(%.3f, %.3f, %.3f)\n",
                    aabb.min.x, aabb.min.y, aabb.min.z,
                    aabb.max.x, aabb.max.y, aabb.max.z);
                OutputDebugStringA(buf);
                minDistance = tmin;
                closest = obj.get();
            }
        }
    }

    return closest;
}

bool logic::RayIntersectsAABB(const XMVECTOR& rayOrigin, const XMVECTOR& rayDir, const XMFLOAT3& aabbMin, const XMFLOAT3& aabbMax, float& tminOut, float& tmaxOut)
{
    // 0 나누기 방지용 아주 작은 epsilon
    XMVECTOR epsilon = XMVectorReplicate(1e-20f);
    XMVECTOR invDir = XMVectorDivide(
        XMVectorSet(1.0f, 1.0f, 1.0f, 0.0f),
        XMVectorAdd(rayDir, epsilon));

	XMVECTOR boxMin = XMLoadFloat3(&aabbMin);
	XMVECTOR boxMax = XMLoadFloat3(&aabbMax);

    XMVECTOR tNear = XMVectorMultiply(XMVectorSubtract(boxMin, rayOrigin), invDir);
    XMVECTOR tFar = XMVectorMultiply(XMVectorSubtract(boxMax, rayOrigin), invDir);

    XMVECTOR t1 = XMVectorMin(tNear, tFar);   // 각 축의 near
    XMVECTOR t2 = XMVectorMax(tNear, tFar);   // 각 축의 far

    float txmin = XMVectorGetX(t1), tymin = XMVectorGetY(t1), tzmin = XMVectorGetZ(t1);
    float txmax = XMVectorGetX(t2), tymax = XMVectorGetY(t2), tzmax = XMVectorGetZ(t2);

    tminOut = max(txmin, max(tymin, tzmin));
    tmaxOut = min(txmax, min(tymax, tzmax));

    // 유효 조건
    if (tminOut > tmaxOut) return false;
    if (tmaxOut < 0.0f)    return false;

    return true;
}
