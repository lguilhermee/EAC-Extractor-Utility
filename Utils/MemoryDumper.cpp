#include "MemoryDumper.h"
#include "FileUtils.h"
#include "Log.h"
#include <Psapi.h>
#include <TlHelp32.h>
#include <sstream>
#include <iomanip>
#include <cstring>

#pragma comment(lib, "psapi.lib")

namespace MemoryDumper
{
	static bool FixHeaderRawToVirtual(char* pLocalImage)
	{
		auto pNtHeaders = reinterpret_cast<PIMAGE_NT_HEADERS>(
			reinterpret_cast<uint64_t>(pLocalImage) +
			reinterpret_cast<PIMAGE_DOS_HEADER>(pLocalImage)->e_lfanew);

		auto pSectionHeader = IMAGE_FIRST_SECTION(pNtHeaders);
		for (int i = 0; i < pNtHeaders->FileHeader.NumberOfSections; ++i, ++pSectionHeader)
		{
			pSectionHeader->PointerToRawData = pSectionHeader->VirtualAddress;
			pSectionHeader->SizeOfRawData = pSectionHeader->Misc.VirtualSize;
		}

		return true;
	}

	static std::string AppendBaseAddressToPath(const std::string& originalPath, uintptr_t baseAddress)
	{
		std::stringstream ss;
		ss << std::hex << std::uppercase << baseAddress;
		std::string baseAddrStr = ss.str();

		size_t lastDot = originalPath.find_last_of('.');
		if (lastDot != std::string::npos)
		{
			return originalPath.substr(0, lastDot) + "_0x" + baseAddrStr + originalPath.substr(lastDot);
		}
		return originalPath + "_0x" + baseAddrStr;
	}

	std::vector<uint8_t> GetModuleBytes(HMODULE hModule)
	{
		if (!hModule)
		{
			Log::Error("Invalid module handle");
			return {};
		}

		MODULEINFO modInfo = {0};
		if (!GetModuleInformation(GetCurrentProcess(), hModule, &modInfo, sizeof(modInfo)))
		{
			Log::Error("Failed to get module information");
			return {};
		}

		std::vector<uint8_t> moduleBytes(modInfo.SizeOfImage);
		SYSTEM_INFO sysInfo;
		GetSystemInfo(&sysInfo);
		DWORD pageSize = sysInfo.dwPageSize;

		SIZE_T totalBytesRead = 0;
		uint8_t* baseAddress = static_cast<uint8_t*>(modInfo.lpBaseOfDll);
		uint8_t* currentAddress = baseAddress;
		uint8_t* endAddress = baseAddress + modInfo.SizeOfImage;

		while (currentAddress < endAddress)
		{
			SIZE_T bytesToRead = min(pageSize, endAddress - currentAddress);
			SIZE_T bytesRead = 0;

			MEMORY_BASIC_INFORMATION mbi;
			if (VirtualQuery(currentAddress, &mbi, sizeof(mbi)))
			{
				if (mbi.State == MEM_COMMIT &&
				    (mbi.Protect & (PAGE_READONLY | PAGE_READWRITE | PAGE_WRITECOPY |
				                    PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY)))
				{
					if (ReadProcessMemory(GetCurrentProcess(), currentAddress,
					                      moduleBytes.data() + (currentAddress - baseAddress),
					                      bytesToRead, &bytesRead))
					{
						totalBytesRead += bytesRead;
					}
					else
					{
						memset(moduleBytes.data() + (currentAddress - baseAddress), 0, bytesToRead);
						Log::Warning("Failed to read page at 0x%llX", (unsigned long long)(uintptr_t)currentAddress);
					}
				}
				else
				{
					memset(moduleBytes.data() + (currentAddress - baseAddress), 0, bytesToRead);
				}
			}

			currentAddress += pageSize;
		}

		Log::Success("Dumped module from memory: %u bytes (%zu bytes read)",
			modInfo.SizeOfImage, totalBytesRead);
		Log::Info("Module base address: 0x%llX",
			(unsigned long long)(uintptr_t)modInfo.lpBaseOfDll);
		return moduleBytes;
	}

	bool DumpModuleFromMemory(HMODULE hModule, const std::string& outputPath)
	{
		if (!hModule)
		{
			Log::Error("Invalid module handle");
			return false;
		}

		auto moduleBytes = GetModuleBytes(hModule);
		if (moduleBytes.empty())
		{
			return false;
		}

		if (!FixHeaderRawToVirtual(reinterpret_cast<char*>(moduleBytes.data())))
		{
			Log::Warning("Failed to fix PE header");
		}

		MODULEINFO modInfo = {0};
		if (GetModuleInformation(GetCurrentProcess(), hModule, &modInfo, sizeof(modInfo)))
		{
			std::string modifiedPath = AppendBaseAddressToPath(
				outputPath, reinterpret_cast<uintptr_t>(modInfo.lpBaseOfDll));
			return FileUtils::SaveBinaryFile(modifiedPath, moduleBytes);
		}

		return FileUtils::SaveBinaryFile(outputPath, moduleBytes);
	}

	bool DumpProcessModule(DWORD processId, const std::string& moduleName, const std::string& outputPath)
	{
		HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, processId);
		if (!hProcess)
		{
			Log::Error("Failed to open process: %u", processId);
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
							SYSTEM_INFO sysInfo;
							GetSystemInfo(&sysInfo);
							DWORD pageSize = sysInfo.dwPageSize;

							SIZE_T totalBytesRead = 0;
							uint8_t* baseAddress = static_cast<uint8_t*>(modInfo.lpBaseOfDll);
							uint8_t* currentAddress = baseAddress;
							uint8_t* endAddress = baseAddress + modInfo.SizeOfImage;

							while (currentAddress < endAddress)
							{
								SIZE_T bytesToRead = min(pageSize, endAddress - currentAddress);
								SIZE_T bytesRead = 0;

								if (!ReadProcessMemory(hProcess, currentAddress,
								                       moduleBytes.data() + (currentAddress - baseAddress),
								                       bytesToRead, &bytesRead))
								{
									memset(moduleBytes.data() + (currentAddress - baseAddress), 0, bytesToRead);
								}
								else
								{
									totalBytesRead += bytesRead;
								}

								currentAddress += pageSize;
							}

							if (totalBytesRead > 0)
							{
								Log::Success("Dumped module from process: %u bytes (%zu bytes read)",
									modInfo.SizeOfImage, totalBytesRead);
								Log::Info("Module base address: 0x%llX",
									(unsigned long long)(uintptr_t)modInfo.lpBaseOfDll);

								if (!FixHeaderRawToVirtual(reinterpret_cast<char*>(moduleBytes.data())))
								{
									Log::Warning("Failed to fix PE header");
								}

								std::string modifiedPath = AppendBaseAddressToPath(
									outputPath, reinterpret_cast<uintptr_t>(modInfo.lpBaseOfDll));
								CloseHandle(hProcess);
								return FileUtils::SaveBinaryFile(modifiedPath, moduleBytes);
							}
						}
					}
				}
			}
		}

		CloseHandle(hProcess);
		Log::Error("Module not found: %s", moduleName.c_str());
		return false;
	}
}
