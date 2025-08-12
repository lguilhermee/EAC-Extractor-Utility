#pragma once
#include <Windows.h>
#include <vector>
#include <string>
#include <cstdint>

namespace MemoryDumper
{
	bool DumpModuleFromMemory(HMODULE hModule, const std::string& outputPath);
	std::vector<uint8_t> GetModuleBytes(HMODULE hModule);
	bool DumpProcessModule(DWORD processId, const std::string& moduleName, const std::string& outputPath);
}
