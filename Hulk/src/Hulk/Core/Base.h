#pragma once

#include <memory>

#include "PlatformDetection.h"

#ifdef HK_DEBUG
	#if defined(HK_PLATFORM_WINDOWS)
		#define HK_DEBUGBREAK() __debugbreak()
	#elif defined(HK_PLATFORM_LINUX)
		#include <signal.h>
		#define HK_DEBUGBREAK() raise(SIGTRAP)
	#else
		#error "Platform doesn't support debugbreak yet!"
	#endif
	#define HK_ENABLE_ASSERTS
#else
	#define HK_DEBUGBREAK()
#endif

#define HK_EXPAND_MACRO(x) x
#define HK_STRINGIFY_MACRO(x) #x

#define BIT(x) (1 << x)

#define HK_BIND_EVENT_FN(fn) [this](auto&&... args) -> decltype(auto) { return this->fn(std::forward<decltype(args)>(args)...); }

namespace Hulk {

	template<typename T>
	using Scope = std::unique_ptr<T>;
	template<typename T, typename ... Args>
	constexpr Scope<T> CreateScope(Args&& ... args)
	{
		return std::make_unique<T>(std::forward<Args>(args)...);
	}

	template<typename T>
	using Ref = std::shared_ptr<T>;
	template<typename T, typename ... Args>
	constexpr Ref<T> CreateRef(Args&& ... args)
	{
		return std::make_shared<T>(std::forward<Args>(args)...);
	}

}