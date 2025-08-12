///**
// * Document Name: EAC_Decrypt_Extract.cpp
// * Author: Lucas Guilherme
// * Description: This C++ program decrypts and extracts files from an EAC (Easy Anti-Cheat) binary.
// *              It performs a sequence of operations such as loading an encrypted file, decrypting the file,
// *              searching for specific patterns within the loaded library, copying encrypted data into separate vectors,
// *              and decrypting the data, followed by saving them as separate files.
// * Date: 22/05/2023
// *
// * This program performs the following steps:
// * 1. Loads an encrypted file specified as a command-line argument.
// * 2. Decrypts the loaded file using a custom decryption algorithm.
// * 3. Retrieves the current directory path.
// * 4. Saves the decrypted file with a specific name in the current directory.
// * 5. Loads the decrypted file as a library.
// * 6. Searches for specific patterns within the loaded library to determine the start and size of data.
// * 7. Copies encrypted data based on the identified patterns into separate vectors.
// * 8. Decrypts the data in the vectors.
// * 9. Saves the decrypted data as separate files in the current directory.
// *
// * Command-line usage: EAC_Decrypt_Extract.exe <EAC.Bin>
// *
// * Dependencies: Windows.h, fstream, format
// *
// */
//
//#include <iostream>
//#include <vector>
//#include <Windows.h>
//#include <fstream>
//#include <format>
//
//uintptr_t ResolveRelative(const uintptr_t adressPointer, const ULONG offsetCount,
//                          const ULONG     sizeOfInstruction)
//{
//    const ULONG_PTR adressToResolve               = adressPointer;
//    const LONG      totalBytesFromSpecifiedAdress = *(PLONG)(adressToResolve + offsetCount);
//    const uintptr_t resultFinal                   = (adressToResolve + sizeOfInstruction + totalBytesFromSpecifiedAdress
//    );
//
//    return resultFinal;
//}
//
//std::vector<uintptr_t> PatternScan(const uintptr_t moduleAdress, const char* signature)
//{
//    std::vector<uintptr_t> tmp;
//
//    if (!moduleAdress)
//        return tmp;
//
//    static auto patternToByte = [](const char* pattern)
//    {
//        auto       bytes = std::vector<int>{};
//        const auto start = const_cast<char*>(pattern);
//        const auto end   = const_cast<char*>(pattern) + strlen(pattern);
//
//        for (auto current = start; current < end; ++current)
//        {
//            if (*current == '?')
//            {
//                ++current;
//                if (*current == '?')
//                    ++current;
//                bytes.push_back(-1);
//            }
//            else { bytes.push_back(strtoul(current, &current, 16)); }
//        }
//        return bytes;
//    };
//
//    const auto dosHeader = (PIMAGE_DOS_HEADER)moduleAdress;
//    const auto ntHeaders = (PIMAGE_NT_HEADERS)((std::uint8_t*)moduleAdress + dosHeader->e_lfanew);
//
//    const auto sizeOfImage  = ntHeaders->OptionalHeader.SizeOfImage;
//    auto       patternBytes = patternToByte(signature);
//    const auto scanBytes    = reinterpret_cast<std::uint8_t*>(moduleAdress);
//
//    const auto s = patternBytes.size();
//    const auto d = patternBytes.data();
//
//    for (auto i = 0ul; i < sizeOfImage - s; ++i)
//    {
//        bool found = true;
//        for (auto j = 0ul; j < s; ++j)
//        {
//            if (scanBytes[i + j] != d[j] && d[j] != -1)
//            {
//                found = false;
//                break;
//            }
//        }
//        if (found)
//        {
//            tmp.push_back(reinterpret_cast<uintptr_t>(&scanBytes[i]));
//        }
//    }
//    return tmp;
//}
//
//std::vector<uint8_t> LoadBinaryFile(const std::string& filePath)
//{
//    std::ifstream file(filePath, std::ios::binary);
//    if (!file)
//    {
//        std::cerr << "Failed to open the file: " << filePath << std::endl;
//        return {};
//    }
//
//    // Determine the size of the file
//    file.seekg(0, std::ios::end);
//    std::streampos fileSize = file.tellg();
//    file.seekg(0, std::ios::beg);
//
//    // Create a vector with the appropriate size
//    std::vector<uint8_t> buffer(fileSize);
//
//    // Read the file into the vector
//    if (!file.read(reinterpret_cast<char*>(buffer.data()), fileSize))
//    {
//        std::cerr << "Failed to read the file: " << filePath << std::endl;
//        return {};
//    }
//
//    return buffer;
//}
//
//bool SaveBinaryFile(const std::string& filePath, const std::vector<uint8_t>& data)
//{
//    std::ofstream file(filePath, std::ios::binary);
//    if (!file)
//    {
//        std::cerr << "Failed to create the file: " << filePath << std::endl;
//        return false;
//    }
//
//    file.write(reinterpret_cast<const char*>(data.data()), data.size());
//    if (!file)
//    {
//        std::cerr << "Failed to write the data to the file: " << filePath << std::endl;
//        return false;
//    }
//
//    return true;
//}
//
//
//std::vector<uint8_t> DecryptBuffer(const std::vector<uint8_t>* encryptedVector)
//{
//    // Copy the buffer to a temporary vector
//    std::vector<uint8_t> tmp = *encryptedVector;
//
//    // Begin
//    auto beginBuffer = tmp.data();
//
//    // End
//    auto endBuffer = beginBuffer + tmp.size();
//
//    // Pointer to the current byte in the buffer
//    uint8_t* pCurrentByte = tmp.data();
//
//    // Calculate the size of the module
//    size_t moduleSize = endBuffer - beginBuffer;
//
//    // Ensure the module size is at least 2
//    if (moduleSize >= 2)
//    {
//        // Adjust the last byte in the buffer
//        endBuffer[-1] += 3 - 3 * (LOBYTE(endBuffer) - LOBYTE(beginBuffer));
//
//        // Iterate over the buffer in reverse order, starting from the second last byte
//        for (size_t i = moduleSize - 2; i; --i)
//            pCurrentByte[i] += -3 * i - pCurrentByte[i + 1];
//
//        // Adjust the first byte in the buffer
//        *pCurrentByte -= pCurrentByte[1];
//    }
//
//    return tmp;
//}
//
//std::vector<uint8_t> EncryptBuffer(const std::vector<uint8_t>* decryptedVector)
//{
//
//    // Copy the buffer to a temporary vector
//    std::vector<uint8_t> tmp = *decryptedVector;
//
//    // Begin
//    auto beginBuffer = tmp.data();
//
//    // End
//    auto endBuffer = beginBuffer + tmp.size();
//
//    // Pointer to the current byte in the buffer
//    uint8_t* pCurrentByte = tmp.data();
//
//    // Calculate the size of the module
//    size_t moduleSize = endBuffer - beginBuffer;
//
//    // Ensure the module size is at least 2
//    if (moduleSize >= 2)
//    {
//        // Reverse adjust the first byte in the buffer
//        *pCurrentByte += pCurrentByte[1];
//
//        // Iterate over the buffer in forward order, starting from the first byte
//        for (size_t i = 0; i < moduleSize - 2; ++i)
//            pCurrentByte[i] -= -3 * i - pCurrentByte[i + 1];
//
//        // Reverse adjust the last byte in the buffer
//        endBuffer[-1] -= 3 - 3 * (LOBYTE(endBuffer[-1]) - LOBYTE(beginBuffer[0]));
//    }
//
//    pCurrentByte[0] = 0xA7;
//    tmp.at(moduleSize-1) = 0xfd;
//    tmp.at(moduleSize-2) = 0xfa;
//
//
//    return tmp;
//
//}
//
//
//int main(int argc, char* argv[])
//{
// /*   if (argc < 2)
//    {
//        std::printf("Usage: %s <EAC.Bin>\n", argv[0]);
//        return 1;
//    }*/
//
//    std::printf("[-] Decrypting files...\n");
//
//    std::string filePath = R"(d:\eac_.bin)";
//
//    // 1. Load the file
//    auto encryptedfile = LoadBinaryFile(filePath);
//
//    // 2. Decrypt the file
//    auto decryptedBuffer = DecryptBuffer(&encryptedfile);
//
//
//    char currentDir[MAX_PATH];
//    GetCurrentDirectoryA(MAX_PATH, currentDir);
//
//    // 3. Save the decrypted file
//    SaveBinaryFile(std::format(R"(D:\EAC_Launcher.dll)", currentDir), decryptedBuffer);
//
//    // 4. Load the EAC_Launcher
//    auto launcherData = LoadLibraryA(std::format(R"(D:\EAC_Launcher.dll)", currentDir).c_str());
//
//    // 5. Find the Pattern of start data and the size of the data
//    auto userModuleEncrypted = PatternScan((uintptr_t)launcherData, "A7 ED 96 0C 0F").at(0);
//
//    //auto result2 = ResolveRelative(PatternScan((uintptr_t)launcherData, "4D 8B C6 48 8D 15 ? ? ? ? 48 8B CE").at(0)+3,3,7);
//    //auto size22 = *(uint32_t*)ResolveRelative(PatternScan((uintptr_t)launcherData, "74 04 49 89 40 08").at(0)+6, 3, 7);
//
//#ifdef _WIN64
//    auto userModeModuleSize = *(uint32_t*)ResolveRelative(PatternScan((uintptr_t)launcherData, "8B 15 ? ? ? ? 48 89 7C 24").at(0), 2, 6);
//    //auto size1 = *(uint32_t*)ResolveRelative(PatternScan((uintptr_t)launcherData, "74 04 49 89 40 08").at(0) + 0x6, 3, 7);
//#else
//    auto size1 = *(uint32_t*)*(uint32_t*)(PatternScan((uintptr_t)launcherData, "8B 93 ? 00 00 00 8B 0D").at(0) + 0x8);
//    auto size2 = *(uint32_t*)(*(uint32_t*)(PatternScan((uintptr_t)launcherData, "FF 35 ? ? ? ? 99 68").at(0) + 2));
//#endif
//
//    // 6. Copy to an Vector
//    auto userModeDriverEncrypted = std::vector((uint8_t*)userModuleEncrypted, (uint8_t*)userModuleEncrypted + userModeModuleSize);
//    //auto driverDataEncrypted = std::vector((uint8_t*)result2, (uint8_t*)result2 + size22);
//
//    // 7. Decrypt the data
//    auto usermodeDataDecrypted = DecryptBuffer(&userModeDriverEncrypted);
//    auto encryptedAgain = EncryptBuffer(&usermodeDataDecrypted);
//    SaveBinaryFile(std::format("{}\\1.dll", currentDir), userModeDriverEncrypted);
//    SaveBinaryFile(std::format("{}\\2.dll", currentDir), encryptedAgain);
//
//    
//    auto encryptedAgain3 = DecryptBuffer(&usermodeDataDecrypted);
//
//    //auto driverDataDecrypted   = DecryptBuffer(&driverDataEncrypted);
//
//    // 8. Save the data
//    SaveBinaryFile(std::format(R"(D:\EAC_Launcher_decrypted.dll)", currentDir), usermodeDataDecrypted);
//    //SaveBinaryFile(std::format("{}\\EAC_Driver.sys", currentDir), driverDataDecrypted);
//
//    // 9. Print the result
//    std::printf("[-] All files successfully generated! \n");
//    Sleep(1000);
//
//
//    return 0;
//}
