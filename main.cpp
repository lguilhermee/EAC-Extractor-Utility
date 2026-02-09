#include <iostream>
#include <string>
#include <Windows.h>
#include "EACExtractor.h"
#include "ModuleDownloader.h"
#include "GameConfig.h"
#include "FileUtils.h"
#include "Log.h"

// ---- ANSI color helpers for interactive menus ----
namespace
{
	constexpr auto Reset  = "\x1b[0m";
	constexpr auto Bold   = "\x1b[1m";
	constexpr auto Yellow = "\x1b[33m";
	constexpr auto Cyan   = "\x1b[36m";
	constexpr auto Gray   = "\x1b[90m";
}

static void PrintMainMenu()
{
	printf("\n");
	Log::Info("What would you like to do?");
	printf("%s[1]%s Download and extract from Epic Games CDN\n", Yellow, Reset);
	printf("%s[2]%s Extract from local file\n", Yellow, Reset);
	printf("%s[3]%s List available games\n", Yellow, Reset);
	printf("%s[4]%s Show help\n", Yellow, Reset);
	printf("%s[Q]%s Quit\n", Yellow, Reset);
}

static void PrintUsage(const char* programName)
{
	Log::Banner();
	printf("%sUsage:%s %s [options]\n", Bold, Reset, programName);
	printf("       %s -h | --help\n", programName);
	printf("\n%sOptions:%s\n", Bold, Reset);
	printf("  %s-d, --download%s    Download and then extract from Epic Games CDN\n", Yellow, Reset);
	printf("  %s-e, --extract%s     Extract and decrypt local EAC binary (optional: path)\n", Yellow, Reset);
	printf("  %s-l, --list%s        List available games from configuration\n", Yellow, Reset);
	printf("\n%sOutput:%s\n", Bold, Reset);
	printf("  Creates timestamped folder in C:%s\\EAC_Dumps%s with extracted components\n", Yellow, Reset);
}

static std::string SelectPlatform(const GameConfig& config)
{
	auto platforms = config.GetAvailablePlatforms();
	printf("\n%sSelect platform:%s\n", Bold, Reset);

	for (size_t i = 0; i < platforms.size(); ++i)
	{
		printf("[%zu] %s", i + 1, platforms[i].c_str());
		if (platforms[i] == config.GetDefaultPlatform())
		{
			printf(" %s(default)%s", Gray, Reset);
		}
		printf("\n");
	}

	printf("\n%sChoice (default: %s): %s", Yellow, config.GetDefaultPlatform().c_str(), Reset);
	std::string choice;
	std::getline(std::cin, choice);

	if (choice.empty())
	{
		return config.GetDefaultPlatform();
	}

	try
	{
		int index = std::stoi(choice) - 1;
		if (index >= 0 && index < static_cast<int>(platforms.size()))
		{
			return platforms[index];
		}
	}
	catch (...)
	{
		Log::Error("Invalid input, please enter a valid number.");
		return config.GetDefaultPlatform();
	}

	Log::Warning("Invalid choice, using default platform");
	return config.GetDefaultPlatform();
}

static bool ExtractModuleFrom(const std::string& inputFile, const std::string& outputFolder)
{
	printf("\n%s%s=== EAC Payload Extractor ===%s\n", Bold, Cyan, Reset);
	Log::Info("Input file: %s", inputFile.c_str());

	try
	{
		EACExtractor extractor(inputFile, outputFolder);

		if (!extractor.Process())
		{
			Log::Error("Extraction failed");
			return false;
		}

		return true;
	}
	catch (const std::exception& e)
	{
		Log::Error("Exception: %s", e.what());
		return false;
	}
}

static bool DownloadThenExtract()
{
	GameConfig config;
	config.LoadConfig();
	std::string gameName;
	std::string productId;
	std::string deploymentId;

	printf("\n%s%s=== EAC Module Downloader ===%s\n", Bold, Cyan, Reset);

	if (!config.IsEmpty())
	{
		Log::Success("Found game configuration file!");
		config.ListGames();

		printf("\n%sSelect an option:%s\n", Bold, Reset);
		printf("[1-%zu] Select a game from the list\n", config.GetAllGames().size());
		printf("[%sC%s] Enter ProductID and DeploymentID\n", Yellow, Reset);
		printf("\n%sChoice: %s", Yellow, Reset);

		std::string choice;
		std::getline(std::cin, choice);

		if (!choice.empty() && choice[0] != 'c' && choice[0] != 'C')
		{
			try
			{
				int  gameIndex = std::stoi(choice) - 1;
				auto games     = config.GetAllGames();
				if (gameIndex >= 0 && gameIndex < static_cast<int>(games.size()))
				{
					auto& game   = games[gameIndex];
					gameName     = game.Name;
					productId    = game.ProductId;
					deploymentId = game.DeploymentId;

					Log::Success("Selected: %s", game.Name.c_str());
					Log::Info("ProductID: %s", game.ProductId.c_str());
					Log::Info("DeploymentID: %s", game.DeploymentId.c_str());
				}
				else
				{
					Log::Error("Invalid choice index");
					return false;
				}
			}
			catch (...)
			{
				Log::Error("Invalid choice, please enter a valid number or 'C' for custom input.");
				return false;
			}
		}
	}

	// Manual input if needed
	if (productId.empty() || deploymentId.empty())
	{
		Log::Info("Please enter the EAC module details:");
		printf("%sExample: Gray Zone Warfare%s\n", Gray, Reset);
		printf("%sProductID: 320d5d3f30f5495ebae73a8c74bc349d%s\n", Gray, Reset);
		printf("%sDeploymentID: 5844bb109c5343f78b8f76f604ae3569%s\n", Gray, Reset);

		printf("\n%sGame name (optional): %s", Yellow, Reset);
		std::getline(std::cin, gameName);

		printf("%sProductID: %s", Yellow, Reset);
		std::getline(std::cin, productId);

		printf("%sDeploymentID: %s", Yellow, Reset);
		std::getline(std::cin, deploymentId);

		if (productId.empty() || deploymentId.empty())
		{
			Log::Error("Product ID and Deployment ID are required!");
			return false;
		}

		if (!gameName.empty())
		{
			printf("\n%s[?] Save this configuration for future use? (y/N): %s", Yellow, Reset);
			std::string saveChoice;
			std::getline(std::cin, saveChoice);

			if (!saveChoice.empty() && (saveChoice[0] == 'y' || saveChoice[0] == 'Y'))
			{
				GameInfo gameInfo;
				gameInfo.Name         = gameName;
				gameInfo.ProductId    = productId;
				gameInfo.DeploymentId = deploymentId;

				config.AddGame(gameInfo);
				if (config.SaveConfig())
					Log::Success("Configuration saved successfully!");
				else
					Log::Error("Failed to save configuration");
			}
		}
	}

	// Create dump folder with game name
	const std::string dumpFolder = gameName.empty()
		                               ? FileUtils::CreateDumpFolder()
		                               : FileUtils::CreateDumpFolder(gameName);
	if (dumpFolder.empty())
	{
		Log::Error("Failed to create dump folder");
		return false;
	}

	// Select platform
	std::string platform = SelectPlatform(config);
	Log::Info("Platform: %s", platform.c_str());

	const std::string downloadedPath = dumpFolder + "\\eac_.bin";

	if (!ModuleDownloader::DownloadModule(productId, deploymentId, platform, config.GetBaseUrl(),
	                                      downloadedPath, Log::Progress))
	{
		Log::Error("Download failed");
		return false;
	}

	Log::Info("Download finished. Starting extraction...");

	return ExtractModuleFrom(downloadedPath, dumpFolder);
}

int main(int argc, char* argv[])
{
	Log::Init();
	Log::Banner();

	// Command line arguments
	if (argc > 1)
	{
		std::string arg = argv[1];
		if (arg == "-h" || arg == "--help")
		{
			PrintUsage(argv[0]);
			return 0;
		}

		if (arg == "-d" || arg == "--download")
		{
			return DownloadThenExtract() ? 0 : 1;
		}

		if (arg == "-e" || arg == "--extract")
		{
			std::string path = (argc > 2 && argv[2]) ? std::string(argv[2]) : std::string("eac_.bin");
			return ExtractModuleFrom(path, "") ? 0 : 1;
		}

		if (arg == "-l" || arg == "--list")
		{
			GameConfig config;
			if (config.LoadConfig())
			{
				config.ListGames();
			}
			else
			{
				Log::Info("No game configuration file found.");
			}
			return 0;
		}
	}

	// Interactive mode
	PrintMainMenu();

	while (true)
	{
		printf("\n%sChoice (1-4, Q): %s", Yellow, Reset);
		std::string choice;
		std::getline(std::cin, choice);

		if (choice.empty()) continue;

		switch (choice[0])
		{
		case '1':
			return DownloadThenExtract() ? 0 : 1;

		case '2':
			{
				printf("%sEnter path to EAC binary (default: eac_.bin): %s", Yellow, Reset);
				std::string path;
				std::getline(std::cin, path);
				if (path.empty()) path = "eac_.bin";
				return ExtractModuleFrom(path, "") ? 0 : 1;
			}

		case '3':
			{
				GameConfig config;
				if (config.LoadConfig())
				{
					config.ListGames();
				}
				else
				{
					Log::Info("No game configuration file found.");
					Log::Info("Use option 1 to download and configure games.");
				}
				break;
			}

		case '4':
			PrintUsage(argv[0]);
			break;

		case 'q':
		case 'Q':
			Log::Success("Goodbye!");
			return 0;

		default:
			Log::Warning("Invalid choice. Please enter 1-4 or Q.");
			break;
		}
	}

	return 0;
}
