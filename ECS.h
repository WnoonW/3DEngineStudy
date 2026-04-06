#pragma once
#include <unordered_map>
#include <typeindex>
#include <any>
#include <DirectXMath.h>
#include <d3d12.h>
#include <wrl.h>
#include "d3dx12.h"

using Entity = uint32_t;

// ----------------------------------------------------
// [ 컴포넌트(Component) 정의 ]
// ----------------------------------------------------

struct TransformComponent {
    DirectX::XMFLOAT3 position = { 0.0f, 0.0f, 0.0f };
    DirectX::XMFLOAT3 rotation = { 0.0f, 0.0f, 0.0f };
    DirectX::XMFLOAT3 scale = { 1.0f, 1.0f, 1.0f };
    DirectX::XMFLOAT3 velocity = { 0.0f, 0.0f, 0.0f };
    DirectX::XMFLOAT4X4 worldMatrix = {};
};

struct GravityComponent {
    float strength = -9.8f;
    bool isActive = true;
};

// 렌더링에 필요한 버퍼, 뷰, 루트 시그니처 등을 담는 데이터
struct RenderComponent {
    bool isUI = false;
    bool isSelected = false;
    UINT indexCount = 0;

    D3D12_VERTEX_BUFFER_VIEW vertexBufferView;
    D3D12_INDEX_BUFFER_VIEW indexBufferView;
    Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> pso;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> descHeap;
    UINT descriptorSize = 0;

    // 상수 버퍼 매핑 주소
    UINT8* pMappedConstantData = nullptr;
};

// 충돌 및 피킹(Picking)을 위한 바운딩 박스
struct AABBComponent {
    DirectX::XMFLOAT3 min;
    DirectX::XMFLOAT3 max;
};

// ----------------------------------------------------
// [ 레지스트리(Registry) - 기존 구현 유지/보완 ]
// ----------------------------------------------------

class Registry {
private:
    uint32_t nextEntityId = 0;
    // 타입에 따라 Entity -> Component 매핑을 저장
    std::unordered_map<std::type_index, std::any> components;

public:
    Entity CreateEntity() {
        return nextEntityId++;
    }

    template<typename T>
    void AddComponent(Entity entity, T component) {
        auto type = std::type_index(typeid(T));
        if (components.find(type) == components.end()) {
            components[type] = std::unordered_map<Entity, T>();
        }
        auto& componentMap = std::any_cast<std::unordered_map<Entity, T>&>(components[type]);
        componentMap[entity] = component;
    }

    template<typename T>
    T& GetComponent(Entity entity) {
        auto type = std::type_index(typeid(T));
        auto& componentMap = std::any_cast<std::unordered_map<Entity, T>&>(components[type]);
        return componentMap.at(entity);
    }

    template<typename T>
    std::unordered_map<Entity, T>& GetComponentMap() {
        auto type = std::type_index(typeid(T));
        if (components.find(type) == components.end()) {
            components[type] = std::unordered_map<Entity, T>();
        }
        return std::any_cast<std::unordered_map<Entity, T>&>(components[type]);
    }
};