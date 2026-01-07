// Google Sign-In Extension for Defold
// Cross-platform wrapper for Google Sign-In

#define EXTENSION_NAME google_signin
#define LIB_NAME "google_signin"
#define MODULE_NAME "google_signin"

#include <dmsdk/sdk.h>
#include <string.h>
#include <stdlib.h>

#if defined(DM_PLATFORM_ANDROID) || defined(DM_PLATFORM_IOS)

// Forward declarations for platform-specific functions
extern void GoogleSignIn_Init(const char* client_id);
extern void GoogleSignIn_Login();
extern void GoogleSignIn_Logout();
extern bool GoogleSignIn_IsSignedIn();

// Pending result structure - stores result until Lua polls it
struct PendingResult {
    bool has_result;
    bool success;
    char* id_token;
    char* user_id;
    char* email;
    char* display_name;
    char* error;
};

static PendingResult g_pending = {false, false, nullptr, nullptr, nullptr, nullptr, nullptr};

// Helper to duplicate string safely
static char* DuplicateString(const char* str) {
    if (str == nullptr) return nullptr;
    size_t len = strlen(str) + 1;
    char* copy = (char*)malloc(len);
    if (copy) {
        memcpy(copy, str, len);
    }
    return copy;
}

// Helper to free pending result strings
static void ClearPendingResult() {
    if (g_pending.id_token) { free(g_pending.id_token); g_pending.id_token = nullptr; }
    if (g_pending.user_id) { free(g_pending.user_id); g_pending.user_id = nullptr; }
    if (g_pending.email) { free(g_pending.email); g_pending.email = nullptr; }
    if (g_pending.display_name) { free(g_pending.display_name); g_pending.display_name = nullptr; }
    if (g_pending.error) { free(g_pending.error); g_pending.error = nullptr; }
    g_pending.has_result = false;
}

// Called from platform code when sign-in completes (may be called from JNI thread)
extern "C" void GoogleSignIn_OnResult(bool success, const char* id_token, const char* user_id, const char* email, const char* display_name, const char* error)
{
    // Store result for Lua to poll
    ClearPendingResult();

    g_pending.success = success;
    g_pending.id_token = DuplicateString(id_token);
    g_pending.user_id = DuplicateString(user_id);
    g_pending.email = DuplicateString(email);
    g_pending.display_name = DuplicateString(display_name);
    g_pending.error = DuplicateString(error);
    g_pending.has_result = true;

    dmLogInfo("Google Sign-In: Result stored (success=%d)", success);
}

// Lua API: google_signin.init(client_id)
static int GoogleSignIn_Lua_Init(lua_State* L)
{
    const char* client_id = luaL_checkstring(L, 1);
    GoogleSignIn_Init(client_id);
    return 0;
}

// Lua API: google_signin.login() - no callback, use poll_result
static int GoogleSignIn_Lua_Login(lua_State* L)
{
    ClearPendingResult(); // Clear any previous result
    GoogleSignIn_Login();
    return 0;
}

// Lua API: google_signin.logout()
static int GoogleSignIn_Lua_Logout(lua_State* L)
{
    GoogleSignIn_Logout();
    return 0;
}

// Lua API: google_signin.is_signed_in()
static int GoogleSignIn_Lua_IsSignedIn(lua_State* L)
{
    lua_pushboolean(L, GoogleSignIn_IsSignedIn());
    return 1;
}

// Lua API: google_signin.poll_result() - returns nil if no result, or result table
static int GoogleSignIn_Lua_PollResult(lua_State* L)
{
    if (!g_pending.has_result) {
        lua_pushnil(L);
        return 1;
    }

    // Build result table
    lua_newtable(L);

    lua_pushboolean(L, g_pending.success);
    lua_setfield(L, -2, "success");

    if (g_pending.success) {
        if (g_pending.id_token) {
            lua_pushstring(L, g_pending.id_token);
            lua_setfield(L, -2, "id_token");
        }
        if (g_pending.user_id) {
            lua_pushstring(L, g_pending.user_id);
            lua_setfield(L, -2, "user_id");
        }
        if (g_pending.email) {
            lua_pushstring(L, g_pending.email);
            lua_setfield(L, -2, "email");
        }
        if (g_pending.display_name) {
            lua_pushstring(L, g_pending.display_name);
            lua_setfield(L, -2, "display_name");
        }
    } else {
        if (g_pending.error) {
            lua_pushstring(L, g_pending.error);
            lua_setfield(L, -2, "error");
        }
    }

    // Clear the result after returning
    ClearPendingResult();

    return 1;
}

static const luaL_reg Module_methods[] =
{
    {"init", GoogleSignIn_Lua_Init},
    {"login", GoogleSignIn_Lua_Login},
    {"logout", GoogleSignIn_Lua_Logout},
    {"is_signed_in", GoogleSignIn_Lua_IsSignedIn},
    {"poll_result", GoogleSignIn_Lua_PollResult},
    {0, 0}
};

static void LuaInit(lua_State* L)
{
    int top = lua_gettop(L);
    luaL_register(L, MODULE_NAME, Module_methods);
    lua_pop(L, 1);
    assert(top == lua_gettop(L));
}

static dmExtension::Result AppInitializeExtension(dmExtension::AppParams* params)
{
    return dmExtension::RESULT_OK;
}

static dmExtension::Result InitializeExtension(dmExtension::Params* params)
{
    LuaInit(params->m_L);
    dmLogInfo("Registered %s Extension", MODULE_NAME);
    return dmExtension::RESULT_OK;
}

static dmExtension::Result AppFinalizeExtension(dmExtension::AppParams* params)
{
    return dmExtension::RESULT_OK;
}

static dmExtension::Result FinalizeExtension(dmExtension::Params* params)
{
    ClearPendingResult();
    return dmExtension::RESULT_OK;
}

DM_DECLARE_EXTENSION(EXTENSION_NAME, LIB_NAME, AppInitializeExtension, AppFinalizeExtension, InitializeExtension, 0, 0, FinalizeExtension)

#else
// Stub implementation for unsupported platforms

static int GoogleSignIn_Lua_Init(lua_State* L)
{
    dmLogWarning("Google Sign-In is not supported on this platform");
    return 0;
}

static int GoogleSignIn_Lua_Login(lua_State* L)
{
    dmLogWarning("Google Sign-In is not supported on this platform");
    return 0;
}

static int GoogleSignIn_Lua_Logout(lua_State* L)
{
    return 0;
}

static int GoogleSignIn_Lua_IsSignedIn(lua_State* L)
{
    lua_pushboolean(L, 0);
    return 1;
}

static int GoogleSignIn_Lua_PollResult(lua_State* L)
{
    // Return error result immediately for unsupported platforms
    lua_newtable(L);
    lua_pushboolean(L, 0);
    lua_setfield(L, -2, "success");
    lua_pushstring(L, "Platform not supported");
    lua_setfield(L, -2, "error");
    return 1;
}

static const luaL_reg Module_methods[] =
{
    {"init", GoogleSignIn_Lua_Init},
    {"login", GoogleSignIn_Lua_Login},
    {"logout", GoogleSignIn_Lua_Logout},
    {"is_signed_in", GoogleSignIn_Lua_IsSignedIn},
    {"poll_result", GoogleSignIn_Lua_PollResult},
    {0, 0}
};

static void LuaInit(lua_State* L)
{
    int top = lua_gettop(L);
    luaL_register(L, MODULE_NAME, Module_methods);
    lua_pop(L, 1);
    assert(top == lua_gettop(L));
}

static dmExtension::Result AppInitializeExtension(dmExtension::AppParams* params)
{
    return dmExtension::RESULT_OK;
}

static dmExtension::Result InitializeExtension(dmExtension::Params* params)
{
    LuaInit(params->m_L);
    dmLogInfo("Registered %s Extension (stub)", MODULE_NAME);
    return dmExtension::RESULT_OK;
}

static dmExtension::Result AppFinalizeExtension(dmExtension::AppParams* params)
{
    return dmExtension::RESULT_OK;
}

static dmExtension::Result FinalizeExtension(dmExtension::Params* params)
{
    return dmExtension::RESULT_OK;
}

DM_DECLARE_EXTENSION(EXTENSION_NAME, LIB_NAME, AppInitializeExtension, AppFinalizeExtension, InitializeExtension, 0, 0, FinalizeExtension)

#endif
