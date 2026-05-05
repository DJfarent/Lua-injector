#pragma once

#include <Windows.h>
#include <string>
#include <vector>
#include <filesystem>
#include <memory>

namespace fs = std::filesystem;

/**
 * @brief Safe file operations with error handling
 */
class FileUtils {
public:
    /**
     * @brief Read entire file into string
     * @return File contents, or empty string if failed
     */
    static std::string ReadFile(const std::string& path);

    /**
     * @brief Write string to file, creating directories if needed
     * @return true if successful
     */
    static bool WriteFile(const std::string& path, const std::string& content);

    /**
     * @brief Create directory recursively
     * @return true if successful or already exists
     */
    static bool CreateDirectoryRecursive(const std::string& path);

    /**
     * @brief Check if file exists
     */
    static bool FileExists(const std::string& path);

    /**
     * @brief Get file extension
     */
    static std::string GetFileExtension(const std::string& path);

    /**
     * @brief Get directory from path
     */
    static std::string GetDirectory(const std::string& path);

    /**
     * @brief Get file size
     */
    static size_t GetFileSize(const std::string& path);

    /**
     * @brief List files in directory
     */
    static std::vector<std::string> ListFiles(const std::string& directory, 
                                             const std::string& extension = "");

private:
    FileUtils() = delete;
};

/**
 * @brief Safe string operations
 */
class StringUtils {
public:
    /**
     * @brief Convert string to lowercase
     */
    static std::string ToLower(const std::string& str);

    /**
     * @brief Convert string to uppercase
     */
    static std::string ToUpper(const std::string& str);

    /**
     * @brief Trim whitespace from both ends
     */
    static std::string Trim(const std::string& str);

    /**
     * @brief Split string by delimiter
     */
    static std::vector<std::string> Split(const std::string& str, char delimiter);

    /**
     * @brief Replace all occurrences
     */
    static std::string Replace(const std::string& str, const std::string& from, 
                              const std::string& to);

    /**
     * @brief Check if string starts with prefix
     */
    static bool StartsWith(const std::string& str, const std::string& prefix);

    /**
     * @brief Check if string ends with suffix
     */
    static bool EndsWith(const std::string& str, const std::string& suffix);

    /**
     * @brief Generate random string (for unique identifiers)
     * @param length Length of random string
     * @param useSpecialChars Include +*-/ characters
     */
    static std::string GenerateRandomString(size_t length, bool useSpecialChars = true);

    /**
     * @brief Format string with sprintf-like syntax (safe)
     */
    static std::string Format(const char* format, ...);

private:
    StringUtils() = delete;
};

/**
 * @brief Windows-specific utilities
 */
class WindowsUtils {
public:
    /**
     * @brief Get module handle safely
     */
    static HMODULE GetModuleHandleSafe(const std::string& moduleName);

    /**
     * @brief Get procedure address safely
     */
    static FARPROC GetProcAddressSafe(HMODULE module, const std::string& procName);

    /**
     * @brief Convert between ANSI and Unicode strings
     */
    static std::wstring AnsiToUnicode(const std::string& ansi);
    static std::string UnicodeToAnsi(const std::wstring& unicode);

    /**
     * @brief Find window by class name or window name with retry
     */
    static HWND FindWindowEx(const std::string& className, const std::string& windowName, 
                            int maxRetries = 10, int retryDelayMs = 100);

    /**
     * @brief Get current username
     */
    static std::string GetUsername();

    /**
     * @brief Get program path
     */
    static std::string GetProgramPath();

private:
    WindowsUtils() = delete;
};

/**
 * @brief Memory-safe RAII wrapper for raw buffers
 */
class SafeBuffer {
public:
    explicit SafeBuffer(size_t size)
        : buffer(std::make_unique<char[]>(size)), size(size) {}

    char* Data() { return buffer.get(); }
    const char* Data() const { return buffer.get(); }
    
    size_t Size() const { return size; }

    void Clear() { std::fill(buffer.get(), buffer.get() + size, 0); }

    // Prevent copying, allow moving
    SafeBuffer(const SafeBuffer&) = delete;
    SafeBuffer& operator=(const SafeBuffer&) = delete;
    SafeBuffer(SafeBuffer&&) = default;
    SafeBuffer& operator=(SafeBuffer&&) = default;

private:
    std::unique_ptr<char[]> buffer;
    size_t size;
};
