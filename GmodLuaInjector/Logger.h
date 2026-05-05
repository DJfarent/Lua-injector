#pragma once

#include <Windows.h>
#include <string>
#include <sstream>
#include <fstream>
#include <ctime>
#include <mutex>

/**
 * @brief Logging levels for categorizing messages
 */
enum class LogLevel {
    Debug = 0,      // Detailed diagnostic information
    Info = 1,       // General informational messages
    Warning = 2,    // Warning messages
    Error = 3,      // Error messages
    Critical = 4    // Critical failures
};

/**
 * @brief Thread-safe logging system
 * Logs to both file and console with timestamps
 */
class Logger {
public:
    static Logger& Instance();

    /**
     * @brief Initialize logger with file path
     */
    bool Initialize(const std::string& logFilePath);

    /**
     * @brief Log a message with specified level
     */
    void Log(LogLevel level, const std::string& message, const std::string& context = "");

    /**
     * @brief Convenience methods
     */
    void Debug(const std::string& msg, const std::string& ctx = "") { Log(LogLevel::Debug, msg, ctx); }
    void Info(const std::string& msg, const std::string& ctx = "") { Log(LogLevel::Info, msg, ctx); }
    void Warning(const std::string& msg, const std::string& ctx = "") { Log(LogLevel::Warning, msg, ctx); }
    void Error(const std::string& msg, const std::string& ctx = "") { Log(LogLevel::Error, msg, ctx); }
    void Critical(const std::string& msg, const std::string& ctx = "") { Log(LogLevel::Critical, msg, ctx); }

    /**
     * @brief Set minimum log level (below this won't be logged)
     */
    void SetMinimumLevel(LogLevel level) { minimumLevel = level; }

    /**
     * @brief Enable/disable console output
     */
    void SetConsoleOutput(bool enabled) { consoleOutput = enabled; }

    /**
     * @brief Enable/disable file output
     */
    void SetFileOutput(bool enabled) { fileOutput = enabled; }

    /**
     * @brief Flush any pending log entries
     */
    void Flush();

private:
    Logger() = default;
    ~Logger() { Flush(); if (logFile.is_open()) logFile.close(); }

    // Prevent copying
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    std::string GetTimestamp() const;
    std::string GetLevelString(LogLevel level) const;
    std::string FormatMessage(LogLevel level, const std::string& message, const std::string& context) const;

    std::ofstream logFile;
    std::mutex logMutex;
    bool consoleOutput = true;
    bool fileOutput = true;
    LogLevel minimumLevel = LogLevel::Debug;
};

/**
 * @brief RAII helper for scoped logging
 */
class LogScope {
public:
    LogScope(const std::string& context)
        : context(context) {
        Logger::Instance().Debug("Enter: " + context, context);
    }

    ~LogScope() {
        Logger::Instance().Debug("Exit: " + context, context);
    }

private:
    std::string context;
};

// Macro for convenient logging
#define LOG_DEBUG(msg) Logger::Instance().Debug(msg, __FUNCTION__)
#define LOG_INFO(msg) Logger::Instance().Info(msg, __FUNCTION__)
#define LOG_WARNING(msg) Logger::Instance().Warning(msg, __FUNCTION__)
#define LOG_ERROR(msg) Logger::Instance().Error(msg, __FUNCTION__)
#define LOG_CRITICAL(msg) Logger::Instance().Critical(msg, __FUNCTION__)
#define LOG_SCOPE() LogScope _scope(__FUNCTION__)
