#pragma once
#include <string>
#include <DirectXMath.h>
#include <wrl.h>
#include <d3d12.h>

struct MaterialData
{
    std::string name;

    DirectX::XMFLOAT3 ambient{ 0.2f, 0.2f, 0.2f };
    DirectX::XMFLOAT3 diffuse{ 0.8f, 0.8f, 0.8f };
    DirectX::XMFLOAT3 specular{ 1.0f, 1.0f, 1.0f };
    float shininess = 32.0f;
    float transparency = 1.0f;

    std::wstring diffuseMap;   // map_Kd
    std::wstring normalMap;

    // === 새로 추가: PSO / RootSignature / InputLayout 등을 Material이 직접 관리 ===
    Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> pso;
    D3D12_VERTEX_BUFFER_VIEW vertexBufferView = {};
    D3D12_INDEX_BUFFER_VIEW indexBufferView = {};

    Microsoft::WRL::ComPtr<ID3D12Resource> vertexBuffer;
    Microsoft::WRL::ComPtr<ID3D12Resource> indexBuffer;
    Microsoft::WRL::ComPtr<ID3D12Resource> texture;

    UINT indexCount = 0;
    bool isUI = false;                    // UI인지 Mesh인지 구분
    size_t materialID = 0;
};

class Material
{
public:
    MaterialData data;
};