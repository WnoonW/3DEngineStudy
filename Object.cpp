#include "Object.h"	
#include "AppUtill.h"
#include "d3dx12.h"
#include <fstream>
#include <vector>

Object::Object(ResourceManager* manager, XMFLOAT3 pos, XMFLOAT3 scale, bool UI, Entity entity)
{
	m_pResourceManager = manager;
	m_pDevice = m_pResourceManager->m_pDevice;
	mObjectPosition = pos;
	mObjectScale = scale;
	isUI = UI;
	mEntityID = entity;
	Init();
}

Object::~Object()
{
	Release();
}

void Object::Init()
{
	m_DescriptorSize = m_pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	XMMATRIX initial = XMMatrixTranslation(mObjectPosition.x, mObjectPosition.y, mObjectPosition.z);
	XMStoreFloat4x4(&mWorld, initial);

	//vertex
	//index
	if (isUI) { BuildObject1(); }
	else { BuildObject(); }
	BuildHeap();
	//constant
	BuildConstant();
	//srv
	BuildSRV();
	//shader
	BuildShader();
	//rootsignature
	BuildRootSignature();
	//pso
	BuildPSO();
}

void Object::Render(ID3D12GraphicsCommandList10* cl)
{


	cl->SetGraphicsRootSignature(m_pRootSignature.Get());
	cl->SetPipelineState(m_pPSO.Get());
	cl->IASetVertexBuffers(0,1, &m_VertexBufferView);
	cl->IASetIndexBuffer(&m_IndexBufferView);
	cl->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	ID3D12DescriptorHeap* pHeaps[] = { m_pDescHeap.Get() };
	cl->SetDescriptorHeaps(_countof(pHeaps), pHeaps);
	CD3DX12_GPU_DESCRIPTOR_HANDLE cbvGpuHandle(m_pDescHeap->GetGPUDescriptorHandleForHeapStart(), 0, m_DescriptorSize);
	cl->SetGraphicsRootDescriptorTable(0, cbvGpuHandle);

	cl->DrawIndexedInstanced(indexCount,1,0,0,0);
}

void Object::Release()
{
	m_pConstantBuffer->Unmap(0, nullptr);
}


void Object::BuildObject()
{
	ObjectVertex Vertices[] =
	{
		// Front (+Z)
		{ XMFLOAT3(-1, -1, +1), XMFLOAT2(0, 1) },
		{ XMFLOAT3(+1, -1, +1), XMFLOAT2(1, 1) },
		{ XMFLOAT3(+1, +1, +1), XMFLOAT2(1, 0) },
		{ XMFLOAT3(-1, +1, +1), XMFLOAT2(0, 0) },

		// Back  (-Z)
		{ XMFLOAT3(-1, -1, -1), XMFLOAT2(0, 1) },
		{ XMFLOAT3(-1, +1, -1), XMFLOAT2(0, 0) },
		{ XMFLOAT3(+1, +1, -1), XMFLOAT2(1, 0) },
		{ XMFLOAT3(+1, -1, -1), XMFLOAT2(1, 1) },

		// Right (+X)
		{ XMFLOAT3(+1, -1, -1), XMFLOAT2(0, 1) },
		{ XMFLOAT3(+1, -1, +1), XMFLOAT2(1, 1) },
		{ XMFLOAT3(+1, +1, +1), XMFLOAT2(1, 0) },
		{ XMFLOAT3(+1, +1, -1), XMFLOAT2(0, 0) },

		// Left  (-X)
		{ XMFLOAT3(-1, -1, +1), XMFLOAT2(0, 1) },
		{ XMFLOAT3(-1, +1, +1), XMFLOAT2(0, 0) },
		{ XMFLOAT3(-1, +1, -1), XMFLOAT2(1, 0) },
		{ XMFLOAT3(-1, -1, -1), XMFLOAT2(1, 1) },

		// Top   (+Y)
		{ XMFLOAT3(-1, +1, -1), XMFLOAT2(0, 1) },
		{ XMFLOAT3(-1, +1, +1), XMFLOAT2(0, 0) },
		{ XMFLOAT3(+1, +1, +1), XMFLOAT2(1, 0) },
		{ XMFLOAT3(+1, +1, -1), XMFLOAT2(1, 1) },

		// Bottom (-Y)
		{ XMFLOAT3(-1, -1, -1), XMFLOAT2(0, 1) },
		{ XMFLOAT3(+1, -1, -1), XMFLOAT2(1, 1) },
		{ XMFLOAT3(+1, -1, +1), XMFLOAT2(1, 0) },
		{ XMFLOAT3(-1, -1, +1), XMFLOAT2(0, 0) },
	};

	XMFLOAT3 minPt(FLT_MAX, FLT_MAX, FLT_MAX);
	XMFLOAT3 maxPt(-FLT_MAX, -FLT_MAX, -FLT_MAX);

	constexpr UINT vertexCount = _countof(Vertices);
	for (UINT i = 0; i < vertexCount; ++i)
	{
		const XMFLOAT3& pos = Vertices[i].position;

		minPt.x = min(minPt.x, pos.x);
		minPt.y = min(minPt.y, pos.y);
		minPt.z = min(minPt.z, pos.z);

		maxPt.x = max(maxPt.x, pos.x);
		maxPt.y = max(maxPt.y, pos.y);
		maxPt.z = max(maxPt.z, pos.z);
	}

	mLocalAABB.min = minPt;
	mLocalAABB.max = maxPt;

	WORD Indices[] =
	{
		// Front
		0, 1, 2,    0, 2, 3,
		// Back
		4, 5, 6,    4, 6, 7,
		// Right
		8, 9,10,    8,10,11,
		// Left
		12,13,14,   12,14,15,
		// Top
		16,17,18,   16,18,19,
		// Bottom
		20,21,22,   20,22,23
	};

	indexCount = _countof(Indices);
	m_pResourceManager->CreateVertexBuffer(sizeof(ObjectVertex),(DWORD)_countof(Vertices), &m_VertexBufferView, m_pVertexBuffer.GetAddressOf(), Vertices);
	m_pResourceManager->CreateIndexBuffer((DWORD)_countof(Indices), &m_IndexBufferView, m_pIndexBuffer.GetAddressOf(), Indices);
}

void Object::BuildObject1()
{
	UIVertex Vertices[] =
	{
		{ XMFLOAT2(-0.9f, -0.9f), XMFLOAT2(0, 1), XMFLOAT4(1, 0, 0, 0) }, //왼쪽아래
		{ XMFLOAT2(-0.5f, -0.9f), XMFLOAT2(1, 1), XMFLOAT4(1, 0, 0, 0) },	//오른쪽 아래
		{ XMFLOAT2(-0.5f, -0.5f), XMFLOAT2(1, 0), XMFLOAT4(1, 0, 0, 0) }, //오른쪽 위
		{ XMFLOAT2(-0.9f, -0.5f), XMFLOAT2(0, 0), XMFLOAT4(1, 0, 0, 0) }, //왼쪽 위
	};

	WORD Indices[] =
	{
		0, 1, 2,    0, 2, 3
	};
	indexCount = _countof(Indices);
	m_pResourceManager->CreateVertexBuffer(sizeof(UIVertex), (DWORD)_countof(Vertices), &m_VertexBufferView, m_pVertexBuffer.GetAddressOf(), Vertices);
	m_pResourceManager->CreateIndexBuffer((DWORD)_countof(Indices), &m_IndexBufferView, m_pIndexBuffer.GetAddressOf(), Indices);
}

void Object::BuildHeap()
{
	D3D12_DESCRIPTOR_HEAP_DESC resourceHeapDesc;
	resourceHeapDesc.NumDescriptors = 2;
	resourceHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	resourceHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	resourceHeapDesc.NodeMask = 0;
	m_pDevice->CreateDescriptorHeap(&resourceHeapDesc, IID_PPV_ARGS(m_pDescHeap.GetAddressOf()));
}

void Object::BuildConstant()
{
	mElementByteSize = CalcConstantBufferByteSize(sizeof(ObjectConstants));
	m_pResourceManager->CreateConstantBuffer(mElementByteSize,1,m_pConstantBuffer.GetAddressOf());
	D3D12_RANGE readRange = { 0, 0 };
	m_pVirtualConstantMemory = nullptr;
	m_pConstantBuffer->Map(0, &readRange, (void**)&m_pVirtualConstantMemory);

	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
	cbvDesc.BufferLocation = m_pConstantBuffer->GetGPUVirtualAddress();
	cbvDesc.SizeInBytes = mElementByteSize * 1;  // elementCount=1 기준

	CD3DX12_CPU_DESCRIPTOR_HANDLE cbvHandle(m_pDescHeap->GetCPUDescriptorHandleForHeapStart(), 0, m_DescriptorSize);
	m_pDevice->CreateConstantBufferView(&cbvDesc, cbvHandle);
}

void Object::BuildSRV()
{
/*	std::vector<byte> pngData;
	std::ifstream file("assets/textures/girl.png", std::ios::binary | std::ios::ate);

	auto fileSize = file.tellg();
	pngData.resize(static_cast<size_t>(fileSize));
	file.seekg(0, std::ios::beg);
	file.read(reinterpret_cast<char*>(pngData.data()), fileSize);
	file.close();

	srvHandle = m_pDescHeap->GetCPUDescriptorHandleForHeapStart();
	srvHandle.ptr += m_pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV); // CBV 다음 위치로 이동
	m_pResourceManager->CreateTexture2( pngData.data(), pngData.size(), srvHandle, m_pTexture.GetAddressOf());*/

	ID3D12Resource* pTexResource = nullptr;
	D3D12_RESOURCE_DESC desc = {};
	m_pResourceManager->CreateTexture3(&pTexResource, &desc, L"assets/textures/girl.dds");

	D3D12_SHADER_RESOURCE_VIEW_DESC SRVDesc = {};
	SRVDesc.Format = desc.Format;
	SRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	SRVDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	SRVDesc.Texture2D.MipLevels = desc.MipLevels;
	SRVDesc.Texture2D.MostDetailedMip = 0;

	CD3DX12_CPU_DESCRIPTOR_HANDLE srvHandle(m_pDescHeap->GetCPUDescriptorHandleForHeapStart(), 1, m_DescriptorSize);
	m_pDevice->CreateShaderResourceView(pTexResource, &SRVDesc, srvHandle);
}

void Object::BuildShader()
{
	if (isUI)
	{
		mvsByteCode = AppUtill::CompileShader(L"Shaders\\UI.hlsl", nullptr, "VS", "vs_5_0");
		mpsByteCode = AppUtill::CompileShader(L"Shaders\\UI.hlsl", nullptr, "PS", "ps_5_0");

		mInputLayout =
		{
			{ "POSITION",	0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD",	0, DXGI_FORMAT_R32G32_FLOAT, 0, 8, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "COLOR",		0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 16, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
		};
	}
	else 
	{
		mvsByteCode = AppUtill::CompileShader(L"Shaders\\color_v2.hlsl", nullptr, "VS", "vs_5_0");
		mpsByteCode = AppUtill::CompileShader(L"Shaders\\color_v2.hlsl", nullptr, "PS", "ps_5_0");

		mInputLayout =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
		};
	}
}

void Object::BuildRootSignature()
{
	const int rootParamCount = 1;
	const int rangeCount = 2;
	CD3DX12_ROOT_PARAMETER slotRootParameter[rootParamCount] = {};

	CD3DX12_DESCRIPTOR_RANGE descriptorRange[rangeCount] = {};
	descriptorRange[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0, 0);
	descriptorRange[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0);
	
	slotRootParameter[0].InitAsDescriptorTable(rangeCount, descriptorRange, D3D12_SHADER_VISIBILITY_ALL);
	//slotRootParameter[0].InitAsDescriptorTable(1, &descriptorRange[0]);
	//slotRootParameter[1].InitAsDescriptorTable(1, &descriptorRange[1]);

	D3D12_STATIC_SAMPLER_DESC sampler = {};
	sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	sampler.MipLODBias = 0.0f;
	sampler.MaxAnisotropy = 1;
	sampler.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
	sampler.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
	sampler.MinLOD = 0.0f;
	sampler.MaxLOD = D3D12_FLOAT32_MAX;
	sampler.ShaderRegister = 0;
	sampler.RegisterSpace = 0;
	sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;


	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc;
	rootSigDesc.Init(rootParamCount, slotRootParameter, 1, &sampler, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
	ComPtr<ID3DBlob> serializedRootSig = nullptr;
	ComPtr<ID3DBlob> errorBlob = nullptr;
	D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1, serializedRootSig.GetAddressOf(), errorBlob.GetAddressOf());
	m_pDevice->CreateRootSignature(
		0,
		serializedRootSig->GetBufferPointer(),
		serializedRootSig->GetBufferSize(),
		IID_PPV_ARGS(m_pRootSignature.GetAddressOf()));
}

void Object::BuildPSO()
{
	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc;
	ZeroMemory(&psoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	psoDesc.pRootSignature = m_pRootSignature.Get();
	psoDesc.VS = { reinterpret_cast<BYTE*>(mvsByteCode->GetBufferPointer()),
		mvsByteCode->GetBufferSize() };
	psoDesc.PS = { reinterpret_cast<BYTE*>(mpsByteCode->GetBufferPointer()),
		mpsByteCode->GetBufferSize() };
	psoDesc.DS = { nullptr, 0 };
	psoDesc.HS = { nullptr, 0 };
	psoDesc.GS = { nullptr, 0 };
	psoDesc.StreamOutput = { nullptr, 0, nullptr, 0, 0 };
	psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
	psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	psoDesc.InputLayout = { mInputLayout.data(), (UINT)mInputLayout.size() };
	psoDesc.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	psoDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
	psoDesc.SampleDesc.Count = 1;
	psoDesc.SampleDesc.Quality = 0;
	psoDesc.NodeMask = 0;
	psoDesc.CachedPSO = { nullptr, 0 };
	psoDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
	m_pDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(m_pPSO.GetAddressOf()));
}

void Object::Update(const GameTimer& gt, XMMATRIX view, XMMATRIX proj)
{
	//movement
	//mObjectPosition.x = sinf(gt.TotalTime()) * 10.0f;



	//Matrix
	XMMATRIX worldBase = XMLoadFloat4x4(&mWorld); 
	XMMATRIX scaling = XMMatrixScaling(mObjectScale.x, mObjectScale.y, mObjectScale.z);
	XMMATRIX translation = XMMatrixTranslation(mObjectPosition.x, mObjectPosition.y, mObjectPosition.z);
	//Scale → Rotate → Translate 순서
	XMMATRIX world = worldBase * scaling * translation;
	XMMATRIX worldViewProj;
	if (isUI)
	{
		worldViewProj = world * proj;
	}
	else 
	{
		worldViewProj = world * view * proj;
	}

	ObjectConstants objConstants;
	objConstants.isSelected = isSelected ? 1 : 0;
	XMStoreFloat4x4(&objConstants.WorldViewProj, XMMatrixTranspose(worldViewProj));
	ConstantUpdate(0, objConstants);
}



void Object::ConstantUpdate(int elementIndex, ObjectConstants &data)
{
	memcpy(&m_pVirtualConstantMemory[elementIndex * mElementByteSize], &data, sizeof(ObjectConstants));
}


AABB Object::GetWorldAABB() const
{
	XMVECTOR centerLocal = XMVectorScale(
		XMVectorAdd(XMLoadFloat3(&mLocalAABB.min), XMLoadFloat3(&mLocalAABB.max)), 0.5f);
	XMVECTOR extentLocal = XMVectorScale(
		XMVectorSubtract(XMLoadFloat3(&mLocalAABB.max), XMLoadFloat3(&mLocalAABB.min)), 0.5f);

	// 월드 위치에 중심 이동
	XMVECTOR centerWorld = XMVectorAdd(centerLocal, XMLoadFloat3(&mObjectPosition));

	// 스케일 적용 (대략적)
	extentLocal = XMVectorMultiply(extentLocal, XMLoadFloat3(&mObjectScale));

	AABB result;
	XMStoreFloat3(&result.min, XMVectorSubtract(centerWorld, extentLocal));
	XMStoreFloat3(&result.max, XMVectorAdd(centerWorld, extentLocal));

	return result;
}

void Object::SetPosition(TransformComponent& transform)
{
	mObjectPosition = transform.position;
	mObjectVelocity = transform.velocity;
}

