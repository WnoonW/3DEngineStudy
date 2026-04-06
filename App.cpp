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