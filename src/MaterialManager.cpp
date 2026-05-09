#include "MaterialManager.h"
#include "../EntityFactory.h"
#include "../ResourceManager.h"
#include "../AppUtill.h"
#include <d3d12.h>
#include <wrl.h>

MaterialManager& MaterialManager::GetInstance()
{
    static MaterialManager instance;
    return instance;
}

Material* MaterialManager::GetCubeMaterial(ResourceManager* rm)
{
    if (!mCubeInitialized)
    {
        ID3D12Device14* device = rm->m_pDevice;

        // Vertex / Index Buffer (Cube)
        ObjectVertex cubeVertices[] = { 
            { DirectX::XMFLOAT3(-1, -1, +1), DirectX::XMFLOAT2(0, 1) }, { DirectX::XMFLOAT3(+1, -1, +1), DirectX::XMFLOAT2(1, 1) }, { DirectX::XMFLOAT3(+1, +1, +1), DirectX::XMFLOAT2(1, 0) }, { DirectX::XMFLOAT3(-1, +1, +1), DirectX::XMFLOAT2(0, 0) },
            { DirectX::XMFLOAT3(-1, -1, -1), DirectX::XMFLOAT2(0, 1) }, { DirectX::XMFLOAT3(-1, +1, -1), DirectX::XMFLOAT2(0, 0) }, { DirectX::XMFLOAT3(+1, +1, -1), DirectX::XMFLOAT2(1, 0) }, { DirectX::XMFLOAT3(+1, -1, -1), DirectX::XMFLOAT2(1, 1) },
            { DirectX::XMFLOAT3(+1, -1, -1), DirectX::XMFLOAT2(0, 1) }, { DirectX::XMFLOAT3(+1, -1, +1), DirectX::XMFLOAT2(1, 1) }, { DirectX::XMFLOAT3(+1, +1, +1), DirectX::XMFLOAT2(1, 0) }, { DirectX::XMFLOAT3(+1, +1, -1), DirectX::XMFLOAT2(0, 0) },
            { DirectX::XMFLOAT3(-1, -1, +1), DirectX::XMFLOAT2(0, 1) }, { DirectX::XMFLOAT3(-1, +1, +1), DirectX::XMFLOAT2(0, 0) }, { DirectX::XMFLOAT3(-1, +1, -1), DirectX::XMFLOAT2(1, 0) }, { DirectX::XMFLOAT3(-1, -1, -1), DirectX::XMFLOAT2(1, 1) },
            { DirectX::XMFLOAT3(-1, +1, -1), DirectX::XMFLOAT2(0, 1) }, { DirectX::XMFLOAT3(-1, +1, +1), DirectX::XMFLOAT2(0, 0) }, { DirectX::XMFLOAT3(+1, +1, +1), DirectX::XMFLOAT2(1, 0) }, { DirectX::XMFLOAT3(+1, +1, -1), DirectX::XMFLOAT2(1, 1) },
            { DirectX::XMFLOAT3(-1, -1, -1), DirectX::XMFLOAT2(0, 1) }, { DirectX::XMFLOAT3(+1, -1, -1), DirectX::XMFLOAT2(1, 1) }, { DirectX::XMFLOAT3(+1, -1, +1), DirectX::XMFLOAT2(1, 0) }, { DirectX::XMFLOAT3(-1, -1, +1), DirectX::XMFLOAT2(0, 0) }
        };
        WORD cubeIndices[] = { 0,1,2, 0,2,3, 4,5,6, 4,6,7, 8,9,10, 8,10,11, 12,13,14, 12,14,15, 16,17,18, 16,18,19, 20,21,22, 20,22,23 };

        rm->CreateVertexBuffer(sizeof(ObjectVertex), _countof(cubeVertices),
            &mCubeMaterial.data.vertexBufferView, mCubeMaterial.data.vertexBuffer.GetAddressOf(), cubeVertices);
        rm->CreateIndexBuffer(_countof(cubeIndices),
            &mCubeMaterial.data.indexBufferView, mCubeMaterial.data.indexBuffer.GetAddressOf(), cubeIndices);

        mCubeMaterial.data.indexCount = _countof(cubeIndices);

        // Texture
        D3D12_RESOURCE_DESC desc = {};
        rm->CreateTexture3(mCubeMaterial.data.texture.GetAddressOf(), &desc, L"assets/textures/girl.dds");

        // Shader
        auto vs = AppUtill::CompileShader(L"Shaders/color_v2.hlsl", nullptr, "VS", "vs_5_0");
        auto ps = AppUtill::CompileShader(L"Shaders/color_v2.hlsl", nullptr, "PS", "ps_5_0");

        // Input Layout
        std::vector<D3D12_INPUT_ELEMENT_DESC> inputLayout = {
            {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
            {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
        };

        // Root Signature + PSO (기존 코드 그대로 이동)
        CD3DX12_DESCRIPTOR_RANGE ranges[2];
        ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0);
        ranges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);
        CD3DX12_ROOT_PARAMETER rootParams[1];
        rootParams[0].InitAsDescriptorTable(_countof(ranges), ranges, D3D12_SHADER_VISIBILITY_ALL);

        D3D12_STATIC_SAMPLER_DESC sampler = {};
        sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
        sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;

        CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(1, rootParams, 1, &sampler, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
        Microsoft::WRL::ComPtr<ID3DBlob> signature, error;
        D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error);
        device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&mCubeMaterial.data.rootSignature));

        D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
        psoDesc.pRootSignature = mCubeMaterial.data.rootSignature.Get();
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
        device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&mCubeMaterial.data.pso));

        mCubeInitialized = true;
    }
    return &mCubeMaterial;
}

Material* MaterialManager::GetUIMaterial(ResourceManager* rm)
{
    if (!mUIInitialized) {

        ID3D12Device14* device = rm->m_pDevice;

        UIVertex Vertices[] = {
            { DirectX::XMFLOAT2(0.0f, 1.0f), DirectX::XMFLOAT2(0.0f, 1.0f), DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f) }, // 좌하단
            { DirectX::XMFLOAT2(0.0f, 0.0f), DirectX::XMFLOAT2(0.0f, 0.0f), DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f) }, // 좌상단
            { DirectX::XMFLOAT2(1.0f, 0.0f), DirectX::XMFLOAT2(1.0f, 0.0f), DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f) }, // 우상단
            { DirectX::XMFLOAT2(1.0f, 1.0f), DirectX::XMFLOAT2(1.0f, 1.0f), DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f) }  // 우하단
        };

        WORD Indices[] = { 0, 1, 2, 0, 2, 3 };
        mUIMaterial.data.indexCount = _countof(Indices);

        // [수정 2] sizeof(ObjectVertex) -> sizeof(UIVertex) 로 변경
        rm->CreateVertexBuffer(sizeof(UIVertex), _countof(Vertices), &mUIMaterial.data.vertexBufferView, mUIMaterial.data.vertexBuffer.GetAddressOf(), Vertices);
        rm->CreateIndexBuffer(_countof(Indices), &mUIMaterial.data.indexBufferView, mUIMaterial.data.indexBuffer.GetAddressOf(), Indices);
        // UI용 텍스처 로드
        D3D12_RESOURCE_DESC desc = {};
        rm->CreateTexture3(mUIMaterial.data.texture.GetAddressOf(), &desc, L"assets/textures/girl.dds");

        auto vs = AppUtill::CompileShader(L"Shaders\\UI.hlsl", nullptr, "VS", "vs_5_0");
        auto ps = AppUtill::CompileShader(L"Shaders\\UI.hlsl", nullptr, "PS", "ps_5_0");

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
        device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(mUIMaterial.data.rootSignature.GetAddressOf()));

        // UI 전용 PSO 설정
        D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
        psoDesc.pRootSignature = mUIMaterial.data.rootSignature.Get();
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
        device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(mUIMaterial.data.pso.GetAddressOf()));

        mUIInitialized = true;
    }
    return &mUIMaterial;
}

Material* MaterialManager::GetMeshMaterial(ResourceManager* rm, const std::wstring& filename)
{
    std::vector<SubMesh> meshData;
    if (!ObjLoader::LoadWithMaterials(filename + L".obj", meshData, filename + L".mtl"))
    {
        OutputDebugStringA("OBJ Load Failed!\n");
        return nullptr;
    }

    if (meshData.empty())
    {
        OutputDebugStringA("meshData is empty!\n");
        return nullptr;
    }

    // ★★★★★ 모든 SubMesh를 하나로 합치기 ★★★★★
    std::vector<Vertex> allVertices;
    std::vector<WORD>   allIndices;
    WORD baseIndex = 0;

    for (const auto& sub : meshData)
    {
        allVertices.insert(allVertices.end(), sub.vertices.begin(), sub.vertices.end());

        for (WORD idx : sub.indices)
        {
            allIndices.push_back(baseIndex + idx);
        }
        baseIndex += (WORD)sub.vertices.size();
    }

    if (allVertices.empty() || allIndices.empty())
    {
        OutputDebugStringA("No geometry after merging SubMeshes!\n");
        return nullptr;
    }

    // 이제 합쳐진 데이터로 버퍼 생성
    rm->CreateVertexBuffer(sizeof(Vertex), (DWORD)allVertices.size(),
        &mMeshMaterial.data.vertexBufferView, mMeshMaterial.data.vertexBuffer.GetAddressOf(), allVertices.data());

    rm->CreateIndexBuffer((DWORD)allIndices.size(), &mMeshMaterial.data.indexBufferView,
        mMeshMaterial.data.indexBuffer.GetAddressOf(), allIndices.data());

    mMeshMaterial.data.indexCount = (UINT)allIndices.size();

    // 텍스처 (일단 임시로 girl.dds 사용 중)
    D3D12_RESOURCE_DESC texDesc = {};
    rm->CreateTexture3(mMeshMaterial.data.texture.GetAddressOf(), &texDesc, L"assets/textures/girl.dds");

    // PSO / RootSignature 생성 (기존 코드 그대로)
    if (!mMeshInitialized)
    {
        ID3D12Device14* device = rm->m_pDevice;

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
        sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;

        CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(1, rootParams, 1, &sampler, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
        Microsoft::WRL::ComPtr<ID3DBlob> signature, error;
        D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error);
        device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(mMeshMaterial.data.rootSignature.GetAddressOf()));

        D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
        psoDesc.pRootSignature = mMeshMaterial.data.rootSignature.Get();
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
        device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(mMeshMaterial.data.pso.GetAddressOf()));

        mMeshInitialized = true;
    }

    return &mMeshMaterial;
}
