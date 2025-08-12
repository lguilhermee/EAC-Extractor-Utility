#pragma once
#include <vector>
#include <cstdint>
#include <Windows.h>

namespace PatternScanner {
    std::vector<uintptr_t> PatternScan(uintptr_t moduleAddress, const char* signature);
    uintptr_t ResolveRelative(uintptr_t addressPointer, ULONG offsetCount, ULONG sizeOfInstruction);
    uintptr_t FindFirstMatch(uintptr_t moduleAddress, const char* signature);
}