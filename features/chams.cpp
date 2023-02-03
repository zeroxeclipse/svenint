// Chams

#include <Windows.h>
#include <gl/GL.h>

#include <dbg.h>
#include <convar.h>

#include <hl_sdk/common/cl_entity.h>
#include <hl_sdk/common/r_studioint.h>
#include <hl_sdk/engine/APIProxy.h>
#include <hl_sdk/cl_dll/StudioModelRenderer.h>

#include "chams.h"
#include "firstperson_roaming.h"

#include "../game/utils.h"
#include "../config.h"

//-----------------------------------------------------------------------------
// Global Vars
//-----------------------------------------------------------------------------

CChams g_Chams;

int g_iChamsType = 0;
bool g_bOverrideColor = false;

float g_flOverrideColor_R = 0.0f;
float g_flOverrideColor_G = 0.1f;
float g_flOverrideColor_B = 0.0f;

//-----------------------------------------------------------------------------
// ConCommands
//-----------------------------------------------------------------------------

CON_COMMAND_NO_WRAPPER(sc_chams, "Toggle chams on models")
{
	Msg(g_Config.cvars.chams ? "Chams disabled\n" : "Chams enabled\n");
	g_Config.cvars.chams = !g_Config.cvars.chams;
}

CON_COMMAND_NO_WRAPPER(sc_glow, "Toggle glow on models")
{
	Msg(g_Config.cvars.glow ? "Glow disabled\n" : "Glow enabled\n");
	g_Config.cvars.glow = !g_Config.cvars.glow;
}

//-----------------------------------------------------------------------------
// Chams Implementation
//-----------------------------------------------------------------------------

bool __fastcall Glow(cl_entity_s *pEntity, int iGlow, bool bGlowWall, int iChams, int iGlowWidth, float flGlowColor[3])
{
	if (g_Config.cvars.glow && iGlow)
	{
		if (bGlowWall)
		{
			g_pEngineStudio->SetChromeOrigin();
			g_pEngineStudio->SetForceFaceFlags(0);

			if (iGlow == 2)
			{
				glDepthFunc(GL_GREATER);

				if (g_Config.cvars.glow_optimize)
					glDisable(GL_DEPTH_TEST);

				pEntity->curstate.renderfx = 0;
				g_pStudioRenderer->StudioRenderFinal_Hardware();

				if (!g_Config.cvars.glow_optimize)
				{
					glDepthFunc(GL_LESS);
					g_pStudioRenderer->StudioRenderFinal_Hardware();
				}
			}

			glDepthFunc(GL_GREATER);
			glDisable(GL_DEPTH_TEST);

			g_pEngineStudio->SetForceFaceFlags(STUDIO_NF_CHROME);

			pEntity->curstate.renderfx = kRenderFxGlowShell;
			pEntity->curstate.renderamt = iGlowWidth;
			pEntity->curstate.rendermode = 0;

			pEntity->curstate.rendercolor.r = byte(255.f * flGlowColor[0]);
			pEntity->curstate.rendercolor.g = byte(255.f * flGlowColor[1]);
			pEntity->curstate.rendercolor.b = byte(255.f * flGlowColor[2]);

			if (iGlow != 1)
				g_pTriangleAPI->SpriteTexture(g_pStudioRenderer->m_pChromeSprite, 0);

			g_pStudioRenderer->StudioRenderFinal_Hardware();

			if (!g_Config.cvars.glow_optimize)
				glEnable(GL_DEPTH_TEST);

			if (!g_Config.cvars.chams || !iChams)
			{
				if (iGlow == 1)
				{
					g_pEngineStudio->SetForceFaceFlags(0);
					pEntity->curstate.renderfx = 0;

					g_pStudioRenderer->StudioRenderFinal_Hardware();
				}

				glEnable(GL_DEPTH_TEST);
				glDepthFunc(GL_LESS);

				if (iGlow == 1 && !g_Config.cvars.glow_optimize)
					g_pStudioRenderer->StudioRenderFinal_Hardware();
			}
		}
		else
		{
			g_pEngineStudio->SetChromeOrigin();
			g_pEngineStudio->SetForceFaceFlags(0);

			if (iGlow == 2)
			{
				pEntity->curstate.renderfx = 0;
				g_pStudioRenderer->StudioRenderFinal_Hardware();
			}

			g_pEngineStudio->SetForceFaceFlags(STUDIO_NF_CHROME);

			pEntity->curstate.renderfx = kRenderFxGlowShell;
			pEntity->curstate.renderamt = iGlowWidth;
			pEntity->curstate.rendermode = 0;

			pEntity->curstate.rendercolor.r = byte(255.f * flGlowColor[0]);
			pEntity->curstate.rendercolor.g = byte(255.f * flGlowColor[1]);
			pEntity->curstate.rendercolor.b = byte(255.f * flGlowColor[2]);

			if (iGlow != 1)
				g_pTriangleAPI->SpriteTexture(g_pStudioRenderer->m_pChromeSprite, 0);

			g_pStudioRenderer->StudioRenderFinal_Hardware();

			if (iGlow == 1 && (!g_Config.cvars.chams || !iChams))
			{
				g_pEngineStudio->SetForceFaceFlags(0);
				pEntity->curstate.renderfx = 0;

				g_pStudioRenderer->StudioRenderFinal_Hardware();
			}
		}

		return true;
	}

	return false;
}

bool __fastcall Chams(cl_entity_s *pEntity, int iChams, bool bChamsWall, float flChamsColor[3], float flChamsWallColor[3])
{
	if (g_Config.cvars.chams && iChams)
	{
		// Processed in OpenGL module
		g_bOverrideColor = true;
		g_iChamsType = iChams;

		pEntity->curstate.rendermode = 0;
		pEntity->curstate.renderfx = 0;
		pEntity->curstate.renderamt = 0;

		g_pEngineStudio->SetForceFaceFlags(0);

		if (iChams != 3)
			glDisable(GL_TEXTURE_2D);

		glBindTexture(GL_TEXTURE_2D, 0);

		if (bChamsWall)
		{
			// Processed in OpenGL module
			g_flOverrideColor_R = flChamsWallColor[0];
			g_flOverrideColor_G = flChamsWallColor[1];
			g_flOverrideColor_B = flChamsWallColor[2];

			glDepthFunc(GL_GREATER);
			glDisable(GL_DEPTH_TEST);

			g_pStudioRenderer->StudioRenderFinal_Hardware();
		}

		// Processed in OpenGL module
		g_flOverrideColor_R = flChamsColor[0];
		g_flOverrideColor_G = flChamsColor[1];
		g_flOverrideColor_B = flChamsColor[2];

		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LESS);

		g_pStudioRenderer->StudioRenderFinal_Hardware();

		if (iChams != 3)
			glEnable(GL_TEXTURE_2D);

		g_bOverrideColor = false;

		return true;
	}

	return false;
}

bool CChams::StudioRenderModel()
{
	bool bRenderHandled = false;
	cl_entity_s *pEntity = g_pEngineStudio->GetCurrentEntity();

	if (pEntity)
	{
		if ( !pEntity->player )
		{
			const char *pszModelName = pEntity->model->name;

			if (pszModelName && *pszModelName)
			{
				if ( strstr(pszModelName, "cubemath") )
				{
					return bRenderHandled;
				}

				const char *pszSlashLastOccur = strrchr(pszModelName, '/');

				if (pszSlashLastOccur)
					pszModelName = pszSlashLastOccur + 1;

				if ((pszModelName[0] == 'v' || pszModelName[0] == 'w') && pszModelName[1] == '_') // view / world item
				{
					bRenderHandled = Glow(pEntity, g_Config.cvars.glow_items, g_Config.cvars.glow_items_wall, g_Config.cvars.chams_items, g_Config.cvars.glow_items_width, g_Config.cvars.glow_items_color);
					bRenderHandled = Chams(pEntity, g_Config.cvars.chams_items, g_Config.cvars.chams_items_wall, g_Config.cvars.chams_items_color, g_Config.cvars.chams_items_wall_color) || bRenderHandled;
				}
				else // world entity
				{
					bRenderHandled = Glow(pEntity, g_Config.cvars.glow_entities, g_Config.cvars.glow_entities_wall, g_Config.cvars.chams_entities, g_Config.cvars.glow_entities_width, g_Config.cvars.glow_entities_color);
					bRenderHandled = Chams(pEntity, g_Config.cvars.chams_entities, g_Config.cvars.chams_entities_wall, g_Config.cvars.chams_entities_color, g_Config.cvars.chams_entities_wall_color) || bRenderHandled;
				}
			}
		}
		else // player entity
		{
			if ( g_Config.cvars.fp_roaming && g_pStudioRenderer->m_pCurrentEntity == g_FirstPersonRoaming.GetTargetPlayer() )
				return true;

			bRenderHandled = Glow(pEntity, g_Config.cvars.glow_players, g_Config.cvars.glow_players_wall, g_Config.cvars.chams_players, g_Config.cvars.glow_players_width, g_Config.cvars.glow_players_color);
			bRenderHandled = Chams(pEntity, g_Config.cvars.chams_players, g_Config.cvars.chams_players_wall, g_Config.cvars.chams_players_color, g_Config.cvars.chams_players_wall_color) || bRenderHandled;
		}
	}

	return bRenderHandled;
}