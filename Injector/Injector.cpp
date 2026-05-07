#include <Windows.h>
#include <TlHelp32.h>
#include <iostream>
#include <string>

// Find process by name
DWORD FindProcessByName(const char* processName)
{
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE)
    {
        std::cout << "[ERROR] Could not create snapshot" << std::endl;
        return 0;
    }

    PROCESSENTRY32 pe32;
    pe32.dwSize = sizeof(PROCESSENTRY32);

    if (!Process32First(hSnapshot, &pe32))
    {
        std::cout << "[ERROR] Could not get first process" << std::endl;
        CloseHandle(hSnapshot);
        return 0;
    }

    do
    {
        if (strcmp(pe32.szExeFile, processName) == 0)
        {
            std::cout << "[OK] Found process: " << pe32.szExeFile << " (PID: " << pe32.th32ProcessID << ")" << std::endl;
            CloseHandle(hSnapshot);
            return pe32.th32ProcessID;
        }
    } while (Process32Next(hSnapshot, &pe32));

    CloseHandle(hSnapshot);
    return 0;
}

// Inject DLL into process
bool InjectDLL(DWORD processID, const char* dllPath)
{
    // Open process
    HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, processID);
    if (!hProcess)
    {
        std::cout << "[ERROR] Could not open process (PID: " << processID << ")" << std::endl;
        std::cout << "[ERROR] Make sure Gmod is running!" << std::endl;
        return false;
    }
    std::cout << "[OK] Opened process handle" << std::endl;

    // Allocate memory for DLL path
    size_t pathLen = strlen(dllPath) + 1;
    LPVOID remoteBuffer = VirtualAllocEx(hProcess, NULL, pathLen, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!remoteBuffer)
    {
        std::cout << "[ERROR] Could not allocate memory in target process" << std::endl;
        CloseHandle(hProcess);
        return false;
    }
    std::cout << "[OK] Allocated remote memory" << std::endl;

    // Write DLL path to memory
    if (!WriteProcessMemory(hProcess, remoteBuffer, (LPVOID)dllPath, pathLen, NULL))
    {
        std::cout << "[ERROR] Could not write to target process memory" << std::endl;
        VirtualFreeEx(hProcess, remoteBuffer, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return false;
    }
    std::cout << "[OK] Wrote DLL path to memory" << std::endl;

    // Get LoadLibraryA address
    LPVOID loadLibraryAddr = (LPVOID)GetProcAddress(GetModuleHandleA("kernel32.dll"), "LoadLibraryA");
    if (!loadLibraryAddr)
    {
        std::cout << "[ERROR] Could not get LoadLibraryA address" << std::endl;
        VirtualFreeEx(hProcess, remoteBuffer, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return false;
    }
    std::cout << "[OK] Got LoadLibraryA address: " << loadLibraryAddr << std::endl;

    // Create remote thread
    HANDLE hThread = CreateRemoteThread(hProcess, NULL, 0, (LPTHREAD_START_ROUTINE)loadLibraryAddr, remoteBuffer, 0, NULL);
    if (!hThread)
    {
        std::cout << "[ERROR] Could not create remote thread" << std::endl;
        VirtualFreeEx(hProcess, remoteBuffer, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return false;
    }
    std::cout << "[OK] Created remote thread" << std::endl;

    // Wait for thread to finish
    std::cout << "[WAIT] Waiting for DLL to load..." << std::endl;
    WaitForSingleObject(hThread, INFINITE);

    DWORD exitCode = 0;
    GetExitCodeThread(hThread, &exitCode);
    std::cout << "[OK] Thread exited with code: " << exitCode << std::endl;

    // Cleanup
    CloseHandle(hThread);
    VirtualFreeEx(hProcess, remoteBuffer, 0, MEM_RELEASE);
    CloseHandle(hProcess);

    return true;
}

int main()
{
    std::cout << "\n========================================" << std::endl;
    std::cout << "  GmodLuaInjector - DLL Injector" << std::endl;
    std::cout << "========================================\n" << std::endl;

    // Get current directory
    char currentDir[MAX_PATH];
    GetCurrentDirectoryA(MAX_PATH, currentDir);
    std::cout << "[INFO] Current directory: " << currentDir << std::endl;

    // Build DLL path
    char dllPath[MAX_PATH];
    sprintf_s(dllPath, MAX_PATH, "%s\\GmodLuaInjector.dll", currentDir);
    std::cout << "[INFO] DLL path: " << dllPath << std::endl;

    // Check if DLL exists
    if (GetFileAttributesA(dllPath) == INVALID_FILE_ATTRIBUTES)
    {
        std::cout << "[ERROR] DLL not found!" << std::endl;
        std::cout << "[ERROR] Make sure you compiled the DLL first!" << std::endl;
        std::cout << "[ERROR] Expected at: " << dllPath << std::endl;
        system("pause");
        return 1;
    }
    std::cout << "[OK] DLL file exists" << std::endl;

    // Find Gmod process
    std::cout << "\n[SEARCH] Looking for Garry's Mod x64 process..." << std::endl;
    DWORD processID = FindProcessByName("gmod.exe");

    
    if (processID == 0)
    {
        std::cout << "[ERROR] Could not find hl2.exe process!" << std::endl;
        std::cout << "[ERROR] Make sure Garry's Mod is running!" << std::endl;
        std::cout << "[ERROR] Try launching with: gmod.exe -dxlevel 90" << std::endl;
        system("pause");
        return 1;
    }

    // Inject DLL
    std::cout << "\n[INJECT] Injecting DLL into Gmod..." << std::endl;
    if (!InjectDLL(processID, dllPath))
    {
        std::cout << "\n[ERROR] Injection failed!" << std::endl;
        system("pause");
        return 1;
    }

    std::cout << "\n========================================" << std::endl;
    std::cout << "✓ DLL INJECTED SUCCESSFULLY!" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "\nIn Gmod:" << std::endl;
    std::cout << "  1. Press INSERT to open file picker" << std::endl;
    std::cout << "  2. Select a .lua file" << std::endl;
    std::cout << "  3. Watch it execute!" << std::endl;
    std::cout << "\nKeep this window open for console output.\n" << std::endl;

    system("pause");
    return 0;
}
