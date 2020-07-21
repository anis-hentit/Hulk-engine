#pragma once
#ifdef HK_PLATFORM_WINDOWS
    #ifdef HK_BUILD_DLL
        #define HULK_API __declspec(dllexport)
    #else
        #define HULK_API __declspec(dllimport)
    #endif
#endif
