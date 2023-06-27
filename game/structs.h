// Game's Reverse Engineered Structures and Offsets

#ifndef GAMESTRUCTS_H
#define GAMESTRUCTS_H

#ifdef _WIN32
#pragma once
#endif

#include <stdlib.h>
#include <IDetoursAPI.h>

#include <hl_sdk/engine/progdefs.h>

//-----------------------------------------------------------------------------
// Forward declarations
//-----------------------------------------------------------------------------

class CBaseEntity;
class CBasePlayer;
class CBaseMonster;

//-----------------------------------------------------------------------------
// Offsets to various classes, structures
//-----------------------------------------------------------------------------

namespace Offsets
{
	namespace BasePlayer
	{
		constexpr size_t IsAlive = 47;
		constexpr size_t BeginRevive = 88;
		constexpr size_t EndRevive = 89;
		constexpr size_t SpecialSpawn = 210;
		constexpr size_t IsConnected = 218;
	}
}

//-----------------------------------------------------------------------------
// Function signatures to various classes, procedures
//-----------------------------------------------------------------------------

namespace Signatures
{
	namespace BasePlayer
	{
		FUNC_SIGNATURE( bool, __thiscall, IsAlive, CBasePlayer * );
		FUNC_SIGNATURE( bool, __thiscall, IsConnected, CBasePlayer * );
	}
}

//-----------------------------------------------------------------------------
// Game Structures
//-----------------------------------------------------------------------------

class CBaseDeadPlayer
{
public:
	void		*vptr;
	entvars_t	*pev;

#ifdef _WIN32
	char		unknown[ 340 ];
#else // Linux
	char		unknown[ 356 ]; // 0x10 bytes
#endif

	edict_t		*m_pPlayer;
	int			m_iPlayerSerialNumber; // supposed to be...
};

class CSurvivalMode
{
public:
	void		*vptr;
	bool		m_bEnabledNow;
	char		unknown[ 7 ];
	bool		m_bEnabled;
};

#endif // GAMESTRUCTS_H