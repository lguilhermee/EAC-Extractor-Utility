#pragma once
#include <cstdio>
#include <cstdarg>
#include <Windows.h>

namespace Log
{
	void Init();

	void Info(const char* fmt, ...);
	void Success(const char* fmt, ...);
	void Warning(const char* fmt, ...);
	void Error(const char* fmt, ...);
	void Banner();
	void Progress(double pct, size_t downloaded, size_t total);
}
