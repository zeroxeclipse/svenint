#ifndef MOVEMENT_RECORDER_H
#define MOVEMENT_RECORDER_H

#ifdef _WIN32
#pragma once
#endif

#include <vector>
#include <math/vector.h>
#include <hl_sdk/common/usercmd.h>
#include <hl_sdk/engine/edict.h>

#include <base_feature.h>
#include <IDetoursAPI.h>

//-----------------------------------------------------------------------------
// Movement Info
//-----------------------------------------------------------------------------

typedef struct
{
	Vector origin;
	Vector viewangles;
	Vector velocity;

	int weapon; // @weaponName = stringBase + MovementFrame::weapon

	unsigned short buttons;
	unsigned char impulse;

	int msec;

} MovementFrame;

//-----------------------------------------------------------------------------
// Movement Recorder
//-----------------------------------------------------------------------------

class CMovementRecorder : public CBaseFeature
{
public:
	CMovementRecorder( void );

	virtual bool Load( void ) override;
	virtual void PostLoad( void ) override;

	virtual void Unload( void ) override;

public:
	void OnPostRunCmd( void );

	bool ShouldAbortRunCmd( void );

	void OnPreRunCmd( usercmd_t *cmd, int random_seed );
	void OnPostRunCmd( usercmd_t *cmd, int random_seed );

	void OnCmdStart( edict_t *pPlayer, usercmd_t *cmd, unsigned int random_seed );
	void OnCmdEnd( edict_t *pPlayer );

private:
	edict_t **m_pServerPlayer;

	void *m_pfnSV_RunCmd;

	DetourHandle_t m_hSV_RunCmd;
	DetourHandle_t m_hCmdStart;
	DetourHandle_t m_hCmdEnd;
};

extern CMovementRecorder g_MovementReader;

#endif // MOVEMENT_RECORDER_H