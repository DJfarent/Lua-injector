#include <Windows.h>
#include <stdint.h>
#include <iostream>
#include <string>
#include <thread>
#include <fstream>
#include <sstream>

#include "Console.h"
#include "Color.h"
#include "Utils.h"
#include "CLuaShared.h"
#include "ICVar.h"
#include "Hooking.h"
#include "Config.h"
#include "Executor.h"

// Global variables
RunStringEx oRunStringEx = nullptr;
PVOID cLuaInterface = nullptr;
CCvar* cvarInterface = nullptr;
std::string lastFileName = "";

// Hook callback
bool __fastcall hkRunStringEx(PVOID _this,
#ifndef _WIN64
    void*,
#endif
    const char* filename, const char* path, const char* stringToRun, bool run, bool printErrors, bool dontPushErrors, bool noReturns)
{
    // Skip internal Lua calls
    if (!filename || !strlen(filename) || !strcmp(filename, "LuaCmd") || !strcmp(filename, "RunString(Ex)"))
    {
        return oRunStringEx(_this, filename, path, stringToRun, run, printErrors, dontPushErrors, noReturns);
    }

    lastFileName = std::string(filename);
    std::cout << "[HOOK] Intercepted script: " << filename << std::endl;
    
    // Execute the script
    bool result = oRunStringEx(_this, filename, path, stringToRun, run, printErrors, dontPushErrors, noReturns);
    std::cout << "[HOOK] Executed successfully!" << std::endl;
    
    return result;
}

// File picker dialog
std::string OpenFileDialog()
{
    OPENFILENAMEA ofn;
    CHAR szFile[260] = { 0 };

    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = NULL;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile);
    ofn.lpstrFilter = "Lua Files\0*.lua\0All Files\0*.*\0";
    ofn.nFilterIndex = 1;
    ofn.lpstrFileTitle = NULL;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = NULL;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

    if (GetOpenFileNameA(&ofn))
    {
        std::cout << "[FILE PICKER] Selected: " << szFile << std::endl;
        return std::string(szFile);
    }
    else
    {
        std::cout << "[FILE PICKER] Cancelled or error" << std::endl;
        return "";
    }
}

// Read file contents
std::string ReadFileContents(const std::string& path)
{
    std::ifstream file(path);
    if (!file.is_open())
    {
        std::cout << "[ERROR] Could not open file: " << path << std::endl;
        return "";
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    file.close();
    
    std::string contents = buffer.str();
    std::cout << "[FILE] Read " << contents.length() << " bytes" << std::endl;
    
    return contents;
}

// Input thread for manual testing
void InputThread()
{
    std::cout << "\n[INPUT] Commands: 'file' to load file, 'exit' to quit\n" << std::endl;
    
    while (true)
    {
        std::cout << "> ";
        std::string input;
        std::getline(std::cin, input);

        if (input == "file")
        {
            if (!oRunStringEx || !cLuaInterface)
            {
                std::cout << "[ERROR] Not ready yet!" << std::endl;
                continue;
            }

            std::string filePath = OpenFileDialog();
            if (filePath.empty())
            {
                std::cout << "[ERROR] No file selected" << std::endl;
                continue;
            }

            std::string fileContents = ReadFileContents(filePath);
            if (fileContents.empty())
            {
                std::cout << "[ERROR] Could not read file" << std::endl;
                continue;
            }

            Execute(filePath, fileContents);
        }
        else if (input == "exit")
        {
            break;
        }
    }
}

// Keyboard hook for INSERT key
LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    if (nCode == HC_ACTION)
    {
        if (wParam == WM_KEYDOWN)
        {
            KBDLLHOOKSTRUCT* pKeyBoard = (KBDLLHOOKSTRUCT*)lParam;
            
            // INSERT key = 45
            if (pKeyBoard->vkCode == VK_INSERT)
            {
                std::cout << "[KEYBOARD] INSERT pressed!" << std::endl;

                if (!oRunStringEx || !cLuaInterface)
                {
                    std::cout << "[ERROR] Not ready yet!" << std::endl;
                    return CallNextHookEx(NULL, nCode, wParam, lParam);
                }

                std::string filePath = OpenFileDialog();
                if (filePath.empty())
                {
                    return CallNextHookEx(NULL, nCode, wParam, lParam);
                }

                std::string fileContents = ReadFileContents(filePath);
                if (fileContents.empty())
                {
                    return CallNextHookEx(NULL, nCode, wParam, lParam);
                }

                Execute(filePath, fileContents);
            }
        }
    }

    return CallNextHookEx(NULL, nCode, wParam, lParam);
}

// Main injection function
void Main()
{
    std::cout << "\n========================================" << std::endl;
    std::cout << "  GmodLuaInjector - Fresh Build" << std::endl;
    std::cout << "========================================\n" << std::endl;

    // Get ConColorMsg for colored output
    HMODULE tier0 = GetModuleHandleW(L"tier0.dll");
    if (tier0)
    {
        #if _WIN64
        #define ConColorMsg "?ConColorMsg@@YAXAEBVColor@@PEBDZZ"
        #else
        #define ConColorMsg "?ConColorMsg@@YAXAEBVColor@@PBDZZ"
        #endif
        
        fn = (MsgFn)GetProcAddress(tier0, ConColorMsg);
        if (fn)
        {
            std::cout << "[OK] Got ConColorMsg" << std::endl;
            PrintWithPrefix("GmodLuaInjector Loaded!", Color(0, 255, 0));
        }
    }

    // Get CVars interface
    std::cout << "\n[INIT] Getting CVars interface..." << std::endl;
    cvarInterface = (CCvar*)GetInterface("vstdlib.dll", "VEngineCvar007");
    if (cvarInterface)
    {
        std::cout << "[OK] CVars: " << cvarInterface << std::endl;
        PrintWithPrefix("CVars OK!", Color(0, 255, 0));
    }
    else
    {
        std::cout << "[ERROR] Failed to get CVars" << std::endl;
        PrintWithPrefix("CVars FAILED!", Color(255, 0, 0));
    }

    // Get Lua interface
    std::cout << "\n[INIT] Getting Lua interface..." << std::endl;
    CLuaShared* LuaShared = (CLuaShared*)GetInterface("lua_shared.dll", "LUASHARED003");
    if (!LuaShared)
    {
        std::cout << "[ERROR] Failed to get LuaShared" << std::endl;
        return;
    }
    std::cout << "[OK] LuaShared: " << LuaShared << std::endl;

    // Wait for server join
    std::cout << "\n[INIT] Waiting for Lua interface (join a server)..." << std::endl;
    int waitCount = 0;
    while (!cLuaInterface)
    {
        cLuaInterface = LuaShared->GetLuaInterface(0);
        if (!cLuaInterface)
        {
            if (waitCount % 20 == 0)
            {
                std::cout << "[WAIT] Still waiting..." << std::endl;
                PrintWithPrefix("Join a server...", Color(255, 165, 0));
            }
            waitCount++;
            Sleep(100);
        }
    }

    std::cout << "[OK] Lua interface found: " << cLuaInterface << std::endl;
    PrintWithPrefix("Lua Interface Found!", Color(0, 255, 0));

    // Try hooking at different offsets
    std::cout << "\n[HOOK] Attempting to hook RunStringEx..." << std::endl;
    int offsets[] = { 111, 112, 113, 110, 114, 115 };
    bool hooked = false;

    for (int offset : offsets)
    {
        std::cout << "[HOOK] Trying offset " << offset << "..." << std::endl;
        oRunStringEx = (RunStringEx)VMTHook((PVOID**)cLuaInterface, hkRunStringEx, offset);
        
        if (oRunStringEx)
        {
            std::cout << "[OK] HOOKED at offset " << offset << "!" << std::endl;
            PrintWithPrefix("Hooked!", Color(0, 255, 0));
            hooked = true;
            break;
        }
    }

    if (!hooked)
    {
        std::cout << "[ERROR] Failed to hook at any offset!" << std::endl;
        PrintWithPrefix("Hook Failed!", Color(255, 0, 0));
        return;
    }

    std::cout << "\n[READY] Injector ready!" << std::endl;
    std::cout << "[INFO] Press INSERT in Gmod to open file picker" << std::endl;
    std::cout << "[INFO] Or type 'file' here to manually load a file\n" << std::endl;

    // Start keyboard hook
    HHOOK keyboardHook = SetWindowsHookExA(WH_KEYBOARD_LL, KeyboardProc, NULL, 0);
    if (keyboardHook)
    {
        std::cout << "[OK] Keyboard hook installed" << std::endl;
    }
    else
    {
        std::cout << "[WARNING] Keyboard hook failed" << std::endl;
    }

    // Start input thread
    std::thread(InputThread).detach();

    // Keep main thread alive
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    UnhookWindowsHookEx(keyboardHook);
}

// DLL entry point
BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    if (ul_reason_for_call == DLL_PROCESS_ATTACH)
{
    AllocConsole();
    FILE* f;
    freopen_s(&f, "CONOUT$", "w", stdout);
    freopen_s(&f, "CONIN$", "r", stdin);
    SetConsoleTitle(L"GmodLuaInjector - Console");

    std::thread(Main).detach();
}
    return TRUE;
}
