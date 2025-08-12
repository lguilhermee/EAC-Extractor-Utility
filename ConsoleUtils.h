#pragma once
#include <string>

namespace ConsoleUtils
{
	// Terminal colors using ANSI VT sequences
	namespace Color
	{
		static constexpr auto Reset   = "\x1b[0m";
		static constexpr auto Bold    = "\x1b[1m";
		static constexpr auto Red     = "\x1b[31m";
		static constexpr auto Green   = "\x1b[32m";
		static constexpr auto Yellow  = "\x1b[33m";
		static constexpr auto Blue    = "\x1b[34m";
		static constexpr auto Magenta = "\x1b[35m";
		static constexpr auto Cyan    = "\x1b[36m";
		static constexpr auto Gray    = "\x1b[90m";
	}

	// Enable virtual terminal processing for Windows
	bool EnableVirtualTerminalProcessing();

	// Formatted output functions
	void PrintInfo(const std::string& msg);
	void PrintSuccess(const std::string& msg);
	void PrintWarning(const std::string& msg);
	void PrintError(const std::string& msg);
	void PrintBanner();

	// Progress bar display
	void ShowProgress(double percentage, size_t downloaded, size_t total);
}
