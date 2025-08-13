#include "MemoryDumper.h"
#include "FileUtils.h"
#include <iostream>
#include <Psapi.h>
#include <TlHelp32.h>
#include <sstream>
#include <iomanip>

#pragma comment(lib, "psapi.lib")

namespace MemoryDumper
{
	bool FixHeaderRawToVirtual(char* pLocalImage)
	{
		auto pNtHeaders = reinterpret_cast<PIMAGE_NT_HEADERS>(reinterpret_cast<uint64_t>(pLocalImage) + reinterpret_cast<PIMAGE_DOS_HEADER>(pLocalImage)->e_lfanew);

		auto pSectionHeader = IMAGE_FIRST_SECTION(pNtHeaders);
		for (int i = 0; i < pNtHeaders->FileHeader.NumberOfSections; ++i, ++pSectionHeader)
		{
			pSectionHeader->PointerToRawData = pSectionHeader->VirtualAddress;
			pSectionHeader->SizeOfRawData = pSectionHeader->Misc.VirtualSize;
		}

		return true;
	}

	std::string AppendBaseAddressToPath(const std::string& originalPath, uintptr_t baseAddress)
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
			std::cerr << "[!] Invalid module handle" << '\n';
			return {};
		}

		MODULEINFO modInfo = {0};
		if (!GetModuleInformation(GetCurrentProcess(), hModule, &modInfo, sizeof(modInfo)))
		{
			std::cerr << "[!] Failed to get module information" << '\n';
			return {};
		}

		std::vector<uint8_t> moduleBytes(modInfo.SizeOfImage);
		SYSTEM_INFO sysInfo;
		GetSystemInfo(&sysInfo);
		DWORD pageSize = sysInfo.dwPageSize;

		// Dump the module page by page to handle potential access violations
		SIZE_T totalBytesRead = 0;
		uint8_t* baseAddress = static_cast<uint8_t*>(modInfo.lpBaseOfDll);
		uint8_t* currentAddress = baseAddress;
		uint8_t* endAddress = baseAddress + modInfo.SizeOfImage;

		while (currentAddress < endAddress)
		{
			SIZE_T bytesToRead = min(pageSize, endAddress - currentAddress);
			SIZE_T bytesRead = 0;

			// Query the memory protection to ensure it's readable
			MEMORY_BASIC_INFORMATION mbi;
			if (VirtualQuery(currentAddress, &mbi, sizeof(mbi)))
			{
				// Check if the page is readable
				if (mbi.State == MEM_COMMIT && 
				    (mbi.Protect & (PAGE_READONLY | PAGE_READWRITE | PAGE_WRITECOPY | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY)))
				{
					if (ReadProcessMemory(GetCurrentProcess(), currentAddress,
					                      moduleBytes.data() + (currentAddress - baseAddress),
					                      bytesToRead, &bytesRead))
					{
						totalBytesRead += bytesRead;
					}
					else
					{
						// Fill unreadable pages with zeros
						memset(moduleBytes.data() + (currentAddress - baseAddress), 0, bytesToRead);
						std::cerr << "[!] Warning: Failed to read page at 0x" << std::hex << reinterpret_cast<uintptr_t>(currentAddress) << std::dec << '\n';
					}
				}
				else
				{
					// Page is not readable, fill with zeros
					memset(moduleBytes.data() + (currentAddress - baseAddress), 0, bytesToRead);
				}
			}

			currentAddress += pageSize;
		}

		std::cout << "[+] Dumped module from memory: " << modInfo.SizeOfImage << " bytes (" << totalBytesRead << " bytes read)" << '\n';
		std::cout << "[+] Module base address: 0x" << std::hex << reinterpret_cast<uintptr_t>(modInfo.lpBaseOfDll) << std::dec << '\n';
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

		// Apply the header fix to convert raw addresses to virtual addresses
		if (!FixHeaderRawToVirtual(reinterpret_cast<char*>(moduleBytes.data())))
		{
			std::cerr << "[!] Warning: Failed to fix PE header" << '\n';
		}

		// Get module base address and append it to the filename
		MODULEINFO modInfo = {0};
		if (GetModuleInformation(GetCurrentProcess(), hModule, &modInfo, sizeof(modInfo)))
		{
			std::string modifiedPath = AppendBaseAddressToPath(outputPath, reinterpret_cast<uintptr_t>(modInfo.lpBaseOfDll));
			return FileUtils::SaveBinaryFile(modifiedPath, moduleBytes);
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
							SYSTEM_INFO sysInfo;
							GetSystemInfo(&sysInfo);
							DWORD pageSize = sysInfo.dwPageSize;

							// Dump the module page by page
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
									// Fill unreadable pages with zeros
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
								std::cout << "[+] Dumped module from process: " << modInfo.SizeOfImage << " bytes (" << totalBytesRead << " bytes read)" << '\n';
								std::cout << "[+] Module base address: 0x" << std::hex << reinterpret_cast<uintptr_t>(modInfo.lpBaseOfDll) << std::dec << '\n';

								// Apply the header fix
								if (!FixHeaderRawToVirtual(reinterpret_cast<char*>(moduleBytes.data())))
								{
									std::cerr << "[!] Warning: Failed to fix PE header" << '\n';
								}

								// Append base address to filename
								std::string modifiedPath = AppendBaseAddressToPath(outputPath, reinterpret_cast<uintptr_t>(modInfo.lpBaseOfDll));
								CloseHandle(hProcess);
								return FileUtils::SaveBinaryFile(modifiedPath, moduleBytes);
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
