// Edge Pixels Player

#ifndef EDGEPIXELSPLAYER_H
#define EDGEPIXELSPLAYER_H

#ifdef _WIN32
#pragma once
#endif

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
	void CreateMove( float frametime, usercmd_t *cmd, int active );

private:
	void *m_pfnCL_TempEntInit;
	DetourHandle_t m_hCL_TempEntInit;
};

extern CEdgePixelsPlayer g_EdgePixelsPlayer;

#endif // EDGEPIXELSPLAYER_H