#ifndef SPEEDRUN_TOOLS_H
#define SPEEDRUN_TOOLS_H

#ifdef _WIN32
#pragma once
#endif

#include <base_feature.h>
#include <IDetoursAPI.h>

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
	void Think();
	void CreateMove(float frametime, struct usercmd_s *cmd, int active);
	void OnVideoInit();
	void V_CalcRefDef();
	void OnHUDRedraw(float flTime);

	float SegmentCurrentTime();

	void ShowTimer(float flTime, bool bServer);

	void StartTimer();
	void StopTimer();

	void BroadcastTimescale();
	void SendTimescale(edict_t *pPlayer);

	void SetTimescale(float timescale);
	void SetTimescale_Comm(bool notify, float framerate, float fpsmax, float min_frametime);
	
	void OnBeginLoading();
	void OnFirstClientdataReceived(client_data_t *pcldata, float flTime);

private:
	void FindCvars();

private:
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
};

extern CSpeedrunTools g_SpeedrunTools;

#endif // SPEEDRUN_TOOLS_H