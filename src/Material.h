#pragma once
#include <string>
#include <DirectXMath.h>

struct MaterialData
{
    std::string name;

    DirectX::XMFLOAT3 ambient{ 0.2f, 0.2f, 0.2f };   // Ka
    DirectX::XMFLOAT3 diffuse{ 0.8f, 0.8f, 0.8f };   // Kd
    DirectX::XMFLOAT3 specular{ 1.0f, 1.0f, 1.0f };  // Ks
    float shininess = 32.0f;                          // Ns
    float transparency = 1.0f;                        // d (1.0 = opaque)

    std::wstring diffuseMap;   // map_Kd (텍스처 파일명)
    std::wstring normalMap;    // map_Bump 또는 map_normal (추후 확장 가능)

    // 나중에 PSO, RootSignature 등과 연결될 때 사용할 ID
    size_t materialID = 0;
};