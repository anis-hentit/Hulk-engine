#pragma once
#include "../../../Core.h"

#ifdef HK_PLATFORM_WINDOWS

extern D3DApp* Hulk::CreateApplication();


int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE prevInstance,
	PSTR cmdLine, int showCmd)
{
	// Enable run-time memory check for debug builds.
#if defined(DEBUG) | defined(_DEBUG)
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	
#endif
	try
	{	
		auto theApp = Hulk::CreateApplication();
		
		if (!theApp->Initialize(hInstance))
		{	
			return 0;
		}
		
		return theApp->Run();
	}
	catch (DxException& e)
	{
		MessageBox(nullptr, e.ToString().c_str(), L"HR Failed", MB_OK);
		
		return 0;
	}
}


int main(int argc,char** argv)
{
	Hulk::Log::Init();
	HK_CORE_INFO("Initialized Log!");
	
	//entry point
	WinMain(GetModuleHandle(NULL), NULL, GetCommandLineA(), SW_SHOWNORMAL);
	

	return 0;
}

#endif