#include "EntityFactory.h"
#include "src/ObjLoader.h"     
#include <vector>

void EntityFactory::SetupTransformComponent(Registry& registry, Entity entity,
    DirectX::XMFLOAT3 pos, DirectX::XMFLOAT3 scale)
{
    TransformComponent transform;
    transform.position = pos;
    transform.scale = scale;
    DirectX::XMStoreFloat4x4(&transform.worldMatrix, DirectX::XMMatrixTranslation(pos.x, pos.y, pos.z));
    registry.AddComponent(entity, transform);
}

void EntityFactory::SetupPerEntityRenderResources(RenderComponent& render, ResourceManager* rm,
    ID3D12Device14* device, ID3D12Resource* sharedTexture)
{
    render.descriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    // Descriptor Heap
    D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
    heapDesc.NumDescriptors = 2;
    heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(render.descHeap.GetAddressOf()));

    // Constant Buffer
    UINT elementByteSize = (sizeof(ObjectConstants) + 255) & ~255;
    rm->CreateConstantBuffer(elementByteSize, 1, render.constantBuffer.GetAddressOf());
    CD3DX12_RANGE readRange(0, 0);
    render.constantBuffer->Map(0, &readRange, reinterpret_cast<void**>(&render.pMappedConstantData));

    // CBV
    D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
    cbvDesc.BufferLocation = render.constantBuffer->GetGPUVirtualAddress();
    cbvDesc.SizeInBytes = elementByteSize;
    CD3DX12_CPU_DESCRIPTOR_HANDLE cbvHandle(render.descHeap->GetCPUDescriptorHandleForHeapStart(), 0, render.descriptorSize);
    device->CreateConstantBufferView(&cbvDesc, cbvHandle);

    // SRV (공유 텍스처 사용)
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = sharedTexture->GetDesc().Format;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = sharedTexture->GetDesc().MipLevels;
    CD3DX12_CPU_DESCRIPTOR_HANDLE srvHandle(render.descHeap->GetCPUDescriptorHandleForHeapStart(), 1, render.descriptorSize);
    device->CreateShaderResourceView(sharedTexture, &srvDesc, srvHandle);
}

// ====================== CreateCube ======================
Entity EntityFactory::CreateCube(Registry& registry, ResourceManager* rm, DirectX::XMFLOAT3 pos, DirectX::XMFLOAT3 scale)
{
    Entity entity = registry.CreateEntity();
    ID3D12Device14* device = rm->m_pDevice;

    SetupTransformComponent(registry, entity, pos, scale);

    RenderComponent render;
    AABBComponent aabb;
    render.isUI = false;

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

    // 공유 리소스 연결
    render.vertexBuffer = s_Shared.vertexBuffer;
    render.indexBuffer = s_Shared.indexBuffer;
    render.vertexBufferView = s_Shared.vertexBufferView;
    render.indexBufferView = s_Shared.indexBufferView;
    render.indexCount = s_Shared.indexCount;
    render.rootSignature = s_Shared.rootSignature;
    render.pso = s_Shared.pso;

    aabb.min = DirectX::XMFLOAT3(-1, -1, -1);
    aabb.max = DirectX::XMFLOAT3(1, 1, 1);

    SetupPerEntityRenderResources(render, rm, device, s_Shared.texture.Get());

    registry.AddComponent(entity, render);
    registry.AddComponent(entity, aabb);
    return entity;
}

// ====================== CreateUI ======================
Entity EntityFactory::CreateUI(Registry& registry, ResourceManager* rm, DirectX::XMFLOAT3 pos, DirectX::XMFLOAT3 scale)
{
    Entity entity = registry.CreateEntity();
    ID3D12Device14* device = rm->m_pDevice;

    SetupTransformComponent(registry, entity, pos, scale);

    RenderComponent render;
    AABBComponent aabb;
    render.isUI = true;

    static SharedUIRenderResources s_UI;
    if (!s_UI.isInitialized) {
        // =========================================================================
        // [수정 1] ObjectVertex 대신 UIVertex 사용 및 색상(Color) 데이터 추가
        // 셰이더의 float2 Position, float2 UV, float4 Color 규격에 맞춤
        // =========================================================================
        UIVertex Vertices[] = {
            { DirectX::XMFLOAT2(0.0f, 1.0f), DirectX::XMFLOAT2(0.0f, 1.0f), DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f) }, // 좌하단
            { DirectX::XMFLOAT2(0.0f, 0.0f), DirectX::XMFLOAT2(0.0f, 0.0f), DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f) }, // 좌상단
            { DirectX::XMFLOAT2(1.0f, 0.0f), DirectX::XMFLOAT2(1.0f, 0.0f), DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f) }, // 우상단
            { DirectX::XMFLOAT2(1.0f, 1.0f), DirectX::XMFLOAT2(1.0f, 1.0f), DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f) }  // 우하단
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

    aabb.min = DirectX::XMFLOAT3(-1, -1, 0);
    aabb.max = DirectX::XMFLOAT3(1, 1, 0);

    SetupPerEntityRenderResources(render, rm, device, s_UI.texture.Get());

    registry.AddComponent(entity, render);
    registry.AddComponent(entity, aabb);
    return entity;
}

// ====================== CreateMesh (OBJ) ======================
Entity EntityFactory::CreateMesh(const std::string& filename, Registry& registry, ResourceManager* rm,
    DirectX::XMFLOAT3 pos, DirectX::XMFLOAT3 scale)
{
    Entity entity = registry.CreateEntity();
    ID3D12Device14* device = rm->m_pDevice;

    SetupTransformComponent(registry, entity, pos, scale);

    RenderComponent render;
    AABBComponent aabb;
    render.isUI = false;

    // 1. OBJ 파일 파싱
    MeshData meshData;
    if (!ObjLoader::Load(filename, meshData.vertices, meshData.indices)) 
    {
        return CreateCube(registry, rm, pos, scale);
    }

    // 2. Vertex / Index Buffer 생성 (매번 새로 생성)
    rm->CreateVertexBuffer(sizeof(Vertex), (DWORD)meshData.vertices.size(),
        &render.vertexBufferView, render.vertexBuffer.GetAddressOf(), meshData.vertices.data());

    rm->CreateIndexBuffer((DWORD)meshData.indices.size(), &render.indexBufferView,
        render.indexBuffer.GetAddressOf(), meshData.indices.data());

    render.indexCount = (UINT)meshData.indices.size();

    // 3. Cube와 동일한 공유 PSO / RootSignature / Texture 사용 (나중에 Material 시스템으로 확장 가능)
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

        rm->CreateVertexBuffer(sizeof(Vertex), _countof(Vertices), &s_Shared.vertexBufferView, s_Shared.vertexBuffer.GetAddressOf(), Vertices);
        rm->CreateIndexBuffer(_countof(Indices), &s_Shared.indexBufferView, s_Shared.indexBuffer.GetAddressOf(), Indices);

        D3D12_RESOURCE_DESC desc = {};
        rm->CreateTexture3(s_Shared.texture.GetAddressOf(), &desc, L"assets/textures/girl.dds");

        auto vs = AppUtill::CompileShader(L"Shaders\\color_v2.hlsl", nullptr, "VS", "vs_5_0");
        auto ps = AppUtill::CompileShader(L"Shaders\\color_v2.hlsl", nullptr, "PS", "ps_5_0");

        std::vector<D3D12_INPUT_ELEMENT_DESC> inputLayout = {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
        };

        // RootSignature + PSO 생성 (Cube와 동일 코드 그대로 복사)
        CD3DX12_DESCRIPTOR_RANGE ranges[2];
        ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0);
        ranges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);
        CD3DX12_ROOT_PARAMETER rootParams[1];
        rootParams[0].InitAsDescriptorTable(2, ranges, D3D12_SHADER_VISIBILITY_ALL);

        D3D12_STATIC_SAMPLER_DESC sampler = {};
        sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
        sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;

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

    render.rootSignature = s_Shared.rootSignature;
    render.pso = s_Shared.pso;

    // AABB (OBJ 파일에서 계산하거나 임시로 설정)
    aabb.min = DirectX::XMFLOAT3(-1, -1, -1);  // 필요하면 ObjLoader에서 min/max 계산하도록 확장 가능
    aabb.max = DirectX::XMFLOAT3(1, 1, 1);

    SetupPerEntityRenderResources(render, rm, device, s_Shared.texture.Get());

    registry.AddComponent(entity, render);
    registry.AddComponent(entity, aabb);

    return entity;
}