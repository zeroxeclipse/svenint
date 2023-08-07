// Game Utils

#ifndef GAMEUTILS_H
#define GAMEUTILS_H

#ifdef _WIN32
#pragma once
#endif

#ifndef M_SQR
#define M_SQR(x) (x * x)
#endif

#include <hl_sdk/common/pmtrace.h>
#include <hl_sdk/pm_shared/pm_defs.h>
#include <platform.h>

#include "../modules/server.h"

typedef pmtrace_t ( __cdecl *PM_PlayerTraceFn )( float *start, float *end, int traceflags, int numents, physent_t *ents, int ignore_ent, int ( __cdecl *pfnEntFilter )( int ) );

struct screen_info_t
{
	int width;
	int height;
};

extern float *g_flNextCmdTime;
extern double *g_dbGameSpeed;
extern double *dbRealtime;

extern screen_info_t g_ScreenInfo;

// Data conversion
FORCEINLINE long FloatToLong32( float val )
{
	union
	{
		float un_fl;
		unsigned long un_ul;
	};

	un_fl = val;

	return un_ul;
}

FORCEINLINE float Long32ToFloat( long val )
{
	union
	{
		float un_fl;
		unsigned long un_ul;
	};

	un_ul = val;

	return un_fl;
};

// Client utilities
extern PM_PlayerTraceFn PM_PlayerTrace;

bool UTIL_IsDead( void );
bool UTIL_IsSpectating( void );
int UTIL_GetLocalPlayerIndex( void );

// Intersection tests
#define UTIL_IsPointInsideAABB(point, mins, maxs) UTIL_IsAABBIntersectingAABB( point, point, mins, maxs )

FORCEINLINE bool UTIL_IsAABBIntersectingAABB( const Vector &vecBoxMins1, const Vector &vecBoxMaxs1, const Vector &vecBoxMins2, const Vector &vecBoxMaxs2 )
{
	return ( vecBoxMins1.x <= vecBoxMaxs2.x && vecBoxMaxs1.x >= vecBoxMins2.x ) &&
		( vecBoxMins1.y <= vecBoxMaxs2.y && vecBoxMaxs1.y >= vecBoxMins2.y ) &&
		( vecBoxMins1.z <= vecBoxMaxs2.z && vecBoxMaxs1.z >= vecBoxMins2.z );
}

bool UTIL_IsLineIntersectingAABB( const Vector &p1, const Vector &p2, const Vector &vecBoxMins, const Vector &vecBoxMaxs );
bool UTIL_IsSphereIntersectingAABB( const Vector &vecCenter, const float flRadiusSqr, const Vector &vecAbsMins, const Vector &vecAbsMaxs, float *pflOutDistance );
bool UTIL_IsRayIntersectingAABB( const Vector &vecBoxMins, const Vector &vecBoxMaxs, const Vector &vecRayOrigin, const Vector &vecRayDir, float *pflMinIntersection, float *pflMaxIntersection );

// Random
int UTIL_SharedRandomLong( unsigned int seed, int low, int high );
float UTIL_SharedRandomFloat( unsigned int seed, float low, float high );

// String conversions
const wchar_t *UTIL_CStringToWideCString( const char *pszString );

// Player move utilities
int UTIL_ClipVelocity( const Vector &in, const Vector &normal, Vector &out, float overbounce );
void UTIL_AddCorrectGravity( Vector &vecVelocity, float frametime );
void UTIL_FixupGravityVelocity( Vector &vecVelocity, float frametime );
void UTIL_AddCorrectGravity( Vector &vecVelocity, float gravity, float entgravity, float frametime );
void UTIL_FixupGravityVelocity( Vector &vecVelocity, float gravity, float entgravity, float frametime );

// Hulls smh
void UTIL_FindHullIntersection( const Vector &vecSrc, TraceResult &tr, float *mins, float *maxs, edict_t *pEntity );
void UTIL_FindHullIntersectionClient( const Vector &vecSrc, pmtrace_t &tr, float *mins, float *maxs, int ignore_ent );

// Viewport transformations
bool UTIL_WorldToScreen( float *pflOrigin, float *pflVecScreen );
void UTIL_ScreenToWorld( float *pflNDC, float *pflWorldOrigin );

// Aim-related
void UTIL_SetAnglesSilent( float *angles, struct usercmd_s *cmd );
bool UTIL_IsFiring( struct usercmd_s *cmd );

// Speedhacking, lag exploit
void UTIL_SetGameSpeed( double dbSpeed );
void UTIL_SendPacket( bool bSend );

#endif