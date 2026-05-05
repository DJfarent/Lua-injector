#pragma once

#include <Windows.h>
#include <string>

class Config {
public:
    static Config& Instance() {
        static Config instance;
        return instance;
    }

    const std::string& GetScriptDirectory() const { return scriptDirectory; }
    void SetScriptDirectory(const std::string& path) { scriptDirectory = path; }

    uint32_t GetMenuHotkey() const { return menuHotkey; }
    void SetMenuHotkey(uint32_t key) { menuHotkey = key; }

    const std::string& GetLogFilePath() const { return logFilePath; }
    
private:
    Config() = default;
    ~Config() = default;

    Config(const Config&) = delete;
    Config& operator=(const Config&) = delete;

    std::string scriptDirectory = "C:/GaztoofScriptHook/";
    std::string logFilePath = "gmod_injector.log";
    uint32_t menuHotkey = VK_INSERT;
};