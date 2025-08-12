#include <iostream>
#include <string>
#include <Windows.h>
#include "EACExtractor.h"
#include "ModuleDownloader.h"
#include "GameConfig.h"
#include "FileUtils.h"
#include "ConsoleUtils.h"

using namespace ConsoleUtils;

static void PrintMainMenu()
{
	std::cout << '\n';
	PrintInfo("What would you like to do?");
	std::cout << Color::Yellow << "[1]" << Color::Reset << " Download and extract from Epic Games CDN" << '\n';
	std::cout << Color::Yellow << "[2]" << Color::Reset << " Extract from local file" << '\n';
	std::cout << Color::Yellow << "[3]" << Color::Reset << " List available games" << '\n';
	std::cout << Color::Yellow << "[4]" << Color::Reset << " Show help" << '\n';
	std::cout << Color::Yellow << "[Q]" << Color::Reset << " Quit" << '\n';
}

void PrintUsage(const char* programName)
{
	PrintBanner();
	std::cout << Color::Bold << "Usage:" << Color::Reset << ' ' << programName << " [options]" << '\n';
	std::cout << "       " << programName << " -h | --help" << '\n';
	std::cout << '\n' << Color::Bold << "Options:" << Color::Reset << '\n';
	std::cout << "  " << Color::Yellow << "-d, --download" << Color::Reset
		<< "    Download and then extract from Epic Games CDN" << '\n';
	std::cout << "  " << Color::Yellow << "-e, --extract" << Color::Reset
		<< "     Extract and decrypt local EAC binary (optional: path)" << '\n';
	std::cout << "  " << Color::Yellow << "-l, --list" << Color::Reset
		<< "        List available games from configuration" << '\n';
	std::cout << '\n' << Color::Bold << "Output:" << Color::Reset << '\n';
	std::cout << "  Creates timestamped folder in C:" << Color::Yellow << "\\EAC_Dumps" << Color::Reset
		<< " with extracted components" << '\n';
}

static std::string SelectPlatform(const GameConfig& config)
{
	auto platforms = config.GetAvailablePlatforms();
	std::cout << '\n' << Color::Bold << "Select platform:" << Color::Reset << '\n';

	for (size_t i = 0; i < platforms.size(); ++i)
	{
		std::cout << "[" << (i + 1) << "] " << platforms[i];
		if (platforms[i] == config.GetDefaultPlatform())
		{
			std::cout << " " << Color::Gray << "(default)" << Color::Reset;
		}
		std::cout << '\n';
	}

	std::cout << '\n' << Color::Yellow << "Choice (default: " << config.GetDefaultPlatform() << "): " << Color::Reset;
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
		PrintError("[!] Invalid input, please enter a valid number.");
		return config.GetDefaultPlatform();
	}

	PrintWarning("[!] Invalid choice, using default platform");
	return config.GetDefaultPlatform();
}

static bool DownloadModuleTo(const std::string& outputPath, const std::string& gameName = "")
{
	GameConfig  config;
	bool        hasConfig        = config.LoadConfig();
	std::string selectedGameName = gameName;
	std::string productId;
	std::string deploymentId;

	std::cout << '\n' << Color::Bold << Color::Cyan << "=== EAC Module Downloader ===" << Color::Reset <<
		'\n';

	if (hasConfig && !config.IsEmpty())
	{
		PrintSuccess("[+] Found game configuration file!");
		config.ListGames();

		std::cout << '\n' << Color::Bold << "Select an option:" << Color::Reset << '\n';
		std::cout << "[1-" << config.GetAllGames().size() << "] Select a game from the list" << '\n';
		std::cout << "[" << Color::Yellow << "C" << Color::Reset << "] Enter ProductID and DeploymentID" <<
			'\n';
		std::cout << '\n' << Color::Yellow << "Choice: " << Color::Reset;

		std::string choice;
		std::getline(std::cin, choice);

		if (!choice.empty() && (choice[0] == 'c' || choice[0] == 'C'))
		{
			// Custom input - fall through to manual input
		}
		else
		{
			try
			{
				int  gameIndex = std::stoi(choice) - 1;
				auto games     = config.GetAllGames();
				if (gameIndex >= 0 && gameIndex < static_cast<int>(games.size()))
				{
					auto& game       = games[gameIndex];
					selectedGameName = game.Name;
					productId        = game.ProductId;
					deploymentId     = game.DeploymentId;

					PrintSuccess(std::string("[+] Selected: ") + game.Name);
					PrintInfo(std::string("[+] ProductID: ") + game.ProductId);
					PrintInfo(std::string("[+] DeploymentID: ") + game.DeploymentId);

					// Select platform
					std::string platform = SelectPlatform(config);
					PrintInfo(std::string("[+] Platform: ") + platform);

					return ModuleDownloader::DownloadModule(game.ProductId, game.DeploymentId,
					                                        platform, config.GetBaseUrl(),
					                                        outputPath, ShowProgress);
				}
			}
			catch (...)
			{
				PrintError("[!] Invalid choice, please enter a valid number or 'C' for custom input.");
			}
		}
	}

	// Manual input
	if (productId.empty() || deploymentId.empty())
	{
		PrintInfo("\n[+] Please enter the EAC module details:");
		std::cout << Color::Gray << "[+] Example: Gray Zone Warfare" << Color::Reset << '\n';
		std::cout << Color::Gray << "[+] ProductID: 320d5d3f30f5495ebae73a8c74bc349d" << Color::Reset << '\n';
		std::cout << Color::Gray << "[+] DeploymentID: 5844bb109c5343f78b8f76f604ae3569" << Color::Reset << '\n';

		std::cout << "\n" << Color::Yellow << "Game name (optional): " << Color::Reset;
		std::getline(std::cin, selectedGameName);

		std::cout << Color::Yellow << "ProductID: " << Color::Reset;
		std::getline(std::cin, productId);

		std::cout << Color::Yellow << "DeploymentID: " << Color::Reset;
		std::getline(std::cin, deploymentId);
	}

	if (productId.empty() || deploymentId.empty())
	{
		PrintError("[!] Product ID and Deployment ID are required!");
		return false;
	}

	// Save to config if game name provided
	if (!selectedGameName.empty() && (gameName.empty() || gameName == selectedGameName))
	{
		std::cout << '\n' << Color::Yellow << "[?] Save this configuration for future use? (y/N): " <<
			Color::Reset;
		std::string saveChoice;
		std::getline(std::cin, saveChoice);

		if (!saveChoice.empty() && (saveChoice[0] == 'y' || saveChoice[0] == 'Y'))
		{
			GameInfo gameInfo;
			gameInfo.Name         = selectedGameName;
			gameInfo.ProductId    = productId;
			gameInfo.DeploymentId = deploymentId;

			config.AddGame(gameInfo);
			if (config.SaveConfig())
			{
				PrintSuccess("[+] Configuration saved successfully!");
			}
			else
			{
				PrintError("[!] Failed to save configuration");
			}
		}
	}

	// Select platform
	std::string platform = SelectPlatform(config);
	PrintInfo(std::string("[+] Platform: ") + platform);

	return ModuleDownloader::DownloadModule(productId, deploymentId, platform, config.GetBaseUrl(),
	                                        outputPath, ShowProgress);
}

bool ExtractModuleFrom(const std::string& inputFile, const std::string& outputFolder)
{
	std::cout << '\n' << Color::Bold << Color::Cyan << "=== EAC Payload Extractor ===" << Color::Reset <<
		'\n';
	PrintInfo(std::string("[+] Input file: ") + inputFile);

	try
	{
		EACExtractor extractor(outputFolder.empty() ? inputFile : inputFile, outputFolder);

		if (!extractor.Process())
		{
			PrintError("[!] Extraction failed");
			return false;
		}

		PrintSuccess("[+] Extraction completed successfully!");
		return true;
	}
	catch (const std::exception& e)
	{
		PrintError(std::string("[!] Exception: ") + e.what());
		return false;
	}
}

static bool ExtractModule()
{
	return ExtractModuleFrom("eac_.bin", "");
}

static bool DownloadThenExtract()
{
	GameConfig config;
	config.LoadConfig();
	std::string gameName;
	std::string productId;
	std::string deploymentId;

	std::cout << '\n' << Color::Bold << Color::Cyan << "=== EAC Module Downloader ===" << Color::Reset << '\n';

	// Handle game selection
	if (!config.IsEmpty())
	{
		PrintSuccess("[+] Found game configuration file!");
		config.ListGames();

		std::cout << '\n' << Color::Bold << "Select an option:" << Color::Reset << '\n';
		std::cout << "[1-" << config.GetAllGames().size() << "] Select a game from the list" << '\n';
		std::cout << "[" << Color::Yellow << "C" << Color::Reset << "] Enter ProductID and DeploymentID" << '\n';
		std::cout << '\n' << Color::Yellow << "Choice: " << Color::Reset;

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

					PrintSuccess(std::string("[+] Selected: ") + game.Name);
					PrintInfo(std::string("[+] ProductID: ") + game.ProductId);
					PrintInfo(std::string("[+] DeploymentID: ") + game.DeploymentId);
				}
				else
				{
					PrintError("[!] Invalid choice index");
					return false;
				}
			}
			catch (...)
			{
				PrintError("[!] Invalid choice, please enter a valid number or 'C' for custom input.");
				return false;
			}
		}
	}

	// Manual input if needed
	if (productId.empty() || deploymentId.empty())
	{
		PrintInfo("\n[+] Please enter the EAC module details:");
		std::cout << Color::Gray << "[+] Example: Gray Zone Warfare" << Color::Reset << '\n';
		std::cout << Color::Gray << "[+] ProductID: 320d5d3f30f5495ebae73a8c74bc349d" << Color::Reset << '\n';
		std::cout << Color::Gray << "[+] DeploymentID: 5844bb109c5343f78b8f76f604ae3569" << Color::Reset << '\n';

		std::cout << "\n" << Color::Yellow << "Game name (optional): " << Color::Reset;
		std::getline(std::cin, gameName);

		std::cout << Color::Yellow << "ProductID: " << Color::Reset;
		std::getline(std::cin, productId);

		std::cout << Color::Yellow << "DeploymentID: " << Color::Reset;
		std::getline(std::cin, deploymentId);

		if (productId.empty() || deploymentId.empty())
		{
			PrintError("[!] Product ID and Deployment ID are required!");
			return false;
		}

		// Save to config if game name provided
		if (!gameName.empty())
		{
			std::cout << '\n' << Color::Yellow << "[?] Save this configuration for future use? (y/N): " <<
				Color::Reset;
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
				{
					PrintSuccess("[+] Configuration saved successfully!");
				}
				else
				{
					PrintError("[!] Failed to save configuration");
				}
			}
		}
	}

	// Create dump folder with game name
	const std::string dumpFolder = gameName.empty()
		                               ? FileUtils::CreateDumpFolder()
		                               : FileUtils::CreateDumpFolder(gameName);
	if (dumpFolder.empty())
	{
		PrintError("[!] Failed to create dump folder");
		return false;
	}

	PrintSuccess(std::string("[+] Created dump folder: ") + dumpFolder);

	// Select platform
	std::string platform = SelectPlatform(config);
	PrintInfo(std::string("[+] Platform: ") + platform);

	const std::string downloadedPath = dumpFolder + "\\eac_.bin";

	// Download directly using ModuleDownloader
	if (!ModuleDownloader::DownloadModule(productId, deploymentId, platform, config.GetBaseUrl(),
	                                      downloadedPath, ShowProgress))
	{
		PrintError("[!] Download failed");
		return false;
	}

	PrintInfo("\n[*] Download finished. Starting extraction...");

	return ExtractModuleFrom(downloadedPath, dumpFolder);
}

int main(int argc, char* argv[])
{
	SetConsoleOutputCP(CP_UTF8);
	EnableVirtualTerminalProcessing();
	PrintBanner();

	// Check for command line arguments
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
				PrintInfo("[i] No game configuration file found.");
			}
			return 0;
		}
	}

	// Interactive mode
	PrintMainMenu();

	while (true)
	{
		std::cout << '\n' << Color::Yellow << "Choice (1-4, Q): " << Color::Reset;
		std::string choice;
		std::getline(std::cin, choice);

		if (choice.empty()) continue;

		switch (choice[0])
		{
		case '1':
			return DownloadThenExtract() ? 0 : 1;

		case '2':
			{
				std::cout << Color::Yellow << "Enter path to EAC binary (default: eac_.bin): " << Color::Reset;
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
					PrintInfo("[i] No game configuration file found.");
					PrintInfo("[i] Use option 1 to download and configure games.");
				}
				break;
			}

		case '4':
			PrintUsage(argv[0]);
			break;

		case 'q':
		case 'Q':
			PrintSuccess("Goodbye!");
			return 0;

		default:
			PrintWarning("[!] Invalid choice. Please enter 1-4 or Q.");
			break;
		}
	}

	return 0;
}
