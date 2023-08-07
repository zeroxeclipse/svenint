// Game Utils

#include "utils.h"
#include "draw_context.h"

#include <regex>

#include <convar.h>
#include <interface.h>
#include <sys.h>
#include <dbg.h>

#include <IClient.h>
#include <IEngineClient.h>
#include <IClientWeapon.h>
#include <IFileSystem.h>
#include <IRender.h>

#include <hl_sdk/engine/APIProxy.h>
#include <hl_sdk/cl_dll/hud.h>

extern CHud *g_pHUD;
extern ref_params_t refparams;
extern bool g_bPlayingbackDemo;

//-----------------------------------------------------------------------------
// Vars
//-----------------------------------------------------------------------------

static float forwardmove, sidemove, upmove; //backup for fixmove
static Vector vViewForward, vViewRight, vViewUp, vAimForward, vAimRight, vAimUp; //backup for fixmove

float *g_flNextCmdTime = NULL;
double *g_dbGameSpeed = NULL;
double *dbRealtime = NULL;

screen_info_t g_ScreenInfo;

//-----------------------------------------------------------------------------
// ConCommands, CVars..
//-----------------------------------------------------------------------------

CON_COMMAND(sc_steamid_to_steam64id, "Converts Steam ID to Steam64 ID, apostrophes \"\" are required")
{
	if (args.ArgC() > 1)
	{
		const char *pszSteamID = args[1];

		std::cmatch match;
		std::regex regex_steamid("^STEAM_[0-5]:([01]):([0-9]+)$");

		if (std::regex_search(pszSteamID, match, regex_steamid))
		{
			uint64_t steamID = 76561197960265728; // base num

			uint64_t v1 = atoll(match[1].str().c_str());
			uint64_t v2 = atoll(match[2].str().c_str());

			steamID += v1 + v2 * 2;

			Msg("Steam64 ID: %llu\n", steamID);
			Msg("https://steamcommunity.com/profiles/%llu\n", steamID);
		}
		else
		{
			Msg("Invalid SteamID, did you forget to write SteamID with apostrophes? ( \"\" )\n");
		}
	}
	else
	{
		ConMsg("Usage:  sc_steamid_to_steam64id <steamid>\n");
	}
}

CON_COMMAND(sc_load_model, "Load a given modelname")
{
	if (args.ArgC() > 1)
	{
		const char *pszModelPath = args[1];
		g_pEngineClient->LoadClientModel( pszModelPath );
	}
}

CON_COMMAND(sc_find_model, "Find a model in which the given name occurs")
{
	if (args.ArgC() > 1)
	{
		const char *pszModelNameOccur = args[1];

		int count = 0;
		FileFindHandle_t hFindHandle;

		const char *pszFilename = g_pFileSystem->FindFirst("models/player/*", &hFindHandle);

		while (pszFilename)
		{
			if ( g_pFileSystem->FindIsDirectory(hFindHandle) && *pszFilename != '.' )
			{
				if ( strstr(pszFilename, pszModelNameOccur) )
				{
					ConMsg("%d. %s\n", ++count, pszFilename);
				}
			}

			pszFilename = g_pFileSystem->FindNext(hFindHandle);
		}

		g_pFileSystem->FindClose(hFindHandle);

		ConMsg("Found %d models\n", count);
	}
	else
	{
		ConMsg("Usage:  sc_find_model <modelname>\n");
	}
}

CON_COMMAND(sc_find_model_starts_with, "Find a model that starts with given name")
{
	if (args.ArgC() > 1)
	{
		const char *pszModelNamePrefix = args[1];

		int count = 0;
		FileFindHandle_t hFindHandle;

		const char *pszFilename = g_pFileSystem->FindFirst("models/player/*", &hFindHandle);

		while (pszFilename)
		{
			if ( g_pFileSystem->FindIsDirectory(hFindHandle) && *pszFilename != '.' )
			{
				if ( !strncmp(pszModelNamePrefix, pszFilename, strlen(pszModelNamePrefix)) )
				{
					ConMsg("%d. %s\n", ++count, pszFilename);
				}
			}

			pszFilename = g_pFileSystem->FindNext(hFindHandle);
		}

		g_pFileSystem->FindClose(hFindHandle);

		ConMsg("Found %d models\n", count);
	}
	else
	{
		ConMsg("Usage:  sc_find_model_starts_with <modelname>\n");
	}
}

CON_COMMAND(getpos, "Prints current origin")
{
	Vector vecOrigin = ( g_bPlayingbackDemo ? refparams.simorg : g_pPlayerMove->origin );

	Warning( "setpos %.6f %.6f %.6f\n", VectorExpand( vecOrigin ) );
}

CON_COMMAND(getpos_exact, "Prints current origin from view angles")
{
	Vector vecEyes = ( g_bPlayingbackDemo ? *(Vector *)refparams.simorg + refparams.viewheight : g_pPlayerMove->origin + g_pPlayerMove->view_ofs );

	Warning( "setpos_exact %.6f %.6f %.6f\n", VectorExpand( vecEyes ) );
}

CON_COMMAND(getang, "Prints current view angles")
{
	Vector va;
	g_pEngineFuncs->GetViewAngles(va);

	NormalizeAngles( va );

	Warning("setang %.6f %.6f %.6f\n", VectorExpand(va));
}

CON_COMMAND(setang, "Sets view angles")
{
	if (args.ArgC() >= 2)
	{
		Vector va;
		float x, y, z;

		x = atof(args[1]);

		g_pEngineFuncs->GetViewAngles(va);

		if (args.ArgC() >= 3)
		{
			y = atof(args[2]);

			if (args.ArgC() >= 4)
			{
				z = atof(args[3]);
			}
			else
			{
				z = va.z;
			}
		}
		else
		{
			y = va.y;
			z = va.z;
		}

		va.x = x;
		va.y = y;
		va.z = z;

		g_pEngineFuncs->SetViewAngles( va );
	}
	else
	{
		Msg("Usage:  setang <x> <optional: y> <optional: z>\n");
	}
}

CON_COMMAND( sc_debug_draw_point, "" )
{
	if ( args.ArgC() >= 10 )
	{
		Vector vecPoint( atof( args[ 1 ] ), atof( args[ 2 ] ), atof( args[ 3 ] ) );

		unsigned char r = atoi( args[ 4 ] );
		unsigned char g = atoi( args[ 5 ] );
		unsigned char b = atoi( args[ 6 ] );
		unsigned char a = atoi( args[ 7 ] );

		float size = atoi( args[ 8 ] );
		float duration = atoi( args[ 9 ] );

		Render()->DrawPoint( vecPoint, { r, g, b, a }, size, duration );
	}
}

CON_COMMAND( sc_debug_draw_line, "" )
{
	if ( args.ArgC() >= 13 )
	{
		Vector vecStart( atof( args[ 1 ] ), atof( args[ 2 ] ), atof( args[ 3 ] ) );
		Vector vecEnd( atof( args[ 4 ] ), atof( args[ 5 ] ), atof( args[ 6 ] ) );

		unsigned char r = atoi( args[ 7 ] );
		unsigned char g = atoi( args[ 8 ] );
		unsigned char b = atoi( args[ 9 ] );
		unsigned char a = atoi( args[ 10 ] );

		float width = atoi( args[ 11 ] );
		float duration = atoi( args[ 12 ] );

		Render()->DrawLine( vecStart, vecEnd, { r, g, b, a }, width, duration );
	}
}

CON_COMMAND( sc_debug_draw_box, "" )
{
	if ( args.ArgC() >= 16 )
	{
		Vector vOrigin( atof( args[ 1 ] ), atof( args[ 2 ] ), atof( args[ 3 ] ) );
		Vector vMins( atof( args[ 4 ] ), atof( args[ 5 ] ), atof( args[ 6 ] ) );
		Vector vMaxs( atof( args[ 7 ] ), atof( args[ 8 ] ), atof( args[ 9 ] ) );

		//unsigned char r = atoi( args[ 10 ] );
		//unsigned char g = atoi( args[ 11 ] );
		//unsigned char b = atoi( args[ 12 ] );
		//unsigned char a = atoi( args[ 13 ] );

		bool wireframe = !!atoi( args[ 14 ] );
		float duration = atoi( args[ 15 ] );

		DrawBox( vOrigin,
				 vMins,
				 vMaxs,
				 atof( args[ 10 ] ) / 255.f,
				 atof( args[ 11 ] ) / 255.f,
				 atof( args[ 12 ] ) / 255.f,
				 atof( args[ 13 ] ) / 255.f,
				 4.f,
				 wireframe,
				 duration );

		//Render()->DrawBox( vOrigin, vMins, vMaxs, { r, g, b, a }, duration );
	}
}

CON_COMMAND( sc_debug_draw_box_angles, "" )
{
	if ( args.ArgC() >= 18 )
	{
		Vector vOrigin( atof( args[ 1 ] ), atof( args[ 2 ] ), atof( args[ 3 ] ) );
		Vector vMins( atof( args[ 4 ] ), atof( args[ 5 ] ), atof( args[ 6 ] ) );
		Vector vMaxs( atof( args[ 7 ] ), atof( args[ 8 ] ), atof( args[ 9 ] ) );
		Vector vAngles( atof( args[ 10 ] ), atof( args[ 11 ] ), atof( args[ 12 ] ) );

		//unsigned char r = atoi( args[ 13 ] );
		//unsigned char g = atoi( args[ 14 ] );
		//unsigned char b = atoi( args[ 15 ] );
		//unsigned char a = atoi( args[ 16 ] );

		bool wireframe = !!atoi( args[ 17 ] );
		float duration = atoi( args[ 18 ] );

		DrawBoxAngles( vOrigin,
					   vMins,
					   vMaxs,
					   vAngles,
					   atof( args[ 13 ] ) / 255.f,
					   atof( args[ 14 ] ) / 255.f,
					   atof( args[ 15 ] ) / 255.f,
					   atof( args[ 16 ] ) / 255.f,
					   4.f,
					   wireframe,
					   duration );

		//Render()->DrawBoxAngles( vOrigin, vMins, vMaxs, vAngles, { r, g, b, a }, duration );
	}
}

CON_COMMAND( sc_debug_draw_clear, "" )
{
	Render()->DrawClear();
}

//-----------------------------------------------------------------------------
// Client utilities
//-----------------------------------------------------------------------------

PM_PlayerTraceFn PM_PlayerTrace = NULL;

bool UTIL_IsDead( void )
{
	// Dead or spectating
	return g_bPlayingbackDemo ?
		( refparams.health == 0 ) || ( g_pHUD != NULL && ( *( (int *)g_pHUD + 21 ) & HIDEHUD_HEALTH ) ) :
		( g_pPlayerMove->iuser1 != 0 || g_pPlayerMove->dead );
}

bool UTIL_IsSpectating( void )
{
	return g_bPlayingbackDemo ? ( g_pHUD != NULL && ( *( (int *)g_pHUD + 21 ) & HIDEHUD_HEALTH ) ) : ( g_pPlayerMove->iuser1 != 0 );
}

int UTIL_GetLocalPlayerIndex( void )
{
	return g_bPlayingbackDemo ? g_pEngineFuncs->GetLocalPlayer()->index : g_pPlayerMove->player_index + 1;
}

//-----------------------------------------------------------------------------
// Intersection tests
//-----------------------------------------------------------------------------

bool UTIL_IsLineIntersectingAABB( const Vector &p1, const Vector &p2, const Vector &vecBoxMins, const Vector &vecBoxMaxs )
{
	Vector vecLineDir = ( p2 - p1 ) * 0.5f;
	Vector vecBoxMid = ( vecBoxMaxs - vecBoxMins ) * 0.5f;
	Vector p3 = p1 + vecLineDir - ( vecBoxMins + vecBoxMaxs ) * 0.5f;
	Vector vecAbsLineMid = Vector( abs( vecLineDir.x ), abs( vecLineDir.y ), abs( vecLineDir.z ) );

	if ( abs( p3.x ) > vecBoxMid.x + vecAbsLineMid.x || abs( p3.y ) > vecBoxMid.y + vecAbsLineMid.y || abs( p3.z ) > vecBoxMid.z + vecAbsLineMid.z )
		return false;

	if ( abs( vecLineDir.y * p3.z - vecLineDir.z * p3.y ) > vecBoxMid.y * vecAbsLineMid.z + vecBoxMid.z * vecAbsLineMid.y )
		return false;

	if ( abs( vecLineDir.z * p3.x - vecLineDir.x * p3.z ) > vecBoxMid.z * vecAbsLineMid.x + vecBoxMid.x * vecAbsLineMid.z )
		return false;

	if ( abs( vecLineDir.x * p3.y - vecLineDir.y * p3.x ) > vecBoxMid.x * vecAbsLineMid.y + vecBoxMid.y * vecAbsLineMid.x )
		return false;

	return true;
}

bool UTIL_IsSphereIntersectingAABB( const Vector &vecCenter, const float flRadiusSqr, const Vector &vecAbsMins, const Vector &vecAbsMaxs, float *pflOutDistance )
{
	float flDistanceToEdge = 0.f;
	float flDistanceSqr = 0.f;

	for ( int i = 0; i < 3 /* && flDistanceSqr <= flRadiusSqr */; i++ )
	{
		if ( vecCenter[ i ] < vecAbsMins[ i ] )
		{
			flDistanceToEdge = vecCenter[ i ] - vecAbsMins[ i ];
		}
		else if ( vecCenter[ i ] > vecAbsMaxs[ i ] )
		{
			flDistanceToEdge = vecCenter[ i ] - vecAbsMaxs[ i ];
		}
		else
		{
			flDistanceToEdge = 0.f;
		}

		flDistanceSqr += flDistanceToEdge * flDistanceToEdge;
	}

	if ( flDistanceSqr <= flRadiusSqr )
	{
		if ( pflOutDistance != NULL )
			*pflOutDistance = sqrtf( flDistanceSqr );

		return true;
	}

	if ( pflOutDistance != NULL )
		*pflOutDistance = -1.f;

	return false;
}

bool UTIL_IsRayIntersectingAABB( const Vector &vecBoxMins, const Vector &vecBoxMaxs, const Vector &vecRayOrigin, const Vector &vecRayDir, float *pflMinIntersection, float *pflMaxIntersection )
{
	float tmin, tmax, tymin, tymax, tzmin, tzmax;

	if ( pflMinIntersection != NULL )
		*pflMinIntersection = 0.f;

	if ( pflMaxIntersection != NULL )
		*pflMaxIntersection = 0.f;

	if ( vecRayDir.x >= 0 )
	{
		tmin = ( vecBoxMins.x - vecRayOrigin.x ) / vecRayDir.x;
		tmax = ( vecBoxMaxs.x - vecRayOrigin.x ) / vecRayDir.x;
	}
	else
	{
		tmin = ( vecBoxMaxs.x - vecRayOrigin.x ) / vecRayDir.x;
		tmax = ( vecBoxMins.x - vecRayOrigin.x ) / vecRayDir.x;
	}

	if ( vecRayDir.y >= 0 )
	{
		tymin = ( vecBoxMins.y - vecRayOrigin.y ) / vecRayDir.y;
		tymax = ( vecBoxMaxs.y - vecRayOrigin.y ) / vecRayDir.y;
	}
	else
	{
		tymin = ( vecBoxMaxs.y - vecRayOrigin.y ) / vecRayDir.y;
		tymax = ( vecBoxMins.y - vecRayOrigin.y ) / vecRayDir.y;
	}

	if ( tmin > tymax || tymin > tmax )
		return false;

	if ( tymin > tmin )
		tmin = tymin;

	if ( tymax < tmax )
		tmax = tymax;

	if ( vecRayDir.z >= 0 )
	{
		tzmin = ( vecBoxMins.z - vecRayOrigin.z ) / vecRayDir.z;
		tzmax = ( vecBoxMaxs.z - vecRayOrigin.z ) / vecRayDir.z;
	}
	else
	{
		tzmin = ( vecBoxMaxs.z - vecRayOrigin.z ) / vecRayDir.z;
		tzmax = ( vecBoxMins.z - vecRayOrigin.z ) / vecRayDir.z;
	}

	if ( tmin > tzmax || tzmin > tmax )
		return false;

	if ( tzmin > tmin )
		tmin = tzmin;

	if ( tzmax < tmax )
		tmax = tzmax;

	if ( pflMinIntersection != NULL )
		*pflMinIntersection = tmin;
	
	if ( pflMaxIntersection != NULL )
		*pflMaxIntersection = tmax;

	return true;
}

//-----------------------------------------------------------------------------
// Shared Random
//-----------------------------------------------------------------------------

static unsigned int glSeed = 0;

static unsigned int seed_table[256] =
{
	28985U, 27138U, 26457U, 9451U, 17764U, 10909U, 28790U, 8716U, 6361U, 4853U, 17798U, 21977U, 19643U, 20662U, 10834U, 20103,
	27067U, 28634U, 18623U, 25849U, 8576U, 26234U, 23887U, 18228U, 32587U, 4836U, 3306U, 1811U, 3035U, 24559U, 18399U, 315,
	26766U, 907U, 24102U, 12370U, 9674U, 2972U, 10472U, 16492U, 22683U, 11529U, 27968U, 30406U, 13213U, 2319U, 23620U, 16823,
	10013U, 23772U, 21567U, 1251U, 19579U, 20313U, 18241U, 30130U, 8402U, 20807U, 27354U, 7169U, 21211U, 17293U, 5410U, 19223,
	10255U, 22480U, 27388U, 9946U, 15628U, 24389U, 17308U, 2370U, 9530U, 31683U, 25927U, 23567U, 11694U, 26397U, 32602U, 15031,
	18255U, 17582U, 1422U, 28835U, 23607U, 12597U, 20602U, 10138U, 5212U, 1252U, 10074U, 23166U, 19823U, 31667U, 5902U, 24630,
	18948U, 14330U, 14950U, 8939U, 23540U, 21311U, 22428U, 22391U, 3583U, 29004U, 30498U, 18714U, 4278U, 2437U, 22430U, 3439,
	28313U, 23161U, 25396U, 13471U, 19324U, 15287U, 2563U, 18901U, 13103U, 16867U, 9714U, 14322U, 15197U, 26889U, 19372U, 26241,
	31925U, 14640U, 11497U, 8941U, 10056U, 6451U, 28656U, 10737U, 13874U, 17356U, 8281U, 25937U, 1661U, 4850U, 7448U, 12744,
	21826U, 5477U, 10167U, 16705U, 26897U, 8839U, 30947U, 27978U, 27283U, 24685U, 32298U, 3525U, 12398U, 28726U, 9475U, 10208,
	617U, 13467U, 22287U, 2376U, 6097U, 26312U, 2974U, 9114U, 21787U, 28010U, 4725U, 15387U, 3274U, 10762U, 31695U, 17320,
	18324U, 12441U, 16801U, 27376U, 22464U, 7500U, 5666U, 18144U, 15314U, 31914U, 31627U, 6495U, 5226U, 31203U, 2331U, 4668,
	12650U, 18275U, 351U, 7268U, 31319U, 30119U, 7600U, 2905U, 13826U, 11343U, 13053U, 15583U, 30055U, 31093U, 5067U, 761,
	9685U, 11070U, 21369U, 27155U, 3663U, 26542U, 20169U, 12161U, 15411U, 30401U, 7580U, 31784U, 8985U, 29367U, 20989U, 14203,
	29694U, 21167U, 10337U, 1706U, 28578U, 887U, 3373U, 19477U, 14382U, 675U, 7033U, 15111U, 26138U, 12252U, 30996U, 21409,
	25678U, 18555U, 13256U, 23316U, 22407U, 16727U, 991U, 9236U, 5373U, 29402U, 6117U, 15241U, 27715U, 19291U, 19888U, 19847U
};

static unsigned int U_Random()
{
	glSeed *= 69069;
	glSeed += seed_table[glSeed & 0xFF] + 1;
	return (glSeed & 0xFFFFFFF);
}

static void U_Srand(unsigned int seed)
{
	glSeed = seed_table[seed & 0xFF];
}

int UTIL_SharedRandomLong(unsigned int seed, int low, int high)
{
	unsigned int range = high - low + 1;
	U_Srand((unsigned int)(high + low + seed));
	if (range != 1)
	{
		int rnum = U_Random();
		int offset = rnum % range;
		return (low + offset);
	}

	return low;
}

float UTIL_SharedRandomFloat(unsigned int seed, float low, float high)
{
	unsigned int range = high - low;
	U_Srand((unsigned int)seed + *(unsigned int *)&low + *(unsigned int *)&high);

	U_Random();
	U_Random();

	if (range)
	{
		int tensixrand = U_Random() & 0xFFFFu;
		float offset = float(tensixrand) / 0x10000u;
		return (low + offset * range);
	}

	return low;

	//int v3; // ecx
	//int v4; // ecx
	//int v5; // ecx

	//v3 = 69069 * seed_table[(unsigned __int8)(seed + LOBYTE(low) + LOBYTE(high))];
	//v4 = 69069 * (v3 + 1 + seed_table[(unsigned __int8)v3]);
	//v5 = seed_table[(unsigned __int8)v4] + v4 + 1;
	//glSeed = v5;
	//if ((float)(high - low) == 0.0)
	//	return low;
	//glSeed = 69069 * v5 + seed_table[(unsigned __int8)(-51 * v5)] + 1;
	//return (float)((float)((float)((float)(unsigned __int16)glSeed * 0.0000152587890625) * (float)(high - low)) + low);
}

//-----------------------------------------------------------------------------
// char * to wchar_t *
//-----------------------------------------------------------------------------

const wchar_t *UTIL_CStringToWideCString(const char *pszString)
{
	const size_t length = strlen(pszString) + 1;
	wchar_t *wcString = new wchar_t[length];

	mbstowcs(wcString, pszString, length);

	return wcString;
}

//-----------------------------------------------------------------------------
// Player move utilities
//-----------------------------------------------------------------------------

int UTIL_ClipVelocity( const Vector &in, const Vector &normal, Vector &out, float overbounce )
{
	float	backoff;
	float	change;
	float angle;
	int		i, blocked;

	const float STOP_EPSILON = 0.1f;

	angle = normal[ 2 ];

	blocked = 0x00;            // Assume unblocked.
	if ( angle > 0 )      // If the plane that is blocking us has a positive z component, then assume it's a floor.
		blocked |= 0x01;		// 
	if ( !angle )         // If the plane has no Z, it is vertical (wall/step)
		blocked |= 0x02;		// 

	// Determine how far along plane to slide based on incoming direction.
	// Scale by overbounce factor.
	backoff = DotProduct( in, normal ) * overbounce;

	for ( i = 0; i < 3; i++ )
	{
		change = normal[ i ] * backoff;
		out[ i ] = in[ i ] - change;
		// If out velocity is too small, zero it out.
		if ( out[ i ] > -STOP_EPSILON && out[ i ] < STOP_EPSILON )
			out[ i ] = 0.f;
	}

	// Return blocking flags.
	return blocked;
}

void UTIL_AddCorrectGravity( Vector &vecVelocity, float frametime )
{
	float ent_gravity;

	if ( g_pPlayerMove->gravity )
		ent_gravity = g_pPlayerMove->gravity;
	else
		ent_gravity = 1.f;

	// Add gravity so they'll be in the correct position during movement
	// yes, this 0.5 looks wrong, but it's not.  
	vecVelocity.z -= ( ent_gravity * g_pPlayerMove->movevars->gravity * 0.5 * frametime );
}

void UTIL_FixupGravityVelocity( Vector &vecVelocity, float frametime )
{
	float ent_gravity;

	if ( g_pPlayerMove->gravity )
		ent_gravity = g_pPlayerMove->gravity;
	else
		ent_gravity = 1.f;

	vecVelocity.z -= ( ent_gravity * g_pPlayerMove->movevars->gravity * frametime * 0.5f );
}

void UTIL_AddCorrectGravity( Vector &vecVelocity, float gravity, float entgravity, float frametime )
{
	float ent_gravity;

	if ( entgravity )
		ent_gravity = entgravity;
	else
		ent_gravity = 1.f;

	// Add gravity so they'll be in the correct position during movement
	// yes, this 0.5 looks wrong, but it's not.  
	vecVelocity.z -= ( ent_gravity * gravity * 0.5 * frametime );
}

void UTIL_FixupGravityVelocity( Vector &vecVelocity, float gravity, float entgravity, float frametime )
{
	float ent_gravity;

	if ( entgravity )
		ent_gravity = entgravity;
	else
		ent_gravity = 1.f;

	vecVelocity.z -= ( ent_gravity * gravity * frametime * 0.5f );
}

//-----------------------------------------------------------------------------
// Hull intersection
//-----------------------------------------------------------------------------

void UTIL_FindHullIntersection( const Vector &vecSrc, TraceResult &tr, float *mins, float *maxs, edict_t *pEntity )
{
	int			i, j, k;
	float		distance;
	float *minmaxs[ 2 ] = { mins, maxs };
	TraceResult tmpTrace;
	Vector		vecHullEnd = tr.vecEndPos;
	Vector		vecEnd;

	distance = 1e6f;

	vecHullEnd = vecSrc + ( ( vecHullEnd - vecSrc ) * 2 );
	g_pServerEngineFuncs->pfnTraceLine( vecSrc, vecHullEnd, 0, pEntity, &tmpTrace );

	if ( tmpTrace.flFraction < 1.0 )
	{
		tr = tmpTrace;
		return;
	}

	for ( i = 0; i < 2; i++ )
	{
		for ( j = 0; j < 2; j++ )
		{
			for ( k = 0; k < 2; k++ )
			{
				vecEnd.x = vecHullEnd.x + minmaxs[ i ][ 0 ];
				vecEnd.y = vecHullEnd.y + minmaxs[ j ][ 1 ];
				vecEnd.z = vecHullEnd.z + minmaxs[ k ][ 2 ];

				g_pServerEngineFuncs->pfnTraceLine( vecSrc, vecEnd, 0, pEntity, &tmpTrace );
				if ( tmpTrace.flFraction < 1.0 )
				{
					float thisDistance = ( tmpTrace.vecEndPos - vecSrc ).Length();
					if ( thisDistance < distance )
					{
						tr = tmpTrace;
						distance = thisDistance;
					}
				}
			}
		}
	}
}

void UTIL_FindHullIntersectionClient( const Vector &vecSrc, pmtrace_t &tr, float *mins, float *maxs, int ignore_ent )
{
	int			i, j, k;
	float		distance;
	float		*minmaxs[ 2 ] = { mins, maxs };
	pmtrace_t	tmpTrace;
	Vector		vecHullEnd = tr.endpos;
	Vector		vecEnd;

	distance = 1e6f;

	vecHullEnd = vecSrc + ( ( vecHullEnd - vecSrc ) * 2 );

	g_pEventAPI->EV_SetTraceHull( PM_HULL_POINT );
	g_pEventAPI->EV_PlayerTrace( (float *)&vecSrc, vecHullEnd, PM_NORMAL, ignore_ent, &tmpTrace );

	if ( tmpTrace.fraction < 1.0 )
	{
		tr = tmpTrace;
		return;
	}

	for ( i = 0; i < 2; i++ )
	{
		for ( j = 0; j < 2; j++ )
		{
			for ( k = 0; k < 2; k++ )
			{
				vecEnd.x = vecHullEnd.x + minmaxs[ i ][ 0 ];
				vecEnd.y = vecHullEnd.y + minmaxs[ j ][ 1 ];
				vecEnd.z = vecHullEnd.z + minmaxs[ k ][ 2 ];

				g_pEventAPI->EV_SetTraceHull( PM_HULL_POINT );
				g_pEventAPI->EV_PlayerTrace( (float *)&vecSrc, vecEnd, PM_NORMAL, ignore_ent, &tmpTrace );

				if ( tmpTrace.fraction < 1.0 )
				{
					float thisDistance = ( tmpTrace.endpos - vecSrc ).Length();

					if ( thisDistance < distance )
					{
						tr = tmpTrace;
						distance = thisDistance;
					}
				}
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Viewport transformations
//-----------------------------------------------------------------------------

bool UTIL_WorldToScreen(float *pflOrigin, float *pflVecScreen)
{
	int iResult = g_pTriangleAPI->WorldToScreen(pflOrigin, pflVecScreen);

	if (!iResult && pflVecScreen[0] <= 1 && pflVecScreen[1] <= 1 && pflVecScreen[0] >= -1 && pflVecScreen[1] >= -1)
	{
		pflVecScreen[0] = (g_ScreenInfo.width / 2 * pflVecScreen[0]) + (pflVecScreen[0] + g_ScreenInfo.width / 2);
		pflVecScreen[1] = -(g_ScreenInfo.height / 2 * pflVecScreen[1]) + (pflVecScreen[1] + g_ScreenInfo.height / 2);

		return true;
	}

	return false;
}

void UTIL_ScreenToWorld(float *pflNDC, float *pflWorldOrigin)
{
	g_pTriangleAPI->ScreenToWorld(pflNDC, pflWorldOrigin);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

static void FixMoveStart(struct usercmd_s *cmd)
{
	forwardmove = cmd->forwardmove;
	sidemove = cmd->sidemove;
	upmove = cmd->upmove;

	if (g_pPlayerMove->iuser1 == 0)
		g_pEngineFuncs->AngleVectors(Vector(0.0f, cmd->viewangles.y, 0.0f), vViewForward, vViewRight, vViewUp);
	else
		g_pEngineFuncs->AngleVectors(cmd->viewangles, vViewForward, vViewRight, vViewUp);
}

static void FixMoveEnd(struct usercmd_s *cmd)
{
	NormalizeAngles(cmd->viewangles);

	if (g_pPlayerMove->iuser1 == 0)
		g_pEngineFuncs->AngleVectors(Vector(0.0f, cmd->viewangles.y, 0.0f), vAimForward, vAimRight, vAimUp);
	else
		g_pEngineFuncs->AngleVectors(cmd->viewangles, vAimForward, vAimRight, vAimUp);

	Vector forwardmove_normalized = vViewForward * forwardmove;
	Vector sidemove_normalized = vViewRight * sidemove;
	Vector upmove_normalized = vViewUp * upmove;

	cmd->forwardmove = DotProduct(forwardmove_normalized, vAimForward) + DotProduct(sidemove_normalized, vAimForward) + DotProduct(upmove_normalized, vAimForward);
	cmd->sidemove = DotProduct(forwardmove_normalized, vAimRight) + DotProduct(sidemove_normalized, vAimRight) + DotProduct(upmove_normalized, vAimRight);
	cmd->upmove = DotProduct(forwardmove_normalized, vAimUp) + DotProduct(sidemove_normalized, vAimUp) + DotProduct(upmove_normalized, vAimUp);

	Vector vMove(cmd->forwardmove, cmd->sidemove, cmd->upmove);
	float flSpeed = sqrtf(vMove.x * vMove.x + vMove.y * vMove.y), flYaw;
	Vector vecMove, vecRealView(cmd->viewangles);
	VectorAngles(vMove, vecMove);
	flYaw = (cmd->viewangles.y - vecRealView.y + vecMove.y) * static_cast<float>(M_PI) / 180.0f;

	cmd->forwardmove = cosf(flYaw) * flSpeed;

	if (cmd->viewangles.x >= 90.f || cmd->viewangles.x <= -90.f)
		cmd->forwardmove *= -1;

	cmd->sidemove = sinf(flYaw) * flSpeed;
}

void UTIL_SetAnglesSilent(float *angles, struct usercmd_s *cmd)
{
	FixMoveStart(cmd);

	cmd->viewangles[0] = angles[0];
	cmd->viewangles[1] = angles[1];
	cmd->viewangles[2] = angles[2];

	FixMoveEnd(cmd);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

bool UTIL_IsContiniousFiring(struct usercmd_s *cmd)
{
	if ( Client()->HasWeapon() )
	{
		switch ( Client()->GetCurrentWeaponID() )
		{
		case WEAPON_M16:
			if ( ClientWeapon()->GetWeaponData()->fuser2 != 0.f )
				return true;

			break;
		}
	}

	return false;
}

bool UTIL_IsFiring(struct usercmd_s *cmd)
{
	static int throw_nade_state = 0;

	if ( Client()->HasWeapon() )
	{
		switch ( Client()->GetCurrentWeaponID() )
		{
		case WEAPON_M16:
			if ( ClientWeapon()->GetWeaponData()->fuser2 != 0.f )
				return true;

			break;

		case WEAPON_RPG:
			if ( ClientWeapon()->GetWeaponData()->iuser4 && ClientWeapon()->GetWeaponData()->fuser1 != 0.f )
				return true;

			if ( cmd->buttons & IN_ATTACK2 )
				return false;

			break;

		case WEAPON_GAUSS:
			if ( ClientWeapon()->GetWeaponData()->fuser4 > 0.f )
			{
				if ( Client()->ButtonLast() & IN_ATTACK2 )
				{
					if ( !(cmd->buttons & IN_ATTACK2) )
						return true;
				}
				else if ( Client()->ButtonLast() & IN_ALT1 )
				{
					if ( !(cmd->buttons & IN_ALT1) )
						return true;
				}
				else if ( ClientWeapon()->GetWeaponData()->fuser4 == 1.f )
				{
					return true;
				}

				return false;
			}
			else if ( cmd->buttons & IN_ATTACK2 )
			{
				return false;
			}

			break;

		case WEAPON_HANDGRENADE:
			if ( ClientWeapon()->GetWeaponData()->fuser1 < 0.f && throw_nade_state != 2 )
			{
				throw_nade_state = 1;

				if ( Client()->ButtonLast() & (IN_ATTACK | IN_ATTACK2) )
				{
					if ( !(cmd->buttons & (IN_ATTACK | IN_ATTACK2)) )
						return true;
				}
				else
				{
					if ( !(cmd->buttons & (IN_ATTACK | IN_ATTACK2)) )
						throw_nade_state = 2;
				}
			}

			if ( ClientWeapon()->GetWeaponData()->fuser2 < 0.f && throw_nade_state == 2 )
				return true;

			throw_nade_state = 0;
			return false;

		case WEAPON_DISPLACER:
			if ( ClientWeapon()->GetWeaponData()->fuser1 == 1.f )
				return true;

			return false;
		}

		if ( cmd->buttons & (IN_ATTACK | IN_ATTACK2) && Client()->CanAttack() && !ClientWeapon()->IsReloading() )
		{
			if (cmd->buttons & IN_ATTACK)
			{
				if ( ClientWeapon()->CanPrimaryAttack() )
					return true;
			}
			else
			{
				if ( ClientWeapon()->CanSecondaryAttack() )
					return true;
			}
		}
	}

	return false;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

void UTIL_SetGameSpeed(double dbSpeed)
{
	*g_dbGameSpeed = dbSpeed * 1000.0;
}

void UTIL_SendPacket(bool bSend)
{
	if (bSend)
		*g_flNextCmdTime = 0.0f;
	else
		*g_flNextCmdTime = FLT_MAX;
}