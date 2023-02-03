// Speedrun Tools

#include <ISvenModAPI.h>
#include <IMemoryUtils.h>
#include <IHooks.h>

#include <convar.h>
#include <dbg.h>

#include <data_struct/hash.h>
#include <generichash.h>

#include "speedrun_tools.h"

#include "../modules/server.h"
#include "../modules/server_client_bridge.h"

#include "../game/drawing.h"
#include "../game/utils.h"
#include "../strafe/strafe_utils.h"

#include "../patterns.h"
#include "../config.h"

//-----------------------------------------------------------------------------
// Declare Hooks... and function pointer
//-----------------------------------------------------------------------------

DECLARE_HOOK(void, __cdecl, UTIL_GetCircularGaussianSpread, float *, float *);
DECLARE_HOOK(qboolean, __cdecl, Host_FilterTime, float);
DECLARE_HOOK(int, __cdecl, Cbuf_AddText, const char *);
DECLARE_HOOK(int, __cdecl, ServerCmd, const char *);

//-----------------------------------------------------------------------------
// Forward declaration of CHash-related functions
//-----------------------------------------------------------------------------

typedef const char *cstring_t;

static bool CompareFunc(const cstring_t &a, const cstring_t &b)
{
	return !stricmp(a, b);
}

static unsigned int HashFunc(const cstring_t &a)
{
	return HashStringCaseless(a);
}

//-----------------------------------------------------------------------------
// Vars
//-----------------------------------------------------------------------------

DEFINE_PATTERN(UTIL_GetCircularGaussianSpread_sig, "56 8B 74 24 08 57 8B 7C 24 10 66 0F 1F 44 00 00");
DEFINE_PATTERN(Host_FilterTime_sig, "E9 ? ? ? ? 90 90 90 8B 0D ? ? ? ? D8");
DEFINE_PATTERN(host_framerate_patch_sig, "74 ? DD ? B8");

CSpeedrunTools g_SpeedrunTools;
CHash<cstring_t> g_WhitelistCommands(15, CompareFunc, HashFunc);

// input manager
bool im_record = false;
FILE *im_file = NULL;
static bool im_saved_move = false;
static int im_version = IM_FILE_VERSION;
static input_frame_t frame_buffer;
static std::string im_filename;
static std::string im_commands;

// sc_st_setangles
static bool s_bSetAngles = false;
static Vector s_vecSetAngles;
static Vector s_vecSetAnglesSpeed;
// sc_st_follow_point
static bool s_bFollowPoint = false;
static float s_flFollowPointLerp = 1.f;
static Vector s_vecFollowPoint;

bool is_hl_c17 = false;
int iNihilanthIndex = 0;
entvars_t *pNihilanthVars = NULL;

// cvars
static cvar_t *fps_max = NULL;
static cvar_t *host_framerate = NULL;
static bool s_bIgnoreCvarChange = false;
static bool s_bNotifyTimescaleChanged = false;

//-----------------------------------------------------------------------------
// ConCommands, CVars..
//-----------------------------------------------------------------------------

ConVar sc_im_autorecord("sc_im_autorecord", "0", FCVAR_CLIENTDLL, "Automatically record inputs at map start");
ConVar sc_im_autoplay("sc_im_autoplay", "0", FCVAR_CLIENTDLL, "Automatically play inputs at map start");

ConVar sc_st_min_frametime("sc_st_min_frametime", "0", FCVAR_CLIENTDLL, "Min frametime to run a frame");

ConVar sc_st_map_start_position("sc_st_map_start_position", "", FCVAR_CLIENTDLL, "Restart map if player hasn't spawned in the given position\n\"x y\" - 2D position\n\"\" - disabled");
ConVar sc_st_disable_spread("sc_st_disable_spread", "0", FCVAR_CLIENTDLL, "Disables spread");

static void CvarChangeHook_fps_max(cvar_t *pCvar, const char *pszOldValue, float flOldValue)
{
	if ( s_bIgnoreCvarChange || flOldValue == pCvar->value )
		return;

	if ( sc_st_min_frametime.GetFloat() != 0.f )
	{
		float timescale = 1.f / (sc_st_min_frametime.GetFloat() * flOldValue);
		float multiplier = 1.f / timescale;

		sc_st_min_frametime.SetValue( multiplier / pCvar->value );
		CVar()->SetValue( CVar()->FindCvar("host_framerate"), 1.f / pCvar->value );

		Utils()->PrintChatText("<SvenInt> Automatically adjusted timescale %.2f for %d fps\n", timescale, (int)pCvar->value);
	}
}

CON_COMMAND_NO_WRAPPER(sc_st_timer, "Show speedrun timer")
{
	Msg(g_Config.cvars.st_timer ? "Speedrun Timer disabled\n" : "Speedrun Timer enabled\n");
	g_Config.cvars.st_timer = !g_Config.cvars.st_timer;
}

CON_COMMAND(sc_st_timescale, "Set timescale")
{
	if (args.ArgC() >= 2)
	{
		g_SpeedrunTools.SetTimescale( atof(args[1]) );
		s_bNotifyTimescaleChanged = true;
	}
	else
	{
		if ( sc_st_min_frametime.GetFloat() != 0.f )
			Msg("Current timescale: %.3f\n", 1.f / ( sc_st_min_frametime.GetFloat() * CVar()->FindCvar("fps_max")->value ));
		else
			Msg("Current timescale: 1.000\n");
	}
}

CON_COMMAND(sc_st_follow_point, "Set local player view angles to a point and follow it")
{
	if ( args.ArgC() != 5 )
	{
		Msg("Usage: sc_st_follow_point <x> <y> <z> <lerp>\n");
		s_bFollowPoint = false;
		
		return;
	}

	s_vecFollowPoint.x = atof(args[1]);
	s_vecFollowPoint.y = atof(args[2]);
	s_vecFollowPoint.z = atof(args[3]);
	s_flFollowPointLerp = atof(args[4]);

	s_bFollowPoint = true;
}

CON_COMMAND(sc_st_setangles, "Set local player view angles")
{
	if ( args.ArgC() != 4 )
	{
		Msg("Usage: sc_st_setangles <pitch> <yaw> <frames>\n");
		return;
	}

	int nFrames = atoi(args.Arg(3));

	if ( nFrames > 0 )
	{
		Vector va;
		g_pEngineFuncs->GetViewAngles( va );

		float flPitch = atof(args.Arg(1));
		float flYaw = atof(args.Arg(2));

		float flNormalizedPitch = Strafe::NormalizeDeg(static_cast<double>(flPitch) - va[PITCH]);
		float flNormalizedYaw = Strafe::NormalizeDeg(static_cast<double>(flYaw) - va[YAW]);

		s_vecSetAngles[PITCH] = flPitch;
		s_vecSetAngles[YAW] = flYaw;

		s_vecSetAnglesSpeed[PITCH] = std::abs(flNormalizedPitch) / nFrames;
		s_vecSetAnglesSpeed[YAW] = std::abs(flNormalizedYaw) / nFrames;

		s_bSetAngles = true;
	}
}

CON_COMMAND(sc_im_record, "Record inputs")
{
	if (args.ArgC() >= 2)
	{
		void IM_Record(const char *pszFilename);

		IM_Record(args[1]);
	}
	else
	{
		Msg("Usage:  sc_im_record <filename>\n");
	}
}

CON_COMMAND(sc_im_play, "Playback inputs")
{
	if (args.ArgC() >= 2)
	{
		void IM_Play(const char *pszFilename);

		IM_Play(args[1]);
	}
	else
	{
		Msg("Usage:  sc_im_play <filename>\n");
	}
}

CON_COMMAND(sc_im_split, "Split playing back inputs")
{
	void IM_Split();

	IM_Split();
}

CON_COMMAND(sc_im_stop, "Stop recording inputs")
{
	void IM_Stop();

	IM_Stop();
}

//-----------------------------------------------------------------------------
// Input Manager
//-----------------------------------------------------------------------------

void IM_Record(const char *pszFilename)
{
	if (im_file)
	{
		Msg("Already in action\n");
		return;
	}

	auto ends_with = [](std::string const &value, std::string const &ending)
	{
		if (ending.size() > value.size())
			return false;

		return std::equal(ending.rbegin(), ending.rend(), value.rbegin());
	};

	std::string sFilename = pszFilename;

	if ( !ends_with(sFilename, ".bin") )
	{
		sFilename += ".bin";
	}

	im_filename = "sven_internal/input_manager/";
	im_filename += sFilename;

	im_file = fopen(im_filename.c_str(), "wb");

	if (im_file)
	{
		int header_buffer = IM_FILE_HEADER;
		fwrite(&header_buffer, 1, sizeof(short), im_file);

		int header_version = IM_FILE_VERSION;
		fwrite(&header_version, 1, sizeof(short), im_file);

		im_version = IM_FILE_VERSION;

		Msg("Recording inputs..\n");

		im_record = true;
		im_saved_move = false;
	}
	else
	{
		Warning("sc_im_record: failed to create file\n");
	}
}

void IM_Play(const char *pszFilename)
{
	if (im_file)
	{
		Msg("Already in action\n");
		return;
	}

	auto ends_with = [](std::string const &value, std::string const &ending)
	{
		if (ending.size() > value.size())
			return false;

		return std::equal(ending.rbegin(), ending.rend(), value.rbegin());
	};

	std::string sFilename = pszFilename;

	if ( !ends_with(sFilename, ".bin") )
	{
		sFilename += ".bin";
	}

	im_filename = "sven_internal/input_manager/";
	im_filename += sFilename;

	im_file = fopen(im_filename.c_str(), "rb");

	if (im_file)
	{
		int header_buffer = IM_FILE_HEADER;
		int header_version = IM_FILE_VERSION;

		fread(&header_buffer, 1, sizeof(short), im_file);
		fread(&header_version, 1, sizeof(short), im_file);

		if ( header_buffer == IM_FILE_HEADER )
		{
			if ( header_version >= 1 && header_version <= IM_FILE_VERSION )
			{
				im_record = false;
				im_version = header_version;
				Msg("Playing back inputs..\n");
			}
			else
			{
				Warning("sc_im_play: Unsupported file version\n");
				im_file = NULL;
			}
		}
		else
		{
			Warning("sc_im_play: Unsupported file\n");
			im_file = NULL;
		}
	}
	else
	{
		Warning("sc_im_play: failed to open file\n");
	}
}

void IM_Split()
{
	if ( !im_file || im_record )
	{
		Msg("Not playing back inputs\n");
		return;
	}

	long size = ftell(im_file);
	unsigned char *fileData = (unsigned char *)malloc(size);

	if ( !fileData )
	{
		Sys_Error("sc_im_split: Failed to allocate memory");
		return;
	}

	fseek(im_file, 0, SEEK_SET);
	fread(fileData, 1, size, im_file);

	fclose(im_file);

	im_file = fopen(im_filename.c_str(), "wb");
	im_record = true;
	im_saved_move = false;

	if (im_file)
	{
		im_version = *((short *)(fileData + sizeof(short)));

		fwrite(fileData, 1, size, im_file);
		Msg("Split playing back inputs\n");
	}
	else
	{
		im_file = NULL;

		Warning("sc_im_split: failed to create file\n");
	}
}

void IM_Stop()
{
	if (im_file)
	{
		fclose(im_file);
		Msg(im_record ? "Saved recorded inputs\n" : "Stopped playing back inputs\n");
	}
	else
	{
		Msg("Not in action\n");
	}

	im_file = NULL;
	im_record = false;
}

//-----------------------------------------------------------------------------
// Hooks
//-----------------------------------------------------------------------------

DECLARE_FUNC(void, __cdecl, HOOKED_UTIL_GetCircularGaussianSpread, float *x, float *y)
{
	ORIG_UTIL_GetCircularGaussianSpread(x, y);

	if ( sc_st_disable_spread.GetBool() )
	{
		*x = *y = 0.f;
	}
}

// Timescale
DECLARE_FUNC(qboolean, __cdecl, HOOKED_Host_FilterTime, float time)
{
	static double timeCounter = 0.0;
	static bool usePassedTime = false;

	float minFrametime = sc_st_min_frametime.GetFloat();

	if (minFrametime == 0.f)
	{
		timeCounter = 0.0;
		usePassedTime = false;
		return ORIG_Host_FilterTime(time);
	}

	timeCounter += time;

	if ( timeCounter < minFrametime )
		return 0;

	if ( ORIG_Host_FilterTime(usePassedTime ? time : static_cast<float>(timeCounter)) )
	{
		usePassedTime = false;
		timeCounter = std::fmod(timeCounter, minFrametime);
		return 1;
	}
	else
	{
		usePassedTime = true;
		return 0;
	}

	return 0;
}

// Input Manager commands recorder
DECLARE_FUNC(int, __cdecl, HOOKED_Cbuf_AddText, const char *pszCommand)
{
	if ( im_file && im_record && im_version >= 2 )
	{
		if ( *pszCommand != '\n' )
		{
			if ( !strncmp("weapon_", pszCommand, strlen("weapon_")) || g_WhitelistCommands.Find(pszCommand) != NULL )
			{
				im_commands += pszCommand;
				im_commands += '\n';
			}
		}
	}

	return ORIG_Cbuf_AddText(pszCommand);
}

// Input Manager commands recorder
DECLARE_FUNC(int, __cdecl, HOOKED_ServerCmd, const char *pszCommand)
{
	if ( im_file && im_record && im_version >= 2 )
	{
		if ( *pszCommand != '\n' )
		{
			if ( !strncmp("weapon_", pszCommand, strlen("weapon_")) )
			{
				im_commands += pszCommand;
				im_commands += '\n';
			}
		}
	}

	return ORIG_ServerCmd(pszCommand);
}

void CSpeedrunTools::OnBeginLoading()
{
	is_hl_c17 = false;

	iNihilanthIndex = 0;
	pNihilanthVars = NULL;

	if (im_file)
	{
		fclose(im_file);
		Msg(im_record ? "Saved recorded inputs\n" : "Stopped playing back inputs\n");
	}

	im_file = NULL;
	im_record = false;

	StopTimer();
}

void CSpeedrunTools::OnFirstClientdataReceived(client_data_t *pcldata, float flTime)
{
	if ( Host_IsServerActive() )
	{
		const char *pszMapname = gpGlobals->pStringBase + gpGlobals->mapname;

		iNihilanthIndex = 0;
		pNihilanthVars = NULL;

		if ( !stricmp(pszMapname, "hl_c17") )
		{
			is_hl_c17 = true;

			edict_t *pNihilanth = NULL;

			if ( (pNihilanth = g_pServerEngineFuncs->pfnFindEntityByString(NULL, "targetname", "nihilanth")) != NULL )
			{
				iNihilanthIndex = g_pServerEngineFuncs->pfnIndexOfEdict( pNihilanth );
				pNihilanthVars = &pNihilanth->v;
			}
		}
		else
		{
			is_hl_c17 = false;
		}

		if ( sc_st_map_start_position.GetString()[0] != '\0' )
		{
			float x, y;

			int nParamsRead = sscanf( sc_st_map_start_position.GetString(), "%f %f", &x, &y );

			if ( nParamsRead >= 2 )
			{
				//Vector vecSpawnPoint;
				//Vector vecOrigin = pcldata->origin;

				//vecSpawnPoint.x = x;
				//vecSpawnPoint.y = y;

				//vecOrigin.z = 0.f;

				//if ( (vecOrigin - vecSpawnPoint).LengthSqr() > M_SQR(16.f) )
				//{
				//	g_pEngineFuncs->ClientCmd("restart\nwait");
				//	return;
				//}

				auto IsAABBIntersectingAABB = [](const Vector &vecBoxMins1, const Vector &vecBoxMaxs1, const Vector &vecBoxMins2, const Vector &vecBoxMaxs2) -> bool
				{
					return ( vecBoxMins1.x <= vecBoxMaxs2.x && vecBoxMaxs1.x >= vecBoxMins2.x ) &&
							( vecBoxMins1.y <= vecBoxMaxs2.y && vecBoxMaxs1.y >= vecBoxMins2.y ) &&
							( vecBoxMins1.z <= vecBoxMaxs2.z && vecBoxMaxs1.z >= vecBoxMins2.z );
				};

				Vector vecSpawnPoint;
				Vector vecSpawnMins, vecSpawnMaxs;
				Vector vecMins, vecMaxs;

				vecSpawnPoint.x = x;
				vecSpawnPoint.y = y;

				vecMins = pcldata->origin + VEC_HULL_MIN;
				vecMaxs = pcldata->origin + VEC_HULL_MAX;
				
				vecSpawnMins = vecSpawnPoint + VEC_HULL_MIN;
				vecSpawnMaxs = vecSpawnPoint + VEC_HULL_MAX;

				vecMins.z = vecMaxs.z = 0.f;
				vecSpawnMins.z = vecSpawnMaxs.z = 0.f;

				if ( !IsAABBIntersectingAABB(vecSpawnMins, vecSpawnMaxs, vecMins, vecMaxs) )
				{
					g_pEngineFuncs->ClientCmd("restart\nwait");
					return;
				}

				//const float radius_sqr = M_SQR(16.f) * sqrtf(2.f);
				//float dmin = 0.f;

				//for (int i = 0; i < 2; i++)
				//{
				//	if ( vecSpawnPoint[i] < vecMins[i] )
				//	{
				//		dmin += M_SQR(vecSpawnPoint[i] - vecMins[i]);
				//	}
				//	else if ( vecSpawnPoint[i] > vecMaxs[i] )
				//	{
				//		dmin += M_SQR(vecSpawnPoint[i] - vecMaxs[i]);
				//	}
				//}

				//if ( dmin > radius_sqr )
				//{
				//	g_pEngineFuncs->ClientCmd("restart\nwait");
				//	return;
				//}
			}
		}
	}

	if ( sc_im_autorecord.GetBool() || sc_im_autoplay.GetBool() )
	{
		if ( !im_file )
		{
			char mapname_buffer[MAX_PATH];

			char *pszMapName = mapname_buffer;
			char *pszExt = NULL;

			strncpy(mapname_buffer, g_pEngineFuncs->GetLevelName(), MAX_PATH);
			mapname_buffer[MAX_PATH - 1] = 0;

			// maps/<mapname>.bsp to <mapname>
			while (*pszMapName)
			{
				if (*pszMapName == '/')
				{
					pszMapName++;
					break;
				}

				pszMapName++;
			}

			pszExt = pszMapName;

			while (*pszExt)
			{
				if (*pszExt == '.')
				{
					*pszExt = 0;
					break;
				}

				pszExt++;
			}

			if ( sc_im_autoplay.GetBool() )
			{
				IM_Play(pszMapName);
			}
			else
			{
				IM_Record(pszMapName);
			}
		}
	}

	StartTimer();
}

//-----------------------------------------------------------------------------
// CSpeedrunTools implementation
//-----------------------------------------------------------------------------

void CSpeedrunTools::Think()
{
	auto FixedUnsigned16 = [](float value, float scale) -> unsigned short
	{
		int output;

		output = value * scale;
		if (output < 0)
			output = 0;
		if (output > 0xFFFF)
			output = 0xFFFF;

		return (unsigned short)output;
	};

	if ( Host_IsServerActive() )
	{
		union
		{
			float un_fl;
			unsigned long un_ul;
		};

		FindCvars();

		g_pServerEngineFuncs->pfnMessageBegin( MSG_BROADCAST, SVC_SVENINT, NULL, NULL );
			g_pServerEngineFuncs->pfnWriteByte( SVENINT_COMM_TIMESCALE );
			g_pServerEngineFuncs->pfnWriteByte( s_bNotifyTimescaleChanged ? 1 : 0 );

			//g_pServerEngineFuncs->pfnWriteShort( FixedUnsigned16( host_framerate->value, 1 << 3 ) );
			//g_pServerEngineFuncs->pfnWriteShort( FixedUnsigned16( fps_max->value, 1 << 3 ) );
			//g_pServerEngineFuncs->pfnWriteShort( FixedUnsigned16( sc_st_min_frametime.GetFloat(), 1 << 3 ) );
			
			un_fl = host_framerate->value;
			g_pServerEngineFuncs->pfnWriteLong( un_ul );

			un_fl = fps_max->value;
			g_pServerEngineFuncs->pfnWriteLong( un_ul );

			un_fl = sc_st_min_frametime.GetFloat();
			g_pServerEngineFuncs->pfnWriteLong( un_ul );
		g_pServerEngineFuncs->pfnMessageEnd();

		s_bNotifyTimescaleChanged = false;
	}

	if ( is_hl_c17 && iNihilanthIndex != 0 )
	{
		edict_t *pNihilanth = g_pServerEngineFuncs->pfnPEntityOfEntIndex( iNihilanthIndex );

		if ( pNihilanth != NULL && &pNihilanth->v == pNihilanthVars )
		{
			float flHealth = pNihilanthVars->health;

			if ( flHealth <= 0.f )
			{
				StopTimer();

				iNihilanthIndex = 0;
				pNihilanthVars = NULL;
			}
		}
		else
		{
			iNihilanthIndex = 0;
			pNihilanthVars = NULL;
		}
	}

	if ( im_file && im_record && im_saved_move && im_version >= 2 )
	{
		size_t length = im_commands.length();

		fwrite(&length, sizeof(size_t), 1, im_file);
		fwrite(im_commands.c_str(), length, 1, im_file);
	}

	im_commands.clear();
}

void CSpeedrunTools::CreateMove(float frametime, struct usercmd_s *cmd, int active)
{
	Vector va;

	if ( (s_bSetAngles || s_bFollowPoint) && (im_file == NULL || im_record) )
	{
		auto ChangeAngleBySpeed = [](float &flAngle, float flTargetAngle, float flChangeSpeed) -> bool
		{
			float normalizedDiff = Strafe::NormalizeDeg(static_cast<double>(flTargetAngle) - flAngle);

			if (std::abs(normalizedDiff) > flChangeSpeed)
			{
				flAngle += std::copysign(flChangeSpeed, normalizedDiff);
				return true;
			}

			flAngle = flTargetAngle;
			return false;
		};

		if ( s_bFollowPoint )
		{
			Vector vecAngles, vecDir;

			g_pEngineFuncs->GetViewAngles(va);

			vecDir = s_vecFollowPoint - (Client()->GetOrigin() + Client()->GetViewOffset());

			vecAngles.x = -atan2f(vecDir.z, vecDir.Length2D()) * (float)(180.0 / M_PI);
			vecAngles.y = atan2f(vecDir.y, vecDir.x) * (float)(180.0 / M_PI);
			vecAngles.z = 0.f;

			float flNormalizedPitch = Strafe::NormalizeDeg(s_vecFollowPoint[PITCH] - va[PITCH]);
			float flNormalizedYaw = Strafe::NormalizeDeg(s_vecFollowPoint[YAW] - va[YAW]);

			float flSetPitchSpeed = std::abs(flNormalizedPitch) * s_flFollowPointLerp;
			float flSetYawSpeed = std::abs(flNormalizedYaw) * s_flFollowPointLerp;

			bool bPitchChanged = ChangeAngleBySpeed( va[PITCH], vecAngles[PITCH], flSetPitchSpeed );
			bool bYawChanged = ChangeAngleBySpeed( va[YAW], vecAngles[YAW], flSetYawSpeed );

			if ( bPitchChanged || bYawChanged )
				g_pEngineFuncs->SetViewAngles(va);
		}
		else if ( s_bSetAngles )
		{
			g_pEngineFuncs->GetViewAngles(va);

			bool bPitchChanged = ChangeAngleBySpeed( va[PITCH], s_vecSetAngles[PITCH], s_vecSetAnglesSpeed[PITCH] );
			bool bYawChanged = ChangeAngleBySpeed( va[YAW], s_vecSetAngles[YAW], s_vecSetAnglesSpeed[YAW] );

			//if ( bPitchChanged )
			//	cmd->viewangles[PITCH] = va[PITCH];

			//if ( bYawChanged )
			//	cmd->viewangles[YAW] = va[YAW];

			if ( !bPitchChanged && !bYawChanged )
				s_bSetAngles = false;
			else
				g_pEngineFuncs->SetViewAngles(va);
		}
	}

	if ( im_file != NULL )
	{
		if ( im_record )
		{
			g_pEngineFuncs->GetViewAngles(va);

			frame_buffer.realviewangles[0] = va[0];
			frame_buffer.realviewangles[1] = va[1];
			frame_buffer.realviewangles[2] = va[2];
			
			frame_buffer.viewangles[0] = cmd->viewangles[0];
			frame_buffer.viewangles[1] = cmd->viewangles[1];
			frame_buffer.viewangles[2] = cmd->viewangles[2];

			frame_buffer.forwardmove = cmd->forwardmove;
			frame_buffer.sidemove = cmd->sidemove;

			frame_buffer.buttons = cmd->buttons;
			frame_buffer.impulse = cmd->impulse;
			frame_buffer.weaponselect = cmd->weaponselect;

			fwrite(&frame_buffer, IM_FRAME_SIZE, 1, im_file);

			im_saved_move = true;
		}
		else
		{
			size_t command_buffer_length;
			size_t bytes = fread(&frame_buffer, 1, IM_FRAME_SIZE, im_file);

			if ( bytes != IM_FRAME_SIZE )
			{
				fclose(im_file);

				im_file = NULL;
				im_record = false;

				Msg("Finished playing back inputs\n");
				
				return;
			}

			if ( im_version >= 2 )
			{
				fread(&command_buffer_length, 1, sizeof(size_t), im_file);

				if ( command_buffer_length > 0 )
				{
					char *command_buffer = (char *)malloc(command_buffer_length + 1);

					Assert( command_buffer != NULL );

					if ( command_buffer != NULL )
					{
						fread(command_buffer, 1, command_buffer_length, im_file);
						command_buffer[command_buffer_length] = '\0';

						g_pEngineFuncs->ClientCmd( command_buffer );
					}
				}
			}

			va[0] = frame_buffer.realviewangles[0];
			va[1] = frame_buffer.realviewangles[1];
			va[2] = frame_buffer.realviewangles[2];

			cmd->viewangles[0] = frame_buffer.viewangles[0];
			cmd->viewangles[1] = frame_buffer.viewangles[1];
			cmd->viewangles[2] = frame_buffer.viewangles[2];

			cmd->forwardmove = frame_buffer.forwardmove;
			cmd->sidemove = frame_buffer.sidemove;

			cmd->buttons = frame_buffer.buttons;
			cmd->impulse = frame_buffer.impulse;
			cmd->weaponselect = frame_buffer.weaponselect;

			g_pEngineFuncs->SetViewAngles(va);
		}
	}
}

void CSpeedrunTools::OnVideoInit()
{
	if (im_file)
	{
		fclose(im_file);
	}

	im_file = NULL;
	im_record = false;

	m_flTimerTime = 0.f;
	m_flLastTimerUpdate = -1.f;
}

void CSpeedrunTools::V_CalcRefDef()
{
	if ( g_Config.cvars.st_player_hulls )
	{
		cl_entity_t *pLocal = g_pEngineFuncs->GetLocalPlayer();

		for (int i = 0; i < MAXENTS; i++)
		{
			//Vector vecScreen;

			model_t *pModel = NULL;
			cl_entity_t *pEntity = g_pEngineFuncs->GetEntityByIndex(i);

			if ( pEntity == NULL || pEntity->curstate.renderfx != (int)kRenderFxDeadPlayer && !pEntity->player )
				continue;

			if ( pEntity->curstate.messagenum < pLocal->curstate.messagenum || pEntity == pLocal )
				continue;

			Render()->DrawBox( pEntity->origin,
							 pEntity->curstate.mins,
							 pEntity->curstate.maxs,
							 g_Config.cvars.st_player_hulls_color[0],
							 g_Config.cvars.st_player_hulls_color[1],
							 g_Config.cvars.st_player_hulls_color[2],
							 g_Config.cvars.st_player_hulls_color[3] );

			//if ( !strstr(pModel->name, "models/player/") )
			//{
			//	bool bVisible = UTIL_WorldToScreen(pEntity->curstate.origin + pEntity->curstate.mins +
			//									   ((pEntity->curstate.origin + pEntity->curstate.maxs) - (pEntity->curstate.origin + pEntity->curstate.mins)) * 0.5f, vecScreen);

			//	if ( bVisible )
			//	{
			//		g_Drawing.DrawStringF( g_hFontESP, vecScreen.x, vecScreen.y, 255, 255, 255, 255, FONT_ALIGN_CENTER, "idx: %d", i );
			//	}
			//}
		}
	}
}

void CSpeedrunTools::OnHUDRedraw(float time)
{
	if ( Host_IsServerActive() )
	{
		float flSegmentTime;

		if ( m_bSegmentStarted )
			flSegmentTime = gpGlobals->time - m_flSegmentStart;
		else
			flSegmentTime = m_flSegmentTime;

		ShowTimer(flSegmentTime, true);

		g_pServerEngineFuncs->pfnMessageBegin(MSG_BROADCAST, SVC_SVENINT, NULL, NULL);
		    g_pServerEngineFuncs->pfnWriteByte(SVENINT_COMM_TIMER);
		    g_pServerEngineFuncs->pfnWriteCoord(flSegmentTime);
		g_pServerEngineFuncs->pfnMessageEnd();
	}
	else if ( m_flLastTimerUpdate > 0.f && *dbRealtime - m_flLastTimerUpdate <= 1.f )
	{
		ShowTimer(m_flTimerTime, false);
	}
}

float CSpeedrunTools::SegmentCurrentTime()
{
	if ( Host_IsServerActive() && m_bSegmentStarted )
	{
		return gpGlobals->time - m_flSegmentStart;
	}

	return 0.f;
}

void CSpeedrunTools::ShowTimer(float flTime, bool bServer)
{
	if ( !bServer )
	{
		m_flTimerTime = flTime;
		m_flLastTimerUpdate = *dbRealtime;
	}

	if ( !g_Config.cvars.st_timer )
		return;

	int minutes = static_cast<int>(flTime) / 60;
	int seconds = static_cast<int>(flTime) % 60;
	int ms = static_cast<int>((flTime - floorf(flTime)) * 1000.f);

	int iSpriteWidth = g_Drawing.GetNumberSpriteWidth();
	int iSpriteHeight = g_Drawing.GetNumberSpriteHeight();

	int iThickness = int((float)iSpriteWidth / 8.f);

	int x = int(g_ScreenInfo.width * g_Config.cvars.st_timer_width_frac);
	int y = int(g_ScreenInfo.height * g_Config.cvars.st_timer_height_frac);

	int r = int(255.f * g_Config.cvars.st_timer_color[0]);
	int g = int(255.f * g_Config.cvars.st_timer_color[1]);
	int b = int(255.f * g_Config.cvars.st_timer_color[2]);
	int a = 232;
			
	x += g_Drawing.DrawDigit(minutes / 10, x, y, r, g, b, FONT_ALIGN_LEFT);
	x += g_Drawing.DrawDigit(minutes % 10, x, y, r, g, b, FONT_ALIGN_LEFT);

	g_Drawing.FillArea(x + (iSpriteWidth / 2) - iThickness,
						y + (iSpriteHeight / 6),
						iThickness * 2,
						iThickness * 2,
						r, g, b, a);

	g_Drawing.FillArea(x + (iSpriteWidth / 2) - iThickness,
						y + iSpriteHeight - (iSpriteHeight / 4),
						iThickness * 2,
						iThickness * 2,
						r, g, b, a);

	x += iSpriteWidth;

	x += g_Drawing.DrawDigit(seconds / 10, x, y, r, g, b, FONT_ALIGN_LEFT);
	x += g_Drawing.DrawDigit(seconds % 10, x, y, r, g, b, FONT_ALIGN_LEFT);

	g_Drawing.FillArea(x + (iSpriteWidth / 2) - iThickness,
						y + iSpriteHeight - (iSpriteHeight / 4),
						iThickness * 2,
						iThickness * 2,
						r, g, b, a);

	x += iSpriteWidth;

	x += g_Drawing.DrawDigit(ms / 100, x, y, r, g, b, FONT_ALIGN_LEFT);
	x += g_Drawing.DrawDigit((ms / 10) % 10, x, y, r, g, b, FONT_ALIGN_LEFT);
	g_Drawing.DrawDigit(ms % 10, x, y, r, g, b, FONT_ALIGN_LEFT);
}

void CSpeedrunTools::StartTimer()
{
	if ( Host_IsServerActive() || g_pDemoAPI->IsPlayingback() )
	{
		m_bSegmentStarted = true;
		m_flSegmentStart = gpGlobals->time;
		m_flSegmentTime = 0.f;

		if ( g_Config.cvars.st_timer )
		{
			ConColorMsg({ 255, 165, 0, 255 }, "> Started segment (map: %s)\n", gpGlobals->pStringBase + gpGlobals->mapname);
		}
	}
	else
	{
		m_bSegmentStarted = false;
		m_flSegmentStart = 0.f;
		m_flSegmentTime = 0.f;
	}
}

void CSpeedrunTools::StopTimer()
{
	if ( g_Config.cvars.st_timer && ( Host_IsServerActive() || g_pDemoAPI->IsPlayingback() ) )
	{
		if ( m_bSegmentStarted )
		{
			char timer_buffer[128];

			float flSegmentTime = m_flSegmentTime = gpGlobals->time - m_flSegmentStart;

			int minutes = static_cast<int>(flSegmentTime) / 60;
			int seconds = static_cast<int>(flSegmentTime) % 60;
			int ms = static_cast<int>((flSegmentTime - floorf(flSegmentTime)) * 1000.f);

			snprintf(timer_buffer, sizeof(timer_buffer) / sizeof(*timer_buffer), "%d%d:%d%d,%d%d%d",
					 minutes / 10, minutes % 10,
					 seconds / 10, seconds % 10,
					 ms / 100, (ms / 10) % 10, ms % 10);

			Utils()->PrintChatText("Finished segment in %s (%.3f) (map: %s)\n", timer_buffer, flSegmentTime, gpGlobals->pStringBase + gpGlobals->mapname);

			ConColorMsg({ 255, 165, 0, 255 }, "> Finished segment in ");
			ConColorMsg({ 179, 255, 32, 255 }, timer_buffer);
			ConColorMsg({ 122, 200, 0, 255 }, " (%.3f) ", flSegmentTime);
			ConColorMsg({ 255, 165, 0, 255 }, "(map: %s)\n", gpGlobals->pStringBase + gpGlobals->mapname);
		}
	}

	m_bSegmentStarted = false;
	m_flSegmentStart = 0.f;
}

void CSpeedrunTools::SetTimescale(float timescale)
{
	FindCvars();

	if ( timescale == 1.f )
	{
		sc_st_min_frametime.SetValue( 0.f );
		CVar()->SetValue( host_framerate, 0.f );
		return;
	}
	else if ( timescale > 1.f )
	{
		Msg("Timescale can't be bigger than 1\n");
		return;
	}
	else if ( timescale <= 0.f )
	{
		Msg("Timescale must be bigger than 0\n");
		return;
	}

	float multiplier = 1.f / timescale;

	sc_st_min_frametime.SetValue( multiplier / fps_max->value );
	CVar()->SetValue( host_framerate, 1.f / fps_max->value );

	Utils()->PrintChatText("<SvenInt> Timescale has been set to %.2f\n", timescale);
}

void CSpeedrunTools::SetTimescale_Comm(bool notify, float framerate, float fpsmax, float min_frametime)
{
	s_bIgnoreCvarChange = true;

	FindCvars();

	CVar()->SetValue( host_framerate, framerate );
	CVar()->SetValue( fps_max, fpsmax );

	sc_st_min_frametime.SetValue( min_frametime );

	if ( notify )
	{
		if ( min_frametime != 0.f )
			Msg("<SvenInt-Comm> Timescale has been changed to %.2f\n", 1.f / ( min_frametime * fpsmax ));
		else
			Msg("<SvenInt-Comm> Timescale has been changed to 1.0\n");
	}

	s_bIgnoreCvarChange = false;
}

void CSpeedrunTools::FindCvars()
{
	if ( host_framerate == NULL )
		host_framerate = CVar()->FindCvar("host_framerate");
	
	if ( fps_max == NULL )
		fps_max = CVar()->FindCvar("fps_max");
}

//-----------------------------------------------------------------------------
// Init
//-----------------------------------------------------------------------------

CSpeedrunTools::CSpeedrunTools()
{
	m_bSegmentStarted = false;

	m_flSegmentTime = 0.f;
	m_flSegmentStart = 0.f;

	m_flTimerTime = 0.f;
	m_flLastTimerUpdate = -1.f;

	m_pJumpOpCode = NULL;
	m_PatchedJumpOpCode = 0x9090;

	m_pfnHost_FilterTime = NULL;
	m_pfnCbuf_AddText = NULL;
	m_pfnServerCmd = NULL;
}

bool CSpeedrunTools::Load()
{
	ud_t inst;

	m_pfnUTIL_GetCircularGaussianSpread = MemoryUtils()->FindPattern( Sys_GetModuleHandle("server.dll"), UTIL_GetCircularGaussianSpread_sig );

	if ( !m_pfnUTIL_GetCircularGaussianSpread )
	{
		Warning("Couldn't find function \"UTIL_GetCircularGaussianSpread\"\n");
		return false;
	}
	
	m_pfnHost_FilterTime = MemoryUtils()->FindPattern( SvenModAPI()->Modules()->Hardware, Host_FilterTime_sig );

	if ( !m_pfnHost_FilterTime )
	{
		Warning("Couldn't find function \"Host_FilterTime\"\n");
		return false;
	}
	
	m_pJumpOpCode = (unsigned short *)MemoryUtils()->FindPatternWithin( SvenModAPI()->Modules()->Hardware,
																		 host_framerate_patch_sig,
																		 m_pfnHost_FilterTime,
																		 (unsigned char *)m_pfnHost_FilterTime + 128 );

	if ( !m_pJumpOpCode )
	{
		Warning("Failed to locate jump condition of \"host_framerate\"\n");
		return false;
	}

	MemoryUtils()->InitDisasm(&inst, g_pEngineFuncs->ServerCmd, 32, 17);

	if ( MemoryUtils()->Disassemble(&inst) )
	{
		if ( inst.mnemonic == UD_Ijmp )
		{
			m_pfnServerCmd = (unsigned char *)MemoryUtils()->CalcAbsoluteAddress( g_pEngineFuncs->ServerCmd );
		}
	}

	if ( m_pfnServerCmd == NULL )
	{
		Warning("Failed to get function \"ServerCmd\"\n");
		return false;
	}

	unsigned char *pfnClientCmd = NULL;

	MemoryUtils()->InitDisasm(&inst, g_pEngineFuncs->ClientCmd, 32, 17);

	if ( MemoryUtils()->Disassemble(&inst) )
	{
		if ( inst.mnemonic == UD_Ijmp )
		{
			pfnClientCmd = (unsigned char *)MemoryUtils()->CalcAbsoluteAddress( g_pEngineFuncs->ClientCmd );
		}
	}

	if ( pfnClientCmd == NULL )
	{
		Warning("Failed to get function \"ClientCmd\"\n");
		return false;
	}

	int iDisassembledBytes = 0;
	MemoryUtils()->InitDisasm(&inst, pfnClientCmd, 32, 24);

	while ( iDisassembledBytes = MemoryUtils()->Disassemble(&inst) )
	{
		if ( inst.mnemonic == UD_Icall )
		{
			m_pfnCbuf_AddText = (unsigned char *)MemoryUtils()->CalcAbsoluteAddress( pfnClientCmd );

			break;
		}

		pfnClientCmd += iDisassembledBytes;
	}

	if ( m_pfnCbuf_AddText == NULL )
	{
		Warning("Failed to get function \"Cbuf_AddText\"\n");
		return false;
	}

	m_PatchedJumpOpCode = *(unsigned short *)m_pJumpOpCode;

	return true;
}

void CSpeedrunTools::PostLoad()
{
	g_WhitelistCommands.Insert( "kill" );
	g_WhitelistCommands.Insert( "stuck_kill" );
	g_WhitelistCommands.Insert( "gibme" );
	g_WhitelistCommands.Insert( "lastinv" );
	g_WhitelistCommands.Insert( "drop" );
	g_WhitelistCommands.Insert( "dropammo" );
	g_WhitelistCommands.Insert( "sc_chasecam" );
	g_WhitelistCommands.Insert( "thirdperson" );
	g_WhitelistCommands.Insert( "firstperson" );
	g_WhitelistCommands.Insert( "npc_moveto" );
	g_WhitelistCommands.Insert( "npc_findcover" );
	g_WhitelistCommands.Insert( "medic" );
	g_WhitelistCommands.Insert( "healme" );
	g_WhitelistCommands.Insert( "grenade" );
	g_WhitelistCommands.Insert( "takecover" );

	Hooks()->HookCvarChange( CVar()->FindCvar("fps_max"), CvarChangeHook_fps_max );

	 m_hUTIL_GetCircularGaussianSpread = DetoursAPI()->DetourFunction( m_pfnUTIL_GetCircularGaussianSpread, HOOKED_UTIL_GetCircularGaussianSpread, GET_FUNC_PTR(ORIG_UTIL_GetCircularGaussianSpread) );
	 m_hHost_FilterTime = DetoursAPI()->DetourFunction( m_pfnHost_FilterTime, HOOKED_Host_FilterTime, GET_FUNC_PTR(ORIG_Host_FilterTime) );
	 m_hCbuf_AddText = DetoursAPI()->DetourFunction( m_pfnCbuf_AddText, HOOKED_Cbuf_AddText, GET_FUNC_PTR(ORIG_Cbuf_AddText) );
	 m_hServerCmd = DetoursAPI()->DetourFunction( m_pfnServerCmd, HOOKED_ServerCmd, GET_FUNC_PTR(ORIG_ServerCmd) );

	 *(unsigned short *)m_pJumpOpCode = 0x9090; // NOP NOP
}

void CSpeedrunTools::Unload()
{
	*(unsigned short *)m_pJumpOpCode = m_PatchedJumpOpCode;

	Hooks()->UnhookCvarChange( CVar()->FindCvar("fps_max"), CvarChangeHook_fps_max );

	DetoursAPI()->RemoveDetour( m_hUTIL_GetCircularGaussianSpread );
	DetoursAPI()->RemoveDetour( m_hHost_FilterTime );
	DetoursAPI()->RemoveDetour( m_hCbuf_AddText );
	DetoursAPI()->RemoveDetour( m_hServerCmd );
}