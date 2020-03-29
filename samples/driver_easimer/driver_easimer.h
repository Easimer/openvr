// === Copyright (c) 2017-2020 easimer.net. All rights reserved. ===

#pragma once
#include <openvr_driver.h>
#include <thread>

//-----------------------------------------------------------------------------
// Purpose: Watchdog driver that periodically wakes up the server
//-----------------------------------------------------------------------------

class CWatchdogDriver : public vr::IVRWatchdogProvider {
public:
	CWatchdogDriver();
	virtual vr::EVRInitError Init(vr::IVRDriverContext* pCtx) override;
	virtual void Cleanup() override;

private:
	std::thread m_watchdogThread;
};

//-----------------------------------------------------------------------------
// Purpose: the server driver
//-----------------------------------------------------------------------------

class CServerDriver : public vr::IServerTrackedDeviceProvider {
public:
	CServerDriver();

	virtual vr::EVRInitError Init(vr::IVRDriverContext* pDriverContext) override;
	virtual void Cleanup() override;
	virtual const char* const* GetInterfaceVersions() override { return vr::k_InterfaceVersions; }
	virtual bool ShouldBlockStandbyMode() override { return true; }
	virtual void EnterStandby() override;
	virtual void LeaveStandby() override;

private:
};
