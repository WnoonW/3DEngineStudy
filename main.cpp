#include "App.h"

class RenderApp : public App
{
public:
	RenderApp(HINSTANCE hInstance);
	virtual bool Initialize() override;

public:
	void* g_pMeshObj = nullptr;

};

int WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE prevInstance, _In_ PSTR cmdLine, _In_ int showCmd)
{
	RenderApp app(hInstance);
	try 
	{
		app.Initialize();
	}
	catch (DxException& e)
	{
		MessageBox(nullptr, e.ToString().c_str(), L"HR Failed", MB_OK);
		return 0;
	}

	return app.Run();
}

RenderApp::RenderApp(HINSTANCE hInstance) : App(hInstance)
{
}

bool RenderApp::Initialize()
{
	if (!App::Initialize())
		return false;

	return true;
}