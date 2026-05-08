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
    // 단순화: 매 프레임 키 하나만 처리
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

    // 카메라
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

// ... (CreateResourceManager, CreateObject 등 기존 코드 유지)
