#include <Windows.h>
#include <stdint.h>
#include <iostream>
#include <string>
#include <thread>
#include <memory>
#include <exception>
#include "Utils.h"
#include "Console.h"
#include "Color.h"
#include "ICVar.h"
#include "CLuaShared.h"
#include "Hooking.h"
#include "ModernUtils.h"
#include "GUI.h"
#include "Executor.h"
#include "Logger.h"
#include "Config.h"
#include "SecurityValidator.h"

// ============ CONSTANTS ============
// Replace magic numbers with named constants
constexpr uint32_t VMT_RUNSTRINGEX_INDEX = 111;
constexpr uint32_t INTERFACE_RETRY_DELAY_MS = 5;
constexpr uint32_t INTERFACE_RETRY_TIMEOUT_MS = 30000;
constexpr uint32_t LUAINTERFACE_RETRY_DELAY_MS = 10;

#if _WIN64
#define ConColorMsg "?ConColorMsg@@YAXAEBVColor@@PEBDZZ"
#else
#define ConColorMsg "?ConColorMsg@@YAXAEBVColor@@PBDZZ"
#endif

// ============ TYPE DEFINITIONS ============
typedef __int64(__fastcall* RunStringEx)(PVOID _this, const char* filename, 
    const char* path, const char* stringToRun, bool run, bool printErrors, 
    bool dontPushErrors, bool noReturns);

// ============ GLOBAL STATE (Encapsulated) ============
class InjectorState {
public:
    static InjectorState& Instance() {
        static InjectorState instance;
        return instance;
    }

    RunStringEx GetOriginalRunStringEx() const { return originalRunStringEx; }
    void SetOriginalRunStringEx(RunStringEx fn) { originalRunStringEx = fn; }

    PVOID GetLuaInterface() const { return luaInterface; }
    void SetLuaInterface(PVOID iface) { luaInterface = iface; }

    CCvar* GetCVarInterface() const { return cvarInterface; }
    void SetCVarInterface(CCvar* iface) { cvarInterface = iface; }

    const std::string& GetCVarName() const { return cvarName; }
    const std::string& GetLastFileName() const { return lastFileName; }
    void SetLastFileName(const std::string& name) { lastFileName = name; }

private:
    InjectorState() : originalRunStringEx(nullptr), luaInterface(nullptr), 
                     cvarInterface(nullptr) {
        cvarName = StringUtils::GenerateRandomString(10);
    }

    ~InjectorState() = default;

    // Prevent copying
    InjectorState(const InjectorState&) = delete;
    InjectorState& operator=(const InjectorState&) = delete;

    RunStringEx originalRunStringEx;
    PVOID luaInterface;
    CCvar* cvarInterface;
    std::string cvarName;
    std::string lastFileName;
};

// ============ HOOKING IMPLEMENTATION ============

/**
 * @brief Hook for RunStringEx - intercepts Lua script execution
 * Downloads and logs scripts from servers
 */
__int64 __fastcall HookedRunStringEx(
    PVOID _this,
#ifndef _WIN64
    void*,
#endif
    const char* filename, const char* path, const char* stringToRun, 
    bool run, bool printErrors, bool dontPushErrors, bool noReturns)
{
    try {
        auto& state = InjectorState::Instance();
        auto original = state.GetOriginalRunStringEx();

        // Skip internal Lua calls
        if (!filename || !strlen(filename) || !strcmp(filename, "LuaCmd") || 
            !strcmp(filename, "RunString(Ex)")) {
            LOG_DEBUG("Skipping internal Lua call");
            return original(_this, filename, path, stringToRun, run, printErrors, 
                          dontPushErrors, noReturns);
        }

        state.SetLastFileName(filename);
        LOG_INFO("Processing script: " + std::string(filename), "HookedRunStringEx");

        // Execute script extraction via ConVar
        if (!stringToRun) {
            LOG_ERROR("stringToRun is null", "HookedRunStringEx");
            return original(_this, filename, path, stringToRun, run, printErrors, 
                          dontPushErrors, noReturns);
        }

        // Build ConVar command
        std::string cvarCmd = "CreateClientConVar(\"";
        cvarCmd += state.GetCVarName();
        cvarCmd += "\", \"\", true, false) :SetString(GetHostName() .. \" - \" ..game.GetIPAddress())";

        // Execute command to set ConVar
        original(_this, filename, path, cvarCmd.c_str(), run, printErrors, 
                dontPushErrors, noReturns);

        // Try to retrieve server info from ConVar
        try {
            auto cvarInterface = state.GetCVarInterface();
            if (cvarInterface) {
                uintptr_t cvarPtr = (uintptr_t)cvarInterface->FindVar(state.GetCVarName().c_str());
                if (cvarPtr) {
                    // Safe ConVar string retrieval
                    std::string serverInfo = CVarStr(cvarPtr);
                    
                    // Validate server info
                    if (!SecurityValidator::IsPathSafe(serverInfo)) {
                        LOG_WARNING("Suspicious server info detected: " + serverInfo, 
                                  "HookedRunStringEx");
                        return original(_this, filename, path, stringToRun, run, printErrors, 
                                      dontPushErrors, noReturns);
                    }

                    // Skip loopback/local servers
                    if (serverInfo.find("loopback") != std::string::npos) {
                        LOG_INFO("Skipping loopback server", "HookedRunStringEx");
                        return original(_this, filename, path, stringToRun, run, printErrors, 
                                      dontPushErrors, noReturns);
                    }

                    // Save script with sanitized path
                    try {
                        std::string savePath = Config::Instance().GetScriptDirectory() + 
                                             serverInfo + "/" + filename;
                        
                        // Validate and sanitize path
                        SafePath validPath(savePath, Config::Instance().GetScriptDirectory());
                        
                        // Create directories and save
                        fs::path pathObj(validPath.Get());
                        FileUtils::CreateDirectoryRecursive(pathObj.parent_path().string());
                        FileUtils::WriteFile(validPath.Get(), stringToRun);
                        
                        LOG_INFO("Downloaded script: " + std::string(filename), 
                                "HookedRunStringEx");
                    }
                    catch (const std::exception& e) {
                        LOG_ERROR("Failed to save script: " + std::string(e.what()), 
                                 "HookedRunStringEx");
                    }
                }
            }
        }
        catch (const std::exception& e) {
            LOG_ERROR("ConVar retrieval failed: " + std::string(e.what()), 
                     "HookedRunStringEx");
        }

        // Execute original script
        return original(_this, filename, path, stringToRun, run, printErrors, 
                       dontPushErrors, noReturns);
    }
    catch (const std::exception& e) {
        LOG_CRITICAL("Exception in HookedRunStringEx: " + std::string(e.what()), 
                    "HookedRunStringEx");
        // Fall back to original to prevent crash
        auto& state = InjectorState::Instance();
        return state.GetOriginalRunStringEx()(_this, filename, path, stringToRun, run, 
                                            printErrors, dontPushErrors, noReturns);
    }
}

// ============ INITIALIZATION ============

/**
 * @brief Initialize console for debug builds
 */
void InitializeConsole() {
#ifdef _DEBUG
    AllocConsole();
    FILE* f = nullptr;
    freopen_s(&f, "CONOUT$", "w", stdout);
    freopen_s(&f, "CONIN$", "r", stdin);
    SetConsoleTitle(L"Gmod Lua Injector - Debug Console");
    LOG_INFO("Debug console initialized", "InitializeConsole");
#endif
}

/**
 * @brief Main initialization thread
 * Sets up all systems in order
 */
void InitializationThread() {
    LOG_SCOPE();
    
    try {
        LOG_INFO("Starting injector initialization", "InitializationThread");

        // Initialize configuration
        if (!Config::Instance().Initialize()) {
            LOG_WARNING("Failed to load config, using defaults", "InitializationThread");
        }

        // Get color output function
        MsgFn colorMsgFn = (MsgFn)GetProcAddress(GetModuleHandleW(L"tier0.dll"), ConColorMsg);
        if (!colorMsgFn) {
            LOG_ERROR("Failed to get ConColorMsg function", "InitializationThread");
            return;
        }

        // Initialize GUI
        InitializeGUI();
        LOG_INFO("GUI initialized", "InitializationThread");

        // Setup CVars
        auto& state = InjectorState::Instance();
        CCvar* cvarInterface = (CCvar*)GetInterface("vstdlib.dll", "VEngineCvar007");
        if (!cvarInterface) {
            LOG_ERROR("Failed to get CVars interface", "InitializationThread");
            return;
        }
        state.SetCVarInterface(cvarInterface);
        LOG_INFO("CVars interface obtained", "InitializationThread");

        // Get Lua interface with retries
        CLuaShared* luaShared = (CLuaShared*)GetInterface("lua_shared.dll", "LUASHARED003");
        if (!luaShared) {
            LOG_ERROR("Failed to get Lua shared interface", "InitializationThread");
            return;
        }

        PVOID luaInterface = nullptr;
        uint32_t totalWaitTime = 0;
        while (!luaInterface && totalWaitTime < INTERFACE_RETRY_TIMEOUT_MS) {
            luaInterface = luaShared->GetLuaInterface(0);
            if (!luaInterface) {
                Sleep(INTERFACE_RETRY_DELAY_MS);
                totalWaitTime += INTERFACE_RETRY_DELAY_MS;
            }
        }

        if (!luaInterface) {
            LOG_ERROR("Timeout waiting for Lua interface", "InitializationThread");
            return;
        }

        state.SetLuaInterface(luaInterface);
        LOG_INFO("Lua interface obtained", "InitializationThread");

        // Setup hook
        RunStringEx originalFn = (RunStringEx)VMTHook((PVOID**)luaInterface, 
                                                      (PVOID)HookedRunStringEx, 
                                                      VMT_RUNSTRINGEX_INDEX);
        if (!originalFn) {
            LOG_ERROR("Failed to hook RunStringEx", "InitializationThread");
            return;
        }

        state.SetOriginalRunStringEx(originalFn);
        LOG_INFO("RunStringEx hooked successfully", "InitializationThread");

        // Rehook on reconnect loop
        while (true) {
            Sleep(LUAINTERFACE_RETRY_DELAY_MS);

            luaInterface = luaShared->GetLuaInterface(0);
            if (!luaInterface) {
                // Wait for interface to become available again
                for (int i = 0; i < 1000 && !luaInterface; ++i) {
                    Sleep(LUAINTERFACE_RETRY_DELAY_MS);
                    luaInterface = luaShared->GetLuaInterface(0);
                }

                if (luaInterface) {
                    state.SetLuaInterface(luaInterface);
                    originalFn = (RunStringEx)VMTHook((PVOID**)luaInterface, 
                                                     (PVOID)HookedRunStringEx, 
                                                     VMT_RUNSTRINGEX_INDEX);
                    if (originalFn) {
                        state.SetOriginalRunStringEx(originalFn);
                        LOG_INFO("Re-hooked RunStringEx", "InitializationThread");
                    }
                }
            }
        }
    }
    catch (const std::exception& e) {
        LOG_CRITICAL("Initialization failed: " + std::string(e.what()), 
                    "InitializationThread");
    }
}

/**
 * @brief DLL entry point
 */
BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    if (ul_reason_for_call == DLL_PROCESS_ATTACH) {
        InitializeConsole();
        Logger::Instance().Initialize(Config::Instance().GetLogFilePath());
        LOG_INFO("DLL loaded", "DllMain");

        // Start initialization in background thread
        std::thread initThread(InitializationThread);
        initThread.detach();
    }
    else if (ul_reason_for_call == DLL_PROCESS_DETACH) {
        LOG_INFO("DLL unloaded", "DllMain");
        Logger::Instance().Flush();
    }

    return TRUE;
}
