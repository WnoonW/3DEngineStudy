#pragma once
#include "ResourceManager.h"
#include <DirectXMath.h>
#include <DirectXColors.h>
#include "GameTimer.h"
#include "ECS.h"

using namespace DirectX;
struct ObjectVertex
{
	XMFLOAT3 position;
	XMFLOAT2 textcoord;
};

struct UIVertex
{
	XMFLOAT2 position;
	XMFLOAT2 uv;
	XMFLOAT4 color;
};

struct ObjectConstants
{
	XMFLOAT4X4 WorldViewProj = {};
	int isSelected = 0;
};

struct AABB
{
	XMFLOAT3 min;
	XMFLOAT3 max;
};

class Object
{
public:
	Object(ResourceManager* manager, XMFLOAT3 pos, XMFLOAT3 scale, bool UI, Entity entity);
	~Object();

	void Init();
	void Release();

	void Render(ID3D12GraphicsCommandList10* commandList);

	void BuildObject();
	void BuildObject1();
	void BuildHeap();
	void BuildConstant();
	void BuildSRV();
	void BuildShader();
	void BuildRootSignature();
	void BuildPSO();
	void Update(const GameTimer& gt, XMMATRIX view, XMMATRIX proj);


	UINT CalcConstantBufferByteSize(UINT byteSize) { return (byteSize + 255) & ~255; }

	void ConstantUpdate(int elementIndex, ObjectConstants& data);

	AABB GetWorldAABB() const;

public:
	void SetPosition(TransformComponent& transform);


public:
	ResourceManager* m_pResourceManager;
	ID3D12Device14* m_pDevice = nullptr;
	ComPtr<ID3D12Resource> m_pVertexBuffer = nullptr;
	ComPtr<ID3D12Resource> m_pIndexBuffer = nullptr;
	D3D12_VERTEX_BUFFER_VIEW m_VertexBufferView;
	D3D12_INDEX_BUFFER_VIEW m_IndexBufferView;
	ComPtr<ID3D12DescriptorHeap> m_pDescHeap;
	ComPtr<ID3D12Resource> m_pConstantBuffer = nullptr;
	ComPtr<ID3D12Resource> m_pTexture = nullptr;
	UINT8* m_pVirtualConstantMemory = nullptr;
	UINT mElementByteSize;

	UINT m_DescriptorSize;

	std::vector<D3D12_INPUT_ELEMENT_DESC> mInputLayout;
	ComPtr<ID3D12RootSignature> m_pRootSignature = nullptr;
	ComPtr<ID3D12PipelineState> m_pPSO = nullptr;
	ComPtr<ID3DBlob> mvsByteCode = nullptr;
	ComPtr<ID3DBlob> mpsByteCode = nullptr;


	//data
	Entity mEntityID;
	UINT indexCount = 0;

	AABB mLocalAABB;
	XMFLOAT3 mObjectPosition	= { 0, 0, 0 };
	XMFLOAT3 mObjectRotation	= { 0, 0, 0 };
	XMFLOAT3 mObjectScale		= { 1, 1, 1 };
	XMFLOAT3 mObjectVelocity	= { 0, 0, 0 };
	XMFLOAT4X4 mWorld = {};

	//flags
	bool isUI = false;
	bool isSelected = false;
};