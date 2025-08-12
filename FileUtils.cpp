#include "FileUtils.h"
#include <iostream>
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
			std::cerr << "Failed to open the file: " << filePath << std::endl;
			return {};
		}

		file.seekg(0, std::ios::end);
		std::streampos fileSize = file.tellg();
		file.seekg(0, std::ios::beg);

		std::vector<uint8_t> buffer(fileSize);

		if (!file.read(reinterpret_cast<char*>(buffer.data()), fileSize))
		{
			std::cerr << "Failed to read the file: " << filePath << std::endl;
			return {};
		}

		return buffer;
	}

	bool SaveBinaryFile(const std::string& filePath, const std::vector<uint8_t>& data)
	{
		std::ofstream file(filePath, std::ios::binary);
		if (!file)
		{
			std::cerr << "Failed to create the file: " << filePath << std::endl;
			return false;
		}

		file.write(reinterpret_cast<const char*>(data.data()), data.size());
		if (!file)
		{
			std::cerr << "Failed to write the data to the file: " << filePath << std::endl;
			return false;
		}

		std::cout << "[+] Saved file: " << filePath << " (" << data.size() << " bytes)" << std::endl;
		return true;
	}

	std::string SanitizeFileName(const std::string& name)
	{
		std::string sanitized = name;
		// Replace invalid filename characters
		const std::string invalid = "<>:\"|?*";
		for (char& c : sanitized) {
			if (invalid.find(c) != std::string::npos || c < 32) {
				c = '_';
			}
		}
		// Trim spaces
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
			std::cerr << "Failed to create directory: " << path << " - " << e.what() << std::endl;
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
			std::cerr << "Failed to create base directory: " << baseDir << std::endl;
			return "";
		}

		std::string timestampedFolder = baseDir + "\\" + GetTimestampedFolderName(gameName);

		if (!EnsureDirectoryExists(timestampedFolder))
		{
			std::cerr << "Failed to create dump folder: " << timestampedFolder << std::endl;
			return "";
		}

		std::cout << "[+] Created dump folder: " << timestampedFolder << std::endl;
		return timestampedFolder;
	}
}
