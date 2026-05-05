#pragma once

#include <Windows.h>
#include <string>
#include <vector>
#include <algorithm>
#include <regex>

/**
 * @brief Security validation and sanitization utilities
 * Prevents common attack vectors like path traversal, buffer overflows, etc.
 */
class SecurityValidator {
public:
    // ============ PATH VALIDATION ============

    /**
     * @brief Canonicalize a path and prevent directory traversal
     * Removes ../../, resolves to absolute path, checks for escape attempts
     * @param path Path to validate
     * @param allowedBaseDir Base directory that all paths must resolve within
     * @return Sanitized path, or empty string if invalid
     */
    static std::string ValidatePath(const std::string& path, const std::string& allowedBaseDir = "");

    /**
     * @brief Check if path contains any suspicious patterns
     * @return true if path is safe, false if potentially dangerous
     */
    static bool IsPathSafe(const std::string& path);

    /**
     * @brief Remove characters that are invalid in Windows filenames
     */
    static std::string SanitizeFilename(const std::string& filename);

    // ============ STRING VALIDATION ============

    /**
     * @brief Validate string doesn't exceed maximum length
     */
    static bool ValidateStringLength(const std::string& str, size_t maxLength);

    /**
     * @brief Check if string contains only printable ASCII characters
     */
    static bool IsAsciiOnly(const std::string& str);

    /**
     * @brief Sanitize string for safe display/storage
     */
    static std::string SanitizeString(const std::string& str, bool allowSpecialChars = false);

    // ============ LUA CODE VALIDATION ============

    /**
     * @brief Basic Lua syntax validation
     * Checks for obviously broken code patterns
     */
    static bool ValidateLuaCode(const std::string& code);

    /**
     * @brief Check if Lua code contains potentially dangerous functions
     */
    static bool HasDangerousFunctions(const std::string& code);

    /**
     * @brief List of functions that should trigger warnings
     */
    static const std::vector<std::string>& GetDangerousFunctions();

    // ============ BUFFER OPERATIONS ============

    /**
     * @brief Safe string copy with bounds checking
     * @return true if copy succeeded, false if source too large
     */
    static bool SafeStringCopy(char* dest, size_t destSize, const std::string& src);

    /**
     * @brief Safe buffer read with bounds checking
     */
    static bool SafeBufferRead(const void* buffer, size_t bufferSize, void* dest, 
                               size_t destSize, size_t offset);

private:
    // Prevent instantiation
    SecurityValidator() = delete;
    ~SecurityValidator() = delete;

    static constexpr const char* INVALID_FILENAME_CHARS = "<>:\"|?*";
    static constexpr const char* PATH_TRAVERSAL_PATTERNS[] = {
        "..",
        "\\",
        ":",
        "~"
    };

    static const std::vector<std::string> DANGEROUS_LUA_FUNCTIONS;
    
    static std::string ResolveRelativePath(const std::string& path);
    static bool ContainsPathTraversalAttempt(const std::string& path);
};

/**
 * @brief RAII wrapper for safe string operations
 * Automatically validates and sanitizes input
 */
class SafeString {
public:
    SafeString(const std::string& input, size_t maxLength = 65535)
        : value(input) {
        if (!SecurityValidator::ValidateStringLength(value, maxLength)) {
            throw std::runtime_error("String exceeds maximum length");
        }
        value = SecurityValidator::SanitizeString(value);
    }

    const std::string& Get() const { return value; }
    operator const std::string&() const { return value; }

private:
    std::string value;
};

/**
 * @brief RAII wrapper for safe file paths
 */
class SafePath {
public:
    SafePath(const std::string& path, const std::string& baseDir = "")
        : path(SecurityValidator::ValidatePath(path, baseDir)) {
        if (path.empty()) {
            throw std::runtime_error("Invalid file path");
        }
    }

    const std::string& Get() const { return path; }
    operator const std::string&() const { return path; }

private:
    std::string path;
};
