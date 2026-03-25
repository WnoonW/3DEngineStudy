#include "ResourceManager.h"
#include <DirectXTex\DirectXTex.h>
#include <DDSTextureLoader/DDSTextureLoader12.h>

using namespace DirectX;

ResourceManager::ResourceManager(ID3D12Device14* device)
{
	m_pDevice = device;
	BuildCommandObject();
}

ResourceManager::~ResourceManager()
{
	CloseHandle(mFenceEvent);
}


void ResourceManager::BuildCommandObject()
{
	D3D12_COMMAND_LIST_TYPE type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	queueDesc.Type = type;
	queueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	m_pDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(m_pCommandQueue.GetAddressOf()));
	m_pDevice->CreateCommandAllocator(type, IID_PPV_ARGS(m_pCommandAllocator.GetAddressOf()));
	m_pDevice->CreateCommandList(0, type, m_pCommandAllocator.Get(), nullptr, IID_PPV_ARGS(m_pCommandList.GetAddressOf()));
	m_pCommandList->Close();

	m_pDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_pFence));
	mFenceEvent = CreateEventEx(nullptr, nullptr, false, EVENT_ALL_ACCESS);

	m_HeapPropertiesDefault = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	m_HeapPropertiesUpload = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
}

void ResourceManager::FlushCommandQueue()
{
	mCurrentFence++;
	m_pCommandQueue->Signal(m_pFence.Get(), mCurrentFence);

	if (m_pFence->GetCompletedValue() < mCurrentFence)
	{
		m_pFence->SetEventOnCompletion(mCurrentFence, mFenceEvent);
		WaitForSingleObject(mFenceEvent, INFINITE);
	}
}

void ResourceManager::CreateVertexBuffer(UINT SizePerVertex, DWORD dwVertexNum, D3D12_VERTEX_BUFFER_VIEW* pOutVertexBufferView, ID3D12Resource** ppOutBuffer, void* pData)
{
	D3D12_VERTEX_BUFFER_VIEW	VertexBufferView = {};
	ID3D12Resource* pVertexBuffer;
	ID3D12Resource* pUploadBuffer;
	UINT		VertexBufferSize = SizePerVertex * dwVertexNum;
	CD3DX12_RESOURCE_DESC VertexDesc = CD3DX12_RESOURCE_DESC::Buffer(VertexBufferSize);
	//Default Buffer
	m_pDevice->CreateCommittedResource(
		&m_HeapPropertiesDefault,
		D3D12_HEAP_FLAG_NONE, 
		&VertexDesc,
		D3D12_RESOURCE_STATE_COMMON,
		nullptr, 
		IID_PPV_ARGS(&pVertexBuffer));
	//Upload Buffer
	m_pDevice->CreateCommittedResource(
		&m_HeapPropertiesUpload,
		D3D12_HEAP_FLAG_NONE,
		&VertexDesc,
		D3D12_RESOURCE_STATE_COMMON,
		nullptr,
		IID_PPV_ARGS(&pUploadBuffer));

	if(pVertexBuffer == nullptr || pUploadBuffer == nullptr)
	{
		return;
	}

	//data copy to upload buffer
	UINT8* pVirtualMemory = nullptr;
	D3D12_RANGE readRange = { 0, 0 };
	pUploadBuffer->Map(0, &readRange, (void**)&pVirtualMemory);
	memcpy(pVirtualMemory, pData, VertexBufferSize);
	pUploadBuffer->Unmap(0, nullptr);

	//data copy from upload buffer to default buffer
	m_pCommandAllocator->Reset();
	m_pCommandList->Reset(m_pCommandAllocator.Get(), nullptr);
	auto barr = CD3DX12_RESOURCE_BARRIER::Transition(pVertexBuffer, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);
	m_pCommandList->CopyBufferRegion(pVertexBuffer, 0, pUploadBuffer, 0, VertexBufferSize);
	barr = CD3DX12_RESOURCE_BARRIER::Transition(pVertexBuffer, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
	m_pCommandList->ResourceBarrier(1, &barr);
	m_pCommandList->Close();
	ID3D12CommandList* ppCommandLists[] = { m_pCommandList.Get() };
	m_pCommandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);
	FlushCommandQueue();

	VertexBufferView.BufferLocation = pVertexBuffer->GetGPUVirtualAddress();
	VertexBufferView.StrideInBytes = SizePerVertex;
	VertexBufferView.SizeInBytes = VertexBufferSize;

	*pOutVertexBufferView = VertexBufferView;
	*ppOutBuffer = pVertexBuffer;

	pUploadBuffer->Release();
	pUploadBuffer = nullptr;
}

void ResourceManager::CreateIndexBuffer(DWORD dwIndexNum, D3D12_INDEX_BUFFER_VIEW* pOutIndexBufferView, ID3D12Resource** ppOutBuffer, void* pData)
{	
	D3D12_INDEX_BUFFER_VIEW	IndexBufferView = {};
	ID3D12Resource* pIndexBuffer;
	ID3D12Resource* pUploadBuffer;
	UINT		IndexBufferSize = sizeof(WORD) * dwIndexNum;
	CD3DX12_RESOURCE_DESC IndexDesc = CD3DX12_RESOURCE_DESC::Buffer(IndexBufferSize);

	//Default Buffer
	m_pDevice->CreateCommittedResource(
		&m_HeapPropertiesDefault,
		D3D12_HEAP_FLAG_NONE,
		&IndexDesc,
		D3D12_RESOURCE_STATE_COMMON,
		nullptr,
		IID_PPV_ARGS(&pIndexBuffer));
	//Upload Buffer
	m_pDevice->CreateCommittedResource(
		&m_HeapPropertiesUpload,
		D3D12_HEAP_FLAG_NONE,
		&IndexDesc,
		D3D12_RESOURCE_STATE_COMMON,
		nullptr,
		IID_PPV_ARGS(&pUploadBuffer));

	if(pIndexBuffer == nullptr || pUploadBuffer == nullptr)
	{
		return;
	}

	//data copy to upload buffer
	UINT8* pVirtualMemory = nullptr;
	D3D12_RANGE readRange = { 0, 0 };
	pUploadBuffer->Map(0, &readRange, (void**)&pVirtualMemory);
	memcpy(pVirtualMemory, pData, IndexBufferSize);
	pUploadBuffer->Unmap(0, nullptr);

	//data copy from upload buffer to default buffer
	m_pCommandAllocator->Reset();
	m_pCommandList->Reset(m_pCommandAllocator.Get(), nullptr);
	auto barr = CD3DX12_RESOURCE_BARRIER::Transition(pIndexBuffer, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);
	m_pCommandList->ResourceBarrier(1, &barr);
	m_pCommandList->CopyBufferRegion(pIndexBuffer, 0, pUploadBuffer, 0, IndexBufferSize);
	barr = CD3DX12_RESOURCE_BARRIER::Transition(pIndexBuffer, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_INDEX_BUFFER);
	m_pCommandList->ResourceBarrier(1, &barr);
	m_pCommandList->Close();
	ID3D12CommandList* ppCommandLists[] = { m_pCommandList.Get() };
	m_pCommandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);
	FlushCommandQueue();

	IndexBufferView.BufferLocation = pIndexBuffer->GetGPUVirtualAddress();
	IndexBufferView.Format = DXGI_FORMAT_R16_UINT;
	IndexBufferView.SizeInBytes = IndexBufferSize;

	*pOutIndexBufferView = IndexBufferView;
	*ppOutBuffer = pIndexBuffer;

	pUploadBuffer->Release();
	pUploadBuffer = nullptr;
}

void ResourceManager::CreateConstantBuffer(UINT elementByteSize,UINT elementCount,ID3D12Resource** ppOutBuffer)
{
	ID3D12Resource* pUploadBuffer;
	D3D12_RESOURCE_DESC rd = CD3DX12_RESOURCE_DESC::Buffer(elementByteSize * elementCount);
	m_pDevice->CreateCommittedResource(
		&m_HeapPropertiesUpload,
		D3D12_HEAP_FLAG_NONE,
		&rd,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&pUploadBuffer));
	*ppOutBuffer = pUploadBuffer;
}

void ResourceManager::CreateTexture(ID3D12Resource** ppOutResource, UINT Width, UINT Height, DXGI_FORMAT format, const BYTE* pInitImage)
{
	ID3D12Resource* pTexResource = nullptr;
	ID3D12Resource* pUploadBuffer = nullptr;

	D3D12_RESOURCE_DESC textureDesc = {};
	textureDesc.MipLevels = 1;
	textureDesc.Format = format;	// ex) DXGI_FORMAT_R8G8B8A8_UNORM, etc...
	textureDesc.Width = Width;
	textureDesc.Height = Height;
	textureDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
	textureDesc.DepthOrArraySize = 1;
	textureDesc.SampleDesc.Count = 1;
	textureDesc.SampleDesc.Quality = 0;
	textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	m_pDevice->CreateCommittedResource(
		&m_HeapPropertiesDefault,
		D3D12_HEAP_FLAG_NONE,
		&textureDesc,
		D3D12_RESOURCE_STATE_COMMON,
		nullptr,
		IID_PPV_ARGS(&pTexResource));
	m_pDevice->CreateCommittedResource(
		&m_HeapPropertiesUpload,
		D3D12_HEAP_FLAG_NONE,
		&textureDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&pUploadBuffer));
	UINT8* pVirtualMemory = nullptr;
	D3D12_RANGE readRange = { 0, 0 };
	pUploadBuffer->Map(0, &readRange, (void**)&pVirtualMemory);
	//memcpy(pVirtualMemory, , );
	pUploadBuffer->Unmap(0, nullptr);
}

void ResourceManager::CreateTexture2(byte* pngBytes, size_t pngSize, D3D12_CPU_DESCRIPTOR_HANDLE srvHandle, ID3D12Resource** ppOutTexture)
{
	ID3D12Resource* texture;
	DirectX::ResourceUploadBatch uploadBatch(m_pDevice);
	uploadBatch.Begin();

	DirectX::CreateWICTextureFromMemory(
		m_pDevice,
		uploadBatch,
		pngBytes,
		pngSize,
		&texture,
		true,   // generateMips = true → 자동으로 mip chain 생성 (좋은 품질)
		0       // maxsize = 0 (원본 크기 사용)
	);
/*	DirectX::CreateShaderResourceView(
		m_pDevice,
		texture,
		srvHandle
	);*/

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;           // 또는 _SRGB
	srvDesc.Texture2D.MipLevels = -1;                      // 전체 mip
	srvDesc.Texture2D.MostDetailedMip = 0;
	m_pDevice->CreateShaderResourceView(texture, &srvDesc, srvHandle);

	uploadBatch.End(m_pCommandQueue.Get());

	FlushCommandQueue();

	*ppOutTexture = texture;
}

BOOL ResourceManager::CreateTexture3(ID3D12Resource** ppOutResource, D3D12_RESOURCE_DESC* pOutDesc, const WCHAR* wchFileName)
{
	ID3D12Resource* pTexResource = nullptr;
	ID3D12Resource* pUploadBuffer = nullptr;

	D3D12_RESOURCE_DESC textureDesc = {};

	std::unique_ptr<uint8_t[]> ddsData;
	std::vector<D3D12_SUBRESOURCE_DATA> subresouceData;

	LoadDDSTextureFromFile(m_pDevice, wchFileName, &pTexResource, ddsData, subresouceData);

	textureDesc = pTexResource->GetDesc();
	UINT subresoucesize = (UINT)subresouceData.size();
	UINT64 uploadBufferSize = GetRequiredIntermediateSize(pTexResource, 0, subresoucesize);


	D3D12_RESOURCE_DESC uploadBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize);
	if (FAILED(m_pDevice->CreateCommittedResource(
		&m_HeapPropertiesUpload,
		D3D12_HEAP_FLAG_NONE,
		&uploadBufferDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&pUploadBuffer))))
	{
		return false;
	}

	m_pCommandAllocator->Reset();
	m_pCommandList->Reset(m_pCommandAllocator.Get(), nullptr);

	CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(pTexResource, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);
	m_pCommandList->ResourceBarrier(1, &barrier);
	UpdateSubresources(m_pCommandList.Get(), pTexResource, pUploadBuffer, 0, 0, subresoucesize, subresouceData.data());
	barrier = CD3DX12_RESOURCE_BARRIER::Transition(pTexResource, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	m_pCommandList->ResourceBarrier(1, &barrier);

	m_pCommandList->Close();

	ID3D12CommandList* ppCommandLists[] = { m_pCommandList.Get() };
	m_pCommandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

	FlushCommandQueue();

	pUploadBuffer->Release();
	pUploadBuffer = nullptr;

	*ppOutResource = pTexResource;
	*pOutDesc = textureDesc;

	return true;
}





