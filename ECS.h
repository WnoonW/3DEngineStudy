#pragma once
#include <unordered_map>
#include <vector>
#include <typeindex>
#include <memory>
#include <DirectXMath.h>

// 1. 엔티티 정의 (단순한 ID)
using Entity = uint32_t;

// 2. 컴포넌트 정의 (데이터 구조체)
struct TransformComponent {
    DirectX::XMFLOAT3 position = { 0.0f, 0.0f, 0.0f };
    DirectX::XMFLOAT3 scale = { 1.0f, 1.0f, 1.0f };
    DirectX::XMFLOAT3 velocity = { 0.0f, 0.0f, 0.0f };
};

struct GravityComponent {
    float strength = -9.8f;
    bool isActive = false;
};

// 3. 레지스트리 (컴포넌트 저장소)
class Registry {
public:
    Entity CreateEntity() {
        return nextEntity++;
    }

    // 컴포넌트 추가/수정
    template<typename T>
    void AddComponent(Entity entity, T component) {
        GetComponentMap<T>()[entity] = component;
    }

    // 컴포넌트 가져오기
    template<typename T>
    T& GetComponent(Entity entity) {
        return GetComponentMap<T>()[entity];
    }

    // 특정 컴포넌트를 가진 모든 엔티티 맵 반환
    template<typename T>
    static std::unordered_map<Entity, T>& GetComponentMap() {
        static std::unordered_map<Entity, T> components;
        return components;
    }

private:
    Entity nextEntity = 0;
};