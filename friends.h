#ifndef FRIENDS_H
#define FRIENDS_H

#ifdef _WIN32
#pragma once
#endif

#include <vector>
#include <steamtypes.h>

//-----------------------------------------------------------------------------
// Macro defs
//-----------------------------------------------------------------------------

#define XOR_STEAMID(sid) ( sid ^ 0xBABEBEEADEAFC0CAuLL )
#define DEFINE_GOD( who, sid, ... ) { CGod( XOR_STEAMID( sid ), { __VA_ARGS__ } ) }

//-----------------------------------------------------------------------------
// God's structure
//-----------------------------------------------------------------------------

class CGod
{
public:
	explicit CGod( uint64 ullSteamID, std::vector<unsigned char> CpuIdHash );

	uint64 m_ullSteamID;
	std::vector<unsigned char> m_CpuIdHash;
};

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

extern std::vector<CGod> g_Gods;

#endif // FRIENDS_H