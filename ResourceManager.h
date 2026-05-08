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

    void CreateVertexBuffer(UINT SizePerVertex, DWORD dwVertexNum, D3D12_VERTEX_BUFFER_VIEW* pOutVertexBufferView, ID3D12Resource** ppOutBuffer, void* pData);
    void CreateIndexBuffer(DWORD dwIndexNum, D3D12_INDEX_BUFFER_VIEW* pOutIndexBufferView, ID3D12Resource** ppOutBuffer, void* pData);
    void CreateConstantBuffer(UINT elementByteSize, UINT elementCount, ID3D12Resource** ppOutBuffer);
    void CreateTexture2(byte* pngBytes, size_t pngSize, D3D12_CPU_DESCRIPTOR_HANDLE srvHandle, ID3D12Resource** ppOutTexture);
    BOOL CreateTexture3(ID3D12Resource** ppOutResource, D3D12_RESOURCE_DESC* pOutDesc, const WCHAR* wchFileName);

public:
    ID3D12Device14* m_pDevice = nullptr;
};