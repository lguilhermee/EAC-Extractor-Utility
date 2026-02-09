#include "FileUtils.h"
#include "Log.h"
#include <fstream>
#include <filesystem>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <Windows.h>

namespace FileUtils
{
	std::vector<uint8_t> LoadBinaryFile(const std::string& filePath)
	{
		std::ifstream file(filePath, std::ios::binary);
		if (!file)
		{
			Log::Error("Failed to open the file: %s", filePath.c_str());
			return {};
		}

		file.seekg(0, std::ios::end);
		std::streampos fileSize = file.tellg();
		file.seekg(0, std::ios::beg);

		std::vector<uint8_t> buffer(fileSize);

		if (!file.read(reinterpret_cast<char*>(buffer.data()), fileSize))
		{
			Log::Error("Failed to read the file: %s", filePath.c_str());
			return {};
		}

		return buffer;
	}

	bool SaveBinaryFile(const std::string& filePath, const std::vector<uint8_t>& data)
	{
		std::ofstream file(filePath, std::ios::binary);
		if (!file)
		{
			Log::Error("Failed to create the file: %s", filePath.c_str());
			return false;
		}

		file.write(reinterpret_cast<const char*>(data.data()), data.size());
		if (!file)
		{
			Log::Error("Failed to write the data to the file: %s", filePath.c_str());
			return false;
		}

		Log::Success("Saved file: %s (%zu bytes)", filePath.c_str(), data.size());
		return true;
	}

	std::string SanitizeFileName(const std::string& name)
	{
		std::string sanitized = name;
		const std::string invalid = "<>:\"|?*";
		for (char& c : sanitized) {
			if (invalid.find(c) != std::string::npos || c < 32) {
				c = '_';
			}
		}
		while (!sanitized.empty() && sanitized.back() == ' ') {
			sanitized.pop_back();
		}
		while (!sanitized.empty() && sanitized.front() == ' ') {
			sanitized.erase(0, 1);
		}
		return sanitized;
	}

	std::string GetTimestampedFolderName()
	{
		return GetTimestampedFolderName("");
	}

	std::string GetTimestampedFolderName(const std::string& gameName)
	{
		auto now    = std::chrono::system_clock::now();
		auto time_t = std::chrono::system_clock::to_time_t(now);

		tm timeinfo;
		localtime_s(&timeinfo, &time_t);

		std::stringstream ss;
		if (!gameName.empty()) {
			ss << SanitizeFileName(gameName) << "_";
		}
		ss << "Dump_"
			<< std::put_time(&timeinfo, "%Y%m%d_%H%M%S");

		return ss.str();
	}

	bool EnsureDirectoryExists(const std::string& path)
	{
		try
		{
			if (!std::filesystem::exists(path))
			{
				return std::filesystem::create_directories(path);
			}
			return true;
		}
		catch (const std::exception& e)
		{
			Log::Error("Failed to create directory: %s - %s", path.c_str(), e.what());
			return false;
		}
	}

	std::string CreateDumpFolder()
	{
		return CreateDumpFolder("");
	}

	std::string CreateDumpFolder(const std::string& gameName)
	{
		const std::string baseDir = "C:\\EAC_Dumps";

		if (!EnsureDirectoryExists(baseDir))
		{
			Log::Error("Failed to create base directory: %s", baseDir.c_str());
			return "";
		}

		std::string timestampedFolder = baseDir + "\\" + GetTimestampedFolderName(gameName);

		if (!EnsureDirectoryExists(timestampedFolder))
		{
			Log::Error("Failed to create dump folder: %s", timestampedFolder.c_str());
			return "";
		}

		Log::Success("Created dump folder: %s", timestampedFolder.c_str());
		return timestampedFolder;
	}
}
