#include "ConsoleUtils.h"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <Windows.h>

namespace ConsoleUtils
{
	bool EnableVirtualTerminalProcessing()
	{
		HANDLE stdoutHandle = GetStdHandle(STD_OUTPUT_HANDLE);
		if (stdoutHandle == INVALID_HANDLE_VALUE) return false;

		DWORD outMode = 0;
		if (!GetConsoleMode(stdoutHandle, &outMode)) return false;
		outMode |= ENABLE_PROCESSED_OUTPUT | ENABLE_VIRTUAL_TERMINAL_PROCESSING;
		if (!SetConsoleMode(stdoutHandle, outMode)) return false;

		HANDLE stderrHandle = GetStdHandle(STD_ERROR_HANDLE);
		if (stderrHandle != INVALID_HANDLE_VALUE)
		{
			DWORD errMode = 0;
			if (GetConsoleMode(stderrHandle, &errMode))
			{
				errMode |= ENABLE_PROCESSED_OUTPUT | ENABLE_VIRTUAL_TERMINAL_PROCESSING;
				SetConsoleMode(stderrHandle, errMode);
			}
		}
		return true;
	}

	void PrintInfo(const std::string& msg)
	{
		std::cout << Color::Cyan << msg << Color::Reset << '\n';
	}

	void PrintSuccess(const std::string& msg)
	{
		std::cout << Color::Green << msg << Color::Reset << '\n';
	}

	void PrintWarning(const std::string& msg)
	{
		std::cout << Color::Yellow << msg << Color::Reset << '\n';
	}

	void PrintError(const std::string& msg)
	{
		std::cerr << Color::Red << msg << Color::Reset << '\n';
	}

	void PrintBanner()
	{
		std::cout << Color::Bold << Color::Cyan
			<< "EAC Module Downloader & Payload Extractor" << Color::Reset << '\n';
		std::cout << Color::Cyan << "==========================================" << Color::Reset << '\n';
	}

	void ShowProgress(double percentage, size_t downloaded, size_t total)
	{
		constexpr int barWidth = 50;
		const int     pos      = static_cast<int>(barWidth * (percentage / 100.0));

		std::ostringstream bar;
		bar << '\r' << '[' << Color::Green;
		for (int i = 0; i < barWidth; ++i)
		{
			if (i < pos) bar << '=';
			else if (i == pos) bar << '>';
			else bar << ' ';
		}
		bar << Color::Reset << "] "
			<< Color::Bold << std::fixed << std::setprecision(1) << percentage << "%" << Color::Reset
			<< ' ' << Color::Gray << '('
			<< (downloaded / 1024 / 1024) << "/" << (total / 1024 / 1024) << " MB)" << Color::Reset;

		std::cout << bar.str();
		std::cout.flush();
	}
} 
