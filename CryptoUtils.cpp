#include "CryptoUtils.h"
#include "Log.h"
#include "miniz.h"

namespace CryptoUtils
{
	std::vector<uint8_t> DecryptPayload(const std::vector<uint8_t>& encryptedVector)
	{
		std::vector<uint8_t> tmp = encryptedVector;

		auto     beginBuffer  = tmp.data();
		auto     endBuffer    = beginBuffer + tmp.size();
		uint8_t* pCurrentByte = tmp.data();
		size_t   moduleSize   = endBuffer - beginBuffer;

		if (moduleSize >= 2)
		{
			endBuffer[-1] += 3 - 3 * moduleSize;

			for (size_t i = moduleSize - 2; i; --i)
				pCurrentByte[i] += -3 * i - pCurrentByte[i + 1];

			*pCurrentByte -= pCurrentByte[1];
		}

		Log::Success("Decrypted payload (%zu bytes)", moduleSize);
		return tmp;
	}

	std::vector<uint8_t> UnpackModule(const std::vector<uint8_t>& encryptedVector)
	{
		std::vector<uint8_t> decrypted = DecryptPayload(encryptedVector);

		size_t decompressedSize = 0;
		void*  pDecompressed    = tinfl_decompress_mem_to_heap(
			decrypted.data(),
			decrypted.size(),
			&decompressedSize,
			0  // Raw deflate -- no zlib header
		);

		if (!pDecompressed)
		{
			Log::Error("Failed to inflate module");
			return {};
		}

		Log::Success("Unpacked module: %zu -> %zu bytes", decrypted.size(), decompressedSize);

		std::vector<uint8_t> result(
			static_cast<uint8_t*>(pDecompressed),
			static_cast<uint8_t*>(pDecompressed) + decompressedSize
		);

		mz_free(pDecompressed);
		return result;
	}

	std::vector<uint8_t> PackModule(const std::vector<uint8_t>& plainVector)
	{
		// Step 1: Raw deflate compression
		size_t compressedSize = 0;
		int    compFlags      = tdefl_create_comp_flags_from_zip_params(
			MZ_BEST_COMPRESSION, -MZ_DEFAULT_WINDOW_BITS, MZ_DEFAULT_STRATEGY);

		void* pCompressed = tdefl_compress_mem_to_heap(
			plainVector.data(),
			plainVector.size(),
			&compressedSize,
			compFlags
		);

		if (!pCompressed)
		{
			Log::Error("Failed to deflate module");
			return {};
		}

		Log::Success("Compressed module: %zu -> %zu bytes", plainVector.size(), compressedSize);

		std::vector<uint8_t> result(
			static_cast<uint8_t*>(pCompressed),
			static_cast<uint8_t*>(pCompressed) + compressedSize
		);
		mz_free(pCompressed);

		// Step 2: Chain cipher encryption (exact inverse of DecryptPayload)
		size_t   n = result.size();
		uint8_t* p = result.data();

		if (n >= 2)
		{
			p[0] += p[1];

			for (size_t i = 1; i < n - 1; ++i)
				p[i] -= -3 * i - p[i + 1];

			p[n - 1] -= 3 - 3 * n;
		}

		Log::Success("Encrypted payload (%zu bytes)", n);
		return result;
	}
}
