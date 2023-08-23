// Video Capture

#ifndef CAPTURE_H
#define CAPTURE_H

#ifdef _WIN32
#pragma once
#endif

#include <GL/GL.h>
#include <Windows.h>
#include <string>

#include <base_feature.h>
#include <IDetoursAPI.h>
#include <IMemoryUtils.h>
#include <ISvenModAPI.h>

/*
//-----------------------------------------------------------------------------
// Macro definitions
//-----------------------------------------------------------------------------

// Sound engine paintbuffer
#define	PAINTBUFFER_SIZE ( 512 )

// GoldSrc Sound Flags (Xash3D ref)
#define SND_VOLUME			( 1 )	// a scaled byte
#define SND_ATTENUATION		( 2 )	// a byte
#define SND_LARGE_INDEX		( 4 )	// a send sound as short
#define SND_PITCH			( 8 )	// a byte
#define SND_SENTENCE		( 16 )	// set if sound num is actually a sentence num
#define SND_STOP			( 32 )	// stop the sound
#define SND_CHANGE_VOL		( 64 )	// change sound vol
#define SND_CHANGE_PITCH	( 128 )	// change sound pitch
#define SND_SPAWNING		( 256 )	// we're spawning, used in some cases for ambients (not sent across network)

// FMOD Sound flags
#define FMOD_SND_NONE					( 0 )		// No flags set, play in 2D mode
#define FMOD_SND_VOLUME					( 1 )		// Volume flag
#define FMOD_SND_PITCH					( 2 )		// Pitch flag
#define FMOD_SND_ATTENUATION			( 4 )		// Attenuation flag
#define FMOD_SND_ORIGIN					( 8 )		// Origin flag
#define FMOD_SND_ENT					( 16 )		// Entity flag
#define FMOD_SND_STOP					( 32 )		// Stop flag
#define FMOD_SND_CHANGE_VOL				( 64 )		// Change volume flag
#define FMOD_SND_CHANGE_PITCH			( 128 )		// Change pitch flag
#define FMOD_SND_SENTENCE				( 256 )		// Sentence flag
#define FMOD_SND_REFRESH				( 512 )		// Refresh flag
#define FMOD_SND_FORCE_SINGLE			( 1024 )	// Force single flag
#define FMOD_SND_FORCE_LOOP				( 2048 )	// Force loop flag
#define FMOD_SND_LINEAR					( 4096 )	// Linear flag
#define FMOD_SND_SKIP_ORIGIN_USE_ENT	( 8192 )	// Skip Origin and use entity flag
#define FMOD_SND_UNKNOWN				( 16384 )	// @wootguy says!!! :OOO >> this is set by ambient_music but idk what it does
#define FMOD_SND_OFFSET					( 32768 )

//-----------------------------------------------------------------------------
// Structures
//-----------------------------------------------------------------------------

struct dma_t
{
	// Quake 1
	qboolean		gamealive; // 0x0
	qboolean		soundalive; // 0x4
	qboolean		splitbuffer; // 0x8
	int				channels; // 0xC
	int				samples; // 0x10				// mono samples in buffer
	int				submission_chunk; // 0x14		// don't mix less than this #
	int				samplepos; // 0x18				// in mono samples
	int				samplebits; // 0x1C
	int				speed; // 0x20
	int				width; // 0x24 NOT PRESENTED IN QUAKE 1, CAN'T CONFIRM. MOST LIKELY SPEED TOO
	unsigned char *buffer; // 0x28

	// int		initialized; // 0x0 OK
	// int		unknown; // 0x4 DUMMY?
	// int     sampleframes; // 0x8 NOT CONFIRMED
	// int     stereo; // 0xC OK
	// int		samples; // 0x10 OK
	// int		submission_chunk; // 0x14 OK
	// int		samplepos; // 0x18 OK
	// int		samplebits; // 0x1C OK

	// unsigned int speed; // 0x20 OK
	// unsigned int width; // 0x24 NOT CONFIRMED
	// // unsigned int channels; // 0x28 NOT CONFIRMED
	// unsigned char *buffer; // 0x28 OK
};
*/

//-----------------------------------------------------------------------------
// CCapture
//-----------------------------------------------------------------------------

class CCapture : public CBaseFeature
{
public:
	CCapture();

	virtual bool Load( void ) override;
	virtual void PostLoad( void ) override;
	virtual void Unload( void ) override;

public:
	bool CClient_SoundEngine__PlayFMODSound( void *thisptr, int fFlags, int entindex, float *vecOrigin, int iChannel, const char *pszSample, float flVolume, float flAttenuation, int iUnknown, int iPitch, int iSoundIndex, float flOffset );

	bool Start( const char *pszFilename, double fps, double slowdown, double sampling_fps );
	bool Stop( void );

	void SaveImage( void );

	bool OpenPipe();
	bool ClosePipe();

	void PostUpdateScreen( void );
	void GameFrame( client_state_t state );

	bool IsRecording( void ) const;

private:
	bool m_bRecording;
	bool m_bFirstCapture;
	int m_iCaptureFrameCount;
	int m_iFpsMultiplier;
	double m_lastRecordTime;
	double m_captureFps;
	double m_samplingFps;
	double m_fps;
	double m_frametime;

	std::string m_sFilename;

	int m_iWidth;
	int m_iHeight;
	int m_nPixelsBufferSize;
	char *m_pPixelsBuffer;

	HANDLE m_hReadPipe;
	HANDLE m_hWritePipe;
	PROCESS_INFORMATION m_pi;
};

extern CCapture g_Capture;

#endif // CAPTURE_H