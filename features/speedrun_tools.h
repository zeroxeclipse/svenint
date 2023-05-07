#ifndef SPEEDRUN_TOOLS_H
#define SPEEDRUN_TOOLS_H

#ifdef _WIN32
#pragma once
#endif

#include <vector>

#include <base_feature.h>
#include <IDetoursAPI.h>
#include <IRender.h>
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

struct playerhull_display_info_t
{
	int dead;

	Vector origin;

	Vector mins;
	Vector maxs;

	float time;
};

//-----------------------------------------------------------------------------
// Draw box, no depth buffer
//-----------------------------------------------------------------------------

class CDrawBoxNoDepthBuffer : public IDrawContext
{
public:
	CDrawBoxNoDepthBuffer( const Vector &vOrigin, const Vector &vMins, const Vector &vMaxs, const Color &color );
	virtual ~CDrawBoxNoDepthBuffer( void ) {}

	virtual void Draw( void ) override;
	virtual bool ShouldStopDraw( void ) override { return false; };

	virtual const Vector &GetDrawOrigin( void ) const override { return m_vecDrawOrigin; };

private:
	Vector m_vecDrawOrigin;
	Vector m_vecOrigin;

	Vector m_vecMins;
	Vector m_vecMaxs;

	Color m_color;
};

//-----------------------------------------------------------------------------
// Draw wireframe box
//-----------------------------------------------------------------------------

class CWireframeBox : public IDrawContext
{
public:
	CWireframeBox( const Vector &vOrigin, const Vector &vMins, const Vector &vMaxs, const Color &color, float width, bool bIgnoreDepthBuffer );
	virtual ~CWireframeBox( void ) {}

	virtual void Draw( void ) override;
	virtual bool ShouldStopDraw( void ) override { return false; };

	virtual const Vector &GetDrawOrigin( void ) const override { return m_vecDrawOrigin; };

private:
	Vector m_vecDrawOrigin;
	Vector m_vecOrigin;

	Vector m_vecMins;
	Vector m_vecMaxs;

	Color m_color;

	float m_flWidth;
	bool m_bIgnoreDepthBuffer;
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

	void CheckPlayerHulls_Server();
	void DrawPlayersHullsNickname_Server();
	void BroadcastPlayerHull_Server(int client, int dead, const Vector &vecOrigin, bool bDuck);
	void DrawPlayerHull_Comm(int client, int dead, const Vector &vecOrigin, bool bDuck);
	void DrawBox(const Vector &vecOrigin, const Vector &vecMins, const Vector &vecMaxs, float r, float g, float b, float alpha, float width, bool wireframe);

	void DrawReviveInfo();

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
	void ShowReviveInfo(int r, int g, int b);
	
	void OnBeginLoading();
	void OnFirstClientdataReceived(client_data_t *pcldata, float flTime);

private:
	std::vector<playerhull_display_info_t> m_vPlayersHulls;

	bool m_bSegmentStarted;

	float m_flSegmentStart;
	float m_flSegmentTime;

	float m_flTimerTime;
	float m_flLastTimerUpdate;

	float m_flDisplayHullsNextSend;

	bool m_bShowReviveInfo;
	cl_entity_t *m_pReviveTarget;
	float m_flReviveDistance;

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

extern bool IM_IsPlayingBack();
extern bool IM_IsRecording();

#endif // SPEEDRUN_TOOLS_H