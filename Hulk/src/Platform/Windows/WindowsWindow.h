#pragma once

#include "../../Hulk/Core/Window.h"
#include <WindowsX.h>
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
		
		
	    LRESULT MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
		
		unsigned int GetWidth() const override { return m_Data.Width; }
		unsigned int GetHeight() const override { return m_Data.Height; }
		std::string GetTitle()const override { return m_Data.Title;}
		// Window attributes
		void SetEventCallback(const EventCallbackFn& callback) override { m_Data.EventCallback = callback; }
		void SetVSync(bool enabled) override {m_Data.VSync = enabled;};
		bool IsVSync() const override {return m_Data.VSync;};

		virtual void* GetNativeWindow() const { return mhMainWnd; }
	private:
		virtual bool Init(const WindowProps& props);
		//virtual void Shutdown();
	private:
		HINSTANCE mhAppInst = nullptr;
		HWND mhMainWnd = nullptr; //main window handle
	    static WindowsWindow* mWindowsWnObject;
		//Scope<GraphicsContext> m_Context;

		struct WindowData
		{
			std::string Title;
			unsigned int Width, Height;
			bool VSync;

			EventCallbackFn EventCallback;
		};

		WindowData m_Data;
	};
}