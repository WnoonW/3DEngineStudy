#pragma once
#include "ECS.h"
#include "ResourceManager.h"
#include "AppUtill.h"
#include "Systems.h"

// 정점 구조체들
struct ObjectVertex {
    DirectX::XMFLOAT3 position;
    DirectX::XMFLOAT2 textcoord;
};

struct UIVertex {
    DirectX::XMFLOAT2 position;
    DirectX::XMFLOAT2 uv;
    DirectX::XMFLOAT4 color;
};

// [추가됨] 모든 큐브가 공유할 무거운 리소스들을 담는 구조체
struct SharedRenderResources {
    bool isInitialized = false;

    Microsoft::WRL::ComPtr<ID3D12Resource> vertexBuffer;
    Microsoft::WRL::ComPtr<ID3D12Resource> indexBuffer;
    D3D12_VERTEX_BUFFER_VIEW vertexBufferView = {};
    D3D12_INDEX_BUFFER_VIEW indexBufferView = {};
    UINT indexCount = 0;

    Microsoft::WRL::ComPtr<ID3D12Resource> texture;
    Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> pso;
};

struct SharedUIRenderResources {
    bool isInitialized = false;

    Microsoft::WRL::ComPtr<ID3D12Resource> vertexBuffer;
    Microsoft::WRL::ComPtr<ID3D12Resource> indexBuffer;
    D3D12_VERTEX_BUFFER_VIEW vertexBufferView = {};
    D3D12_INDEX_BUFFER_VIEW indexBufferView = {};
    UINT indexCount = 0;

    Microsoft::WRL::ComPtr<ID3D12Resource> texture;
    Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> pso;
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

        // =====================================================================
        // [최적화 1] 무거운 리소스들은 정적(static) 변수에 1회만 생성하여 저장
        // =====================================================================
        static SharedRenderResources s_Shared;

        if (!s_Shared.isInitialized) {
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
            s_Shared.indexCount = _countof(Indices);

            // 버퍼 생성, 텍스처 로드, 셰이더 컴파일을 딱 한 번만 수행!
            rm->CreateVertexBuffer(sizeof(ObjectVertex), _countof(Vertices), &s_Shared.vertexBufferView, s_Shared.vertexBuffer.GetAddressOf(), Vertices);
            rm->CreateIndexBuffer(_countof(Indices), &s_Shared.indexBufferView, s_Shared.indexBuffer.GetAddressOf(), Indices);

            D3D12_RESOURCE_DESC desc = {};
            rm->CreateTexture3(s_Shared.texture.GetAddressOf(), &desc, L"assets/textures/girl.dds");

            auto vs = AppUtill::CompileShader(L"Shaders\\color_v2.hlsl", nullptr, "VS", "vs_5_0");
            auto ps = AppUtill::CompileShader(L"Shaders\\color_v2.hlsl", nullptr, "PS", "ps_5_0");

            std::vector<D3D12_INPUT_ELEMENT_DESC> inputLayout = {
                { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
                { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
            };

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
            device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(s_Shared.rootSignature.GetAddressOf()));

            D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
            psoDesc.pRootSignature = s_Shared.rootSignature.Get();
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
            device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(s_Shared.pso.GetAddressOf()));

            s_Shared.isInitialized = true;
        }

        // 공유 리소스의 포인터들만 컴포넌트로 넘겨줌 (스마트 포인터의 참조 카운트만 증가하므로 비용 0)
        render.vertexBuffer = s_Shared.vertexBuffer;
        render.indexBuffer = s_Shared.indexBuffer;
        render.vertexBufferView = s_Shared.vertexBufferView;
        render.indexBufferView = s_Shared.indexBufferView;
        render.indexCount = s_Shared.indexCount;
        render.rootSignature = s_Shared.rootSignature;
        render.pso = s_Shared.pso;

        // [최적화 2] 큐브의 로컬 AABB는 어차피 고정이므로, 매번 정점을 순회하지 않고 상수로 세팅
        aabb.min = DirectX::XMFLOAT3(-1, -1, -1);
        aabb.max = DirectX::XMFLOAT3(1, 1, 1);

        // =====================================================================
        // [개별 생성] 각 큐브마다 반드시 독립적으로 가져야 하는 가벼운 리소스
        // =====================================================================
        render.descriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
        heapDesc.NumDescriptors = 2;
        heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(render.descHeap.GetAddressOf()));

        // 나만의 상수 버퍼 생성
        UINT elementByteSize = (sizeof(ObjectConstants) + 255) & ~255;
        rm->CreateConstantBuffer(elementByteSize, 1, render.constantBuffer.GetAddressOf());
        CD3DX12_RANGE readRange(0, 0);
        render.constantBuffer->Map(0, &readRange, reinterpret_cast<void**>(&render.pMappedConstantData));

        D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
        cbvDesc.BufferLocation = render.constantBuffer->GetGPUVirtualAddress();
        cbvDesc.SizeInBytes = elementByteSize;
        CD3DX12_CPU_DESCRIPTOR_HANDLE cbvHandle(render.descHeap->GetCPUDescriptorHandleForHeapStart(), 0, render.descriptorSize);
        device->CreateConstantBufferView(&cbvDesc, cbvHandle);

        // SRV (텍스처) - 텍스처를 새로 로드하지 않고 공유 텍스처(s_Shared.texture)의 뷰만 내 힙에 생성!
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format = s_Shared.texture->GetDesc().Format;
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MipLevels = s_Shared.texture->GetDesc().MipLevels;
        CD3DX12_CPU_DESCRIPTOR_HANDLE srvHandle(render.descHeap->GetCPUDescriptorHandleForHeapStart(), 1, render.descriptorSize);
        device->CreateShaderResourceView(s_Shared.texture.Get(), &srvDesc, srvHandle);

        // 3. 레지스트리에 컴포넌트 등록
        registry.AddComponent(entity, render);
        registry.AddComponent(entity, aabb);

        return entity;
    }
    
//=================================================================================================

    static Entity CreateUI(Registry& registry, ResourceManager* rm, DirectX::XMFLOAT2 pos, DirectX::XMFLOAT2 scale) {
        Entity entity = registry.CreateEntity();
        ID3D12Device14* device = rm->m_pDevice;

        // 1. Transform 설정
        UITransformComponent transform;
        transform.position = pos;
        transform.scale = scale;
        DirectX::XMStoreFloat4x4(&transform.worldMatrix, DirectX::XMMatrixTranslation(pos.x, pos.y, 0.0f));
        registry.AddComponent(entity, transform);

        // 2. Render & AABB 컴포넌트 세팅
        RenderComponent render;
        AABBComponent aabb;
        render.isUI = true; // [중요] 시스템에서 뷰 매트릭스(카메라)를 무시하도록 true로 설정

        static SharedUIRenderResources s_UI;

        if (!s_UI.isInitialized) {
            // =========================================================================
            // [수정 1] ObjectVertex 대신 UIVertex 사용 및 색상(Color) 데이터 추가
            // 셰이더의 float2 Position, float2 UV, float4 Color 규격에 맞춤
            // =========================================================================
            UIVertex Vertices[] = {
                // position(x,y), uv(x,y), color(r,g,b,a)
                { XMFLOAT2(-0.9f, -0.9f), XMFLOAT2(0, 1), XMFLOAT4(1, 0, 0, 0) }, //왼쪽아래
                { XMFLOAT2(-0.5f, -0.9f), XMFLOAT2(1, 1), XMFLOAT4(1, 0, 0, 0) },	//오른쪽 아래
                { XMFLOAT2(-0.5f, -0.5f), XMFLOAT2(1, 0), XMFLOAT4(1, 0, 0, 0) }, //오른쪽 위
                { XMFLOAT2(-0.9f, -0.5f), XMFLOAT2(0, 0), XMFLOAT4(1, 0, 0, 0) }, //왼쪽 위
            };

            WORD Indices[] = { 0, 1, 2, 0, 2, 3 };
            s_UI.indexCount = _countof(Indices);

            // [수정 2] sizeof(ObjectVertex) -> sizeof(UIVertex) 로 변경
            rm->CreateVertexBuffer(sizeof(UIVertex), _countof(Vertices), &s_UI.vertexBufferView, s_UI.vertexBuffer.GetAddressOf(), Vertices);
            rm->CreateIndexBuffer(_countof(Indices), &s_UI.indexBufferView, s_UI.indexBuffer.GetAddressOf(), Indices);

            // UI용 텍스처 로드
            D3D12_RESOURCE_DESC desc = {};
            rm->CreateTexture3(s_UI.texture.GetAddressOf(), &desc, L"assets/textures/girl.dds");

            auto vs = AppUtill::CompileShader(L"Shaders\\UI.hlsl", nullptr, "VS", "vs_5_0");
            auto ps = AppUtill::CompileShader(L"Shaders\\UI.hlsl", nullptr, "PS", "ps_5_0");

            // =========================================================================
            // [수정 3] UIVertex 구조체 레이아웃에 맞게 Input Layout 전면 수정
            // POSITION: 8바이트 (float2)
            // TEXCOORD: 8바이트 (float2) -> 오프셋 8
            // COLOR:   16바이트 (float4) -> 오프셋 16 (8 + 8)
            // =========================================================================
            std::vector<D3D12_INPUT_ELEMENT_DESC> inputLayout = {
                { "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
                { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 8, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
                { "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 16, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
            };

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
            device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(s_UI.rootSignature.GetAddressOf()));

            // UI 전용 PSO 설정
            D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
            psoDesc.pRootSignature = s_UI.rootSignature.Get();
            psoDesc.VS = { reinterpret_cast<BYTE*>(vs->GetBufferPointer()), vs->GetBufferSize() };
            psoDesc.PS = { reinterpret_cast<BYTE*>(ps->GetBufferPointer()), ps->GetBufferSize() };
            psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
            psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE; // UI는 양면 렌더링

            // 투명도(알파) 지원을 위한 블렌딩 활성화
            D3D12_BLEND_DESC blendDesc = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
            blendDesc.RenderTarget[0].BlendEnable = TRUE;
            blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
            blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
            blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
            blendDesc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
            blendDesc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
            blendDesc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
            blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
            psoDesc.BlendState = blendDesc;

            // UI는 화면 맨 앞에 그려지므로 Z-Depth 판정을 끕니다.
            psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
            psoDesc.DepthStencilState.DepthEnable = FALSE;

            psoDesc.SampleMask = UINT_MAX;
            psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
            psoDesc.NumRenderTargets = 1;
            psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
            psoDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
            psoDesc.SampleDesc.Count = 1;
            psoDesc.InputLayout = { inputLayout.data(), (UINT)inputLayout.size() };
            device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(s_UI.pso.GetAddressOf()));

            s_UI.isInitialized = true;
        }

        render.vertexBuffer = s_UI.vertexBuffer;
        render.indexBuffer = s_UI.indexBuffer;
        render.vertexBufferView = s_UI.vertexBufferView;
        render.indexBufferView = s_UI.indexBufferView;
        render.indexCount = s_UI.indexCount;
        render.rootSignature = s_UI.rootSignature;
        render.pso = s_UI.pso;

        // 평면 형태에 맞춘 AABB
        aabb.min = DirectX::XMFLOAT3(-1, -1, 0);
        aabb.max = DirectX::XMFLOAT3(1, 1, 0);

        // 개별 디스크립터 힙 및 상수 버퍼 (큐브와 동일)
        render.descriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
        heapDesc.NumDescriptors = 2;
        heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(render.descHeap.GetAddressOf()));

        // =========================================================================
        // 상수 버퍼 매핑 안내: 
        // 기존의 ObjectConstants (C++) 가 메모리 최상단에 XMFLOAT4X4 행렬을 가지고 있으므로,
        // UI.hlsl 의 cbuffer UIConstants { float4x4 OrthoProj; } 와 완벽하게 호환됩니다.
        // 따라서 C++ 단의 상수 버퍼 구조체는 기존 것을 그대로 재사용합니다.
        // =========================================================================
        UINT elementByteSize = (sizeof(ObjectConstants) + 255) & ~255;
        rm->CreateConstantBuffer(elementByteSize, 1, render.constantBuffer.GetAddressOf());
        CD3DX12_RANGE readRange(0, 0);
        render.constantBuffer->Map(0, &readRange, reinterpret_cast<void**>(&render.pMappedConstantData));

        D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
        cbvDesc.BufferLocation = render.constantBuffer->GetGPUVirtualAddress();
        cbvDesc.SizeInBytes = elementByteSize;
        CD3DX12_CPU_DESCRIPTOR_HANDLE cbvHandle(render.descHeap->GetCPUDescriptorHandleForHeapStart(), 0, render.descriptorSize);
        device->CreateConstantBufferView(&cbvDesc, cbvHandle);

        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format = s_UI.texture->GetDesc().Format;
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MipLevels = s_UI.texture->GetDesc().MipLevels;
        CD3DX12_CPU_DESCRIPTOR_HANDLE srvHandle(render.descHeap->GetCPUDescriptorHandleForHeapStart(), 1, render.descriptorSize);
        device->CreateShaderResourceView(s_UI.texture.Get(), &srvDesc, srvHandle);

        registry.AddComponent(entity, render);
        registry.AddComponent(entity, aabb);

        return entity;
    }
};