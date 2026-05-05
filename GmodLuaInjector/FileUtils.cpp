```cpp
#include "ModernUtils.h"
#include "Logger.h"
#include <fstream>
#include <sstream>

std::string FileUtils::ReadFile(const std::string& path) {
    try {
        std::ifstream file(path, std::ios::binary);
        if (!file.is_open()) {
            LOG_WARNING("Failed to open file: " + path, "FileUtils::ReadFile");
            return "";
        }

        std::stringstream buffer;
        buffer << file.rdbuf();
        return buffer.str();
    }
    catch (const std::exception& e) {
        LOG_ERROR("Exception reading file: " + std::string(e.what()), 
                 "FileUtils::ReadFile");
        return "";
    }
}

bool FileUtils::WriteFile(const std::string& path, const std::string& content) {
    try {
        // Ensure directory exists
        fs::path filePath(path);
        fs::create_directories(filePath.parent_path());

        std::ofstream file(path, std::ios::binary);
        if (!file.is_open()) {
            LOG_WARNING("Failed to open file for writing: " + path, 
                       "FileUtils::WriteFile");
            return false;
        }

        file.write(content.c_str(), content.size());
        file.close();

        LOG_DEBUG("File written: " + path, "FileUtils::WriteFile");
        return true;
    }
    catch (const std::exception& e) {
        LOG_ERROR("Exception writing file: " + std::string(e.what()), 
                 "FileUtils::WriteFile");
        return false;
    }
}

bool FileUtils::CreateDirectoryRecursive(const std::string& path) {
    try {
        fs::create_directories(path);
        LOG_DEBUG("Directory created: " + path, "FileUtils::CreateDirectoryRecursive");
        return true;
    }
    catch (const std::exception& e) {
        LOG_WARNING("Failed to create directory: " + std::string(e.what()), 
                   "FileUtils::CreateDirectoryRecursive");
        return false;
    }
}

bool FileUtils::FileExists(const std::string& path) {
    try {
        return fs::exists(path);
    }
    catch (...) {
        return false;
    }
}

std::string FileUtils::GetFileExtension(const std::string& path) {
    fs::path filePath(path);
    return filePath.extension().string();
}

std::string FileUtils::GetDirectory(const std::string& path) {
    fs::path filePath(path);
    return filePath.parent_path().string();
}

size_t FileUtils::GetFileSize(const std::string& path) {
    try {
        return fs::file_size(path);
    }
    catch (...) {
        return 0;
    }
}

std::vector<std::string> FileUtils::ListFiles(const std::string& directory,
                                             const std::string& extension) {
    std::vector<std::string> files;
    try {
        for (const auto& entry : fs::directory_iterator(directory)) {
            if (entry.is_regular_file()) {
                if (extension.empty() || 
                    entry.path().extension().string() == extension) {
                    files.push_back(entry.path().string());
                }
            }
        }
    }
    catch (const std::exception& e) {
        LOG_WARNING("Exception listing files: " + std::string(e.what()), 
                   "FileUtils::ListFiles");
    }
    return files;
}
```

