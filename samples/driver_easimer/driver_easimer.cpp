// === Copyright (c) 2017-2020 easimer.net. All rights reserved. ===

#include <openvr_driver.h>
#include "driverlog.h"
#include "driver_easimer.h"

#if defined( _WINDOWS )
#include <windows.h>
#endif

#if defined(_WIN32)
#define HMD_DLL_EXPORT extern "C" __declspec( dllexport )
#define HMD_DLL_IMPORT extern "C" __declspec( dllimport )
#elif defined(__GNUC__) || defined(COMPILER_GCC) || defined(__APPLE__)
#define HMD_DLL_EXPORT extern "C" __attribute__((visibility("default")))
#define HMD_DLL_IMPORT extern "C" 
#else
#error "Unsupported Platform."
#endif

using namespace vr;

HMD_DLL_EXPORT void* HmdDriverFactory(const char* pInterfaceName, int* pReturnCode)
{
	if (strcmp(IServerTrackedDeviceProvider_Version, pInterfaceName) == 0) {
		//return &g_serverDriver;
	}
	
	if (strcmp(IVRWatchdogProvider_Version, pInterfaceName) == 0) {
		// return &g_watchdog;
	}

	if (pReturnCode) {
		*pReturnCode = VRInitError_Init_InterfaceNotFound;
	}

	return NULL;
}
