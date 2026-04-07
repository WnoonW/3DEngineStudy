#include "UpdateLogic.h"

bool logic::ScreenPointToWorldRay(
    int clientWidth, int clientHeight,
    int mouseX, int mouseY,
    XMFLOAT4X4& viewMatrix,
    XMFLOAT4X4& projMatrix,
    XMVECTOR& outRayOrigin,
    XMVECTOR& outRayDirection)
{
    float viewportX = static_cast<float>(mouseX) / clientWidth;
    float viewportY = static_cast<float>(mouseY) / clientHeight;

    float ndcX = viewportX * 2.0f - 1.0f;
    float ndcY = 1.0f - viewportY * 2.0f;

    XMVECTOR nearPoint = XMVectorSet(ndcX, ndcY, 0.0f, 1.0f);
    XMMATRIX invProj = XMMatrixInverse(nullptr, XMLoadFloat4x4(&projMatrix));
    XMMATRIX invView = XMMatrixInverse(nullptr, XMLoadFloat4x4(&viewMatrix));

    XMVECTOR nearWorld = XMVector3TransformCoord(nearPoint, invProj);
    nearWorld = XMVector3TransformCoord(nearWorld, invView);

    XMVECTOR eyePos = XMVector3TransformCoord(XMVectorSet(0, 0, 0, 1), invView);

    outRayOrigin = eyePos;
    outRayDirection = XMVector3Normalize(XMVectorSubtract(nearWorld, eyePos));

    return true;
}

Entity logic::PickObject(Registry& registry, XMVECTOR rayOrigin, XMVECTOR rayDir)
{
    Entity closestEntity = UINT32_MAX; // 유효하지 않은 엔티티 ID
    float minDistance = FLT_MAX;

    // AABB와 Transform 컴포넌트 맵을 가져옴
    auto& aabbs = registry.GetComponentMap<AABBComponent>();
    auto& transforms = registry.GetComponentMap<TransformComponent>();

    for (auto& [entity, aabb] : aabbs)
    {
        // Transform이 있는 오브젝트만 피킹 처리
        if (transforms.find(entity) == transforms.end()) continue;
        auto& transform = transforms[entity];

        // 로컬 AABB에서 월드 AABB 계산 로직
        XMVECTOR centerLocal = XMVectorScale(XMVectorAdd(XMLoadFloat3(&aabb.min), XMLoadFloat3(&aabb.max)), 0.5f);
        XMVECTOR extentLocal = XMVectorScale(XMVectorSubtract(XMLoadFloat3(&aabb.max), XMLoadFloat3(&aabb.min)), 0.5f);

        XMVECTOR centerWorld = XMVectorAdd(centerLocal, XMLoadFloat3(&transform.position));
        XMVECTOR extentWorld = XMVectorMultiply(extentLocal, XMLoadFloat3(&transform.scale));

        XMFLOAT3 worldMin, worldMax;
        XMStoreFloat3(&worldMin, XMVectorSubtract(centerWorld, extentWorld));
        XMStoreFloat3(&worldMax, XMVectorAdd(centerWorld, extentWorld));

        // 레이-AABB 충돌 검사
        float tmin, tmax;
        if (RayIntersectsAABB(rayOrigin, rayDir, worldMin, worldMax, tmin, tmax))
        {
            if (tmin > 0.0f && tmin < minDistance)
            {
                char buf[256];
                sprintf_s(buf, sizeof(buf),
                    "AABB Hit! EntityID: %d | min(%.3f, %.3f, %.3f)  max(%.3f, %.3f, %.3f)\n",
                    entity, worldMin.x, worldMin.y, worldMin.z, worldMax.x, worldMax.y, worldMax.z);
                OutputDebugStringA(buf);

                minDistance = tmin;
                closestEntity = entity;
            }
        }
    }

    return closestEntity;
}

Entity logic::PickUI(Registry& registry, int mouseX, int mouseY)
{
    auto& renders = registry.GetComponentMap<RenderComponent>();
    auto& transforms = registry.GetComponentMap<TransformComponent>();
    auto& aabbs = registry.GetComponentMap<AABBComponent>();

    for (auto& [entity, render] : renders)
    {
        // 1. UI 컴포넌트인지 확인
        if (!render.isUI) continue;
        if (transforms.find(entity) == transforms.end() || aabbs.find(entity) == aabbs.end()) continue;

        auto& transform = transforms[entity];
        auto& aabb = aabbs[entity];

        // 2. UI의 화면상 영역 계산 (단순화된 예시)
        // Transform의 position과 scale을 pixel 단위로 가정
        float left = transform.position.x + aabb.min.x * transform.scale.x;
        float right = transform.position.x + aabb.max.x * transform.scale.x;
        float top = transform.position.y + aabb.min.y * transform.scale.y;
        float bottom = transform.position.y + aabb.max.y * transform.scale.y;

        // 3. 마우스 좌표가 UI 사각형 안에 있는지 검사
        if (mouseX >= left && mouseX <= right && mouseY >= top && mouseY <= bottom)
        {
            return entity;
        }
    }
    return UINT32_MAX;
}

bool logic::RayIntersectsAABB(const XMVECTOR& rayOrigin, const XMVECTOR& rayDir, const XMFLOAT3& aabbMin, const XMFLOAT3& aabbMax, float& tminOut, float& tmaxOut)
{
    XMVECTOR epsilon = XMVectorReplicate(1e-20f);
    XMVECTOR invDir = XMVectorDivide(XMVectorSet(1.0f, 1.0f, 1.0f, 0.0f), XMVectorAdd(rayDir, epsilon));

    XMVECTOR boxMin = XMLoadFloat3(&aabbMin);
    XMVECTOR boxMax = XMLoadFloat3(&aabbMax);

    XMVECTOR tNear = XMVectorMultiply(XMVectorSubtract(boxMin, rayOrigin), invDir);
    XMVECTOR tFar = XMVectorMultiply(XMVectorSubtract(boxMax, rayOrigin), invDir);

    XMVECTOR t1 = XMVectorMin(tNear, tFar);
    XMVECTOR t2 = XMVectorMax(tNear, tFar);

    float txmin = XMVectorGetX(t1), tymin = XMVectorGetY(t1), tzmin = XMVectorGetZ(t1);
    float txmax = XMVectorGetX(t2), tymax = XMVectorGetY(t2), tzmax = XMVectorGetZ(t2);

    tminOut = max(txmin, max(tymin, tzmin));
    tmaxOut = min(txmax, min(tymax, tzmax));

    if (tminOut > tmaxOut) return false;
    if (tmaxOut < 0.0f)    return false;

    return true;
}