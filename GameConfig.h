#pragma once
#include <string>
#include <vector>
#include <map>

struct GameInfo
{
	std::string Name;
	std::string ProductId;
	std::string DeploymentId;
};

struct ConfigSettings
{
	std::string BaseUrl         = "https://modules-cdn.eac-prod.on.epicgames.com/modules";
	std::string DefaultPlatform = "win64";
};

class GameConfig
{
public:
	GameConfig(std::string configFile = "games.json");

	bool LoadConfig();
	bool SaveConfig();

	void                  AddGame(const GameInfo& gameInfo);
	std::vector<GameInfo> GetAllGames() const;
	GameInfo*             FindGameByName(const std::string& name);
	GameInfo*             FindGameByProductId(const std::string& productId);

	void ListGames() const;
	bool IsEmpty() const;

	// Config settings getters
	std::string GetBaseUrl() const { return Settings.BaseUrl; }
	std::string GetDefaultPlatform() const { return Settings.DefaultPlatform; }

	// Platform related methods
	static std::vector<std::string> GetAvailablePlatforms();

private:
	std::string           ConfigFilePath;
	std::vector<GameInfo> Games;
	ConfigSettings        Settings;

	static std::string EscapeJson(const std::string& str);
	static std::string UnescapeJson(const std::string& str);
	void               ParseJsonFile(const std::string& content);
	std::string        GenerateJsonContent() const;
};
