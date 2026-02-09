#pragma once
#include <string>
#include <vector>

namespace FileUtils
{
	std::vector<uint8_t> LoadBinaryFile(const std::string& filePath);
	bool                 SaveBinaryFile(const std::string& filePath, const std::vector<uint8_t>& data);
	std::string          CreateDumpFolder();
	std::string          CreateDumpFolder(const std::string& gameName);
	std::string          GetTimestampedFolderName();
	std::string          GetTimestampedFolderName(const std::string& gameName);
	bool                 EnsureDirectoryExists(const std::string& path);
	std::string          SanitizeFileName(const std::string& name);
}
