#include "EACExtractor.h"
#include "FileUtils.h"
#include "CryptoUtils.h"
#include "PatternScanner.h"
#include "MemoryDumper.h"
#include "Log.h"
#include <algorithm>
#include <initializer_list>

// Try multiple signatures in order, return first match
static uintptr_t FindFirstMatchMulti(uintptr_t base, std::initializer_list<const char*> patterns)
{
	for (auto* sig : patterns)
	{
		if (auto result = PatternScanner::FindFirstMatch(base, sig))
			return result;
	}
	return 0;
}

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
		if (!FileUtils::EnsureDirectoryExists(dumpFolder))
		{
			Log::Error("Failed to ensure output folder exists");
			return false;
		}
		return true;
	}

	dumpFolder = FileUtils::CreateDumpFolder();
	if (dumpFolder.empty())
	{
		Log::Error("Failed to create dump folder");
		return false;
	}
	return true;
}

bool EACExtractor::SaveOriginalFile()
{
	try
	{
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
				Log::Info("Input file is already in the output folder. Skipping copy of original.");
				return true;
			}
		}

		auto originalData = FileUtils::LoadBinaryFile(InputFilePath);
		if (originalData.empty())
		{
			Log::Error("Failed to read input file for original copy");
			return false;
		}

		std::string outputPath = dumpFolder + "\\original_eac.bin";
		return FileUtils::SaveBinaryFile(outputPath, originalData);
	}
	catch (const std::exception& e)
	{
		Log::Error("Failed to copy original file: %s", e.what());
		return false;
	}
}

bool EACExtractor::DecryptAndSaveLauncher()
{
	Log::Info("Loading and decrypting launcher...");

	auto encryptedFile = FileUtils::LoadBinaryFile(InputFilePath);
	if (encryptedFile.empty())
	{
		Log::Error("Failed to load input file");
		return false;
	}

	auto decryptedBuffer = CryptoUtils::DecryptPayload(encryptedFile);

	std::string launcherPath = dumpFolder + "\\EAC_Launcher_decrypted.dll";
	if (!FileUtils::SaveBinaryFile(launcherPath, decryptedBuffer))
	{
		Log::Error("Failed to save decrypted launcher");
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
		Log::Error("Failed to load launcher module");
		return false;
	}

	Log::Success("Launcher module loaded at: 0x%llX", (unsigned long long)(uintptr_t)LauncherModule);
	return true;
}

bool EACExtractor::DumpLoadedModule()
{
	if (!LauncherModule)
	{
		Log::Error("Launcher module not loaded");
		return false;
	}

	std::string dumpPath = dumpFolder + "\\EAC_Launcher_memory_dump.dll";
	if (!MemoryDumper::DumpModuleFromMemory(LauncherModule, dumpPath))
	{
		Log::Error("Failed to dump launcher from memory");
		return false;
	}

	return true;
}

ExtractedPayloads EACExtractor::ExtractPayloads()
{
	ExtractedPayloads payloads;

	if (!LauncherModule)
	{
		Log::Error("Launcher module not loaded");
		return payloads;
	}

	Log::Info("Extracting embedded payloads...");

	auto userModuleEncrypted = PatternScanner::FindFirstMatch(
		(uintptr_t)LauncherModule, "48 8D 05 ? ? ? ? 89 54 24");

	if (userModuleEncrypted)
	{
#ifdef _WIN64
		userModuleEncrypted = PatternScanner::ResolveRelative(userModuleEncrypted, 3, 7);

		uint32_t userModeModuleSize = 0;

		auto patternResult = PatternScanner::FindFirstMatch(
			(uintptr_t)LauncherModule, "8B 15 ? ? ? ? ? 89 ? 24");
		if (patternResult)
		{
			userModeModuleSize = *(uint32_t*)PatternScanner::ResolveRelative(patternResult, 2, 6);
		}
#else
		auto scanResult = PatternScanner::FindFirstMatch((uintptr_t)LauncherModule,
		                                                 "8B 93 ? 00 00 00 8B 0D");
		uint32_t userModeModuleSize = 0;
		if (scanResult) {
			userModeModuleSize = *(uint32_t*)*(uint32_t*)(scanResult + 0x8);
		}
#endif

		if (userModeModuleSize > 0 && userModeModuleSize < 100 * 1024 * 1024)
		{
			Log::Success("Found user mode module: %u bytes", userModeModuleSize);

			std::vector<uint8_t> encryptedUserMode(
				(uint8_t*)userModuleEncrypted,
				(uint8_t*)userModuleEncrypted + userModeModuleSize);

			payloads.UserModeModule = CryptoUtils::UnpackModule(encryptedUserMode);
			payloads.HasUserMode    = !payloads.UserModeModule.empty();
		}
	}

#ifdef _WIN64
	auto driverPattern = FindFirstMatchMulti((uintptr_t)LauncherModule, {
		"48 8D 15 ? ? ? ? 48 8B CE E8 ? ? ? ? 49 83 FE",
		"48 8D 15 ? ? ? ? 48 8B CB E8 ? ? ? ? 41 BE"
	});

	if (driverPattern)
	{
		auto driverDataPtr = PatternScanner::ResolveRelative(driverPattern, 3, 7);

		auto driverSizePattern = FindFirstMatchMulti((uintptr_t)LauncherModule, {
			"44 8B 35 ? ? ? ? 41 8B CE",
			"44 8B 35 ? ? ? ? 33 D2",
			"44 8B 3D ? ? ? ? 49 3B 40"
		});

		if (driverSizePattern)
		{
			auto driverSize = *(uint32_t*)PatternScanner::ResolveRelative(
				driverSizePattern, 3, 7);

			if (driverSize > 0 && driverSize < 100 * 1024 * 1024)
			{
				Log::Success("Found driver module: %u bytes", driverSize);

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
		(uintptr_t)LauncherModule, "FF 35 ? ? ? ? 99 68");

	if (driverSizePattern) {
		auto driverSize = *(uint32_t*)(*(uint32_t*)(driverSizePattern + 2));

		if (driverSize > 0 && driverSize < 10 * 1024 * 1024) {
			Log::Success("Found driver module: %u bytes", driverSize);
			payloads.HasDriver = true;
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
			Log::Error("Failed to save user mode module");
			success = false;
		}
	}

	if (payloads.HasDriver)
	{
		std::string driverPath = dumpFolder + "\\EAC_Driver.sys";
		if (!FileUtils::SaveBinaryFile(driverPath, payloads.DriverModule))
		{
			Log::Error("Failed to save driver module");
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
	Log::Banner();
	Log::Info("Input file: %s", InputFilePath.c_str());

	if (!CreateOutputFolder())  return false;
	if (!SaveOriginalFile())    return false;
	if (!DecryptAndSaveLauncher()) return false;
	if (!LoadLauncherModule())  return false;
	if (!DumpLoadedModule())    return false;

	auto payloads = ExtractPayloads();

	if (!SaveExtractedPayloads(payloads))
		return false;

	// --- Results summary ---
	printf("\n");
	Log::Info("Results:");
	Log::Info("  Output folder: %s", dumpFolder.c_str());

	if (payloads.HasUserMode)
		Log::Success("  UserMode:  %zu bytes", payloads.UserModeModule.size());
	else
		Log::Warning("  UserMode:  not found");

	if (payloads.HasDriver)
		Log::Success("  Driver:    %zu bytes", payloads.DriverModule.size());
	else
		Log::Warning("  Driver:    not found");

	Log::Success("Extraction completed successfully!");
	return true;
}
