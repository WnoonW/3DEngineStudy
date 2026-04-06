#pragma once
#include "ECS.h"
#include "ResourceManager.h"
#include "AppUtill.h"
#include "Systems.h"

// 기존에 Object.h에 있던 정점 구조체들
struct ObjectVertex {
    DirectX::XMFLOAT3 position;
    DirectX::XMFLOAT2 textcoord;
};

struct UIVertex {
    DirectX::XMFLOAT2 position;
    DirectX::XMFLOAT2 uv;
    DirectX::XMFLOAT4 color;
};

class EntityFactory {
public:
    static Entity CreateCube(Registry& registry, ResourceManager* rm, DirectX::XMFLOAT3 pos, DirectX::XMFLOAT3 scale) {
        Entity entity = registry.CreateEntity();
        ID3D12Device14* device = rm->m_pDevice;

        // 1. Transform 컴포넌트 세팅
        TransformComponent transform;
        transform.position = pos;
        transform.scale = scale;
        DirectX::XMStoreFloat4x4(&transform.worldMatrix, DirectX::XMMatrixTranslation(pos.x, pos.y, pos.z));
        registry.AddComponent(entity, transform);

        // 2. Render & AABB 컴포넌트 세팅
        RenderComponent render;
        AABBComponent aabb;
        render.isUI = false;

        // --- 정점 및 인덱스 데이터 ---
        ObjectVertex Vertices[] = {
            { DirectX::XMFLOAT3(-1, -1, +1), DirectX::XMFLOAT2(0, 1) }, { DirectX::XMFLOAT3(+1, -1, +1), DirectX::XMFLOAT2(1, 1) }, { DirectX::XMFLOAT3(+1, +1, +1), DirectX::XMFLOAT2(1, 0) }, { DirectX::XMFLOAT3(-1, +1, +1), DirectX::XMFLOAT2(0, 0) },
            { DirectX::XMFLOAT3(-1, -1, -1), DirectX::XMFLOAT2(0, 1) }, { DirectX::XMFLOAT3(-1, +1, -1), DirectX::XMFLOAT2(0, 0) }, { DirectX::XMFLOAT3(+1, +1, -1), DirectX::XMFLOAT2(1, 0) }, { DirectX::XMFLOAT3(+1, -1, -1), DirectX::XMFLOAT2(1, 1) },
            { DirectX::XMFLOAT3(+1, -1, -1), DirectX::XMFLOAT2(0, 1) }, { DirectX::XMFLOAT3(+1, -1, +1), DirectX::XMFLOAT2(1, 1) }, { DirectX::XMFLOAT3(+1, +1, +1), DirectX::XMFLOAT2(1, 0) }, { DirectX::XMFLOAT3(+1, +1, -1), DirectX::XMFLOAT2(0, 0) },
            { DirectX::XMFLOAT3(-1, -1, +1), DirectX::XMFLOAT2(0, 1) }, { DirectX::XMFLOAT3(-1, +1, +1), DirectX::XMFLOAT2(0, 0) }, { DirectX::XMFLOAT3(-1, +1, -1), DirectX::XMFLOAT2(1, 0) }, { DirectX::XMFLOAT3(-1, -1, -1), DirectX::XMFLOAT2(1, 1) },
            { DirectX::XMFLOAT3(-1, +1, -1), DirectX::XMFLOAT2(0, 1) }, { DirectX::XMFLOAT3(-1, +1, +1), DirectX::XMFLOAT2(0, 0) }, { DirectX::XMFLOAT3(+1, +1, +1), DirectX::XMFLOAT2(1, 0) }, { DirectX::XMFLOAT3(+1, +1, -1), DirectX::XMFLOAT2(1, 1) },
            { DirectX::XMFLOAT3(-1, -1, -1), DirectX::XMFLOAT2(0, 1) }, { DirectX::XMFLOAT3(+1, -1, -1), DirectX::XMFLOAT2(1, 1) }, { DirectX::XMFLOAT3(+1, -1, +1), DirectX::XMFLOAT2(1, 0) }, { DirectX::XMFLOAT3(-1, -1, +1), DirectX::XMFLOAT2(0, 0) }
        };

        WORD Indices[] = {
            0,1,2, 0,2,3, 4,5,6, 4,6,7, 8,9,10, 8,10,11, 12,13,14, 12,14,15, 16,17,18, 16,18,19, 20,21,22, 20,22,23
        };
        render.indexCount = _countof(Indices);

        // AABB 계산
        DirectX::XMFLOAT3 minPt(FLT_MAX, FLT_MAX, FLT_MAX);
        DirectX::XMFLOAT3 maxPt(-FLT_MAX, -FLT_MAX, -FLT_MAX);
        for (UINT i = 0; i < _countof(Vertices); ++i) {
            minPt.x = min(minPt.x, Vertices[i].position.x); minPt.y = min(minPt.y, Vertices[i].position.y); minPt.z = min(minPt.z, Vertices[i].position.z);
            maxPt.x = max(maxPt.x, Vertices[i].position.x); maxPt.y = max(maxPt.y, Vertices[i].position.y); maxPt.z = max(maxPt.z, Vertices[i].position.z);
        }
        aabb.min = minPt; aabb.max = maxPt;

        // 버퍼 생성 (ResourceManager 활용)
        Microsoft::WRL::ComPtr<ID3D12Resource> vBuffer, iBuffer;
        rm->CreateVertexBuffer(sizeof(ObjectVertex), _countof(Vertices), &render.vertexBufferView, vBuffer.GetAddressOf(), Vertices);
        rm->CreateIndexBuffer(_countof(Indices), &render.indexBufferView, iBuffer.GetAddressOf(), Indices);

        // --- 서술자 힙(Descriptor Heap) ---
        render.descriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
        heapDesc.NumDescriptors = 2;
        heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(render.descHeap.GetAddressOf()));

        // --- 상수 버퍼(Constant Buffer) ---
        Microsoft::WRL::ComPtr<ID3D12Resource> cBuffer;
        UINT elementByteSize = (sizeof(ObjectConstants) + 255) & ~255;
        rm->CreateConstantBuffer(elementByteSize, 1, cBuffer.GetAddressOf());
        CD3DX12_RANGE readRange(0, 0);
        cBuffer->Map(0, &readRange, reinterpret_cast<void**>(&render.pMappedConstantData));

        D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
        cbvDesc.BufferLocation = cBuffer->GetGPUVirtualAddress();
        cbvDesc.SizeInBytes = elementByteSize;
        CD3DX12_CPU_DESCRIPTOR_HANDLE cbvHandle(render.descHeap->GetCPUDescriptorHandleForHeapStart(), 0, render.descriptorSize);
        device->CreateConstantBufferView(&cbvDesc, cbvHandle);

        // --- SRV (텍스처) ---
        ID3D12Resource* pTexResource = nullptr;
        D3D12_RESOURCE_DESC desc = {};
        rm->CreateTexture3(&pTexResource, &desc, L"assets/textures/girl.dds");
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format = desc.Format;
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MipLevels = desc.MipLevels;
        CD3DX12_CPU_DESCRIPTOR_HANDLE srvHandle(render.descHeap->GetCPUDescriptorHandleForHeapStart(), 1, render.descriptorSize);
        device->CreateShaderResourceView(pTexResource, &srvDesc, srvHandle);

        // --- 셰이더 및 PSO ---
        auto vs = AppUtill::CompileShader(L"Shaders\\color_v2.hlsl", nullptr, "VS", "vs_5_0");
        auto ps = AppUtill::CompileShader(L"Shaders\\color_v2.hlsl", nullptr, "PS", "ps_5_0");

        std::vector<D3D12_INPUT_ELEMENT_DESC> inputLayout = {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
        };

        // 루트 시그니처 생성 (간소화)
        CD3DX12_DESCRIPTOR_RANGE ranges[2];
        ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0);
        ranges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);
        CD3DX12_ROOT_PARAMETER rootParams[1];
        rootParams[0].InitAsDescriptorTable(2, ranges, D3D12_SHADER_VISIBILITY_ALL);

        D3D12_STATIC_SAMPLER_DESC sampler = {};
        sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
        sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP; sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP; sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;

        CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(1, rootParams, 1, &sampler, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
        Microsoft::WRL::ComPtr<ID3DBlob> signature, error;
        D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error);
        device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(render.rootSignature.GetAddressOf()));

        // PSO 생성
        D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
        psoDesc.pRootSignature = render.rootSignature.Get();
        psoDesc.VS = { reinterpret_cast<BYTE*>(vs->GetBufferPointer()), vs->GetBufferSize() };
        psoDesc.PS = { reinterpret_cast<BYTE*>(ps->GetBufferPointer()), ps->GetBufferSize() };
        psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
        psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
        psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
        psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
        psoDesc.SampleMask = UINT_MAX;
        psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        psoDesc.NumRenderTargets = 1;
        psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
        psoDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
        psoDesc.SampleDesc.Count = 1;
        psoDesc.InputLayout = { inputLayout.data(), (UINT)inputLayout.size() };
        device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(render.pso.GetAddressOf()));

        registry.AddComponent(entity, render);
        registry.AddComponent(entity, aabb);

        // ※ 주의: 실제 엔진에서는 이 리소스들(버퍼 등)을 컴포넌트 생명주기와 연동하여 Release 해주는 소멸 시스템이 필요합니다.
        return entity;
    }
};