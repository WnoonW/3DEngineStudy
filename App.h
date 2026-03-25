#pragma once

#include "AppUtill.h"
#include "ResourceManager.h"
#include "Object.h"
#include "GameTimer.h"
#include "InputLayout.h"
#include "RingBuffer.h"
#include "UpdateLogic.h"
#include "ECS.h"
#include "PhysicsSystem.h"

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


	//유틸
	float     AspectRatio() {
		return static_cast<float>(mClientWidth) / mClientHeight;
	}
protected:
	//메인 로직
	virtual bool Initialize();
	void Update(const GameTimer& gt);
	void Draw();

	//클래스 생성
	void CreateResourceManager(ID3D12Device14* device);
	void CreateObject(XMFLOAT3 pos, XMFLOAT3 scale);
	void CreateUIObject(XMFLOAT3 pos, XMFLOAT3 scale);
	void DestroyObject(Object* object);

protected:
	//초기화
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
	//입력
	virtual void OnMouseDown(WPARAM btnState, int x, int y);
	virtual void OnMouseUp(WPARAM btnState, int x, int y);
	virtual void OnMouseMove(WPARAM btnState, int x, int y);

protected:
	static App* mApp;
	HINSTANCE mhAppInst = nullptr;
	HWND      mhMainWnd = nullptr;

	bool      m4xMsaaState = false;    // 4X MSAA enabled
	UINT      m4xMsaaQuality = 0;      // quality level of 4X MSAA

	//Objects
	std::vector<std::unique_ptr<Object>> mObjects;
	Object* mSelectedObject = nullptr;

	//Class
	std::unique_ptr<ResourceManager> mResourceManager = nullptr;
	GameTimer mTimer;
	Input mInputLayout;
	logic mUpdateLogic;
	RingBuffer rbKeyDown{16};
	RingBuffer rbKeyUp{16};
	Registry mRegistry;

	//SwapChain
	ComPtr<IDXGISwapChain1> mSwapChain = nullptr;
	static const int SwapChainBufferCount = 2;
	int mCurrBackBuffer = 0;

	//Device
	ComPtr<IDXGIFactory7> mdxgiFactory = nullptr;
	ComPtr<ID3D12Device14> md3dDevice = nullptr;
	ComPtr<ID3D12Fence> mFence;
	UINT64 mCurrentFence = 0;
	
	//Events
	HANDLE mFenceEvent = nullptr;

	//Command Objects
	ComPtr<ID3D12CommandQueue> mCommandQueue = nullptr;
	ComPtr<ID3D12CommandAllocator> mCommandAlloc = nullptr;
	ComPtr<ID3D12GraphicsCommandList10> mCommandList = nullptr;

	//Resources
	ComPtr<ID3D12Resource> mSwapChainBuffer[SwapChainBufferCount];
	ComPtr<ID3D12Resource> mDepthStencilBuffer;

	//Descriptor Heaps
	ComPtr<ID3D12DescriptorHeap> mRtvHeap;
	ComPtr<ID3D12DescriptorHeap> mDsvHeap;

	D3D12_VIEWPORT mScreenViewport;
	D3D12_RECT mScissorRect;

	//Size
	UINT mRtvDescriptorSize = 0;
	UINT mDsvDescriptorSize = 0;
	UINT mCbvSrvUavDescriptorSize = 0;

	//Format
	D3D_DRIVER_TYPE md3dDriverType = D3D_DRIVER_TYPE_HARDWARE;
	DXGI_FORMAT mBackBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
	DXGI_FORMAT mDepthStencilFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

	int mClientWidth = 800;
	int mClientHeight = 600;

protected:
	XMFLOAT4X4 Imatrix = 
		XMFLOAT4X4(
		1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f);
	XMFLOAT4X4 mWorld = Imatrix;
	XMFLOAT4X4 mView = Imatrix;
	XMFLOAT4X4 mProj = Imatrix;

	float mTheta = 1.5f * XM_PI;
	float mPhi = XM_PIDIV4;
	float mRadius = 5.0f;

	POINT mLastMousePos = { 0, 0 };


	float a = 0;
	float d = 0;
};