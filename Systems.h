#pragma once
#include "ECS.h"

// 상수 버퍼 구조체 (기존 Object.h 에 있던 것)
struct ObjectConstants {
    DirectX::XMFLOAT4X4 WorldViewProj = {};
    uint32_t isSelected = 0;
};

class TransformSystem {
public:
    // Transform 값을 바탕으로 World Matrix를 갱신
    static void Update(Registry& registry) {
        auto& transforms = registry.GetComponentMap<TransformComponent>();

        for (auto& [entity, transform] : transforms) {
            DirectX::XMVECTOR scale = DirectX::XMLoadFloat3(&transform.scale);
            // 필요에 따라 회전(Rotation) 쿼터니언 변환 추가 가능
            DirectX::XMVECTOR rot = DirectX::XMQuaternionIdentity();
            DirectX::XMVECTOR pos = DirectX::XMLoadFloat3(&transform.position);

            DirectX::XMMATRIX world = DirectX::XMMatrixAffineTransformation(scale, DirectX::XMVectorZero(), rot, pos);
            DirectX::XMStoreFloat4x4(&transform.worldMatrix, world);
        }
    }
};

class RenderSystem {
public:
    // 카메라 정보(View, Proj)를 받아 상수 버퍼를 업데이트
    static void UpdateConstants(Registry& registry, DirectX::XMMATRIX view, DirectX::XMMATRIX proj) {
        auto& transforms = registry.GetComponentMap<TransformComponent>();
        auto& renders = registry.GetComponentMap<RenderComponent>();

        for (auto& [entity, render] : renders) {
            // Transform이 없는 렌더 객체는 무시하거나 예외 처리
            if (transforms.find(entity) != transforms.end()) {
                auto& transform = transforms[entity];

                DirectX::XMMATRIX world = DirectX::XMLoadFloat4x4(&transform.worldMatrix);
                DirectX::XMMATRIX worldViewProj = render.isUI ? (world * proj) : (world * view * proj);

                ObjectConstants objConstants;
                objConstants.isSelected = render.isSelected ? 1 : 0;
                DirectX::XMStoreFloat4x4(&objConstants.WorldViewProj, DirectX::XMMatrixTranspose(worldViewProj));

                if (render.pMappedConstantData != nullptr) {
                    memcpy(render.pMappedConstantData, &objConstants, sizeof(ObjectConstants));
                }
            }
        }
    }

    // 커맨드 리스트를 사용해 실제 GPU에 그리기 명령 하달
    static void Render(Registry& registry, ID3D12GraphicsCommandList* commandList) {
        auto& renders = registry.GetComponentMap<RenderComponent>();

        for (auto& [entity, render] : renders) {
            commandList->SetGraphicsRootSignature(render.rootSignature.Get());
            commandList->SetPipelineState(render.pso.Get());
            commandList->IASetVertexBuffers(0, 1, &render.vertexBufferView);
            commandList->IASetIndexBuffer(&render.indexBufferView);
            commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

            ID3D12DescriptorHeap* pHeaps[] = { render.descHeap.Get() };
            commandList->SetDescriptorHeaps(_countof(pHeaps), pHeaps);

            CD3DX12_GPU_DESCRIPTOR_HANDLE cbvGpuHandle(render.descHeap->GetGPUDescriptorHandleForHeapStart(), 0, render.descriptorSize);
            commandList->SetGraphicsRootDescriptorTable(0, cbvGpuHandle);

            commandList->DrawIndexedInstanced(render.indexCount, 1, 0, 0, 0);
        }
    }
};