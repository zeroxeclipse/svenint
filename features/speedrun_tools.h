#ifndef SPEEDRUN_TOOLS_H
#define SPEEDRUN_TOOLS_H

#ifdef _WIN32
#pragma once
#endif

#include <vector>

#include <base_feature.h>
#include <IDetoursAPI.h>
#include <IVGUI.h>

#include <hl_sdk/cl_dll/cl_dll.h>

//-----------------------------------------------------------------------------
// Defs
//-----------------------------------------------------------------------------

#define IM_FILE_HEADER 0x4D49 // "IM"
#define IM_FILE_VERSION 2
#define IM_FRAME_SIZE sizeof(input_frame_t)

//-----------------------------------------------------------------------------
// Structs
//-----------------------------------------------------------------------------

struct input_frame_t
{
	float			realviewangles[3];
	float			viewangles[3];
	float			forwardmove;
	float			sidemove;
	float			upmove;
	unsigned short	buttons;
	unsigned char	impulse;
	unsigned char	weaponselect;
};

struct deadplayer_display_info_t
{
	Vector origin;

	Vector mins;
	Vector maxs;

	float time;
};

//-----------------------------------------------------------------------------
// CSpeedrunTools
//-----------------------------------------------------------------------------

class CSpeedrunTools : public CBaseFeature
{
public:
	CSpeedrunTools();

	virtual bool Load();
	virtual void PostLoad();

	virtual void Unload();

public:
	void GameFrame();
	void CreateMove(float frametime, struct usercmd_s *cmd, int active);
	void OnVideoInit();
	void V_CalcRefDef();
	void Draw();
	void OnHUDRedraw(float flTime);

	float SegmentCurrentTime();

	void ShowTimer(float flTime, bool bServer);
	void StartTimer();
	void StopTimer();

	void CheckDeadPlayers();
	void DrawDeadPlayersNickname();
	void BroadcastDeadPlayer(int client, const Vector &vecOrigin, const Vector &vecMins, const Vector &vecMaxs);
	void DrawDeadPlayer_Comm(int client, const Vector &vecOrigin, const Vector &vecMins, const Vector &vecMaxs);

	void BroadcastTimescale();
	void SendTimescale(edict_t *pPlayer);
	void SetTimescale(float timescale);
	void SetTimescale_Comm(bool notify, float framerate, float fpsmax, float min_frametime);

	void ShowViewangles(int r, int g, int b);
	void ShowPosition(int r, int g, int b);
	void ShowVelocity(int r, int g, int b);
	void ShowGaussBoostInfo(int r, int g, int b);
	void ShowSelfgaussInfo(int r, int g, int b);
	void ShowEntityInfo(int r, int g, int b);
	
	void OnBeginLoading();
	void OnFirstClientdataReceived(client_data_t *pcldata, float flTime);

private:
	std::vector<deadplayer_display_info_t> m_vDeadPlayers;

	bool m_bSegmentStarted;

	float m_flSegmentStart;
	float m_flSegmentTime;

	float m_flTimerTime;
	float m_flLastTimerUpdate;

	unsigned short *m_pJumpOpCode;
	unsigned short m_PatchedJumpOpCode;

	void *m_pfnUTIL_GetCircularGaussianSpread;
	void *m_pfnHost_FilterTime;
	void *m_pfnCbuf_AddText;
	void *m_pfnServerCmd;

	DetourHandle_t m_hUTIL_GetCircularGaussianSpread;
	DetourHandle_t m_hHost_FilterTime;
	DetourHandle_t m_hCbuf_AddText;
	DetourHandle_t m_hServerCmd;

	vgui::HFont m_engineFont;
};

extern CSpeedrunTools g_SpeedrunTools;

#endif // SPEEDRUN_TOOLS_H