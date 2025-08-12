#include "PatternScanner.h"
#include <iostream>
#include <cstring>

namespace PatternScanner
{
	uintptr_t ResolveRelative(uintptr_t addressPointer, ULONG offsetCount, ULONG sizeOfInstruction)
	{
		const ULONG_PTR addressToResolve = addressPointer;
		const LONG      totalBytesFromSpecifiedAddress = *(PLONG)(addressToResolve + offsetCount);
		const uintptr_t resultFinal = (addressToResolve + sizeOfInstruction + totalBytesFromSpecifiedAddress);

		return resultFinal;
	}

	std::vector<uintptr_t> PatternScan(uintptr_t moduleAddress, const char* signature)
	{
		std::vector<uintptr_t> results;

		if (!moduleAddress)
			return results;

		auto patternToByte = [](const char* pattern)
		{
			auto       bytes = std::vector<int>{};
			const auto start = const_cast<char*>(pattern);
			const auto end   = const_cast<char*>(pattern) + strlen(pattern);

			for (auto current = start; current < end; ++current)
			{
				if (*current == '?')
				{
					++current;
					if (*current == '?')
						++current;
					bytes.push_back(-1);
				}
				else
				{
					bytes.push_back(strtoul(current, &current, 16));
				}
			}
			return bytes;
		};

		const auto dosHeader = (PIMAGE_DOS_HEADER)moduleAddress;
		const auto ntHeaders = (PIMAGE_NT_HEADERS)((std::uint8_t*)moduleAddress + dosHeader->e_lfanew);

		const auto sizeOfImage  = ntHeaders->OptionalHeader.SizeOfImage;
		auto       patternBytes = patternToByte(signature);
		const auto scanBytes    = reinterpret_cast<std::uint8_t*>(moduleAddress);

		const auto s = patternBytes.size();
		const auto d = patternBytes.data();

		for (auto i = 0ul; i < sizeOfImage - s; ++i)
		{
			bool found = true;
			for (auto j = 0ul; j < s; ++j)
			{
				if (scanBytes[i + j] != d[j] && d[j] != -1)
				{
					found = false;
					break;
				}
			}
			if (found)
			{
				results.push_back(reinterpret_cast<uintptr_t>(&scanBytes[i]));
			}
		}

		return results;
	}

	uintptr_t FindFirstMatch(uintptr_t moduleAddress, const char* signature)
	{
		auto results = PatternScan(moduleAddress, signature);
		if (!results.empty())
		{
			std::cout << "[+] Pattern found: " << signature << " at 0x"
				<< std::hex << results[0] << std::dec << std::endl;
			return results[0];
		}
		std::cerr << "[!] Pattern not found: " << signature << std::endl;
		return 0;
	}
}
