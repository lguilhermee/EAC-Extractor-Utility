#include "EACExtractor.h"
#include "FileUtils.h"
#include "CryptoUtils.h"
#include "PatternScanner.h"
#include "MemoryDumper.h"
#include <iostream>
#include <algorithm>

EACExtractor::EACExtractor(std::string inputFile)
	: InputFilePath(std::move(inputFile))
{
}

EACExtractor::EACExtractor(std::string inputFile, std::string outputFolder)
	: dumpFolder(std::move(outputFolder)), InputFilePath(std::move(inputFile))
{
}

EACExtractor::~EACExtractor()
{
	Cleanup();
}

bool EACExtractor::CreateOutputFolder()
{
	if (!dumpFolder.empty())
	{
		// Custom output folder already set (e.g., by caller). Ensure it exists.
		if (!FileUtils::EnsureDirectoryExists(dumpFolder))
		{
			std::cerr << "[!] Failed to ensure output folder exists" << '\n';
			return false;
		}
		return true;
	}

	dumpFolder = FileUtils::CreateDumpFolder();
	if (dumpFolder.empty())
	{
		std::cerr << "[!] Failed to create dump folder" << '\n';
		return false;
	}
	return true;
}

bool EACExtractor::SaveOriginalFile()
{
	try
	{
		// If the input file already resides in the dump folder, avoid duplicating it (simple prefix check, case-insensitive)
		auto normalize = [](std::string s)
		{
			std::ranges::replace(s, '/', '\\');
			std::string lower;
			lower.resize(s.size());
			std::ranges::transform(s, lower.begin(), [](unsigned char c)
			{
				return static_cast<char>(std::tolower(c));
			});
			return lower;
		};

		const std::string inputLower = normalize(InputFilePath);
		const std::string dumpLower  = normalize(dumpFolder);

		if (!dumpLower.empty())
		{
			if (inputLower.starts_with(dumpLower + "\\") || inputLower == dumpLower)
			{
				std::cout << "[i] Input file is already in the output folder. Skipping copy of original." << '\n';
				return true;
			}
		}

		// Copy the original into the dump folder for reference
		auto originalData = FileUtils::LoadBinaryFile(InputFilePath);
		if (originalData.empty())
		{
			std::cerr << "[!] Failed to read input file for original copy" << '\n';
			return false;
		}

		std::string outputPath = dumpFolder + "\\original_eac.bin";
		if (!FileUtils::SaveBinaryFile(outputPath, originalData))
		{
			return false;
		}
		return true;
	}
	catch (const std::exception& e)
	{
		std::cerr << "[!] Failed to copy original file: " << e.what() << '\n';
		return false;
	}
}

bool EACExtractor::DecryptAndSaveLauncher()
{
	std::cout << "\n[*] Loading and decrypting launcher..." << '\n';

	auto encryptedFile = FileUtils::LoadBinaryFile(InputFilePath);
	if (encryptedFile.empty())
	{
		std::cerr << "[!] Failed to load input file" << '\n';
		return false;
	}

	auto decryptedBuffer = CryptoUtils::DecryptPayload(encryptedFile);

	std::string launcherPath = dumpFolder + "\\EAC_Launcher_decrypted.dll";
	if (!FileUtils::SaveBinaryFile(launcherPath, decryptedBuffer))
	{
		std::cerr << "[!] Failed to save decrypted launcher" << '\n';
		return false;
	}

	return true;
}

bool EACExtractor::LoadLauncherModule()
{
	std::string launcherPath = dumpFolder + "\\EAC_Launcher_decrypted.dll";
	LauncherModule           = LoadLibraryA(launcherPath.c_str());

	if (!LauncherModule)
	{
		std::cerr << "[!] Failed to load launcher module" << '\n';
		return false;
	}

	std::cout << "[+] Launcher module loaded at: 0x" << std::hex << LauncherModule << std::dec << '\n';
	return true;
}

bool EACExtractor::DumpLoadedModule()
{
	if (!LauncherModule)
	{
		std::cerr << "[!] Launcher module not loaded" << '\n';
		return false;
	}

	std::string dumpPath = dumpFolder + "\\EAC_Launcher_memory_dump.dll";
	if (!MemoryDumper::DumpModuleFromMemory(LauncherModule, dumpPath))
	{
		std::cerr << "[!] Failed to dump launcher from memory" << '\n';
		return false;
	}

	std::cout << "[+] Dumped launcher from memory to: " << dumpPath << '\n';
	return true;
}


ExtractedPayloads EACExtractor::ExtractPayloads()
{
	ExtractedPayloads payloads;

	if (!LauncherModule)
	{
		std::cerr << "[!] Launcher module not loaded" << '\n';
		return payloads;
	}

	std::cout << "\n[*] Extracting embedded payloads..." << '\n';

	auto userModuleEncrypted = PatternScanner::FindFirstMatch(
		(uintptr_t)LauncherModule, "48 8D 05 ? ? ? ? 89 54 24");

	if (userModuleEncrypted)
	{
#ifdef _WIN64

		userModuleEncrypted = PatternScanner::ResolveRelative(userModuleEncrypted, 3, 7);

		uint32_t userModeModuleSize = 0;

		// First Sig
		auto patternResult = PatternScanner::FindFirstMatch((uintptr_t)LauncherModule,
		                                                    "8B 15 ? ? ? ? ? 89 ? 24");
		if (patternResult)
		{
			userModeModuleSize = *(uint32_t*)PatternScanner::ResolveRelative(patternResult, 2, 6);
		}


#else
        auto scanResult = PatternScanner::FindFirstMatch((uintptr_t)launcherModule, 
                                                        "8B 93 ? 00 00 00 8B 0D");
        uint32_t userModeModuleSize = 0;
        if (scanResult) {
            userModeModuleSize = *(uint32_t*)*(uint32_t*)(scanResult + 0x8);
        }
#endif

		if (userModeModuleSize > 0 && userModeModuleSize < 100 * 1024 * 1024)
		{
			std::cout << "[+] Found user mode module: " << userModeModuleSize << " bytes" << '\n';

			std::vector<uint8_t> encryptedUserMode(
				(uint8_t*)userModuleEncrypted,
				(uint8_t*)userModuleEncrypted + userModeModuleSize);

			payloads.UserModeModule = CryptoUtils::UnpackModule(encryptedUserMode);
			payloads.HasUserMode    = !payloads.UserModeModule.empty();
		}
	}

#ifdef _WIN64
	auto driverPattern = PatternScanner::FindFirstMatch(
		(uintptr_t)LauncherModule, "48 8D 15 ? ? ? ? 48 8B CE E8 ? ? ? ? 49 83 FE");

	if (!driverPattern)
		driverPattern = PatternScanner::FindFirstMatch(
			(uintptr_t)LauncherModule, "48 8D 15 ? ? ? ? 48 8B CB E8 ? ? ? ? 41 BE");


	if (driverPattern)
	{
		auto driverDataPtr     = PatternScanner::ResolveRelative(driverPattern, 3, 7);
		
		auto driverSizePattern = PatternScanner::FindFirstMatch(
			(uintptr_t)LauncherModule, "44 8B 35 ? ? ? ? 41 8B CE");

		if (!driverSizePattern)
		{
			driverSizePattern = PatternScanner::FindFirstMatch(
				(uintptr_t)LauncherModule, "44 8B 35 ? ? ? ? 33 D2");
			if (!driverSizePattern)
			{
				driverSizePattern = PatternScanner::FindFirstMatch(
					(uintptr_t)LauncherModule, "44 8B 3D ? ? ? ? 49 3B 40");
			}
		}


		if (driverSizePattern)
		{
			auto driverSize = *(uint32_t*)PatternScanner::ResolveRelative(
				driverSizePattern, 3, 7);

			if (driverSize > 0 && driverSize < 100 * 1024 * 1024)
			{
				std::cout << "[+] Found driver module: " << driverSize << " bytes" << '\n';

				std::vector<uint8_t> encryptedDriver(
					(uint8_t*)driverDataPtr,
					(uint8_t*)driverDataPtr + driverSize);

				payloads.DriverModule = CryptoUtils::UnpackModule(encryptedDriver);
				payloads.HasDriver    = !payloads.DriverModule.empty();
			}
		}
	}
#else
    auto driverSizePattern = PatternScanner::FindFirstMatch(
        (uintptr_t)launcherModule, "FF 35 ? ? ? ? 99 68");
    
    if (driverSizePattern) {
        auto driverSize = *(uint32_t*)(*(uint32_t*)(driverSizePattern + 2));
        
        if (driverSize > 0 && driverSize < 10 * 1024 * 1024) {
            std::cout << "[+] Found driver module: " << driverSize << " bytes" << std::endl;
            payloads.hasDriver = true;
        }
    }
#endif

	return payloads;
}

bool EACExtractor::SaveExtractedPayloads(const ExtractedPayloads& payloads)
{
	bool success = true;

	if (payloads.HasUserMode)
	{
		std::string userModePath = dumpFolder + "\\EAC_UserMode.dll";
		if (!FileUtils::SaveBinaryFile(userModePath, payloads.UserModeModule))
		{
			std::cerr << "[!] Failed to save user mode module" << '\n';
			success = false;
		}
	}

	if (payloads.HasDriver)
	{
		std::string driverPath = dumpFolder + "\\EAC_Driver.sys";
		if (!FileUtils::SaveBinaryFile(driverPath, payloads.DriverModule))
		{
			std::cerr << "[!] Failed to save driver module" << '\n';
			success = false;
		}
	}

	return success;
}

void EACExtractor::Cleanup()
{
	if (LauncherModule)
	{
		FreeLibrary(LauncherModule);
		LauncherModule = nullptr;
	}
}

bool EACExtractor::Process()
{
	std::cout << "========================================" << '\n';
	std::cout << "       EAC Payload Extractor v2.0      " << '\n';
	std::cout << "========================================" << '\n';
	std::cout << "[*] Input file: " << InputFilePath << '\n';

	if (!CreateOutputFolder())
	{
		return false;
	}

	if (!SaveOriginalFile())
	{
		return false;
	}

	if (!DecryptAndSaveLauncher())
	{
		return false;
	}

	if (!LoadLauncherModule())
	{
		return false;
	}

	if (!DumpLoadedModule())
	{
		return false;
	}

	auto payloads = ExtractPayloads();

	if (!SaveExtractedPayloads(payloads))
	{
		return false;
	}

	std::cout << "\n========================================" << '\n';
	std::cout << "[+] Extraction completed successfully!" << '\n';
	std::cout << "[+] Output folder: " << dumpFolder << '\n';
	std::cout << "========================================" << '\n';

	return true;
}
