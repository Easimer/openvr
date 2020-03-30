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

#define HMD_SCRIPT_PATH "drivers/easimer/scripts/driver_easimer.lua"

using namespace vr;

static CServerDriver g_serverDriver;
static CWatchdogDriver g_watchdog;
static bool g_bExiting = false;

HMD_DLL_EXPORT void* HmdDriverFactory(const char* pInterfaceName, int* pReturnCode)
{
	if (strcmp(IServerTrackedDeviceProvider_Version, pInterfaceName) == 0) {
		return &g_serverDriver;
	}
	
	if (strcmp(IVRWatchdogProvider_Version, pInterfaceName) == 0) {
		return &g_watchdog;
	}

	if (pReturnCode) {
		*pReturnCode = VRInitError_Init_InterfaceNotFound;
	}

	return NULL;
}

void WatchdogThreadFunction() {
	while (!g_bExiting) {
		std::this_thread::sleep_for(std::chrono::microseconds(500));
		vr::VRWatchdogHost()->WatchdogWakeUp(vr::TrackedDeviceClass_HMD);
	}
}

CServerDriver::CServerDriver() : m_pHMD(NULL) {}

vr::EVRInitError CServerDriver::Init(vr::IVRDriverContext* pDriverContext) {
	VR_INIT_SERVER_DRIVER_CONTEXT(pDriverContext);
	InitDriverLog(vr::VRDriverLog());
	DriverLog("CServer::Init");

	m_pHMD = new CLuaHMDDriver(HMD_SCRIPT_PATH);
	if (m_pHMD) {
		VRServerDriverHost()->TrackedDeviceAdded(m_pHMD->GetSerialNumber().c_str(), vr::TrackedDeviceClass_HMD, m_pHMD);
	}

	return m_pHMD != NULL ? VRInitError_None : VRInitError_Driver_HmdDisplayNotFound;
}

void CServerDriver::Cleanup() {
	if (m_pHMD != NULL) {
		delete m_pHMD;
	}

	CleanupDriverLog();
	VR_CLEANUP_SERVER_DRIVER_CONTEXT();
}

void CServerDriver::EnterStandby() {}

void CServerDriver::LeaveStandby() {}

void CServerDriver::RunFrame() {

}

vr::EVRInitError CWatchdogDriver::Init(vr::IVRDriverContext* pDriverContext) {
	VR_INIT_WATCHDOG_DRIVER_CONTEXT(pDriverContext);
	InitDriverLog(vr::VRDriverLog());

	DriverLog("Creating watchdog thread\n");
	g_bExiting = false;
	m_watchdogThread = std::thread(WatchdogThreadFunction);

	return VRInitError_None;
}

void CWatchdogDriver::Cleanup() {
	g_bExiting = true;
	DriverLog("Joining watchdog thread");
	m_watchdogThread.join();
	DriverLog("Joined watchdog thread");
}
