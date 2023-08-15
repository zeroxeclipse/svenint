// Misc

#include <ICvar.h>
#include <IMemoryUtils.h>
#include <ISvenModAPI.h>
#include <IInventory.h>
#include <IClient.h>
#include <IClientWeapon.h>

#include <convar.h>
#include <dbg.h>

#include <hl_sdk/cl_dll/hud.h>
#include <hl_sdk/cl_dll/in_defs.h>
#include <hl_sdk/common/entity_types.h>
#include <hl_sdk/common/com_model.h>
#include <hl_sdk/engine/APIProxy.h>
#include <netchan.h>

#include "misc.h"
#include "bsp.h"
#include "input_manager.h"
#include "speedrun_tools.h"

#include <algorithm>
#include <cmath>
#include <string>

#include "../features/strafer.h"
#include "../modules/patches.h"
#include "../game/utils.h"
#include "../game/draw_context.h"

#include "../config.h"
#include "../patterns.h"

#define USE_GAY_PERFECT_AUTOJUMP ( 0 )

//-----------------------------------------------------------------------------
// Imports
//-----------------------------------------------------------------------------

extern bool bSendPacket;

//-----------------------------------------------------------------------------
// Declare hooks
//-----------------------------------------------------------------------------

DECLARE_HOOK(BOOL, WINAPI, fQueryPerformanceCounter, LARGE_INTEGER *);
DECLARE_HOOK(void, __cdecl, fNetchan_Transmit, netchan_t *, int, unsigned char *);
DECLARE_CLASS_HOOK(void, CClient_SoundEngine__Play2DSound, void *, const char *, float);

//-----------------------------------------------------------------------------
// Vars
//-----------------------------------------------------------------------------

CMisc g_Misc;

cvar_t *ex_interp = NULL;
cvar_t *fps_max = NULL;

Vector g_vecSpinAngles(0.f, 0.f, 0.f);

static Strafe::StrafeData s_StickStrafeData;

int g_iPlayerSaveIndex = 0;
FILE *g_pPlayerSaveFile = NULL;

bool g_bDupeWeapon = false;
static bool s_bDupeWeaponPrev = false;
static int s_iDupeWeaponID = WEAPON_NONE;

bool g_bSpamKill = false;
static bool s_bSpamKillPrev = false;

static bool s_bSelfSink = false;
static bool s_bSelfSink2 = false;
//static int s_nSinkState = 0;
static int s_iWeaponID = -1;

static bool s_bFreeze = false;
static bool s_bFreeze2 = false;
bool g_bForceFreeze2 = false;

static bool s_bDucktap = false;
static bool s_bJumpbugLegit = false;

static float s_flTopColorDelay = 0.0f;
static float s_flBottomColorDelay = 0.0f;

static int s_iTopColorOffset = 0;
static int s_iBottomColorOffset = 0;

static float s_flWeaponOffset[32] =
{
	0.0f, // 0
	-1.5f, // 1
	-4.0f, // 2
	0.0f, // 3
	-2.5f, // 4
	0.0f, // 5
	-4.0f, // 6
	-5.0f, // 7
	-9.2f, // 8
	-3.0f, // 9
	-5.0f, // 10
	-8.0f, // 11
	-4.0f, // 12
	0.0f, // 13
	0.0f, // 14
	-3.0f, // 15
	0.0f, // 16
	-4.15f, // 17
	-0.7f, // 18
	0.0f, // 19
	-2.0f, // 20
	0.0f, // 21
	-9.0f, // 22
	-7.3f, // 23
	-4.25f, // 24
	0.0f, // 25
	-2.0f, // 26
	-2.7f, // 27
	0.0f, // 28
	-7.5f, // 29
	0.0f, // 30
	0.0f, // 31
};

//-----------------------------------------------------------------------------
// Common Functions
//-----------------------------------------------------------------------------

static float GetWeaponOffset(cl_entity_s *pViewModel)
{
	if ( ClientWeapon()->IsCustom() )
	{
		const char *pszModelName = pViewModel->model->name;

		if (pszModelName && *pszModelName)
		{
			const char *pszModelNameEnd = pszModelName + strlen(pszModelName);

			while (pszModelNameEnd > pszModelName)
			{
				if (*(pszModelNameEnd - 1) == '/')
				{
					pszModelName = pszModelNameEnd;
					break;
				}

				--pszModelNameEnd;
			}

			if ( !strcmp(pszModelName, "v_357.mdl") )
			{
				return -6.2f;
			}
		}
	}
	else
	{
		int iWeaponID = Client()->GetCurrentWeaponID();
		constexpr int iMaxWeapons = (sizeof(s_flWeaponOffset) / sizeof(s_flWeaponOffset[0]));

		//Assert( iWeaponID >= 0 && iWeaponID < iMaxWeapons );

		//iWeaponID = (int)clamp(iWeaponID, 0, iMaxWeapons - 1);

		if ( iWeaponID >= 0 && iWeaponID < iMaxWeapons )
			return s_flWeaponOffset[ iWeaponID ];
		else
			return 0.0f;
	}

	return 0.0f;
}

//-----------------------------------------------------------------------------
// Commands, CVars..
//-----------------------------------------------------------------------------

extern ConVar sc_st_legit_mode_ignore_freeze;

ConVar sc_ducktap_adjust_fps( "sc_ducktap_adjust_fps", "0", FCVAR_CLIENTDLL, "Change fps to the given value when ducktapping" );
ConVar sc_jumpbug_adjust_fps( "sc_jumpbug_adjust_fps", "1000", FCVAR_CLIENTDLL, "Change fps to the given value to perform jumpbug in legit mode" );
ConVar sc_jumpbug_min_fall_velocity( "sc_jumpbug_min_fall_velocity", "580", FCVAR_CLIENTDLL, "Minimal fall velocity for auto jumpbug" );

ConVar sc_app_speed( "sc_app_speed", "1", FCVAR_CLIENTDLL, "Speed of application", true, 0.1f, false, FLT_MAX );
ConVar sc_speedhack( "sc_speedhack", "1", FCVAR_CLIENTDLL, "sc_speedhack <value> - Set speedhack value", true, 0.f, false, FLT_MAX );
ConVar sc_speedhack_ltfx( "sc_speedhack_ltfx", "0", FCVAR_CLIENTDLL, "sc_speedhack_ltfx <value> - Set LTFX speedhack value; 0 - disable, value < 0 - slower, value > 0 - faster", true, -100.f, false, FLT_MAX );

ConVar sc_stick( "sc_stick", "0", FCVAR_CLIENTDLL, "Player's index to stick" );
ConVar sc_stick_strafe( "sc_stick_strafe", "0", FCVAR_CLIENTDLL, "Use strafer when stick is enabled" );
ConVar sc_stick_auto( "sc_stick_auto", "0", FCVAR_CLIENTDLL, "Automatically stick to nearest player" );
ConVar sc_stick_steal_model( "sc_stick_steal_model", "0", FCVAR_CLIENTDLL, "Steal model of player to stick" );

CON_COMMAND(sc_test, "Retrieve an entity's info")
{
	if (args.ArgC() > 1)
	{
		int index = atoi(args[1]);

		cl_entity_s *pEntity = g_pEngineFuncs->GetEntityByIndex(index);

		if (pEntity)
		{
			Msg("Entity Pointer: %X\n", pEntity);

			if (pEntity->player)
			{
				Msg("Player Info Pointer: %X\n", g_pEngineStudio->PlayerInfo(index - 1));

				hud_player_info_t playerInfo;
				ZeroMemory(&playerInfo, sizeof(hud_player_info_s));

				g_pEngineFuncs->GetPlayerInfo(index, &playerInfo);

				if (playerInfo.name && playerInfo.model && *playerInfo.model)
					Msg("Model: %s\n", playerInfo.model);

				Msg("Top Color: %d\n", playerInfo.topcolor);
				Msg("Bottom Color: %d\n", playerInfo.bottomcolor);
			}
			else if (pEntity->model && pEntity->model->name)
			{
				Msg("Model: %s\n", pEntity->model->name);
			}
		}
	}
	else
	{
		ConMsg("Usage:  sc_test <entindex>\n");
	}
}

CON_COMMAND(sc_trace_test, "")
{
	bool bDucked = ( Client()->GetFlags() & FL_DUCKING );

	pmtrace_t *tr = g_pEngineFuncs->PM_TraceLine( g_pPlayerMove->origin,
												  g_pPlayerMove->origin - Vector( 0, 0, 72 ),
												  PM_TRACELINE_PHYSENTSONLY,
												  bDucked ? PM_HULL_DUCKED_PLAYER : PM_HULL_PLAYER,
												  -1 );

	DrawBox( g_pPlayerMove->origin,
			 bDucked ? VEC_DUCK_HULL_MIN : VEC_HULL_MIN,
			 bDucked ? VEC_DUCK_HULL_MAX : VEC_HULL_MAX,
			 1.f,
			 1.f,
			 1.f,
			 0.5f,
			 4.f,
			 true,
			 10.f );

	DrawBox( tr->endpos,
			 bDucked ? VEC_DUCK_HULL_MIN : VEC_HULL_MIN,
			 bDucked ? VEC_DUCK_HULL_MAX : VEC_HULL_MAX,
			 0.f,
			 1.f,
			 0.f,
			 0.5f,
			 4.f,
			 true,
			 10.f );

	Msg( "frac: %f\n", tr->fraction );
}

CON_COMMAND(sc_autojump, "Toggle autojump")
{
	bool bValue;

	if ( args.ArgC() > 1 )
	{
		bValue = !!atoi( args[ 1 ] );
	}
	else
	{
		bValue = !g_Config.cvars.autojump;
	}

#if USE_GAY_PERFECT_AUTOJUMP
	extern bool g_bJumpDown;
#endif

	Msg(bValue ? "Auto Jump enabled\n" : "Auto Jump disabled\n");
	g_Config.cvars.autojump = bValue;

	Utils()->PrintChatText("<SvenInt> Autojump is %s\n", bValue ? "ON" : "OFF");

#if USE_GAY_PERFECT_AUTOJUMP
	g_bJumpDown = false;
#endif
}

CON_COMMAND(sc_autojump_legacy, "Toggle autojump")
{
	extern bool g_bJumpDown;

	bool bValue;

	if ( args.ArgC() > 1 )
	{
		bValue = !!atoi( args[ 1 ] );
	}
	else
	{
		bValue = !g_Config.cvars.autojump_legacy;
	}

	Msg( bValue ? "Auto Jump Legacy enabled\n" : "Auto Jump Legacy disabled\n");
	g_Config.cvars.autojump_legacy = bValue;

	Utils()->PrintChatText("<SvenInt> Autojump Legacy is %s\n", bValue ? "ON" : "OFF");

#if USE_GAY_PERFECT_AUTOJUMP
	g_bJumpDown = false;
#endif
}

CON_COMMAND_NO_WRAPPER(sc_ducktap, "Toggle ducktapping")
{
	Msg(g_Config.cvars.ducktap ? "Ducktap disabled\n" : "Ducktap enabled\n");
	g_Config.cvars.ducktap = !g_Config.cvars.ducktap;

	Utils()->PrintChatText("<SvenInt> Auto ducktap is %s\n", g_Config.cvars.ducktap ? "ON" : "OFF");
	
	if ( g_Config.cvars.ducktap && Client()->IsOnGround() )
		g_pEngineFuncs->ClientCmd("+duck;wait;-duck");
}

CON_COMMAND(sc_jumpbug, "Toggle jumpbug")
{
	bool bValue;

	if ( args.ArgC() > 1 )
	{
		bValue = !!atoi( args[ 1 ] );
	}
	else
	{
		bValue = !g_Config.cvars.jumpbug;
	}

	Msg( bValue ? "Jump Bug enabled\n" : "Jump Bug disabled\n");
	g_Config.cvars.jumpbug = bValue;

	Utils()->PrintChatText("<SvenInt> Auto jumpbug is %s\n", bValue ? "ON" : "OFF");
}

CON_COMMAND(sc_jumpbug_legit, "Toggle legit jumpbug that adjusts your FPS")
{
	bool bValue;

	if ( args.ArgC() > 1 )
	{
		bValue = !!atoi( args[ 1 ] );
	}
	else
	{
		bValue = !s_bJumpbugLegit;
	}

	Msg( bValue ? "Legit Jump Bug enabled\n" : "Legit Jump Bug disabled\n");
	s_bJumpbugLegit = bValue;

	Utils()->PrintChatText("<SvenInt> Legit Auto jumpbug is %s\n", bValue ? "ON" : "OFF");
}

CON_COMMAND_NO_WRAPPER(sc_edgejump, "Toggle edgejump")
{
	Msg(g_Config.cvars.edgejump ? "Edge Jump disabled\n" : "Edge Jump enabled\n");
	g_Config.cvars.edgejump = !g_Config.cvars.edgejump;

	Utils()->PrintChatText("<SvenInt> Auto edgejump is %s\n", g_Config.cvars.edgejump ? "ON" : "OFF");
}

CON_COMMAND_NO_WRAPPER(sc_fakelag, "Toggle fake lag")
{
	Msg(g_Config.cvars.fakelag ? "Fake Lag disabled\n" : "Fake Lag enabled\n");
	g_Config.cvars.fakelag = !g_Config.cvars.fakelag;

	Utils()->PrintChatText("<SvenInt> Fake lag is %s\n", g_Config.cvars.fakelag ? "ON" : "OFF");
}

CON_COMMAND_NO_WRAPPER(sc_fastrun, "Toggle fast run")
{
	Msg(g_Config.cvars.fastrun ? "Fast Run disabled\n" : "Fast Run enabled\n");
	g_Config.cvars.fastrun = !g_Config.cvars.fastrun;

	Utils()->PrintChatText("<SvenInt> Fast run is %s\n", g_Config.cvars.fastrun ? "ON" : "OFF");
}

CON_COMMAND_NO_WRAPPER(sc_auto_ceil_clipping, "Automatically suicide when you touch a ceil to perform clipping")
{
	Msg(g_Config.cvars.auto_ceil_clipping ? "Auto Ceil-Clipping disabled\n" : "Auto Ceil-Clipping enabled\n");
	g_Config.cvars.auto_ceil_clipping = !g_Config.cvars.auto_ceil_clipping;

	Utils()->PrintChatText("<SvenInt> Auto ceil-clipping is %s\n", g_Config.cvars.auto_ceil_clipping ? "ON" : "OFF");
}

CON_COMMAND_EXTERN_NO_WRAPPER(sc_rotate_dead_body, ConCommand_RotateDeadBody, "Toggle rotate dead body")
{
	Msg(g_Config.cvars.rotate_dead_body ? "Rotate Dead Body disabled\n" : "Rotate Dead Body enabled\n");
	g_Config.cvars.rotate_dead_body = !g_Config.cvars.rotate_dead_body;
}

CON_COMMAND_EXTERN_NO_WRAPPER(sc_tertiary_attack_glitch, ConCommand_TertiaryAttackGlitch, "Toggle tertiary attack glitch")
{
	Msg(g_Config.cvars.tertiary_attack_glitch ? "Tertiary Attack Glitch disabled\n" : "Tertiary Attack Glitch enabled\n");
	g_Config.cvars.tertiary_attack_glitch = !g_Config.cvars.tertiary_attack_glitch;
}

CON_COMMAND_EXTERN_NO_WRAPPER(sc_quake_guns, ConCommand_QuakeGuns, "Toggle Quake guns")
{
	Msg(g_Config.cvars.quake_guns ? "Quake Guns disabled\n" : "Quake Guns enabled\n");
	g_Config.cvars.quake_guns = !g_Config.cvars.quake_guns;
}

CON_COMMAND_EXTERN_NO_WRAPPER(sc_selfsink, ConCommand_AutoSelfSink, "Perform self sink")
{
	if ( Client()->IsDead() )
		return;

	s_bSelfSink = true;
}

CON_COMMAND_EXTERN_NO_WRAPPER(sc_selfsink2, ConCommand_AutoSelfSink2, "Perform self sink, method #2")
{
	if ( Client()->IsDead() )
		return;

	s_bSelfSink2 = true;
}

CON_COMMAND_EXTERN_NO_WRAPPER(sc_reset_colors, ConCommand_ResetColors, "Reset colors in Color Pulsator")
{
	s_iBottomColorOffset = s_iTopColorOffset;
}

CON_COMMAND_EXTERN_NO_WRAPPER(sc_sync_colors, ConCommand_SyncColors, "Sync. change time for top and bottom colors in Color Pulsator")
{
	if (s_flTopColorDelay > s_flBottomColorDelay)
		s_flBottomColorDelay = s_flTopColorDelay;

	if (s_flBottomColorDelay > s_flTopColorDelay)
		s_flTopColorDelay = s_flBottomColorDelay;
}

CON_COMMAND_EXTERN_NO_WRAPPER(sc_drop_empty_weapon, ConCommand_DropEmptyWeapon, "Drop an empty weapon from your inventory")
{
	for (int i = 0; i < Inventory()->GetMaxWeaponSlots(); i++)
	{
		for (int j = 0; j < Inventory()->GetMaxWeaponPositions(); j++)
		{
			WEAPON *pWeapon = Inventory()->GetWeapon(i, j);

			if (pWeapon && !Inventory()->HasAmmo(pWeapon))
			{
				Inventory()->DropWeapon(pWeapon);
				return;
			}
		}
	}
}

CON_COMMAND_EXTERN_NO_WRAPPER(sc_first_person_roaming, ConCommand_FirstPersonRoaming, "Toggle first-person roaming")
{
	Msg(g_Config.cvars.fp_roaming ? "First-Person Roaming disabled\n" : "First-Person Roaming enabled\n");
	g_Config.cvars.fp_roaming = !g_Config.cvars.fp_roaming;
}

CON_COMMAND_NO_WRAPPER(sc_auto_wallstrafing, "Automatically strafe using walls")
{
	Msg(g_Config.cvars.auto_wallstrafing ? "Auto Wallstrafing disabled\n" : "Auto Wallstrafing enabled\n");
	g_Config.cvars.auto_wallstrafing = !g_Config.cvars.auto_wallstrafing;

	Utils()->PrintChatText("<SvenInt> Auto wallstrafing is %s\n", g_Config.cvars.auto_wallstrafing ? "ON" : "OFF");
}

CON_COMMAND_NO_WRAPPER(sc_spam_kill, "Spam gibme")
{
	Msg(g_bSpamKill ? "Spam Kill disabled\n" : "Spam Kill enabled\n");
	g_bSpamKill = !g_bSpamKill;
}

CON_COMMAND_NO_WRAPPER(sc_dupe_weapon, "Dupes a weapon that you're holding in your hands")
{
	Msg(g_bDupeWeapon ? "Dupe Weapon disabled\n" : "Dupe Weapon enabled\n");
	g_bDupeWeapon = !g_bDupeWeapon;
}

CON_COMMAND_NO_WRAPPER(sc_one_tick_exploit, "Exploits an action on one tick")
{
	Msg(g_Config.cvars.one_tick_exploit ? "One Tick Exploit disabled\n" : "One Tick Exploit enabled\n");
	g_Config.cvars.one_tick_exploit = !g_Config.cvars.one_tick_exploit;

	Utils()->PrintChatText("<SvenInt> One tick exploit is %s\n", g_Config.cvars.one_tick_exploit ? "ON" : "OFF");
}

CON_COMMAND_NO_WRAPPER(sc_fastcrowbar, "Toggle fast crowbar")
{
	Msg(g_Config.cvars.fast_crowbar ? "Fast Crowbar disabled\n" : "Fast Crowbar enabled\n");
	g_Config.cvars.fast_crowbar = !g_Config.cvars.fast_crowbar;

	Utils()->PrintChatText("<SvenInt> Fast crowbar is %s\n", g_Config.cvars.fast_crowbar ? "ON" : "OFF");
}

CON_COMMAND_NO_WRAPPER(sc_fastcrowbar2, "Toggle fast crowbar [auto freeze]")
{
	Msg(g_Config.cvars.fast_crowbar2 ? "Fast Crowbar #2 disabled\n" : "Fast Crowbar #2 enabled\n");
	g_Config.cvars.fast_crowbar2 = !g_Config.cvars.fast_crowbar2;

	Utils()->PrintChatText("<SvenInt> Fast crowbar #2 is %s\n", g_Config.cvars.fast_crowbar2 ? "ON" : "OFF");
}

CON_COMMAND_NO_WRAPPER(sc_fastmedkit, "Toggle fast medkit")
{
	Msg(g_Config.cvars.fast_medkit ? "Fast Medkit disabled\n" : "Fast Medkit enabled\n");
	g_Config.cvars.fast_medkit = !g_Config.cvars.fast_medkit;

	Utils()->PrintChatText("<SvenInt> Fast medkit is %s\n", g_Config.cvars.fast_medkit ? "ON" : "OFF");
}

CON_COMMAND_EXTERN_NO_WRAPPER(sc_freeze, ConCommand_Freeze, "Block connection with a server")
{
	if ( g_SpeedrunTools.IsLegitMode() && !sc_st_legit_mode_ignore_freeze.GetBool() && Host_IsServerActive() )
		return;

	Msg(s_bFreeze ? "Connection restored\n" : "Connection blocked\n");
	s_bFreeze = !s_bFreeze;

	Utils()->PrintChatText("<SvenInt> Freeze is %s\n", s_bFreeze ? "ON" : "OFF");
}

CON_COMMAND_EXTERN_NO_WRAPPER(sc_freeze2, ConCommand_Freeze2, "Block connection with a server with 2nd method")
{
	if ( g_SpeedrunTools.IsLegitMode() && !sc_st_legit_mode_ignore_freeze.GetBool() && Host_IsServerActive() )
		return;

	Msg(s_bFreeze2 ? "Connection restored\n" : "Connection blocked\n");
	s_bFreeze2 = !s_bFreeze2;

	Utils()->PrintChatText("<SvenInt> Freeze #2 is %s\n", s_bFreeze2 ? "ON" : "OFF");
}

CON_COMMAND_NO_WRAPPER(sc_print_skybox_name, "sc_print_skybox_name - Prints current skybox name")
{
	if (g_pPlayerMove && g_pPlayerMove->movevars)
	{
		Msg("Skybox: %s\n", g_pPlayerMove->movevars->skyName);
	}
}

CON_COMMAND_NO_WRAPPER(sc_print_steamids, "sc_print_steamids - Print Steam64 IDs of players")
{
	for (int i = 1; i <= MAXCLIENTS; i++)
	{
		player_info_t *pPlayerInfo = g_pEngineStudio->PlayerInfo(i - 1);

		if ( pPlayerInfo && pPlayerInfo->name && pPlayerInfo->name[0] )
		{
			Msg("%d. %s - %llu\n", i, pPlayerInfo->name, pPlayerInfo->m_nSteamID);
		}
	}
}

CON_COMMAND(sc_register_on_tick_command, "sc_register_on_tick_command - Register a command that will be called each tick")
{
	if (args.ArgC() >= 3)
	{
		const char *pszAlias = args[1];
		const char *pszCommand = args[2];

		if (*pszAlias == 0)
		{
			Msg("No alias\n");
			return;
		}
		
		if (*pszCommand == 0)
		{
			Msg("No command\n");
			return;
		}

		if (strlen(pszAlias) > 32)
		{
			Msg("Alias' name is too long!\n");
			return;
		}

		std::string sAlias = pszAlias;
		std::string sCommand = pszCommand;

		g_Misc.m_OnTickCommands.insert_or_assign( sAlias, sCommand );

		Msg("On tick command with alias \"%s\" was registered/replaced\n", pszAlias);
	}
	else
	{
		Msg("Usage:  sc_register_on_tick_command <alias> <command>\n");
	}
}

CON_COMMAND(sc_remove_on_tick_command, "sc_remove_on_tick_command - Remove a command that called each tick")
{
	if (args.ArgC() >= 2)
	{
		const char *pszAlias = args[1];

		if (*pszAlias == 0)
		{
			Msg("No alias\n");
			return;
		}
		
		if (strlen(pszAlias) > 32)
		{
			Msg("Alias' name is too long!\n");
			return;
		}

		std::string sAlias = pszAlias;

		auto found = g_Misc.m_OnTickCommands.find( sAlias );

		if ( found != g_Misc.m_OnTickCommands.end() )
		{
			if ( g_Misc.m_OnTickCommands.erase( sAlias ) == 1 )
			{
				Msg("On tick command with alias \"%s\" was removed\n", pszAlias);
			}
			else
			{
				Msg("Failed to remove on tick command with given alias\n");
			}
		}
		else
		{
			Msg("On tick command with alias \"%s\" isn't registered\n", pszAlias);
		}
	}
	else
	{
		Msg("Usage:  sc_remove_on_tick_command <alias>\n");
	}
}

CON_COMMAND_NO_WRAPPER(sc_print_on_tick_commands, "sc_print_on_tick_commands - Prints all on tick commands")
{
	Msg("[Alias = Command]\n");

	for (const std::pair<std::string, std::string> &pair : g_Misc.m_OnTickCommands)
	{
		Msg("%s = \"%s\"\n", pair.first.c_str(), pair.second.c_str());
	}
}

CON_COMMAND(sc_player_stats_record, "Record stats about a specified player by their index")
{
	if (args.ArgC() > 1)
	{
		int index = atoi(args[1]);

		if ( g_pPlayerSaveFile != NULL )
		{
			Msg("Changed save of client stats #%d to client #%d\n", g_iPlayerSaveIndex, index);
		}
		else
		{
			g_pPlayerSaveFile = fopen("sven_internal/player_stats.txt", "w");

			if ( g_pPlayerSaveFile == NULL )
			{
				Msg("Failed to create file \"sven_internal/player_stats.txt\"\n");
				return;
			}

			Msg("Started saving stats of client #%d\n", index);
		}

		g_iPlayerSaveIndex = index;
	}
	else
	{
		Msg("Usage:  sc_save_player_stats <playerIndex>\n");
	}
}

CON_COMMAND(sc_player_stats_save, "Save stats about a specified player")
{
	if ( g_pPlayerSaveFile != NULL )
	{
		Msg("Saved stats of players in file \"sven_internal/player_stats.txt\" (last client #%d)\n", g_iPlayerSaveIndex);
		fclose( g_pPlayerSaveFile );
	}
	else
	{
		Msg("Nothing was recorded yet\n");
	}

	g_iPlayerSaveIndex = 0;
	g_pPlayerSaveFile = NULL;
}

static void freeze_toggle_key_down()
{
	Msg("Connection blocked\n");
	s_bFreeze = true;
}

static void freeze_toggle_key_up()
{
	Msg("Connection restored\n");
	s_bFreeze = false;
}

static void freeze2_toggle_key_down()
{
	Msg("Connection blocked\n");
	s_bFreeze2 = true;
}

static void freeze2_toggle_key_up()
{
	Msg("Connection restored\n");
	s_bFreeze2 = false;
}

static void ducktap_toggle_key_down()
{
	Msg("Auto ducktap enabled\n");
	//Utils()->PrintChatText("<SvenInt> Auto ducktap is ON\n");

	if ( Client()->IsOnGround() )
		g_pEngineFuncs->ClientCmd("+duck;wait;-duck");

	s_bDucktap = true;
}

static void ducktap_toggle_key_up()
{
	Msg("Auto ducktap disabled\n");
	//Utils()->PrintChatText("<SvenInt> Auto ducktap is OFF\n");

	s_bDucktap = false;
}

static ConCommand input_command__sc_freeze_toggle("+sc_freeze_toggle", freeze_toggle_key_down, "Freeze input");
static ConCommand output_command__sc_freeze_toggle("-sc_freeze_toggle", freeze_toggle_key_up, "Freeze output");

static ConCommand input_command__sc_freeze2_toggle("+sc_freeze2_toggle", freeze2_toggle_key_down, "Freeze #2 input");
static ConCommand output_command__sc_freeze2_toggle("-sc_freeze2_toggle", freeze2_toggle_key_up, "Freeze #2 output");

static ConCommand input_command__sc_ducktap_toggle("+sc_ducktap", ducktap_toggle_key_down, "Auto ducktap input");
static ConCommand output_command__sc_ducktap_toggle("-sc_ducktap", ducktap_toggle_key_up, "Auto ducktap output");

//-----------------------------------------------------------------------------
// IN_JUMP hook
//-----------------------------------------------------------------------------

#if USE_GAY_PERFECT_AUTOJUMP
bool g_bJumpDown = false;

CommandCallbackFn ORIG_JumpDown = NULL;
CommandCallbackFn ORIG_JumpUp = NULL;

static DetourHandle_t hJumpDown = DETOUR_INVALID_HANDLE;
static DetourHandle_t hJumpUp = DETOUR_INVALID_HANDLE;

static void HOOKED_JumpDown()
{
	//Utils()->PrintChatText( "+jump\n" );

	g_bJumpDown = true;

	if ( !g_Config.cvars.autojump || Client()->IsDead() )
		ORIG_JumpDown();
}

static void HOOKED_JumpUp()
{
	//Utils()->PrintChatText( "-jump\n" );

	g_bJumpDown = false;

	if ( !g_Config.cvars.autojump || Client()->IsDead() )
		ORIG_JumpUp();
}
#endif

//-----------------------------------------------------------------------------
// Hooks
//-----------------------------------------------------------------------------

DECLARE_FUNC(BOOL, WINAPI, HOOKED_fQueryPerformanceCounter, LARGE_INTEGER *lpPerformanceCount)
{
	static LONGLONG oldfakevalue = 0;
	static LONGLONG oldrealvalue = 0;

	LONGLONG newvalue;

	if (oldfakevalue == 0 || oldrealvalue == 0)
	{
		oldfakevalue = lpPerformanceCount->QuadPart;
		oldrealvalue = lpPerformanceCount->QuadPart;
	}

	BOOL result = ORIG_fQueryPerformanceCounter(lpPerformanceCount);

	newvalue = lpPerformanceCount->QuadPart;
	newvalue = oldfakevalue + (LONGLONG)((newvalue - oldrealvalue) * static_cast<double>(sc_app_speed.GetFloat()));

	oldrealvalue = lpPerformanceCount->QuadPart;
	oldfakevalue = newvalue;

	lpPerformanceCount->QuadPart = newvalue;

	return result;
}

DECLARE_FUNC(void, __cdecl, HOOKED_fNetchan_Transmit, netchan_t *chan, int lengthInBytes, unsigned char *data)
{
	//g_pNetChan = chan;

	if (s_bFreeze2 || g_bForceFreeze2)
	{
		ORIG_fNetchan_Transmit(chan, 0, NULL); // cancel size and data
		return;
	}

	ORIG_fNetchan_Transmit(chan, lengthInBytes, data);
}

DECLARE_CLASS_FUNC(void, HOOKED_CClient_SoundEngine__Play2DSound, void *thisptr, const char *pszFilename, float flVolume)
{
	if ( !strcmp(pszFilename, "misc/talk.wav") )
	{
		pszFilename = "sven_internal/talk.wav";
	}

	ORIG_CClient_SoundEngine__Play2DSound(thisptr, pszFilename, flVolume);
}

//-----------------------------------------------------------------------------
// Callbacks
//-----------------------------------------------------------------------------

static bool airrun = false;

CON_COMMAND(sc_airrun, "")
{
	airrun = !airrun;
	Msg(airrun ? "Air Run enabled\n" : "Air Run disabled\n");
}

static void AirRun(struct usercmd_s *cmd)
{
	if ( airrun && !Client()->IsSpectating() && Client()->GetMoveType() == MOVETYPE_WALK )
	{
		cmd->buttons |= (1 << 22); // IN_BULLRUSH

		if ( cmd->buttons & IN_JUMP )
		{
			cmd->buttons |= IN_DUCK;

			if ( cmd->buttons & IN_DUCK && !Client()->IsOnGround() )
				cmd->buttons &= ~IN_DUCK;
		};
	}
}

void CMisc::CreateMove(float frametime, struct usercmd_s *cmd, int active)
{
	// Execute on tick commands
	for (const std::pair<std::string, std::string> &pair : m_OnTickCommands)
	{
		g_pEngineFuncs->ClientCmd( pair.second.c_str() );
	}

	// Clamp speedhack values
	sc_app_speed.Clamp();
	sc_speedhack.Clamp();
	sc_speedhack_ltfx.Clamp();

	// Set speedhack
	UTIL_SetGameSpeed( static_cast<double>(sc_speedhack.GetFloat()) );
	*dbRealtime += static_cast<double>(sc_speedhack_ltfx.GetFloat()) * frametime;

	m_bSpinnerDelayed = false;

	WeaponConfig();
	SavePlayerStats();

	FakeLag(frametime);
	ColorPulsator();
	TertiaryAttackGlitch();

	if ( g_Config.cvars.rotate_dead_body && Client()->IsDying() )
	{
		Vector va;

		g_pEngineFuncs->GetViewAngles(va);
		cmd->viewangles = va;
	}

	if ( !Client()->IsSpectating() )
	{
		AutoSelfSink(cmd);
		AutoCeilClipping(cmd);
		AutoJump(cmd);
		AutoJumpLegacy(cmd);
		JumpBug(frametime, cmd);
		JumpBugLegit(frametime, cmd);
		EdgeJump(frametime, cmd);
		Ducktap(cmd);
		FastRun(cmd);
		Spinner_Wrapper(cmd);
		TriggerPushExploit(cmd);
		AutoWallstrafing(cmd);
		AutoReload(cmd);
		Stick(cmd);
		LookAt(cmd);
		OneTickExploit(cmd);
		SpamKill(cmd);
		DupeWeapon(cmd);
		AirRun(cmd);
	}

	if ( s_bFreeze )
		bSendPacket = false;
}

void CMisc::V_CalcRefdef(struct ref_params_s *pparams)
{
	QuakeGuns_V_CalcRefdef();

	if (m_bSpinCanChangePitch)
	{
		cl_entity_t *pLocal = g_pEngineFuncs->GetLocalPlayer();

		pLocal->angles.x = m_flSpinPitchAngle;
		pLocal->curstate.angles.x = m_flSpinPitchAngle;
		pLocal->prevstate.angles.x = m_flSpinPitchAngle;
		pLocal->latched.prevangles.x = m_flSpinPitchAngle;
	}
}

void CMisc::HUD_PostRunCmd(struct local_state_s *from, struct local_state_s *to, struct usercmd_s *cmd, int runfuncs, double time, unsigned int random_seed)
{
	// For proper work of revive boost info
	if ( s_bFreeze2 )
	{
		Vector vecOrigin = Client()->GetClientData()->origin;

		g_pPlayerMove->origin = vecOrigin;
		g_pEngineFuncs->GetLocalPlayer()->origin = vecOrigin;
		g_pEngineFuncs->GetLocalPlayer()->curstate.origin = vecOrigin;
		from->playerstate.origin = vecOrigin;
		to->playerstate.origin = vecOrigin;
	}

	QuakeGuns_HUD_PostRunCmd(to);
}

static int line_beamindex = 0;

void CMisc::OnAddEntityPost(int is_visible, int type, struct cl_entity_s *ent, const char *modelname)
{
	if ( g_Config.cvars.show_players_push_direction )
	{
		if ( is_visible && type == ET_PLAYER && ent->index != Client()->GetPlayerIndex() )
		{
			Vector vecEnd;
			Vector vecEnd2;

			Vector vecBegin = ent->origin;

			Vector vecPushDir(1.f, 0.f, 0.f);
			Vector vecPushDir2(0.f, 1.f, 0.f);

			if ( ent->curstate.usehull )
				vecBegin[2] += VEC_DUCK_HULL_MIN.z + 1.5f;
			else
				vecBegin[2] += VEC_HULL_MIN.z + 1.5f;

			vecEnd = vecBegin + vecPushDir * g_Config.cvars.push_direction_length;
			vecEnd2 = vecBegin + vecPushDir2 * g_Config.cvars.push_direction_length * (1.f / 3.f);

			if ( !line_beamindex )
				line_beamindex = g_pEngineFuncs->pEventAPI->EV_FindModelIndex("sprites/laserbeam.spr");

			// Opposite direction
			g_pEngineFuncs->pEfxAPI->R_BeamPoints(vecBegin,
												  vecEnd,
												  line_beamindex,
												  0.001f, // life time
												  g_Config.cvars.push_direction_width,
												  0.f, // amplitude
												  32.f, // brightness
												  2.f, // speed
												  0, // startFrame
												  0.f, // framerate
												  g_Config.cvars.push_direction_color[0],
												  g_Config.cvars.push_direction_color[1],
												  g_Config.cvars.push_direction_color[2]);

			// 90 deg. opposite direction that also lets you to push a player
			g_pEngineFuncs->pEfxAPI->R_BeamPoints(vecBegin,
												  vecEnd2,
												  line_beamindex,
												  0.001f, // life time
												  g_Config.cvars.push_direction_width,
												  0.f, // amplitude
												  32.f, // brightness
												  2.f, // speed
												  0, // startFrame
												  0.f, // framerate
												  g_Config.cvars.push_direction_color[0],
												  g_Config.cvars.push_direction_color[1],
												  g_Config.cvars.push_direction_color[2]);
		}
	}

	if ( g_Config.cvars.show_players_sight_direction )
	{
		if ( is_visible && type == ET_PLAYER && ent->index != Client()->GetPlayerIndex() )
		{
			Vector vecEyes, vecEnd, vecForward, vecAngles;

			vecAngles = ent->angles;
			vecAngles.x *= -3.f;

			AngleVectors( vecAngles, &vecForward, NULL, NULL );

			vecEyes = ent->origin + Vector(0.f, 0.f, ent->curstate.usehull ? 12.5f : 28.5f /* VEC_DUCK_VIEW.z : VEC_VIEW.z */);
			vecEnd = vecEyes + vecForward * g_Config.cvars.players_sight_direction_length;

			if ( !line_beamindex )
				line_beamindex = g_pEventAPI->EV_FindModelIndex("sprites/laserbeam.spr");

			g_pEffectsAPI->R_BeamPoints(vecEyes,
										vecEnd,
										line_beamindex,
										0.001f, // life time
										g_Config.cvars.players_sight_direction_width,
										0.f, // amplitude
										32.f, // brightness
										2.f, // speed
										0, // startFrame
										0.f, // framerate
										g_Config.cvars.players_sight_direction_color[0],
										g_Config.cvars.players_sight_direction_color[1],
										g_Config.cvars.players_sight_direction_color[2]);
		}
	}
}

void CMisc::OnVideoInit()
{
	line_beamindex = 0;
	m_iLastWeaponID = 0;

	g_pEngineFuncs->ClientCmd( "exec \"sven_internal/cfg/weapon_none.cfg\"" );
}

//-----------------------------------------------------------------------------
// Freeze getters
//-----------------------------------------------------------------------------

bool CMisc::IsFreezeOn( void ) const
{
	return s_bFreeze;
}

bool CMisc::IsFreeze2On( void ) const
{
	return s_bFreeze2;
}

//-----------------------------------------------------------------------------
// Auto execution of weapon configs
//-----------------------------------------------------------------------------

void CMisc::WeaponConfig(void)
{
	static const char *s_szWeaponNames[] =
	{
		"weapon_none.cfg",
		"weapon_crowbar.cfg",
		"weapon_glock.cfg",
		"weapon_python.cfg",
		"weapon_mp5.cfg",
		"weapon_chaingun.cfg",
		"weapon_crossbow.cfg",
		"weapon_shotgun.cfg",
		"weapon_rpg.cfg",
		"weapon_gauss.cfg",
		"weapon_egon.cfg",
		"weapon_hornetgun.cfg",
		"weapon_handgrenade.cfg",
		"weapon_tripmine.cfg",
		"weapon_satchel.cfg",
		"weapon_snark.cfg",
		"weapon_unk16.cfg", // not defined
		"weapon_uzi.cfg",
		"weapon_medkit.cfg",
		"weapon_crowbar_electric.cfg",
		"weapon_pipewrench.cfg",
		"weapon_minigun.cfg",
		"weapon_grapple.cfg",
		"weapon_sniperrifle.cfg",
		"weapon_m249.cfg",
		"weapon_m16.cfg",
		"weapon_sporelauncher.cfg",
		"weapon_desert_eagle.cfg",
		"weapon_shockrifle.cfg",
		"weapon_displacer.cfg",
	};

	int iWeaponID = Client()->GetCurrentWeaponID();

	if ( iWeaponID != m_iLastWeaponID && g_Config.cvars.weapon_configs && iWeaponID >= WEAPON_NONE && iWeaponID <= WEAPON_DISPLACER )
	{
		char command_buffer[64];
		snprintf(command_buffer, M_ARRAYSIZE(command_buffer), "exec \"sven_internal/cfg/%s\"", s_szWeaponNames[iWeaponID]);
		command_buffer[M_ARRAYSIZE(command_buffer) - 1] = 0;

		g_pEngineFuncs->ClientCmd( command_buffer );
	}

	m_iLastWeaponID = iWeaponID;
}

//-----------------------------------------------------------------------------
// Save player stats
//-----------------------------------------------------------------------------

void CMisc::SavePlayerStats(void)
{
	if ( g_pPlayerSaveFile != NULL )
	{
		if ( g_iPlayerSaveIndex > 0 && g_iPlayerSaveIndex <= MAXCLIENTS )
		{
			cl_entity_t *pLocal = g_pEngineFuncs->GetLocalPlayer();

			cl_entity_t *pPlayer = g_pEngineFuncs->GetEntityByIndex( g_iPlayerSaveIndex );
			player_info_t *pPlayerInfo = g_pEngineStudio->PlayerInfo( g_iPlayerSaveIndex - 1 );

			if ( pPlayer != NULL && pPlayerInfo != NULL && pPlayer->curstate.messagenum >= pLocal->curstate.messagenum )
			{
				Vector vecAngles = pPlayer->curstate.angles;
				vecAngles.x *= (89.0f / 9.8876953125f);

				Vector vecVelocity = pPlayer->prevstate.origin - pPlayer->curstate.origin;

				fprintf( g_pPlayerSaveFile,
						"<%d> <Name: %s> <Origin: %.3f %.3f %.3f> <View Angles: %.3f %.3f %.3f> <Velocity: %.3f %.3f %.3f> <Duck: %d> <Model: %s (%d %d)> <Steam64 ID: %llu> <User Info: %s>\n",
						g_iPlayerSaveIndex,
						pPlayerInfo->name,
						VectorExpand(pPlayer->curstate.origin),
						VectorExpand(vecAngles),
						VectorExpand(vecVelocity),
						pPlayer->curstate.usehull,
						pPlayerInfo->model,
						pPlayerInfo->topcolor,
						pPlayerInfo->bottomcolor,
						pPlayerInfo->m_nSteamID,
						pPlayerInfo->userinfo );
			}
		}
		else
		{
			Assert( g_iPlayerSaveIndex > 0 && g_iPlayerSaveIndex <= MAXCLIENTS );
			fclose( g_pPlayerSaveFile );

			g_iPlayerSaveIndex = 0;
			g_pPlayerSaveFile = NULL;
		}
	}
}

//-----------------------------------------------------------------------------
// One tick exploit
//-----------------------------------------------------------------------------

void CMisc::OneTickExploit(struct usercmd_s *cmd)
{
	if ( !Client()->IsDead() )
	{
		bool bDoRapidAction = false;

		if ( g_Config.cvars.fast_crowbar )
		{
			cl_entity_t *pViewModel = NULL;

			if ( Client()->GetCurrentWeaponID() == WEAPON_CROWBAR ||
				Client()->GetCurrentWeaponID() == WEAPON_WRENCH ||
				( (pViewModel = g_pEngineFuncs->GetViewModel()) && pViewModel->model && strstr(pViewModel->model->name, "crowbar.mdl") ) )
			{
				if ( cmd->buttons & IN_ATTACK )
				{
					bDoRapidAction = true;
				}
			}
		}
		
		if ( g_Config.cvars.fast_medkit )
		{
			if ( Client()->GetCurrentWeaponID() == WEAPON_MEDKIT )
			{
				if ( cmd->buttons & (IN_ATTACK | IN_ATTACK2) )
				{
					bDoRapidAction = true;
				}
			}
		}

		if ( g_Config.cvars.fast_crowbar2 )
		{
			if ( Client()->GetCurrentWeaponID() == WEAPON_CROWBAR || Client()->GetCurrentWeaponID() == WEAPON_WRENCH )
			{
				if ( cmd->buttons & IN_ATTACK )
				{
					bDoRapidAction = true;
					g_bForceFreeze2 = true;
				}
				else if ( Client()->ButtonLast() & IN_ATTACK )
				{
					bDoRapidAction = true;
					g_bForceFreeze2 = false;
				}
			}
			else
			{
				g_bForceFreeze2 = false;
			}
		}
		else
		{
			g_bForceFreeze2 = false;
		}

		if (bDoRapidAction)
		{
			if (m_iFakeLagCounter < 45)
			{
				bSendPacket = false;
				m_iFakeLagCounter++;
			}
			else
			{
				bSendPacket = true;
				m_iFakeLagCounter = 0;
			}

			UTIL_SetGameSpeed(20000.f);

			cmd->forwardmove = 0.f;
			cmd->sidemove = 0.f;
		}
	}
	else
	{
		m_iFakeLagCounter = 0;
	}

	if ( g_Config.cvars.one_tick_exploit )
	{
		if ( m_iOneTickExploitLagInterval < g_Config.cvars.one_tick_exploit_lag_interval )
		{
			bSendPacket = false;
			m_iOneTickExploitLagInterval++;
		}
		else
		{
			bSendPacket = true;
			m_iOneTickExploitLagInterval = 0;
		}

		UTIL_SetGameSpeed( g_Config.cvars.one_tick_exploit_speedhack );
	}
	else
	{
		m_iOneTickExploitLagInterval = 0;
	}
}

//-----------------------------------------------------------------------------
// Spam Kill
//-----------------------------------------------------------------------------

void CMisc::SpamKill(struct usercmd_s *cmd)
{
	if ( g_bSpamKill )
	{
		g_Config.cvars.one_tick_exploit = true;

		if ( Client()->IsDead() )
		{
			cmd->buttons |= IN_JUMP;
		}

		g_pEngineFuncs->ClientCmd("gibme\n");
	}
	else if (s_bSpamKillPrev)
	{
		g_Config.cvars.one_tick_exploit = false;
	}

	s_bSpamKillPrev = g_bSpamKill;
}

//-----------------------------------------------------------------------------
// Dupe Weapon
//-----------------------------------------------------------------------------

void CMisc::DupeWeapon(struct usercmd_s *cmd)
{
	if ( g_bDupeWeapon )
	{
		if ( !s_bDupeWeaponPrev )
		{
			s_iDupeWeaponID = Client()->GetCurrentWeaponID();

			if (s_iDupeWeaponID == WEAPON_NONE)
			{
				Msg("You don't have any weapon to dupe\n");
				//m_iFakeLagCounter2 = 0;
				g_bDupeWeapon = false;
				g_Config.cvars.one_tick_exploit = false;

				return;
			}
		}

		cmd->forwardmove = 0.f;
		cmd->sidemove = 0.f;

		if ( Client()->IsDead() )
		{
			cmd->buttons |= IN_JUMP;

			//if (m_iFakeLagCounter2 < 45)
			//{
			//	bSendPacket = false;
			//	m_iFakeLagCounter2++;
			//}
			//else
			//{
			//	bSendPacket = true;
			//	m_iFakeLagCounter2 = 0;
			//}

			//UTIL_SetGameSpeed(20000.f);

			g_Config.cvars.one_tick_exploit = true;
		}
		else
		{
			if ( Client()->GetCurrentWeaponID() != s_iDupeWeaponID )
			{
				bool bFound = false;
				WEAPON *pWeapon = NULL;

				for (int i = 0; i < Inventory()->GetMaxWeaponSlots(); i++)
				{
					for (int j = 0; j < Inventory()->GetMaxWeaponPositions(); j++)
					{
						if ( pWeapon = Inventory()->GetWeapon(i, j) )
						{
							if ( pWeapon->iId == s_iDupeWeaponID )
							{
								if ( Inventory()->HasAmmo(pWeapon) )
									Inventory()->SelectWeapon( pWeapon );

								bFound = true;
								break;
							}
						}
					}

					if (bFound)
						break;
				}
			}
			else
			{
				g_pEngineFuncs->ClientCmd("gibme\n");
			}

			//UTIL_SetGameSpeed(1.f);
			bSendPacket = true;
			m_iOneTickExploitLagInterval = 0;
			//m_iFakeLagCounter2 = 0;
			//g_Config.cvars.one_tick_exploit = false;
		}
	}
	else if (s_bDupeWeaponPrev)
	{
		//m_iFakeLagCounter2 = 0;
		g_Config.cvars.one_tick_exploit = false;
	}

	s_bDupeWeaponPrev = g_bDupeWeapon;
}

//-----------------------------------------------------------------------------
// Auto Jump
//-----------------------------------------------------------------------------

ConVar sc_autojump_inconsistent( "sc_autojump_inconsistent", "0", FCVAR_CLIENTDLL );

void CMisc::AutoJump(struct usercmd_s *cmd)
{
	if ( g_InputManager.IsPlayingback() )
		return;

#if USE_GAY_PERFECT_AUTOJUMP
	if ( g_Config.cvars.autojump && g_bJumpDown && !Client()->IsDead() )
	{
		// Do jump if we're not dead, standing on ground or swimming
		if ( g_pPlayerMove->movetype != MOVETYPE_WALK || g_pPlayerMove->onground != -1 || g_pPlayerMove->waterlevel >= WL_WAIST )
		{
			cmd->buttons |= IN_JUMP;
		}
		else if ( g_pPlayerMove->onground == -1 && g_pPlayerMove->movetype == MOVETYPE_WALK )
		{
			pmtrace_t *pTrace = g_pEngineFuncs->PM_TraceLine( g_pPlayerMove->origin,
															  g_pPlayerMove->origin - Vector( 0.f, 0.f, 8192.f ),
															  PM_NORMAL,
															  ( Client()->GetFlags() & FL_DUCKING ) ? PM_HULL_DUCKED_PLAYER : PM_HULL_PLAYER /* g_pPlayerMove->usehull */,
															  -1 );

			if ( pTrace->fraction == 0.f )
			{
				cmd->buttons |= IN_JUMP;
			}
		}
	}
#else
	if ( g_Config.cvars.autojump && ( cmd->buttons & IN_JUMP ) && g_pPlayerMove->movetype == MOVETYPE_WALK && !Client()->IsDead() && g_pPlayerMove->movevars != NULL )
	{
		if ( g_pPlayerMove->waterlevel >= WL_WAIST )
			return;

		if ( g_pPlayerMove->onground == -1 )
		{
			if ( !sc_autojump_inconsistent.GetBool() )
			{
				Vector vecVelocity = g_pPlayerMove->velocity;

				UTIL_AddCorrectGravity( vecVelocity, g_pPlayerMove->frametime );

				float step = 2 * vecVelocity.z * g_pPlayerMove->frametime;

				pmtrace_t *pTrace = g_pEngineFuncs->PM_TraceLine( g_pPlayerMove->origin,
																  g_pPlayerMove->origin - Vector( 0.f, 0.f, step ),
																  PM_NORMAL,
																  ( Client()->GetFlags() & FL_DUCKING ) ? PM_HULL_DUCKED_PLAYER : PM_HULL_PLAYER /* g_pPlayerMove->usehull */,
																  -1 );

				if ( pTrace->fraction != 0.f )
				{
					cmd->buttons &= ~IN_JUMP;
				}
			}
			else if ( g_pPlayerMove->flFallVelocity != 0.f ) // ground friction still applies when standing on an entity
			{
				cmd->buttons &= ~IN_JUMP;
			}
		}
		else if ( g_pPlayerMove->oldbuttons & IN_JUMP )
		{
			cmd->buttons &= ~IN_JUMP;
		}
	}
#endif
}

void CMisc::AutoJumpLegacy(struct usercmd_s *cmd)
{
	static bool bAllowJump = false;

	if ( g_Config.cvars.autojump_legacy && cmd->buttons & IN_JUMP )
	{
		if ( bAllowJump && GetAsyncKeyState( VK_SPACE ) )
		{
			cmd->buttons &= ~IN_JUMP;
			bAllowJump = false;
		}
		else
		{
			bAllowJump = true;
		}
	}
}

//-----------------------------------------------------------------------------
// Auto Jumpbug
//-----------------------------------------------------------------------------

void CMisc::JumpBug(float frametime, struct usercmd_s *cmd)
{
	static int nJumpBugState = 0;

	if ( g_Config.cvars.jumpbug &&
		 g_pPlayerMove->flFallVelocity > sc_jumpbug_min_fall_velocity.GetFloat() /* PLAYER_MAX_SAFE_FALL_SPEED */ &&
		 g_pPlayerMove->movevars != NULL /*&& ( !g_InputManager.IsInAction() || g_InputManager.IsRecording() )*/ )
	{
		int placeHolder, contents;

		Vector vecPredictVelocity = Client()->GetVelocity() * frametime;

		vecPredictVelocity.z = 0.f; // 2D only, height will be predicted separately

		Vector vecPredictOrigin = g_pPlayerMove->origin + vecPredictVelocity;
		Vector vBottomOrigin = vecPredictOrigin;

		vBottomOrigin.z -= 8192.0f;

		pmtrace_t *pTrace = g_pEngineFuncs->PM_TraceLine( vecPredictOrigin,
														  vBottomOrigin,
														  PM_TRACELINE_PHYSENTSONLY,
														  ( Client()->GetFlags() & FL_DUCKING ) ? PM_HULL_DUCKED_PLAYER : PM_HULL_PLAYER /* g_pPlayerMove->usehull */,
														  -1);

		float flHeight = fabsf(pTrace->endpos.z - vecPredictOrigin.z);
		float flGroundNormalAngle = acos(pTrace->plane.normal.z);

		if ( flGroundNormalAngle <= acosf(0.7f) && Client()->GetWaterLevel() == WL_NOT_IN_WATER )
		{
			contents = g_pPlayerMove->PM_PointContents( pTrace->endpos, &placeHolder );

			if ( contents <= CONTENTS_WATER && contents > CONTENTS_TRANSLUCENT )
				return;

		#if 0
			bool bInAir = true;
			bool bDucking = ( g_pPlayerMove->flags & FL_DUCKING );
			int oldhull = g_pPlayerMove->usehull;

			cmd->buttons |= IN_DUCK;
			cmd->buttons &= ~IN_JUMP;

			if ( g_pPlayerMove->bInDuck || bDucking )
			{
				Vector newOrigin = g_pPlayerMove->origin;

				g_pPlayerMove->usehull = bDucking ? PM_HULL_DUCKED_PLAYER : PM_HULL_PLAYER;
				pmtrace_t tr = g_pPlayerMove->PM_PlayerTrace( newOrigin, newOrigin, PM_NORMAL, -1 );

				if ( !tr.startsolid )
				{
					g_pPlayerMove->usehull = PM_HULL_PLAYER;
					tr = g_pPlayerMove->PM_PlayerTrace( newOrigin, newOrigin, PM_NORMAL, -1 );

					if ( !tr.startsolid )
					{
						bDucking = false;

						// Check water. If we're under water, return here.

						Vector point = newOrigin;

						// Pick a spot just above the players feet.
						point[ 0 ] += ( bDucking ? ( VEC_DUCK_HULL_MIN[ 0 ] + VEC_DUCK_HULL_MAX[ 0 ] ) : ( VEC_HULL_MIN[ 0 ] + VEC_HULL_MAX[ 0 ] ) ) * 0.5f;
						point[ 1 ] += ( bDucking ? ( VEC_DUCK_HULL_MIN[ 1 ] + VEC_DUCK_HULL_MAX[ 1 ] ) : ( VEC_HULL_MIN[ 1 ] + VEC_HULL_MAX[ 1 ] ) ) * 0.5f;
						point[ 2 ] += ( ( bDucking ? VEC_DUCK_HULL_MIN[ 2 ] : VEC_HULL_MIN[ 2 ] ) + 1 );

						contents = g_pPlayerMove->PM_PointContents( point, &placeHolder );

						if ( !( contents <= CONTENTS_WATER && contents > CONTENTS_TRANSLUCENT ) )
						{
							// Check ground.
							if ( g_pPlayerMove->velocity[ 2 ] <= 180 )
							{
								point = newOrigin;
								point[ 2 ] -= 2;

								g_pPlayerMove->usehull = bDucking ? PM_HULL_DUCKED_PLAYER : PM_HULL_PLAYER;
								tr = g_pPlayerMove->PM_PlayerTrace( newOrigin, point, PM_NORMAL, -1 );

								if ( !( tr.plane.normal[ 2 ] < 0.7 || tr.ent == -1 ) )
								{
									//if ( !tr.StartSolid && !tr.AllSolid )
									//	VecCopy<float, 3>( tr.EndPos, player.Origin );

									bInAir = false;
								}
							}
						}
					}
				}
			}

			if ( !bInAir && !bDucking )
			{
				cmd->buttons |= IN_JUMP;
				cmd->buttons &= ~IN_DUCK;
			}

			g_pPlayerMove->usehull = oldhull;
		#else
			float flFrameZDist = fabsf( (Client()->GetFallVelocity() + (g_pPlayerMove->movevars->gravity * frametime)) * frametime );

			cmd->buttons |= IN_DUCK;
			cmd->buttons &= ~IN_JUMP;

			switch (nJumpBugState)
			{
			case 1:
				cmd->buttons &= ~IN_DUCK;
				cmd->buttons |= IN_JUMP;

				nJumpBugState = 2;
				break;

			case 2:
				nJumpBugState = 0;
				break;

			default:
				if (flFrameZDist > 0.f && fabsf(flHeight - flFrameZDist * 1.5f) <= 20.f)
				{
					float flNeedSpeed = fabsf(flHeight - 19.f);
					float flScale = fabsf(flNeedSpeed / flFrameZDist);

					UTIL_SetGameSpeed(flScale);

					nJumpBugState = 1;
				}
				break;
			}
		#endif
		}
		else
		{
			nJumpBugState = 0;
		}
	}
	else
	{
		nJumpBugState = 0;
	}
}

//-----------------------------------------------------------------------------
// Legit Auto Jumpbug
//-----------------------------------------------------------------------------

static void inline JB_ReturnFps(bool &bMustReturnFps, int iPreviousFps, char *pszFpsExecuteBuffer, int size)
{
	CVar()->SetValue( fps_max, iPreviousFps );

	// Record fps change
	if ( g_InputManager.IsRecording() )
	{
		snprintf( pszFpsExecuteBuffer, size, "fps_max %d", iPreviousFps );
		g_InputManager.RecordCommand( pszFpsExecuteBuffer );
	}

	bMustReturnFps = false;
}

static void JB_Predict( bool &bInAir, bool &bDucking, Vector &vecOrigin, Vector &vecVelocity)
{
	pmtrace_t tr;
	int contents, placeHolder;

	// Predict jumpbug
	if ( g_pPlayerMove->bInDuck || bDucking )
	{
		Vector newOrigin = vecOrigin;

		g_pPlayerMove->usehull = bDucking ? PM_HULL_DUCKED_PLAYER : PM_HULL_PLAYER;
		tr = g_pPlayerMove->PM_PlayerTrace( newOrigin, newOrigin, PM_NORMAL, -1 );

		if ( !tr.startsolid )
		{
			g_pPlayerMove->usehull = PM_HULL_PLAYER;
			tr = g_pPlayerMove->PM_PlayerTrace( newOrigin, newOrigin, PM_NORMAL, -1 );

			if ( !tr.startsolid )
			{
				bDucking = false;

				// Check water. If we're under water, return here.

				Vector point = newOrigin;

				// Pick a spot just above the players feet.
				point[ 0 ] += ( bDucking ? ( VEC_DUCK_HULL_MIN[ 0 ] + VEC_DUCK_HULL_MAX[ 0 ] ) : ( VEC_HULL_MIN[ 0 ] + VEC_HULL_MAX[ 0 ] ) ) * 0.5f;
				point[ 1 ] += ( bDucking ? ( VEC_DUCK_HULL_MIN[ 1 ] + VEC_DUCK_HULL_MAX[ 1 ] ) : ( VEC_HULL_MIN[ 1 ] + VEC_HULL_MAX[ 1 ] ) ) * 0.5f;
				point[ 2 ] += ( ( bDucking ? VEC_DUCK_HULL_MIN[ 2 ] : VEC_HULL_MIN[ 2 ] ) + 1 );

				contents = g_pPlayerMove->PM_PointContents( point, &placeHolder );

				if ( !( contents <= CONTENTS_WATER && contents > CONTENTS_TRANSLUCENT ) )
				{
					// Check ground.
					if ( vecVelocity[ 2 ] <= 180 )
					{
						point = newOrigin;
						point[ 2 ] -= 2;

						g_pPlayerMove->usehull = bDucking ? PM_HULL_DUCKED_PLAYER : PM_HULL_PLAYER;
						tr = g_pPlayerMove->PM_PlayerTrace( newOrigin, point, PM_NORMAL, -1 );

						if ( !( tr.plane.normal[ 2 ] < 0.7 || tr.ent == -1 ) )
						{
							//if ( !tr.StartSolid && !tr.AllSolid )
							//	VecCopy<float, 3>( tr.EndPos, player.Origin );

							bInAir = false;
						}
					}
				}
			}
		}
	}
}

void CMisc::JumpBugLegit(float frametime, struct usercmd_s *cmd)
{
	//static char fps_buffer[ 32 ];
	//static int fps_prev = 200;
	//static bool must_return_fps = false;

	if ( s_bJumpbugLegit &&
		 g_pPlayerMove->flFallVelocity > sc_jumpbug_min_fall_velocity.GetFloat() /* PLAYER_MAX_SAFE_FALL_SPEED */ &&
		 g_pPlayerMove->movevars != NULL /*&& ( !g_InputManager.IsInAction() || g_InputManager.IsRecording() )*/ )
	{
		pmtrace_t tr;
		Vector vecOrigin, vecMove, vecVelocity, vecPredictOrigin, vecBottom;
		int placeHolder, contents;

		const int oldhull = g_pPlayerMove->usehull;

		vecOrigin = g_pPlayerMove->origin;
		vecVelocity = g_pPlayerMove->velocity;

		// Apply gravity
		UTIL_AddCorrectGravity( vecVelocity, frametime );

		// Step
		vecMove = vecVelocity * frametime;

		// Trace forward
		g_pPlayerMove->usehull = ( g_pPlayerMove->flags & FL_DUCKING ) ? PM_HULL_DUCKED_PLAYER : PM_HULL_PLAYER;

		tr = g_pPlayerMove->PM_PlayerTrace( vecOrigin, vecOrigin + vecMove, PM_NORMAL, -1 );

		vecPredictOrigin = tr.endpos;
		vecBottom = vecPredictOrigin - Vector( 0.f, 0.f, 8192.f );

		// Trace down
		tr = g_pPlayerMove->PM_PlayerTrace( vecPredictOrigin, vecBottom, PM_NORMAL, -1 );

		g_pPlayerMove->usehull = oldhull;

		if ( acos( tr.plane.normal.z ) <= acos( 0.7f ) && g_pPlayerMove->waterlevel == WL_NOT_IN_WATER )
		{
			contents = g_pPlayerMove->PM_PointContents( tr.endpos, &placeHolder );

			// In water, don't jumpbug
			if ( contents <= CONTENTS_WATER && contents > CONTENTS_TRANSLUCENT )
			{
				//if ( must_return_fps )
				//	JB_ReturnFps( must_return_fps, fps_prev, fps_buffer, M_ARRAYSIZE( fps_buffer ) );

				return;
			}

			bool bInAir = true;
			bool bDucking = ( g_pPlayerMove->flags & FL_DUCKING );

			cmd->buttons |= IN_DUCK;
			cmd->buttons &= ~IN_JUMP;

			// Predict jumpbug
			JB_Predict( bInAir, bDucking, g_pPlayerMove->origin, g_pPlayerMove->velocity );

			//Utils()->PrintChatText( "bInAir: %d | bDucking: %d | onground: %d", bInAir, bDucking, Client()->IsOnGround() );

			//// Still ducking, impossible to jumpbug! Need to adjust our FPS.
			//if ( bInAir && !bDucking )
			//{
			//	bool bPredictInAir = true;
			//	bool bPredictDucking = ( g_pPlayerMove->flags & FL_DUCKING );

			//	JB_Predict( bPredictInAir, bPredictDucking, vecPredictOrigin, vecVelocity );

			//	if ( bPredictInAir && bPredictDucking )
			//	{
			//		if ( !must_return_fps )
			//		{
			//			fps_prev = int( fps_max->value );

			//			CVar()->SetValue( fps_max, sc_jumpbug_adjust_fps.GetInt() );

			//			// Record fps change
			//			if ( g_InputManager.IsRecording() )
			//			{
			//				snprintf( fps_buffer, M_ARRAYSIZE( fps_buffer ), "fps_max %d", sc_jumpbug_adjust_fps.GetInt() );
			//				g_InputManager.RecordCommand( fps_buffer );
			//			}

			//			must_return_fps = true;

			//			//Utils()->PrintChatText( "Can't jumpbug!" );
			//		}
			//	}
			//}
			//// Can jumpbug
			//else
			if ( !bInAir && !bDucking )
			{
				cmd->buttons |= IN_JUMP;
				cmd->buttons &= ~IN_DUCK;
			}

			g_pPlayerMove->usehull = oldhull;
			return;
		}
	}

	//if ( must_return_fps )
	//	JB_ReturnFps( must_return_fps, fps_prev, fps_buffer, M_ARRAYSIZE( fps_buffer ) );
}

//-----------------------------------------------------------------------------
// Auto Edgejump
//-----------------------------------------------------------------------------

void CMisc::EdgeJump(float frametime, struct usercmd_s *cmd)
{
	if ( g_Config.cvars.edgejump )
	{
		if ( Client()->IsOnGround() )
		{
			Vector vecPredictVelocity = Client()->GetVelocity() * frametime;
			Vector vecPredictOrigin = Client()->GetOrigin() + vecPredictVelocity;

			Vector vecEnd = vecPredictOrigin;
			vecEnd.z -= 3.f;

			pmtrace_t *pTrace = g_pEngineFuncs->PM_TraceLine(vecPredictOrigin,
															 vecEnd,
															 PM_NORMAL,
															 (Client()->GetFlags() & FL_DUCKING) ? 1 : 0,
															 -1);

			if (pTrace->fraction == 1.f)
			{
				cmd->buttons |= IN_JUMP;
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Auto Ducktap
//-----------------------------------------------------------------------------

void CMisc::Ducktap(struct usercmd_s *cmd)
{
	//if (g_Config.cvars.doubleduck && GetAsyncKeyState(VK_LCONTROL))
	//{
	//	static bool bForceUnduck = false;

	//	if ( bForceUnduck )
	//	{
	//		cmd->buttons &= ~IN_DUCK;

	//		bForceUnduck = false;
	//	}
	//	else if ( Client()->IsOnGround() )
	//	{
	//		cmd->buttons |= IN_DUCK;

	//		bForceUnduck = true;
	//	}
	//}

	static char fps_buffer[ 64 ];
	static int onground_prev = 0;
	static int fps_prev = 200;
	static bool must_return_fps = false;

	// Change fps back to normal
	if ( must_return_fps )
	{
		CVar()->SetValue( fps_max, fps_prev );

		// Record fps change
		if ( g_InputManager.IsRecording() )
		{
			// shit hack
			if ( g_InputManager.GetInputContext().FrameCounter() - 1 >= 0 )
			{
				snprintf( fps_buffer, M_ARRAYSIZE( fps_buffer ), "fps_max %d\n", fps_prev );

				std::vector<im_frame_t> &frames = g_InputManager.GetInputContext().Frames();
				int prevframe = g_InputManager.GetInputContext().FrameCounter() - 1;

				if ( frames[ prevframe ].commands != NULL )
				{
					std::string sCommandsBuffer = frames[ prevframe ].commands;
					sCommandsBuffer += fps_buffer;

					free( (void *)( frames[ prevframe ].commands ) );

					frames[ prevframe ].commands = strdup( sCommandsBuffer.c_str() );
				}
				else
				{
					frames[ prevframe ].commands = strdup( fps_buffer );
				}
			}
		}

		must_return_fps = false;
	}

	if ( s_bDucktap || g_Config.cvars.ducktap )
	{
		// No friction
		if ( sc_ducktap_adjust_fps.GetInt() > 20 && Host_IsServerActive() && !g_pPlayerMove->bInDuck )
		{
			if ( g_pPlayerMove->onground == -1 && g_pPlayerMove->velocity.z < 0.f )
			{
				pmtrace_t tr;

				bool bPredictedOnGround = false;

				const int oldhull = g_pPlayerMove->usehull;
				float flFrametime = g_pPlayerMove->frametime;
				Vector vecVelocity = g_pPlayerMove->velocity;
				Vector vecOrigin = g_pPlayerMove->origin;

				UTIL_AddCorrectGravity( vecVelocity, flFrametime );

				Vector vecMove = vecVelocity * flFrametime;

				// Trace forward
				g_pPlayerMove->usehull = ( g_pPlayerMove->flags & FL_DUCKING ) ? PM_HULL_DUCKED_PLAYER : PM_HULL_PLAYER;

				tr = g_pPlayerMove->PM_PlayerTrace( vecOrigin, vecOrigin + vecMove, PM_NORMAL, -1 );

				// Save trace pos
				vecOrigin = tr.endpos;

				// Did hit a wall or started in solid
				if ( tr.fraction != 1.f && !tr.allsolid && tr.plane.normal.z >= 0.7f )
				{
					bPredictedOnGround = true;
				}
				else
				{
					Vector point = vecOrigin;
					point.z -= 2.f;

					// Trace down
					g_pPlayerMove->usehull = ( g_pPlayerMove->flags & FL_DUCKING ) ? PM_HULL_DUCKED_PLAYER : PM_HULL_PLAYER;

					tr = g_pPlayerMove->PM_PlayerTrace( vecOrigin, point, PM_NORMAL, -1 );

					if ( tr.plane.normal.z >= 0.7f )
					{
						bPredictedOnGround = true;
					}
				}

				g_pPlayerMove->usehull = oldhull;

				// Not needed actually
				//FixupGravityVelocity( vecVelocity, flFrametime );

				if ( bPredictedOnGround )
				{
					fps_prev = int( fps_max->value );

					CVar()->SetValue( fps_max, sc_ducktap_adjust_fps.GetInt() );

					// Record fps change
					if ( g_InputManager.IsRecording() )
					{
						// shit hack
						if ( g_InputManager.GetInputContext().FrameCounter() - 1 >= 0 )
						{
							snprintf( fps_buffer, M_ARRAYSIZE( fps_buffer ), "fps_max %d\n", sc_ducktap_adjust_fps.GetInt() );

							std::vector<im_frame_t> &frames = g_InputManager.GetInputContext().Frames();
							int prevframe = g_InputManager.GetInputContext().FrameCounter() - 1;

							if ( frames[ prevframe ].commands != NULL )
							{
								std::string sCommandsBuffer = frames[ prevframe ].commands;
								sCommandsBuffer += fps_buffer;

								free( (void *)( frames[ prevframe ].commands ) );

								frames[ prevframe ].commands = strdup( sCommandsBuffer.c_str() );
							}
							else
							{
								frames[ prevframe ].commands = strdup( fps_buffer );
							}
						}
					}

					must_return_fps = true;
					return;
				}
			}
		}

		if ( g_pPlayerMove->onground != -1 && onground_prev == -1 && !g_pPlayerMove->bInDuck )
		{
			cmd->buttons |= IN_DUCK;
		}
	}

	onground_prev = g_pPlayerMove->onground;
}

//-----------------------------------------------------------------------------
// Fast Run
//-----------------------------------------------------------------------------

void CMisc::FastRun(struct usercmd_s *cmd)
{
	if ( g_Config.cvars.fastrun && ( Client()->IsOnGround() || Client()->IsSpectating() ) )
	{
		static bool bFastRun = false;
		float flMaxSpeed = Client()->GetMaxSpeed();

		if ((cmd->buttons & IN_FORWARD && cmd->buttons & IN_MOVELEFT) || (cmd->buttons & IN_BACK && cmd->buttons & IN_MOVERIGHT))
		{
			if (bFastRun)
			{
				cmd->sidemove -= flMaxSpeed; // sqrtf(2.0f) * flMaxSpeed   vvv
				cmd->forwardmove -= flMaxSpeed;

				bFastRun = false;
			}
			else
			{
				cmd->sidemove += flMaxSpeed;
				cmd->forwardmove += flMaxSpeed;

				bFastRun = true;
			}
		}
		else if ((cmd->buttons & IN_FORWARD && cmd->buttons & IN_MOVERIGHT) || (cmd->buttons & IN_BACK && cmd->buttons & IN_MOVELEFT))
		{
			if (bFastRun)
			{
				cmd->sidemove -= flMaxSpeed;
				cmd->forwardmove += flMaxSpeed;

				bFastRun = false;
			}
			else
			{
				cmd->sidemove += flMaxSpeed;
				cmd->forwardmove -= flMaxSpeed; // sqrtf(2.0f) * flMaxSpeed  ^^^

				bFastRun = true;
			}
		}
		else if (cmd->buttons & IN_FORWARD || cmd->buttons & IN_BACK)
		{
			if (bFastRun)
			{
				cmd->sidemove -= flMaxSpeed;
				bFastRun = false;
			}
			else
			{
				cmd->sidemove += flMaxSpeed;
				bFastRun = true;
			}
		}
		else if (cmd->buttons & IN_MOVELEFT || cmd->buttons & IN_MOVERIGHT)
		{
			if (bFastRun)
			{
				cmd->forwardmove -= flMaxSpeed;
				bFastRun = false;
			}
			else
			{
				cmd->forwardmove += flMaxSpeed;
				bFastRun = true;
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Spinner
//-----------------------------------------------------------------------------

static bool IsBusyWithLongJump(usercmd_t *cmd)
{
	if ( cmd->buttons & IN_JUMP && Client()->IsOnGround() )
	{
		if ( Client()->IsDucking() )
		{
			if ( cmd->buttons & IN_DUCK /* && g_pPlayerMove->flDuckTime > 0.f */ )
			{
				const char *pszValue = g_pEngineFuncs->PhysInfo_ValueForKey("slj");
				bool bCanSuperJump = (pszValue != NULL && *pszValue == '1');

				if ( bCanSuperJump && Client()->GetVelocity().Length() > 50.f)
				{
					return true;
				}
			}
		}
	}

	return false;
}

void CMisc::Spinner_Wrapper(struct usercmd_s *cmd)
{
	if ( g_Config.cvars.silent_aimbot || g_Config.cvars.ragebot )
	{
		m_bSpinnerDelayed = true;
		return;
	}

	Spinner(cmd);
	m_bSpinnerDelayed = false;
}

void CMisc::Spinner(struct usercmd_s *cmd)
{
	bool bAnglesChanged = false;
	m_bSpinCanChangePitch = false;

	if ( g_Config.cvars.spin_yaw_angle )
	{
		if ( !g_Config.cvars.lock_pitch && !g_Config.cvars.spin_pitch_angle )
			g_vecSpinAngles.x = cmd->viewangles.x;

		g_vecSpinAngles.y += g_Config.cvars.spin_yaw_rotation_angle;
		g_vecSpinAngles.y = NormalizeAngle(g_vecSpinAngles.y);

		bAnglesChanged = true;
	}
	else if ( g_Config.cvars.lock_yaw )
	{
		if ( !g_Config.cvars.lock_pitch && !g_Config.cvars.spin_pitch_angle )
			g_vecSpinAngles.x = cmd->viewangles.x;

		g_vecSpinAngles.y = g_Config.cvars.lock_yaw_angle;
		bAnglesChanged = true;
	}
	else if ( g_Config.cvars.revert_yaw )
	{
		if ( !g_Config.cvars.lock_pitch && !g_Config.cvars.spin_pitch_angle && !g_Config.cvars.revert_pitch )
			g_vecSpinAngles.x = cmd->viewangles.x;

		g_vecSpinAngles.y = NormalizeAngle( cmd->viewangles.y - 180.f );
		bAnglesChanged = true;
	}

	if ( g_Config.cvars.spin_pitch_angle )
	{
		if ( !g_Config.cvars.lock_yaw && !g_Config.cvars.spin_yaw_angle )
			g_vecSpinAngles.y = cmd->viewangles.y;

		g_vecSpinAngles.x += g_Config.cvars.spin_pitch_rotation_angle;
		g_vecSpinAngles.x = NormalizeAngle(g_vecSpinAngles.x);

		m_bSpinCanChangePitch = true;
		bAnglesChanged = true;
	}
	else if ( g_Config.cvars.lock_pitch )
	{
		if ( !g_Config.cvars.lock_yaw && !g_Config.cvars.spin_yaw_angle )
			g_vecSpinAngles.y = cmd->viewangles.y;

		g_vecSpinAngles.x = g_Config.cvars.lock_pitch_angle;

		m_bSpinCanChangePitch = true;
		bAnglesChanged = true;
	}
	else if ( g_Config.cvars.revert_pitch )
	{
		if ( !g_Config.cvars.lock_yaw && !g_Config.cvars.spin_yaw_angle && !g_Config.cvars.revert_yaw )
			g_vecSpinAngles.y = cmd->viewangles.y;

		g_vecSpinAngles.x = NormalizeAngle( -cmd->viewangles.x );

		m_bSpinCanChangePitch = true;
		bAnglesChanged = true;
	}

	if ( bAnglesChanged )
	{
		if ( Client()->GetMoveType() == MOVETYPE_WALK && Client()->GetWaterLevel() <= WL_FEET )
		{
			if ( !(cmd->buttons & IN_USE || cmd->impulse == 201 || UTIL_IsFiring(cmd) || IsBusyWithLongJump(cmd)) )
			{
				m_flSpinPitchAngle = g_vecSpinAngles.x / -3.0f;
				UTIL_SetAnglesSilent(g_vecSpinAngles, cmd);
			}
			else
			{
				m_bSpinCanChangePitch = false;
			}
		}
		else
		{
			m_bSpinCanChangePitch = false;
		}
	}
}

//-----------------------------------------------------------------------------
// Trigger push boost exploit
//-----------------------------------------------------------------------------

void CMisc::TriggerPushExploit( struct usercmd_s *cmd )
{
	static int duck_wait = 0;

	if ( false )
	{
		Vector vecMins, vecMaxs, vecDuckMins, vecDuckMaxs;

		bool bInteractedWithTrigger = false;

		vecMins = g_pPlayerMove->origin + VEC_HULL_MIN;
		vecMaxs = g_pPlayerMove->origin + VEC_HULL_MAX;
		
		vecDuckMins = g_pPlayerMove->origin + VEC_DUCK_HULL_MIN;
		vecDuckMaxs = g_pPlayerMove->origin + VEC_DUCK_HULL_MAX;

		const std::vector<TriggerEntity> &triggers = g_Bsp.GetTriggers();

		for ( const TriggerEntity &trigger : triggers )
		{
			Vector vecTriggerMins = trigger.vecOrigin + trigger.vecMins;
			Vector vecTriggerMaxs = trigger.vecOrigin + trigger.vecMaxs;
			
			Vector vecTriggerTestMins = vecTriggerMins + VEC_HULL_MIN;
			Vector vecTriggerTestMaxs = vecTriggerMaxs + VEC_HULL_MAX;

			if ( UTIL_IsAABBIntersectingAABB( vecMins, vecMaxs, vecTriggerTestMins, vecTriggerTestMaxs ) )
			{
				if ( !UTIL_IsAABBIntersectingAABB( vecDuckMins, vecDuckMaxs, vecTriggerMins, vecTriggerMaxs ) )
				{
					if ( duck_wait > 1 )
						duck_wait = 0;

					if ( duck_wait <= 0 )
						cmd->buttons |= IN_DUCK;
					else
						cmd->buttons &= ~IN_DUCK;

					duck_wait++;
				}
				else
				{
					duck_wait = 0;
					cmd->buttons &= ~IN_DUCK;
				}

				bInteractedWithTrigger = true;
			}
		}

		if ( !bInteractedWithTrigger )
		{
			duck_wait = 0;
		}
	}
}

//-----------------------------------------------------------------------------
// Auto Wallstrafing
//-----------------------------------------------------------------------------

ConVar sc_auto_wallstrafing_angle( "sc_auto_wallstrafing_angle", "", FCVAR_CLIENTDLL, "Yaw angle to wall strafe" );

void CMisc::AutoWallstrafing(struct usercmd_s *cmd)
{
	if ( g_Config.cvars.auto_wallstrafing && !Client()->IsDead() && Client()->GetWaterLevel() == WL_NOT_IN_WATER && Client()->IsOnGround() )
	{
		if ( cmd->buttons & (IN_FORWARD | IN_BACK | IN_LEFT | IN_RIGHT | IN_RUN) )
			return;

		pmtrace_t trace;
		Vector2D vecNormal;

		Vector va;
		Vector vecForward, vecRight, vecLeft;
		Vector vecOrigin = Client()->GetOrigin();

		cl_entity_t *pLocal = g_pEngineFuncs->GetLocalPlayer();

		bool bWallStrafe = false;
		bool bRight = false;

		if ( sc_auto_wallstrafing_angle.GetString()[ 0 ] == '\0' )
			g_pEngineFuncs->GetViewAngles( va );
		else
			va.y = sc_auto_wallstrafing_angle.GetFloat();

		vecForward.x = cosf(va.y * static_cast<float>(M_PI) / 180.f);
		vecForward.y = sinf(va.y * static_cast<float>(M_PI) / 180.f);
		vecForward.z = 0.f;

		vecRight.x = vecForward.y * g_Config.cvars.wallstrafing_dist;
		vecRight.y = -vecForward.x * g_Config.cvars.wallstrafing_dist;
		vecRight.z = 0.f;

		vecLeft = -vecRight;

		g_pEventAPI->EV_SetTraceHull( (Client()->GetFlags() & FL_DUCKING) ? PM_HULL_DUCKED_PLAYER : PM_HULL_PLAYER );
		g_pEventAPI->EV_PlayerTrace(vecOrigin, vecOrigin + vecRight, PM_WORLD_ONLY, -1, &trace);

		if (trace.fraction < 1.f)
		{
			bWallStrafe = true;
			bRight = true;
			vecNormal = trace.plane.normal.AsVector2D();
		}
		else
		{
			g_pEventAPI->EV_SetTraceHull( (Client()->GetFlags() & FL_DUCKING) ? PM_HULL_DUCKED_PLAYER : PM_HULL_PLAYER );
			g_pEventAPI->EV_PlayerTrace(vecOrigin, vecOrigin + vecLeft, PM_WORLD_ONLY, -1, &trace);

			if (trace.fraction < 1.f)
			{
				bWallStrafe = true;
				vecNormal = trace.plane.normal.AsVector2D();
			}
		}

		if (bWallStrafe)
		{
			Vector vecDir;

			float flBestStrafeAngle = (g_Config.cvars.wallstrafing_angle + 90.f + 45.f) * static_cast<float>(M_PI) / 180.f; // radians

			if ( bRight )
				flBestStrafeAngle *= -1.f;

			vecNormal.NormalizeInPlace();

			// Rotate
			vecDir.x = vecNormal.x * cosf(flBestStrafeAngle) - vecNormal.y * sinf(flBestStrafeAngle);
			vecDir.y = vecNormal.x * sinf(flBestStrafeAngle) + vecNormal.y * cosf(flBestStrafeAngle);
			vecDir.z = 0.f;

			vecForward.x = cosf(cmd->viewangles.y * static_cast<float>(M_PI) / 180.f);
			vecForward.y = sinf(cmd->viewangles.y * static_cast<float>(M_PI) / 180.f);
			vecForward.z = 0.f;

			vecRight.x = vecForward.y;
			vecRight.y = -vecForward.x;
			vecRight.z = 0.f;

			vecForward *= Client()->GetMaxSpeed();
			vecRight *= Client()->GetMaxSpeed();

			float forwardmove = DotProduct(vecForward, vecDir);
			float sidemove = DotProduct(vecRight, vecDir);

			cmd->forwardmove = forwardmove;
			cmd->sidemove = sidemove;
		}
	}
}

//-----------------------------------------------------------------------------
// Auto Reload
//-----------------------------------------------------------------------------

void CMisc::AutoReload(struct usercmd_s *cmd)
{
	if ( !g_Config.cvars.autoreload )
		return;

	if ( ClientWeapon()->IsReloading() )
		return;

	int iClip = ClientWeapon()->Clip();

	if (iClip == 0)
	{
		if ( Client()->GetCurrentWeaponID() == WEAPON_RPG )
		{
			// Can't reload while using laser homing
			if (ClientWeapon()->GetWeaponData()->iuser4 && ClientWeapon()->GetWeaponData()->fuser1 != 0.f)
			{
				return;
			}
		}

		cmd->buttons |= IN_RELOAD;
	}
}

//-----------------------------------------------------------------------------
// Stick
//-----------------------------------------------------------------------------

void CMisc::Stick(struct usercmd_s *cmd)
{
	static bool s_iClimb = 0;

	if ( Client()->IsDead() )
	{
		if ( sc_stick.GetInt() != 0 )
			sc_stick.SetValue( 0 );

		s_iClimb = 0;
		return;
	}

	if ( sc_stick_auto.GetBool() )
	{
		int iPrevTarget = sc_stick.GetInt();
		float flDistanceSqr = FLT_MAX;

		cl_entity_t *pTarget = NULL;
		cl_entity_t *pLocal = g_pEngineFuncs->GetLocalPlayer();

		for (int i = 1; i <= MAXCLIENTS; i++)
		{
			cl_entity_t *pPlayer = g_pEngineFuncs->GetEntityByIndex(i);

			if ( pPlayer && pPlayer != pLocal && pPlayer->curstate.messagenum >= pLocal->curstate.messagenum )
			{
				float dist_sqr = (pLocal->curstate.origin - pPlayer->curstate.origin).LengthSqr();

				if (dist_sqr < flDistanceSqr)
				{
					pTarget = pPlayer;
					flDistanceSqr = dist_sqr;
				}
			}
		}

		if ( pTarget && (pLocal->curstate.origin - pTarget->curstate.origin).LengthSqr() <= M_SQR(512.f) )
		{
			if ( iPrevTarget != pTarget->index )
			{
				s_iClimb = 0;
			}

			sc_stick.SetValue( pTarget->index );
		}
		else
		{
			sc_stick.SetValue( 0 );
		}
	}

	if ( sc_stick.GetInt() != 0 )
	{
		cl_entity_t *pPlayer = g_pEngineFuncs->GetEntityByIndex( sc_stick.GetInt() );

		if ( pPlayer && pPlayer->curstate.messagenum >= g_pEngineFuncs->GetLocalPlayer()->curstate.messagenum )
		{
			Vector vPredictPos = pPlayer->curstate.origin + (pPlayer->curstate.origin - pPlayer->prevstate.origin);

			Vector2D vecDir = vPredictPos.AsVector2D() - g_pPlayerMove->origin.AsVector2D();
			vecDir.NormalizeInPlace();

			if ( sc_stick_steal_model.GetBool() )
			{
				player_info_t *pPlayerInfo = g_pEngineStudio->PlayerInfo( sc_stick.GetInt() - 1 );

				if ( pPlayerInfo )
				{
					cvar_t *model = CVar()->FindCvar("model");
					cvar_t *topcolor = CVar()->FindCvar("topcolor");
					cvar_t *bottomcolor = CVar()->FindCvar("bottomcolor");

					if ( stricmp(pPlayerInfo->model, model->string) )
					{
						CVar()->SetValue(model, pPlayerInfo->model);
					}

					if ( pPlayerInfo->topcolor != (int)topcolor->value )
					{
						CVar()->SetValue(topcolor, pPlayerInfo->topcolor);
					}

					if ( pPlayerInfo->bottomcolor != (int)bottomcolor->value )
					{
						CVar()->SetValue(bottomcolor, pPlayerInfo->bottomcolor);
					}
				}
			}

			if ( Client()->GetMoveType() == MOVETYPE_FLY )
			{
				physent_t *pe;
				hull_t *hull;
				int num;
				Vector test;

				physent_t *pLadder = NULL;

				for (int i = 0; i < g_pPlayerMove->nummoveent; i++)
				{
					pe = &g_pPlayerMove->moveents[i];

					if (pe->model && (modtype_t)g_pPlayerMove->PM_GetModelType(pe->model) == mod_brush && pe->skin == CONTENTS_LADDER)
					{
						hull = (hull_t *)g_pPlayerMove->PM_HullForBsp(pe, test);
						num = hull->firstclipnode;

						// Offset the test point appropriately for this hull.
						VectorSubtract(g_pPlayerMove->origin, test, test);

						// Test the player's hull for intersection with this model
						if ( g_pPlayerMove->PM_HullPointContents(hull, num, test) == CONTENTS_EMPTY )
							continue;

						pLadder = pe;
						break;
					}
				}

				if ( pLadder != NULL )
				{
					trace_t trace;

					Vector vecAngles;
					Vector ladderCenter;
					Vector modelmins, modelmaxs;

					if ( s_iClimb == 0 )
					{
						if ( pPlayer->curstate.origin.z > g_pPlayerMove->origin.z )
						{
							s_iClimb = 1;
						}
						else
						{
							s_iClimb = 2;
						}
					}

					g_pPlayerMove->PM_GetModelBounds(pLadder->model, modelmins, modelmaxs);

					VectorAdd(modelmins, modelmaxs, ladderCenter);
					VectorScale(ladderCenter, 0.5, ladderCenter);

					g_pPlayerMove->PM_TraceModel(pLadder, g_pPlayerMove->origin, ladderCenter, &trace);

					vecAngles.x = 89.f;
					vecAngles.y = atan2f(trace.plane.normal.y, trace.plane.normal.x) * static_cast<float>(180.0 / M_PI);
					vecAngles.z = 0.f;

					vecAngles.y -= 90.f;

					cmd->viewangles = vecAngles;

					if (s_iClimb == 1)
					{
						if ( pPlayer->curstate.origin.z <= g_pPlayerMove->origin.z )
						{
							cmd->buttons |= IN_JUMP;
						}
						else
						{
							cmd->forwardmove = -g_pPlayerMove->maxspeed;
							cmd->sidemove = -g_pPlayerMove->maxspeed;

							cmd->buttons |= ( IN_BACK | IN_MOVELEFT );
						}
					}
					else
					{
						if ( pPlayer->curstate.origin.z >= g_pPlayerMove->origin.z )
						{
							cmd->buttons |= IN_JUMP;
						}
						else
						{
							cmd->forwardmove = g_pPlayerMove->maxspeed;
							cmd->sidemove = g_pPlayerMove->maxspeed;

							cmd->buttons |= ( IN_FORWARD | IN_MOVERIGHT );
						}
					}
				}
				else
				{
					cmd->buttons |= IN_JUMP;
				}
			}
			else
			{
				pmtrace_t trace;

				g_pEventAPI->EV_SetTraceHull( (Client()->GetFlags() & FL_DUCKING) ? PM_HULL_DUCKED_PLAYER : PM_HULL_PLAYER );
				g_pEventAPI->EV_PlayerTrace( Client()->GetOrigin(), Client()->GetOrigin() + vecDir.ToVector() * 4.f, PM_WORLD_ONLY, -1, &trace);

				if ( trace.fraction != 1.f )
				{
					cmd->buttons |= IN_JUMP;
				}

				s_iClimb = 0;

				if ( !sc_stick_strafe.GetBool() )
				{
					Vector2D vecForward;
					Vector2D vecRight;

					vecForward.x = cosf(cmd->viewangles.y * static_cast<float>(M_PI / 180.0));
					vecForward.y = sinf(cmd->viewangles.y * static_cast<float>(M_PI / 180.0));

					vecRight.x = vecForward.y;
					vecRight.y = -vecForward.x;

					vecForward *= Client()->GetMaxSpeed();
					vecRight *= Client()->GetMaxSpeed();

					float forwardmove = DotProduct(vecForward, vecDir);
					float sidemove = DotProduct(vecRight, vecDir);

					cmd->forwardmove = forwardmove;
					cmd->sidemove = sidemove;
				}
				else
				{
					Vector va;
					g_pEngineFuncs->GetViewAngles(va);

					UpdateStrafeData(s_StickStrafeData,
									 true,
									 Strafe::StrafeDir::POINT,
									 Strafe::StrafeType::MAXACCEL,
									 va[1],
									 vPredictPos.x,
									 vPredictPos.y);

					Strafe::ProcessedFrame out;
					out.Yaw = va[1];

					Strafe::Friction(s_StickStrafeData);

					Strafe::StrafeVectorial(s_StickStrafeData, out, false);

					if (out.Processed)
					{
						cmd->forwardmove = out.Forwardspeed;
						cmd->sidemove = out.Sidespeed;

						//va[1] = static_cast<float>(out.Yaw);
					}

					if ( (vPredictPos.AsVector2D() - Client()->GetOrigin().AsVector2D()).LengthSqr() > M_SQR(300.f))
					{
						// Jump
						if ( Client()->IsOnGround() )
						{
							cmd->buttons |= IN_JUMP;
						}
					}
				}
			}

			if ( Client()->GetWaterLevel() > WL_NOT_IN_WATER && pPlayer->curstate.origin.z >= g_pPlayerMove->origin.z )
			{
				if ( Client()->GetFlags() & FL_WATERJUMP )
					cmd->buttons |= IN_DUCK;

				cmd->upmove = Client()->GetMaxSpeed();
			}
		}
		else
		{
			sc_stick.SetValue( 0 );
		}
	}

	if ( sc_stick_auto.GetBool() && sc_stick.GetInt() == 0 )
	{
		if ( Client()->GetMoveType() == MOVETYPE_FLY )
			cmd->buttons |= IN_JUMP;

		static bool forward_step = true;

		cmd->forwardmove = forward_step ? 50.0f : -50.0f;

		forward_step = !forward_step;
	}
}

//-----------------------------------------------------------------------------
// LookAt
//-----------------------------------------------------------------------------

ConVar sc_look_at("sc_look_at", "0", FCVAR_CLIENTDLL, "Look at entity");

void CMisc::LookAt(struct usercmd_s *cmd)
{
	int index = sc_look_at.GetInt();

	if ( index > 0 && index != Client()->GetPlayerIndex() && !Client()->IsSpectating() )
	{
		cl_entity_t *pEntity = g_pEngineFuncs->GetEntityByIndex(index);

		if ( pEntity != NULL )
		{
			Vector vecDir;
			Vector vecAngles;

			Vector vecSrc = g_pPlayerMove->origin + g_pPlayerMove->view_ofs;
			Vector vecTarget = pEntity->curstate.origin + Vector(0.f, 0.f, pEntity->curstate.usehull ? 12.5f : 28.5f);

			VectorSubtract( vecTarget, vecSrc, vecDir );

			vecAngles.x = -atan2f(vecDir.z, vecDir.Length2D()) * (180.0 / M_PI);
			vecAngles.y = atan2f(vecDir.y, vecDir.x) * (180.0 / M_PI);
			vecAngles.z = 0.f;

			NormalizeAngles( vecAngles );

			g_pEngineFuncs->SetViewAngles( vecAngles );
			cmd->viewangles = vecAngles;

			return;
		}
	}

	sc_look_at.SetValue( 0 );
}

//-----------------------------------------------------------------------------
// Auto Ceil Clipping
//-----------------------------------------------------------------------------

void CMisc::AutoCeilClipping(struct usercmd_s *cmd)
{
	static bool jumped = false;

	if ( g_Config.cvars.auto_ceil_clipping && !Client()->IsDead() )
	{
		if ( jumped )
		{
			if ( !Client()->IsOnGround() && Client()->GetWaterLevel() <= WL_FEET )
			{
				cmd->buttons |= IN_DUCK;

				// Suicide only if we reached apex or started falling
				if ( Client()->GetVelocity().z <= 0.f )
				{
					Vector vecStart = g_pPlayerMove->origin;
					Vector vecEnd = vecStart + Vector(0.f, 0.f, VEC_DUCK_HULL_MAX.z);

					pmtrace_t *pTrace = g_pEngineFuncs->PM_TraceLine(vecStart, vecEnd, PM_NORMAL, (Client()->GetFlags() & FL_DUCKING) ? 1 : 0, -1);

					if ( pTrace->fraction != 1.0f )
					{
						g_pEngineFuncs->ClientCmd("kill\n");
						jumped = false;
					}
				}
			}
			else
			{
				jumped = false;
			}
		}
		else
		{
			if ( !Client()->IsOnGround() && Client()->GetVelocity().z > 0.f )
				jumped = true;
			else
				jumped = false;
		}
	}
	else
	{
		jumped = false;
	}
}

//-----------------------------------------------------------------------------
// Fake Lag
//-----------------------------------------------------------------------------

void CMisc::FakeLag(float frametime)
{
	static bool bSetInterpOnce = false;

	if ( g_Config.cvars.fakelag_adaptive_ex_interp )
	{
		if (ex_interp->value != 0.01f)
			ex_interp->value = 0.01f;

		bSetInterpOnce = true;
	}
	else if ( bSetInterpOnce )
	{
		if (ex_interp->value == 0.01f)
			ex_interp->value = 0.1f;

		bSetInterpOnce = false;
	}

	if ( g_Config.cvars.fakelag )
	{
		bool bFakeLag = true;

		if ( g_Config.cvars.fakelag_move != 0 )
		{
			float flVelocity = Client()->GetVelocity().Length2D();

			if ( g_Config.cvars.fakelag_move == 1 ) // On land
			{
				if ( flVelocity > 0.f )
					bFakeLag = false;
			}
			else if ( g_Config.cvars.fakelag_move == 2 ) // On move
			{
				if ( flVelocity == 0.f )
					bFakeLag = false;
			}
			else if ( g_Config.cvars.fakelag_move == 3 ) // In air
			{
				if ( Client()->IsOnGround() )
					bFakeLag = false;
			}
		}

		if (bFakeLag)
		{
			static int choked = 0;
			static int good = 0;

			if (g_Config.cvars.fakelag_type == 0) // Dynamic
			{
				if (choked < g_Config.cvars.fakelag_limit)
				{
					bSendPacket = false;

					choked++;

					good = 0;
				}
				else
				{
					float one = g_Config.cvars.fakelag_limit / 100.f;
					float tmp = one * g_Config.cvars.fakelag_variance;

					good++;

					if (good > tmp)
					{
						choked = 0;
					}
				}
			}
			else if (g_Config.cvars.fakelag_type == 1) // Maximum
			{
				choked++;

				if (choked > 0)
					bSendPacket = false;

				if (choked > g_Config.cvars.fakelag_limit)
					choked = -1; // 1 tick valid
			}
			else if (g_Config.cvars.fakelag_type == 2) // Flucture
			{
				static bool jitter = false;

				if (jitter)
					bSendPacket = false;

				jitter = !jitter;
			}
			else if (g_Config.cvars.fakelag_type == 3) // Break lag compensation
			{
				Vector velocity = Client()->GetVelocity();
				velocity.z = 0;
				float len = velocity.Length() * frametime;

				int choke = std::min<int>(static_cast<int>(std::ceilf(64.0f / len)), 14);
				if (choke > 14) return;

				static int choked = 0;
				if (choked > choke)
				{
					bSendPacket = true;
					choked = 0;
				}
				else
				{
					bSendPacket = false;
					choked++;
				}
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Auto Selfsink
//-----------------------------------------------------------------------------

ConVar sc_selfsink2_min_height("sc_selfsink2_min_height", "5", FCVAR_CLIENTDLL);

void CMisc::AutoSelfSink(struct usercmd_s *cmd) // improve it tf
{
	static int selfsink2_state = 0;
	static int selfsink2_frames = 0;

	if ( s_bSelfSink )
	{
		if ( Client()->IsDead() )
		{
			s_bSelfSink = false;
			return;
		}

		cmd->buttons |= IN_DUCK;

		if ( g_pPlayerMove->view_ofs.z == VEC_DUCK_VIEW.z )
		{
			cmd->buttons |= IN_JUMP;

			if ( g_pPlayerMove->onground == -1 )
			{
				g_pEngineFuncs->ClientCmd("kill");
				s_bSelfSink = false;
			}
		}
	}
	else if ( s_bSelfSink2 )
	{
		if ( Client()->IsDead() )
		{
			s_bSelfSink2 = false;
			selfsink2_state = 0;
			return;
		}

		switch ( selfsink2_state )
		{
		case 0:
		{
			if ( g_pPlayerMove->onground != -1 )
			{
				cmd->buttons |= IN_DUCK;
				selfsink2_state = 1;
			}
			else
			{
				cmd->buttons |= IN_DUCK;
				selfsink2_state = 3;
			}

			selfsink2_frames = 0;
			break;
		}

		case 1:
		{
			cmd->buttons |= IN_DUCK;

			if ( selfsink2_frames++ >= 5 )
			{
				selfsink2_state = 2;
				selfsink2_frames = 0;
			}

			break;
		}
		
		case 2:
		{
			cmd->buttons &= ~IN_DUCK;

			if ( selfsink2_frames++ >= 5 )
				selfsink2_state = 3;
			
			break;
		}

		case 3:
		{
			if ( g_pPlayerMove->onground != -1 )
			{
				selfsink2_state = 0;
				s_bSelfSink2 = false;
				break;
			}

			pmtrace_t tr;

			bool bPredictedOnGround = false;
			int oldhull = g_pPlayerMove->usehull;
			float flFrametime = g_pPlayerMove->frametime;
			Vector vecVelocity = g_pPlayerMove->velocity;
			Vector vecOrigin = g_pPlayerMove->origin;

			for ( int i = 0; i < 2; i++ )
			{
				UTIL_AddCorrectGravity( vecVelocity, flFrametime );

				Vector vecMove = vecVelocity * flFrametime;

				// Trace forward
				g_pPlayerMove->usehull = ( g_pPlayerMove->flags & FL_DUCKING ) ? PM_HULL_DUCKED_PLAYER : PM_HULL_PLAYER;
				tr = g_pPlayerMove->PM_PlayerTrace( vecOrigin, vecOrigin + vecMove, PM_NORMAL, -1 );

				// Save trace pos
				vecOrigin = tr.endpos;

				// Did hit a wall or started in solid
				if ( tr.fraction != 1.f && !tr.allsolid && tr.plane.normal.z >= 0.7f )
				{
					bPredictedOnGround = true;
				}
				else
				{
					Vector point = vecOrigin;
					point.z -= 2.f;

					// Trace down
					g_pPlayerMove->usehull = ( g_pPlayerMove->flags & FL_DUCKING ) ? PM_HULL_DUCKED_PLAYER : PM_HULL_PLAYER;
					tr = g_pPlayerMove->PM_PlayerTrace( vecOrigin, point, PM_NORMAL, -1 );

					if ( tr.plane.normal.z >= 0.7f )
					{
						bPredictedOnGround = true;
					}
				}

				g_pPlayerMove->usehull = oldhull;

				UTIL_FixupGravityVelocity( vecVelocity, flFrametime );

				if ( bPredictedOnGround )
				{
					g_pEngineFuncs->ClientCmd( "kill" );

					selfsink2_state = 0;
					s_bSelfSink2 = false;

					break;
				}
			}

			cmd->buttons |= IN_DUCK;
			break;

			/*
			//float ent_gravity;

			//Vector vecMove;
			//Vector vecOrigin = g_pPlayerMove->origin;
			//Vector vecVelocity = g_pPlayerMove->velocity;

			// Trace forward
			const int old_hull = g_pPlayerMove->usehull;
			g_pPlayerMove->usehull = ( g_pPlayerMove->flags & FL_DUCKING ? PM_HULL_DUCKED_PLAYER : PM_HULL_PLAYER );

			pmtrace_t trace = g_pPlayerMove->PM_PlayerTrace( vecOrigin, vecOrigin - Vector( 0, 0, 8192 ), PM_NORMAL, -1);
			//pmtrace_t trace = g_pPlayerMove->PM_PlayerTrace( vecOrigin, vecOrigin + vecMove, PM_NORMAL, -1 );

			g_pPlayerMove->usehull = old_hull;

			// Save trace pos
			//vecOrigin = trace.endpos;

			// Did hit a wall or started in solid
			if ( ( ( fabs( vecOrigin.z - trace.endpos.z ) <= sc_selfsink2_min_height.GetFloat() && !trace.allsolid ) || trace.startsolid ) )
			{
				g_pEngineFuncs->ClientCmd( "kill" );

				selfsink2_state = 0;
				s_bSelfSink2 = false;
			}

			if ( g_pPlayerMove->onground != -1 )
			{
				selfsink2_state = 0;
				s_bSelfSink2 = false;
				break;
			}
			*/
		}
		}
	}
	else
	{
		selfsink2_state = 0;
	}
}

//-----------------------------------------------------------------------------
// Tertiary Attack Glitch
//-----------------------------------------------------------------------------

void CMisc::TertiaryAttackGlitch()
{
	if (g_Config.cvars.tertiary_attack_glitch)
	{
		if (!IsTertiaryAttackGlitchPatched())
		{
			EnableTertiaryAttackGlitch();
		}
	}
	else if (IsTertiaryAttackGlitchPatched())
	{
		DisableTertiaryAttackGlitch();
	}

	if (IsTertiaryAttackGlitchInit_Server())
	{
		if (g_Config.cvars.tertiary_attack_glitch)
		{
			if (!IsTertiaryAttackGlitchPatched_Server())
			{
				EnableTertiaryAttackGlitch_Server();
			}
		}
		else if (IsTertiaryAttackGlitchPatched_Server())
		{
			DisableTertiaryAttackGlitch_Server();
		}
	}
}

//-----------------------------------------------------------------------------
// Color Pulsator
//-----------------------------------------------------------------------------

void CMisc::ColorPulsator()
{
	static char command_buffer[32];

	if (g_Config.cvars.color_pulsator)
	{
		if (g_Config.cvars.color_pulsator_top && g_pEngineFuncs->Sys_FloatTime() - s_flTopColorDelay >= g_Config.cvars.color_pulsator_delay)
		{
			if (s_iTopColorOffset > 12)
				s_iTopColorOffset = 0;
			
			s_flTopColorDelay = g_pEngineFuncs->Sys_FloatTime() + g_Config.cvars.color_pulsator_delay;

			sprintf_s(command_buffer, sizeof(command_buffer), "topcolor %d\n", s_iTopColorOffset * 20);
			g_pEngineFuncs->ClientCmd(command_buffer);

			++s_iTopColorOffset;
		}

		if (g_Config.cvars.color_pulsator_bottom && g_pEngineFuncs->Sys_FloatTime() - s_flBottomColorDelay >= g_Config.cvars.color_pulsator_delay)
		{
			if (s_iBottomColorOffset > 12)
				s_iBottomColorOffset = 0;

			s_flBottomColorDelay = g_pEngineFuncs->Sys_FloatTime() + g_Config.cvars.color_pulsator_delay;

			sprintf_s(command_buffer, sizeof(command_buffer), "bottomcolor %d\n", s_iBottomColorOffset * 20);
			g_pEngineFuncs->ClientCmd(command_buffer);

			++s_iBottomColorOffset;
		}
	}
}

//-----------------------------------------------------------------------------
// Quake Guns
//-----------------------------------------------------------------------------

void CMisc::QuakeGuns_V_CalcRefdef()
{
	if (g_Config.cvars.quake_guns)
	{
		cl_entity_s *pViewModel = g_pEngineFuncs->GetViewModel();

		if ( !pViewModel )
			return;

		if ( Client()->GetCurrentWeaponID() == WEAPON_NONE )
			return;

		float offset = GetWeaponOffset(pViewModel);

		Vector va, right;

		float *org = pViewModel->origin;
		float *ang = pViewModel->angles;

		g_pEngineFuncs->GetViewAngles(va);
		g_pEngineFuncs->AngleVectors(va, NULL, right, NULL);

		org[0] += right[0] * offset;
		org[1] += right[1] * offset;
		org[2] += right[2] * offset;
	}
}

void CMisc::QuakeGuns_HUD_PostRunCmd(struct local_state_s *to)
{
	s_iWeaponID = to->client.m_iId;
}

//-----------------------------------------------------------------------------
// Init
//-----------------------------------------------------------------------------

CMisc::CMisc()
{
	m_pfnQueryPerformanceCounter = NULL;
	m_pfnNetchan_Transmit = NULL;

	m_hQueryPerformanceCounter = 0;
	m_hNetchan_Transmit = 0;

	m_flSpinPitchAngle = 0.f;
	m_bSpinCanChangePitch = false;

	m_iFakeLagCounter = 0;
	m_iFakeLagCounter2 = 0;
	m_iOneTickExploitLagInterval = 0;

	s_StickStrafeData.frame.UseGivenButtons = true;
	s_StickStrafeData.frame.buttons = Strafe::StrafeButtons();
	s_StickStrafeData.frame.buttons.AirLeft = Strafe::Button::LEFT;
	s_StickStrafeData.frame.buttons.AirRight = Strafe::Button::RIGHT;
}

CMisc::~CMisc()
{
	m_OnTickCommands.clear();
}

bool CMisc::Load()
{
	bool ScanOK = true;

	ex_interp = CVar()->FindCvar("ex_interp");

	if ( !ex_interp )
	{
		Warning("Can't find cvar \"ex_interp\"\n");
		return false;
	}
	

	fps_max = CVar()->FindCvar("fps_max");

	if ( !fps_max )
	{
		Warning("Can't find cvar \"fps_max\"\n");
		return false;
	}

	m_pfnQueryPerformanceCounter = (void *)QueryPerformanceCounter;

	auto fpfnNetchan_Transmit = MemoryUtils()->FindPatternAsync( SvenModAPI()->Modules()->Hardware, Patterns::Hardware::Netchan_Transmit );
	auto fpfnCClient_SoundEngine__Play2DSound = MemoryUtils()->FindPatternAsync( SvenModAPI()->Modules()->Client, Patterns::Client::CClient_SoundEngine__Play2DSound );

	if ( !( m_pfnNetchan_Transmit = fpfnNetchan_Transmit.get() ) )
	{
		Warning("Couldn't find function \"Netchan_Transmit\"\n");
		ScanOK = false;
	}
	
	if ( !( m_pfnCClient_SoundEngine__Play2DSound = fpfnCClient_SoundEngine__Play2DSound.get() ) )
	{
		Warning("Couldn't find function \"CClient_SoundEngine::Play2DSound\"\n");
		ScanOK = false;
	}

	if ( !ScanOK )
		return false;

	return true;
}

void CMisc::PostLoad()
{
	m_hQueryPerformanceCounter = DetoursAPI()->DetourFunction( m_pfnQueryPerformanceCounter, HOOKED_fQueryPerformanceCounter, GET_FUNC_PTR(ORIG_fQueryPerformanceCounter) );
	m_hNetchan_Transmit = DetoursAPI()->DetourFunction( m_pfnNetchan_Transmit, HOOKED_fNetchan_Transmit, GET_FUNC_PTR(ORIG_fNetchan_Transmit) );
	m_hCClient_SoundEngine__Play2DSound = DetoursAPI()->DetourFunction( m_pfnCClient_SoundEngine__Play2DSound, HOOKED_CClient_SoundEngine__Play2DSound, GET_FUNC_PTR(ORIG_CClient_SoundEngine__Play2DSound) );

#if USE_GAY_PERFECT_AUTOJUMP
	hJumpDown = Hooks()->HookConsoleCommand( "+jump", HOOKED_JumpDown, &ORIG_JumpDown );
	hJumpUp = Hooks()->HookConsoleCommand( "-jump", HOOKED_JumpUp, &ORIG_JumpUp );
#endif
}

void CMisc::Unload()
{
	DetoursAPI()->RemoveDetour( m_hQueryPerformanceCounter );
	DetoursAPI()->RemoveDetour( m_hNetchan_Transmit );
	DetoursAPI()->RemoveDetour( m_hCClient_SoundEngine__Play2DSound );

#if USE_GAY_PERFECT_AUTOJUMP
	Hooks()->UnhookConsoleCommand( hJumpDown );
	Hooks()->UnhookConsoleCommand( hJumpUp );
#endif
}