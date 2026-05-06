#pragma once
#include <Windows.h>
#include <string>
#include <iostream>
#include "Console.h"
#include "CLuaShared.h"
#include "ICVar.h"

typedef __int64(__fastcall* RunStringEx)(PVOID _this, const char* filename, const char* path, const char* stringToRun, bool run, bool printErrors, bool dontPushErrors, bool noReturns);

extern RunStringEx oRunStringEx;
extern std::string lastFileName;

inline void Execute(const std::string& fileName, const std::string& stringToRun)
{
    CLuaShared* LuaShared = (CLuaShared*)GetInterface("lua_shared.dll", "LUASHARED003");
    if (!LuaShared)
    {
        std::cout << "[EXEC] ERROR: Could not get Lua interface" << std::endl;
        PrintWithPrefix("ERROR: Lua interface not found!", Color(255, 0, 0));
        return;
    }

    PVOID cLuaInterface = LuaShared->GetLuaInterface(0);
    if (!cLuaInterface)
    {
        std::cout << "[EXEC] ERROR: Lua interface is null" << std::endl;
        PrintWithPrefix("ERROR: Lua interface is null!", Color(255, 0, 0));
        return;
    }

    if (!oRunStringEx)
    {
        std::cout << "[EXEC] ERROR: RunStringEx not hooked" << std::endl;
        PrintWithPrefix("ERROR: RunStringEx not hooked!", Color(255, 0, 0));
        return;
    }

    std::cout << "[EXEC] Executing: " << fileName << std::endl;
    PrintWithPrefix("Executing: " + fileName, Color(0, 154, 255));

    try
    {
        __int64 result = oRunStringEx(
            cLuaInterface,
            lastFileName.c_str(),
            "",
            stringToRun.c_str(),
            true,
            true,
            true,
            true
        );

        std::cout << "[EXEC] SUCCESS! Result: " << result << std::endl;
        PrintWithPrefix("Executed successfully!", Color(0, 255, 0));
    }
    catch (const std::exception& e)
    {
        std::cout << "[EXEC] Exception: " << e.what() << std::endl;
        PrintWithPrefix("Exception!", Color(255, 0, 0));
    }
    catch (...)
    {
        std::cout << "[EXEC] Unknown exception!" << std::endl;
        PrintWithPrefix("Unknown error!", Color(255, 0, 0));
    }
}
