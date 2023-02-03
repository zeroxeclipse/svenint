// Custom Flashlight

#include <dbg.h>
#include <convar.h>

#include <ISvenModAPI.h>
#include <IMemoryUtils.h>

#include "flashlight.h"
#include "firstperson_roaming.h"

#include "../config.h"
#include "../patterns.h"

//-----------------------------------------------------------------------------
// Declare hooks
//-----------------------------------------------------------------------------

DECLARE_HOOK(void, __cdecl, CL_PlayerFlashlight, cl_entity_t *);

//-----------------------------------------------------------------------------
// Vars
//-----------------------------------------------------------------------------

CFlashlight g_Flashlight;

//-----------------------------------------------------------------------------
// Hooks
//-----------------------------------------------------------------------------

DECLARE_FUNC(void, __cdecl, HOOKED_CL_PlayerFlashlight, cl_entity_t *pEntity)
{
	if ( !g_Config.cvars.custom_flashlight )
	{
		ORIG_CL_PlayerFlashlight(pEntity);
		return;
	}

	bool localplayer;

	float falloff;
	pmtrace_t trace;

	Vector vecStart, vecEnd;
	Vector vecForward, v_angle, view_ofs;

	// Invalid entity or flashlight not enabled
	if ( pEntity == NULL || !(pEntity->curstate.effects & 0xC) )
		return;

	// Local player
	if ( localplayer = (pEntity->index == Client()->GetPlayerIndex()) )
	{
		g_pEngineFuncs->GetViewAngles( v_angle );

		VectorCopy( g_pPlayerMove->view_ofs, view_ofs );
		AngleVectors( v_angle, &vecForward, NULL, NULL );
	}
	else
	{
		view_ofs.x = 0.f;
		view_ofs.y = 0.f;
		view_ofs.z = pEntity->curstate.usehull ? 12.5f : 28.5f;

		if ( g_FirstPersonRoaming.GetTargetPlayer() == NULL )
		{
			v_angle = pEntity->curstate.angles;
			v_angle.x *= (89.0f / 9.8876953125f);
		}
		else
		{
			v_angle = g_FirstPersonRoaming.GetLerpViewAngles();
		}

		AngleVectors( v_angle, &vecForward, NULL, NULL );
	}

	VectorAdd( pEntity->origin, view_ofs, vecStart );

	if ( (localplayer && g_Config.cvars.flashlight_localplayer) || (!localplayer && g_Config.cvars.flashlight_players) )
	{
		VectorMA( vecStart, localplayer ? g_Config.cvars.flashlight_localplayer_flashlight_distance : g_Config.cvars.flashlight_players_flashlight_distance, vecForward, vecEnd );

		g_pEventAPI->EV_SetTraceHull( PM_HULL_POINT );
		g_pEventAPI->EV_PlayerTrace( vecStart, vecEnd, PM_STUDIO_BOX, -1, &trace );

		falloff = trace.fraction * (localplayer ? g_Config.cvars.flashlight_localplayer_flashlight_distance : g_Config.cvars.flashlight_players_flashlight_distance);

		if ( falloff < (localplayer ? g_Config.cvars.flashlight_localplayer_falloff_distance : g_Config.cvars.flashlight_players_falloff_distance) )
			falloff = 1.f;
		else
			falloff = (localplayer ? g_Config.cvars.flashlight_localplayer_falloff_distance : g_Config.cvars.flashlight_players_falloff_distance) / falloff;

		falloff *= falloff;

		// Flashlight
		dlight_t *pDynamicLight = g_pEffectsAPI->CL_AllocDlight( pEntity->index );

		VectorCopy( trace.endpos, pDynamicLight->origin );

		pDynamicLight->color.r = 255.f * (localplayer ? g_Config.cvars.flashlight_localplayer_color[0] : g_Config.cvars.flashlight_players_color[0]) * falloff;
		pDynamicLight->color.g = 255.f * (localplayer ? g_Config.cvars.flashlight_localplayer_color[1] : g_Config.cvars.flashlight_players_color[1]) * falloff;
		pDynamicLight->color.b = 255.f * (localplayer ? g_Config.cvars.flashlight_localplayer_color[2] : g_Config.cvars.flashlight_players_color[2]) * falloff;
		pDynamicLight->radius = (localplayer ? g_Config.cvars.flashlight_localplayer_radius : g_Config.cvars.flashlight_players_radius);

		pDynamicLight->die = g_pEngineFuncs->GetClientTime() + 0.2f;
	}

	// Lighting from flashlight
	if ( (localplayer && g_Config.cvars.flashlight_lighting_localplayer) || (!localplayer && g_Config.cvars.flashlight_lighting_players) )
	{
		if ( !localplayer || (localplayer && g_pClientFuncs->CL_IsThirdPerson()) )
		{
			Vector test;

			dlight_t *pEntityLight = g_pEffectsAPI->CL_AllocElight( pEntity->index );

			VectorMA( vecStart, localplayer ? g_Config.cvars.flashlight_lighting_localplayer_distance : g_Config.cvars.flashlight_lighting_players_distance, vecForward, test );
			
			test.z += 24.f;

			VectorCopy( test, pEntityLight->origin );

			pEntityLight->color.r = 255.f * (localplayer ? g_Config.cvars.flashlight_lighting_localplayer_color[0] : g_Config.cvars.flashlight_lighting_players_color[0]);
			pEntityLight->color.g = 255.f * (localplayer ? g_Config.cvars.flashlight_lighting_localplayer_color[1] : g_Config.cvars.flashlight_lighting_players_color[1]);
			pEntityLight->color.b = 255.f * (localplayer ? g_Config.cvars.flashlight_lighting_localplayer_color[2] : g_Config.cvars.flashlight_lighting_players_color[2]);
			pEntityLight->radius = (localplayer ? g_Config.cvars.flashlight_lighting_localplayer_radius : g_Config.cvars.flashlight_lighting_players_radius);

			pEntityLight->die = g_pEngineFuncs->GetClientTime() + 0.2f;
		}
	}
}

//-----------------------------------------------------------------------------
// CFlashlight implementations
//-----------------------------------------------------------------------------

CFlashlight::CFlashlight()
{
	m_pfnCL_PlayerFlashlight = NULL;
	m_hCL_PlayerFlashlight = DETOUR_INVALID_HANDLE;
}

bool CFlashlight::Load()
{
	m_pfnCL_PlayerFlashlight = MemoryUtils()->FindPattern( SvenModAPI()->Modules()->Hardware, Patterns::Hardware::CL_PlayerFlashlight );

	if ( m_pfnCL_PlayerFlashlight == NULL )
	{
		Warning("Failed to locate function \"CL_PlayerFlashlight\"\n");
		return false;
	}

	return true;
}

void CFlashlight::PostLoad()
{
	m_hCL_PlayerFlashlight = DetoursAPI()->DetourFunction( m_pfnCL_PlayerFlashlight, HOOKED_CL_PlayerFlashlight, GET_FUNC_PTR(ORIG_CL_PlayerFlashlight) );
}

void CFlashlight::Unload()
{
	DetoursAPI()->RemoveDetour( m_hCL_PlayerFlashlight );
}