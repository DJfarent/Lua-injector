```cpp
#include "Logger.h"
#include <chrono>
#include <iomanip>

Logger& Logger::Instance() {
    static Logger instance;
    return instance;
}

bool Logger::Initialize(const std::string& logFilePath) {
    std::lock_guard<std::mutex> lock(logMutex);
    
    logFile.open(logFilePath, std::ios::app);
    if (!logFile.is_open()) {
        return false;
    }
    
    Log(LogLevel::Info, "Logger initialized", "Logger");
    return true;
}

std::string Logger::GetTimestamp() const {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;
    
    std::ostringstream oss;
    oss << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S")
        << '.' << std::setfill('0') << std::setw(3) << ms.count();
    
    return oss.str();
}

std::string Logger::GetLevelString(LogLevel level) const {
    switch (level) {
        case LogLevel::Debug:    return "DEBUG";
        case LogLevel::Info:     return "INFO";
        case LogLevel::Warning:  return "WARNING";
        case LogLevel::Error:    return "ERROR";
        case LogLevel::Critical: return "CRITICAL";
        default:                 return "UNKNOWN";
    }
}

std::string Logger::FormatMessage(LogLevel level, const std::string& message, 
                                 const std::string& context) const {
    std::ostringstream oss;
    oss << "[" << GetTimestamp() << "] "
        << "[" << GetLevelString(level) << "] "
        << "[" << context << "] "
        << message;
    return oss.str();
}

void Logger::Log(LogLevel level, const std::string& message, 
                const std::string& context) {
    if (level < minimumLevel) {
        return;
    }

    std::lock_guard<std::mutex> lock(logMutex);
    
    std::string formatted = FormatMessage(level, message, context);
    
    if (consoleOutput) {
        std::cout << formatted << std::endl;
    }
    
    if (fileOutput && logFile.is_open()) {
        logFile << formatted << std::endl;
        logFile.flush();
    }
}

void Logger::Flush() {
    std::lock_guard<std::mutex> lock(logMutex);
    if (logFile.is_open()) {
        logFile.flush();
    }
}
```

