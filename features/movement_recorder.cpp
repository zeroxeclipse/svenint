// Movement Recorder

#include <dbg.h>
#include <convar.h>
#include <ISvenModAPI.h>
#include <IMemoryUtils.h>

#include "movement_recorder.h"
#include "../modules/server.h"

#include "../patterns.h"

//-----------------------------------------------------------------------------
// ConVars, ConCommands
//-----------------------------------------------------------------------------

static int state = 0;
static size_t curframe = 0;
static std::vector<MovementFrame> MR_Frames;
static MovementFrame frame;

static bool bRunningOriginalCmd = true;
static float flLastMove = 0.f;

CON_COMMAND( st_mr_record, "" )
{
	state = 1;

	MR_Frames.clear();
}

CON_COMMAND( st_mr_play, "" )
{
	flLastMove = gpGlobals->time;

	state = 2;
	curframe = 0;
}

CON_COMMAND( st_mr_stop, "" )
{
	state = 0;
}

//-----------------------------------------------------------------------------
// Hooks related
//-----------------------------------------------------------------------------

DECLARE_HOOK( void, __cdecl, SV_RunCmd, usercmd_t *, int );

DECLARE_HOOK( void, __cdecl, CmdStart, const edict_t *player, const struct usercmd_s *cmd, unsigned int random_seed );
DECLARE_HOOK( void, __cdecl, CmdEnd, const edict_t *player );

//-----------------------------------------------------------------------------
// Vars
//-----------------------------------------------------------------------------

CMovementRecorder g_MovementReader;

//-----------------------------------------------------------------------------
// Hooks
//-----------------------------------------------------------------------------

DECLARE_FUNC( void, __cdecl, HOOKED_SV_RunCmd, usercmd_t *cmd, int random_seed )
{
	if ( g_MovementReader.ShouldAbortRunCmd() )
		return;

	g_MovementReader.OnPreRunCmd( cmd, random_seed );

	ORIG_SV_RunCmd( cmd, random_seed );

	g_MovementReader.OnPostRunCmd( cmd, random_seed );
}

DECLARE_FUNC( void, __cdecl, HOOKED_CmdStart, const edict_t *pPlayer, const usercmd_t *cmd, unsigned int random_seed )
{
	g_MovementReader.OnCmdStart( const_cast<edict_t *>( pPlayer ), const_cast<usercmd_t *>( cmd ), random_seed );

	ORIG_CmdStart( pPlayer, cmd, random_seed );
}

DECLARE_FUNC( void, __cdecl, HOOKED_CmdEnd, const edict_t *pPlayer )
{
	ORIG_CmdEnd( pPlayer );

	g_MovementReader.OnCmdEnd( const_cast<edict_t *>( pPlayer ) );
}

//-----------------------------------------------------------------------------
// Movement Recorder
//-----------------------------------------------------------------------------

CMovementRecorder::CMovementRecorder( void )
{
	m_pfnSV_RunCmd = NULL;

	m_hSV_RunCmd = DETOUR_INVALID_HANDLE;
	m_hCmdStart = DETOUR_INVALID_HANDLE;
	m_hCmdEnd = DETOUR_INVALID_HANDLE;
}

void CMovementRecorder::OnPostRunCmd( void )
{
	//bRunningOriginalCmd = true;

	//if ( state == 2 )
	//{
	//	if ( curframe >= MR_Frames.size() )
	//	{
	//		state = 0;
	//		return;
	//	}

	//	edict_t *pPlayer = g_pServerEngineFuncs->pfnPEntityOfEntIndex( 1 );

	//	if ( pPlayer == NULL )
	//		return;

	//	bRunningOriginalCmd = false;

	//	pPlayer->v.fixangle = 1; // FAM_FORCEVIEWANGLES

	//	byte msec = int( ( gpGlobals->time - flLastMove ) * 1000.f );
	//	flLastMove = gpGlobals->time;

	//	g_pServerEngineFuncs->pfnRunPlayerMove( pPlayer,
	//											MR_Frames[ curframe ].viewangles,
	//											0.f, 0.f, 0.f,
	//											MR_Frames[ curframe ].buttons,
	//											MR_Frames[ curframe ].impulse,
	//											MR_Frames[ curframe ].msec );

	//	pPlayer->v.origin = MR_Frames[ curframe ].origin;
	//	pPlayer->v.angles = MR_Frames[ curframe ].viewangles;
	//	pPlayer->v.velocity = MR_Frames[ curframe ].velocity;

	//	bRunningOriginalCmd = true;

	//	curframe++;
	//}
	//else if ( state == 1 )
	//{
	//	if ( MR_Frames.size() > 0 )
	//	{
	//		edict_t *pPlayer = g_pServerEngineFuncs->pfnPEntityOfEntIndex( 1 );

	//		if ( pPlayer == NULL )
	//			return;

	//		MR_Frames.back().origin = pPlayer->v.origin;
	//		MR_Frames.back().velocity = pPlayer->v.velocity;
	//	}
	//}
}

bool CMovementRecorder::ShouldAbortRunCmd( void )
{
	edict_t *pPlayer = *m_pServerPlayer;

	if ( pPlayer != NULL && g_pServerEngineFuncs->pfnIndexOfEdict( pPlayer ) == 1 && state == 2 && bRunningOriginalCmd )
		return true;

	return false;
}

void CMovementRecorder::OnPreRunCmd( usercmd_t *cmd, int random_seed )
{
	edict_t *pPlayer = *m_pServerPlayer;

	if ( pPlayer == NULL )
		return;

	if ( g_pServerEngineFuncs->pfnIndexOfEdict( pPlayer ) == 1 )
	{
		if ( state == 1 )
		{
			frame.buttons = cmd->buttons;
			frame.impulse = cmd->impulse;
			frame.msec = cmd->msec;
		}
	}
}

void CMovementRecorder::OnPostRunCmd( usercmd_t *cmd, int random_seed )
{
	edict_t *pPlayer = *m_pServerPlayer;

	if ( pPlayer == NULL )
		return;

	if ( g_pServerEngineFuncs->pfnIndexOfEdict( pPlayer ) == 1 )
	{
		if ( state == 1 )
		{
			frame.origin = pPlayer->v.origin;
			frame.viewangles = cmd->viewangles;
			frame.velocity = pPlayer->v.velocity;
			//frame.buttons = pPlayer->v.button;
			//frame.impulse = pPlayer->v.impulse;

			MR_Frames.push_back( frame );
		}
	}
}

void CMovementRecorder::OnCmdStart( edict_t *pPlayer, usercmd_t *cmd, unsigned int random_seed )
{
	if ( g_pServerEngineFuncs->pfnIndexOfEdict( pPlayer ) != 1 )
		return;

	if ( state == 2 )
	{
		if ( curframe >= MR_Frames.size() )
		{
			state = 0;
			return;
		}

		cmd->viewangles = MR_Frames[ curframe ].viewangles;
		cmd->buttons |= MR_Frames[ curframe ].buttons;
		cmd->impulse = MR_Frames[ curframe ].impulse;

		pPlayer->v.v_angle = MR_Frames[ curframe ].viewangles;
		pPlayer->v.button |= MR_Frames[ curframe ].buttons;
		pPlayer->v.impulse = MR_Frames[ curframe ].impulse;
	}
}

void CMovementRecorder::OnCmdEnd( edict_t *pPlayer )
{
	if ( g_pServerEngineFuncs->pfnIndexOfEdict( pPlayer ) != 1 )
		return;

	if ( state == 1 )
	{
		frame.origin = pPlayer->v.origin;
		frame.viewangles = pPlayer->v.v_angle;
		frame.velocity = pPlayer->v.velocity;
		frame.buttons = pPlayer->v.button;
		frame.impulse = pPlayer->v.impulse;

		MR_Frames.push_back( frame );
	}
	else if ( state == 2 )
	{
		pPlayer->v.origin = MR_Frames[ curframe ].origin;
		pPlayer->v.v_angle = MR_Frames[ curframe ].viewangles;
		pPlayer->v.velocity = MR_Frames[ curframe ].velocity;

		curframe++;
	}
}

//-----------------------------------------------------------------------------
// Movement Recorder Feature
//-----------------------------------------------------------------------------

bool CMovementRecorder::Load( void )
{
	m_pfnSV_RunCmd = MemoryUtils()->FindPattern( SvenModAPI()->Modules()->Hardware, Patterns::Hardware::SV_RunCmd );

	if ( !m_pfnSV_RunCmd )
	{
		Warning( "Couldn't find function \"SV_RunCmd\"\n" );
		return false;
	}

	void *sv_player = MemoryUtils()->FindPattern( SvenModAPI()->Modules()->Hardware, Patterns::Hardware::sv_player );

	if ( !sv_player )
	{
		Warning( "Couldn't get \"sv_player\"\n" );
		return false;
	}

	m_pServerPlayer = reinterpret_cast<edict_t **>( *(unsigned long *)( (unsigned char *)sv_player + 2 ) );

	return true;
}

void CMovementRecorder::PostLoad( void )
{
	//m_hSV_RunCmd = DetoursAPI()->DetourFunction( m_pfnSV_RunCmd, HOOKED_SV_RunCmd, GET_FUNC_PTR( ORIG_SV_RunCmd ) );
	//m_hCmdStart = DetoursAPI()->DetourFunction( g_pServerFuncs->pfnCmdStart, HOOKED_CmdStart, GET_FUNC_PTR( ORIG_CmdStart ) );
	//m_hCmdEnd = DetoursAPI()->DetourFunction( g_pServerFuncs->pfnCmdEnd, HOOKED_CmdEnd, GET_FUNC_PTR( ORIG_CmdEnd ) );
}

void CMovementRecorder::Unload( void )
{
	//DetoursAPI()->RemoveDetour( m_hSV_RunCmd );
	//DetoursAPI()->RemoveDetour( m_hCmdStart );
	//DetoursAPI()->RemoveDetour( m_hCmdEnd );
}