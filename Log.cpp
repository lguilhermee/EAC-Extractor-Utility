#include "Log.h"

namespace
{
	constexpr auto Reset   = "\x1b[0m";
	constexpr auto Bold    = "\x1b[1m";
	constexpr auto Red     = "\x1b[31m";
	constexpr auto Green   = "\x1b[32m";
	constexpr auto Yellow  = "\x1b[33m";
	constexpr auto Cyan    = "\x1b[36m";
	constexpr auto Gray    = "\x1b[90m";

	void VPrint(FILE* stream, const char* color, const char* prefix, const char* fmt, va_list args)
	{
		fprintf(stream, "%s%s", color, prefix);
		vfprintf(stream, fmt, args);
		fprintf(stream, "%s\n", Reset);
	}
}

namespace Log
{
	void Init()
	{
		SetConsoleOutputCP(CP_UTF8);

		auto enableVT = [](DWORD handle)
		{
			HANDLE h = GetStdHandle(handle);
			if (h == INVALID_HANDLE_VALUE) return;
			DWORD mode = 0;
			if (!GetConsoleMode(h, &mode)) return;
			SetConsoleMode(h, mode | ENABLE_PROCESSED_OUTPUT | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
		};

		enableVT(STD_OUTPUT_HANDLE);
		enableVT(STD_ERROR_HANDLE);
	}

	void Info(const char* fmt, ...)
	{
		va_list args;
		va_start(args, fmt);
		VPrint(stdout, "", "[*] ", fmt, args);
		va_end(args);
	}

	void Success(const char* fmt, ...)
	{
		va_list args;
		va_start(args, fmt);
		VPrint(stdout, Green, "[+] ", fmt, args);
		va_end(args);
	}

	void Warning(const char* fmt, ...)
	{
		va_list args;
		va_start(args, fmt);
		VPrint(stdout, Yellow, "[!] ", fmt, args);
		va_end(args);
	}

	void Error(const char* fmt, ...)
	{
		va_list args;
		va_start(args, fmt);
		VPrint(stderr, Red, "[!] ", fmt, args);
		va_end(args);
	}

	void Banner()
	{
		printf("%s%s", Bold, Cyan);
		printf("EAC Module Downloader & Payload Extractor\n");
		printf("==========================================%s\n", Reset);
	}

	void Progress(double pct, size_t downloaded, size_t total)
	{
		constexpr int barWidth = 50;
		int pos = static_cast<int>(barWidth * (pct / 100.0));

		printf("\r[%s", Green);
		for (int i = 0; i < barWidth; ++i)
		{
			if (i < pos)       putchar('=');
			else if (i == pos) putchar('>');
			else               putchar(' ');
		}
		printf("%s] %s%s%.1f%%%s %s(%zu/%zu MB)%s",
			Reset, Bold, "", pct, Reset,
			Gray, downloaded / 1024 / 1024, total / 1024 / 1024, Reset);
		fflush(stdout);
	}
}
