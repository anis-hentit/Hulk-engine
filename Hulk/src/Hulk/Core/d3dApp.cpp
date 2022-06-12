

#include "hkpch.h"
#include "d3dApp.h"

#include "../Events/ApplicationEvent.h"
#include "../Debug//Instrumentor.h"
#include "Log.h"
#include "../../Platform/Windows/WindowsWindow.h"


using Microsoft::WRL::ComPtr;
using namespace std;
using namespace DirectX;




namespace Hulk {


	void D3DApp::ImGuiInitialize(HWND hwnd, ID3D12Device * device,int num_frames_in_flight,
				         DXGI_FORMAT rendertargetformart)
{

	auto fonthandle =(CD3DX12_GPU_DESCRIPTOR_HANDLE) ImguiSrvHeap->GetGPUDescriptorHandleForHeapStart();
	fonthandle.Offset(imguiDescriptorOffset, mCbvSrvUavDescriptorSize);
	auto cpuHandle = (CD3DX12_CPU_DESCRIPTOR_HANDLE)ImguiSrvHeap->GetCPUDescriptorHandleForHeapStart();
	cpuHandle.Offset(imguiDescriptorOffset, mCbvSrvUavDescriptorSize);

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	(void)io;

	ImGui::StyleColorsDark();
	ImGui_ImplWin32_Init(hwnd);
	ImGui_ImplDX12_Init(device, num_frames_in_flight, rendertargetformart,
			    ImguiSrvHeap.Get(), cpuHandle, fonthandle);// the srv heap is unused here ill look into the
                                                   //  func definition for more info
	
		
}


	
  D3DApp* D3DApp::mApp = nullptr;
	
  D3DApp* D3DApp::GetApp()
{
    return mApp;
}

  D3DApp::D3DApp() 
{
	// Only one D3DApp can be constructed.
	assert(mApp == nullptr);
	mApp = this;
	
}

  D3DApp::~D3DApp()
{
	if(md3dDevice != nullptr)
		FlushCommandQueue();
	
}

  HINSTANCE D3DApp::AppInst()const
{
	return mhAppInst;
}
  void D3DApp::SetAppInst(HINSTANCE hInstance)
{
	mhAppInst = hInstance;
}
	
  Window& D3DApp::MainWnd()const // returns ref to the Window object
{
	return *mMainWnd;
}

  float D3DApp::AspectRatio()const
{
	
	return static_cast<float>(float(mMainWnd->GetWidth()) / (float)mMainWnd->GetHeight());
}

  bool D3DApp::Get4xMsaaState()const
{
    return m4xMsaaState;
}

  void D3DApp::Set4xMsaaState(bool value)
{
    if(m4xMsaaState != value)
    {
        m4xMsaaState = value;

        // Recreate the swapchain and buffers with new multisample settings.
        CreateSwapChain();
        OnResize();
    }
}

  int D3DApp::Run()
{
	HK_PROFILE_FUNCTION();
	MSG msg = {0};
 
	mTimer.Reset();
	
	while(msg.message != WM_QUIT)
	{
		// If there are Window messages then process them.
		if(PeekMessage( &msg, 0, 0, 0, PM_REMOVE ))
		{
            mMainWnd->OnUpdate(msg);
		}
		// Otherwise, do animation/game stuff.
		
		else
        {
			
			mTimer.Tick();
			
			if( !mAppPaused )
			{
				CalculateFrameStats();
				Update(mTimer);
				ImGuiUpdate();
                Draw(mTimer);
				
			}
			else
			{
				Sleep(100);
			}
        }
    }
	
	return (int)msg.wParam;
}

  bool D3DApp::Initialize(HINSTANCE hInstance)
{
	mhAppInst = hInstance;
  	
	
	InitMainWindow();
	HK_CORE_INFO("Initialized Main Window!");
	
	if(!InitDirect3D())
		return false;
	// Do the initial resize code.
	OnResize();

	HK_CORE_INFO("Initialized Direct3D!");
    

	return true;
}
 
  void D3DApp::CreateRtvAndDsvDescriptorHeaps()
{
    D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc;
    rtvHeapDesc.NumDescriptors = SwapChainBufferCount;
    rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	rtvHeapDesc.NodeMask = 0;
    ThrowIfFailed(md3dDevice->CreateDescriptorHeap(
        &rtvHeapDesc, IID_PPV_ARGS(mRtvHeap.GetAddressOf())));


    D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc;
    dsvHeapDesc.NumDescriptors = 1;
    dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	dsvHeapDesc.NodeMask = 0;

    ThrowIfFailed(md3dDevice->CreateDescriptorHeap(
        &dsvHeapDesc, IID_PPV_ARGS(mDsvHeap.GetAddressOf())));

	HK_CORE_INFO("Created rtv and dsv heaps!");
}

  void D3DApp::OnResize()
{
	assert(md3dDevice);
	assert(mSwapChain);
    assert(mDirectCmdListAlloc);

	// Flush before changing any resources.
	FlushCommandQueue();

    ThrowIfFailed(mCommandList->Reset(mDirectCmdListAlloc.Get(), nullptr));

	// Release the previous resources we will be recreating.
	for (int i = 0; i < SwapChainBufferCount; ++i)
		mSwapChainBuffer[i].Reset();
    mDepthStencilBuffer.Reset();
	
	// Resize the swap chain.
    ThrowIfFailed(mSwapChain->ResizeBuffers(
		SwapChainBufferCount, 
		mMainWnd->GetWidth(), mMainWnd->GetHeight(), 
		mBackBufferFormat, 
		DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH));

	mCurrBackBuffer = 0;
 
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHeapHandle(mRtvHeap->GetCPUDescriptorHandleForHeapStart());
	for (UINT i = 0; i < SwapChainBufferCount; i++)
	{
		ThrowIfFailed(mSwapChain->GetBuffer(i, IID_PPV_ARGS(mSwapChainBuffer[i].GetAddressOf())));
		md3dDevice->CreateRenderTargetView(mSwapChainBuffer[i].Get(), nullptr, rtvHeapHandle);
		rtvHeapHandle.Offset(1, mRtvDescriptorSize);
	}

    // Create the depth/stencil buffer and view.
    D3D12_RESOURCE_DESC depthStencilDesc;
    depthStencilDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    depthStencilDesc.Alignment = 0;
    depthStencilDesc.Width = mMainWnd->GetWidth();
    depthStencilDesc.Height = mMainWnd->GetHeight();
    depthStencilDesc.DepthOrArraySize = 1;
    depthStencilDesc.MipLevels = 1;

	// Correction 11/12/2016: SSAO chapter requires an SRV to the depth buffer to read from 
	// the depth buffer.  Therefore, because we need to create two views to the same resource:
	//   1. SRV format: DXGI_FORMAT_R24_UNORM_X8_TYPELESS
	//   2. DSV Format: DXGI_FORMAT_D24_UNORM_S8_UINT
	// we need to create the depth buffer resource with a typeless format.  
	depthStencilDesc.Format = DXGI_FORMAT_R24G8_TYPELESS;

    depthStencilDesc.SampleDesc.Count = m4xMsaaState ? 4 : 1;
    depthStencilDesc.SampleDesc.Quality = m4xMsaaState ? (m4xMsaaQuality - 1) : 0;
    depthStencilDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    depthStencilDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

    D3D12_CLEAR_VALUE optClear;
    optClear.Format = mDepthStencilFormat;
    optClear.DepthStencil.Depth = 1.0f;
    optClear.DepthStencil.Stencil = 0;
    ThrowIfFailed(md3dDevice->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
        &depthStencilDesc,
		D3D12_RESOURCE_STATE_COMMON,
        &optClear,
        IID_PPV_ARGS(mDepthStencilBuffer.GetAddressOf())));

    // Create descriptor to mip level 0 of entire resource using the format of the resource.
	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc;
	dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	dsvDesc.Format = mDepthStencilFormat;
	dsvDesc.Texture2D.MipSlice = 0;
    md3dDevice->CreateDepthStencilView(mDepthStencilBuffer.Get(), &dsvDesc, DepthStencilView());

    // Transition the resource from its initial state to be used as a depth buffer.
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mDepthStencilBuffer.Get(),
		D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_DEPTH_WRITE));
	
    // Execute the resize commands.
    ThrowIfFailed(mCommandList->Close());
    ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
    mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	// Wait until resize is complete.
	FlushCommandQueue();

	// Update the viewport transform to cover the client area.
	mScreenViewport.TopLeftX = 0;
	mScreenViewport.TopLeftY = 0;
	mScreenViewport.Width    = static_cast<float>(mMainWnd->GetWidth());
	mScreenViewport.Height   = static_cast<float>(mMainWnd->GetHeight());
	mScreenViewport.MinDepth = 0.0f;
	mScreenViewport.MaxDepth = 1.0f;

    mScissorRect = { 0, 0, (LONG)mMainWnd->GetWidth(), (LONG)mMainWnd->GetHeight() };
}


 



  void  HULK_API D3DApp::ImGuiUpdate()
{
	HK_PROFILE_FUNCTION();
	
	ImGui_ImplDX12_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();
	
	//ImGui::GetIO();
		
	if (show_demo_window)
		ImGui::ShowDemoWindow(&show_demo_window);

	{
		
		

		ImGui::Begin("Hello, world!");                          // Create a window called "Hello, world!" and append into it.

		ImGui::Text("This is some useful text.");               // Display some text (you can use a format strings too)
		ImGui::Checkbox("WireFrame Mode", &mIsWireframe);      // Edit bools storing our window open/close state
		

		ImGui::SliderInt("Switch scenes", &SceneIndex, 0, 1); 
		
		ImGui::ColorEdit3("clear color", (float*)&clear_color); // Edit 3 floats representing a color
		

		

		ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
		ImGui::End();
	}
	
}

	void D3DApp::RenderOverlay(ID3D12GraphicsCommandList* cmdlist)
	{
	ImGui::Render();
	ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), cmdlist);
	}

	void D3DApp::Shutdown()
	{
	
	ImGui_ImplDX12_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
	
	}

	bool D3DApp::InitMainWindow()
{	
	// only windows api window for now-------
	WindowProps wp;
  	
  	wp.mhAppInst = mhAppInst;
	mMainWnd = WindowsWindow::Create(wp);
	mMainWnd->Init(mMainWnd->GetWnProps());

	return true;
}

  bool D3DApp::InitDirect3D()
{
#if defined(DEBUG) || defined(_DEBUG) 
	// Enable the D3D12 debug layer.
	{
		ComPtr<ID3D12Debug> spDebugController0;
		ComPtr<ID3D12Debug1> spDebugController1;
		ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(&spDebugController0)));
		ThrowIfFailed(spDebugController0->QueryInterface(IID_PPV_ARGS(&spDebugController1)));
		spDebugController1->SetEnableGPUBasedValidation(true);

		ComPtr<ID3D12Debug> debugController;

		ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)));
		debugController->EnableDebugLayer();
		
		HK_CORE_INFO("Debug Layer Enabled");
	}
#endif

	ThrowIfFailed(CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG,IID_PPV_ARGS(&mdxgiFactory)));

	// Try to create hardware device.
	HRESULT hardwareResult = D3D12CreateDevice(
		nullptr,             // default adapter
		D3D_FEATURE_LEVEL_12_0,
		IID_PPV_ARGS(&md3dDevice));

	// Fallback to WARP device.
	if(FAILED(hardwareResult))
	{
		ComPtr<IDXGIAdapter> pWarpAdapter;
		ThrowIfFailed(mdxgiFactory->EnumWarpAdapter(IID_PPV_ARGS(&pWarpAdapter)));

		ThrowIfFailed(D3D12CreateDevice(
			pWarpAdapter.Get(),
			D3D_FEATURE_LEVEL_11_0,
			IID_PPV_ARGS(&md3dDevice)));
	}

	ThrowIfFailed(md3dDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE,
		IID_PPV_ARGS(&mFence)));

	mRtvDescriptorSize = md3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	mDsvDescriptorSize = md3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
	mCbvSrvUavDescriptorSize = md3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    // Check 4X MSAA quality support for our back buffer format.
    // All Direct3D 11 capable devices support 4X MSAA for all render 
    // target formats, so we only need to check quality support.

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
	
#ifdef _DEBUG
    LogAdapters();
#endif

	CreateCommandObjects();
    CreateSwapChain();
    CreateRtvAndDsvDescriptorHeaps();

	return true;
}

  void D3DApp::CreateCommandObjects()
{
	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	ThrowIfFailed(md3dDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(mCommandQueue.GetAddressOf())));

	ThrowIfFailed(md3dDevice->CreateCommandAllocator(
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		IID_PPV_ARGS(mDirectCmdListAlloc.GetAddressOf())));

	ThrowIfFailed(md3dDevice->CreateCommandList(
		0,
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		mDirectCmdListAlloc.Get(), // Associated command allocator
		nullptr,                   // Initial PipelineStateObject
		IID_PPV_ARGS(mCommandList.GetAddressOf())));
	
	// Start off in a closed state.  This is because the first time we refer 
	// to the command list we will Reset it, and it needs to be closed before
	// calling Reset.
	mCommandList->Close();

	HK_CORE_INFO("Created command queue,command allocator and command list!");
}

  void D3DApp::CreateSwapChain()
{
    // Release the previous swapchain we will be recreating.
    mSwapChain.Reset();
	

    DXGI_SWAP_CHAIN_DESC sd;
    sd.BufferDesc.Width = mMainWnd->GetWidth();
    sd.BufferDesc.Height = mMainWnd->GetHeight();
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.BufferDesc.Format = mBackBufferFormat;
    sd.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
    sd.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
    sd.SampleDesc.Count = m4xMsaaState ? 4 : 1;
    sd.SampleDesc.Quality = m4xMsaaState ? (m4xMsaaQuality - 1) : 0;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.BufferCount = SwapChainBufferCount;
    sd.OutputWindow = (HWND)mMainWnd->GetNativeWindow();
    sd.Windowed = true;
	sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

	// Note: Swap chain uses queue to perform flush.
    ThrowIfFailed(mdxgiFactory->CreateSwapChain(
		mCommandQueue.Get(),
		&sd, 
		mSwapChain.GetAddressOf()));
	
	HK_CORE_INFO("Created Swap chain!");
}

  void D3DApp::FlushCommandQueue()
{
	// Advance the fence value to mark commands up to this fence point.
    mCurrentFence++;

    // Add an instruction to the command queue to set a new fence point.  Because we 
	// are on the GPU timeline, the new fence point won't be set until the GPU finishes
	// processing all the commands prior to this Signal().
    ThrowIfFailed(mCommandQueue->Signal(mFence.Get(), mCurrentFence));

	// Wait until the GPU has completed commands up to this fence point.
    if(mFence->GetCompletedValue() < mCurrentFence)
	{
		HANDLE eventHandle = CreateEventEx(nullptr, false, false, EVENT_ALL_ACCESS);

        // Fire event when GPU hits current fence.  
        ThrowIfFailed(mFence->SetEventOnCompletion(mCurrentFence, eventHandle));

        // Wait until the GPU hits current fence event is fired.
		WaitForSingleObject(eventHandle, INFINITE);
        CloseHandle(eventHandle);
	}
}

  ID3D12Resource* D3DApp::CurrentBackBuffer()const
{
	return mSwapChainBuffer[mCurrBackBuffer].Get();
}

  D3D12_CPU_DESCRIPTOR_HANDLE D3DApp::CurrentBackBufferView()const
{
	return CD3DX12_CPU_DESCRIPTOR_HANDLE(
		mRtvHeap->GetCPUDescriptorHandleForHeapStart(),
		mCurrBackBuffer,
		mRtvDescriptorSize);
}

  D3D12_CPU_DESCRIPTOR_HANDLE D3DApp::DepthStencilView()const
{
	return mDsvHeap->GetCPUDescriptorHandleForHeapStart();
}

  void D3DApp::CalculateFrameStats()
{
	// Code computes the average frames per second, and also the 
	// average time it takes to render one frame.  These stats 
	// are appended to the window caption bar.
    
	static int frameCnt = 0;
	static float timeElapsed = 0.0f;

	frameCnt++;

	// Compute averages over one second period.
	if( (mTimer.TotalTime() - timeElapsed) >= 1.0f )
	{
		float fps = (float)frameCnt; // fps = frameCnt / 1
		float mspf = 1000.0f / fps;

        wstring fpsStr = to_wstring(fps);
        wstring mspfStr = to_wstring(mspf);

        wstring windowText = s2ws(mMainWnd->GetTitle()) +
            L"    fps: " + fpsStr +
            L"   mspf: " + mspfStr;

        SetWindowText((HWND)mMainWnd->GetNativeWindow(), windowText.c_str());
		
		// Reset for next average.
		frameCnt = 0;
		timeElapsed += 1.0f;
	}
}

  void D3DApp::LogAdapters()
{
    UINT i = 0;
    IDXGIAdapter* adapter = nullptr;
    std::vector<IDXGIAdapter*> adapterList;

    while(mdxgiFactory->EnumAdapters(i, &adapter) != DXGI_ERROR_NOT_FOUND)
    {
        DXGI_ADAPTER_DESC desc;
        adapter->GetDesc(&desc);

        std::wstring text = L"***Adapter: ";
        text += desc.Description;
        text += L"\n";

        OutputDebugString(text.c_str());
    	
    	auto input =text.c_str();
    	size_t size = (wcslen(input) + 1) * sizeof(wchar_t);
		char *buffer = new char[size];
    	
		std::wcstombs(buffer, input, size);
		
    	 HK_CORE_INFO(buffer);

	delete[] buffer;

        adapterList.push_back(adapter);
        
        ++i;
    }

    for(size_t i = 0; i < adapterList.size(); ++i)
    {
        LogAdapterOutputs(adapterList[i]);
        ReleaseCom(adapterList[i]);
    }
}

  void D3DApp::LogAdapterOutputs(IDXGIAdapter* adapter)
{
    UINT i = 0;
    IDXGIOutput* output = nullptr;
    while(adapter->EnumOutputs(i, &output) != DXGI_ERROR_NOT_FOUND)
    {
        DXGI_OUTPUT_DESC desc;
        output->GetDesc(&desc);
        
        std::wstring text = L"***Output: ";
        text += desc.DeviceName;
        text += L"\n";
        OutputDebugString(text.c_str());
    	
		auto input =text.c_str();
    	size_t size = (wcslen(input) + 1) * sizeof(wchar_t);
		char *buffer = new char[size];
    	
		std::wcstombs(buffer, input, size);
		
    	 HK_CORE_INFO(buffer);

	delete[] buffer;
    	
        LogOutputDisplayModes(output, mBackBufferFormat);

        ReleaseCom(output);

        ++i;
    }
}

  void D3DApp::LogOutputDisplayModes(IDXGIOutput* output, DXGI_FORMAT format)
{
    UINT count = 0;
    UINT flags = 0;

    // Call with nullptr to get list count.
    output->GetDisplayModeList(format, flags, &count, nullptr);

    std::vector<DXGI_MODE_DESC> modeList(count);
    output->GetDisplayModeList(format, flags, &count, &modeList[0]);

    for(auto& x : modeList)
    {
        UINT n = x.RefreshRate.Numerator;
        UINT d = x.RefreshRate.Denominator;
        std::wstring text =
            L"Width = " + std::to_wstring(x.Width) + L" " +
            L"Height = " + std::to_wstring(x.Height) + L" " +
            L"Refresh = " + std::to_wstring(n) + L"/" + std::to_wstring(d) +
            L"\n";
    	
		
        ::OutputDebugString(text.c_str());

		auto input = text.c_str();
		size_t size = (wcslen(input) + 1) * sizeof(wchar_t);
		char* buffer = new char[size];

		std::wcstombs(buffer, input, size);

		HK_CORE_INFO("Display Mode");
		HK_CORE_INFO(buffer);

		delete[] buffer;
    	
    }
}
}
