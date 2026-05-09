#pragma once
#include <fstream>
#include <Windows.h>
#include <string>
#include <iostream>
#include "Console.h"
#include "CLuaShared.h"
#include "ICVar.h"

typedef int(__fastcall* RunStringEx)(PVOID _this, const char* filename, const char* path, const char* stringToRun, bool run, bool printErrors, bool dontPushErrors, bool noReturns);

extern RunStringEx oRunStringEx;
extern std::string lastFileName;

inline void Execute(const std::string& fileName, const std::string& stringToRun)
{
   std::ofstream log("C:\\Users\\marce\\Documents\\GitHub\\Lua injector\\errors.txt", std::ios::app);
    log << "[EXEC] Starting execution..." << std::endl;

    CLuaShared* LuaShared = (CLuaShared*)GetInterface("lua_shared.dll", "LUASHARED003");
    if (!LuaShared)
    {
        log << "[EXEC] ERROR: No LuaShared" << std::endl;
        PrintWithPrefix("ERROR: Lua interface not found!", Color(255, 0, 0));
        return;
    }

    PVOID cLuaInterface = LuaShared->GetLuaInterface(0);
    if (!cLuaInterface)
    {
        log << "[EXEC] ERROR: No interface" << std::endl;
        PrintWithPrefix("ERROR: Lua interface is null!", Color(255, 0, 0));
        return;
    }

    if (!oRunStringEx)
    {
        log << "[EXEC] ERROR: Not hooked" << std::endl;
        PrintWithPrefix("ERROR: RunStringEx not hooked!", Color(255, 0, 0));
        return;
    }

    log << "[EXEC] Executing: " << fileName << std::endl;
    log << "[EXEC] Size: " << stringToRun.length() << " bytes" << std::endl;
    PrintWithPrefix("Executing: " + fileName, Color(0, 154, 255));

    typedef int  (__fastcall* LuaTopFn)(PVOID _this);
    typedef void (__fastcall* LuaPopFn)(PVOID _this, int iAmt);

    PVOID* vtable  = *(PVOID**)cLuaInterface;
    LuaTopFn fnTop = (LuaTopFn)vtable[0];
    LuaPopFn fnPop = (LuaPopFn)vtable[2];

    int stackBefore = fnTop(cLuaInterface);
    log << "[EXEC] Stack before: " << stackBefore << std::endl;
    std::cout << "[EXEC] Stack before: " << stackBefore << std::endl;

    log << "[EXEC] Executing file: " << fileName << std::endl;
    log << "[EXEC] Script size: " << stringToRun.length() << " bytes" << std::endl;
    std::cout << "[EXEC] Executing file: " << fileName << std::endl;
    std::cout << "[EXEC] Script size: " << stringToRun.length() << " bytes" << std::endl;
    PrintWithPrefix("Executing: " + fileName, Color(0, 154, 255));

    try
    {
        oRunStringEx(
            cLuaInterface,
            fileName.c_str(),
            "",
            stringToRun.c_str(),
            true,
            true,
            true,
            true
        );

        int stackAfter = fnTop(cLuaInterface);
        log << "[EXEC] Stack after: " << stackAfter << std::endl;
        std::cout << "[EXEC] Stack after: " << stackAfter << std::endl;

        if (stackAfter > stackBefore)
        {
            log << "[EXEC] Cleaning " << (stackAfter - stackBefore) << " leaked stack value(s)" << std::endl;
            std::cout << "[EXEC] Cleaning " << (stackAfter - stackBefore) << " leaked stack value(s)" << std::endl;
            fnPop(cLuaInterface, stackAfter - stackBefore);
        }

        log << "[EXEC] Done." << std::endl;
        std::cout << "[EXEC] Done." << std::endl;
        PrintWithPrefix("Script executed!", Color(0, 255, 0));
    }
    catch (const std::exception& e)
    {
        log << "[EXEC] Exception: " << e.what() << std::endl;
        std::cout << "[EXEC] Exception: " << e.what() << std::endl;
        PrintWithPrefix("Exception!", Color(255, 0, 0));
    }
    catch (...)
    {
        log << "[EXEC] Unknown exception!" << std::endl;
        std::cout << "[EXEC] Unknown exception!" << std::endl;
        PrintWithPrefix("Unknown error!", Color(255, 0, 0));
    }
}