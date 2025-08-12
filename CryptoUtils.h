#pragma once
#include <vector>
#include <cstdint>

namespace CryptoUtils
{
	std::vector<uint8_t> DecryptBuffer(const std::vector<uint8_t>& encryptedVector);
	std::vector<uint8_t> EncryptBuffer(const std::vector<uint8_t>& decryptedVector);
	bool                  DecompressBuffer(
		BYTE* compressedData,
		size_t      compressedSize,
		BYTE**      outBuffer,
		size_t*     outSize
	);
}
