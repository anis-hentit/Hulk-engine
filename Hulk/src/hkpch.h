#pragma once



#include <iostream>
#include <memory>
#include <utility>
#include <algorithm>
#include <functional>

#include <string>
#include <sstream>
#include <array>
#include <vector>
#include <unordered_map>
#include <unordered_set>

#include "Hulk/Core/Base.h"

#include "Hulk/Core/Log.h"

#include "Hulk/Debug/Instrumentor.h"

#ifdef HK_PLATFORM_WINDOWS
	#include <Windows.h>
    #include <WindowsX.h>
    #include <wrl/client.h>
#endif
