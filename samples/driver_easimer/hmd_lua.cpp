// === Copyright (c) 2017-2020 easimer.net. All rights reserved. ===

#include "hmd_lua.h"
#include "driverlog.h"

extern "C" {
#include "lauxlib.h"
#include "lua.h"
#include "lualib.h"
}

static int Lua_AtPanic(lua_State* L) {
    const char* pszMsg = lua_tostring(L, 1);
    DriverLog("script fatal error: %s", pszMsg);
    return 0;
}

static int Lua_DriverLog(lua_State* L) {
    const char* pszMsg = lua_tostring(L, 1);
    DriverLog("script: %s", pszMsg);
    return 0;
}

static bool InitializeLuaState(lua_State* L) {
    int res;

    luaL_openlibs(L);

    // Set panic func
    lua_atpanic(L, Lua_AtPanic);

    // Define a DriverLog function for the script
    lua_pushcfunction(L, Lua_DriverLog);
    lua_setglobal(L, "DriverLog");

    // Load and exec script file
    res = luaL_dofile(L, "drivers/easimer/scripts/driver_easimer.lua");

    if(res == LUA_OK) {
        DriverLog("script loaded successfully!");
        return true;
    } else {
        DriverLog("failed to load the script! (lua err=%d)", res);
        return false;
    }
}

CLuaHMDDriver::CLuaHMDDriver(const char* pszPath) :
    m_unObjectId(vr::k_unTrackedDeviceIndexInvalid),
    m_ulPropertyContainer(vr::k_ulInvalidPropertyContainer),
    m_sSerialNumber("SN00000001"),
    m_sModelNumber("v1.hmd.vr.easimer.net") {
    m_pLua = luaL_newstate();
    if (m_pLua) {
        if (!InitializeLuaState(m_pLua)) {
            lua_close(m_pLua);
            m_pLua = NULL;
        }
    } else {
        DriverLog("Failed to create Lua state!");
    }
}

CLuaHMDDriver::~CLuaHMDDriver() {
    if (m_pLua != NULL) {
        lua_close(m_pLua);
    }
}

vr::EVRInitError CLuaHMDDriver::Activate(uint32_t unObjectId) {
    return vr::EVRInitError();
}

void CLuaHMDDriver::Deactivate() {}

void CLuaHMDDriver::EnterStandby() {}

void* CLuaHMDDriver::GetComponent(const char* pchComponentNameAndVersion) {
    return nullptr;
}

void CLuaHMDDriver::DebugRequest(const char* pchRequest, char* pchResponseBuffer, uint32_t unResponseBufferSize) {}

vr::DriverPose_t CLuaHMDDriver::GetPose() {
    return vr::DriverPose_t();
}

void CLuaHMDDriver::GetWindowBounds(int32_t* pnX, int32_t* pnY, uint32_t* pnWidth, uint32_t* pnHeight) {}

bool CLuaHMDDriver::IsDisplayOnDesktop() {
    return false;
}

bool CLuaHMDDriver::IsDisplayRealDisplay() {
    return false;
}

void CLuaHMDDriver::GetRecommendedRenderTargetSize(uint32_t* pnWidth, uint32_t* pnHeight) {}

void CLuaHMDDriver::GetEyeOutputViewport(vr::EVREye eEye, uint32_t* pnX, uint32_t* pnY, uint32_t* pnWidth, uint32_t* pnHeight) {}

void CLuaHMDDriver::GetProjectionRaw(vr::EVREye eEye, float* pfLeft, float* pfRight, float* pfTop, float* pfBottom) {}

vr::DistortionCoordinates_t CLuaHMDDriver::ComputeDistortion(vr::EVREye eEye, float fU, float fV) {
    return vr::DistortionCoordinates_t();
}

void CLuaHMDDriver::RunFrame() {}

