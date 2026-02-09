#include "ModuleDownloader.h"
#include "Log.h"
#include <cstdio>
#include <iomanip>
#include <Windows.h>
#include <wininet.h>

#pragma comment(lib, "wininet.lib")


std::string ModuleDownloader::BuildDownloadUrl(const std::string& productId, const std::string& deploymentId,
                                               const std::string& platform, const std::string&  baseUrl)
{
	return baseUrl + "/" + productId + "/" + deploymentId + "/" + platform;
}

bool ModuleDownloader::DownloadModule(const std::string& productId, const std::string&       deploymentId,
                                      const std::string& outputPath, const ProgressCallback& progressCallback)
{
	std::string url = BuildDownloadUrl(productId, deploymentId);
	return PerformDownload(url, outputPath, progressCallback);
}

bool ModuleDownloader::DownloadModule(const std::string&      productId, const std::string& deploymentId,
                                      const std::string&      platform, const std::string&  outputPath,
                                      const ProgressCallback& progressCallback)
{
	std::string url = BuildDownloadUrl(productId, deploymentId, platform);
	return PerformDownload(url, outputPath, progressCallback);
}

bool ModuleDownloader::DownloadModule(const std::string& productId, const std::string&       deploymentId,
                                      const std::string& platform, const std::string&        baseUrl,
                                      const std::string& outputPath, const ProgressCallback& progressCallback)
{
	std::string url = BuildDownloadUrl(productId, deploymentId, platform, baseUrl);
	return PerformDownload(url, outputPath, progressCallback);
}

bool ModuleDownloader::PerformDownload(const std::string& url, const std::string& outputPath, const ProgressCallback&
                                       progressCallback)
{
	HINTERNET hInternet = InternetOpenA("EAC Module Downloader", INTERNET_OPEN_TYPE_DIRECT, nullptr, nullptr, 0);
	if (!hInternet)
	{
		Log::Error("Failed to initialize WinINet");
		return false;
	}

	HINTERNET hUrl = InternetOpenUrlA(hInternet, url.c_str(), nullptr, 0, INTERNET_FLAG_RELOAD, 0);
	if (!hUrl)
	{
		Log::Error("Failed to open URL: %s", url.c_str());
		InternetCloseHandle(hInternet);
		return false;
	}

	DWORD bufferSize    = sizeof(DWORD);
	DWORD contentLength = 0;
	HttpQueryInfoA(hUrl, HTTP_QUERY_CONTENT_LENGTH | HTTP_QUERY_FLAG_NUMBER,
	               &contentLength, &bufferSize, nullptr);

	FILE* file = nullptr;
	if (fopen_s(&file, outputPath.c_str(), "wb") != 0 || !file)
	{
		Log::Error("Failed to create output file: %s", outputPath.c_str());
		InternetCloseHandle(hUrl);
		InternetCloseHandle(hInternet);
		return false;
	}

	Log::Info("Downloading module...");
	Log::Info("URL: %s", url.c_str());
	Log::Info("Output: %s", outputPath.c_str());

	if (contentLength > 0)
	{
		Log::Info("Size: %.2f MB", contentLength / 1024.0 / 1024.0);
	}

	constexpr DWORD bufferLen = 8192;
	char            buffer[bufferLen];
	DWORD           bytesRead;
	DWORD           totalDownloaded = 0;

	while (InternetReadFile(hUrl, buffer, bufferLen, &bytesRead) && bytesRead > 0)
	{
		fwrite(buffer, 1, bytesRead, file);
		totalDownloaded += bytesRead;

		if (progressCallback && contentLength > 0)
		{
			double percentage = (static_cast<double>(totalDownloaded) / contentLength) * 100.0;
			progressCallback(percentage, totalDownloaded, contentLength);
		}
		else
		{
			printf("\r[+] Downloaded: %.2f MB", totalDownloaded / 1024.0 / 1024.0);
			fflush(stdout);
		}
	}

	printf("\n");

	fclose(file);
	InternetCloseHandle(hUrl);
	InternetCloseHandle(hInternet);

	if (totalDownloaded == 0)
	{
		Log::Error("No data downloaded");
		return false;
	}

	Log::Success("Download completed successfully!");
	Log::Info("Total downloaded: %.2f MB", totalDownloaded / 1024.0 / 1024.0);

	return true;
}
