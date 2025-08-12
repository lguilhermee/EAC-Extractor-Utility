#include "ModuleDownloader.h"
#include <iostream>
#include <cstdio>
#include <Windows.h>
#include <wininet.h>
#include <iomanip>

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
		std::cerr << "[!] Failed to initialize WinINet" << '\n';
		return false;
	}

	HINTERNET hUrl = InternetOpenUrlA(hInternet, url.c_str(), nullptr, 0, INTERNET_FLAG_RELOAD, 0);
	if (!hUrl)
	{
		std::cerr << "[!] Failed to open URL: " << url << '\n';
		InternetCloseHandle(hInternet);
		return false;
	}

	// Get content length
	DWORD bufferSize    = sizeof(DWORD);
	DWORD contentLength = 0;
	HttpQueryInfoA(hUrl, HTTP_QUERY_CONTENT_LENGTH | HTTP_QUERY_FLAG_NUMBER,
	               &contentLength, &bufferSize, nullptr);

	FILE* file = nullptr;
	if (fopen_s(&file, outputPath.c_str(), "wb") != 0 || !file)
	{
		std::cerr << "[!] Failed to create output file: " << outputPath << '\n';
		InternetCloseHandle(hUrl);
		InternetCloseHandle(hInternet);
		return false;
	}

	std::cout << "[+] Downloading module..." << '\n';
	std::cout << "[+] URL: " << url << '\n';
	std::cout << "[+] Output: " << outputPath << '\n';

	if (contentLength > 0)
	{
		std::cout << "[+] Size: " << std::fixed << std::setprecision(2)
			<< (contentLength / 1024.0 / 1024.0) << " MB" << '\n';
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
			// Simple progress without total size
			std::cout << "\r[+] Downloaded: " << std::fixed << std::setprecision(2)
				<< (totalDownloaded / 1024.0 / 1024.0) << " MB";
			std::cout.flush();
		}
	}

	std::cout << '\n';

	fclose(file);
	InternetCloseHandle(hUrl);
	InternetCloseHandle(hInternet);

	if (totalDownloaded == 0)
	{
		std::cerr << "[!] No data downloaded" << '\n';
		return false;
	}

	std::cout << "[+] Download completed successfully!" << '\n';
	std::cout << "[+] Total downloaded: " << std::fixed << std::setprecision(2)
		<< (totalDownloaded / 1024.0 / 1024.0) << " MB" << '\n';

	return true;
}
