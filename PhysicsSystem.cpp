#include "PhysicsSystem.h"

void PhysicsSystem::Update(float deltaTime, Registry& registry) {
    auto& gravityMap = registry.GetComponentMap<GravityComponent>();
    auto& transformMap = registry.GetComponentMap<TransformComponent>();

    for (auto& [entity, gravity] : gravityMap) {
        if (gravity.isActive) {
            if (transformMap.find(entity) != transformMap.end()) {
                auto& transform = transformMap[entity];

                // 가속도 및 위치 계산
                transform.velocity.y += gravity.strength * deltaTime;
                transform.position.y += transform.velocity.y * deltaTime;

                // [수정됨] 중력이 아래(-방향)일 때만 바닥 충돌 체크
                if (gravity.strength < 0.0f && transform.position.y <= -5.0f) {
                    transform.position.y = -5.0f;
                    transform.velocity.y = 0.0f;
                    gravity.isActive = false; // 정지
                }

                // [수정됨] 중력이 위(+방향)일 때만 천장 충돌 체크
                if (gravity.strength > 0.0f && transform.position.y >= 5.0f) {
                    transform.position.y = 5.0f;
                    transform.velocity.y = 0.0f;
                    gravity.isActive = false; // 정지
                }
            }
        }
    }
}