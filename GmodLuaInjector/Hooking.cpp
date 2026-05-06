#include <Windows.h>
#include <iostream>
#include "Hooking.h"

PVOID VMTHook(PVOID** src, PVOID dst, int index)
{
    if (!src || !*src || !dst)
    {
        std::cout << "[HOOK] Invalid parameters!" << std::endl;
        return nullptr;
    }

    PVOID* VMT = *src;
    PVOID original = VMT[index];

    DWORD oldProtection;
    if (!VirtualProtect(&VMT[index], sizeof(PVOID), PAGE_EXECUTE_READWRITE, &oldProtection))
    {
        std::cout << "[HOOK] VirtualProtect failed!" << std::endl;
        return nullptr;
    }

    VMT[index] = dst;

    VirtualProtect(&VMT[index], sizeof(PVOID), oldProtection, &oldProtection);

    std::cout << "[HOOK] VMT hook applied at index " << index << std::endl;
    return original;
}
