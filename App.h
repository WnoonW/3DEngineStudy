#pragma once

#include "AppUtill.h"
#include "ResourceManager.h"
// Object.h 삭제됨
#include "GameTimer.h"
#include "InputLayout.h"
#include "RingBuffer.h"
#include "UpdateLogic.h"
#include "ECS.h"
#include "PhysicsSystem.h"
#include "EntityFactory.h" // 새로 추가됨
#include "Systems.h"       // 새로 추가됨

#pragma comment(lib,"d3dcompiler.lib")
#pragma comment(lib, "D3D12.lib")
#pragma comment(lib, "dxgi.lib")

using Microsoft::WRL::ComPtr;

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

    // 유틸
    float AspectRatio() {
        return static_cast<float>(mClientWidth) / mClientHeight;
    }

protected:
    // 메인 로직
    virtual bool Initialize();
    void Update(const GameTimer& gt);
    void Draw();

    // 클래스 생성
    void CreateResourceManager(ID3D12Device14* device);
    void CreateObject(DirectX::XMFLOAT3 pos, DirectX::XMFLOAT3 scale);
    void CreateUIObject(DirectX::XMFLOAT3 pos, DirectX::XMFLOAT3 scale);
    void DestroyObject(Entity entity); // 포인터 대신 Entity ID 사용

protected:
    // 초기화
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
    // 입력
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

    // 디바이스 자원
    ComPtr<ID3D12Device14> md3dDevice;
    ComPtr<IDXGIFactory4> mdxgiFactory;
    ComPtr<ID3D12CommandQueue> mCommandQueue;
    ComPtr<ID3D12CommandAllocator> mCommandAlloc;
    ComPtr<ID3D12GraphicsCommandList10> mCommandList;
    ComPtr<IDXGISwapChain> mSwapChain;
    ComPtr<ID3D12DescriptorHeap> mRtvHeap;
    ComPtr<ID3D12DescriptorHeap> mDsvHeap;
    ComPtr<ID3D12Resource> mSwapChainBuffer[2];
    ComPtr<ID3D12Resource> mDepthStencilBuffer;

    int mCurrBackBuffer = 0;
    static const int SwapChainBufferCount = 2;
    UINT mRtvDescriptorSize = 0;
    UINT mDsvDescriptorSize = 0;

    D3D12_VIEWPORT mScreenViewport;
    D3D12_RECT mScissorRect;

    UINT mClientWidth = 800;
    UINT mClientHeight = 600;

    GameTimer mTimer;
    InputLayout mInputLayout;

    // --- ECS 구조 반영 ---
    std::unique_ptr<ResourceManager> mResourceManager;
    Registry mRegistry;
    std::vector<Entity> mEntities;          // 관리용 Entity 리스트
    Entity mSelectedEntity = UINT32_MAX;    // 선택된 Entity ID (포인터 대신 ID)

    logic mLogic;

    float a = 0;
    float d = 0;

    RingBuffer<KeyCode, 256> rbKeyDown;
    RingBuffer<KeyCode, 256> rbKeyUp;

    // 카메라 파라미터
    float mTheta = 1.5f * DirectX::XM_PI;
    float mPhi = DirectX::XM_PIDIV4;
    float mRadius = 5.0f;

    DirectX::XMFLOAT4X4 mView = MathHelper::Identity4x4();
    DirectX::XMFLOAT4X4 mProj = MathHelper::Identity4x4();

    POINT mLastMousePos;
};