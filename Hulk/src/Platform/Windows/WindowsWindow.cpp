// This is a personal academic project. Dear PVS-Studio, please check it.

// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "hkpch.h"
#include"WindowsWindow.h"

#include <iostream>

#include "../../Hulk/Events/ApplicationEvent.h"


extern  HULK_API LRESULT im_gui_impl_win32_wnd_proc_handler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace Hulk
{
	WindowsWindow* WindowsWindow::mWindowsWnObject = nullptr;
	


	LRESULT HULK_API CALLBACK MainWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	// Forward hwnd on because we can get messages (e.g., WM_CREATE)
	// before CreateWindow returns, and thus before mhMainWnd is valid.

    return WindowsWindow::GetWindowsWindowObject()->MsgProc(hwnd, msg, wParam, lParam);
}

	
	Scope<Window> WindowsWindow::Create(const WindowProps & props)
	{
		Scope<WindowsWindow>wn = std::make_unique<WindowsWindow>(props);
		
		return wn;
	}

	LRESULT WindowsWindow::MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{   
	if (im_gui_impl_win32_wnd_proc_handler(hwnd, msg, wParam, lParam))
		return true;
	
	switch( msg )
	{
	// WM_ACTIVATE is sent when the window is activated or deactivated.  
	// We pause the game when the window is deactivated and unpause it 
	// when it becomes active.  
	case WM_ACTIVATE:
		if( LOWORD(wParam) == WA_INACTIVE )
		{
			D3DApp::GetApp()->mAppPaused = true;
			D3DApp::GetApp()->mTimer.Stop();
		}
		else
		{
			D3DApp::GetApp()->mAppPaused = false;
			D3DApp::GetApp()->mTimer.Start();
		}
		return 0;

	// WM_SIZE is sent when the user resizes the window.  
	case WM_SIZE:
		// Save the new client area dimensions.
			m_Data.Width  = LOWORD(lParam);
			m_Data.Height = HIWORD(lParam);
		if( D3DApp::GetApp()->md3dDevice )
		{
			if( wParam == SIZE_MINIMIZED )
			{
				D3DApp::GetApp()->mAppPaused = true;
				D3DApp::GetApp()->mMinimized = true;
				D3DApp::GetApp()->mMaximized = false;
			}
			else if( wParam == SIZE_MAXIMIZED )
			{
				D3DApp::GetApp()->mAppPaused = false;
				D3DApp::GetApp()->mMinimized = false;
				D3DApp::GetApp()->mMaximized = true;
				//ImGui_ImplDX12_InvalidateDeviceObjects();
				D3DApp::GetApp()->OnResize();
				WindowResizeEvent e(m_Data.Width,m_Data.Height);
				HK_TRACE(e);
				//ImGui_ImplDX12_CreateDeviceObjects();
			}
			else if( wParam == SIZE_RESTORED )
			{
				
				// Restoring from minimized state?
				if( D3DApp::GetApp()->mMinimized )
				{
					D3DApp::GetApp()->mAppPaused = false;
					D3DApp::GetApp()->mMinimized = false;
					//ImGui_ImplDX12_InvalidateDeviceObjects();
					D3DApp::GetApp()->OnResize();
					WindowResizeEvent e(m_Data.Width,m_Data.Height);
					HK_TRACE(e);
					//ImGui_ImplDX12_CreateDeviceObjects();
				}

				// Restoring from maximized state?
				else if( D3DApp::GetApp()->mMaximized )
				{
					D3DApp::GetApp()->mAppPaused = false;
					D3DApp::GetApp()->mMaximized = false;
					//ImGui_ImplDX12_InvalidateDeviceObjects();
					D3DApp::GetApp()->OnResize();
					WindowResizeEvent e(m_Data.Width,m_Data.Height);
		            HK_TRACE(e);
					//ImGui_ImplDX12_CreateDeviceObjects();
				}
				else if( D3DApp::GetApp()->mResizing )
				{
					// If user is dragging the resize bars, we do not resize 
					// the buffers here because as the user continuously 
					// drags the resize bars, a stream of WM_SIZE messages are
					// sent to the window, and it would be pointless (and slow)
					// to resize for each WM_SIZE message received from dragging
					// the resize bars.  So instead, we reset after the user is 
					// done resizing the window and releases the resize bars, which 
					// sends a WM_EXITSIZEMOVE message.
				}
				else // API call such as SetWindowPos or mSwapChain->SetFullscreenState.
				{
					//ImGui_ImplDX12_InvalidateDeviceObjects();
					D3DApp::GetApp()->OnResize();
					WindowResizeEvent e(m_Data.Width,m_Data.Height);
					HK_TRACE(e);
					//ImGui_ImplDX12_CreateDeviceObjects();
				}
			}
		}
		return 0;

	// WM_EXITSIZEMOVE is sent when the user grabs the resize bars.
	case WM_ENTERSIZEMOVE:
		D3DApp::GetApp()->mAppPaused = true;
		D3DApp::GetApp()->mResizing  = true;
		D3DApp::GetApp()->mTimer.Stop();
		return 0;

	// WM_EXITSIZEMOVE is sent when the user releases the resize bars.
	// Here we reset everything based on the new window dimensions.
	case WM_EXITSIZEMOVE:

		D3DApp::GetApp()->mAppPaused = false;
		D3DApp::GetApp()->mResizing  = false;
		D3DApp::GetApp()->mTimer.Start();
		D3DApp::GetApp()->OnResize();
		
		{
		WindowResizeEvent e(m_Data.Width,m_Data.Height);
		 HK_TRACE(e);
		}
		
		return 0;
 
	// WM_DESTROY is sent when the window is being destroyed.
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;

	// The WM_MENUCHAR message is sent when a menu is active and the user presses 
	// a key that does not correspond to any mnemonic or accelerator key. 
	case WM_MENUCHAR:
        // Don't beep when we alt-enter.
        return MAKELRESULT(0, MNC_CLOSE);

	// Catch this message so to prevent the window from becoming too small.
	case WM_GETMINMAXINFO:
		((MINMAXINFO*)lParam)->ptMinTrackSize.x = 200;
		((MINMAXINFO*)lParam)->ptMinTrackSize.y = 200; 
		return 0;

	case WM_LBUTTONDOWN:
	case WM_MBUTTONDOWN:
	case WM_RBUTTONDOWN:
		D3DApp::GetApp()->OnMouseDown(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		return 0;
	case WM_LBUTTONUP:
	case WM_MBUTTONUP:
	case WM_RBUTTONUP:
		D3DApp::GetApp()->OnMouseUp(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));// the sandbox version gets called because i created a sandbox app object in the winmain func :D
		return 0;
	case WM_MOUSEMOVE:
		D3DApp::GetApp()->OnMouseMove(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		return 0;
    case WM_KEYUP:
        if(wParam == VK_ESCAPE)
        {
            PostQuitMessage(0);
        }
        else if((int)wParam == VK_F2)
            //Set4xMsaaState(!m4xMsaaState);

        return 0;
	}

	return DefWindowProc(hwnd, msg, wParam, lParam);
}

	
	WindowsWindow::WindowsWindow(const WindowProps& props)
	{
		HK_PROFILE_FUNCTION();
		
		assert(mWindowsWnObject == nullptr);
		WindowsWindow::mWindowsWnObject = this;
		mWndProps = props;
		
		
		
	}

	WindowsWindow::~WindowsWindow()
	{
		HK_PROFILE_FUNCTION();

		
	}

	void WindowsWindow::OnUpdate(MSG msg )
	{
		
		
		
            TranslateMessage( &msg );
            DispatchMessage( &msg );
		
	}

	bool WindowsWindow::Init(const WindowProps& props)
	{
		m_Data.Title = props.Title;
		m_Data.Width = props.Width;
		m_Data.Height = props.Height;
		m_Data.mhAppInst = (HINSTANCE) props.mhAppInst;
		//HK_CORE_INFO("Creating window {0} ({1}, {2})", props.Title, props.Width, props.Height);
		
		
		WNDCLASS wc;
		wc.style         = CS_HREDRAW | CS_VREDRAW;
		wc.lpfnWndProc   = MainWndProc; 
		wc.cbClsExtra    = 0;
		wc.cbWndExtra    = 0;
		wc.hInstance     = m_Data.mhAppInst;
		wc.hIcon         = LoadIcon(0, IDI_APPLICATION);
		wc.hCursor       = LoadCursor(0, IDC_ARROW);
		wc.hbrBackground = (HBRUSH)GetStockObject(NULL_BRUSH);
		wc.lpszMenuName  = 0;
		wc.lpszClassName = L"MainWnd";

		if( !RegisterClass(&wc) )
		{
			MessageBox(0, L"RegisterClass Failed.", 0, 0);
			return false;
		}

		// Compute window rectangle dimensions based on requested client area dimensions.
		RECT R = { 0, 0, m_Data.Width, m_Data.Height };
		AdjustWindowRect(&R, WS_OVERLAPPEDWINDOW, false);
		int width  = R.right - R.left;
		int height = R.bottom - R.top;

		mhMainWnd = CreateWindow(L"MainWnd", s2ws(m_Data.Title).c_str(), 
			WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, width, height, 0, 0, m_Data.mhAppInst, 0);
		if( !mhMainWnd )
		{
			MessageBox(0, L"CreateWindow Failed.", 0, 0);
			return false;
		}

		ShowWindow(mhMainWnd, SW_SHOW);
		UpdateWindow(mhMainWnd);

		


		return true;
		
	}


	
}
