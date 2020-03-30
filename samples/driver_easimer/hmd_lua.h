// === Copyright (c) 2017-2020 easimer.net. All rights reserved. ===

#pragma once
#include <openvr_driver.h>
#include "CSteamController.h"

struct lua_State;

enum HandlerType_t {
	k_unHandlerType_SteamController = 0,
	k_unHandlerType_Max
};

//-----------------------------------------------------------------------------
// Purpose: HMD driver that calls back to a Lua script
//-----------------------------------------------------------------------------

class CLuaHMDDriver : public vr::ITrackedDeviceServerDriver, public vr::IVRDisplayComponent {
public:
    CLuaHMDDriver(const char* pszPath);
    virtual ~CLuaHMDDriver();

	virtual vr::EVRInitError Activate(uint32_t unObjectId) override;
	virtual void Deactivate() override;
	virtual void EnterStandby() override;
	virtual void* GetComponent(const char* pchComponentNameAndVersion) override;
	virtual void DebugRequest(const char* pchRequest, char* pchResponseBuffer, uint32_t unResponseBufferSize) override;
	virtual vr::DriverPose_t GetPose() override;

	// Inherited via IVRDisplayComponent
	virtual void GetWindowBounds(int32_t* pnX, int32_t* pnY, uint32_t* pnWidth, uint32_t* pnHeight) override;
	virtual bool IsDisplayOnDesktop() override;
	virtual bool IsDisplayRealDisplay() override;
	virtual void GetRecommendedRenderTargetSize(uint32_t* pnWidth, uint32_t* pnHeight) override;
	virtual void GetEyeOutputViewport(vr::EVREye eEye, uint32_t* pnX, uint32_t* pnY, uint32_t* pnWidth, uint32_t* pnHeight) override;
	virtual void GetProjectionRaw(vr::EVREye eEye, float* pfLeft, float* pfRight, float* pfTop, float* pfBottom) override;
	virtual vr::DistortionCoordinates_t ComputeDistortion(vr::EVREye eEye, float fU, float fV) override;

	void RunFrame();
	const std::string& GetSerialNumber() const { return m_sSerialNumber; }

	class BaseLuaInterface;
	void SetHandler(HandlerType_t type, int refHandler);

private:

	void Reload();
	void Unload();

private:
	vr::TrackedDeviceIndex_t m_unObjectId;
	vr::PropertyContainerHandle_t m_ulPropertyContainer;

	std::string m_sSerialNumber;
	std::string m_sModelNumber;
	std::string m_sScriptPath;

	vr::HmdQuaternion_t m_qCalibration;

	lua_State* m_pLua;

	// References to handlers' method table
	int m_arefHandlers[k_unHandlerType_Max];

	// Lua Handler for SteamController
	void* m_pLuaSteamController;
};
