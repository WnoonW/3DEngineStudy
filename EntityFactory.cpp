#include "EntityFactory.h"
#include "src/ObjLoader.h"
#include "src/MaterialManager.h"
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
    ID3D12Device14* device)
{
    render.descriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
    heapDesc.NumDescriptors = 2;
    heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(render.descHeap.GetAddressOf()));

    UINT elementByteSize = (sizeof(ObjectConstants) + 255) & ~255;
    rm->CreateConstantBuffer(elementByteSize, 1, render.constantBuffer.GetAddressOf());
    CD3DX12_RANGE readRange(0, 0);
    render.constantBuffer->Map(0, &readRange, reinterpret_cast<void**>(&render.pMappedConstantData));

    D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
    cbvDesc.BufferLocation = render.constantBuffer->GetGPUVirtualAddress();
    cbvDesc.SizeInBytes = elementByteSize;
    CD3DX12_CPU_DESCRIPTOR_HANDLE cbvHandle(render.descHeap->GetCPUDescriptorHandleForHeapStart(), 0, render.descriptorSize);
    device->CreateConstantBufferView(&cbvDesc, cbvHandle);

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = render.texture->GetDesc().Format;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = render.texture->GetDesc().MipLevels;
    CD3DX12_CPU_DESCRIPTOR_HANDLE srvHandle(render.descHeap->GetCPUDescriptorHandleForHeapStart(), 1, render.descriptorSize);
    device->CreateShaderResourceView(render.texture.Get(), &srvDesc, srvHandle);

}

Entity EntityFactory::CreateCube(Registry& registry, ResourceManager* rm, DirectX::XMFLOAT3 pos, DirectX::XMFLOAT3 scale)
{
    Entity entity = registry.CreateEntity();
    SetupTransformComponent(registry, entity, pos, scale);

    RenderComponent render;
    AABBComponent aabb;
    render.isUI = false;

    Material* mat = MaterialManager::GetInstance().GetCubeMaterial(rm);

    render.vertexBuffer = mat->data.vertexBuffer;
    render.indexBuffer = mat->data.indexBuffer;
    render.vertexBufferView = mat->data.vertexBufferView;
    render.indexBufferView = mat->data.indexBufferView;
    render.indexCount = mat->data.indexCount;
    render.texture = mat->data.texture;
    render.rootSignature = mat->data.rootSignature;
    render.pso = mat->data.pso;

    aabb.min = DirectX::XMFLOAT3(-1, -1, -1);
    aabb.max = DirectX::XMFLOAT3(1, 1, 1);

    SetupPerEntityRenderResources(render, rm, rm->m_pDevice);

    registry.AddComponent(entity, render);
    registry.AddComponent(entity, aabb);

    return entity;
}

Entity EntityFactory::CreateUI(Registry& registry, ResourceManager* rm, DirectX::XMFLOAT3 pos, DirectX::XMFLOAT3 scale)
{
    Entity entity = registry.CreateEntity();
    ID3D12Device14* device = rm->m_pDevice;

    SetupTransformComponent(registry, entity, pos, scale);

    RenderComponent render;
    AABBComponent aabb;
    render.isUI = true;

    Material* mat = MaterialManager::GetInstance().GetUIMaterial(rm);

    render.vertexBuffer = mat->data.vertexBuffer;
    render.indexBuffer = mat->data.indexBuffer;
    render.vertexBufferView = mat->data.vertexBufferView;
    render.indexBufferView = mat->data.indexBufferView;
    render.indexCount = mat->data.indexCount;
    render.texture = mat->data.texture;
    render.rootSignature = mat->data.rootSignature;
    render.pso = mat->data.pso;

    aabb.min = DirectX::XMFLOAT3(-1, -1, 0);
    aabb.max = DirectX::XMFLOAT3(1, 1, 0);

    SetupPerEntityRenderResources(render, rm, device);

    registry.AddComponent(entity, render);
    registry.AddComponent(entity, aabb);

    return entity;
}

Entity EntityFactory::CreateMesh(const std::wstring& filename, Registry& registry, ResourceManager* rm,
    DirectX::XMFLOAT3 pos, DirectX::XMFLOAT3 scale)
{
    Entity entity = registry.CreateEntity();
    ID3D12Device14* device = rm->m_pDevice;

    SetupTransformComponent(registry, entity, pos, scale);

    RenderComponent render;
    AABBComponent aabb;
    render.isUI = false;

    Material* mat = MaterialManager::GetInstance().GetMeshMaterial(rm, filename);

    render.vertexBuffer = mat->data.vertexBuffer;
    render.indexBuffer = mat->data.indexBuffer;
    render.vertexBufferView = mat->data.vertexBufferView;
    render.indexBufferView = mat->data.indexBufferView;
    render.indexCount = mat->data.indexCount;
	render.texture = mat->data.texture;
    render.rootSignature = mat->data.rootSignature;
    render.pso = mat->data.pso;

    aabb.min = DirectX::XMFLOAT3(-1, -1, -1);
    aabb.max = DirectX::XMFLOAT3(1, 1, 1);

    SetupPerEntityRenderResources(render, rm, device);

    registry.AddComponent(entity, render);
    registry.AddComponent(entity, aabb);

    return entity;
}