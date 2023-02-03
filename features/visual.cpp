// Visual

#include <Windows.h>
#include <gl/GL.h>

#include <algorithm>

#include <dbg.h>
#include <convar.h>
#include <IClient.h>
#include <IClientWeapon.h>
#include <IPlayerUtils.h>

#include <hl_sdk/engine/studio.h>
#include <hl_sdk/cl_dll/cl_dll.h>
#include <hl_sdk/cl_dll/StudioModelRenderer.h>
#include <hl_sdk/common/cl_entity.h>
#include <hl_sdk/common/com_model.h>
#include <hl_sdk/common/r_studioint.h>

#include "visual.h"
#include "camhack.h"
#include "firstperson_roaming.h"

#include "../game/utils.h"
#include "../game/drawing.h"
#include "../game/entitylist.h"
#include "../game/class_table.h"

#include "../config.h"

//#define PROCESS_PLAYER_BONES_ONLY

extern bool g_bScreenshot;
extern bool g_bOverrideHUD;

//-----------------------------------------------------------------------------
// Definitions
//-----------------------------------------------------------------------------

struct bone_s
{
	Vector vecPoint[MAXSTUDIOBONES];
	int nParent[MAXSTUDIOBONES] = { -1 };
};

#ifdef PROCESS_PLAYER_BONES_ONLY
Hitbox g_Bones[MAXCLIENTS + 1];
#else
bone_s g_Bones[MAXENTS + 1];
#endif

//-----------------------------------------------------------------------------
// Global Vars
//-----------------------------------------------------------------------------

CVisual g_Visual;
bone_matrix3x4_t *g_pBoneTransform = NULL;

cvar_t *r_drawentities = NULL;

UserMsgHookFn ORIG_UserMsgHook_ScreenShake = NULL;
UserMsgHookFn ORIG_UserMsgHook_ScreenFade = NULL;

//-----------------------------------------------------------------------------
// Console Commands
//-----------------------------------------------------------------------------

CON_COMMAND_EXTERN(sc_wallhack, ConCommand_Wallhack, "Simple OpenGL wallhack")
{
	Msg(g_Config.cvars.wallhack ? "Wallhack disabled\n" : "Wallhack enabled\n");
	g_Config.cvars.wallhack = !g_Config.cvars.wallhack;
}

CON_COMMAND_EXTERN(sc_wallhack_negative, ConCommand_NegativeMode, "Night Mode wallhack")
{
	Msg(g_Config.cvars.wallhack_negative ? "Negative mode disabled\n" : "Negative mode enabled\n");
	g_Config.cvars.wallhack_negative = !g_Config.cvars.wallhack_negative;
}

CON_COMMAND_EXTERN(sc_wallhack_lambert, ConCommand_WhiteWalls, "Lambert wallhack")
{
	Msg(g_Config.cvars.wallhack_white_walls ? "Lambert wallhack disabled\n" : "Lambert wallhack enabled\n");
	g_Config.cvars.wallhack_white_walls = !g_Config.cvars.wallhack_white_walls;
}

CON_COMMAND_EXTERN(sc_wallhack_wireframe, ConCommand_Wireframe, "Wireframe view")
{
	Msg(g_Config.cvars.wallhack_wireframe ? "Wireframe disabled\n" : "Wireframe enabled\n");
	g_Config.cvars.wallhack_wireframe = !g_Config.cvars.wallhack_wireframe;
}

CON_COMMAND_EXTERN(sc_wallhack_wireframe_models, ConCommand_WireframeModels, "Wireframe view (entity models only)")
{
	Msg(g_Config.cvars.wallhack_wireframe_models ? "Wireframe Models disabled\n" : "Wireframe Models enabled\n");
	g_Config.cvars.wallhack_wireframe_models = !g_Config.cvars.wallhack_wireframe_models;
}

CON_COMMAND_EXTERN(sc_esp, ConCommand_ESP, "Toggle ESP")
{
	Msg(g_Config.cvars.esp ? "ESP disabled\n" : "ESP enabled\n");
	g_Config.cvars.esp = !g_Config.cvars.esp;
}

//-----------------------------------------------------------------------------
// Helpers
//-----------------------------------------------------------------------------

static bool IsWorldModelItem(const char *pszModelName)
{
	if (*pszModelName && *pszModelName == 'w' && pszModelName[1] && pszModelName[1] == '_')
	{
		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Visual Hack
//-----------------------------------------------------------------------------

void CVisual::OnVideoInit()
{
	m_flTime = 0.f;

	ResetJumpSpeed();
}

void CVisual::OnHUDRedraw(float flTime)
{
	m_flTime = flTime;

	ShowSpeed();
	ShowGrenadeTimer();
}

void CVisual::Process()
{
	m_iScreenWidth = g_ScreenInfo.width;
	m_iScreenHeight = g_ScreenInfo.height;

	Lightmap();

	ESP();

	DrawCrosshair();
}

void CVisual::ResetJumpSpeed()
{
	m_flPrevTime = m_flTime;
	m_flFadeTime = g_Config.cvars.jumpspeed_fade_duration;
	m_flJumpSpeed = 0.f;

	m_clFadeFrom[0] = int(255.f * g_Config.cvars.speed_color[0]);
	m_clFadeFrom[1] = int(255.f * g_Config.cvars.speed_color[1]);
	m_clFadeFrom[2] = int(255.f * g_Config.cvars.speed_color[2]);

	m_bOnGround = true;
}

void CVisual::ShowSpeed()
{
	if ( !g_pClient->IsSpectating() && !(g_CamHack.IsEnabled() && g_Config.cvars.camhack_hide_hud) )
	{
		float flSpeed;

		if (g_Config.cvars.show_speed)
		{
			g_bOverrideHUD = false;

			if ( g_Config.cvars.show_vertical_speed )
				flSpeed = g_pPlayerMove->velocity.Length();
			else
				flSpeed = g_pPlayerMove->velocity.Length2D();

			g_Drawing.DrawNumber(flSpeed > 0.f ? int(floor(flSpeed)) : int(ceil(flSpeed)),
								 int(m_iScreenWidth * g_Config.cvars.speed_width_fraction),
								 int(m_iScreenHeight * g_Config.cvars.speed_height_fraction),
								 int(255.f * g_Config.cvars.speed_color[0]),
								 int(255.f * g_Config.cvars.speed_color[1]),
								 int(255.f * g_Config.cvars.speed_color[2]),
								 FONT_ALIGN_CENTER);

			if (g_Config.cvars.show_jumpspeed)
			{
				int r = int(255.f * g_Config.cvars.speed_color[0]);
				int g = int(255.f * g_Config.cvars.speed_color[1]);
				int b = int(255.f * g_Config.cvars.speed_color[2]);

				float flFadeDuration = g_Config.cvars.jumpspeed_fade_duration;
				int iSpriteHeight = g_Drawing.GetNumberSpriteHeight();

				if (flFadeDuration > 0.0f)
				{
					if ( !g_pClient->IsOnGround() )
					{
						if ( m_bOnGround && g_pClient->Buttons() & IN_JUMP )
						{
							float flDifference = flSpeed - m_flJumpSpeed;

							if (flDifference != 0.0f)
							{
								if (flDifference > 0.0f)
								{
									m_clFadeFrom[0] = 0;
									m_clFadeFrom[1] = 255;
									m_clFadeFrom[2] = 0;
								}
								else
								{
									m_clFadeFrom[0] = 255;
									m_clFadeFrom[1] = 0;
									m_clFadeFrom[2] = 0;
								}

								m_flFadeTime = 0.0f;
								m_flJumpSpeed = flSpeed;
							}
						}

						m_bOnGround = false;
					}
					else
					{
						m_bOnGround = true;
					}

					float flDelta = V_max(m_flTime - m_flPrevTime, 0.0f);

					m_flFadeTime += flDelta;

					if (m_flFadeTime > flFadeDuration || !IsFloatFinite(m_flFadeTime) )
						m_flFadeTime = flFadeDuration;

					float flFadeFrom_R = int(255.f * g_Config.cvars.speed_color[0]) - m_clFadeFrom[0] / flFadeDuration;
					float flFadeFrom_G = int(255.f * g_Config.cvars.speed_color[1]) - m_clFadeFrom[1] / flFadeDuration;
					float flFadeFrom_B = int(255.f * g_Config.cvars.speed_color[2]) - m_clFadeFrom[2] / flFadeDuration;

					r = int(int(255.f * g_Config.cvars.speed_color[0]) - flFadeFrom_R * (flFadeDuration - m_flFadeTime));
					g = int(int(255.f * g_Config.cvars.speed_color[1]) - flFadeFrom_G * (flFadeDuration - m_flFadeTime));
					b = int(int(255.f * g_Config.cvars.speed_color[2]) - flFadeFrom_B * (flFadeDuration - m_flFadeTime));

					m_flPrevTime = m_flTime;
				}

				g_Drawing.DrawNumber(m_flJumpSpeed > 0.f ? int(floor(m_flJumpSpeed)) : int(ceil(m_flJumpSpeed)),
									 int(m_iScreenWidth * g_Config.cvars.speed_width_fraction),
									 int(m_iScreenHeight * g_Config.cvars.speed_height_fraction) - (iSpriteHeight + iSpriteHeight / 4),
									 r,
									 g,
									 b,
									 FONT_ALIGN_CENTER);
			}

			g_bOverrideHUD = true;
		}
		else if (g_Config.cvars.show_speed_legacy)
		{
			float flSpeed;

			if ( g_Config.cvars.show_vertical_speed_legacy )
				flSpeed = g_pPlayerMove->velocity.Length();
			else
				flSpeed = g_pPlayerMove->velocity.Length2D();

			g_Drawing.DrawStringF(g_hFontSpeedometer,
								  int(m_iScreenWidth * g_Config.cvars.speed_width_fraction_legacy),
								  int(m_iScreenHeight * g_Config.cvars.speed_height_fraction_legacy),
								  int(255.f * g_Config.cvars.speed_color_legacy[0]),
								  int(255.f * g_Config.cvars.speed_color_legacy[1]),
								  int(255.f * g_Config.cvars.speed_color_legacy[2]),
								  int(255.f * g_Config.cvars.speed_color_legacy[3]),
								  FONT_ALIGN_CENTER,
								  "%.1f",
								  flSpeed);
		}
	}
}

void CVisual::ShowGrenadeTimer()
{
	if ( g_Config.cvars.grenade_timer )
	{
		if ( Client()->GetCurrentWeaponID() == WEAPON_HANDGRENADE )
		{
			float flTime = ClientWeapon()->GetWeaponData()->fuser1;

			if (flTime < 0.f)
			{
				flTime *= -1; // revert

				int r = int(255.f * g_Config.cvars.grenade_timer_color[0]);
				int g = int(255.f * g_Config.cvars.grenade_timer_color[1]);
				int b = int(255.f * g_Config.cvars.grenade_timer_color[2]);

				if ( flTime >= 4.f )
				{
					r = int(255.f * g_Config.cvars.grenade_explosive_time_color[0]);
					g = int(255.f * g_Config.cvars.grenade_explosive_time_color[1]);
					b = int(255.f * g_Config.cvars.grenade_explosive_time_color[2]);

					if ( flTime >= 10.f )
						flTime = 10.f;
				}

				int x = int(m_iScreenWidth * g_Config.cvars.grenade_timer_width_frac);
				int y = int(m_iScreenHeight * g_Config.cvars.grenade_timer_height_frac);

				int iSpriteWidth = g_Drawing.GetNumberSpriteWidth();
				int iSpriteHeight = g_Drawing.GetNumberSpriteHeight();

				int iThickness = int((float)iSpriteWidth / 8.f);

				g_bOverrideHUD = false;

				g_Drawing.DrawNumber(int(flTime), x, y, r, g, b, FONT_ALIGN_LEFT); // seconds

				if (flTime < 10.f)
				{
					g_Drawing.FillArea(x + iSpriteWidth + (iSpriteWidth / 2) - iThickness,
									   y + iSpriteHeight - iThickness * 2,
									   iThickness * 2,
									   iThickness * 2,
									   r, g, b, 232);

					float ms = (flTime - floorf(flTime)) * 1000.f;

					//g_Drawing.DrawNumber(int(ms), x + iSpriteWidth * (flTime >= 10.f ? 4 : 3), y, r, g, b, FONT_ALIGN_CENTER); // milliseconds
					g_Drawing.DrawNumber(int(ms), x + iSpriteWidth * 2, y, r, g, b, FONT_ALIGN_LEFT); // milliseconds
				}

				//g_Drawing.DrawNumber(int(flTime), x, y, r, g, b, FONT_ALIGN_CENTER); // seconds

				//if (flTime < 10.f)
				//{
				//	//int iWidth = iSpriteWidth * (flTime >= 10.f ? 2 : 1);
				//	int iWidth = iSpriteWidth;

				//	g_Drawing.FillArea(x + iWidth - iThickness,
				//					   y + iSpriteHeight - iThickness * 2,
				//					   iThickness * 2,
				//					   iThickness * 2,
				//					   r, g, b, 216);

				//	float ms = (flTime - floorf(flTime)) * 1000.f;

				//	//g_Drawing.DrawNumber(int(ms), x + iSpriteWidth * (flTime >= 10.f ? 4 : 3), y, r, g, b, FONT_ALIGN_CENTER); // milliseconds
				//	g_Drawing.DrawNumber(int(ms), x + iSpriteWidth * 3, y, r, g, b, FONT_ALIGN_CENTER); // milliseconds
				//}

				g_bOverrideHUD = true;
			}
		}
	}
}

void CVisual::Lightmap()
{
	if (!g_Config.cvars.lightmap_override)
	{
		g_pEngineFuncs->OverrideLightmap(0);
		return;
	}

	static bool replaced = false;

	if (g_Config.cvars.lightmap_brightness > 0.f)
	{
		if (!replaced)
		{
			g_pEngineFuncs->OverrideLightmap(1);
			replaced = true;
		}
		else
		{
			replaced = false;
		}

		g_pEngineFuncs->SetLightmapColor(g_Config.cvars.lightmap_color[0], g_Config.cvars.lightmap_color[1], g_Config.cvars.lightmap_color[2]);
		g_pEngineFuncs->SetLightmapDarkness(g_Config.cvars.lightmap_brightness);
	}
}

void CVisual::DrawCrosshair()
{
	if ( (g_CamHack.IsEnabled() && g_Config.cvars.camhack_hide_hud) )
		return;

	if ( !(g_pPlayerMove->iuser1 == 0 || g_Config.cvars.fp_roaming_draw_crosshair && g_FirstPersonRoaming.GetTargetPlayer()) )
		return;

	if ( g_Config.cvars.draw_crosshair_dot )
	{
		if ( g_Config.cvars.draw_crosshair_outline )
		{
			g_Drawing.DrawDotShadow((m_iScreenWidth / 2) - 1,
									(m_iScreenHeight / 2) - 1,
									int(255.f * g_Config.cvars.crosshair_outline_color[0]),
									int(255.f * g_Config.cvars.crosshair_outline_color[1]),
									int(255.f * g_Config.cvars.crosshair_outline_color[2]),
									int(255.f * g_Config.cvars.crosshair_outline_color[3]),
									g_Config.cvars.crosshair_thickness,
									g_Config.cvars.crosshair_outline_thickness);
		}

		g_Drawing.DrawDot((m_iScreenWidth / 2) - 1,
							(m_iScreenHeight / 2) - 1,
							int(255.f * g_Config.cvars.crosshair_color[0]),
							int(255.f * g_Config.cvars.crosshair_color[1]),
							int(255.f * g_Config.cvars.crosshair_color[2]),
							int(255.f * g_Config.cvars.crosshair_color[3]),
							g_Config.cvars.crosshair_thickness);

		//g_Drawing.DrawDot((m_iScreenWidth / 2) - 1,
		//				  (m_iScreenHeight / 2) - 1,
		//				  int(255.f * g_Config.cvars.crosshair_outline_color[0]),
		//				  int(255.f * g_Config.cvars.crosshair_outline_color[1]),
		//				  int(255.f * g_Config.cvars.crosshair_outline_color[2]),
		//				  int(255.f * g_Config.cvars.crosshair_outline_color[3]),
		//				  g_Config.cvars.crosshair_thickness + g_Config.cvars.crosshair_outline_thickness);
	}

	if ( g_Config.cvars.draw_crosshair )
	{
		if ( g_Config.cvars.draw_crosshair_outline )
		{
			g_Drawing.DrawCrosshairShadow((m_iScreenWidth / 2) - 1,
											(m_iScreenHeight / 2) - 1,
											int(255.f * g_Config.cvars.crosshair_outline_color[0]),
											int(255.f * g_Config.cvars.crosshair_outline_color[1]),
											int(255.f * g_Config.cvars.crosshair_outline_color[2]),
											int(255.f * g_Config.cvars.crosshair_outline_color[3]),
											g_Config.cvars.crosshair_size,
											g_Config.cvars.crosshair_gap,
											g_Config.cvars.crosshair_thickness,
											g_Config.cvars.crosshair_outline_thickness);
			
			//g_Drawing.DrawCrosshair((m_iScreenWidth / 2) - 1,
			//						(m_iScreenHeight / 2) - 1,
			//						int(255.f * g_Config.cvars.crosshair_outline_color[0]),
			//						int(255.f * g_Config.cvars.crosshair_outline_color[1]),
			//						int(255.f * g_Config.cvars.crosshair_outline_color[2]),
			//						int(255.f * g_Config.cvars.crosshair_outline_color[3]),
			//						g_Config.cvars.crosshair_size,
			//						g_Config.cvars.crosshair_gap,
			//						g_Config.cvars.crosshair_thickness + g_Config.cvars.crosshair_outline_thickness);
		}

		g_Drawing.DrawCrosshair((m_iScreenWidth / 2) - 1,
								(m_iScreenHeight / 2) - 1,
								int(255.f * g_Config.cvars.crosshair_color[0]),
								int(255.f * g_Config.cvars.crosshair_color[1]),
								int(255.f * g_Config.cvars.crosshair_color[2]),
								int(255.f * g_Config.cvars.crosshair_color[3]),
								g_Config.cvars.crosshair_size,
								g_Config.cvars.crosshair_gap,
								g_Config.cvars.crosshair_thickness);
	}
}

void CVisual::ESP()
{
	if ( !g_Config.cvars.esp )
		return;

	static Vector vecAABBPoints[8];
	static Vector2D vecScreenProj[8];

	static Vector vecBottom;
	static Vector vecTop;

	bool bSpectating = Client()->IsSpectating();

	cl_entity_s *pLocal = g_pEngineFuncs->GetLocalPlayer();
	CEntity *pEnts = g_EntityList.GetList();

	for (register int i = 1; i <= g_EntityList.GetMaxEntities(); i++)
	{
		CEntity &ent = pEnts[i];

		if ( !ent.m_bValid )
			continue;

		cl_entity_t *pEntity = ent.m_pEntity;

		bool bPlayer = ent.m_bPlayer;
		bool bItem = ent.m_bItem;

		if ( g_bScreenshot && !bPlayer )
			continue;

		if ( bItem && !g_Config.cvars.esp_show_items )
			continue;

		if ( !ent.m_bVisible )
			continue;

		float flDistance = (pEntity->origin - pLocal->origin).Length();

		if ( flDistance > g_Config.cvars.esp_distance )
			continue;

		float x, y, w, h;
		float top_mid_x, top_mid_y, bottom_mid_x, bottom_mid_y;

		vecBottom = pEntity->origin;
		vecTop = pEntity->origin;

		float boxWidth;
		float vecScreenBottom[2], vecScreenTop[2];

		if ( !bPlayer )
		{
			if ( ent.m_classInfo.id == CLASS_NONE && g_Config.cvars.esp_ignore_unknown_ents )
				continue;

			// Don't process if entity isn't an ESP's target
			if ( g_Config.cvars.esp_targets == 2 )
				continue;

			if ( !bItem )
			{
				vecTop.z += ent.m_vecMaxs.z;
				vecBottom.z -= ent.m_vecMins.z;

				float height = ( ent.m_vecMaxs.z - ent.m_vecMins.z );

				boxWidth = ( height != 0.f ) ? ( max(ent.m_vecMaxs.x - ent.m_vecMins.x, ent.m_vecMaxs.y - ent.m_vecMins.y) / height ) : 0.f;
			}
		}
		else
		{
			if ( g_Config.cvars.esp_targets == 1 )
				continue;

			if ( ent.m_bDucked )
			{
				vecTop.z += VEC_DUCK_HULL_MAX.z;
				vecBottom.z += VEC_DUCK_HULL_MIN.z;

				boxWidth = (VEC_DUCK_HULL_MAX.x - VEC_DUCK_HULL_MIN.x) / (VEC_DUCK_HULL_MAX.z - VEC_DUCK_HULL_MIN.z);
			}
			else
			{
				vecTop.z += ent.m_vecMaxs.z;
				vecBottom.z -= ent.m_vecMaxs.z;

				boxWidth = (VEC_HULL_MAX.x - VEC_HULL_MIN.x) / (VEC_HULL_MAX.z - VEC_HULL_MIN.z);
			}
		}

		if ( !UTIL_WorldToScreen(vecBottom, vecScreenBottom) || !UTIL_WorldToScreen(vecTop, vecScreenTop) )
			continue;

		float boxHeight = vecScreenBottom[1] - vecScreenTop[1];
		boxWidth = boxHeight * boxWidth;

		top_mid_x = vecScreenTop[0];
		top_mid_y = vecScreenTop[1];
			
		bottom_mid_x = vecScreenBottom[0];
		bottom_mid_y = vecScreenBottom[1];

		//x = vecScreenTop[0] - (boxWidth * 0.5f); // rotate around the head
		//y = vecScreenTop[1];
		x = vecScreenBottom[0] - (boxWidth * 0.5f); // rotate around the pivot/legs
		y = vecScreenBottom[1] - boxHeight;
		w = boxWidth;
		h = boxHeight;

		if ( !g_Config.cvars.esp_optimize && !bItem )
		{
			Vector vecMins = pEntity->origin + ent.m_vecMins;
			Vector vecMaxs = pEntity->origin + ent.m_vecMaxs;

			vecAABBPoints[0].x = vecMins.x;
			vecAABBPoints[0].y = vecMins.y;
			vecAABBPoints[0].z = vecMins.z;
		
			vecAABBPoints[1].x = vecMins.x;
			vecAABBPoints[1].y = vecMaxs.y;
			vecAABBPoints[1].z = vecMins.z;
		
			vecAABBPoints[2].x = vecMaxs.x;
			vecAABBPoints[2].y = vecMaxs.y;
			vecAABBPoints[2].z = vecMins.z;
		
			vecAABBPoints[3].x = vecMaxs.x;
			vecAABBPoints[3].y = vecMins.y;
			vecAABBPoints[3].z = vecMins.z;
		
			vecAABBPoints[4].x = vecMaxs.x;
			vecAABBPoints[4].y = vecMaxs.y;
			vecAABBPoints[4].z = vecMaxs.z;
		
			vecAABBPoints[5].x = vecMins.x;
			vecAABBPoints[5].y = vecMaxs.y;
			vecAABBPoints[5].z = vecMaxs.z;
		
			vecAABBPoints[6].x = vecMins.x;
			vecAABBPoints[6].y = vecMins.y;
			vecAABBPoints[6].z = vecMaxs.z;
		
			vecAABBPoints[7].x = vecMaxs.x;
			vecAABBPoints[7].y = vecMins.y;
			vecAABBPoints[7].z = vecMaxs.z;

			if ( !UTIL_WorldToScreen(vecAABBPoints[3], vecScreenProj[0]) || !UTIL_WorldToScreen(vecAABBPoints[5], vecScreenProj[1])
				|| !UTIL_WorldToScreen(vecAABBPoints[0], vecScreenProj[2]) || !UTIL_WorldToScreen(vecAABBPoints[4], vecScreenProj[3])
				|| !UTIL_WorldToScreen(vecAABBPoints[2], vecScreenProj[4]) || !UTIL_WorldToScreen(vecAABBPoints[1], vecScreenProj[5])
				|| !UTIL_WorldToScreen(vecAABBPoints[6], vecScreenProj[6]) || !UTIL_WorldToScreen(vecAABBPoints[7], vecScreenProj[7]) )
			{
				continue;
			}

			float left = vecScreenProj[0].x;
			float top = vecScreenProj[0].y;
			float right = vecScreenProj[0].x;
			float bottom = vecScreenProj[0].y;

			for (int i = 1; i < 8; i++)
			{
				if ( left > vecScreenProj[i].x )
					left = vecScreenProj[i].x;

				if ( bottom < vecScreenProj[i].y )
					bottom = vecScreenProj[i].y;

				if ( right < vecScreenProj[i].x )
					right = vecScreenProj[i].x;

				if ( top > vecScreenProj[i].y )
					top = vecScreenProj[i].y;
			}

			x = left;
			y = top;
			w = right - left;
			h = bottom - top;

			top_mid_y = y;
			bottom_mid_y = y + h;
		}

		if (bPlayer && g_Config.cvars.esp_show_visible_players)
		{
			pmtrace_t trace;

			Vector vecStart = Client()->GetOrigin() + Client()->GetViewOffset();
			Vector vecEnd = vecBottom + (vecTop - vecBottom) * 0.5f;

			g_pEventAPI->EV_SetTraceHull(PM_HULL_POINT);
			g_pEventAPI->EV_PlayerTrace(vecStart, vecEnd, PM_WORLD_ONLY, -1, &trace);

			if (trace.fraction != 1.f)
			{
				continue;
			}
		}

		int iHealth = (int)ent.m_flHealth;

		bool bIsEntityFriend = ent.m_bFriend;
		bool bIsEntityNeutral = ent.m_bNeutral;

		int r = int(255.f * g_Config.cvars.esp_friend_color[0]);
		int g = int(255.f * g_Config.cvars.esp_friend_color[1]);
		int b = int(255.f * g_Config.cvars.esp_friend_color[2]);

		if ( bPlayer && ent.m_bEnemy )
			bIsEntityFriend = false;

		if ( bItem )
		{
			bottom_mid_x = top_mid_x = x = x + w / 2;
			bottom_mid_y = top_mid_y = y = y + h / 2;

			if (ent.m_bEnemy)
			{
				r = int(255.f * g_Config.cvars.esp_enemy_color[0]);
				g = int(255.f * g_Config.cvars.esp_enemy_color[1]);
				b = int(255.f * g_Config.cvars.esp_enemy_color[2]);
			}
			else if (ent.m_bNeutral)
			{
				r = int(255.f * g_Config.cvars.esp_neutral_color[0]);
				g = int(255.f * g_Config.cvars.esp_neutral_color[1]);
				b = int(255.f * g_Config.cvars.esp_neutral_color[2]);
			}
			else
			{
				r = int(255.f * g_Config.cvars.esp_item_color[0]);
				g = int(255.f * g_Config.cvars.esp_item_color[1]);
				b = int(255.f * g_Config.cvars.esp_item_color[2]);
			}
		}
		else if ( bIsEntityNeutral )
		{
			r = int(255.f * g_Config.cvars.esp_neutral_color[0]);
			g = int(255.f * g_Config.cvars.esp_neutral_color[1]);
			b = int(255.f * g_Config.cvars.esp_neutral_color[2]);
		}
		else if ( !bIsEntityFriend )
		{
			r = int(255.f * g_Config.cvars.esp_enemy_color[0]);
			g = int(255.f * g_Config.cvars.esp_enemy_color[1]);
			b = int(255.f * g_Config.cvars.esp_enemy_color[2]);
		}

		if ( g_Config.cvars.esp_debug )
		{
			if ( !ent.m_bItem && ((bPlayer && (iHealth > 0 || iHealth < -1)) || !bPlayer) )
			{
				for (int j = 0; j < ent.m_pStudioHeader->numhitboxes; j++)
				{
					float vScreen[2];

					if ( !UTIL_WorldToScreen(ent.m_rgHitboxes[j], vScreen) )
						continue;

					g_Drawing.DrawStringF(g_hFontESP, vScreen[0], vScreen[1], 255, 255, 255, 255, FONT_ALIGN_CENTER, "%d", j);
				}
			}

			g_Drawing.DrawStringF(g_hFontESP, bottom_mid_x, bottom_mid_y + 20.f, 255, 255, 255, 255, FONT_ALIGN_CENTER,
								  "Solid: %d | Movetype: %d", pEntity->curstate.solid, pEntity->curstate.movetype);
			g_Drawing.DrawStringF(g_hFontESP, bottom_mid_x, bottom_mid_y + 30.f, 255, 255, 255, 255, FONT_ALIGN_CENTER,
								  "Sequence: %d", pEntity->curstate.sequence);
		}

		if ( g_Config.cvars.esp_snaplines && !bSpectating )
		{
			g_Drawing.DrawLine(g_ScreenInfo.width / 2, g_ScreenInfo.height, bottom_mid_x, bottom_mid_y, r, g, b, 255);
		}

		if (bPlayer)
		{
			// Box Fill
			if ( g_Config.cvars.esp_box_targets != 1 && w != 0.f && h != 0.f )
			{
				DrawBox(bPlayer, bItem, iHealth, x, y, w, h, r, g, b);
			}

			// Distance
			if ( g_Config.cvars.esp_box_distance && g_Config.cvars.esp_distance_mode != 1 )
			{
				g_Drawing.DrawStringF(g_hFontESP, bottom_mid_x, bottom_mid_y - 8.f, 255, 255, 255, 255, FONT_ALIGN_CENTER, "%.1f", flDistance);
			}

			// General Info
			if (g_Config.cvars.esp_player_style == 0) // Default
			{
				DrawPlayerInfo_Default(i, iHealth, bIsEntityFriend, top_mid_x, top_mid_y, bottom_mid_x, bottom_mid_y);
			}
			else if (g_Config.cvars.esp_player_style == 1) // SAMP
			{
				DrawPlayerInfo_SAMP(i, iHealth, (bool)pEntity->curstate.usehull, bIsEntityFriend, vecTop);
			}
			else if (g_Config.cvars.esp_player_style == 2) // Left 4 Dead
			{
				DrawPlayerInfo_L4D(i, iHealth, (bool)pEntity->curstate.usehull, bIsEntityFriend, vecTop);
			}

			if ( g_Config.cvars.esp_skeleton_type == 1 )
				continue;
		}
		else
		{
			// Box Fill
			if ( g_Config.cvars.esp_box_targets != 2 )
			{
				DrawBox(bPlayer, bItem, iHealth, x, y, w, h, r, g, b);
			}

			// Distance
			if ( g_Config.cvars.esp_box_distance && g_Config.cvars.esp_distance_mode != 2 )
			{
				g_Drawing.DrawStringF(g_hFontESP, bottom_mid_x, bottom_mid_y - 8.f, 255, 255, 255, 255, FONT_ALIGN_CENTER, "%.1f", flDistance);
			}

			// General Info
			if (g_Config.cvars.esp_entity_style == 0) // Default
			{
				DrawEntityInfo_Default(i, ent.m_classInfo, bottom_mid_x, bottom_mid_y, r, g, b);
			}
			else if (g_Config.cvars.esp_entity_style == 1) // SAMP
			{
				DrawEntityInfo_SAMP(i, ent.m_classInfo, vecTop, r, g, b);
			}
			else if (g_Config.cvars.esp_entity_style == 2) // Left 4 Dead
			{
				DrawEntityInfo_L4D(i, ent.m_classInfo, vecTop, r, g, b);
			}

			if ( g_Config.cvars.esp_skeleton_type == 2 )
				continue;
		}

		if ( bItem )
			continue;

		if ( bPlayer && (iHealth == 0 || iHealth == -1) )
			continue;

		DrawBones(i, ent.m_pStudioHeader);
	}
}

void CVisual::DrawPlayerInfo_Default(int index, int iHealth, bool bIsEntityFriend, float top_mid_x, float top_mid_y, float bottom_mid_x, float bottom_mid_y)
{
	if (g_Config.cvars.esp_box_player_health && iHealth != 0)
	{
		int r, g, b;

		int iActualHealth = iHealth;

		if (iHealth == -1)
			iActualHealth = iHealth = 0;
		else if (iHealth > 100)
			iHealth = 100;

		if (bIsEntityFriend)
		{
			r = int( 255.f * (iHealth > 50 ? 1.f - 2.f * (iHealth - 50) / 100.f : 1.f) );
			g = int( 255.f * ((iHealth > 50 ? 1.f : 2.f * iHealth / 100.f)) );
			b = 0;
		}
		else
		{
			iActualHealth = -1;

			r = 0;
			g = 255;
			b = 255;
		}

		g_Drawing.DrawStringF(g_hFontESP,
								top_mid_x,
								top_mid_y - 8.f,
								r,
								g,
								b,
								255,
								FONT_ALIGN_CENTER,
								"%d",
								iActualHealth);
	}

	if (g_Config.cvars.esp_box_player_armor)
	{
		float flArmor = g_pPlayerUtils->GetArmor(index);

		if (flArmor > 0.f)
			g_Drawing.DrawStringF(g_hFontESP, top_mid_x, top_mid_y + 8.f, 153, 191, 255, 255, FONT_ALIGN_CENTER, "%.1f", flArmor);
	}

	if (g_Config.cvars.esp_box_player_name || g_Config.cvars.esp_box_index)
	{
		static char szIndex[16];
		player_info_t *pPlayer = NULL;

		if (g_Config.cvars.esp_box_index)
			snprintf(szIndex, sizeof(szIndex), g_Config.cvars.esp_box_player_name ? " (%d)" : "(%d)", index);

		int nickname_r, nickname_g, nickname_b;

		if (bIsEntityFriend)
		{
			nickname_r = int(255.f * g_Config.cvars.esp_friend_player_color[0]);
			nickname_g = int(255.f * g_Config.cvars.esp_friend_player_color[1]);
			nickname_b = int(255.f * g_Config.cvars.esp_friend_player_color[2]);
		}
		else
		{
			nickname_r = int(255.f * g_Config.cvars.esp_enemy_player_color[0]);
			nickname_g = int(255.f * g_Config.cvars.esp_enemy_player_color[1]);
			nickname_b = int(255.f * g_Config.cvars.esp_enemy_player_color[2]);
		}

		g_Drawing.DrawStringF(g_hFontESP, bottom_mid_x, bottom_mid_y + 8.f, nickname_r, nickname_g, nickname_b, 255, FONT_ALIGN_CENTER, "%s%s",
								g_Config.cvars.esp_box_player_name ? (pPlayer = g_pEngineStudio->PlayerInfo(index - 1), pPlayer->name) : "",
								g_Config.cvars.esp_box_index ? szIndex : "");
	}
}

void CVisual::DrawPlayerInfo_SAMP(int index, int iHealth, bool bDucking, bool bIsEntityFriend, Vector vecTop)
{
	constexpr int iBarWidth = 42;
	constexpr int iBarHeight = 4;
	constexpr int iThickness = 1;

	float vecScreen[2];

	if (bDucking)
		vecTop.z += 12.f;
	else
		vecTop.z += 6.f;

	if ( !UTIL_WorldToScreen(vecTop, vecScreen) )
		return;

	int offset_x = int(vecScreen[0]);
	int offset_y = int(vecScreen[1]);

	int health_offset_y = offset_y;

	if (g_Config.cvars.esp_box_player_armor)
	{
		float flArmor = g_pPlayerUtils->GetArmor(index);

		if (flArmor > 0.f)
		{
			if (flArmor > 100.f)
				flArmor = 100.f;

			float flFraction = flArmor / 100.f;

			// Thickness
			{
				constexpr int iWidth = iBarWidth + 2 * iThickness;
				constexpr int iHeight = iBarHeight + 2 * iThickness;

				g_Drawing.FillArea(offset_x - iWidth / 2, offset_y - iHeight / 2, iWidth, iHeight, 0, 0, 0, 255);
			}

			if (flArmor != 100.f)
			{
				g_Drawing.FillArea(offset_x - iBarWidth / 2, offset_y - iBarHeight / 2, iBarWidth, iBarHeight, 40, 40, 40, 255);
			}

			g_Drawing.FillArea(offset_x - iBarWidth / 2, offset_y - iBarHeight / 2, int((float)iBarWidth * flFraction), iBarHeight, 200, 200, 200, 255);

			health_offset_y += ((iBarHeight + 2 * iThickness) / 2) + 5;
		}
	}

	if (g_Config.cvars.esp_box_player_health)
	{
		if (iHealth < 0)
			iHealth = 0;
		else if (iHealth > 100)
			iHealth = 100;

		int y = health_offset_y;
		float flFraction = (float)iHealth / 100.f;

		// Thickness
		{
			constexpr int iWidth = iBarWidth + 2 * iThickness;
			constexpr int iHeight = iBarHeight + 2 * iThickness;

			g_Drawing.FillArea(offset_x - iWidth / 2, y - iHeight / 2, iWidth, iHeight, 0, 0, 0, 255);
		}

		if (iHealth != 100)
		{
			g_Drawing.FillArea(offset_x - iBarWidth / 2, y - iBarHeight / 2, iBarWidth, iBarHeight, 76, 11, 20, 255);
		}

		g_Drawing.FillArea(offset_x - iBarWidth / 2, y - iBarHeight / 2, int((float)iBarWidth * flFraction), iBarHeight, 187, 32, 40, 255);
	}

	if (g_Config.cvars.esp_box_player_name || g_Config.cvars.esp_box_index)
	{
		static char szIndex[16];
		player_info_t *pPlayer = NULL;

		if (g_Config.cvars.esp_box_index)
			snprintf(szIndex, sizeof(szIndex), g_Config.cvars.esp_box_player_name ? " (%d)" : "(%d)", index);

		int nickname_r, nickname_g, nickname_b;
		int y = offset_y - ((iBarHeight + 2 * iThickness) / 2) - 14;

		if (bIsEntityFriend)
		{
			nickname_r = int(255.f * g_Config.cvars.esp_friend_player_color[0]);
			nickname_g = int(255.f * g_Config.cvars.esp_friend_player_color[1]);
			nickname_b = int(255.f * g_Config.cvars.esp_friend_player_color[2]);
		}
		else
		{
			nickname_r = int(255.f * g_Config.cvars.esp_enemy_player_color[0]);
			nickname_g = int(255.f * g_Config.cvars.esp_enemy_player_color[1]);
			nickname_b = int(255.f * g_Config.cvars.esp_enemy_player_color[2]);
		}

		g_Drawing.DrawStringF(g_hFontESP, offset_x, y, nickname_r, nickname_g, nickname_b, 255, FONT_ALIGN_CENTER, "%s%s",
								g_Config.cvars.esp_box_player_name ? (pPlayer = g_pEngineStudio->PlayerInfo(index - 1), pPlayer->name) : "",
								g_Config.cvars.esp_box_index ? szIndex : "");
	}
}

void CVisual::DrawPlayerInfo_L4D(int index, int iHealth, bool bDucking, bool bIsEntityFriend, Vector vecTop)
{
	float vecScreen[2];

	if (bDucking)
		vecTop.z += 24.f;
	else
		vecTop.z += 14.f;

	if ( !UTIL_WorldToScreen(vecTop, vecScreen) )
		return;

	int offset_x = vecScreen[0];
	int offset_y = vecScreen[1] + 25;

	if (g_Config.cvars.esp_box_player_name || g_Config.cvars.esp_box_player_health || g_Config.cvars.esp_box_player_armor)
	{
		const char *szFormatString;
		static char szInfo[16];

		player_info_t *pPlayer;

		szFormatString = g_Config.cvars.esp_box_player_name ? " (%d)" : "(%d)";

		if (g_Config.cvars.esp_box_player_health && g_Config.cvars.esp_box_player_armor)
		{
			if (iHealth < 0)
				iHealth = 0;
			else if (iHealth > 100)
				iHealth = 100;
			
			float flArmor = g_pPlayerUtils->GetArmor(index);

			if (flArmor > 0.f)
				snprintf(szInfo, sizeof(szInfo), g_Config.cvars.esp_box_player_name ? " (%d) [%.1f]" : "(%d) [%.1f]", iHealth, flArmor);
			else
				snprintf(szInfo, sizeof(szInfo), g_Config.cvars.esp_box_player_name ? " (%d)" : "(%d)", iHealth);
		}
		else if (g_Config.cvars.esp_box_player_health)
		{
			if (iHealth < 0)
				iHealth = 0;
			else if (iHealth > 100)
				iHealth = 100;

			snprintf(szInfo, sizeof(szInfo), g_Config.cvars.esp_box_player_name ? " (%d)" : "(%d)", iHealth);
		}
		else if (g_Config.cvars.esp_box_player_armor)
		{
			float flArmor = g_pPlayerUtils->GetArmor(index);

			if (flArmor > 0.f)
				snprintf(szInfo, sizeof(szInfo), g_Config.cvars.esp_box_player_name ? " [%.1f]" : "[%.1f]", flArmor);
			else
				szInfo[0] = '\0';
		}
		else
		{
			szInfo[0] = '\0';
		}

		int nickname_r, nickname_g, nickname_b;

		int x = vecScreen[0];
		int y = vecScreen[1];

		if (bIsEntityFriend)
		{
			nickname_r = int(255.f * g_Config.cvars.esp_friend_player_color[0]);
			nickname_g = int(255.f * g_Config.cvars.esp_friend_player_color[1]);
			nickname_b = int(255.f * g_Config.cvars.esp_friend_player_color[2]);
		}
		else
		{
			nickname_r = int(255.f * g_Config.cvars.esp_enemy_player_color[0]);
			nickname_g = int(255.f * g_Config.cvars.esp_enemy_player_color[1]);
			nickname_b = int(255.f * g_Config.cvars.esp_enemy_player_color[2]);
		}

		g_Drawing.DrawStringF(g_hFontESP2, x, y, nickname_r, nickname_g, nickname_b, 255, FONT_ALIGN_CENTER, "%s%s",
								g_Config.cvars.esp_box_player_name ? (pPlayer = g_pEngineStudio->PlayerInfo(index - 1), pPlayer->name) : "",
								szInfo);
	}
}

void CVisual::DrawEntityInfo_Default(int index, class_info_t classInfo, float bottom_mid_x, float bottom_mid_y, int r, int g, int b)
{
	if (g_Config.cvars.esp_box_entity_name || g_Config.cvars.esp_box_index)
	{
		static char szIndex[16];

		if (g_Config.cvars.esp_box_index)
			snprintf(szIndex, sizeof(szIndex), g_Config.cvars.esp_box_entity_name ? " (%d)" : "(%d)", index);

		g_Drawing.DrawStringF(g_hFontESP, bottom_mid_x, bottom_mid_y + 8.f, r, g, b, 255, FONT_ALIGN_CENTER, "%s%s",
								g_Config.cvars.esp_box_entity_name ? GetEntityClassname(classInfo) : "",
								g_Config.cvars.esp_box_index ? szIndex : "");
	}
}

void CVisual::DrawEntityInfo_SAMP(int index, class_info_t classInfo, Vector vecTop, int r, int g, int b)
{
	if (g_Config.cvars.esp_box_entity_name || g_Config.cvars.esp_box_index)
	{
		float vecScreen[2];
		vecTop.z += 6.f;

		UTIL_WorldToScreen(vecTop, vecScreen);

		int x = int(vecScreen[0]);
		int y = int(vecScreen[1]);

		static char szIndex[16];

		if (g_Config.cvars.esp_box_index)
			snprintf(szIndex, sizeof(szIndex), g_Config.cvars.esp_box_entity_name ? " (%d)" : "(%d)", index);

		g_Drawing.DrawStringF(g_hFontESP, x, y, r, g, b, 255, FONT_ALIGN_CENTER, "%s%s",
								g_Config.cvars.esp_box_entity_name ? GetEntityClassname(classInfo) : "",
								g_Config.cvars.esp_box_index ? szIndex : "");
	}
}

void CVisual::DrawEntityInfo_L4D(int index, class_info_t classInfo, Vector vecTop, int r, int g, int b)
{
	if (g_Config.cvars.esp_box_entity_name || g_Config.cvars.esp_box_index)
	{
		float vecScreen[2];
		vecTop.z += 14.f;

		UTIL_WorldToScreen(vecTop, vecScreen);

		int x = int(vecScreen[0]);
		int y = int(vecScreen[1]);

		static char szIndex[16];

		if (g_Config.cvars.esp_box_index)
			snprintf(szIndex, sizeof(szIndex), g_Config.cvars.esp_box_entity_name ? " (%d)" : "(%d)", index);

		g_Drawing.DrawStringF(g_hFontESP2, x, y, r, g, b, 255, FONT_ALIGN_CENTER, "%s%s",
								g_Config.cvars.esp_box_entity_name ? GetEntityClassname(classInfo) : "",
								g_Config.cvars.esp_box_index ? szIndex : "");
	}
}

void CVisual::DrawBox(bool bPlayer, bool bItem, int iHealth, int x, int y, int w, int h, int r, int g, int b)
{
	if ( (bPlayer && (iHealth > 0 || iHealth < -1)) || (!bPlayer && !bItem) )
	{
		int nBox = g_Config.cvars.esp_box;
		bool bOutline = g_Config.cvars.esp_box_outline;

		if ( g_Config.cvars.esp_box_fill != 0 )
		{
			g_Drawing.FillArea(x, y, w, h, r, g, b, g_Config.cvars.esp_box_fill);
		}

		if (nBox == 1)
		{
			g_Drawing.Box(x, y, w, h, 1, r, g, b, 200);
				
			if (bOutline)
				g_Drawing.BoxOutline(x, y, w, h, 1, r, g, b, 200);
		}
		else if (nBox == 2)
		{
			g_Drawing.DrawCoalBox(x, y, w, h, 1, r, g, b, 255);

			if (bOutline)
				g_Drawing.DrawOutlineCoalBox(x, y, w, h, 1, r, g, b, 255);
		}
		else if (nBox == 3)
		{
			g_Drawing.BoxCorner(x, y, w, h, 1, r, g, b, 255);

			if (bOutline)
				g_Drawing.BoxCornerOutline(x, y, w, h, 1, r, g, b, 255);
		}
	}
}

void CVisual::DrawBones(int index, studiohdr_t *pStudioHeader)
{
	#ifdef PROCESS_PLAYER_BONES_ONLY
	if ((g_Config.cvars.esp_skeleton || g_Config.cvars.esp_bones_name) && bPlayer)
#else
	if (g_Config.cvars.esp_skeleton || g_Config.cvars.esp_bones_name)
#endif
	{
	#pragma warning(push)
	#pragma warning(disable : 6011)

		mstudiobone_t *pBone = (mstudiobone_t *)((byte *)pStudioHeader + pStudioHeader->boneindex);

		for (int j = 0; j < pStudioHeader->numbones; ++j)
		{
			bool bBonePoint = false;
			float vBonePoint[2];

			if ((bBonePoint = UTIL_WorldToScreen(g_Bones[index].vecPoint[j], vBonePoint)) && g_Config.cvars.esp_bones_name)
				g_Drawing.DrawStringF(g_hFontESP, int(vBonePoint[0]), int(vBonePoint[1]), 255, 255, 255, 255, FONT_ALIGN_CENTER, "%s", pBone[j].name);

			if (g_Config.cvars.esp_skeleton)
			{
				float vParentPoint[2];

				if ( !bBonePoint || g_Bones[index].nParent[j] == -1 )
					continue;

				if ( !UTIL_WorldToScreen(g_Bones[index].vecPoint[g_Bones[index].nParent[j]], vParentPoint) )
					continue;

				g_Drawing.DrawLine(int(vBonePoint[0]), int(vBonePoint[1]), int(vParentPoint[0]), int(vParentPoint[1]), 255, 255, 255, 255);
			}
		}

	#pragma warning(pop)
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

void CVisual::ProcessBones()
{
	if ( !g_Config.cvars.esp )
		return;

	if ( !g_Config.cvars.esp_skeleton && !g_Config.cvars.esp_bones_name )
		return;

	cl_entity_s *pEntity = g_pEngineStudio->GetCurrentEntity();

	int index = pEntity->index;

#ifdef PROCESS_PLAYER_BONES_ONLY
	if ( !pEntity->player )
		return;
#endif

	cl_entity_s *pLocal = g_pEngineFuncs->GetLocalPlayer();
	CEntity &ent = g_EntityList.GetList()[index];

	if ( index == pLocal->index )
		return;

	if ( !ent.m_bValid )
		return;

	bool bPlayer = ent.m_bPlayer;
	bool bItem = ent.m_bItem;

	if ( !ent.m_bVisible )
		return;

	if ( !bPlayer )
	{
		if ( ent.m_classInfo.id == CLASS_NONE && g_Config.cvars.esp_ignore_unknown_ents )
			return;
	}

	Vector vecBottom = pEntity->origin;
	Vector vecTop = pEntity->origin;

	float vecScreenBottom[2], vecScreenTop[2];

	if ( !bPlayer )
	{
		if ( !bItem )
		{
			vecTop.z += pEntity->curstate.maxs.z;
			vecBottom.z -= pEntity->curstate.mins.z;
		}
	}
	else
	{
		if ( ent.m_bDucked )
		{
			vecTop.z += VEC_DUCK_HULL_MAX.z;
			vecBottom.z += VEC_DUCK_HULL_MIN.z;
		}
		else
		{
			vecTop.z += pEntity->curstate.maxs.z;
			vecBottom.z -= pEntity->curstate.maxs.z;
		}
	}

	if ( !UTIL_WorldToScreen(vecBottom, vecScreenBottom) || !UTIL_WorldToScreen(vecTop, vecScreenTop) )
		return;

	//memset(g_Bones[index].nParent, -1, sizeof(int) * MAXSTUDIOBONES);

	Vector vecFrameVelocity = ent.m_vecVelocity * ent.m_frametime;
	mstudiobone_t *pBone = (mstudiobone_t *)((byte *)ent.m_pStudioHeader + ent.m_pStudioHeader->boneindex);

	Assert( ent.m_pStudioHeader->numbones < MAXSTUDIOBONES );

	for (int i = 0; i < ent.m_pStudioHeader->numbones; ++i)
	{
		Vector vecBone( (*g_pBoneTransform)[i][0][3], (*g_pBoneTransform)[i][1][3], (*g_pBoneTransform)[i][2][3] );

		vecBone += vecFrameVelocity;

		g_Bones[index].vecPoint[i] = vecBone;
		g_Bones[index].nParent[i] = pBone[i].parent;
	}
}

bool CVisual::StudioRenderModel()
{
	float flDrawEntitiesMode = float(g_Config.cvars.draw_entities) + 1.0f;

	if ( g_Config.cvars.draw_entities == 7 ) // don't draw player models
	{
		flDrawEntitiesMode = 1.0f;
	}

	if (flDrawEntitiesMode == 6.0f)
	{
		cl_entity_s *pEntity = g_pEngineStudio->GetCurrentEntity();

		if ( pEntity->player )
			r_drawentities->value = 2.0f;
		else
			r_drawentities->value = 1.0f;
	}
	else if (flDrawEntitiesMode == 7.0f)
	{
		cl_entity_s *pEntity = g_pEngineStudio->GetCurrentEntity();

		if ( pEntity->player )
			r_drawentities->value = 3.0f;
		else
			r_drawentities->value = 1.0f;
	}
	else
	{
		r_drawentities->value = flDrawEntitiesMode;
	}

	if (g_Config.cvars.wallhack && r_drawentities->value >= 2.0f && r_drawentities->value <= 5.0f)
	{
		glDisable(GL_DEPTH_TEST);

		g_pStudioRenderer->StudioRenderFinal_Hardware();

		glEnable(GL_DEPTH_TEST);

		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Hooked user messages
//-----------------------------------------------------------------------------

int UserMsgHook_ScreenShake(const char *pszUserMsg, int iSize, void *pBuffer)
{
	if (g_Config.cvars.no_shake)
		return 0;

	return ORIG_UserMsgHook_ScreenShake(pszUserMsg, iSize, pBuffer);
}

int UserMsgHook_ScreenFade(const char *pszUserMsg, int iSize, void *pBuffer)
{
	if (g_Config.cvars.no_fade)
		return 0;

	return ORIG_UserMsgHook_ScreenFade(pszUserMsg, iSize, pBuffer);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

CVisual::CVisual()
{
	m_flTime = 0.f;

	m_flPrevTime = 0.f;
	m_flFadeTime = g_Config.cvars.jumpspeed_fade_duration;
	m_flJumpSpeed = 0.f;

	m_clFadeFrom[0] = int(255.f * g_Config.cvars.speed_color[0]);
	m_clFadeFrom[1] = int(255.f * g_Config.cvars.speed_color[1]);
	m_clFadeFrom[2] = int(255.f * g_Config.cvars.speed_color[2]);

	m_bOnGround = true;

	m_hUserMsgHook_ScreenShake = 0;
	m_hUserMsgHook_ScreenFade = 0;

	m_iScreenWidth = 1920;
	m_iScreenHeight = 1080;
}

bool CVisual::Load()
{
	r_drawentities = CVar()->FindCvar("r_drawentities");

	if ( !r_drawentities )
	{
		Warning("Can't find cvar \"r_drawentities\"\n");
		return false;
	}

	return true;
}

void CVisual::PostLoad()
{
	g_Drawing.SetupFonts();

	g_pBoneTransform = (bone_matrix3x4_t *)g_pEngineStudio->StudioGetLightTransform();

	m_hUserMsgHook_ScreenShake = Hooks()->HookUserMessage( "ScreenShake", UserMsgHook_ScreenShake, &ORIG_UserMsgHook_ScreenShake );
	m_hUserMsgHook_ScreenFade = Hooks()->HookUserMessage( "ScreenFade", UserMsgHook_ScreenFade, &ORIG_UserMsgHook_ScreenFade );
}

void CVisual::Unload()
{
	Hooks()->UnhookUserMessage( m_hUserMsgHook_ScreenShake );
	Hooks()->UnhookUserMessage( m_hUserMsgHook_ScreenFade );
}