// Edge Pixels Player

#ifndef EDGEPIXELSPLAYER_H
#define EDGEPIXELSPLAYER_H

#ifdef _WIN32
#pragma once
#endif

#include <string>
#include <vector>
#include <stdio.h>
#include <hl_sdk/engine/APIProxy.h>

#include <base_feature.h>
#include <IDetoursAPI.h>
#include <IMemoryUtils.h>

//-----------------------------------------------------------------------------
// CEdgePixelsPlayer
//-----------------------------------------------------------------------------

class CEdgePixelsPlayer : public CBaseFeature
{
public:
	CEdgePixelsPlayer();

	virtual bool Load( void ) override;
	virtual void PostLoad( void ) override;
	virtual void Unload( void ) override;

public:
	bool Start( const char *pszFilename, double width, double height, const Vector &vecPos, const Vector &vecAngles, int iDrawCalls, bool bDemoPlayback = false );
	bool Stop( void );

	void CreateMove( float frametime, usercmd_t *cmd, int active );

private:
	bool m_bPlaying;
	bool m_bDemoPlayback;
	FILE *m_pFile;
	unsigned long m_ulFileSize;
	std::string m_sFilename;
	double m_width;
	double m_height;
	double m_frametime;
	double m_lastPlayed;
	Vector m_vecPos;
	Vector m_vecAngles;
	int m_iDrawCalls;
	std::vector<Vector> m_drawAngles;
	std::vector<std::vector<Vector>> m_drawAnglesQueue;
	cmd_t *npc_moveto;

	void *m_pfnCL_TempEntInit;
	DetourHandle_t m_hCL_TempEntInit;
};

extern CEdgePixelsPlayer g_EdgePixelsPlayer;

#endif // EDGEPIXELSPLAYER_H