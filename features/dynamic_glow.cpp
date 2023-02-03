// Dynamic Glow

#include <IClient.h>

#include <hl_sdk/common/dlight.h>
#include <hl_sdk/common/entity_types.h>
#include <hl_sdk/engine/APIProxy.h>

#include "dynamic_glow.h"

#include "../config.h"

#define DYNAMIC_LIGHT_LIFE_TIME 0.05f

//-----------------------------------------------------------------------------
// Vars
//-----------------------------------------------------------------------------

CDynamicGlow g_DynamicGlow;

//-----------------------------------------------------------------------------
// Dynamic Glow implementation
//-----------------------------------------------------------------------------

void CDynamicGlow::OnHUDRedraw() // glow local player
{
	if ( g_Config.cvars.dyn_glow_self && !Client()->IsSpectating() )
	{
		float flRadius = g_Config.cvars.dyn_glow_self_radius;
		float flDecay = g_Config.cvars.dyn_glow_self_decay;
		float flDieTime = g_pEngineFuncs->GetClientTime() + DYNAMIC_LIGHT_LIFE_TIME;

		CreateDynamicLight(Client()->GetPlayerIndex(), g_pPlayerMove->origin, g_Config.cvars.dyn_glow_self_color, flRadius, flDecay, flDieTime);
	}
}

void CDynamicGlow::OnAddEntityPost(int is_visible, int type, struct cl_entity_s *ent, const char *modelname)
{
	if (is_visible && *modelname == 'm')
	{
		if (type == ET_PLAYER)
		{
			if (g_Config.cvars.dyn_glow_players && ent->index != Client()->GetPlayerIndex())
			{
				float flRadius = g_Config.cvars.dyn_glow_players_radius;
				float flDecay = g_Config.cvars.dyn_glow_players_decay;
				float flDieTime = g_pEngineFuncs->GetClientTime() + DYNAMIC_LIGHT_LIFE_TIME;

				CreateDynamicLight(ent->index, ent->curstate.origin, g_Config.cvars.dyn_glow_players_color, flRadius, flDecay, flDieTime);
			}
		}
		else
		{
			const char *pszSlashLastOccur = strrchr(modelname, '/');

			if (pszSlashLastOccur)
				modelname = pszSlashLastOccur + 1;

			if (modelname[0] == 'w' && modelname[1] == '_') // an item
			{
				if (g_Config.cvars.dyn_glow_items)
				{
					float flRadius = g_Config.cvars.dyn_glow_items_radius;
					float flDecay = g_Config.cvars.dyn_glow_items_decay;
					float flDieTime = g_pEngineFuncs->GetClientTime() + DYNAMIC_LIGHT_LIFE_TIME;

					CreateDynamicLight(ent->index, ent->curstate.origin, g_Config.cvars.dyn_glow_items_color, flRadius, flDecay, flDieTime);
				}
			}
			else
			{
				if (g_Config.cvars.dyn_glow_entities)
				{
					if (ent->curstate.solid > SOLID_TRIGGER && ent->curstate.movetype != MOVETYPE_NONE)
					{
						float flRadius = g_Config.cvars.dyn_glow_entities_radius;
						float flDecay = g_Config.cvars.dyn_glow_entities_decay;
						float flDieTime = g_pEngineFuncs->GetClientTime() + DYNAMIC_LIGHT_LIFE_TIME;

						CreateDynamicLight(ent->index, ent->curstate.origin, g_Config.cvars.dyn_glow_entities_color, flRadius, flDecay, flDieTime);
					}
				}
			}
		}
	}
}

void CDynamicGlow::CreateDynamicLight(int entindex, float *vOrigin, float *pColor24, float flRadius, float flDecay, float flDieTime)
{
	dlight_t *pDynamicLight = g_pEngineFuncs->pEfxAPI->CL_AllocDlight(g_Config.cvars.dyn_glow_attach ? entindex : 0);

	pDynamicLight->color.r = int(255.f * pColor24[0]);
	pDynamicLight->color.g = int(255.f * pColor24[1]);
	pDynamicLight->color.b = int(255.f * pColor24[2]);

	pDynamicLight->origin = vOrigin;
	pDynamicLight->die = flDieTime;
	pDynamicLight->radius = flRadius;
	pDynamicLight->decay = flDecay;
}