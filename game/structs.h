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
	namespace BaseEntity
	{
		constexpr size_t ObjectCaps = 9;
	}

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
	namespace BaseEntity
	{
		FUNC_SIGNATURE( int, __thiscall, ObjectCaps, CBaseEntity * );
	}

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

class CEngine
{
public:
	//void *vptr;
	virtual ~CEngine() {}

	virtual unsigned char Load( bool bDedicatedServer, char *pszBaseDir, char *pszCommandLine );
	virtual void Unload( void );

	virtual void SetState( int state );
	virtual int GetState( void );

	virtual void SetSubState( int state );
	virtual int GetSubState( void );

	virtual int Frame( void );

	virtual double GetFrameTime( void );
	virtual double GetCurTime( void );

	virtual void TrapKey_Event( int keyevent, bool down );
	virtual void TrapMouse_Event( int mouseevent, bool down );

	virtual void StartTrapMode( void );
	virtual int IsTrapping( void );
	virtual void CheckDoneTrapping( int *gameactive, int *unk_2_out );

	virtual int GetQuitting( void );
	virtual void SetQuitting( int state );

public:
	int quitting;
	int state;

	double curtime;
	double frametime;
	double oldtime;

	bool trapping;
	bool trapping_unk_appointment;
	char unk_1[ 2 ];

	int unk_2;

	int gameactive;
};

#endif // GAMESTRUCTS_H