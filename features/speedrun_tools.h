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
// Structs
//-----------------------------------------------------------------------------

struct playerhull_display_info_t
{
	int		dead;

	Vector	origin;

	Vector	mins;
	Vector	maxs;

	float	time;
};

//-----------------------------------------------------------------------------
// CSpeedrunTools
//-----------------------------------------------------------------------------

class CSpeedrunTools : public CBaseFeature
{
public:
	CSpeedrunTools();

	virtual bool		Load( void );
	virtual void		PostLoad( void );

	virtual void		Unload( void );

public:
	void				GameFrame( void );
	void				CreateMove( float frametime, struct usercmd_s *cmd, int active );
	void				OnVideoInit( void );
	void				V_CalcRefDef( void );
	void				Draw( void );
	void				OnHUDRedraw( float flTime );

	void				SetLegitMode( bool state );
	bool				IsLegitMode( void ) const;

	float				SegmentCurrentTime( void );

	void				ShowTimer( float flTime, bool bServer );
	void				StartTimer( void );
	void				StopTimer( void );

	void				CheckPlayerHulls_Server( void );
	void				DrawPlayersHullsNickname_Server( void );
	void				BroadcastPlayerHull_Server( int client, int dead, const Vector &vecOrigin, bool bDuck );
	void				DrawPlayerHull_Comm( int client, int dead, const Vector &vecOrigin, bool bDuck );

	void				DrawPlayerHulls( void );
	void				DrawReviveBoostInfo( void );
	void				DrawReviveInfo( void );
	void				DrawReviveUnstuckArea( void );

	void				BroadcastTimescale( void );
	void				SendTimescale( edict_t *pPlayer );
	void				SetTimescale( float timescale );
	void				SetTimescale_Comm( bool notify, float framerate, float fpsmax, float min_frametime );

	void				ShowViewangles( int r, int g, int b );
	void				ShowPosition( int r, int g, int b );
	void				ShowVelocity( int r, int g, int b );
	void				ShowGaussBoostInfo( int r, int g, int b );
	void				ShowSelfgaussInfo( int r, int g, int b );
	void				ShowEntityInfo( int r, int g, int b );
	void				ShowReviveInfo( int r, int g, int b );
	void				ShowReviveBoostInfo( int r, int g, int b );

	void				OnBeginLoading( void );
	void				OnFirstClientdataReceived( client_data_t *pcldata, float flTime );

private:
	std::vector<playerhull_display_info_t>	m_vPlayersHulls;

	bool									m_bLegitMode;
	bool									m_bSegmentStarted;

	float									m_flSegmentStart;
	float									m_flSegmentTime;

	float									m_flTimerTime;
	float									m_flLastTimerUpdate;

	float									m_flDisplayHullsNextSend;

	bool									m_bShowReviveInfo;
	cl_entity_t								*m_pReviveTarget;
	float									m_flReviveDistance;

	bool									m_bShowReviveBoostInfo;
	cl_entity_t								*m_pReviveBoostTarget;
	float									m_flReviveBoostDistance;
	float									m_flReviveBoostAngle;

	unsigned short							*m_pJumpOpCode;
	unsigned short							m_PatchedJumpOpCode;

	void									*m_pfnUTIL_GetCircularGaussianSpread;
	void									*m_pfnHost_FilterTime;
	void									*m_pfnCbuf_AddText;
	void									*m_pfnServerCmd;

	DetourHandle_t							m_hUTIL_GetCircularGaussianSpread;
	DetourHandle_t							m_hHost_FilterTime;
	DetourHandle_t							m_hCbuf_AddText;
	DetourHandle_t							m_hServerCmd;

	vgui::HFont								m_engineFont;
};

extern CSpeedrunTools g_SpeedrunTools;

#endif // SPEEDRUN_TOOLS_H