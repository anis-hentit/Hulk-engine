#pragma once

#include <memory>
#include"Core.h"
#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_color_sinks.h"


namespace Hulk {
	
class HULK_API Log
{
	
public:
	static void Init();
	inline static std::shared_ptr<spdlog::logger>& GetCoreLogger (){ return m_CoreLogger;}
	inline static std::shared_ptr<spdlog::logger>& GetClientLogger (){ return m_ClientLogger;}
private:
	static std::shared_ptr<spdlog::logger> m_CoreLogger;
	static std::shared_ptr<spdlog::logger> m_ClientLogger;
};

}

#define HK_CORE_ERROR(...) ::Hulk::Log::GetCoreLogger()->error(__VA_ARGS__)
#define HK_CORE_WARN(...)  ::Hulk::Log::GetCoreLogger()->warn(__VA_ARGS__)
#define HK_CORE_INFO(...)  ::Hulk::Log::GetCoreLogger()->info(__VA_ARGS__)
#define HK_CORE_TRACE(...) ::Hulk::Log::GetCoreLogger()->trace(__VA_ARGS__)
#define HK_CORE_FATAL(...) ::Hulk::Log::GetCoreLogger()->fatal(__VA_ARGS__)

#define HK_ERROR(...)      ::Hulk::Log::GetClientLogger()->error(__VA_ARGS__)
#define HK_WARN(...)       ::Hulk::Log::GetClientLogger()->warn(__VA_ARGS__)
#define HK_INFO(...)       ::Hulk::Log::GetClientLogger()->info(__VA_ARGS__)
#define HK_TRACE(...)      ::Hulk::Log::GetClientLogger()->trace(__VA_ARGS__)
#define HK_FATAL(...)      ::Hulk::Log::GetClientLogger()->fatal(__VA_ARGS__)