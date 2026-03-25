#pragma once
#include "d3dx12.h"
#include <WICTextureLoader.h>
#include <ResourceUploadBatch.h>
#include <DirectXHelpers.h>

using Microsoft::WRL::ComPtr;

class ResourceManager
{
public:
	ResourceManager(ID3D12Device14* device);
	~ResourceManager();
	void BuildCommandObject();
	void FlushCommandQueue();
	void CreateVertexBuffer(UINT SizePerVertex, DWORD dwVertexNum, D3D12_VERTEX_BUFFER_VIEW* pOutVertexBufferView, ID3D12Resource** ppOutBuffer, void* pData);
	void CreateIndexBuffer(DWORD dwIndexNum, D3D12_INDEX_BUFFER_VIEW* pOutIndexBufferView, ID3D12Resource** ppOutBuffer, void* pData);
	void CreateConstantBuffer(UINT elementByteSize, UINT elementCount, ID3D12Resource** ppOutBuffer);
	void CreateTexture(ID3D12Resource** ppOutResource, UINT Width, UINT Height, DXGI_FORMAT format, const BYTE* pInitImage);
	void CreateTexture2(byte* pngBytes, size_t pngSize, D3D12_CPU_DESCRIPTOR_HANDLE srvHandle, ID3D12Resource** ppOutTexture);
	BOOL CreateTexture3(ID3D12Resource** ppOutResource, D3D12_RESOURCE_DESC* pOutDesc, const WCHAR* wchFileName);

public:
	ID3D12Device14* m_pDevice = nullptr;
	ComPtr<ID3D12CommandQueue> m_pCommandQueue = nullptr;
	ComPtr<ID3D12CommandAllocator> m_pCommandAllocator = nullptr;
	ComPtr<ID3D12GraphicsCommandList10> m_pCommandList = nullptr;
	ComPtr<ID3D12Fence> m_pFence;
	HANDLE mFenceEvent = nullptr;
	UINT64 mCurrentFence = 0;

	CD3DX12_HEAP_PROPERTIES m_HeapPropertiesDefault;
	CD3DX12_HEAP_PROPERTIES m_HeapPropertiesUpload;
};