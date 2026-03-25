#include "PhysicsSystem.h"

void PhysicsSystem::Update(float deltaTime, Registry& registry) {
    // GravityComponent를 가진 모든 엔티티를 순회
    auto& gravityMap = Registry::GetComponentMap<GravityComponent>();
    auto& transformMap = Registry::GetComponentMap<TransformComponent>();

    for (auto& [entity, gravity] : gravityMap) {
        // 중력이 활성화된 경우에만 로직 수행
        if (gravity.isActive) {
            // 해당 엔티티의 Transform 데이터 가져오기
            if (transformMap.find(entity) != transformMap.end()) {
                auto& transform = transformMap[entity];

                // 가속도 계산: v = v0 + at
                transform.velocity.y += gravity.strength * deltaTime;
                // 위치 계산: p = p0 + vt
                transform.position.y += transform.velocity.y * deltaTime;

                // 바닥 충돌 처리 (간단한 예시)
                if (transform.position.y <= -5.0f) {
                    transform.position.y = -5.0f;
                    transform.velocity.y = 0.0f;
                    gravity.isActive = false; // 정지
                }
            }
        }
    }
}