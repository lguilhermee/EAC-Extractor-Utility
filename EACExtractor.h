#pragma once
#include <Windows.h>
#include <string>
#include <vector>
#include <cstdint>

struct ExtractedPayloads
{
	std::vector<uint8_t> UserModeModule;
	std::vector<uint8_t> DriverModule;
	bool                 HasUserMode = false;
	bool                 HasDriver   = false;
};

class EACExtractor
{
	std::string dumpFolder;
	std::string InputFilePath;
	HMODULE     LauncherModule = nullptr;

public:
	EACExtractor(std::string inputFile);
	EACExtractor(std::string inputFile, std::string outputFolder);
	~EACExtractor();

	bool Process();

private:
	bool              CreateOutputFolder();
	bool              SaveOriginalFile();
	bool              DecryptAndSaveLauncher();
	bool              LoadLauncherModule();
	bool              DumpLoadedModule();
	ExtractedPayloads ExtractPayloads();
	bool              SaveExtractedPayloads(const ExtractedPayloads& payloads);
	void              Cleanup();
};
