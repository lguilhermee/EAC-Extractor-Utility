#include "GameConfig.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>

GameConfig::GameConfig(std::string configFile) : ConfigFilePath(std::move(configFile))
{
}

bool GameConfig::LoadConfig()
{
	std::ifstream file(ConfigFilePath);
	if (!file.is_open())
	{
		return false;
	}

	std::stringstream buffer;
	buffer << file.rdbuf();
	std::string content = buffer.str();
	file.close();

	try
	{
		ParseJsonFile(content);
		return true;
	}
	catch (const std::exception& e)
	{
		std::cerr << "[!] Error parsing config file: " << e.what() << '\n';
		return false;
	}
}

bool GameConfig::SaveConfig()
{
	std::ofstream file(ConfigFilePath);
	if (!file.is_open())
	{
		std::cerr << "[!] Failed to create config file: " << ConfigFilePath << '\n';
		return false;
	}

	std::string jsonContent = GenerateJsonContent();
	file << jsonContent;
	file.close();

	return true;
}

void GameConfig::AddGame(const GameInfo& gameInfo)
{
	// Check if game already exists
	for (auto& game : Games)
	{
		if (game.ProductId == gameInfo.ProductId && game.DeploymentId == gameInfo.DeploymentId)
		{
			// Update existing game
			game = gameInfo;
			return;
		}
	}

	// Add new game
	Games.push_back(gameInfo);
}

std::vector<GameInfo> GameConfig::GetAllGames() const
{
	return Games;
}

GameInfo* GameConfig::FindGameByName(const std::string& name)
{
	for (auto& game : Games)
	{
		if (game.Name == name)
		{
			return &game;
		}
	}
	return nullptr;
}

GameInfo* GameConfig::FindGameByProductId(const std::string& productId)
{
	for (auto& game : Games)
	{
		if (game.ProductId == productId)
		{
			return &game;
		}
	}
	return nullptr;
}

void GameConfig::ListGames() const
{
	if (Games.empty())
	{
		std::cout << "[i] No games configured yet." << '\n';
		return;
	}

	std::cout << "\n[+] Available games:" << '\n';
	std::cout << std::string(80, '-') << '\n';

	for (size_t i = 0; i < Games.size(); ++i)
	{
		const auto& game = Games[i];
		std::cout << "[" << (i + 1) << "] " << game.Name << '\n';
		std::cout << "    ProductID: " << game.ProductId << '\n';
		std::cout << "    DeploymentID: " << game.DeploymentId << '\n';
		std::cout << '\n';
	}
}

bool GameConfig::IsEmpty() const
{
	return Games.empty();
}

std::string GameConfig::EscapeJson(const std::string& str)
{
	std::string escaped;
	for (char c : str)
	{
		switch (c)
		{
		case '"': escaped += "\\\"";
			break;
		case '\\': escaped += "\\\\";
			break;
		case '\b': escaped += "\\b";
			break;
		case '\f': escaped += "\\f";
			break;
		case '\n': escaped += "\\n";
			break;
		case '\r': escaped += "\\r";
			break;
		case '\t': escaped += "\\t";
			break;
		default: escaped += c;
			break;
		}
	}
	return escaped;
}

std::string GameConfig::UnescapeJson(const std::string& str)
{
	std::string unescaped;
	for (size_t i = 0; i < str.length(); ++i)
	{
		if (str[i] == '\\' && i + 1 < str.length())
		{
			switch (str[i + 1])
			{
			case '"': unescaped += '"';
				i++;
				break;
			case '\\': unescaped += '\\';
				i++;
				break;
			case 'b': unescaped += '\b';
				i++;
				break;
			case 'f': unescaped += '\f';
				i++;
				break;
			case 'n': unescaped += '\n';
				i++;
				break;
			case 'r': unescaped += '\r';
				i++;
				break;
			case 't': unescaped += '\t';
				i++;
				break;
			default: unescaped += str[i];
				break;
			}
		}
		else
		{
			unescaped += str[i];
		}
	}
	return unescaped;
}

void GameConfig::ParseJsonFile(const std::string& content)
{
	Games.clear();

	// Parse config section first
	size_t configPos = content.find("\"config\"");
	if (configPos != std::string::npos)
	{
		size_t configStart = content.find('{', configPos);
		if (configStart != std::string::npos)
		{
			size_t configEnd = content.find('}', configStart);
			if (configEnd != std::string::npos)
			{
				std::string configObj = content.substr(configStart + 1, configEnd - configStart - 1);

				// Parse baseUrl
				size_t urlStart = configObj.find("\"baseUrl\"");
				if (urlStart != std::string::npos)
				{
					urlStart = configObj.find('"', urlStart + 9);
					if (urlStart != std::string::npos)
					{
						urlStart++;
						size_t urlEnd = configObj.find('"', urlStart);
						if (urlEnd != std::string::npos)
						{
							Settings.BaseUrl = UnescapeJson(configObj.substr(urlStart, urlEnd - urlStart));
						}
					}
				}

				// Parse defaultPlatform
				size_t platStart = configObj.find("\"defaultPlatform\"");
				if (platStart != std::string::npos)
				{
					platStart = configObj.find('"', platStart + 17);
					if (platStart != std::string::npos)
					{
						platStart++;
						size_t platEnd = configObj.find('"', platStart);
						if (platEnd != std::string::npos)
						{
							Settings.DefaultPlatform = configObj.substr(platStart, platEnd - platStart);
						}
					}
				}
			}
		}
	}

	// Simple JSON parser for our specific format
	size_t pos = content.find("\"games\"");
	if (pos == std::string::npos) return;

	pos = content.find('[', pos);
	if (pos == std::string::npos) return;

	size_t endPos = content.find_last_of(']');
	if (endPos == std::string::npos) return;

	std::string gamesArray = content.substr(pos + 1, endPos - pos - 1);

	// Parse each game object
	size_t gameStart = 0;
	while (true)
	{
		size_t objStart = gamesArray.find('{', gameStart);
		if (objStart == std::string::npos) break;

		size_t objEnd = gamesArray.find('}', objStart);
		if (objEnd == std::string::npos) break;

		std::string gameObj = gamesArray.substr(objStart + 1, objEnd - objStart - 1);

		GameInfo game;

		// Parse name
		size_t nameStart = gameObj.find("\"name\"");
		if (nameStart != std::string::npos)
		{
			nameStart = gameObj.find('"', nameStart + 6);
			if (nameStart != std::string::npos)
			{
				nameStart++;
				size_t nameEnd = gameObj.find('"', nameStart);
				if (nameEnd != std::string::npos)
				{
					game.Name = UnescapeJson(gameObj.substr(nameStart, nameEnd - nameStart));
				}
			}
		}

		// Parse productId
		size_t prodStart = gameObj.find("\"productId\"");
		if (prodStart != std::string::npos)
		{
			prodStart = gameObj.find('"', prodStart + 11);
			if (prodStart != std::string::npos)
			{
				prodStart++;
				size_t prodEnd = gameObj.find('"', prodStart);
				if (prodEnd != std::string::npos)
				{
					game.ProductId = gameObj.substr(prodStart, prodEnd - prodStart);
				}
			}
		}

		// Parse deploymentId
		size_t depStart = gameObj.find("\"deploymentId\"");
		if (depStart != std::string::npos)
		{
			depStart = gameObj.find('"', depStart + 14);
			if (depStart != std::string::npos)
			{
				depStart++;
				size_t depEnd = gameObj.find('"', depStart);
				if (depEnd != std::string::npos)
				{
					game.DeploymentId = gameObj.substr(depStart, depEnd - depStart);
				}
			}
		}


		if (!game.Name.empty() && !game.ProductId.empty() && !game.DeploymentId.empty())
		{
			Games.push_back(game);
		}

		gameStart = objEnd + 1;
	}
}

std::string GameConfig::GenerateJsonContent() const
{
	std::stringstream ss;
	ss << "{\n";
	ss << "  \"version\": \"1.1\",\n";
	ss << "  \"description\": \"EAC Module Configuration - Community maintained list of game IDs\",\n";
	ss << "  \"config\": {\n";
	ss << "    \"baseUrl\": \"" << EscapeJson(Settings.BaseUrl) << "\",\n";
	ss << "    \"defaultPlatform\": \"" << Settings.DefaultPlatform << "\"\n";
	ss << "  },\n";
	ss << "  \"games\": [\n";

	for (size_t i = 0; i < Games.size(); ++i)
	{
		const auto& game = Games[i];
		ss << "    {\n";
		ss << R"(      "name": ")" << EscapeJson(game.Name) << "\",\n";
		ss << R"(      "productId": ")" << game.ProductId << "\",\n";
		ss << R"(      "deploymentId": ")" << game.DeploymentId << "\"\n    }";
		if (i < Games.size() - 1)
		{
			ss << ",";
		}
		ss << "\n";
	}

	ss << "  ]\n";
	ss << "}\n";

	return ss.str();
}

std::vector<std::string> GameConfig::GetAvailablePlatforms()
{
	return {
		"win64",
		"win64_wow64",
		"linux64",
		"linux32",
		"mac64",
		"mac_arm64"
	};
}
