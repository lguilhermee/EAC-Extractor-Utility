#include "MemoryDumper.h"
#include "FileUtils.h"
#include <iostream>
#include <Psapi.h>
#include <TlHelp32.h>

#pragma comment(lib, "psapi.lib")

namespace MemoryDumper
{
	std::vector<uint8_t> GetModuleBytes(HMODULE hModule)
	{
		if (!hModule)
		{
			std::cerr << "[!] Invalid module handle" << '\n';
			return {};
		}

		MODULEINFO modInfo = {nullptr};
		if (!GetModuleInformation(GetCurrentProcess(), hModule, &modInfo, sizeof(modInfo)))
		{
			std::cerr << "[!] Failed to get module information" << '\n';
			return {};
		}

		std::vector<uint8_t> moduleBytes(modInfo.SizeOfImage);

		SIZE_T bytesRead = 0;
		if (!ReadProcessMemory(GetCurrentProcess(), modInfo.lpBaseOfDll,
		                       moduleBytes.data(), modInfo.SizeOfImage, &bytesRead))
		{
			std::cerr << "[!] Failed to read module memory" << '\n';
			return {};
		}

		std::cout << "[+] Dumped module from memory: " << modInfo.SizeOfImage << " bytes" << '\n';
		return moduleBytes;
	}

	bool DumpModuleFromMemory(HMODULE hModule, const std::string& outputPath)
	{
		if (!hModule)
		{
			std::cerr << "[!] Invalid module handle" << '\n';
			return false;
		}

		auto moduleBytes = GetModuleBytes(hModule);
		if (moduleBytes.empty())
		{
			return false;
		}

		return FileUtils::SaveBinaryFile(outputPath, moduleBytes);
	}

	bool DumpProcessModule(DWORD processId, const std::string& moduleName, const std::string& outputPath)
	{
		HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, processId);
		if (!hProcess)
		{
			std::cerr << "[!] Failed to open process: " << processId << '\n';
			return false;
		}

		HMODULE hMods[1024];
		DWORD   cbNeeded;

		if (EnumProcessModules(hProcess, hMods, sizeof(hMods), &cbNeeded))
		{
			for (unsigned int i = 0; i < (cbNeeded / sizeof(HMODULE)); i++)
			{
				char szModName[MAX_PATH];

				if (GetModuleFileNameExA(hProcess, hMods[i], szModName, sizeof(szModName) / sizeof(char)))
				{
					std::string modNameStr(szModName);
					if (modNameStr.find(moduleName) != std::string::npos)
					{
						MODULEINFO modInfo;
						if (GetModuleInformation(hProcess, hMods[i], &modInfo, sizeof(modInfo)))
						{
							std::vector<uint8_t> moduleBytes(modInfo.SizeOfImage);
							SIZE_T               bytesRead;

							if (ReadProcessMemory(hProcess, modInfo.lpBaseOfDll,
							                      moduleBytes.data(), modInfo.SizeOfImage, &bytesRead))
							{
								CloseHandle(hProcess);
								return FileUtils::SaveBinaryFile(outputPath, moduleBytes);
							}
						}
					}
				}
			}
		}

		CloseHandle(hProcess);
		std::cerr << "[!] Module not found: " << moduleName << '\n';
		return false;
	}
}
