#include "hashes.h"
#include <dbg.h>

#include "../cryptopp/sha.h"
#include "../cryptopp/hex.h"
#include "../cryptopp/filters.h"

void Hashes::LoadHashInit(void* CSvenInternalInstance)
{
	CSvenInternalPtr = CSvenInternalInstance;

	LoadPtr = reinterpret_cast<void*>(reinterpret_cast<void**>(CSvenInternalPtr)[2]);
	PostLoadPtr = reinterpret_cast<void*>(reinterpret_cast<void**>(CSvenInternalPtr)[3]);

	LoadSize = reinterpret_cast<size_t>(PostLoadPtr) - reinterpret_cast<size_t>(LoadPtr);

	unsigned char* Bytes = reinterpret_cast<unsigned char*>(LoadPtr);

	CryptoPP::byte hash[CryptoPP::SHA256::DIGESTSIZE];

	CryptoPP::SHA256().CalculateDigest(hash, Bytes, LoadSize);

	for (size_t i = 0; i < 31; ++i) 
	{
		snprintf(&InitialLoadHash[i * 2], 3, "%02x", hash[i]);
	}

	Msg("Hash: %s\n", InitialLoadHash);
}

bool Hashes::CheckLoadHash()
{
	unsigned char* Bytes = reinterpret_cast<unsigned char*>(LoadPtr);

	CryptoPP::byte hash[CryptoPP::SHA256::DIGESTSIZE];

	CryptoPP::SHA256().CalculateDigest(hash, Bytes, LoadSize);

	char CurrentLoadHash[63];

	for (size_t i = 0; i < 31; ++i)
	{
		snprintf(&CurrentLoadHash[i * 2], 3, "%02x", hash[i]);
	}

	Msg("Initial Hash: %s\n", InitialLoadHash);

	Msg("\n");

	Msg("Current Hash: %s\n", CurrentLoadHash);

	return false;
}
