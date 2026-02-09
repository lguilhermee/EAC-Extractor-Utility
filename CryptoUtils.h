#pragma once
#include <vector>
#include <cstdint>

namespace CryptoUtils
{
	// Chain cipher only -- used for the EAC launcher payload.
	std::vector<uint8_t> DecryptPayload(const std::vector<uint8_t>& encryptedVector);

	// Chain cipher + raw deflate -- used for driver/usermode modules.
	std::vector<uint8_t> UnpackModule(const std::vector<uint8_t>& encryptedVector);
	std::vector<uint8_t> PackModule(const std::vector<uint8_t>& plainVector);
}
