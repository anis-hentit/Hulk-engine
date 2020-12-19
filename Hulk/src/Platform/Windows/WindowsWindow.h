#pragma once

#include "../../Hulk/Core/Window.h"
#include "../../Hulk/Core/d3dApp.h"

namespace Hulk
{
	class HULK_API WindowsWindow : public Window
	{
		public:
		WindowsWindow(const WindowProps& props);
		virtual ~WindowsWindow();

		void OnUpdate(MSG msg) override;
		static WindowsWindow* GetWindowsWindowObject(){return mWindowsWnObject;}
		
		static Scope<Window> Create(const WindowProps& props = WindowProps());
		
	    LRESULT MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
		
		unsigned int GetWidth() const override { return m_Data.Width; }
		unsigned int GetHeight() const override { return m_Data.Height; }
		std::string GetTitle()const override { return m_Data.Title;}
		// Window attributes
		void SetEventCallback(const EventCallbackFn& callback) override { m_Data.EventCallback = callback; }
		void SetVSync(bool enabled) override {m_Data.VSync = enabled;};
		bool IsVSync() const override {return m_Data.VSync;};
		WindowProps GetWnProps() const override {return mWndProps;}
		virtual void* GetNativeWindow() const { return mhMainWnd; }
	private:
		virtual bool Init(const WindowProps& props);
		
		//virtual void Shutdown();
	private:
		//main windows app handle
		HWND mhMainWnd = nullptr; //main window handle
	    static WindowsWindow* mWindowsWnObject;
		//Scope<GraphicsContext> m_Context; TO BE IMPLEMENTED 

		struct WindowData
		{
			std::string Title;
			int Width, Height;
			bool VSync;
			HINSTANCE mhAppInst = nullptr;
			
			EventCallbackFn EventCallback;
		};

		WindowProps mWndProps;
		WindowData m_Data;
		
	};
}