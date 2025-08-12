#pragma once
#include <Windows.h>
#include <string>
#include <vector>
#include <cstdint>

struct ExtractedPayloads {
    std::vector<uint8_t> userModeModule;
    std::vector<uint8_t> driverModule;
    bool hasUserMode = false;
    bool hasDriver = false;
};

class EACExtractor {
private:
    std::string dumpFolder;
    std::string inputFilePath;
    HMODULE launcherModule = nullptr;
    
public:
    EACExtractor(const std::string& inputFile);
    ~EACExtractor();
    
    bool Process();
    
private:
    bool CreateOutputFolder();
    bool SaveOriginalFile();
    bool DecryptAndSaveLauncher();
    bool LoadLauncherModule();
    bool DumpLoadedModule();
    ExtractedPayloads ExtractPayloads();
    bool SaveExtractedPayloads(const ExtractedPayloads& payloads);
    void Cleanup();
};