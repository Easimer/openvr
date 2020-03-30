// === Copyright (c) 2017-2020 easimer.net. All rights reserved. ===

#include "hmd_lua.h"
#include "driverlog.h"

extern "C" {
#include "lauxlib.h"
#include "lua.h"
#include "lualib.h"
}

#include "CSteamController.h"

using namespace vr;

#define TABLE_VRDISP "VRDisplayComponent"
#define TABLE_TRACKDEV "TrackedDeviceServerDriver"
#define TABLE_CONTROL "SteamController"

// Convert a HmdQuaternion_t into a Lua table
// If the operation is successful, the top of the stack contains
// the table and true is returned.
// On failure the stack remains balanced and false is returned.
// [-0, +1, e]
static bool ToLuaTable(lua_State* L, const vr::HmdQuaternion_t& ev) {
    bool ret = false;

    if (L) {
        lua_newtable(L); // +1
        lua_pushstring(L, "w"); lua_pushnumber(L, ev.w); lua_settable(L, -3);
        lua_pushstring(L, "x"); lua_pushnumber(L, ev.x); lua_settable(L, -3);
        lua_pushstring(L, "y"); lua_pushnumber(L, ev.y); lua_settable(L, -3);
        lua_pushstring(L, "z"); lua_pushnumber(L, ev.z); lua_settable(L, -3);
        ret = true;
    }

    return ret;
}

// Convert a SteamControllerUpdateEvent into a Lua table
// If the operation is successful, the top of the stack contains
// the table and true is returned.
// On failure the stack remains balanced and false is returned.
// [-0, +1, e]
static bool ToLuaTable(lua_State* L, const SteamControllerUpdateEvent& ev) {
    bool ret = false;

    if (L) {
        lua_newtable(L); // +1
        auto x = ev.orientation.x / 32767.0f;
        auto y = ev.orientation.y / 32767.0f;
        auto z = ev.orientation.z / 32767.0f;
        auto w = 1.0f - sqrtf(x * x + y * y + z * z);
        auto orientation = HmdQuaternion_t{ w, x, y, z };
        lua_pushstring(L, "orientation"); // +1
        if (ToLuaTable(L, orientation)) { // +1
            lua_settable(L, -3); // -2
            ret = true;
        } else {
            lua_pop(L, 2); // -2
        }
    }

    return ret;
}

#define X2(x) #x
#define X(x) X2(x)

#define TABLE_GET_T(cont, key, field, conv, coercion) { \
    lua_pushstring(L, X(key));              \
    lua_gettable(L, -1);                    \
    cont.field = coercion conv(L, -1);      \
    lua_pop(L, 1);                          \
}

#define TABLE_MAP_T(cont, key, conv, coercion) TABLE_GET_T(cont, key, key, conv, coercion)

#define TABLE_MAP_NUMBER(cont, key) TABLE_MAP_T(cont, key, lua_tonumber)
#define TABLE_MAP_BOOL(cont, key) TABLE_MAP_T(cont, key, lua_toboolean)
#define TABLE_MAP_INT(cont, key) TABLE_MAP_T(cont, key, lua_tointeger)
#define TABLE_MAP_ENUM(cont, key, type) TABLE_MAP_T(cont, key, lua_tointeger, (type))

#define TABLE_GET_TRIV(cont, key, field, type) { \
    lua_pushstring(L, X(key));                   \
    lua_gettable(L, -1);                         \
    FromLuaTable(L, cont.field);                 \
}
#define TABLE_MAP_TRIV(cont, key) TABLE_GET_TRIV(cont, key, key)

// Convert the Lua table at the top of the stack to a HMD quat
// Returns true on success and false on failure.
// Pops the table from the stack.
// [-1, +0, -]
static bool FromLuaTable(lua_State* L, vr::HmdQuaternion_t& q) {
    bool ret = false;
    
    if (L) {
        TABLE_MAP_NUMBER(q, w);
        TABLE_MAP_NUMBER(q, x);
        TABLE_MAP_NUMBER(q, y);
        TABLE_MAP_NUMBER(q, z);
    }

    lua_pop(L, 1);
    return ret;
}

// Convert the Lua table at the top of the stack to driver pose
// Returns true on success and false on failure.
// Pops the table from the stack.
// [-1, +0, -]
static bool FromLuaTable(lua_State* L, DriverPose_t& pose) {
    bool ret = false;
    pose = { 0 };

    if (L) {
        TABLE_MAP_BOOL(pose, poseIsValid);
        TABLE_MAP_BOOL(pose, deviceIsConnected);
        TABLE_MAP_ENUM(pose, result, ETrackingResult);

        TABLE_MAP_TRIV(pose, qWorldFromDriverRotation);
        TABLE_MAP_TRIV(pose, qDriverFromHeadRotation);
        TABLE_MAP_TRIV(pose, qRotation);

        ret = true;
    }

    lua_pop(L, 1);

    return ret;
}

class CLuaHMDDriver::BaseLuaInterface {
public:
    BaseLuaInterface() : L(NULL), m_nRefMethodTable(LUA_NOREF), m_nRefInstance(LUA_NOREF) {}
    BaseLuaInterface(const BaseLuaInterface&) = delete;

    BaseLuaInterface(lua_State* L, int nRefMethodTable) : L(L), m_nRefMethodTable(nRefMethodTable) {
        if (!CheckMethodTable()) {
            DriverLog("Methods are missing!");
        }
        // Create a new instance by calling Interface:new()
        PushMethodTable();
        lua_getfield(L, -1, "Create");
        PushMethodTable();
        lua_call(L, 1, 1);

        m_nRefInstance = luaL_ref(L, LUA_REGISTRYINDEX);
        if (m_nRefInstance != LUA_NOREF && m_nRefInstance != LUA_REFNIL) {
            OnInit();
        } else {
            DriverLog("new: instance reference is %d, a NOREF or REFNIL!", m_nRefInstance);
        }

        lua_pop(L, 1);
    }

    // Push the method table object onto the Lua stack
    // [-0, +1, -]
    inline void PushMethodTable() {
        lua_rawgeti(L, LUA_REGISTRYINDEX, m_nRefMethodTable);
    }

    // Push the instance object onto the Lua stack
    // [-0, +1, -]
    inline void PushInstance() {
        lua_rawgeti(L, LUA_REGISTRYINDEX, m_nRefInstance);
    }

    // Call OnInit on the handler instance
    // [-0, +0, -]
    void OnInit() {
        PushMethod("OnInit");
        PushInstance();
        lua_call(L, 1, 0);
        lua_pop(L, 1);
    }

    virtual ~BaseLuaInterface() {
        if (L) {
            if (m_nRefInstance != LUA_NOREF && m_nRefMethodTable != LUA_NOREF) {
                PushMethod("OnShutdown");
                PushInstance();
                lua_call(L, 1, 0);
                lua_pop(L, 1);
            }
            if (m_nRefInstance != LUA_NOREF) {
                luaL_unref(L, LUA_REGISTRYINDEX, m_nRefInstance);
            }
            if (m_nRefMethodTable != LUA_NOREF) {
                luaL_unref(L, LUA_REGISTRYINDEX, m_nRefMethodTable);
            }
        }
    }

    bool IsMethodPresent(char const* pszKey) {
        bool ret = true;
        PushMethod(pszKey);
        if (lua_isnil(L, -1)) {
            ret = false;
            DriverLog("Interface method missing: %s", pszKey);
        }
        lua_pop(L, 1);
        return ret;
    }

    virtual bool CheckMethodTable() {
        return IsMethodPresent("Create") &&
            IsMethodPresent("OnInit") &&
            IsMethodPresent("OnShutdown");
    }

protected:
    // Add a method to the method table
    // [-0, +0, -]
    void AddMethod(char const* pszKey, lua_CFunction pFunc) {
        PushMethodTable(); // +1
        lua_pushstring(L, pszKey); // +1
        lua_pushcfunction(L, pFunc); // +1
        lua_settable(L, -3); // -2
        lua_pop(L, 1); // -1
    }

    // Push a method onto the Lua stack
    // Top of the stack will contain the method
    // [-0, +2, -]
    void PushMethod(char const* pszKey) {
        PushMethodTable(); // +1
        lua_pushstring(L, pszKey); // +1
        lua_gettable(L, -2); // -1 +1
    }
    lua_State* L;

    int m_nRefMethodTable;
    int m_nRefInstance;
};

static int Lua_ISteamController_Rumble(lua_State* L) {
    auto pSC = (CSteamController*)lua_touserdata(L, -1);
    if (pSC) {
        pSC->TriggerHaptic(0, 1000, 1000, 500);
        pSC->TriggerHaptic(1, 1000, 1000, 500);
    }
    return 0;
}

class ISteamController : public CLuaHMDDriver::BaseLuaInterface, public CSteamController {
public:
    ISteamController(lua_State* L, int nRefMethodTable, SteamControllerDeviceEnum* it) :
        CLuaHMDDriver::BaseLuaInterface(L, nRefMethodTable),
        CSteamController::CSteamController(it),
        m_bReload(false) {
        Configure(STEAMCONTROLLER_CONFIG_SEND_ORIENTATION);
        DriverLog("Adding Rumble method");
        AddMethod("Rumble", Lua_ISteamController_Rumble);
        DriverLog("Calling OnConnect");
        OnConnect();
    }

    virtual ~ISteamController() {
    }

    virtual void OnUpdate(const SteamControllerUpdateEvent& ev) override {
        if (ev.buttons & STEAMCONTROLLER_BUTTON_HOME) {
            // Reload script
            m_bReload = true;
            DriverLog("User requested script reload by pressing Home");
        } else {
            // Propagate event to Lua script
            PushMethod("OnUpdate"); // +2
            PushInstance(); // +1
            if (ToLuaTable(L, ev)) { // +1
                lua_call(L, 2, 0); // -3
                lua_pop(L, 1); // -1
            } else {
                lua_pop(L, 3);
            }
        }
    }

    virtual bool CheckMethodTable() override {
        return CLuaHMDDriver::BaseLuaInterface::CheckMethodTable() &&
            IsMethodPresent("OnConnect") &&
            IsMethodPresent("OnDisconnect") &&
            IsMethodPresent("OnUpdate") &&
            IsMethodPresent("GetControllerHandle");
    }

    bool UserRequestedReload() {
        auto ret = m_bReload;
        m_bReload = false;
        return ret;
    }

protected:
    void OnConnect() {
        PushMethod("OnConnect"); // +2
        PushInstance(); // +1
        lua_pushlightuserdata(L, this); // +1
        lua_call(L, 2, 0); // -3
        lua_pop(L, 1); // -1
    }

    bool m_bReload;
};

static int Lua_AtPanic(lua_State* L) {
    const char* pszMsg = lua_tostring(L, -1);
    DriverLog("script fatal error: %s", pszMsg);
    return 0;
}

static int Lua_DriverLog(lua_State* L) {
    const char* pszMsg = lua_tostring(L, -1);
    DriverLog("script: %s", pszMsg);
    return 0;
}

static int Lua_RegisterHandler(lua_State* L) {
    auto driver = (CLuaHMDDriver*)lua_touserdata(L, -3);
    if (driver == NULL) {
        DriverLog("Lua_RegisterHandler failed because the driver pointer is NULL!");
        return 0;
    }

    auto interfaceId = lua_tostring(L, -2);
    // pop table off stack and pin it
    auto table = luaL_ref(L, LUA_REGISTRYINDEX);

    DriverLog("Registering handler for %s (driver=%p, table=%d)", interfaceId, driver, table);

    HandlerType_t type = k_unHandlerType_Max;
    CLuaHMDDriver::BaseLuaInterface* pIf = NULL;
    if (strcmp(TABLE_CONTROL, interfaceId) == 0) {
        type = k_unHandlerType_SteamController;
    } else if (strcmp(TABLE_VRDISP, interfaceId) == 0) {
    } else if (strcmp(TABLE_TRACKDEV, interfaceId) == 0) {
    }

    if (type != k_unHandlerType_Max) {
        driver->SetHandler(type, table);
    }
    lua_pop(L, 2);

    return 0;
}

static bool InitializeLuaState(lua_State* L, std::string const& sPath) {
    int res;

    luaL_openlibs(L);

    // Set panic func
    lua_atpanic(L, Lua_AtPanic);

    // Define a DriverLog function for the script
    lua_register(L, "DriverLog", Lua_DriverLog);
    lua_register(L, "RegisterHandler", Lua_RegisterHandler);

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
    m_pLua(NULL),
    m_pLuaSteamController(NULL) {
    for (int i = 0; i < k_unHandlerType_Max; i++) {
        m_arefHandlers[i] = LUA_NOREF;
    }
    Reload();
}

CLuaHMDDriver::~CLuaHMDDriver() {
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
        PushTableFunction(m_pLua, TABLE_TRACKDEV, "Activate");
        lua_getglobal(m_pLua, TABLE_TRACKDEV);
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
    DO_SIMPLE_CALLBACK(TABLE_TRACKDEV, "Deactivate");

    m_unObjectId = k_unTrackedDeviceIndexInvalid;
}

void CLuaHMDDriver::EnterStandby() {
    DO_SIMPLE_CALLBACK(TABLE_TRACKDEV, "EnterStandby");
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
    if (m_pLua != NULL) {
        DriverPose_t pose = { 0 };
        PushTableFunction(m_pLua, TABLE_TRACKDEV, "GetPose");
        lua_getglobal(m_pLua, TABLE_TRACKDEV);
        lua_call(m_pLua, 1, 1);

        if (FromLuaTable(m_pLua, pose)) {
            return pose;
        } else {
            return { 0 };
        }
    }

    return { 0 };
}

void CLuaHMDDriver::GetWindowBounds(int32_t* pnX, int32_t* pnY, uint32_t* pnWidth, uint32_t* pnHeight) {
    if (m_pLua != NULL) {
        PushTableFunction(m_pLua, TABLE_VRDISP, "GetWindowBounds");
        lua_getglobal(m_pLua, TABLE_VRDISP);
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
        PushTableFunction(m_pLua, TABLE_VRDISP, "GetRecommendedRenderTargetSize");
        lua_getglobal(m_pLua, TABLE_VRDISP);
        lua_call(m_pLua, 1, 2);
        *pnWidth = lua_tonumber(m_pLua, -2);
        *pnHeight = lua_tonumber(m_pLua, -1);
        DriverLog("lua GetRecommendedRenderTargetSize returned (%u, %u)", *pnWidth, *pnHeight);
        lua_pop(m_pLua, 4);
    }
}

void CLuaHMDDriver::GetEyeOutputViewport(EVREye eEye, uint32_t* pnX, uint32_t* pnY, uint32_t* pnWidth, uint32_t* pnHeight) {
    if (m_pLua != NULL) {
        PushTableFunction(m_pLua, TABLE_VRDISP, "GetEyeOutputViewport");
        lua_getglobal(m_pLua, TABLE_VRDISP);
        lua_pushnumber(m_pLua, eEye);
        lua_call(m_pLua, 2, 4);
        *pnX = (int32_t)lua_tonumber(m_pLua, -4);
        *pnY = (int32_t)lua_tonumber(m_pLua, -3);
        *pnWidth = (uint32_t)lua_tonumber(m_pLua, -2);
        *pnHeight = (uint32_t)lua_tonumber(m_pLua, -1);
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
    if (m_pLuaSteamController != NULL) {
        auto pSC = (ISteamController*)m_pLuaSteamController;
        if (pSC) {
            pSC->RunFrames();
            if (pSC->UserRequestedReload()) {
                DriverLog("User requested script reload through Steam Controller");
                Reload();
            }
        } else {
            DriverLog("ISteamController* conversion failed?");
        }
    }
}

void CLuaHMDDriver::SetHandler(HandlerType_t type, int refHandler) {
    m_arefHandlers[type] = refHandler;
    DriverLog("Handler #%d set to %d", type, refHandler);
}

void CLuaHMDDriver::Unload() {
    DriverLog("Unloading script...");
    if (m_pLuaSteamController != NULL) {
        delete m_pLuaSteamController;
        m_pLuaSteamController = NULL;
    }
    if (m_pLua != NULL) {
        DO_SIMPLE_CALLBACK(TABLE_VRDISP, "OnShutdown");
        DO_SIMPLE_CALLBACK(TABLE_TRACKDEV, "OnShutdown");

        lua_close(m_pLua);
        m_pLua = NULL;
    }

    for (int i = 0; i < k_unHandlerType_Max; i++) {
        m_arefHandlers[i] = LUA_NOREF;
    }
}

void CLuaHMDDriver::Reload() {
    Unload();

    DriverLog("Creating Lua state");
    m_pLua = luaL_newstate();
    if (m_pLua) {
        DriverLog("Loading script...");
        if (InitializeLuaState(m_pLua, m_sScriptPath)) {
            DriverLog("Script has been reloaded!");

            // Asking script to register it's handlers
            // We pass in the pointer to this instance and the function
            // will pass it back to us by calling RegisterHandler
            lua_getglobal(m_pLua, "API_RegisterHandlers");
            lua_pushlightuserdata(m_pLua, this);
            lua_call(m_pLua, 1, 0);

            DO_SIMPLE_CALLBACK(TABLE_TRACKDEV, "OnInit");
            DO_SIMPLE_CALLBACK(TABLE_VRDISP, "OnInit");

            // Find first Steam Controller
            DriverLog("Discovering Steam Controllers");
            auto it = SteamController_EnumControllerDevices();
            if (it != NULL) {
                DriverLog("Found a Steam Controller");
                this->m_pLuaSteamController = new ISteamController(m_pLua, m_arefHandlers[k_unHandlerType_SteamController], it);

                do {
                    it = SteamController_NextControllerDevice(it);
                } while (it != NULL);
            }
        } else {
            lua_close(m_pLua);
            m_pLua = NULL;
        }
    } else {
        DriverLog("Failed to create Lua state!");
    }
}

