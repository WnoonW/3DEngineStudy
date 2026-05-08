#pragma once

#include "AppUtill.h"
#include "ResourceManager.h"
#include "InputLayout.h"
#include "GameTimer.h"
#include "RingBuffer.h"
#include "UpdateLogic.h"
#include "ECS.h"
#include "PhysicsSystem.h"
#include "EntityFactory.h"
#include "Systems.h"

#pragma comment(lib,"d3dcompiler.lib")
#pragma comment(lib, "D3D12.lib")
#pragma comment(lib, "dxgi.lib")

using Microsoft::WRL::ComPtr;
using namespace DirectX;   // D 단계: 전체 통일

class App
{
protected:
    App(HINSTANCE hInstance);
    ~App();

public:
    static App* GetApp();
    int Run();
    void CleanUp();
    virtual LRESULT MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

    float AspectRatio() { return static_cast<float>(mClientWidth) / mClientHeight; }

protected:
    virtual bool Initialize();
    void Update(const GameTimer& gt);
    void Draw();

    void CreateResourceManager(ID3D12Device14* device);
    void CreateObject(XMFLOAT3 pos, XMFLOAT3 scale);
    void CreateUIObject(XMFLOAT3 pos, XMFLOAT3 scale);
    void DestroyObject(Entity entity);
    void CreateMesh();

protected:
    bool InitMainWindow();
    bool InitDirect3D();
    void CreateCommandObjects();
    void CreateSwapChain();
    void CreateRtvAndDsvDescriptorHeaps();
    void OnResize();
    void FlushCommandQueue();

    ID3D12Resource* CurrentBackBuffer()const;
    D3D12_CPU_DESCRIPTOR_HANDLE CurrentBackBufferView()const;
    D3D12_CPU_DESCRIPTOR_HANDLE DepthStencilView()const;

private:
    virtual void OnMouseDown(WPARAM btnState, int x, int y);
    virtual void OnMouseUp(WPARAM btnState, int x, int y);
    virtual void OnMouseMove(WPARAM btnState, int x, int y);

protected:
    static App* mApp;

    HINSTANCE mhAppInst = nullptr;
    HWND      mhMainWnd = nullptr;

    bool      mAppPaused = false;
    bool      mMinimized = false;
    bool      mMaximized = false;
    bool      mResizing = false;

    ComPtr<ID3D12Device14> md3dDevice;
    ComPtr<IDXGIFactory4> mdxgiFactory;
    ComPtr<ID3D12CommandQueue> mCommandQueue;
    ComPtr<ID3D12CommandAllocator> mCommandAlloc;
    ComPtr<ID3D12GraphicsCommandList10> mCommandList;
    ComPtr<IDXGISwapChain1> mSwapChain;
    ComPtr<ID3D12DescriptorHeap> mRtvHeap;
    ComPtr<ID3D12DescriptorHeap> mDsvHeap;
    ComPtr<ID3D12Resource> mSwapChainBuffer[2];
    ComPtr<ID3D12Resource> mDepthStencilBuffer;

    HANDLE mFenceEvent = nullptr;
    UINT64 mCurrentFence = 0;
    ComPtr<ID3D12Fence> mFence;

    D3D_DRIVER_TYPE md3dDriverType = D3D_DRIVER_TYPE_HARDWARE;
    DXGI_FORMAT mBackBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
    DXGI_FORMAT mDepthStencilFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

    bool      m4xMsaaState = false;
    UINT      m4xMsaaQuality = 0;

    int mCurrBackBuffer = 0;
    static const int SwapChainBufferCount = 2;
    UINT mRtvDescriptorSize = 0;
    UINT mDsvDescriptorSize = 0;
    UINT mCbvSrvUavDescriptorSize = 0;

    D3D12_VIEWPORT mScreenViewport;
    D3D12_RECT mScissorRect;

    UINT mClientWidth = 800;
    UINT mClientHeight = 600;

    GameTimer mTimer;
    InputLayout mInputLayout;

    std::unique_ptr<ResourceManager> mResourceManager;
    Registry mRegistry;
    std::vector<Entity> mEntities;
    Entity mSelectedEntity = UINT32_MAX;

    logic mLogic;

    // 디버그용 변수 (D 단계에서 주석 처리)
    float a = 0;
    float d = 0;

    RingBuffer<KeyCode, 256> rbKeyDown;
    RingBuffer<KeyCode, 256> rbKeyUp;

    float mTheta = 1.5f * XM_PI;
    float mPhi = XM_PIDIV4;
    float mRadius = 5.0f;

    XMFLOAT4X4 mView = MathHelper::Identity4x4();
    XMFLOAT4X4 mProj = MathHelper::Identity4x4();

    POINT mLastMousePos;
};