#pragma once
#include <string>
#include <functional>

class ModuleDownloader
{
public:
	using ProgressCallback = std::function<void(double percentage, size_t downloaded, size_t total)>;

	static bool DownloadModule(const std::string& productId, const std::string&       deploymentId,
	                           const std::string& outputPath, const ProgressCallback& progressCallback = nullptr);

	static bool DownloadModule(const std::string&      productId, const std::string& deploymentId,
	                           const std::string&      platform, const std::string&  outputPath,
	                           const ProgressCallback& progressCallback = nullptr);

	static bool DownloadModule(const std::string& productId, const std::string&       deploymentId,
	                           const std::string& platform, const std::string&        baseUrl,
	                           const std::string& outputPath, const ProgressCallback& progressCallback = nullptr);

private:
	static std::string BuildDownloadUrl(const std::string& productId, const std::string& deploymentId,
	                                    const std::string& platform = "win64",
	                                    const std::string& baseUrl  =
		                                    "https://modules-cdn.eac-prod.on.epicgames.com/modules");
	static bool PerformDownload(const std::string&      url, const std::string& outputPath,
	                            const ProgressCallback& progressCallback);
};
