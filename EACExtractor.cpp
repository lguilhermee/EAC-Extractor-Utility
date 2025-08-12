#include "EACExtractor.h"
#include "FileUtils.h"
#include "CryptoUtils.h"
#include "PatternScanner.h"
#include "MemoryDumper.h"
#include <iostream>
#include <filesystem>

EACExtractor::EACExtractor(const std::string& inputFile)
	: inputFilePath(inputFile)
{
}

EACExtractor::~EACExtractor()
{
	Cleanup();
}

bool EACExtractor::CreateOutputFolder()
{
	dumpFolder = FileUtils::CreateDumpFolder();
	if (dumpFolder.empty())
	{
		std::cerr << "[!] Failed to create dump folder" << std::endl;
		return false;
	}
	return true;
}

bool EACExtractor::SaveOriginalFile()
{
	try
	{
		std::string outputPath = dumpFolder + "\\original_eac.bin";
		std::filesystem::copy_file(inputFilePath, outputPath,
		                           std::filesystem::copy_options::overwrite_existing);
		std::cout << "[+] Saved original file to: " << outputPath << std::endl;
		return true;
	}
	catch (const std::exception& e)
	{
		std::cerr << "[!] Failed to copy original file: " << e.what() << std::endl;
		return false;
	}
}

bool EACExtractor::DecryptAndSaveLauncher()
{
	std::cout << "\n[*] Loading and decrypting launcher..." << std::endl;

	auto encryptedFile = FileUtils::LoadBinaryFile(inputFilePath);
	if (encryptedFile.empty())
	{
		std::cerr << "[!] Failed to load input file" << std::endl;
		return false;
	}

	auto decryptedBuffer = CryptoUtils::DecryptBuffer(encryptedFile);

	std::string launcherPath = dumpFolder + "\\EAC_Launcher_decrypted.dll";
	if (!FileUtils::SaveBinaryFile(launcherPath, decryptedBuffer))
	{
		std::cerr << "[!] Failed to save decrypted launcher" << std::endl;
		return false;
	}

	return true;
}

bool EACExtractor::LoadLauncherModule()
{
	std::string launcherPath = dumpFolder + "\\EAC_Launcher_decrypted.dll";
	launcherModule           = LoadLibraryA(launcherPath.c_str());

	if (!launcherModule)
	{
		std::cerr << "[!] Failed to load launcher module" << std::endl;
		return false;
	}

	std::cout << "[+] Launcher module loaded at: 0x" << std::hex << launcherModule << std::dec << std::endl;
	return true;
}

bool EACExtractor::DumpLoadedModule()
{
	if (!launcherModule)
	{
		std::cerr << "[!] Launcher module not loaded" << std::endl;
		return false;
	}

	std::string dumpPath = dumpFolder + "\\EAC_Launcher_memory_dump.dll";
	if (!MemoryDumper::DumpModuleFromMemory(launcherModule, dumpPath))
	{
		std::cerr << "[!] Failed to dump launcher from memory" << std::endl;
		return false;
	}

	std::cout << "[+] Dumped launcher from memory to: " << dumpPath << std::endl;
	return true;
}


#define COMPRESSION_FORMAT_NONE          (0x0000)
#define COMPRESSION_FORMAT_DEFAULT       (0x0001)
#define COMPRESSION_FORMAT_LZNT1         (0x0002)
#define COMPRESSION_FORMAT_XPRESS        (0x0003)
#define COMPRESSION_FORMAT_XPRESS_HUFF   (0x0004)
#define COMPRESSION_FORMAT_XP10          (0x0005)
#define COMPRESSION_FORMAT_LZ4           (0x0006)
#define COMPRESSION_FORMAT_DEFLATE       (0x0007)
#define COMPRESSION_FORMAT_ZLIB          (0x0008)
#define COMPRESSION_FORMAT_MAX           (0x0008)

void test(uintptr_t moduleBase)
{
	uint32_t xor_key_1 = 0xECB4FC57;
	uint64_t decrypt_offset_24 = 0;
	char encrypted_function_name_1[24];

	memset(encrypted_function_name_1, 0, sizeof(encrypted_function_name_1));

	uintptr_t loaded_module_handle_1 = moduleBase + 0x60e20;
	do
	{
		xor_key_1 = 0xFF3C613D - 0x43FD43FD * xor_key_1;
		*(uint32_t*)&encrypted_function_name_1[decrypt_offset_24] = *(uint32_t*)((char*)loaded_module_handle_1
			+ decrypt_offset_24)
			^ xor_key_1;
		decrypt_offset_24 += 4LL;
	} while (decrypt_offset_24 < 0x18);

	printf("%ls\n", encrypted_function_name_1);
	auto que = 0;
}

void test2(uintptr_t modulebase)
{
	uintptr_t functionEnc = modulebase + 0x60dd0;
	char encrypted_function_name_2[18];

	memset(encrypted_function_name_2, 0, sizeof(encrypted_function_name_2));
	*(uintptr_t*)encrypted_function_name_2 = 0;
	uint32_t xor_key_4 = 0x53A97C6A;
	*(uint16_t*)&encrypted_function_name_2[8] = 0;
	encrypted_function_name_2[10] = 0;

	BYTE xor_key_6 = 0;
	uint64_t byte_index_8 = 8;
	uint64_t decrypt_offset_8 = 0;
	do
	{
		*(uint32_t*)&encrypted_function_name_2[decrypt_offset_8] = *(uint32_t*)((char*)functionEnc
			+ decrypt_offset_8)
			^ xor_key_4;
		decrypt_offset_8 += 4LL;
		xor_key_4 = (4
			* (((((xor_key_4 ^ (xor_key_4 << 13)) >> 7) ^ xor_key_4 ^ (xor_key_4 << 13)) << 17)
				^ ((xor_key_4 ^ (xor_key_4 << 13)) >> 7)
				^ xor_key_4
				^ (xor_key_4 << 13)))
			| ((((((xor_key_4 ^ (xor_key_4 << 13)) >> 7) ^ xor_key_4 ^ (xor_key_4 << 13)) << 17)
				^ ((xor_key_4 ^ (xor_key_4 << 13)) >> 7)
				^ xor_key_4
				^ (xor_key_4 << 13)) >> 30);
	} while (decrypt_offset_8 < 8);


	do
	{
		xor_key_6 = xor_key_4;
		xor_key_4 >>= 8;
		encrypted_function_name_2[byte_index_8] = *((BYTE*)functionEnc + byte_index_8) ^ xor_key_6;
		++byte_index_8;
	} while (byte_index_8 < 0xB);

	printf("%ls\n", (wchar_t*)encrypted_function_name_2);

	auto que = 0;
}


ExtractedPayloads EACExtractor::ExtractPayloads()
{
	ExtractedPayloads payloads;

	if (!launcherModule)
	{
		std::cerr << "[!] Launcher module not loaded" << std::endl;
		return payloads;
	}

	std::cout << "\n[*] Extracting embedded payloads..." << std::endl;

	auto userModuleEncrypted = PatternScanner::FindFirstMatch(
		(uintptr_t)launcherModule, "A7 ED 96 0C 0F");

	if (userModuleEncrypted)
	{
#ifdef _WIN64

		uint32_t userModeModuleSize = 0;

		// First Sig
		auto patternResult = PatternScanner::FindFirstMatch((uintptr_t)launcherModule,
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
			std::cout << "[+] Found user mode module: " << userModeModuleSize << " bytes" << std::endl;

			payloads.userModeModule = std::vector<uint8_t>(
				(uint8_t*)userModuleEncrypted,
				(uint8_t*)userModuleEncrypted + userModeModuleSize);

			payloads.userModeModule = CryptoUtils::DecryptBuffer(payloads.userModeModule);
			payloads.hasUserMode    = true;
		}
	}

#ifdef _WIN64
	auto driverPattern = PatternScanner::FindFirstMatch(
		(uintptr_t)launcherModule, "48 8D 15 ? ? ? ? 48 8B CE E8 ? ? ? ? 49 83 FE");

	using getSize_t  = uint64_t(__fastcall*)(uintptr_t,uintptr_t compressedData, uintptr_t bufferOut);
	auto getSizeFunc = (getSize_t)((uintptr_t)launcherModule + 0x318b0);

	test((uintptr_t)launcherModule);
	test2((uintptr_t)launcherModule);


	if (driverPattern)
	{
		auto driverDataPtr     = PatternScanner::ResolveRelative(driverPattern, 3, 7);
		auto driverSizePattern = PatternScanner::FindFirstMatch(
			(uintptr_t)launcherModule, "44 8B 35 ? ? ? ? 41 8B CE");

		if (driverSizePattern)
		{
			auto driverSize = *(uint32_t*)PatternScanner::ResolveRelative(
				driverSizePattern, 3, 7);

			if (driverSize > 0 && driverSize < 100 * 1024 * 1024)
			{
				std::cout << "[+] Found driver module: " << driverSize << " bytes" << std::endl;

				payloads.driverModule = std::vector(
					(uint8_t*)driverDataPtr,
					(uint8_t*)driverDataPtr + driverSize);

				// Decrypt Buffer
				payloads.driverModule = CryptoUtils::DecryptBuffer(payloads.driverModule);

				// Decompress Buffer
				BYTE* decompressedData = nullptr;
				size_t decompressedSize = 0;

				if (CryptoUtils::DecompressBuffer(
					payloads.driverModule.data(),
					payloads.driverModule.size(),
					&decompressedData,
					&decompressedSize))
				{
					payloads.driverModule = std::vector(
						decompressedData,
						decompressedData + decompressedSize);

					payloads.hasDriver = true;
				}




				
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

	if (payloads.hasUserMode)
	{
		std::string userModePath = dumpFolder + "\\EAC_UserMode.dll";
		if (!FileUtils::SaveBinaryFile(userModePath, payloads.userModeModule))
		{
			std::cerr << "[!] Failed to save user mode module" << std::endl;
			success = false;
		}
	}

	if (payloads.hasDriver)
	{
		std::string driverPath = dumpFolder + "\\EAC_Driver.sys";
		if (!FileUtils::SaveBinaryFile(driverPath, payloads.driverModule))
		{
			std::cerr << "[!] Failed to save driver module" << std::endl;
			success = false;
		}
	}

	return success;
}

void EACExtractor::Cleanup()
{
	if (launcherModule)
	{
		FreeLibrary(launcherModule);
		launcherModule = nullptr;
	}
}

bool EACExtractor::Process()
{
	std::cout << "========================================" << std::endl;
	std::cout << "       EAC Payload Extractor v2.0      " << std::endl;
	std::cout << "========================================" << std::endl;
	std::cout << "[*] Input file: " << inputFilePath << std::endl;

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

	std::cout << "\n========================================" << std::endl;
	std::cout << "[+] Extraction completed successfully!" << std::endl;
	std::cout << "[+] Output folder: " << dumpFolder << std::endl;
	std::cout << "========================================" << std::endl;

	return true;
}
