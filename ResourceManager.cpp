#include "ResourceManager.h"
#include <DirectXTex/DirectXTex.h>
#include <DDSTextureLoader/DDSTextureLoader12.h>
#include <wrl.h>   // ComPtr 사용을 위해 필수

using namespace DirectX;
using Microsoft::WRL::ComPtr;

ResourceManager::ResourceManager(ID3D12Device14* device)
{
    m_pDevice = device;
}

ResourceManager::~ResourceManager()
{
}
void ResourceManager::CreateVertexBuffer(UINT SizePerVertex, DWORD dwVertexNum, D3D12_VERTEX_BUFFER_VIEW* pOutVertexBufferView, ID3D12Resource** ppOutBuffer, void* pData)
{
    D3D12_VERTEX_BUFFER_VIEW VertexBufferView = {};
    ID3D12Resource* pVertexBuffer = nullptr;
    ID3D12Resource* pUploadBuffer = nullptr;
    UINT VertexBufferSize = SizePerVertex * dwVertexNum;
    CD3DX12_RESOURCE_DESC VertexDesc = CD3DX12_RESOURCE_DESC::Buffer(VertexBufferSize);

    CD3DX12_HEAP_PROPERTIES defaultHeap(D3D12_HEAP_TYPE_DEFAULT);
    CD3DX12_HEAP_PROPERTIES uploadHeap(D3D12_HEAP_TYPE_UPLOAD);

    m_pDevice->CreateCommittedResource(&defaultHeap, D3D12_HEAP_FLAG_NONE, &VertexDesc, D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&pVertexBuffer));
    m_pDevice->CreateCommittedResource(&uploadHeap, D3D12_HEAP_FLAG_NONE, &VertexDesc, D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&pUploadBuffer));

    if (!pVertexBuffer || !pUploadBuffer) return;

    UINT8* pVirtualMemory = nullptr;
    D3D12_RANGE readRange = { 0, 0 };
    pUploadBuffer->Map(0, &readRange, (void**)&pVirtualMemory);
    memcpy(pVirtualMemory, pData, VertexBufferSize);
    pUploadBuffer->Unmap(0, nullptr);

    // 간단한 업로드 (ResourceManager 내부)
    ComPtr<ID3D12CommandAllocator> cmdAlloc;
    ComPtr<ID3D12GraphicsCommandList> cmdList;
    ComPtr<ID3D12CommandQueue> cmdQueue;

    D3D12_COMMAND_QUEUE_DESC queueDesc = {};
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    m_pDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&cmdQueue));
    m_pDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&cmdAlloc));
    m_pDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, cmdAlloc.Get(), nullptr, IID_PPV_ARGS(&cmdList));

    auto barr = CD3DX12_RESOURCE_BARRIER::Transition(pVertexBuffer, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);
    cmdList->ResourceBarrier(1, &barr);
    cmdList->CopyBufferRegion(pVertexBuffer, 0, pUploadBuffer, 0, VertexBufferSize);
    barr = CD3DX12_RESOURCE_BARRIER::Transition(pVertexBuffer, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
    cmdList->ResourceBarrier(1, &barr);

    cmdList->Close();
    ID3D12CommandList* lists[] = { cmdList.Get() };
    cmdQueue->ExecuteCommandLists(1, lists);

    ComPtr<ID3D12Fence> fence;
    m_pDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));
    cmdQueue->Signal(fence.Get(), 1);
    if (fence->GetCompletedValue() < 1)
    {
        HANDLE event = CreateEvent(nullptr, FALSE, FALSE, nullptr);
        fence->SetEventOnCompletion(1, event);
        WaitForSingleObject(event, INFINITE);
        CloseHandle(event);
    }

    VertexBufferView.BufferLocation = pVertexBuffer->GetGPUVirtualAddress();
    VertexBufferView.StrideInBytes = SizePerVertex;
    VertexBufferView.SizeInBytes = VertexBufferSize;

    *pOutVertexBufferView = VertexBufferView;
    *ppOutBuffer = pVertexBuffer;

    pUploadBuffer->Release();
}

void ResourceManager::CreateIndexBuffer(DWORD dwIndexNum, D3D12_INDEX_BUFFER_VIEW* pOutIndexBufferView, ID3D12Resource** ppOutBuffer, void* pData)
{
    D3D12_INDEX_BUFFER_VIEW IndexBufferView = {};
    ID3D12Resource* pIndexBuffer = nullptr;
    ID3D12Resource* pUploadBuffer = nullptr;
    UINT IndexBufferSize = sizeof(WORD) * dwIndexNum;
    CD3DX12_RESOURCE_DESC IndexDesc = CD3DX12_RESOURCE_DESC::Buffer(IndexBufferSize);

    CD3DX12_HEAP_PROPERTIES defaultHeap(D3D12_HEAP_TYPE_DEFAULT);
    CD3DX12_HEAP_PROPERTIES uploadHeap(D3D12_HEAP_TYPE_UPLOAD);

    m_pDevice->CreateCommittedResource(&defaultHeap, D3D12_HEAP_FLAG_NONE, &IndexDesc, D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&pIndexBuffer));
    m_pDevice->CreateCommittedResource(&uploadHeap, D3D12_HEAP_FLAG_NONE, &IndexDesc, D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&pUploadBuffer));

    if (!pIndexBuffer || !pUploadBuffer) return;

    UINT8* pVirtualMemory = nullptr;
    D3D12_RANGE readRange = { 0, 0 };
    pUploadBuffer->Map(0, &readRange, (void**)&pVirtualMemory);
    memcpy(pVirtualMemory, pData, IndexBufferSize);
    pUploadBuffer->Unmap(0, nullptr);

    ComPtr<ID3D12CommandAllocator> cmdAlloc;
    ComPtr<ID3D12GraphicsCommandList> cmdList;
    ComPtr<ID3D12CommandQueue> cmdQueue;

    D3D12_COMMAND_QUEUE_DESC queueDesc = {};
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    m_pDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&cmdQueue));
    m_pDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&cmdAlloc));
    m_pDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, cmdAlloc.Get(), nullptr, IID_PPV_ARGS(&cmdList));

    auto barr = CD3DX12_RESOURCE_BARRIER::Transition(pIndexBuffer, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);
    cmdList->ResourceBarrier(1, &barr);
    cmdList->CopyBufferRegion(pIndexBuffer, 0, pUploadBuffer, 0, IndexBufferSize);
    barr = CD3DX12_RESOURCE_BARRIER::Transition(pIndexBuffer, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_INDEX_BUFFER);
    cmdList->ResourceBarrier(1, &barr);

    cmdList->Close();
    ID3D12CommandList* lists[] = { cmdList.Get() };
    cmdQueue->ExecuteCommandLists(1, lists);

    ComPtr<ID3D12Fence> fence;
    m_pDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));
    cmdQueue->Signal(fence.Get(), 1);
    if (fence->GetCompletedValue() < 1)
    {
        HANDLE event = CreateEvent(nullptr, FALSE, FALSE, nullptr);
        fence->SetEventOnCompletion(1, event);
        WaitForSingleObject(event, INFINITE);
        CloseHandle(event);
    }

    IndexBufferView.BufferLocation = pIndexBuffer->GetGPUVirtualAddress();
    IndexBufferView.Format = DXGI_FORMAT_R16_UINT;
    IndexBufferView.SizeInBytes = IndexBufferSize;

    *pOutIndexBufferView = IndexBufferView;
    *ppOutBuffer = pIndexBuffer;

    pUploadBuffer->Release();
}

void ResourceManager::CreateConstantBuffer(UINT elementByteSize, UINT elementCount, ID3D12Resource** ppOutBuffer)
{
    ID3D12Resource* pUploadBuffer = nullptr;
    D3D12_RESOURCE_DESC rd = CD3DX12_RESOURCE_DESC::Buffer(elementByteSize * elementCount);
    CD3DX12_HEAP_PROPERTIES uploadHeap(D3D12_HEAP_TYPE_UPLOAD);
    m_pDevice->CreateCommittedResource(&uploadHeap, D3D12_HEAP_FLAG_NONE, &rd, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&pUploadBuffer));
    *ppOutBuffer = pUploadBuffer;
}

void ResourceManager::CreateTexture2(byte* pngBytes, size_t pngSize, D3D12_CPU_DESCRIPTOR_HANDLE srvHandle, ID3D12Resource** ppOutTexture)
{
    ID3D12Resource* texture = nullptr;
    DirectX::ResourceUploadBatch uploadBatch(m_pDevice);
    uploadBatch.Begin();

    DirectX::CreateWICTextureFromMemory(
        m_pDevice,
        uploadBatch,
        pngBytes,
        pngSize,
        &texture,
        true,
        0
    );

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    srvDesc.Texture2D.MipLevels = -1;
    srvDesc.Texture2D.MostDetailedMip = 0;
    m_pDevice->CreateShaderResourceView(texture, &srvDesc, srvHandle);

    ComPtr<ID3D12CommandQueue> tempQueue;
    D3D12_COMMAND_QUEUE_DESC qd = {};
    qd.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    m_pDevice->CreateCommandQueue(&qd, IID_PPV_ARGS(&tempQueue));
    uploadBatch.End(tempQueue.Get());

    *ppOutTexture = texture;
}

BOOL ResourceManager::CreateTexture3(ID3D12Resource** ppOutResource, D3D12_RESOURCE_DESC* pOutDesc, const WCHAR* wchFileName)
{
    ID3D12Resource* pTexResource = nullptr;
    std::unique_ptr<uint8_t[]> ddsData;
    std::vector<D3D12_SUBRESOURCE_DATA> subresourceData;

    LoadDDSTextureFromFile(m_pDevice, wchFileName, &pTexResource, ddsData, subresourceData);

    *ppOutResource = pTexResource;
    *pOutDesc = pTexResource->GetDesc();

    return true;
}