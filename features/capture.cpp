// Video Capture

#include <dbg.h>
#include <convar.h>
#include <hl_sdk/engine/APIProxy.h>

#include "capture.h"

//-----------------------------------------------------------------------------
// Macro definitions
//-----------------------------------------------------------------------------

#define CAPTURE_SOUND_DEBUG ( 0 )

//-----------------------------------------------------------------------------
// Vars
//-----------------------------------------------------------------------------

CCapture g_Capture;

//-----------------------------------------------------------------------------
// Translate call of FMOD soundsystem back to GoldSrc
//-----------------------------------------------------------------------------

bool CCapture::CClient_SoundEngine__PlayFMODSound( void *thisptr, int fFlags, int entindex, float *vecOrigin, int iChannel, const char *pszSample, float flVolume, float flAttenuation, int iUnknown, int iPitch, int iSoundIndex, float flOffset )
{
#if CAPTURE_SOUND_DEBUG
	bool bOK = false;

	if ( fFlags == FMOD_SND_NONE )
	{
		Assert( pszSample != NULL );

		g_pEngineFuncs->PlaySoundByName( pszSample, flVolume );
		//g_pEventAPI->EV_PlaySoundSample( entindex, vecOrigin, iChannel, pszSample, flVolume, flAttenuation, 0, iPitch );

		bOK = true;
	}

	std::string sFlags;

	if ( fFlags & FMOD_SND_VOLUME )
		sFlags += "| FMOD_SND_VOLUME ";

	if ( fFlags & FMOD_SND_PITCH )
		sFlags += "| FMOD_SND_PITCH ";

	if ( fFlags & FMOD_SND_ATTENUATION )
		sFlags += "| FMOD_SND_ATTENUATION ";

	if ( fFlags & FMOD_SND_ORIGIN )
		sFlags += "| FMOD_SND_ORIGIN ";

	if ( fFlags & FMOD_SND_ENT )
		sFlags += "| FMOD_SND_ENT ";

	if ( fFlags & FMOD_SND_STOP )
		sFlags += "| FMOD_SND_STOP ";

	if ( fFlags & FMOD_SND_CHANGE_VOL )
		sFlags += "| FMOD_SND_CHANGE_VOL ";

	if ( fFlags & FMOD_SND_CHANGE_PITCH )
		sFlags += "| FMOD_SND_CHANGE_PITCH ";

	if ( fFlags & FMOD_SND_SENTENCE )
		sFlags += "| FMOD_SND_SENTENCE ";

	if ( fFlags & FMOD_SND_REFRESH )
		sFlags += "| FMOD_SND_REFRESH ";

	if ( fFlags & FMOD_SND_FORCE_SINGLE )
		sFlags += "| FMOD_SND_FORCE_SINGLE ";

	if ( fFlags & FMOD_SND_FORCE_LOOP )
		sFlags += "| FMOD_SND_FORCE_LOOP ";

	if ( fFlags & FMOD_SND_LINEAR )
		sFlags += "| FMOD_SND_LINEAR ";

	if ( fFlags & FMOD_SND_SKIP_ORIGIN_USE_ENT )
		sFlags += "| FMOD_SND_SKIP_ORIGIN_USE_ENT ";

	if ( fFlags & FMOD_SND_UNKNOWN )
		sFlags += "| FMOD_SND_UNKNOWN ";

	if ( fFlags & FMOD_SND_OFFSET )
		sFlags += "| FMOD_SND_OFFSET ";

	Vector origin;

	if ( vecOrigin != NULL )
		origin = *(Vector *)vecOrigin;

	Color clr = { 255, 90, 90, 255 };
	if ( bOK )
		clr = { 40, 255, 40, 255 };

	ConColorMsg( clr, "\"PlayFMODSound\"\n{" );
	ConColorMsg( clr, "\n    Flags:%s", ( sFlags.empty() ? " FMOD_SND_NONE" : sFlags.c_str() + 1 ) );
	ConColorMsg( clr, "\n    entindex: %d", entindex );
	ConColorMsg( clr, "\n    vecOrigin: %.3f %.3f %.3f", VectorExpand( origin ) );
	ConColorMsg( clr, "\n    iChannel: %d", iChannel );
	ConColorMsg( clr, "\n    pszSample: %s", pszSample );
	ConColorMsg( clr, "\n    flVolume: %.3f", flVolume );
	ConColorMsg( clr, "\n    flAttenuation: %.3f", flAttenuation );
	ConColorMsg( clr, "\n    iUnknown: %d", iUnknown );
	ConColorMsg( clr, "\n    iPitch: %d", iPitch );
	ConColorMsg( clr, "\n    iSoundIndex: %d", iSoundIndex );
	ConColorMsg( clr, "\n    flOffset: %.3f\n}\n", flOffset );

	if ( bOK )
		return true;

	return false;
#else

	if ( !IsRecording() )
		return false;

	return false;
#endif
}

//-----------------------------------------------------------------------------
// Video Capture implementation
//-----------------------------------------------------------------------------

CCapture::CCapture()
{
	m_bRecording = false;
}

bool CCapture::IsRecording( void ) const
{
	return m_bRecording;
}

//-----------------------------------------------------------------------------
// Video Capture feature implementation
//-----------------------------------------------------------------------------

bool CCapture::Load( void )
{
	return true;
}

void CCapture::PostLoad( void )
{
	
}

void CCapture::Unload( void )
{
	
}