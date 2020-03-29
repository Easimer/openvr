// === Copyright (c) 2017-2020 easimer.net. All rights reserved. ===

#include "hmd_lua.h"
#include "driverlog.h"

extern "C" {
#include "lauxlib.h"
#include "lua.h"
#include "lualib.h"
}

using namespace vr;

static int Lua_AtPanic(lua_State* L) {
    const char* pszMsg = lua_tostring(L, -1);
    DriverLog("script fatal error: %s", pszMsg);
    return 0;
}

static int Lua_DriverLog(lua_State* L) {
    const char* pszMsg = lua_tostring(L, 1);
    DriverLog("script: %s", pszMsg);
    return 0;
}

static bool InitializeLuaState(lua_State* L, std::string const& sPath) {
    int res;

    luaL_openlibs(L);

    // Set panic func
    lua_atpanic(L, Lua_AtPanic);

    // Define a DriverLog function for the script
    lua_pushcfunction(L, Lua_DriverLog);
    lua_setglobal(L, "DriverLog");

    // Load and exec script file
    res = luaL_dofile(L, sPath.c_str());

    if(res == LUA_OK) {
        DriverLog("script loaded successfully!");
        return true;
    } else {
        DriverLog("failed to load the script: %s", lua_tostring(L, -1));
        return false;
    }
}

CLuaHMDDriver::CLuaHMDDriver(const char* pszPath) :
    m_unObjectId(k_unTrackedDeviceIndexInvalid),
    m_ulPropertyContainer(k_ulInvalidPropertyContainer),
    m_sSerialNumber("SN00000001"),
    m_sModelNumber("v1.hmd.vr.easimer.net"),
    m_sScriptPath(pszPath),
    m_pLua(NULL) {
    Reload();
}

CLuaHMDDriver::~CLuaHMDDriver() {
    DriverLog("~CLuaHMDDriver");
    Unload();
}

static void PushTableFunction(lua_State* L, const char* pchTable, const char* pchFunction) {
    lua_getglobal(L, pchTable);
    lua_getfield(L, -1, pchFunction);
}

#define DO_SIMPLE_CALLBACK(t, f)            \
    if (m_pLua != NULL) {                   \
        PushTableFunction(m_pLua, t, f);    \
        lua_getglobal(m_pLua, t);           \
        lua_call(m_pLua, 1, 0);             \
    }

EVRInitError CLuaHMDDriver::Activate(uint32_t unObjectId) {
    EVRInitError ret = VRInitError_Init_Internal;

    m_unObjectId = unObjectId;
    m_ulPropertyContainer = VRProperties()->TrackedDeviceToPropertyContainer(m_unObjectId);

    VRProperties()->SetStringProperty(m_ulPropertyContainer, Prop_ModelNumber_String, m_sModelNumber.c_str());
    VRProperties()->SetStringProperty(m_ulPropertyContainer, Prop_RenderModelName_String, m_sModelNumber.c_str());
    VRProperties()->SetFloatProperty(m_ulPropertyContainer, Prop_UserIpdMeters_Float, 0.063f);
    VRProperties()->SetFloatProperty(m_ulPropertyContainer, Prop_UserHeadToEyeDepthMeters_Float, 0.0f);
    VRProperties()->SetFloatProperty(m_ulPropertyContainer, Prop_DisplayFrequency_Float, 60.0f);
    VRProperties()->SetFloatProperty(m_ulPropertyContainer, Prop_SecondsFromVsyncToPhotons_Float, 0.1f);
    VRProperties()->SetUint64Property(m_ulPropertyContainer, Prop_CurrentUniverseId_Uint64, 2);
    VRProperties()->SetBoolProperty(m_ulPropertyContainer, Prop_IsOnDesktop_Bool, false);

    if (m_pLua != NULL) {
        PushTableFunction(m_pLua, "TrackedDeviceServerDriver", "Activate");
        lua_getglobal(m_pLua, "TrackedDeviceServerDriver");
        lua_pushinteger(m_pLua, unObjectId);
        lua_call(m_pLua, 2, 1);
        if (lua_toboolean(m_pLua, -1)) {
            ret = vr::VRInitError_None;
        }
        lua_pop(m_pLua, 1);
    }

    return ret;
}

void CLuaHMDDriver::Deactivate() {
    DO_SIMPLE_CALLBACK("TrackedDeviceServerDriver", "Deactivate");

    m_unObjectId = k_unTrackedDeviceIndexInvalid;
}

void CLuaHMDDriver::EnterStandby() {
    DO_SIMPLE_CALLBACK("TrackedDeviceServerDriver", "EnterStandby");
}

void* CLuaHMDDriver::GetComponent(const char* pchComponentNameAndVersion) {
    if (!_stricmp(pchComponentNameAndVersion, vr::IVRDisplayComponent_Version)) {
        return (vr::IVRDisplayComponent*)this;
    }
    return nullptr;
}

void CLuaHMDDriver::DebugRequest(const char* pchRequest, char* pchResponseBuffer, uint32_t unResponseBufferSize) {
    if (unResponseBufferSize >= 1) {
        pchResponseBuffer[0] = 0;
    }
}

vr::DriverPose_t CLuaHMDDriver::GetPose() {
    DriverPose_t pose = { 0 };
    pose.poseIsValid = false;
    pose.result = TrackingResult_Calibrating_InProgress;
    pose.deviceIsConnected = true;

    pose.qWorldFromDriverRotation = { 1, 0, 0, 0 };
    pose.qDriverFromHeadRotation = { 1, 0, 0, 0 };
    pose.qRotation = { 1, 0, 0, 0 };

    return pose;
}

void CLuaHMDDriver::GetWindowBounds(int32_t* pnX, int32_t* pnY, uint32_t* pnWidth, uint32_t* pnHeight) {
    if (m_pLua != NULL) {
        PushTableFunction(m_pLua, "TrackedDeviceServerDriver", "GetWindowBounds");
        lua_getglobal(m_pLua, "TrackedDeviceServerDriver");
        lua_call(m_pLua, 1, 4);
        *pnX = lua_tonumber(m_pLua, -4);
        *pnY = lua_tonumber(m_pLua, -3);
        *pnWidth = lua_tonumber(m_pLua, -2);
        *pnHeight = lua_tonumber(m_pLua, -1);
        DriverLog("lua GetWindowBounds returned (%d, %d, %u, %u)", *pnX, *pnY, *pnWidth, *pnHeight);
        lua_pop(m_pLua, 4);
    }
}

bool CLuaHMDDriver::IsDisplayOnDesktop() {
    return true;
}

bool CLuaHMDDriver::IsDisplayRealDisplay() {
    return false;
}

void CLuaHMDDriver::GetRecommendedRenderTargetSize(uint32_t* pnWidth, uint32_t* pnHeight) {
    if (m_pLua != NULL) {
        // TODO(danielm): this is not part of the TrackedDeviceServerDriver interface!!!
        PushTableFunction(m_pLua, "TrackedDeviceServerDriver", "GetRecommendedRenderTargetSize");
        lua_getglobal(m_pLua, "TrackedDeviceServerDriver");
        lua_call(m_pLua, 1, 2);
        *pnWidth = lua_tonumber(m_pLua, -2);
        *pnHeight = lua_tonumber(m_pLua, -1);
        DriverLog("lua GetRecommendedRenderTargetSize returned (%u, %u)", *pnWidth, *pnHeight);
        lua_pop(m_pLua, 4);
    }
}

void CLuaHMDDriver::GetEyeOutputViewport(EVREye eEye, uint32_t* pnX, uint32_t* pnY, uint32_t* pnWidth, uint32_t* pnHeight) {
    if (m_pLua != NULL) {
        PushTableFunction(m_pLua, "TrackedDeviceServerDriver", "GetEyeOutputViewport");
        lua_getglobal(m_pLua, "TrackedDeviceServerDriver");
        lua_pushnumber(m_pLua, eEye);
        lua_call(m_pLua, 2, 4);
        *pnX = lua_tonumber(m_pLua, -4);
        *pnY = lua_tonumber(m_pLua, -3);
        *pnWidth = lua_tonumber(m_pLua, -2);
        *pnHeight = lua_tonumber(m_pLua, -1);
        DriverLog("lua GetEyeOutputViewport returned (%d, %d, %u, %u) for eye %d", *pnX, *pnY, *pnWidth, *pnHeight, eEye);
        lua_pop(m_pLua, 4);
    }
}

void CLuaHMDDriver::GetProjectionRaw(EVREye eEye, float* pfLeft, float* pfRight, float* pfTop, float* pfBottom) {
    *pfLeft = -1.0;
    *pfRight = 1.0;
    *pfTop = -1.0;
    *pfBottom = 1.0;
}

DistortionCoordinates_t CLuaHMDDriver::ComputeDistortion(EVREye eEye, float fU, float fV) {
    DistortionCoordinates_t coordinates;
    coordinates.rfBlue[0] = fU;
    coordinates.rfBlue[1] = fV;
    coordinates.rfGreen[0] = fU;
    coordinates.rfGreen[1] = fV;
    coordinates.rfRed[0] = fU;
    coordinates.rfRed[1] = fV;
    return coordinates;
}

void CLuaHMDDriver::RunFrame() {
    DO_SIMPLE_CALLBACK("TrackedDeviceServerDriver", "RunFrame");
}

void CLuaHMDDriver::Unload() {
    if (m_pLua != NULL) {
        DO_SIMPLE_CALLBACK("TrackedDeviceServerDriver", "OnShutdown");
        lua_close(m_pLua);
    }
}

void CLuaHMDDriver::Reload() {
    DriverLog("Reloading script...");
    Unload();
    m_pLua = luaL_newstate();
    if (m_pLua) {
        if (InitializeLuaState(m_pLua, m_sScriptPath)) {
            DriverLog("Script has been reloaded!");
            DO_SIMPLE_CALLBACK("TrackedDeviceServerDriver", "OnInit");
        } else {
            lua_close(m_pLua);
            m_pLua = NULL;
        }
    } else {
        DriverLog("Failed to create Lua state!");
    }
}

