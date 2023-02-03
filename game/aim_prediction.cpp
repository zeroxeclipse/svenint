// Aim Prediction

#include "aim_prediction.h"
#include "../game/utils.h"

#include <hl_sdk/common/Platform.h>
#include <ISvenModAPI.h>

#define	DEFAULT_VIEWHEIGHT	28
#define VEC_DUCK_VIEW 12

#define VECTOR_CONE_1DEGREES	Vector( 0.00873, 0.00873, 0.00873 )
#define VECTOR_CONE_2DEGREES	Vector( 0.01745, 0.01745, 0.01745 )
#define VECTOR_CONE_3DEGREES	Vector( 0.02618, 0.02618, 0.02618 )
#define VECTOR_CONE_4DEGREES	Vector( 0.03490, 0.03490, 0.03490 )
#define VECTOR_CONE_5DEGREES	Vector( 0.04362, 0.04362, 0.04362 )
#define VECTOR_CONE_6DEGREES	Vector( 0.05234, 0.05234, 0.05234 )
#define VECTOR_CONE_7DEGREES	Vector( 0.06105, 0.06105, 0.06105 )
#define VECTOR_CONE_8DEGREES	Vector( 0.06976, 0.06976, 0.06976 )
#define VECTOR_CONE_9DEGREES	Vector( 0.07846, 0.07846, 0.07846 )
#define VECTOR_CONE_10DEGREES	Vector( 0.08716, 0.08716, 0.08716 )
#define VECTOR_CONE_15DEGREES	Vector( 0.13053, 0.13053, 0.13053 )
#define VECTOR_CONE_20DEGREES	Vector( 0.17365, 0.17365, 0.17365 )

typedef	enum
{
	BULLET_NONE = 0,
	BULLET_UNKNOWN_1,
	BULLET_UNKNOWN_2,
	BULLET_UNKNOWN_3,
	BULLET_UNKNOWN_4,
	BULLET_PLAYER_357, // python
} Bullet;

//-----------------------------------------------------------------------------
// Event Utils
//-----------------------------------------------------------------------------

static qboolean EV_IsPlayer(int idx)
{
	if ( idx >= 1 && idx <= g_pEngineFuncs->GetMaxClients() )
		return true;

	return false;
}

qboolean EV_IsLocal(int idx)
{
	return g_pEventAPI->EV_IsLocal(idx - 1) ? true : false;
}

static void EV_GetGunPosition( event_args_t *args, Vector &pos, Vector &origin )
{
	int idx;
	Vector view_ofs;

	idx = args->entindex;

	VectorClear( view_ofs );
	view_ofs[2] = DEFAULT_VIEWHEIGHT;

	if ( EV_IsPlayer( idx ) )
	{
		// in spec mode use entity viewheigh, not own
		if ( EV_IsLocal( idx ) )
		{
			// Grab predicted result for local player
			g_pEventAPI->EV_LocalPlayerViewheight( view_ofs );
		}
		else if ( args->ducking == 1 )
		{
			view_ofs[2] = VEC_DUCK_VIEW;
		}
	}

	VectorAdd( origin, view_ofs, pos );
}

static void BulletAccuracy(Vector &vecBulletSpread, const Vector &vecConeMoving, const Vector &vecConeStanding, const Vector &vecConeCrouched, event_args_s *args)
{
	if ( args->velocity[0] == 0.f && args->velocity[1] == 0.f && args->velocity[2] == 0.f )
	{
		if ( args->ducking == 1 )
			memcpy( &vecBulletSpread, &vecConeCrouched, sizeof(Vector) );
		else
			memcpy( &vecBulletSpread, &vecConeStanding, sizeof(Vector) );
	}
	else
	{
		memcpy( &vecBulletSpread, &vecConeMoving, sizeof(Vector) );
	}
}

static void EV_HLDM_FireBullets(int idx, Vector &forward, Vector &right, Vector &up, int cShots, Vector &vecSrc, Vector &vecDirShooting, Vector &vecBulletSpread, float flDistance, int iBulletType, int iTracerFreq, int *tracerCount)
{
	float x, y;
	float flSpreadX, flSpreadY;
	Vector vecDir, vecEnd, vecSpread;

	unsigned int ulRandomSeed = Client()->GetRandomSeed();

	flSpreadX = vecBulletSpread.x;
	flSpreadY = vecBulletSpread.y;

	//x = UTIL_SharedRandomFloat( ulRandomSeed + cShots, -0.5, 0.5 ) + UTIL_SharedRandomFloat( ulRandomSeed + (1 + cShots), -0.5, 0.5 );
	//y = UTIL_SharedRandomFloat( ulRandomSeed + (2 + cShots), -0.5, 0.5 ) + UTIL_SharedRandomFloat( ulRandomSeed + (3 + cShots), -0.5, 0.5 );
	
	do
	{
		x = g_pEngineFuncs->RandomFloat(-0.5, 0.5) + g_pEngineFuncs->RandomFloat(-0.5, 0.5);
		y = g_pEngineFuncs->RandomFloat(-0.5, 0.5) + g_pEngineFuncs->RandomFloat(-0.5, 0.5);
	}
	while ( x * x + y * y > 1 );

	vecSpread.x = x * vecBulletSpread.x;
	vecSpread.y = y * vecBulletSpread.y;
	vecSpread.z = 0.f;

	for (int i = 0; i < 3; i++)
	{
		vecDir[i] = vecDirShooting[i] + x * flSpreadX * right[i] + y * flSpreadY * up[i];
		vecEnd[i] = vecSrc[i] + flDistance * vecDir[i];
	}

	pmtrace_t trace;

	g_pEventAPI->EV_SetTraceHull( PM_HULL_POINT );
	g_pEventAPI->EV_PlayerTrace( vecSrc, vecEnd, PM_WORLD_ONLY, -1, &trace );

	Vector offset = vecDir * 1.f;
	Vector vecImpact = trace.endpos - offset;

	Render()->DrawBox(vecImpact, Vector(-2, -2, -2), Vector(2, 2, 2), { 255, 255, 255, 100 }, 5.f );
}

//-----------------------------------------------------------------------------
// Events
//-----------------------------------------------------------------------------

void EV_FirePython(event_args_t *args)
{
	Utils()->PrintChatText("EV_FirePython\n");

	const Vector vecConeMoving = VECTOR_CONE_4DEGREES;
	const Vector vecConeStanding = VECTOR_CONE_3DEGREES;
	const Vector vecConeCrouched = VECTOR_CONE_2DEGREES;

	Vector vecGunPosition, vecBulletSpread;
	Vector vecForward, vecRight, vecUp;

	Vector vecOrigin = args->origin;
	Vector vecAngles = args->angles;

	int entindex = args->entindex;

	AngleVectors( vecAngles, &vecForward, &vecRight, &vecUp );

	if ( EV_IsLocal(entindex) )
	{
		// V_PunchAxis( 0, -2.0 );
	}

	Vector vecDirShooting = vecForward;

	EV_GetGunPosition( args, vecGunPosition, vecOrigin );

	BulletAccuracy( vecBulletSpread, vecConeMoving, vecConeStanding, vecConeCrouched, args );

	vecBulletSpread = Vector(0.1f, 0.1f, 0.1f);

	EV_HLDM_FireBullets(
		entindex,
		vecForward,
		vecRight,
		vecUp,
		1,
		vecGunPosition,
		vecDirShooting,
		vecBulletSpread,
		8192.f,
		BULLET_PLAYER_357,
		0,
		NULL);

	if ( EV_IsLocal(entindex) )
	{
		// V_PunchAxis( 0, -12.0 );
	}
}