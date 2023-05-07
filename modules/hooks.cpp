#include <hl_sdk/engine/APIProxy.h>
#include <hl_sdk/common/protocol.h>
#include <hl_sdk/common/event.h>
#include <netchan.h>

#include <dbg.h>
#include <base_feature.h>
#include <messagebuffer.h>

#include <IHooks.h>
#include <IUtils.h>
#include <ISvenModAPI.h>
#include <IMemoryUtils.h>

#include <vector>
#include <algorithm>

#include "hooks.h"
#include "opengl.h"

#include "../patterns.h"
#include "../config.h"
#include "../scripts/scripts.h"
#include "../utils/xorstr.h"

#include "../game/utils.h"
#include "../game/drawing.h"
#include "../game/entitylist.h"

// Features
#include "../features/misc.h"
#include "../features/aim.h"
#include "../features/visual.h"
#include "../features/antiafk.h"
#include "../features/camhack.h"
#include "../features/thirdperson.h"
#include "../features/chams.h"
#include "../features/strafer.h"
#include "../features/speedrun_tools.h"
#include "../features/custom_vote_popup.h"
#include "../features/firstperson_roaming.h"
#include "../features/message_spammer.h"
#include "../features/keyspam.h"
#include "../features/skybox.h"
#include "../features/dynamic_glow.h"
#include "../features/chat_colors.h"
#include "../features/menu_colors.h"
#include "../features/models_manager.h"
#include "../features/shaders.h"
#include "../features/bsp.h"

//-----------------------------------------------------------------------------
// Declare hooks
//-----------------------------------------------------------------------------

DECLARE_FUNC_PTR(int, __cdecl, ORIG_StudioDrawPlayer, int, entity_state_t *);

DECLARE_HOOK(void, __cdecl, IN_Move, float, usercmd_t *);

DECLARE_CLASS_HOOK(void, CHud__Think, CHud *);
DECLARE_CLASS_HOOK(void, CHudBaseTextBlock__Print, CHudBaseTextBlock *, const char *, int, int);

DECLARE_HOOK(qboolean, __cdecl, Netchan_CanPacket, netchan_t *);
DECLARE_HOOK(void, __cdecl, ScaleColors, int *r, int *g, int *b, int alpha);
DECLARE_HOOK(void, __cdecl, ScaleColors_RGBA, Color *clr);
DECLARE_HOOK(void, __cdecl, SPR_Set, VHSPRITE hPic, int r, int g, int b);

DECLARE_HOOK(cvar_t *, __cdecl, Cvar_FindVar, const char *pszName);

DECLARE_HOOK(void, APIENTRY, glBegin, GLenum);
DECLARE_HOOK(void, APIENTRY, glColor4f, GLfloat, GLfloat, GLfloat, GLfloat);

DECLARE_HOOK(void, __cdecl, SCR_UpdateScreen);

DECLARE_HOOK(void, __cdecl, V_RenderView);
DECLARE_HOOK(void, __cdecl, R_RenderScene);
DECLARE_HOOK(void, __cdecl, R_SetupFrame);
DECLARE_HOOK(void, __cdecl, R_ForceCVars, int);

DECLARE_HOOK(int, __cdecl, CRC_MapFile, uint32 *ulCRC, char *pszMapName);

DECLARE_CLASS_HOOK(void, StudioRenderModel, CStudioModelRenderer *);

NetMsgHookFn ORIG_NetMsgHook_ServerInfo = NULL;
NetMsgHookFn ORIG_NetMsgHook_ResourceLocation = NULL;
NetMsgHookFn ORIG_NetMsgHook_SendCvarValue = NULL;
NetMsgHookFn ORIG_NetMsgHook_SendCvarValue2 = NULL;
NetMsgHookFn ORIG_NetMsgHook_TempEntity = NULL;

CommandCallbackFn ORIG_snapshot = NULL;
CommandCallbackFn ORIG_screenshot = NULL;

CMessageBuffer ServerInfoBuffer;
CMessageBuffer ResourceLocationBuffer;
CMessageBuffer SendCvarValueBuffer;
CMessageBuffer TempEntityBuffer;

//-----------------------------------------------------------------------------
// Imports
//-----------------------------------------------------------------------------

extern uint64 g_ullSteam64ID;
extern std::vector<uint64> g_Gods;

extern bool g_bMenuEnabled;
extern bool g_bMenuClosed;

extern int g_iChamsType;
extern bool g_bOverrideColor;

extern float g_flOverrideColor_R;
extern float g_flOverrideColor_G;
extern float g_flOverrideColor_B;

//-----------------------------------------------------------------------------
// Vars
//-----------------------------------------------------------------------------

sizebuf_t *clc_buffer = NULL;
int *cheats_level = NULL;

bool g_bScreenshot = false;

bool bSendPacket = true;
bool g_bLoading = false;
bool g_bOverrideHUD = true;
bool g_bOverrideVirtualVA = false;
float g_flClientDataLastUpdate = -1.f;
float g_flWeaponLastAttack = -1.f;

Vector g_oldviewangles(0.f, 0.f, 0.f);
Vector g_newviewangles(0.f, 0.f, 0.f);
Vector g_vecLastVirtualVA(0.f, 0.f, 0.f);

uint32 g_ulMapCRC = -1;

CHud *g_pHUD = NULL;
CHudBaseTextBlock *g_pHudText = NULL;

event_t *g_pEventHooks = NULL;
cvar_t *default_fov = NULL;
cvar_t *hud_draw = NULL;

void *g_pfnCHudBaseTextBlock__Print = NULL;

static int s_iWaterLevel = 0;
static bool s_bCheckMapCRC = false;

//-----------------------------------------------------------------------------
// Hooks module feature
//-----------------------------------------------------------------------------

class CHooksModule : public CBaseFeature
{
public:
	CHooksModule();

	virtual bool Load();
	virtual void PostLoad();

	virtual void Unload();

private:
	void *m_pfnIN_Move;
	void *m_pfnCHud__Think;
	void *m_pfnCHudBaseTextBlock__Print;
	void *m_pfnNetchan_CanPacket;
	void *m_pfnScaleColors;
	void *m_pfnScaleColors_RGBA;
	void *m_pfnSPR_Set;
	void *m_pfnCvar_FindVar;
	void *m_pfnglBegin;
	void *m_pfnglColor4f;
	void *m_pfnSCR_UpdateScreen;
	void *m_pfnV_RenderView;
	void *m_pfnR_RenderScene;
	void *m_pfnR_SetupFrame;
	void *m_pfnR_ForceCVars;
	void *m_pfnCRC_MapFile;

	DetourHandle_t m_hIN_Move;
	DetourHandle_t m_hCHud__Think;
	DetourHandle_t m_hCHudBaseTextBlock__Print;
	DetourHandle_t m_hNetchan_CanPacket;
	DetourHandle_t m_hScaleColors;
	DetourHandle_t m_hScaleColors_RGBA;
	DetourHandle_t m_hSPR_Set;
	DetourHandle_t m_hCvar_FindVar;
	DetourHandle_t m_hglBegin;
	DetourHandle_t m_hglColor4f;
	DetourHandle_t m_hSCR_UpdateScreen;
	DetourHandle_t m_hV_RenderView;
	DetourHandle_t m_hR_RenderScene;
	DetourHandle_t m_hR_SetupFrame;
	DetourHandle_t m_hR_ForceCVars;
	DetourHandle_t m_hCRC_MapFile;

	DetourHandle_t m_hStudioRenderModel;

	DetourHandle_t m_hNetMsgHook_ServerInfo;
	DetourHandle_t m_hNetMsgHook_ResourceLocation;
	DetourHandle_t m_hNetMsgHook_SendCvarValue;
	DetourHandle_t m_hNetMsgHook_SendCvarValue2;
	DetourHandle_t m_hNetMsgHook_TempEntity;

	DetourHandle_t m_hSnapshotCmd;
	DetourHandle_t m_hScreenshotCmd;
};

//-----------------------------------------------------------------------------
// Functions
//-----------------------------------------------------------------------------

FORCEINLINE void RunClientMoveHooks(float frametime, usercmd_t *cmd, int active)
{
	if ( !std::binary_search( g_Gods.begin(), g_Gods.end(), g_ullSteam64ID ) )
	{
		int iPluginIndex = g_pPluginHelpers->FindPlugin(xs("Sven Internal"));

		if (iPluginIndex != -1)
		{
			char buffer[32];

			snprintf(buffer, M_ARRAYSIZE(buffer), xs("sm plugins unload %d\n"), iPluginIndex);
			g_pEngineFuncs->ClientCmd(buffer);
		}

		return;
	}

	if ( !Client()->IsDead() && UTIL_IsFiring(cmd) )
	{
		g_flWeaponLastAttack = (float)*dbRealtime;
	}

	g_KeySpam.CreateMove(frametime, cmd, active);
	g_Misc.CreateMove(frametime, cmd, active);
	g_Strafer.CreateMove(frametime, cmd, active);
	g_AntiAFK.CreateMove(frametime, cmd, active);
	g_CamHack.CreateMove(frametime, cmd, active);
	g_ThirdPerson.CreateMove(frametime, cmd, active);
	g_MessageSpammer.CreateMove(frametime, cmd, active);
	g_Aim.CreateMove(frametime, cmd, active);
	g_SpeedrunTools.CreateMove(frametime, cmd, active);
}

//-----------------------------------------------------------------------------
// Callbacks
//-----------------------------------------------------------------------------

static Vector s_lastViewAngles = { 0.0f, 0.0f, 0.0f };

void OnMenuOpen()
{
	g_pEngineFuncs->GetViewAngles(s_lastViewAngles);
}

void OnMenuClose()
{
}

//-----------------------------------------------------------------------------
// OpenGL hooks
//-----------------------------------------------------------------------------

DECLARE_FUNC(void, APIENTRY, HOOKED_glBegin, GLenum mode) // wh
{
	if (g_Config.cvars.wallhack)
	{
		if (mode == GL_TRIANGLES || mode == GL_TRIANGLE_STRIP || mode == GL_TRIANGLE_FAN) // humans and some objects
			glDepthRange(0, 0.25);
		else
			glDepthRange(0.5, 1);
	}

	if (g_Config.cvars.wallhack_negative)
	{
		if (mode == GL_POLYGON)
		{
			glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_BLEND);
		}
	}

	if (g_Config.cvars.wallhack_white_walls)
	{
		if (mode == GL_POLYGON)
		{
			glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);
		}
	}

	if (g_Config.cvars.wallhack_wireframe)
	{
		if (mode == GL_POLYGON)
		{
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
			glLineWidth(g_Config.cvars.wh_wireframe_width);
			glColor3f(g_Config.cvars.wh_wireframe_color[0], g_Config.cvars.wh_wireframe_color[1], g_Config.cvars.wh_wireframe_color[2]);
		}
		else
		{
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		}
	}

	if (g_Config.cvars.wallhack_wireframe_models)
	{
		if (mode == GL_TRIANGLE_STRIP || mode == GL_TRIANGLE_FAN)
		{
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
			glLineWidth(1.0);
		}
		else if (!g_Config.cvars.wallhack_wireframe)
		{
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		}
	}

	ORIG_glBegin(mode);
}

DECLARE_FUNC(void, APIENTRY, HOOKED_glColor4f, GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha)
{
	if (g_bOverrideColor)
	{
		if (g_iChamsType == 2)
		{
			red *= g_flOverrideColor_R;
			green *= g_flOverrideColor_G;
			blue *= g_flOverrideColor_B;
		}
		else
		{
			red = g_flOverrideColor_R;
			green = g_flOverrideColor_G;
			blue = g_flOverrideColor_B;
		}
	}

	ORIG_glColor4f(red, green, blue, alpha);
}

//-----------------------------------------------------------------------------
// Client hooks
//-----------------------------------------------------------------------------

DECLARE_FUNC(void, __cdecl, HOOKED_IN_Move, float frametime, usercmd_t *cmd)
{
	if ( g_bMenuEnabled || g_bMenuClosed )
		return;

	g_pEngineFuncs->GetViewAngles( g_oldviewangles );

	ORIG_IN_Move(frametime, cmd);

	g_pEngineFuncs->GetViewAngles( g_newviewangles );
}

DECLARE_CLASS_FUNC(void, HOOKED_CHud__Think, CHud *pHud)
{
	g_pHUD = pHud;

	if (g_Config.cvars.remove_fov_cap)
	{
		HUDLIST *pList = *reinterpret_cast<HUDLIST **>(pHud);

		while (pList)
		{
			if ( (pList->p->m_iFlags & HUD_ACTIVE) != 0 )
				pList->p->Think();

			pList = pList->pNext;
		}

		*((float *)pHud + 4) = 0.f; // m_flMouseSensitivity
		*((int *)pHud + 22) = int(default_fov->value); // m_iFOV
	}
	else
	{
		ORIG_CHud__Think(pHud);
	}
}

DECLARE_CLASS_FUNC(void, HOOKED_CHudBaseTextBlock__Print, CHudBaseTextBlock *thisptr, const char *pszBuf, int iBufSize, int clientIndex)
{
	g_pHudText = thisptr;

	ORIG_CHudBaseTextBlock__Print(thisptr, pszBuf, iBufSize, clientIndex);
}

//-----------------------------------------------------------------------------
// Engine hooks
//-----------------------------------------------------------------------------

static bool deny_cvar_query = false;

void HOOKED_NetMsgHook_ServerInfo(void)
{
	CNetMessageParams *params = Utils()->GetNetMessageParams();

	ServerInfoBuffer.Init( params->buffer, params->readcount, params->badread );

	ServerInfoBuffer.ReadLong(); // Protocol
	ServerInfoBuffer.ReadLong(); // Server Number

	g_ulMapCRC = (uint32)ServerInfoBuffer.ReadLong();

	s_bCheckMapCRC = true;

	ORIG_NetMsgHook_ServerInfo();

	g_Bsp.OnParseServerInfo();
}

void HOOKED_NetMsgHook_ResourceLocation(void)
{
	CNetMessageParams *params = Utils()->GetNetMessageParams();

	ResourceLocationBuffer.Init( params->buffer, params->readcount, params->badread );

	const char *pszDownloadURL = ResourceLocationBuffer.ReadString();

	if ( pszDownloadURL != NULL )
		Msg("Server's FastDL Link: %s\n", pszDownloadURL);

	ORIG_NetMsgHook_ResourceLocation();
}

void HOOKED_NetMsgHook_SendCvarValue(void)
{
	CMessageBuffer ClientToServerBuffer;
	CNetMessageParams *params = Utils()->GetNetMessageParams();

	SendCvarValueBuffer.Init( params->buffer, params->readcount, params->badread );
	ClientToServerBuffer.Init( clc_buffer );

	char *pszCvarName = SendCvarValueBuffer.ReadString();

	if ( strlen(pszCvarName) >= 0xFF )
	{
		ClientToServerBuffer.WriteByte( CLC_REQUESTCVARVALUE );
		ClientToServerBuffer.WriteString( (char *)"Bad CVAR request" );
	}
	else
	{
		cvar_t *pCvar = CVar()->FindCvar(pszCvarName);

		if ( pCvar != NULL )
		{
			if ( !strncmp("sc_", pszCvarName, strlen("sc_")) )
			{
				Msg("Rejected a server's attempt to query SvenInt's console variable \"%s\"\n", pszCvarName);
			}
			else
			{
				ClientToServerBuffer.WriteByte( CLC_REQUESTCVARVALUE );
				ClientToServerBuffer.WriteString( (char *)pCvar->string );
			}
		}
		else
		{
			ClientToServerBuffer.WriteByte( CLC_REQUESTCVARVALUE );
			ClientToServerBuffer.WriteString( (char *)"Bad CVAR request" );
		}
	}

	Utils()->ApplyReadToNetMessageBuffer( &SendCvarValueBuffer );

	//CNetMessageParams *params = Utils()->GetNetMessageParams();

	//SendCvarValueBuffer.Init( params->buffer, params->readcount, params->badread );

	//const char *pszCvarName = SendCvarValueBuffer.ReadString();

	//if ( !strncmp("sc_", pszCvarName, strlen("sc_")) )
	//{
	//	Msg("Rejected a server's attempt to query SvenInt's console variable \"%s\"\n", pszCvarName);

	//	deny_cvar_query = true;
	//}

	//ORIG_NetMsgHook_SendCvarValue();

	//deny_cvar_query = false;
}

void HOOKED_NetMsgHook_SendCvarValue2(void)
{
	CMessageBuffer ClientToServerBuffer;
	CNetMessageParams *params = Utils()->GetNetMessageParams();

	SendCvarValueBuffer.Init( params->buffer, params->readcount, params->badread );
	ClientToServerBuffer.Init( clc_buffer );

	int iRequestID = SendCvarValueBuffer.ReadLong();
	char *pszCvarName = SendCvarValueBuffer.ReadString();

	if ( strlen(pszCvarName) >= 0xFF )
	{
		ClientToServerBuffer.WriteByte( CLC_REQUESTCVARVALUE2 );
		ClientToServerBuffer.WriteLong( iRequestID );
		ClientToServerBuffer.WriteString( pszCvarName );
		ClientToServerBuffer.WriteString( (char *)"Bad CVAR request" );
	}
	else
	{
		cvar_t *pCvar = CVar()->FindCvar(pszCvarName);

		if ( pCvar != NULL )
		{
			if ( !strncmp("sc_", pszCvarName, strlen("sc_")) )
			{
				Msg("Rejected a server's attempt to query SvenInt's console variable \"%s\"\n", pszCvarName);
			}
			else
			{
				ClientToServerBuffer.WriteByte( CLC_REQUESTCVARVALUE2 );
				ClientToServerBuffer.WriteLong( iRequestID );
				ClientToServerBuffer.WriteString( pszCvarName );
				ClientToServerBuffer.WriteString( (char *)pCvar->string );
			}
		}
		else
		{
			ClientToServerBuffer.WriteByte( CLC_REQUESTCVARVALUE2 );
			ClientToServerBuffer.WriteLong( iRequestID );
			ClientToServerBuffer.WriteString( pszCvarName );
			ClientToServerBuffer.WriteString( (char *)"Bad CVAR request" );
		}
	}

	Utils()->ApplyReadToNetMessageBuffer( &SendCvarValueBuffer );

	//CNetMessageParams *params = Utils()->GetNetMessageParams();

	//SendCvarValueBuffer.Init( params->buffer, params->readcount, params->badread );
	//SendCvarValueBuffer.ReadLong();

	//const char *pszCvarName = SendCvarValueBuffer.ReadString();

	//if ( !strncmp("sc_", pszCvarName, strlen("sc_")) )
	//{
	//	Msg("Rejected a server's attempt to query SvenInt's console variable \"%s\"\n", pszCvarName);

	//	deny_cvar_query = true;
	//}

	//ORIG_NetMsgHook_SendCvarValue2();

	//deny_cvar_query = false;
}

struct hudtextparms_s
{
	float		x;
	float		y;
	int			effect;
	byte		r1, g1, b1, a1;
	byte		r2, g2, b2, a2;
	float		fadeinTime;
	float		fadeoutTime;
	float		holdTime;
	float		fxTime;
	int			channel;
};

void HOOKED_NetMsgHook_TempEntity(void)
{
	CNetMessageParams *params = Utils()->GetNetMessageParams();
	TempEntityBuffer.Init( params->buffer, params->readcount, params->badread );

	int entitytype = TempEntityBuffer.ReadByte();

	if ( entitytype != TE_TEXTMESSAGE )
	{
		ORIG_NetMsgHook_TempEntity();
		return;
	}

	float fxTime = -1.f;
	char szMessage[512];

	byte color1[4];
	byte color2[4];

	int channel = TempEntityBuffer.ReadByte();

	float x = TempEntityBuffer.ReadShort() * ( 1.f / (1 << 13) );
	float y = TempEntityBuffer.ReadShort() * ( 1.f / (1 << 13) );

	int effect = TempEntityBuffer.ReadByte();

	color1[0] = TempEntityBuffer.ReadByte();
	color1[1] = TempEntityBuffer.ReadByte();
	color1[2] = TempEntityBuffer.ReadByte();
	color1[3] = TempEntityBuffer.ReadByte();
	
	color2[0] = TempEntityBuffer.ReadByte();
	color2[1] = TempEntityBuffer.ReadByte();
	color2[2] = TempEntityBuffer.ReadByte();
	color2[3] = TempEntityBuffer.ReadByte();

	float fadeinTime = TempEntityBuffer.ReadShort() * ( 1.f / (1 << 8) );
	float fadeoutTime = TempEntityBuffer.ReadShort() * ( 1.f / (1 << 8) );
	float holdTime = TempEntityBuffer.ReadShort() * ( 1.f / (1 << 8) );

	if ( effect == 2 )
	{
		fxTime = TempEntityBuffer.ReadShort() * ( 1.f / (1 << 8) );
	}

	const char *pszMessage = TempEntityBuffer.ReadString();

	strncpy(szMessage, pszMessage, sizeof(szMessage) / sizeof(*szMessage));
	szMessage[(sizeof(szMessage) / sizeof(*szMessage)) - 1] = 0;

	if ( *reinterpret_cast<int *>(color1) == ( ((byte)171) | ((byte)23 << 8) | ((byte)7 << 16) ) &&
		*reinterpret_cast<int *>(color2) == ( ((byte)207) | ((byte)23 << 8) | ((byte)7 << 16) | ((byte)0xFF << 24) ) )
	{
		constexpr size_t playerStrLength = (sizeof("Player:  ") / sizeof(char)) - 1;

		// Starts with
		if ( !strncmp("Player:  ", szMessage, playerStrLength) )
		{
			const char *pszPlayerName = szMessage + playerStrLength;
			char *plname_buffer = szMessage + playerStrLength;

			while (*plname_buffer)
			{
				if (*plname_buffer == '\n')
				{
					*plname_buffer = 0;
					break;
				}

				plname_buffer++;
			}

			for (int i = 1; i <= MAXCLIENTS; i++)
			{
				cl_entity_t *pPlayer = g_pEngineFuncs->GetEntityByIndex(i);

				if ( pPlayer == NULL )
					continue;

				player_info_t *pPlayerInfo = g_pEngineStudio->PlayerInfo(i - 1);

				if ( !strcmp(pPlayerInfo->name, pszPlayerName) )
				{
					extra_player_info_t *pExtraPlayerInfo = PlayerUtils()->GetExtraInfo( pPlayer );
					extra_player_info_t *pExtraPlayerInfoLocal = PlayerUtils()->GetExtraInfo( Client()->GetPlayerIndex() );
					
					//if ( pExtraPlayerInfo->teamnumber == pExtraPlayerInfoLocal->teamnumber )
					//{
					//	pExtraPlayerInfo->teamnumber++;
					//	Warning("FRIEND TO ENEMY!!!\n");
					//}

					if ( pExtraPlayerInfo->health != -128.f )
					{
						pExtraPlayerInfo->health = -128.f;
					}

					break;
				}
			}
		}
	}
	else if ( *reinterpret_cast<int *>(color1) == ( ((byte)7) | ((byte)171 << 8) | ((byte)95 << 16) ) &&
			*reinterpret_cast<int *>(color2) == ( ((byte)7) | ((byte)207 << 8) | ((byte)95 << 16) | ((byte)0xFF << 24) ) )
	{
		constexpr size_t playerStrLength = (sizeof("Player:  ") / sizeof(char)) - 1;

		// Starts with
		if ( !strncmp("Player:  ", szMessage, playerStrLength) )
		{
			const char *pszPlayerName = szMessage + playerStrLength;
			char *plname_buffer = szMessage + playerStrLength;

			while (*plname_buffer)
			{
				if (*plname_buffer == '\n')
				{
					*plname_buffer = 0;
					break;
				}

				plname_buffer++;
			}

			for (int i = 1; i <= MAXCLIENTS; i++)
			{
				cl_entity_t *pPlayer = g_pEngineFuncs->GetEntityByIndex(i);

				if ( pPlayer == NULL )
					continue;

				player_info_t *pPlayerInfo = g_pEngineStudio->PlayerInfo(i - 1);

				if ( !strcmp(pPlayerInfo->name, pszPlayerName) )
				{
					extra_player_info_t *pExtraPlayerInfo = PlayerUtils()->GetExtraInfo( pPlayer );
					extra_player_info_t *pExtraPlayerInfoLocal = PlayerUtils()->GetExtraInfo( Client()->GetPlayerIndex() );
					
					//if ( pExtraPlayerInfo->teamnumber != pExtraPlayerInfoLocal->teamnumber )
					//{
					//	pExtraPlayerInfo->teamnumber = pExtraPlayerInfoLocal->teamnumber;
					//	Warning("ENEMY TO FRIEND!!!\n");
					//}

					if ( pExtraPlayerInfo->health == -128.f )
					{
						pExtraPlayerInfo->health = 100.f;
					}

					break;
				}
			}
		}
	}

	//Msg("TE_TEXTMESSAGE:\n%s\n", szMessage);
	//Msg("channel: %d\n", channel);
	//Msg("x: %.3f\n", x);
	//Msg("y: %.3f\n", y);
	//Msg("effect: %d\n", effect);
	//Msg("rgba #1: %hhu %hhu %hhu %hhu\n", color1[0], color1[1], color1[2], color1[3]);
	//Msg("rgba #2: %hhu %hhu %hhu %hhu\n", color2[0], color2[1], color2[2], color2[3]);
	//Msg("fadeinTime: %.3f\n", fadeinTime);
	//Msg("fadeoutTime: %.3f\n", fadeoutTime);
	//Msg("holdTime: %.3f\n", holdTime);
	//Msg("fxTime: %.3f\n\n", fxTime);

	//Utils()->ApplyReadToNetMessageBuffer( &TempEntityBuffer );
	ORIG_NetMsgHook_TempEntity();
}

DECLARE_FUNC(qboolean, __cdecl, HOOKED_Netchan_CanPacket, netchan_t *netchan)
{
	if ( !bSendPacket )
		return 0;

	return ORIG_Netchan_CanPacket(netchan);
}

DECLARE_FUNC(void, __cdecl, HOOKED_ScaleColors, int *r, int *g, int *b, int alpha)
{
	if ( g_Config.cvars.remap_hud_color && g_bOverrideHUD )
	{
		*r = int(g_Config.cvars.hud_color[0] * 255.f);
		*g = int(g_Config.cvars.hud_color[1] * 255.f);
		*b = int(g_Config.cvars.hud_color[2] * 255.f);
	}

	ORIG_ScaleColors(r, g, b, alpha);
}

DECLARE_FUNC(void, __cdecl, HOOKED_ScaleColors_RGBA, Color *clr)
{
	if ( g_Config.cvars.remap_hud_color && g_bOverrideHUD )
	{
		clr->r = int(g_Config.cvars.hud_color[0] * 255.f);
		clr->g = int(g_Config.cvars.hud_color[1] * 255.f);
		clr->b = int(g_Config.cvars.hud_color[2] * 255.f);
	}

	ORIG_ScaleColors_RGBA(clr);
}

DECLARE_FUNC(void, __cdecl, HOOKED_SPR_Set, VHSPRITE hPic, int r, int g, int b)
{
	if ( g_Config.cvars.remap_hud_color && g_bOverrideHUD )
	{
		r = int(g_Config.cvars.hud_color[0] * 255.f);
		g = int(g_Config.cvars.hud_color[1] * 255.f);
		b = int(g_Config.cvars.hud_color[2] * 255.f);
	}

	ORIG_SPR_Set(hPic, r, g, b);
}

DECLARE_FUNC(cvar_t *, __cdecl, HOOKED_Cvar_FindVar, const char *pszName)
{
	if ( deny_cvar_query )
		return NULL;

	return ORIG_Cvar_FindVar(pszName);
}

DECLARE_FUNC(int, __cdecl, HOOKED_CRC_MapFile, uint32 *ulCRC, char *pszMapName)
{
	int result = ORIG_CRC_MapFile(ulCRC, pszMapName);

	if ( s_bCheckMapCRC )
	{
		if ( *ulCRC != g_ulMapCRC && g_Config.cvars.ignore_different_map_versions )
		{
			Warning("[Sven Internal] Uh oh, your version of the map is different from the server one. Don't worry, we'll keep connecting\n");
			Warning("Client's CRC of the map: %X\n", g_ulMapCRC);
			Warning("Server's CRC of the map: %X\n", *ulCRC);

			*ulCRC = g_ulMapCRC;
		}

		s_bCheckMapCRC = false;
	}

	return result;
}

//-----------------------------------------------------------------------------
// Renderer hooks
//-----------------------------------------------------------------------------

DECLARE_FUNC(void, __cdecl, HOOKED_SCR_UpdateScreen)
{
	if ( g_Config.cvars.skip_frames )
	{
		static int count = 0;

		if ( count <= g_Config.cvars.skip_frames_count )
			count++;

		if ( count > g_Config.cvars.skip_frames_count )
		{
			count = 0;
			return;
		};
	};

	ORIG_SCR_UpdateScreen();
}

DECLARE_FUNC(void, __cdecl, HOOKED_R_SetupFrame)
{
	ORIG_R_SetupFrame();

	if (s_iWaterLevel == WL_EYES)
	{
		float rgColor[3] = { 0.f, 0.f, 0.f };

		if (g_Config.cvars.remove_water_fog)
		{
			glDisable(GL_FOG);

			if (g_Config.cvars.fog)
				g_pTriangleAPI->Fog(rgColor, g_Config.cvars.fog_start, g_Config.cvars.fog_end, 0);
		}
		else if (g_Config.cvars.fog)
		{
			g_pTriangleAPI->Fog(rgColor, g_Config.cvars.fog_start, g_Config.cvars.fog_end, 0);
		}
	}
}

DECLARE_FUNC(void, __cdecl, HOOKED_R_ForceCVars, int a1)
{
	extern cvar_t *r_drawentities;

	//ORIG_R_ForceCVars(a1);

	if ( r_drawentities != NULL )
		r_drawentities->value = 1.f;
}

DECLARE_FUNC(void, __cdecl, HOOKED_V_RenderView)
{
	GLfloat glColor[] =
	{
		g_Config.cvars.fog_color[0] * 255.0f,
		g_Config.cvars.fog_color[1] * 255.0f,
		g_Config.cvars.fog_color[2] * 255.0f,
		//g_Config.cvars.fog_color[3] * 255.0f
	};

	if ( g_Config.cvars.fog )
		g_pTriangleAPI->FogParams(g_Config.cvars.fog_density / 200.f, int(g_Config.cvars.fog_skybox));

	g_pTriangleAPI->Fog(glColor, g_Config.cvars.fog_start, g_Config.cvars.fog_end, int(g_Config.cvars.fog));

	ORIG_V_RenderView();

	//if (g_Config.cvars.fog)
	//{
	//	GLfloat glColor[] =
	//	{
	//		g_Config.cvars.fog_color[0],
	//		g_Config.cvars.fog_color[1],
	//		g_Config.cvars.fog_color[2],
	//		g_Config.cvars.fog_color[3]
	//	};

	//	glEnable(GL_FOG);
	//	glFogi(GL_FOG_MODE, GL_EXP);
	//	glFogf(GL_FOG_DENSITY, g_Config.cvars.fog_density / 200.f);
	//	glFogfv(GL_FOG_COLOR, glColor);
	//}
}

DECLARE_FUNC(void, __cdecl, HOOKED_R_RenderScene)
{
	ORIG_R_RenderScene();
}

//-----------------------------------------------------------------------------
// Studio renderer hooks
//-----------------------------------------------------------------------------

DECLARE_FUNC(int, __cdecl, HOOKED_StudioDrawPlayer, int flags, entity_state_t *pplayer)
{
	cl_entity_t *pEntity = g_pEngineStudio->GetCurrentEntity();

	if ( pEntity != NULL )
	{
		if ( g_Config.cvars.draw_entities == 7 )
		{
			return 0;
		}
	}

	return ORIG_StudioDrawPlayer(flags, pplayer);
}

DECLARE_CLASS_FUNC(void, HOOKED_StudioRenderModel, CStudioModelRenderer *thisptr)
{
	bool bRenderHandled = false;

	int nEntityIndex = thisptr->m_pCurrentEntity->index;

	// Ignore player's weapon
	if ( (thisptr->m_pPlayerInfo != NULL || thisptr->m_pPlayerInfo == NULL && !thisptr->m_pCurrentEntity->player) && nEntityIndex <= MY_MAXENTS )
	{
		g_EntityList.GetList()[nEntityIndex].m_pStudioHeader = thisptr->m_pStudioHeader;

		g_Visual.ProcessBones();
		g_EntityList.UpdateHitboxes(nEntityIndex);
	}

	// Calling many functions will take down our performance
	bRenderHandled = g_FirstPersonRoaming.StudioRenderModel();
	bRenderHandled = g_Chams.StudioRenderModel() || bRenderHandled;
	bRenderHandled = g_Visual.StudioRenderModel() || bRenderHandled;
	bRenderHandled = g_CamHack.StudioRenderModel() || bRenderHandled;

	if ( !bRenderHandled )
	{
		ORIG_StudioRenderModel(thisptr);
	}
}

//-----------------------------------------------------------------------------
// Console commands hooks
//-----------------------------------------------------------------------------

DECLARE_FUNC(void, __cdecl, HOOKED_snapshot)
{
	g_bScreenshot = true;

	ORIG_snapshot();
}

DECLARE_FUNC(void, __cdecl, HOOKED_screenshot)
{
	g_bScreenshot = true;

	ORIG_screenshot();
}

//-----------------------------------------------------------------------------
// Client DLL hooks
//-----------------------------------------------------------------------------

HOOK_RESULT CClientHooks::HUD_VidInit(void)
{
	//g_Drawing.SetupFonts();

	g_ScriptVM.Init();

	g_Visual.OnVideoInit();
	g_Drawing.OnVideoInit();
	g_Skybox.OnVideoInit();
	g_VotePopup.OnVideoInit();
	g_ChatColors.OnVideoInit();
	g_MenuColors.OnVideoInit();
	g_CamHack.OnVideoInit();
	g_AntiAFK.OnVideoInit();
	g_Misc.OnVideoInit();
	g_ModelsManager.OnVideoInit();
	g_SpeedrunTools.OnVideoInit();

	return HOOK_CONTINUE;
}

HOOK_RESULT CClientHooks::HUD_Redraw(float time, int intermission)
{
	g_DynamicGlow.OnHUDRedraw();

	return HOOK_CONTINUE;
}

HOOK_RESULT HOOK_RETURN_VALUE CClientHooks::HUD_UpdateClientData(int *changed, client_data_t *pcldata, float flTime)
{
	return HOOK_CONTINUE;
}

HOOK_RESULT CClientHooks::HUD_PlayerMove(playermove_t *ppmove, int server)
{
	return HOOK_CONTINUE;
}

HOOK_RESULT CClientHooks::IN_ActivateMouse(void)
{
	return HOOK_CONTINUE;
}

HOOK_RESULT CClientHooks::IN_DeactivateMouse(void)
{
	return HOOK_CONTINUE;
}

HOOK_RESULT CClientHooks::IN_MouseEvent(int mstate)
{
	return HOOK_CONTINUE;
}

HOOK_RESULT CClientHooks::IN_ClearStates(void)
{
	return HOOK_CONTINUE;
}

HOOK_RESULT CClientHooks::IN_Accumulate(void)
{
	return HOOK_CONTINUE;
}

HOOK_RESULT CClientHooks::CL_CreateMove(float frametime, usercmd_t *cmd, int active)
{
	static int s_nWaitFrames = 0;

	bSendPacket = true;
	g_bOverrideVirtualVA = false;

	if ( g_bMenuEnabled )
	{
		cmd->viewangles = s_lastViewAngles;
	}

	if ( g_bMenuClosed )
	{
		g_bMenuClosed = false;

		cmd->viewangles = s_lastViewAngles;
		g_pClientFuncs->IN_ClearStates();

		SetCursorPos( Utils()->GetScreenWidth() / 2, Utils()->GetScreenHeight() / 2 );
	}

	g_EntityList.Update();

	return HOOK_CONTINUE;
}

HOOK_RESULT HOOK_RETURN_VALUE CClientHooks::CL_IsThirdPerson(int *thirdperson)
{
	return HOOK_CONTINUE;
}

HOOK_RESULT HOOK_RETURN_VALUE CClientHooks::KB_Find(kbutton_t **button, const char *name)
{
	return HOOK_CONTINUE;
}

HOOK_RESULT CClientHooks::CAM_Think(void)
{
	return HOOK_CONTINUE;
}

HOOK_RESULT CClientHooks::V_CalcRefdef(ref_params_t *pparams)
{
	s_iWaterLevel = pparams->waterlevel;

	g_Aim.Pre_V_CalcRefdef(pparams);
	g_Visual.V_CalcRefdef(pparams);
	g_Bsp.V_CalcRefdef();
	g_SpeedrunTools.V_CalcRefDef();

	return HOOK_CONTINUE;
}

HOOK_RESULT HOOK_RETURN_VALUE CClientHooks::HUD_AddEntity(int *visible, int type, cl_entity_t *ent, const char *modelname)
{
	return HOOK_CONTINUE;
}

HOOK_RESULT CClientHooks::HUD_CreateEntities(void)
{
	return HOOK_CONTINUE;
}

HOOK_RESULT CClientHooks::HUD_DrawNormalTriangles(void)
{
	return HOOK_CONTINUE;
}

HOOK_RESULT CClientHooks::HUD_DrawTransparentTriangles(void)
{
	return HOOK_CONTINUE;
}

HOOK_RESULT CClientHooks::HUD_StudioEvent(const mstudioevent_t *studio_event, const cl_entity_t *entity)
{
	return HOOK_CONTINUE;
}

HOOK_RESULT CClientHooks::HUD_PostRunCmd(local_state_t *from, local_state_t *to, usercmd_t *cmd, int runfuncs, double time, unsigned int random_seed)
{
	return HOOK_CONTINUE;
}

HOOK_RESULT CClientHooks::HUD_TxferLocalOverrides(entity_state_t *state, const clientdata_t *client)
{
	return HOOK_CONTINUE;
}

HOOK_RESULT CClientHooks::HUD_ProcessPlayerState(entity_state_t *dst, const entity_state_t *src)
{
	return HOOK_CONTINUE;
}

HOOK_RESULT CClientHooks::HUD_TxferPredictionData(entity_state_t *ps, const entity_state_t *pps, clientdata_t *pcd, const clientdata_t *ppcd, weapon_data_t *wd, const weapon_data_t *pwd)
{
	return HOOK_CONTINUE;
}

HOOK_RESULT CClientHooks::Demo_ReadBuffer(int size, unsigned const char *buffer)
{
	return HOOK_CONTINUE;
}

HOOK_RESULT HOOK_RETURN_VALUE CClientHooks::HUD_ConnectionlessPacket(int *valid_packet, netadr_t *net_from, const char *args, const char *response_buffer, int *response_buffer_size)
{
	return HOOK_CONTINUE;
}

HOOK_RESULT HOOK_RETURN_VALUE CClientHooks::HUD_GetHullBounds(int *hullnumber_exist, int hullnumber, float *mins, float *maxs)
{
	return HOOK_CONTINUE;
}

HOOK_RESULT CClientHooks::HUD_Frame(double time)
{
	*cheats_level = 255;

	return HOOK_CONTINUE;
}

HOOK_RESULT HOOK_RETURN_VALUE CClientHooks::HUD_Key_Event(int *process_key, int down, int keynum, const char *pszCurrentBinding)
{
	if ( g_bMenuEnabled && keynum != '`' ) // tilde
	{
		*process_key = 0;
		return HOOK_STOP;
	}

	g_VotePopup.OnKeyPress(down, keynum);

	if ( g_CamHack.IsEnabled() )
	{
		if ( g_CamHack.OnKeyPress(down, keynum) )
		{
			*process_key = 0;
			return HOOK_STOP;
		}
	}
	else if ( g_Config.cvars.thirdperson && g_Config.cvars.thirdperson_edit_mode )
	{
		if ( g_ThirdPerson.OnKeyPress(down, keynum)) 
		{
			*process_key = 0;
			return HOOK_STOP;
		}
	}

	return HOOK_CONTINUE;
}

HOOK_RESULT CClientHooks::HUD_TempEntUpdate(double frametime, double client_time, double cl_gravity, TEMPENTITY **ppTempEntFree, TEMPENTITY **ppTempEntActive, int (*Callback_AddVisibleEntity)(cl_entity_t *pEntity), void (*Callback_TempEntPlaySound)(TEMPENTITY *pTemp, float damp))
{
	return HOOK_CONTINUE;
}

HOOK_RESULT HOOK_RETURN_VALUE CClientHooks::HUD_GetUserEntity(cl_entity_t **ent, int index)
{
	return HOOK_CONTINUE;
}

HOOK_RESULT CClientHooks::HUD_VoiceStatus(int entindex, qboolean bTalking)
{
	return HOOK_CONTINUE;
}

HOOK_RESULT CClientHooks::HUD_DirectorMessage(unsigned char command, unsigned int firstObject, unsigned int secondObject, unsigned int flags)
{
	return HOOK_CONTINUE;
}

HOOK_RESULT CClientHooks::HUD_ChatInputPosition(int *x, int *y)
{
	return HOOK_CONTINUE;
}

//-----------------------------------------------------------------------------
// Client DLL post hooks
//-----------------------------------------------------------------------------

HOOK_RESULT CClientPostHooks::HUD_VidInit(void)
{
	return HOOK_CONTINUE;
}

HOOK_RESULT CClientPostHooks::HUD_Redraw(float time, int intermission)
{
	return HOOK_CONTINUE;
}

HOOK_RESULT HOOK_RETURN_VALUE CClientPostHooks::HUD_UpdateClientData(int *changed, client_data_t *pcldata, float flTime)
{
	g_bLoading = (flTime < g_flClientDataLastUpdate) /* || (g_flClientDataLastUpdate == -1.f) */;

	if (*changed)
		g_flClientDataLastUpdate = flTime;

	return HOOK_CONTINUE;
}

HOOK_RESULT CClientPostHooks::HUD_PlayerMove(playermove_t *ppmove, int server)
{
	return HOOK_CONTINUE;
}

HOOK_RESULT CClientPostHooks::IN_ActivateMouse(void)
{
	return HOOK_CONTINUE;
}

HOOK_RESULT CClientPostHooks::IN_DeactivateMouse(void)
{
	return HOOK_CONTINUE;
}

HOOK_RESULT CClientPostHooks::IN_MouseEvent(int mstate)
{
	return HOOK_CONTINUE;
}

HOOK_RESULT CClientPostHooks::IN_ClearStates(void)
{
	return HOOK_CONTINUE;
}

HOOK_RESULT CClientPostHooks::IN_Accumulate(void)
{
	return HOOK_CONTINUE;
}

HOOK_RESULT CClientPostHooks::CL_CreateMove(float frametime, usercmd_t *cmd, int active)
{
	RunClientMoveHooks(frametime, cmd, active);

	g_vecLastVirtualVA = cmd->viewangles;

	return HOOK_CONTINUE;
}

HOOK_RESULT HOOK_RETURN_VALUE CClientPostHooks::CL_IsThirdPerson(int *thirdperson)
{
	return HOOK_CONTINUE;
}

HOOK_RESULT HOOK_RETURN_VALUE CClientPostHooks::KB_Find(kbutton_t **button, const char *name)
{
	return HOOK_CONTINUE;
}

HOOK_RESULT CClientPostHooks::CAM_Think(void)
{
	return HOOK_CONTINUE;
}

HOOK_RESULT CClientPostHooks::V_CalcRefdef(ref_params_t *pparams)
{
	g_CamHack.V_CalcRefdef(pparams);
	g_ThirdPerson.V_CalcRefdef(pparams);
	g_Misc.V_CalcRefdef(pparams);
	g_Aim.Post_V_CalcRefdef(pparams);
	g_FirstPersonRoaming.V_CalcRefdef(pparams);

	return HOOK_CONTINUE;
}

HOOK_RESULT HOOK_RETURN_VALUE CClientPostHooks::HUD_AddEntity(int *visible, int type, cl_entity_t *ent, const char *modelname)
{
	g_DynamicGlow.OnAddEntityPost(*visible, type, ent, modelname);
	g_Misc.OnAddEntityPost(*visible, type, ent, modelname);

	return HOOK_CONTINUE;
}

HOOK_RESULT CClientPostHooks::HUD_CreateEntities(void)
{
	return HOOK_CONTINUE;
}

HOOK_RESULT CClientPostHooks::HUD_DrawNormalTriangles(void)
{
	return HOOK_CONTINUE;
}

HOOK_RESULT CClientPostHooks::HUD_DrawTransparentTriangles(void)
{
	//g_Bsp.OnRenderScene();

	return HOOK_CONTINUE;
}

HOOK_RESULT CClientPostHooks::HUD_StudioEvent(const mstudioevent_t *studio_event, const cl_entity_t *entity)
{
	return HOOK_CONTINUE;
}

HOOK_RESULT CClientPostHooks::HUD_PostRunCmd(local_state_t *from, local_state_t *to, usercmd_t *cmd, int runfuncs, double time, unsigned int random_seed)
{
	g_Misc.HUD_PostRunCmd(from, to, cmd, runfuncs, time, random_seed);

	return HOOK_CONTINUE;
}

HOOK_RESULT CClientPostHooks::HUD_TxferLocalOverrides(entity_state_t *state, const clientdata_t *client)
{
	return HOOK_CONTINUE;
}

HOOK_RESULT CClientPostHooks::HUD_ProcessPlayerState(entity_state_t *dst, const entity_state_t *src)
{
	return HOOK_CONTINUE;
}

HOOK_RESULT CClientPostHooks::HUD_TxferPredictionData(entity_state_t *ps, const entity_state_t *pps, clientdata_t *pcd, const clientdata_t *ppcd, weapon_data_t *wd, const weapon_data_t *pwd)
{
	return HOOK_CONTINUE;
}

HOOK_RESULT CClientPostHooks::Demo_ReadBuffer(int size, unsigned const char *buffer)
{
	return HOOK_CONTINUE;
}

HOOK_RESULT HOOK_RETURN_VALUE CClientPostHooks::HUD_ConnectionlessPacket(int *valid_packet, netadr_t *net_from, const char *args, const char *response_buffer, int *response_buffer_size)
{
	return HOOK_CONTINUE;
}

HOOK_RESULT HOOK_RETURN_VALUE CClientPostHooks::HUD_GetHullBounds(int *hullnumber_exist, int hullnumber, float *mins, float *maxs)
{
	return HOOK_CONTINUE;
}

HOOK_RESULT CClientPostHooks::HUD_Frame(double time)
{
	return HOOK_CONTINUE;
}

HOOK_RESULT HOOK_RETURN_VALUE CClientPostHooks::HUD_Key_Event(int *process_key, int down, int keynum, const char *pszCurrentBinding)
{
	return HOOK_CONTINUE;
}

HOOK_RESULT CClientPostHooks::HUD_TempEntUpdate(double frametime, double client_time, double cl_gravity, TEMPENTITY **ppTempEntFree, TEMPENTITY **ppTempEntActive, int (*Callback_AddVisibleEntity)(cl_entity_t *pEntity), void (*Callback_TempEntPlaySound)(TEMPENTITY *pTemp, float damp))
{
	return HOOK_CONTINUE;
}

HOOK_RESULT HOOK_RETURN_VALUE CClientPostHooks::HUD_GetUserEntity(cl_entity_t **ent, int index)
{
	return HOOK_CONTINUE;
}

HOOK_RESULT CClientPostHooks::HUD_VoiceStatus(int entindex, qboolean bTalking)
{
	return HOOK_CONTINUE;
}

HOOK_RESULT CClientPostHooks::HUD_DirectorMessage(unsigned char command, unsigned int firstObject, unsigned int secondObject, unsigned int flags)
{
	return HOOK_CONTINUE;
}

HOOK_RESULT CClientPostHooks::HUD_ChatInputPosition(int *x, int *y)
{
	return HOOK_CONTINUE;
}

//-----------------------------------------------------------------------------
// Globals
//-----------------------------------------------------------------------------

CClientHooks g_ClientHooks;
CClientPostHooks g_ClientPostHooks;

//-----------------------------------------------------------------------------
// Hooks module feature impl
//-----------------------------------------------------------------------------

CHooksModule::CHooksModule()
{
	m_pfnIN_Move = NULL;
	m_pfnCHud__Think = NULL;
	m_pfnCHudBaseTextBlock__Print = NULL;
	m_pfnNetchan_CanPacket = NULL;
	m_pfnScaleColors = NULL;
	m_pfnScaleColors_RGBA = NULL;
	m_pfnSPR_Set = NULL;
	m_pfnCvar_FindVar = NULL;
	m_pfnglBegin = NULL;
	m_pfnglColor4f = NULL;
	m_pfnSCR_UpdateScreen = NULL;
	m_pfnV_RenderView = NULL;
	m_pfnR_RenderScene = NULL;
	m_pfnR_SetupFrame = NULL;
	m_pfnR_ForceCVars = NULL;
	m_pfnCRC_MapFile = NULL;

	m_hNetchan_CanPacket = 0;
	m_hglBegin = 0;
	m_hglColor4f = 0;
	m_hV_RenderView = 0;
	m_hR_RenderScene = 0;
	m_hR_SetupFrame = 0;
	m_hR_ForceCVars = 0;
}

bool CHooksModule::Load()
{
	//m_pfnR_RenderScene = MemoryUtils()->FindPattern( SvenModAPI()->Modules()->Hardware, Patterns::Hardware::R_RenderScene_HOOKED );

	//if ( !m_pfnR_RenderScene )
	//{
	//	Warning("Couldn't find function \"R_RenderScene\"\n");
	//	return false;
	//}

	ud_t inst;
	bool ScanOK = true;

	default_fov = CVar()->FindCvar("default_fov");
	hud_draw = CVar()->FindCvar("hud_draw");

	m_pfnglBegin = Sys_GetProcAddress(Sys_GetModuleHandle("opengl32.dll"), "glBegin");
	m_pfnglColor4f = Sys_GetProcAddress(Sys_GetModuleHandle("opengl32.dll"), "glColor4f");

	if ( !default_fov )
	{
		Warning("Can't find cvar default_fov\n");
		return false;
	}

	if ( !m_pfnglBegin )
	{
		Warning("Couldn't find function \"glBegin\"\n");
		return false;
	}

	if ( !m_pfnglColor4f )
	{
		Warning("Couldn't find function \"glColor4f\"\n");
		return false;
	}

	MemoryUtils()->InitDisasm( &inst, g_pEngineFuncs->SPR_Set, 32, 17 );

	if ( MemoryUtils()->Disassemble( &inst ) )
	{
		if ( inst.mnemonic == UD_Ijmp )
		{
			m_pfnSPR_Set = MemoryUtils()->CalcAbsoluteAddress( g_pEngineFuncs->SPR_Set );
		}
	}

	if ( m_pfnSPR_Set == NULL )
	{
		Warning( "Couldn't get function \"SPR_Set\"\n" );
		return false;
	}

	auto fpfnIN_Move = MemoryUtils()->FindPatternAsync( SvenModAPI()->Modules()->Client, Patterns::Client::IN_Move );
	auto fpfnCHud__Think = MemoryUtils()->FindPatternAsync( SvenModAPI()->Modules()->Client, Patterns::Client::CHud__Think );
	auto fpfnCHudBaseTextBlock__Print = MemoryUtils()->FindPatternAsync( SvenModAPI()->Modules()->Client, Patterns::Client::CHudBaseTextBlock__Print );
	auto fpfnNetchan_CanPacket = MemoryUtils()->FindPatternAsync( SvenModAPI()->Modules()->Hardware, Patterns::Hardware::Netchan_CanPacket );
	auto fpfnSCR_UpdateScreen = MemoryUtils()->FindPatternAsync( SvenModAPI()->Modules()->Hardware, Patterns::Hardware::SCR_UpdateScreen );
	auto fpfnV_RenderView = MemoryUtils()->FindPatternAsync( SvenModAPI()->Modules()->Hardware, Patterns::Hardware::V_RenderView );
	auto fpfnR_SetupFrame = MemoryUtils()->FindPatternAsync( SvenModAPI()->Modules()->Hardware, Patterns::Hardware::R_SetupFrame );
	auto fpfnR_ForceCVars = MemoryUtils()->FindPatternAsync( SvenModAPI()->Modules()->Hardware, Patterns::Hardware::R_ForceCVars );
	auto fpfnCRC_MapFile = MemoryUtils()->FindPatternAsync( SvenModAPI()->Modules()->Hardware, Patterns::Hardware::CRC_MapFile );
	auto fpfnScaleColors = MemoryUtils()->FindPatternAsync( SvenModAPI()->Modules()->Client, Patterns::Client::ScaleColors );
	auto fpfnScaleColors_RGBA = MemoryUtils()->FindPatternAsync( SvenModAPI()->Modules()->Client, Patterns::Client::ScaleColors_RGBA );
	auto fpclc_buffer = MemoryUtils()->FindPatternAsync( SvenModAPI()->Modules()->Hardware, Patterns::Hardware::clc_buffer );
	auto fcheats_level = MemoryUtils()->FindPatternAsync( SvenModAPI()->Modules()->Hardware, Patterns::Hardware::cheats_level );

	if ( !( m_pfnIN_Move = fpfnIN_Move.get() ) )
	{
		Warning( "Couldn't find function \"IN_Move\"\n" );
		ScanOK = false;
	}

	if ( !( m_pfnCHud__Think = fpfnCHud__Think.get() ) )
	{
		Warning( "Couldn't find function \"CHud::Think\"\n" );
		ScanOK = false;
	}

	if ( !( g_pfnCHudBaseTextBlock__Print = m_pfnCHudBaseTextBlock__Print = fpfnCHudBaseTextBlock__Print.get() ) )
	{
		Warning( "Couldn't find function \"CHudBaseTextBlock::Print\"\n" );
		ScanOK = false;
	}

	if ( !( m_pfnNetchan_CanPacket = fpfnNetchan_CanPacket.get() ) )
	{
		Warning( "Couldn't find function \"Netchan_CanPacket\"\n" );
		ScanOK = false;
	}

	if ( !( m_pfnSCR_UpdateScreen = fpfnSCR_UpdateScreen.get() ) )
	{
		Warning( "Couldn't find function \"SCR_UpdateScreen\"\n" );
		ScanOK = false;
	}

	if ( !( m_pfnV_RenderView = fpfnV_RenderView.get() ) )
	{
		Warning( "Couldn't find function \"V_RenderView\"\n" );
		ScanOK = false;
	}

	if ( !( m_pfnR_SetupFrame = fpfnR_SetupFrame.get() ) )
	{
		Warning( "Couldn't find function \"R_SetupFrame\"\n" );
		ScanOK = false;
	}

	if ( !( m_pfnR_ForceCVars = fpfnR_ForceCVars.get() ) )
	{
		Warning( "Couldn't find function \"R_ForceCVars\"\n" );
		ScanOK = false;
	}

	if ( !( m_pfnCRC_MapFile = fpfnCRC_MapFile.get() ) )
	{
		Warning( "Couldn't find function \"CRC_MapFile\"\n" );
		ScanOK = false;
	}

	if ( !( m_pfnScaleColors = fpfnScaleColors.get() ) )
	{
		Warning( "Couldn't find function \"ScaleColors\"\n" );
		ScanOK = false;
	}

	if ( !( m_pfnScaleColors_RGBA = fpfnScaleColors_RGBA.get() ) )
	{
		Warning( "Couldn't find function \"ScaleColors_RGBA\"\n" );
		ScanOK = false;
	}

	void *pclc_buffer;
	if ( ( pclc_buffer = fpclc_buffer.get() ) == NULL )
	{
		Warning( "Failed to locate \"clc_buffer\"\n" );
		ScanOK = false;
	}
	
	void *pcheats_level;
	if ( ( pcheats_level = fcheats_level.get() ) == NULL )
	{
		Warning( "Failed to locate \"cheats_level\"\n" );
		ScanOK = false;
	}

	if ( !ScanOK )
		return false;

	clc_buffer = *reinterpret_cast<sizebuf_t **>((unsigned char *)pclc_buffer + 1);
	cheats_level = *reinterpret_cast<int **>((unsigned char *)pcheats_level + 2);
	
	//MemoryUtils()->InitDisasm( &inst, g_pEngineFuncs->GetCvarPointer, 32, 17 );

	//if ( MemoryUtils()->Disassemble(&inst) )
	//{
	//	if ( inst.mnemonic == UD_Ijmp )
	//	{
	//		m_pfnCvar_FindVar = MemoryUtils()->CalcAbsoluteAddress( g_pEngineFuncs->GetCvarPointer );
	//	}
	//}

	//if ( m_pfnCvar_FindVar == NULL )
	//{
	//	Warning("Couldn't get function \"Cvar_FindVar\"\n");
	//	return false;
	//}

	unsigned char *pfnHookEvent = (unsigned char *)g_pEngineFuncs->HookEvent;

	MemoryUtils()->InitDisasm(&inst, pfnHookEvent, 32, 16);

	if ( MemoryUtils()->Disassemble(&inst) )
	{
		if ( inst.mnemonic == UD_Ijmp )
		{
			DEFINE_PATTERN(g_pEventHooks_sig, "56 8B 35 ? ? ? ? 85 F6 74 ? 8B 46 04");

			void *pHookEvent = MemoryUtils()->CalcAbsoluteAddress( pfnHookEvent );
			pHookEvent = MemoryUtils()->FindPatternWithin( SvenModAPI()->Modules()->Hardware, g_pEventHooks_sig, pHookEvent, (unsigned char *)pHookEvent + 128 );

			if ( pHookEvent != NULL )
			{
				MemoryUtils()->InitDisasm(&inst, pHookEvent, 32, 32);

				do
				{
					if ( inst.mnemonic == UD_Imov && inst.operand[0].type == UD_OP_REG && inst.operand[0].base == UD_R_ESI && inst.operand[1].type == UD_OP_MEM )
					{
						g_pEventHooks = reinterpret_cast<event_t *>(inst.operand[1].lval.udword);
						break;
					}

				} while ( MemoryUtils()->Disassemble(&inst) );

				if ( g_pEventHooks == NULL )
				{
					Warning("Failed to get \"g_pEventHooks\" #2\n");
					return false;
				}
			}
			else
			{
				Warning("Failed to get \"g_pEventHooks\"\n");
				return false;
			}
		}
		else
		{
			Warning("Couldn't locate JMP op-code on function \"HookEvent\" #2\n");
			return false;
		}
	}
	else
	{
		Warning("Couldn't locate JMP op-code on function \"HookEvent\"\n");
		return false;
	}

	return true;
}

void CHooksModule::PostLoad()
{
	ORIG_StudioDrawPlayer = g_pStudioAPI->StudioDrawPlayer;
	g_pStudioAPI->StudioDrawPlayer = HOOKED_StudioDrawPlayer;

	m_hIN_Move = DetoursAPI()->DetourFunction( m_pfnIN_Move, HOOKED_IN_Move, GET_FUNC_PTR(ORIG_IN_Move) );
	m_hCHud__Think = DetoursAPI()->DetourFunction( m_pfnCHud__Think, HOOKED_CHud__Think, GET_FUNC_PTR(ORIG_CHud__Think) );
	m_hCHudBaseTextBlock__Print = DetoursAPI()->DetourFunction( m_pfnCHudBaseTextBlock__Print, HOOKED_CHudBaseTextBlock__Print, GET_FUNC_PTR(ORIG_CHudBaseTextBlock__Print) );
	m_hNetchan_CanPacket = DetoursAPI()->DetourFunction( m_pfnNetchan_CanPacket, HOOKED_Netchan_CanPacket, GET_FUNC_PTR(ORIG_Netchan_CanPacket) );
	m_hScaleColors = DetoursAPI()->DetourFunction( m_pfnScaleColors, HOOKED_ScaleColors, GET_FUNC_PTR(ORIG_ScaleColors) );
	m_hScaleColors_RGBA = DetoursAPI()->DetourFunction( m_pfnScaleColors_RGBA, HOOKED_ScaleColors_RGBA, GET_FUNC_PTR(ORIG_ScaleColors_RGBA) );
	m_hSPR_Set = DetoursAPI()->DetourFunction( m_pfnSPR_Set, HOOKED_SPR_Set, GET_FUNC_PTR(ORIG_SPR_Set) );
	//m_hCvar_FindVar = DetoursAPI()->DetourFunction( m_pfnCvar_FindVar, HOOKED_Cvar_FindVar, GET_FUNC_PTR(ORIG_Cvar_FindVar) );
	m_hglBegin = DetoursAPI()->DetourFunction( m_pfnglBegin, HOOKED_glBegin, GET_FUNC_PTR(ORIG_glBegin) );
	m_hglColor4f = DetoursAPI()->DetourFunction( m_pfnglColor4f, HOOKED_glColor4f, GET_FUNC_PTR(ORIG_glColor4f) );
	m_hSCR_UpdateScreen = DetoursAPI()->DetourFunction( m_pfnSCR_UpdateScreen, HOOKED_SCR_UpdateScreen, GET_FUNC_PTR(ORIG_SCR_UpdateScreen) );
	m_hV_RenderView = DetoursAPI()->DetourFunction( m_pfnV_RenderView, HOOKED_V_RenderView, GET_FUNC_PTR(ORIG_V_RenderView) );
	//m_hR_RenderScene = DetoursAPI()->DetourFunction( m_pfnR_RenderScene, HOOKED_R_RenderScene, GET_FUNC_PTR(ORIG_R_RenderScene) );
	m_hR_SetupFrame = DetoursAPI()->DetourFunction( m_pfnR_SetupFrame, HOOKED_R_SetupFrame, GET_FUNC_PTR(ORIG_R_SetupFrame) );
	m_hR_ForceCVars = DetoursAPI()->DetourFunction( m_pfnR_ForceCVars, HOOKED_R_ForceCVars, GET_FUNC_PTR(ORIG_R_ForceCVars ) );
	m_hCRC_MapFile = DetoursAPI()->DetourFunction( m_pfnCRC_MapFile, HOOKED_CRC_MapFile, GET_FUNC_PTR(ORIG_CRC_MapFile) );

	m_hStudioRenderModel = DetoursAPI()->DetourVirtualFunction( g_pStudioRenderer, 20, HOOKED_StudioRenderModel, GET_FUNC_PTR(ORIG_StudioRenderModel) );

	m_hNetMsgHook_ServerInfo = Hooks()->HookNetworkMessage( SVC_SERVERINFO, HOOKED_NetMsgHook_ServerInfo, &ORIG_NetMsgHook_ServerInfo );
	m_hNetMsgHook_ResourceLocation = Hooks()->HookNetworkMessage( SVC_RESOURCELOCATION, HOOKED_NetMsgHook_ResourceLocation, &ORIG_NetMsgHook_ResourceLocation );
	m_hNetMsgHook_SendCvarValue = Hooks()->HookNetworkMessage( SVC_SENDCVARVALUE, HOOKED_NetMsgHook_SendCvarValue, &ORIG_NetMsgHook_SendCvarValue );
	m_hNetMsgHook_SendCvarValue2 = Hooks()->HookNetworkMessage( SVC_SENDCVARVALUE2, HOOKED_NetMsgHook_SendCvarValue2, &ORIG_NetMsgHook_SendCvarValue2 );
	m_hNetMsgHook_TempEntity = Hooks()->HookNetworkMessage( SVC_TEMPENTITY, HOOKED_NetMsgHook_TempEntity, &ORIG_NetMsgHook_TempEntity );

	m_hSnapshotCmd = Hooks()->HookConsoleCommand( "snapshot", HOOKED_snapshot, &ORIG_snapshot );
	m_hScreenshotCmd = Hooks()->HookConsoleCommand( "screenshot", HOOKED_screenshot, &ORIG_screenshot );

	Hooks()->RegisterClientHooks( &g_ClientHooks );
	Hooks()->RegisterClientPostHooks( &g_ClientPostHooks );
}

void CHooksModule::Unload()
{
	g_pStudioAPI->StudioDrawPlayer = ORIG_StudioDrawPlayer;

	DetoursAPI()->RemoveDetour( m_hIN_Move );
	DetoursAPI()->RemoveDetour( m_hCHud__Think );
	DetoursAPI()->RemoveDetour( m_hCHudBaseTextBlock__Print );
	DetoursAPI()->RemoveDetour( m_hNetchan_CanPacket );
	DetoursAPI()->RemoveDetour( m_hScaleColors );
	DetoursAPI()->RemoveDetour( m_hScaleColors_RGBA );
	DetoursAPI()->RemoveDetour( m_hSPR_Set );
	//DetoursAPI()->RemoveDetour( m_hCvar_FindVar );
	DetoursAPI()->RemoveDetour( m_hglBegin );
	DetoursAPI()->RemoveDetour( m_hglColor4f );
	DetoursAPI()->RemoveDetour( m_hSCR_UpdateScreen );
	DetoursAPI()->RemoveDetour( m_hV_RenderView );
	//DetoursAPI()->RemoveDetour( m_hR_RenderScene );
	DetoursAPI()->RemoveDetour( m_hR_SetupFrame );
	DetoursAPI()->RemoveDetour( m_hR_ForceCVars );
	DetoursAPI()->RemoveDetour( m_hCRC_MapFile );

	DetoursAPI()->RemoveDetour( m_hStudioRenderModel );

	Hooks()->UnhookNetworkMessage( m_hNetMsgHook_ServerInfo );
	Hooks()->UnhookNetworkMessage( m_hNetMsgHook_ResourceLocation );
	Hooks()->UnhookNetworkMessage( m_hNetMsgHook_SendCvarValue );
	Hooks()->UnhookNetworkMessage( m_hNetMsgHook_SendCvarValue2 );
	Hooks()->UnhookNetworkMessage( m_hNetMsgHook_TempEntity );

	Hooks()->UnhookNetworkMessage( m_hSnapshotCmd );
	Hooks()->UnhookNetworkMessage( m_hScreenshotCmd );

	Hooks()->UnregisterClientPostHooks( &g_ClientPostHooks );
	Hooks()->UnregisterClientHooks( &g_ClientHooks );
}

//-----------------------------------------------------------------------------
// Create singleton
//-----------------------------------------------------------------------------

CHooksModule g_HooksModule;