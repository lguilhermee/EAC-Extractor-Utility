#include <Windows.h>
#include <iostream>
#include "CryptoUtils.h"
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

		std::cout << "[+] Decrypted payload (" << moduleSize << " bytes)" << std::endl;
		return tmp;
	}

	std::vector<uint8_t> UnpackModule(const std::vector<uint8_t>& encryptedVector)
	{
		// Decrypt (chain cipher) then inflate (raw deflate)
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
			std::cerr << "[!] Failed to inflate module" << std::endl;
			return {};
		}

		std::cout << "[+] Unpacked module: " << decrypted.size()
		          << " -> " << decompressedSize << " bytes" << std::endl;

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
			std::cerr << "[!] Failed to deflate module" << std::endl;
			return {};
		}

		std::cout << "[+] Packed module: " << plainVector.size()
		          << " -> " << compressedSize << " bytes" << std::endl;

		std::vector<uint8_t> result(
			static_cast<uint8_t*>(pCompressed),
			static_cast<uint8_t*>(pCompressed) + compressedSize
		);
		mz_free(pCompressed);

		// Step 2: Chain cipher encryption (exact inverse of DecryptPayload)
		//
		// DecryptPayload does (in order):
		//   1. buf[n-1] += (3 - 3*n)
		//   2. for i = n-2 downto 1:  buf[i] += (-3*i - buf[i+1])
		//   3. buf[0] -= buf[1]
		//
		// To encrypt, undo each step in reverse order:
		//   undo 3: buf[0] += buf[1]
		//   undo 2: for i = 1 to n-2:  buf[i] -= (-3*i - buf[i+1])
		//   undo 1: buf[n-1] -= (3 - 3*n)

		size_t   n = result.size();
		uint8_t* p = result.data();

		if (n >= 2)
		{
			p[0] += p[1];

			for (size_t i = 1; i < n - 1; ++i)
				p[i] -= -3 * i - p[i + 1];

			p[n - 1] -= 3 - 3 * n;
		}

		std::cout << "[+] Encrypted payload (" << n << " bytes)" << std::endl;
		return result;
	}

	std::vector<uint8_t> EncryptPayload(const std::vector<uint8_t>& decryptedVector)
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
