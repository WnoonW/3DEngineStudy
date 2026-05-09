#include "App.h"
#include <DirectXColors.h>

using namespace DirectX;

LRESULT CALLBACK
MainWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    return App::GetApp()->MsgProc(hwnd, msg, wParam, lParam);
}

App* App::mApp = nullptr;
App* App::GetApp()
{
    return mApp;
}

App::App(HINSTANCE hInstance) : mhAppInst(hInstance)
{
    assert(mApp == nullptr);
    mApp = this;
    mScreenViewport = { 0,0,0,0 };
    mScissorRect = { 0,0,0,0 };
    mLastMousePos = { 0, 0 };
}

App::~App()
{
    CleanUp();
}

int App::Run()
{
    MSG msg = { 0 };

    mTimer.Reset();

    while (1)
    {
        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            if (msg.message == WM_QUIT)
                break;
        }
        else
        {
            mTimer.Tick();

            Update(mTimer);
            Draw();
        }
    }
    return 0;
}

bool App::Initialize()
{
    if (!InitMainWindow()) return false;
    if (!InitDirect3D()) return false;

    OnResize();

    CreateResourceManager(md3dDevice.Get());

    CreateObject(XMFLOAT3(0, 0, 0), XMFLOAT3(1, 1, 1));
    CreateUIObject(XMFLOAT3(0,0,0), XMFLOAT3(200, 200, 1));

    CreateMesh();

    return true;
}

LRESULT App::MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_LBUTTONDOWN:
    case WM_MBUTTONDOWN:
    case WM_RBUTTONDOWN:
        OnMouseDown(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        return 0;

    case WM_LBUTTONUP:
    case WM_MBUTTONUP:
    case WM_RBUTTONUP:
        OnMouseUp(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        return 0;

    case WM_MOUSEMOVE:
        OnMouseMove(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        return 0;

    case WM_KEYDOWN:
    case WM_SYSKEYDOWN:
    {
        KeyCode keyDown = mInputLayout.ToKeyCode(wParam);
        rbKeyDown.Push(&keyDown, sizeof(KeyCode));
        break;
    }

    case WM_KEYUP:
    case WM_SYSKEYUP:
    {
        KeyCode keyUp = mInputLayout.ToKeyCode(wParam);
        rbKeyUp.Push(&keyUp, sizeof(KeyCode));
        break;
    }

    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

void App::Update(const GameTimer& gt)
{
    KeyCode key;
    if (rbKeyDown.Pop(&key, sizeof(KeyCode)))
    {
        if (key == KeyCode::A) { a++; CreateObject(XMFLOAT3(-a, 0, 0), XMFLOAT3(1, 1, 1)); }
        else if (key == KeyCode::D) { d++; CreateObject(XMFLOAT3(d, 0, 0), XMFLOAT3(1, 1, 1)); }
        else if (key == KeyCode::Z && mSelectedEntity != UINT32_MAX)
        {
            DestroyObject(mSelectedEntity);
        }
    }

    float dt = gt.DeltaTime();
    PhysicsSystem::Update(dt, mRegistry);
    TransformSystem::Update(mRegistry);

    float x = mRadius * sinf(mPhi) * cosf(mTheta);
    float z = mRadius * sinf(mPhi) * sinf(mTheta);
    float y = mRadius * cosf(mPhi);

    XMVECTOR pos = XMVectorSet(x, y, z, 1.0f);
    XMVECTOR target = XMVectorZero();
    XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

    XMMATRIX view = XMMatrixLookAtLH(pos, target, up);
    XMStoreFloat4x4(&mView, view);

    XMMATRIX proj = XMMatrixPerspectiveFovLH(0.25f * MathHelper::Pi, AspectRatio(), 1.0f, 1000.0f);
    RenderSystem::UpdateConstants(mRegistry, view, proj, (float)mClientWidth, (float)mClientHeight);
}

void App::Draw()
{
    mCommandAlloc->Reset();
    mCommandList->Reset(mCommandAlloc.Get(), nullptr);
    mCommandList->RSSetViewports(1, &mScreenViewport);
    mCommandList->RSSetScissorRects(1, &mScissorRect);

    auto tr = CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
    mCommandList->ResourceBarrier(1, &tr);

    auto rtvHandle = CurrentBackBufferView();
    auto dsvHandle = DepthStencilView();
    mCommandList->ClearRenderTargetView(rtvHandle, Colors::LightSteelBlue, 0, nullptr);
    mCommandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);
    mCommandList->OMSetRenderTargets(1, &rtvHandle, true, &dsvHandle);

    RenderSystem::Render(mRegistry, mCommandList.Get());

    tr = CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
    mCommandList->ResourceBarrier(1, &tr);
    mCommandList->Close();

    ID3D12CommandList* cmdLists[] = { mCommandList.Get() };
    mCommandQueue->ExecuteCommandLists(_countof(cmdLists), cmdLists);
    mSwapChain->Present(0, 0);
    mCurrBackBuffer = (mCurrBackBuffer + 1) % SwapChainBufferCount;
    FlushCommandQueue();
}

void App::CreateResourceManager(ID3D12Device14* device)
{
    mResourceManager = std::make_unique<ResourceManager>(device);
}

void App::CreateObject(XMFLOAT3 pos, XMFLOAT3 scale)
{
    Entity entity = EntityFactory::CreateCube(mRegistry, mResourceManager.get(), pos, scale);
    mRegistry.AddComponent(entity, GravityComponent{ -9.8f, true });
    mEntities.push_back(entity);
}

void App::CreateUIObject(XMFLOAT3 pos, XMFLOAT3 scale)
{
    Entity uiEntity = EntityFactory::CreateUI(mRegistry, mResourceManager.get(), pos, scale);
    mEntities.push_back(uiEntity);
}

void App::CreateMesh()
{
    Entity meshEntity = EntityFactory::CreateMesh(L"assets/bibian", mRegistry, mResourceManager.get(), XMFLOAT3(0, 0, 0), XMFLOAT3(1, 1, 1));
    mEntities.push_back(meshEntity);
}

void App::DestroyObject(Entity entity)
{
    auto it = std::find(mEntities.begin(), mEntities.end(), entity);
    if (it != mEntities.end())
    {
        mEntities.erase(it);
        auto& renders = mRegistry.GetComponentMap<RenderComponent>();
        renders.erase(entity);
        OutputDebugStringA("Entity destroyed!\n");
    }
    if (mSelectedEntity == entity) {
        mSelectedEntity = UINT32_MAX;
    }
}

bool App::InitMainWindow()
{
    WNDCLASS wc = {};
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = MainWndProc;
    wc.hInstance = mhAppInst;
    wc.lpszClassName = L"MainWnd";

    if (!RegisterClass(&wc))
    {
        MessageBox(0, L"RegisterClass Failed.", 0, 0);
        return false;
    }

    RECT R = { 0, 0, (long)mClientWidth, (long)mClientHeight };
    AdjustWindowRect(&R, WS_OVERLAPPEDWINDOW, false);
    int width = R.right - R.left;
    int height = R.bottom - R.top;

    mhMainWnd = CreateWindow(L"MainWnd", L"ReZero",
        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, width, height, 0, 0, mhAppInst, 0);
    if (!mhMainWnd)
    {
        MessageBox(0, L"CreateWindow Failed.", 0, 0);
        return false;
    }

    ShowWindow(mhMainWnd, SW_SHOW);
    UpdateWindow(mhMainWnd);
    return true;
}

bool App::InitDirect3D()
{
    UINT flags = 0;
#if defined(DEBUG) || defined(_DEBUG)
    {
        ComPtr<ID3D12Debug> debugController;
        if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
        {
            debugController->EnableDebugLayer();
            flags = DXGI_CREATE_FACTORY_DEBUG;
        }
    }
#endif

    ThrowIfFailed(CreateDXGIFactory2(flags, IID_PPV_ARGS(&mdxgiFactory)));

    HRESULT hardwareResult = D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&md3dDevice));
    if (FAILED(hardwareResult))
    {
        ComPtr<IDXGIAdapter> pWarpAdapter;
        ThrowIfFailed(mdxgiFactory->EnumWarpAdapter(IID_PPV_ARGS(&pWarpAdapter)));
        ThrowIfFailed(D3D12CreateDevice(pWarpAdapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&md3dDevice)));
    }

    ThrowIfFailed(md3dDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&mFence)));
    mFenceEvent = CreateEventEx(nullptr, nullptr, false, EVENT_ALL_ACCESS);

    mRtvDescriptorSize = md3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    mDsvDescriptorSize = md3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
    mCbvSrvUavDescriptorSize = md3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    CreateCommandObjects();
    CreateSwapChain();
    CreateRtvAndDsvDescriptorHeaps();

    return true;
}

void App::CreateCommandObjects()
{
    D3D12_COMMAND_LIST_TYPE type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    D3D12_COMMAND_QUEUE_DESC queueDesc = {};
    queueDesc.Type = type;
    ThrowIfFailed(md3dDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(mCommandQueue.GetAddressOf())));
    ThrowIfFailed(md3dDevice->CreateCommandAllocator(type, IID_PPV_ARGS(mCommandAlloc.GetAddressOf())));
    ThrowIfFailed(md3dDevice->CreateCommandList(0, type, mCommandAlloc.Get(), nullptr, IID_PPV_ARGS(mCommandList.GetAddressOf())));
    mCommandList->Close();
}

void App::CreateSwapChain()
{
    DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
    swapChainDesc.Width = mClientWidth;
    swapChainDesc.Height = mClientHeight;
    swapChainDesc.Format = mBackBufferFormat;
    swapChainDesc.BufferCount = SwapChainBufferCount;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.SampleDesc.Quality = 0;
    ThrowIfFailed(mdxgiFactory->CreateSwapChainForHwnd(mCommandQueue.Get(), mhMainWnd, &swapChainDesc, NULL, NULL, mSwapChain.GetAddressOf()));
}

void App::CreateRtvAndDsvDescriptorHeaps()
{
    D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
    rtvHeapDesc.NumDescriptors = SwapChainBufferCount;
    rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    ThrowIfFailed(md3dDevice->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(mRtvHeap.GetAddressOf())));

    D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
    dsvHeapDesc.NumDescriptors = 1;
    dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    ThrowIfFailed(md3dDevice->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(mDsvHeap.GetAddressOf())));
}

void App::OnResize()
{
    FlushCommandQueue();
    ThrowIfFailed(mCommandList->Reset(mCommandAlloc.Get(), nullptr));

    for (int i = 0; i < SwapChainBufferCount; ++i)
        mSwapChainBuffer[i].Reset();
    mDepthStencilBuffer.Reset();

    ThrowIfFailed(mSwapChain->ResizeBuffers(SwapChainBufferCount, mClientWidth, mClientHeight, mBackBufferFormat, DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH));

    mCurrBackBuffer = 0;

    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHeapHandle(mRtvHeap->GetCPUDescriptorHandleForHeapStart());
    for (UINT i = 0; i < SwapChainBufferCount; i++)
    {
        ThrowIfFailed(mSwapChain->GetBuffer(i, IID_PPV_ARGS(&mSwapChainBuffer[i])));
        md3dDevice->CreateRenderTargetView(mSwapChainBuffer[i].Get(), nullptr, rtvHeapHandle);
        rtvHeapHandle.Offset(1, mRtvDescriptorSize);
    }

    D3D12_RESOURCE_DESC depthStencilDesc = {};
    depthStencilDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    depthStencilDesc.Width = mClientWidth;
    depthStencilDesc.Height = mClientHeight;
    depthStencilDesc.DepthOrArraySize = 1;
    depthStencilDesc.MipLevels = 1;
    depthStencilDesc.Format = DXGI_FORMAT_R24G8_TYPELESS;
    depthStencilDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
	depthStencilDesc.SampleDesc.Count = 1;
	depthStencilDesc.SampleDesc.Quality = 0;

    D3D12_CLEAR_VALUE optClear = {};
    optClear.Format = mDepthStencilFormat;
    optClear.DepthStencil.Depth = 1.0f;

    CD3DX12_HEAP_PROPERTIES heapProperties(D3D12_HEAP_TYPE_DEFAULT);
    ThrowIfFailed(md3dDevice->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &depthStencilDesc, D3D12_RESOURCE_STATE_COMMON, &optClear, IID_PPV_ARGS(mDepthStencilBuffer.GetAddressOf())));

    D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
    dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
    dsvDesc.Format = mDepthStencilFormat;
    md3dDevice->CreateDepthStencilView(mDepthStencilBuffer.Get(), &dsvDesc, DepthStencilView());

    auto barr = CD3DX12_RESOURCE_BARRIER::Transition(mDepthStencilBuffer.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_DEPTH_WRITE);
    mCommandList->ResourceBarrier(1, &barr);

    ThrowIfFailed(mCommandList->Close());
    ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
    mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);
    FlushCommandQueue();

    mScreenViewport = { 0.0f, 0.0f, static_cast<float>(mClientWidth), static_cast<float>(mClientHeight), 0.0f, 1.0f };
    mScissorRect = { 0, 0, (long)mClientWidth, (long)mClientHeight };

    XMMATRIX P = XMMatrixPerspectiveFovLH(0.25f * MathHelper::Pi, AspectRatio(), 1.0f, 1000.0f);
    XMStoreFloat4x4(&mProj, P);
}

void App::FlushCommandQueue()
{
    mCurrentFence++;
    ThrowIfFailed(mCommandQueue->Signal(mFence.Get(), mCurrentFence));

    if (mFence->GetCompletedValue() < mCurrentFence)
    {
        ThrowIfFailed(mFence->SetEventOnCompletion(mCurrentFence, mFenceEvent));
        WaitForSingleObject(mFenceEvent, INFINITE);
    }
}

void App::CleanUp()
{
    if (md3dDevice != nullptr)
        FlushCommandQueue();

    if (mFenceEvent != nullptr)
    {
        CloseHandle(mFenceEvent);
        mFenceEvent = nullptr;
    }
    mEntities.clear();
}

void App::OnMouseDown(WPARAM btnState, int x, int y)
{
    mLastMousePos.x = x;
    mLastMousePos.y = y;
    SetCapture(mhMainWnd);

    if ((btnState & MK_LBUTTON) != 0)
    {
        Entity clickedUI = mLogic.PickUI(mRegistry, x, y);
        if (clickedUI != UINT32_MAX)
        {
            auto& gravityMap = mRegistry.GetComponentMap<GravityComponent>();
            for (auto& [entity, gravity] : gravityMap)
            {
                gravity.strength *= -1.0f;
                gravity.isActive = true;
            }
            return;
        }

        DirectX::XMVECTOR rayOrigin, rayDir;
        mLogic.ScreenPointToWorldRay(mClientWidth, mClientHeight, x, y, mView, mProj, rayOrigin, rayDir);

        if (mSelectedEntity != UINT32_MAX)
        {
            auto& renders = mRegistry.GetComponentMap<RenderComponent>();
            if (renders.find(mSelectedEntity) != renders.end())
                renders[mSelectedEntity].isSelected = false;
        }

        Entity pickedEntity = mLogic.PickObject(mRegistry, rayOrigin, rayDir);
        if (pickedEntity != UINT32_MAX)
        {
            mSelectedEntity = pickedEntity;
            auto& renders = mRegistry.GetComponentMap<RenderComponent>();
            if (renders.find(mSelectedEntity) != renders.end())
                renders[mSelectedEntity].isSelected = true;
        }
        else
        {
            mSelectedEntity = UINT32_MAX;
        }
    }
}

void App::OnMouseUp(WPARAM btnState, int x, int y)
{
    ReleaseCapture();
}

void App::OnMouseMove(WPARAM btnState, int x, int y)
{
    if ((btnState & MK_LBUTTON) != 0)
    {
        float dx = XMConvertToRadians(0.25f * static_cast<float>(x - mLastMousePos.x));
        float dy = XMConvertToRadians(0.25f * static_cast<float>(y - mLastMousePos.y));
        mTheta -= dx;
        mPhi -= dy;
        mPhi = MathHelper::Clamp(mPhi, 0.1f, XM_PI - 0.1f);
    }
    else if ((btnState & MK_RBUTTON) != 0)
    {
        float dx = 0.005f * static_cast<float>(x - mLastMousePos.x);
        float dy = 0.005f * static_cast<float>(y - mLastMousePos.y);
        mRadius += dx - dy;
        mRadius = MathHelper::Clamp(mRadius, 3.0f, 15.0f);
    }
    mLastMousePos.x = x;
    mLastMousePos.y = y;
}

ID3D12Resource* App::CurrentBackBuffer()const
{
    return mSwapChainBuffer[mCurrBackBuffer].Get();
}

D3D12_CPU_DESCRIPTOR_HANDLE App::CurrentBackBufferView()const
{
    return CD3DX12_CPU_DESCRIPTOR_HANDLE(mRtvHeap->GetCPUDescriptorHandleForHeapStart(), mCurrBackBuffer, mRtvDescriptorSize);
}

D3D12_CPU_DESCRIPTOR_HANDLE App::DepthStencilView()const
{
    return mDsvHeap->GetCPUDescriptorHandleForHeapStart();
}