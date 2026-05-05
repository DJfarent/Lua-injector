```cpp
#include "SecurityValidator.h"
#include "Logger.h"
#include <algorithm>
#include <cctype>

const std::vector<std::string> SecurityValidator::DANGEROUS_LUA_FUNCTIONS = {
    "os.execute",
    "os.remove",
    "io.popen",
    "debug.sethook",
    "loadstring",
    "dofile"
};

std::string SecurityValidator::ValidatePath(const std::string& path, 
                                           const std::string& allowedBaseDir) {
    if (path.empty()) {
        LOG_WARNING("Empty path provided", "ValidatePath");
        return "";
    }

    // Check for path traversal attempts
    if (ContainsPathTraversalAttempt(path)) {
        LOG_WARNING("Path traversal attempt detected: " + path, "ValidatePath");
        return "";
    }

    // Check length
    if (path.length() > 260) {  // MAX_PATH
        LOG_WARNING("Path too long: " + path, "ValidatePath");
        return "";
    }

    // Resolve relative paths if base directory provided
    std::string resolved = path;
    if (!allowedBaseDir.empty()) {
        if (path.find("..") != std::string::npos) {
            LOG_WARNING("Relative path containing .. detected", "ValidatePath");
            return "";
        }
        
        // Ensure path starts with base directory
        if (path.find(allowedBaseDir) != 0) {
            resolved = allowedBaseDir + "/" + path;
        }
    }

    return resolved;
}

bool SecurityValidator::IsPathSafe(const std::string& path) {
    if (path.empty()) return true;

    // Check for suspicious patterns
    const char* patterns[] = { "..", "~", ":", "|" };
    for (const char* pattern : patterns) {
        if (path.find(pattern) != std::string::npos) {
            return false;
        }
    }

    return true;
}

std::string SecurityValidator::SanitizeFilename(const std::string& filename) {
    std::string result = filename;
    
    // Remove invalid characters
    auto it = std::remove_if(result.begin(), result.end(), [](char c) {
        const char* invalid = INVALID_FILENAME_CHARS;
        return std::strchr(invalid, c) != nullptr;
    });
    result.erase(it, result.end());

    return result;
}

bool SecurityValidator::ValidateStringLength(const std::string& str, size_t maxLength) {
    return str.length() <= maxLength;
}

bool SecurityValidator::IsAsciiOnly(const std::string& str) {
    return std::all_of(str.begin(), str.end(), [](char c) {
        return (unsigned char)c < 128;
    });
}

std::string SecurityValidator::SanitizeString(const std::string& str, 
                                             bool allowSpecialChars) {
    std::string result;
    for (char c : str) {
        if (std::isprint(c) || c == '\n' || c == '\t') {
            result += c;
        }
    }
    return result;
}

bool SecurityValidator::ValidateLuaCode(const std::string& code) {
    // Basic checks
    if (code.empty()) return false;
    if (code.length() > 1024 * 1024) {  // 1 MB limit
        return false;
    }

    // Check for balanced brackets
    int braces = 0, parens = 0;
    for (char c : code) {
        if (c == '{') braces++;
        else if (c == '}') braces--;
        else if (c == '(') parens++;
        else if (c == ')') parens--;

        if (braces < 0 || parens < 0) return false;
    }

    return braces == 0 && parens == 0;
}

bool SecurityValidator::HasDangerousFunctions(const std::string& code) {
    for (const auto& func : DANGEROUS_LUA_FUNCTIONS) {
        if (code.find(func) != std::string::npos) {
            LOG_WARNING("Dangerous Lua function detected: " + func, 
                       "HasDangerousFunctions");
            return true;
        }
    }
    return false;
}

const std::vector<std::string>& SecurityValidator::GetDangerousFunctions() {
    return DANGEROUS_LUA_FUNCTIONS;
}

bool SecurityValidator::SafeStringCopy(char* dest, size_t destSize, 
                                      const std::string& src) {
    if (!dest || destSize == 0) return false;
    if (src.length() >= destSize) return false;

    strncpy_s(dest, destSize, src.c_str(), destSize - 1);
    return true;
}

bool SecurityValidator::SafeBufferRead(const void* buffer, size_t bufferSize,
                                      void* dest, size_t destSize, size_t offset) {
    if (!buffer || !dest || offset + destSize > bufferSize) {
        return false;
    }

    memcpy_s(dest, destSize, (const char*)buffer + offset, destSize);
    return true;
}

bool SecurityValidator::ContainsPathTraversalAttempt(const std::string& path) {
    // Check for ../ or ..\
    return path.find("..") != std::string::npos;
}
```

