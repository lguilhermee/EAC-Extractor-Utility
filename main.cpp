#include <iostream>
#include <string>
#include <Windows.h>
#include "EACExtractor.h"

void PrintUsage(const char* programName) {
    std::cout << "Usage: " << programName << " <EAC.bin>" << std::endl;
    std::cout << "       " << programName << " -h | --help" << std::endl;
    std::cout << "\nDescription:" << std::endl;
    std::cout << "  Extracts and decrypts EAC payloads for analysis" << std::endl;
    std::cout << "\nOutput:" << std::endl;
    std::cout << "  Creates timestamped folder in C:\\EAC_Dumps with:" << std::endl;
    std::cout << "  - Original EAC binary" << std::endl;
    std::cout << "  - Decrypted launcher DLL" << std::endl;
    std::cout << "  - Memory dump of loaded launcher" << std::endl;
    std::cout << "  - Extracted user mode module" << std::endl;
    std::cout << "  - Extracted driver module (if present)" << std::endl;
}

int main(int argc, char* argv[]) {
    SetConsoleOutputCP(CP_UTF8);
    
    /*if (argc < 2) {
        PrintUsage(argv[0]);
        return 1;
    }
    
    std::string arg = argv[1];
    if (arg == "-h" || arg == "--help") {
        PrintUsage(argv[0]);
        return 0;
    }*/
    
    std::string inputFile = "d:\\eac_.bin";// argv[1];
    
    try {
        EACExtractor extractor(inputFile);
        
        if (!extractor.Process()) {
            std::cerr << "[!] Extraction failed" << std::endl;
            return 1;
        }
        
    } catch (const std::exception& e) {
        std::cerr << "[!] Exception: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}