#ifndef FRIENDS_H
#define FRIENDS_H

#ifdef _WIN32
#pragma once
#endif

#include <vector>
#include <steamtypes.h>

#define XOR_STEAMID(sid) ( sid ^ 0xBABEBEEADEAFC0CAuLL )

extern std::vector<uint64> g_Gods;
extern std::vector<std::vector<unsigned char>> g_CpuIDsHash;

#endif // FRIENDS_H