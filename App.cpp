#include "App.h"
#include <DirectXColors.h>

using Microsoft::WRL::ComPtr;

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

    // ECS Factory를 통한 생성
    CreateObject(XMFLOAT3(0, 0, 0), XMFLOAT3(1, 1, 1));
    // CreateUIObject(XMFLOAT3(0,0,0), XMFLOAT3(1, 1, 1)); // 필요에 따라 Factory에 UI 생성 함수 추가 후 사용

    return true;
}

LRESULT App::MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
        // WM_ACTIVATE is sent when the window is activated or deactivated.  
        // We pause the game when the window is deactivated and unpause it 
        // when it becomes active.  

    // ─────────────── 마우스 ───────────────
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

        // ─────────────── 키보드 ───────────────
    case WM_KEYDOWN:
    case WM_SYSKEYDOWN:
    {
        KeyCode keyDown = mInputLayout.ToKeyCode(wParam);

        // 추가: KeyCode를 RingBuffer에 푸시 (1바이트 크기)
        if (!rbKeyDown.Push(&keyDown, sizeof(KeyCode))) {
            // 버퍼가 꽉 찼을 때 처리
            OutputDebugStringA("RingBuffer full: Key input dropped.\n");
        }

        break;
    }

    case WM_KEYUP:
    case WM_SYSKEYUP:
    {
        KeyCode keyUp = mInputLayout.ToKeyCode(wParam);

        // 추가: KeyCode를 RingBuffer에 푸시 (1바이트 크기)
        if (!rbKeyUp.Push(&keyUp, sizeof(KeyCode))) {
            // 버퍼가 꽉 찼을 때 처리
            OutputDebugStringA("RingBuffer full: Key input dropped.\n");
        }

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
    rbKeyUp.Pop(&key, sizeof(KeyCode));
    if (key == KeyCode::A)
    {
        a++;
        CreateObject(XMFLOAT3(-a, 0, 0), XMFLOAT3(1, 1, 1));
    }
    else if (key == KeyCode::D)
    {
        d++;
        CreateObject(XMFLOAT3(d, 0, 0), XMFLOAT3(1, 1, 1));
    }
    else if (key == KeyCode::Z)
    {
        if (mSelectedEntity != UINT32_MAX)
        {
            DestroyObject(mSelectedEntity);
        }
    }

    float dt = gt.DeltaTime();

    // 1. 물리 시스템 업데이트 (중력 등)
    PhysicsSystem::Update(dt, mRegistry);

    // 2. Transform 시스템 업데이트 (행렬 재계산)
    TransformSystem::Update(mRegistry);

    // 카메라 계산
    float x = mRadius * sinf(mPhi) * cosf(mTheta);
    float z = mRadius * sinf(mPhi) * sinf(mTheta);
    float y = mRadius * cosf(mPhi);

    XMVECTOR pos = XMVectorSet(x, y, z, 1.0f);
    XMVECTOR target = XMVectorZero();
    XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

    XMMATRIX view = XMMatrixLookAtLH(pos, target, up);
    XMStoreFloat4x4(&mView, view);

    XMMATRIX proj = XMMatrixPerspectiveFovLH(0.25f * MathHelper::Pi, AspectRatio(), 1.0f, 1000.0f);

    // 3. Render 시스템 상수 버퍼(CBV) 갱신
    RenderSystem::UpdateConstants(mRegistry, view, proj);
}

void App::Draw()
{
    // Render 준비
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

    // --- ECS 렌더 시스템 호출 (이 한 줄이 모든 Object->Render를 대체합니다!) ---
    RenderSystem::Render(mRegistry, mCommandList.Get());

    // Render 종료
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
    // EntityFactory를 사용해 Entity 생성 후 반환
    Entity entity = EntityFactory::CreateCube(mRegistry, mResourceManager.get(), pos, scale);

    // PhysicsSystem을 위한 중력 컴포넌트 추가
    mRegistry.AddComponent(entity, GravityComponent{ -9.8f, true });

    mEntities.push_back(entity);
}

void App::CreateUIObject(XMFLOAT3 pos, XMFLOAT3 scale)
{
    // 추후 EntityFactory::CreateUI(...) 를 만들어서 호출하도록 구성합니다.
}

void App::DestroyObject(Entity entity)
{
    // 렌더링이 안되도록 렌더 컴포넌트를 비활성화하거나, 목록에서 지웁니다.
    auto it = std::find(mEntities.begin(), mEntities.end(), entity);
    if (it != mEntities.end())
    {
        // 간단한 삭제 처리 (실제 ECS에서는 Registry->RemoveEntity 기능 필요)
        mEntities.erase(it);

        // 렌더링에서 제외시키기 위해 꼼수로 RenderComponent 맵에서 삭제 가능
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
    WNDCLASS wc;
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = MainWndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = mhAppInst;
    wc.hIcon = NULL;
    wc.hCursor = NULL;
    wc.hbrBackground = NULL;
    wc.lpszMenuName = NULL;
    wc.lpszClassName = L"MainWnd";

    if (!RegisterClass(&wc))
    {
        MessageBox(0, L"RegisterClass Failed.", 0, 0);
        return false;
    }

    RECT R = { 0, 0, mClientWidth, mClientHeight };
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

            // GBV 켜기 (ID3D12Debug1 필요)
            ComPtr<ID3D12Debug1> debugController1;
            if (SUCCEEDED(debugController.As(&debugController1)))
            {
                debugController1->SetEnableGPUBasedValidation(TRUE);
            }

            ComPtr<ID3D12Debug3> debug3;
            if (SUCCEEDED(debugController.As(&debug3)))
            {
                // 이제 에러 없이 호출 가능해야 합니다.
                //debug3->SetEnableShaderBasedValidation(TRUE);
                debug3->SetEnableGPUBasedValidation(TRUE);
            }

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


    D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS msQualityLevels;
    msQualityLevels.Format = mBackBufferFormat;
    msQualityLevels.SampleCount = 4;
    msQualityLevels.Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;
    msQualityLevels.NumQualityLevels = 0;
    ThrowIfFailed(md3dDevice->CheckFeatureSupport(
        D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS,
        &msQualityLevels,
        sizeof(msQualityLevels)));

    m4xMsaaQuality = msQualityLevels.NumQualityLevels;
    assert(m4xMsaaQuality > 0 && "Unexpected MSAA quality level.");


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
    queueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
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
    swapChainDesc.Stereo = false;
    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.SampleDesc.Quality = 0;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.BufferCount = SwapChainBufferCount;
    swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
    swapChainDesc.Flags = 0;

    ThrowIfFailed(mdxgiFactory->CreateSwapChainForHwnd(mCommandQueue.Get(), mhMainWnd, &swapChainDesc, NULL, NULL, mSwapChain.GetAddressOf()));
}

void App::CreateRtvAndDsvDescriptorHeaps()
{
    D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc;
    rtvHeapDesc.NumDescriptors = SwapChainBufferCount;
    rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    rtvHeapDesc.NodeMask = 0;
    ThrowIfFailed(md3dDevice->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(mRtvHeap.GetAddressOf())));


    D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc;
    dsvHeapDesc.NumDescriptors = 1;
    dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    dsvHeapDesc.NodeMask = 0;
    ThrowIfFailed(md3dDevice->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(mDsvHeap.GetAddressOf())));
}

void App::OnResize()
{
    assert(mSwapChain);
    assert(md3dDevice);
    assert(mCommandAlloc);

    FlushCommandQueue();

    ThrowIfFailed(mCommandList->Reset(mCommandAlloc.Get(), nullptr));

    for (int i = 0; i < SwapChainBufferCount; ++i)
        mSwapChainBuffer[i].Reset();
    mDepthStencilBuffer.Reset();

    ThrowIfFailed(mSwapChain->ResizeBuffers(
        SwapChainBufferCount,
        mClientWidth, mClientHeight,
        mBackBufferFormat,
        DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH));

    mCurrBackBuffer = 0;

    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHeapHandle(mRtvHeap->GetCPUDescriptorHandleForHeapStart());
    for (UINT i = 0; i < SwapChainBufferCount; i++)
    {
        ThrowIfFailed(mSwapChain->GetBuffer(i, IID_PPV_ARGS(&mSwapChainBuffer[i])));
        md3dDevice->CreateRenderTargetView(mSwapChainBuffer[i].Get(), nullptr, rtvHeapHandle);
        rtvHeapHandle.Offset(1, mRtvDescriptorSize);
    }

    D3D12_RESOURCE_DESC depthStencilDesc;
    depthStencilDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    depthStencilDesc.Alignment = 0;
    depthStencilDesc.Width = mClientWidth;
    depthStencilDesc.Height = mClientHeight;
    depthStencilDesc.DepthOrArraySize = 1;
    depthStencilDesc.MipLevels = 1;
    depthStencilDesc.Format = DXGI_FORMAT_R24G8_TYPELESS;
    depthStencilDesc.SampleDesc.Count = m4xMsaaState ? 4 : 1;
    depthStencilDesc.SampleDesc.Quality = m4xMsaaState ? (m4xMsaaQuality - 1) : 0;
    depthStencilDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    depthStencilDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

    D3D12_CLEAR_VALUE optClear;
    optClear.Format = mDepthStencilFormat;
    optClear.DepthStencil.Depth = 1.0f;
    optClear.DepthStencil.Stencil = 0;

    auto heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
    ThrowIfFailed(md3dDevice->CreateCommittedResource(
        &heapProperties,
        D3D12_HEAP_FLAG_NONE,
        &depthStencilDesc,
        D3D12_RESOURCE_STATE_COMMON,
        &optClear,
        IID_PPV_ARGS(mDepthStencilBuffer.GetAddressOf())));

    D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc;
    dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
    dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
    dsvDesc.Format = mDepthStencilFormat;
    dsvDesc.Texture2D.MipSlice = 0;
    md3dDevice->CreateDepthStencilView(mDepthStencilBuffer.Get(), &dsvDesc, DepthStencilView());

    auto barr = CD3DX12_RESOURCE_BARRIER::Transition(mDepthStencilBuffer.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_DEPTH_WRITE);
    mCommandList->ResourceBarrier(1, &barr);

    // Execute the resize commands.
    ThrowIfFailed(mCommandList->Close());
    ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
    mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

    // Wait until resize is complete.
    FlushCommandQueue();

    // Update the viewport transform to cover the client area.
    mScreenViewport.TopLeftX = 0;
    mScreenViewport.TopLeftY = 0;
    mScreenViewport.Width = static_cast<float>(mClientWidth);
    mScreenViewport.Height = static_cast<float>(mClientHeight);
    mScreenViewport.MinDepth = 0.0f;
    mScreenViewport.MaxDepth = 1.0f;

    mScissorRect = { 0, 0, mClientWidth, mClientHeight };

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
    CloseHandle(mFenceEvent);
    mFenceEvent = nullptr;
    mObjects.clear();
}

//Control
void App::OnMouseDown(WPARAM btnState, int x, int y)
{
    if (btnState & MK_LBUTTON)
    {

        XMVECTOR rayOrigin, rayDir;
        mUpdateLogic.ScreenPointToWorldRay(mClientWidth, mClientHeight, x, y, mView, mProj, rayOrigin, rayDir);


        Object* selected = mUpdateLogic.PickObject(mObjects, rayOrigin, rayDir);

        if (selected)
        {
            if (mSelectedObject)
            {
                mSelectedObject->isSelected = false; // 이전에 선택된 오브젝트가 있다면 불 끄기
            }
            mSelectedObject = selected;  // 멤버 변수에 저장
            mSelectedObject->isSelected = true;  // 선택된 오브젝트 표시 (예: 색상 변경)
            OutputDebugStringA("Object selected!\n");
        }
        else
        {
            if (mSelectedObject)
            {
                mSelectedObject->isSelected = false;
                mSelectedObject = nullptr;
            }
        }
    }

    mLastMousePos.x = x;
    mLastMousePos.y = y;

    SetCapture(mhMainWnd);
}

void App::OnMouseUp(WPARAM btnState, int x, int y)
{
    ReleaseCapture();
}

void App::OnMouseMove(WPARAM btnState, int x, int y)
{
    if ((btnState & MK_LBUTTON) != 0)
    {
        // Make each pixel correspond to a quarter of a degree.
        float dx = XMConvertToRadians(0.25f * static_cast<float>(x - mLastMousePos.x));
        float dy = XMConvertToRadians(0.25f * static_cast<float>(y - mLastMousePos.y));

        // Update angles based on input to orbit camera around box.
        mTheta -= dx;
        mPhi -= dy;

        // Restrict the angle mPhi.
        mPhi = MathHelper::Clamp(mPhi, 0.1f, XM_PI - 0.1f);
    }
    else if ((btnState & MK_RBUTTON) != 0)
    {
        // Make each pixel correspond to 0.005 unit in the scene.
        float dx = 0.005f * static_cast<float>(x - mLastMousePos.x);
        float dy = 0.005f * static_cast<float>(y - mLastMousePos.y);

        // Update the camera radius based on input.
        mRadius += dx - dy;

        // Restrict the radius.
        mRadius = MathHelper::Clamp(mRadius, 3.0f, 15.0f);
    }

    mLastMousePos.x = x;
    mLastMousePos.y = y;
}

//Get
ID3D12Resource* App::CurrentBackBuffer()const
{
    return mSwapChainBuffer[mCurrBackBuffer].Get();
}

D3D12_CPU_DESCRIPTOR_HANDLE App::CurrentBackBufferView()const
{
    D3D12_CPU_DESCRIPTOR_HANDLE handle =
        CD3DX12_CPU_DESCRIPTOR_HANDLE(
            mRtvHeap->GetCPUDescriptorHandleForHeapStart(),
            mCurrBackBuffer,
            mRtvDescriptorSize);
    return handle;
}

D3D12_CPU_DESCRIPTOR_HANDLE App::DepthStencilView()const
{
    return mDsvHeap->GetCPUDescriptorHandleForHeapStart();
}