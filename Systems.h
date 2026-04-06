#pragma once
#include "ECS.h"

struct ObjectConstants {
    DirectX::XMFLOAT4X4 WorldViewProj = {};
    uint32_t isSelected = 0;
};

class TransformSystem {
public:
    static void Update(Registry& registry) {
        auto& transforms = registry.GetComponentMap<TransformComponent>();

        for (auto& [entity, transform] : transforms) {
            DirectX::XMVECTOR scale = DirectX::XMLoadFloat3(&transform.scale);
            DirectX::XMVECTOR rot = DirectX::XMQuaternionIdentity();
            DirectX::XMVECTOR pos = DirectX::XMLoadFloat3(&transform.position);

            DirectX::XMMATRIX world = DirectX::XMMatrixAffineTransformation(scale, DirectX::XMVectorZero(), rot, pos);
            DirectX::XMStoreFloat4x4(&transform.worldMatrix, world);
        }
    }
};

class RenderSystem {
public:
    // [수정] 매개변수에 화면 가로(screenWidth), 세로(screenHeight) 추가
    static void UpdateConstants(Registry& registry, DirectX::XMMATRIX view, DirectX::XMMATRIX proj, float screenWidth, float screenHeight) {
        auto& transforms = registry.GetComponentMap<TransformComponent>();
        auto& renders = registry.GetComponentMap<RenderComponent>();

        for (auto& [entity, render] : renders) {
            if (transforms.find(entity) != transforms.end()) {
                auto& transform = transforms[entity];

                DirectX::XMMATRIX world = DirectX::XMLoadFloat4x4(&transform.worldMatrix);
                DirectX::XMMATRIX worldViewProj;

                if (render.isUI) {
                    DirectX::XMMATRIX ortho = DirectX::XMMatrixOrthographicOffCenterLH(0.0f, screenWidth, screenHeight, 0.0f, -1.0f, 1.0f);
                    worldViewProj = world * ortho;
                }
                else {
                    worldViewProj = world * view * proj;
                }

                ObjectConstants objConstants;
                objConstants.isSelected = render.isSelected ? 1 : 0;
                DirectX::XMStoreFloat4x4(&objConstants.WorldViewProj, DirectX::XMMatrixTranspose(worldViewProj));

                if (render.pMappedConstantData != nullptr) {
                    memcpy(render.pMappedConstantData, &objConstants, sizeof(ObjectConstants));
                }
            }
        }
    }

    static void Render(Registry& registry, ID3D12GraphicsCommandList* commandList) {
        auto& renders = registry.GetComponentMap<RenderComponent>();

        // [수정 2] 렌더링 순서 보장: 3D 오브젝트 먼저, UI를 나중에 렌더링

        // 1단계: 일반 3D 오브젝트 렌더링
        for (auto& [entity, render] : renders) {
            if (render.isUI) continue; // UI는 건너뜀

            DrawRenderComponent(render, commandList);
        }

        // 2단계: UI 오브젝트 렌더링 (가장 위에 그려지도록 마지막에 실행)
        for (auto& [entity, render] : renders) {
            if (!render.isUI) continue; // 3D는 건너뜀

            DrawRenderComponent(render, commandList);
        }
    }

private:
    // 중복되는 그리기 명령어를 묶어둔 헬퍼 함수
    static void DrawRenderComponent(const RenderComponent& render, ID3D12GraphicsCommandList* commandList) {
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
};