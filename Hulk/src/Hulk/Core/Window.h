#pragma once

#include "Base.h"
#include"../Events/Event.h"

namespace Hulk
{

	struct HULK_API WindowProps
	{
		std::string Title;
		uint32_t Width;
		uint32_t Height;
		void* mhAppInst = nullptr;

		WindowProps(const std::string& title = "Hulk Engine",
			        uint32_t width = 1240,
			        uint32_t height = 900)
			: Title(title), Width(width), Height(height)
		{
		}
	};

	// Interface representing a desktop system based Window
	class HULK_API Window
	{
	public:
		using EventCallbackFn = std::function<void(Event&)>;

		virtual ~Window() = default;

		virtual bool Init(const WindowProps& props)=0;
		
		//virtual void OnUpdate();
		virtual void OnUpdate(MSG msg) = 0;

		virtual uint32_t GetWidth() const = 0;
		virtual uint32_t GetHeight() const = 0;
		virtual std::string GetTitle() const = 0;
		virtual WindowProps GetWnProps() const = 0;
		// Window attributes
		virtual void SetEventCallback(const EventCallbackFn& callback) = 0;
		virtual void SetVSync(bool enabled) = 0;
		virtual bool IsVSync() const = 0;

		virtual void* GetNativeWindow() const = 0;

		static Scope<Window> Create(const WindowProps& props = WindowProps());
	};





	
}