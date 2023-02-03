// Radar

#include <IUtils.h>
#include <IClient.h>
#include <IVGUI.h>

#include <hl_sdk/engine/APIProxy.h>
#include <hl_sdk/common/r_studioint.h>

#include "radar.h"

#include "../game/utils.h"
#include "../game/drawing.h"
#include "../game/entitylist.h"

#include "../config.h"

//-----------------------------------------------------------------------------
// Vars
//-----------------------------------------------------------------------------

CRadar g_Radar;

//-----------------------------------------------------------------------------
// Radar implementation
//-----------------------------------------------------------------------------

CRadar::CRadar()
{
	m_iRadarRoundTexture = -1;
}

void CRadar::Init()
{
	m_iRadarRoundTexture = VGUI()->Surface()->CreateNewTextureID(true);
	VGUI()->Surface()->DrawSetTextureFile(m_iRadarRoundTexture, "sven_internal/tex/radar_round", true, false);
}

void CRadar::Draw()
{
	if ( g_Config.cvars.radar && !Client()->IsSpectating() )
	{
		constexpr int iThickness = 1;

		Vector va;
		CEntity *pEnts = g_EntityList.GetList();
		Vector2D vecCenter = g_pEngineFuncs->GetLocalPlayer()->curstate.origin.AsVector2D();

		g_pEngineFuncs->GetViewAngles(va);

		int x = (int)((float)Utils()->GetScreenWidth() * g_Config.cvars.radar_width_frac);
		int y = (int)((float)Utils()->GetScreenHeight() * g_Config.cvars.radar_height_frac);

		float angle = va.y * static_cast<float>(M_PI) / 180.f;

		float cy = cosf(-angle + static_cast<float>(M_PI) / 2);
		float sy = sinf(-angle + static_cast<float>(M_PI) / 2);

		// Transformation from world coordinates to screen coordinates
		float world2radar[2][3] =
		{
			{ (float)(g_Config.cvars.radar_size / g_Config.cvars.radar_distance), 0.f, (float)(g_Config.cvars.radar_size / 2) },
			{ 0.f, (float)(-g_Config.cvars.radar_size / g_Config.cvars.radar_distance), (float)(g_Config.cvars.radar_size / 2) }
		};

		if ( g_Config.cvars.radar_type == 0 ) // round
		{
			//g_Drawing.FillArea(x, y, g_Config.cvars.radar_size, g_Config.cvars.radar_size, 0, 0, 0, 255 / 2);

			// Round background
			g_Drawing.DrawTexture(m_iRadarRoundTexture, x, y, x + g_Config.cvars.radar_size, y + g_Config.cvars.radar_size, 255, 255, 255, 127);

			// Cross
			g_Drawing.DrawLine(x + g_Config.cvars.radar_size / 2, y, x + g_Config.cvars.radar_size / 2, y + g_Config.cvars.radar_size, 90, 90, 90, 127);
			g_Drawing.DrawLine(x, y + g_Config.cvars.radar_size / 2, x + g_Config.cvars.radar_size, y + g_Config.cvars.radar_size / 2, 90, 90, 90, 127);

			// Draw middle point
			g_Drawing.FillArea(x + g_Config.cvars.radar_size / 2 - iThickness, y + g_Config.cvars.radar_size / 2 - iThickness, iThickness * 2, iThickness * 2, 255, 255, 255, 255);

			for (register int i = 1; i <= g_EntityList.GetMaxEntities(); i++)
			{
				CEntity &ent = pEnts[i];

				if ( !ent.m_bValid )
					continue;

				int r = int(255.f * g_Config.cvars.esp_friend_color[0]);
				int g = int(255.f * g_Config.cvars.esp_friend_color[1]);
				int b = int(255.f * g_Config.cvars.esp_friend_color[2]);

				if ( ent.m_bItem )
				{
					if ( ent.m_bEnemy )
					{
						r = int(255.f * g_Config.cvars.esp_enemy_color[0]);
						g = int(255.f * g_Config.cvars.esp_enemy_color[1]);
						b = int(255.f * g_Config.cvars.esp_enemy_color[2]);
					}
					else if ( ent.m_bNeutral )
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
				else if ( ent.m_bNeutral )
				{
					r = int(255.f * g_Config.cvars.esp_neutral_color[0]);
					g = int(255.f * g_Config.cvars.esp_neutral_color[1]);
					b = int(255.f * g_Config.cvars.esp_neutral_color[2]);
				}
				else if ( ent.m_bEnemy )
				{
					r = int(255.f * g_Config.cvars.esp_enemy_color[0]);
					g = int(255.f * g_Config.cvars.esp_enemy_color[1]);
					b = int(255.f * g_Config.cvars.esp_enemy_color[2]);
				}

				Vector2D vecDir;
				Vector2D temp = ent.m_vecOrigin.AsVector2D() - vecCenter;

				vecDir.x = temp.x * cy - temp.y * sy;
				vecDir.y = temp.x * sy + temp.y * cy;

				// Clamp
				if ( vecDir.LengthSqr() > M_SQR(g_Config.cvars.radar_distance / 2) )
				{
					vecDir = vecDir.Normalize() * (g_Config.cvars.radar_distance / 2);
				}

				int point_x = DotProduct(vecDir, *reinterpret_cast<Vector2D *>(world2radar[0])) + world2radar[0][2];
				int point_y = DotProduct(vecDir, *reinterpret_cast<Vector2D *>(world2radar[1])) + world2radar[1][2];

				if (ent.m_bPlayer)
				{
					if (g_Config.cvars.radar_show_player_name)
					{
						player_info_t *pPlayer = g_pEngineStudio->PlayerInfo(i - 1);
						g_Drawing.DrawStringF(g_hFontESP, point_x + x, point_y + y - 8, r, g, b, 255, FONT_ALIGN_CENTER, "%s", pPlayer->name);
					}
				}
				else if (g_Config.cvars.radar_show_entity_name)
				{
					g_Drawing.DrawStringF(g_hFontESP, point_x + x, point_y + y - 8, r, g, b, 255, FONT_ALIGN_CENTER, "%s", GetEntityClassname(ent.m_classInfo));
				}

				g_Drawing.FillArea(point_x + x - iThickness, point_y + y - iThickness, iThickness * 2, iThickness * 2, r, g, b, 255);
			}
		}
		else // square
		{
			// Square background
			g_Drawing.FillArea(x, y, g_Config.cvars.radar_size, g_Config.cvars.radar_size, 0, 0, 0, 255 / 2);

			// Cross
			g_Drawing.DrawLine(x + g_Config.cvars.radar_size / 2, y, x + g_Config.cvars.radar_size / 2, y + g_Config.cvars.radar_size, 90, 90, 90, 127);
			g_Drawing.DrawLine(x, y + g_Config.cvars.radar_size / 2, x + g_Config.cvars.radar_size, y + g_Config.cvars.radar_size / 2, 90, 90, 90, 127);

			// Draw middle point
			g_Drawing.FillArea(x + g_Config.cvars.radar_size / 2 - iThickness, y + g_Config.cvars.radar_size / 2 - iThickness, iThickness * 2, iThickness * 2, 255, 255, 255, 255);

			for (register int i = 1; i <= g_EntityList.GetMaxEntities(); i++)
			{
				CEntity &ent = pEnts[i];

				if ( !ent.m_bValid )
					continue;

				int r = int(255.f * g_Config.cvars.esp_friend_color[0]);
				int g = int(255.f * g_Config.cvars.esp_friend_color[1]);
				int b = int(255.f * g_Config.cvars.esp_friend_color[2]);

				if ( ent.m_bItem )
				{
					if ( ent.m_bEnemy )
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
				else if ( ent.m_bNeutral )
				{
					r = int(255.f * g_Config.cvars.esp_neutral_color[0]);
					g = int(255.f * g_Config.cvars.esp_neutral_color[1]);
					b = int(255.f * g_Config.cvars.esp_neutral_color[2]);
				}
				else if ( ent.m_bEnemy )
				{
					r = int(255.f * g_Config.cvars.esp_enemy_color[0]);
					g = int(255.f * g_Config.cvars.esp_enemy_color[1]);
					b = int(255.f * g_Config.cvars.esp_enemy_color[2]);
				}

				Vector2D vecDir;
				Vector2D temp = ent.m_vecOrigin.AsVector2D() - vecCenter;

				vecDir.x = temp.x * cy - temp.y * sy;
				vecDir.y = temp.x * sy + temp.y * cy;

				// Clamp
				if ( fabs(vecDir.x) > (g_Config.cvars.radar_distance / 2) || fabs(vecDir.y) > (g_Config.cvars.radar_distance / 2) )
				{
					float scale;

					if (abs(vecDir.x) > abs(vecDir.y))
					{
						scale = fabs(vecDir.x) / (g_Config.cvars.radar_distance / 2);
					}
					else
					{
						scale = fabs(vecDir.y) / (g_Config.cvars.radar_distance / 2);
					}

					vecDir.x /= scale;
					vecDir.y /= scale;
				}

				int point_x = DotProduct(vecDir, *reinterpret_cast<Vector2D *>(world2radar[0])) + world2radar[0][2];
				int point_y = DotProduct(vecDir, *reinterpret_cast<Vector2D *>(world2radar[1])) + world2radar[1][2];

				if (ent.m_bPlayer)
				{
					if (g_Config.cvars.radar_show_player_name)
					{
						player_info_t *pPlayer = g_pEngineStudio->PlayerInfo(i - 1);
						g_Drawing.DrawStringF(g_hFontESP, point_x + x, point_y + y - 8, r, g, b, 255, FONT_ALIGN_CENTER, "%s", pPlayer->name);
					}
				}
				else if (g_Config.cvars.radar_show_entity_name)
				{
					g_Drawing.DrawStringF(g_hFontESP, point_x + x, point_y + y - 8, r, g, b, 255, FONT_ALIGN_CENTER, "%s", GetEntityClassname(ent.m_classInfo));
				}

				g_Drawing.FillArea(point_x + x - iThickness, point_y + y - iThickness, iThickness * 2, iThickness * 2, r, g, b, 255);
			}
		}
	}
}