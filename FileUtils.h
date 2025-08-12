#pragma once
#include <string>
#include <vector>
#include <filesystem>

namespace FileUtils {
    std::vector<uint8_t> LoadBinaryFile(const std::string& filePath);
    bool SaveBinaryFile(const std::string& filePath, const std::vector<uint8_t>& data);
    std::string CreateDumpFolder();
    std::string GetTimestampedFolderName();
    bool EnsureDirectoryExists(const std::string& path);
}