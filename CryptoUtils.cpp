#include <Windows.h>
#include <iostream>
#include <compressapi.h>
#include "CryptoUtils.h"

#pragma comment(lib, "Cabinet.lib")


namespace CryptoUtils
{
	std::vector<uint8_t> DecryptBuffer(const std::vector<uint8_t>& encryptedVector)
	{
		std::vector<uint8_t> tmp = encryptedVector;

		auto     beginBuffer  = tmp.data();
		auto     endBuffer    = beginBuffer + tmp.size();
		uint8_t* pCurrentByte = tmp.data();
		size_t   moduleSize   = endBuffer - beginBuffer;

		if (moduleSize >= 2)
		{
			endBuffer[-1] += 3 - 3 * moduleSize; // Use the actual size!

			for (size_t i = moduleSize - 2; i; --i)
				pCurrentByte[i] += -3 * i - pCurrentByte[i + 1];

			*pCurrentByte -= pCurrentByte[1];
		}

		std::cout << "[+] Decrypted buffer (" << moduleSize << " bytes)" << std::endl;
		return tmp;
	}

	bool DecompressBuffer(
		BYTE*   compressedData,
		size_t  compressedSize,
		BYTE**  outBuffer,
		size_t* outSize
	)
	{
		DECOMPRESSOR_HANDLE decHandle = nullptr;

		// Create a decompressor for MSZIP (choose correct algo for your data)
		if (!CreateDecompressor(COMPRESS_ALGORITHM_MSZIP, nullptr, &decHandle))
		{
			return false;
		}

		// First call to get required buffer size
		size_t needed = 0;
		BOOL   ok     = Decompress(decHandle, compressedData, compressedSize,
		                     nullptr, 0, &needed);
		if (!ok && GetLastError() != ERROR_INSUFFICIENT_BUFFER)
		{
			CloseDecompressor(decHandle);
			return false;
		}
		// Allocate buffer
		auto buffer = static_cast<BYTE*>(malloc(needed));
		if (!buffer)
		{
			CloseDecompressor(decHandle);
			return false;
		}

		// Actual decompression
		size_t actual = 0;
		ok            = Decompress(decHandle, compressedData, compressedSize,
		                buffer, needed, &actual);
		if (!ok)
		{
			CloseDecompressor(decHandle);
			free(buffer);
			return false;
		}

		CloseDecompressor(decHandle);

		*outBuffer = buffer;
		*outSize   = actual;
		return true; // success
	}

	std::vector<uint8_t> EncryptBuffer(const std::vector<uint8_t>& decryptedVector)
	{
		std::vector<uint8_t> tmp = decryptedVector;

		auto     beginBuffer  = tmp.data();
		auto     endBuffer    = beginBuffer + tmp.size();
		uint8_t* pCurrentByte = tmp.data();
		size_t   moduleSize   = endBuffer - beginBuffer;

		if (moduleSize >= 2)
		{
			*pCurrentByte += pCurrentByte[1];

			for (size_t i = 0; i < moduleSize - 2; ++i)
				pCurrentByte[i] -= -3 * i - pCurrentByte[i + 1];

			endBuffer[-1] -= 3 - 3 * (LOBYTE(endBuffer[-1]) - LOBYTE(beginBuffer[0]));
		}

		pCurrentByte[0]        = 0xA7;
		tmp.at(moduleSize - 1) = 0xfd;
		tmp.at(moduleSize - 2) = 0xfa;

		std::cout << "[+] Encrypted buffer (" << moduleSize << " bytes)" << std::endl;
		return tmp;
	}
}
