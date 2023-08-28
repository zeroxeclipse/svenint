// Video Capture

#include <dbg.h>
#include <convar.h>
#include <hl_sdk/engine/APIProxy.h>

#include "../game/utils.h"

#include "capture.h"

extern bool g_bPlayingbackDemo;

//-----------------------------------------------------------------------------
// Macro definitions
//-----------------------------------------------------------------------------

#define CAPTURE_SOUND_DEBUG ( 0 )

//-----------------------------------------------------------------------------
// Vars
//-----------------------------------------------------------------------------

CCapture g_Capture;

//-----------------------------------------------------------------------------
// ConVars / ConCommands
//-----------------------------------------------------------------------------

ConVar sc_cap_fps( "sc_cap_fps", "60", FCVAR_CLIENTDLL );
ConVar sc_cap_slowdown( "sc_cap_slowdown", "1", FCVAR_CLIENTDLL );
ConVar sc_cap_sampling_min_fps( "sc_cap_sampling_min_fps", "7200", FCVAR_CLIENTDLL );
ConVar sc_cap_sampling_round_fps( "sc_cap_sampling_round_fps", "1", FCVAR_CLIENTDLL );

CON_COMMAND( sc_cap_start, "" )
{
	if ( args.ArgC() == 1 )
	{
		Msg( "Usage:  sc_cap_start <filename>\n" );
		return;
	}

	g_Capture.Start( args[ 1 ], static_cast<double>( sc_cap_fps.GetFloat() ), static_cast<double>( sc_cap_slowdown.GetFloat() ), static_cast<double>( sc_cap_sampling_min_fps.GetFloat() ) );
}

CON_COMMAND( sc_cap_stop, "" )
{
	g_Capture.Stop();
}

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
	m_bFirstCapture = true;
	m_iCaptureFrameCount = 0;
	m_iFpsMultiplier = 120;
	m_lastRecordTime = 0.0;
	m_captureFps = 60.0;
	m_samplingFps = 7200.0;
	m_fps = 7200.0;
	m_frametime = 1.0 / 7200.0;

	m_iWidth = 1280;
	m_iHeight = 720;
	m_nPixelsBufferSize = 0;
	m_pPixelsBuffer = NULL;

	m_hReadPipe = INVALID_HANDLE_VALUE;
	m_hWritePipe = INVALID_HANDLE_VALUE;
	ZeroMemory( &m_pi, sizeof( PROCESS_INFORMATION ) );
}

//-----------------------------------------------------------------------------
// Start capturing
//-----------------------------------------------------------------------------

bool CCapture::Start( const char *pszFilename, double fps, double slowdown, double sampling_fps )
{
	if ( IsRecording() )
	{
		Warning( "[Capture] Already recording\n" );
		return false;
	}

	double fpsMultiplier;
	double sampleFrametime;

	m_captureFps = fps;
	m_samplingFps = sampling_fps;
	m_iFpsMultiplier = int( fpsMultiplier = ceil( sampling_fps / fps ) );
	m_fps = sc_cap_sampling_round_fps.GetBool() ? m_captureFps * fpsMultiplier : m_samplingFps;
	m_frametime = ( 1.0 / m_captureFps ) * ( 1.0 / slowdown ); // m_fps

	if ( m_fps < m_frametime )
		m_fps = m_frametime;

	m_iWidth = g_ScreenInfo.width;
	m_iHeight = g_ScreenInfo.height;
	m_nPixelsBufferSize = m_iWidth * m_iHeight * 3;
	m_pPixelsBuffer = (char *)malloc( m_nPixelsBufferSize );

	sampleFrametime = ( 1.0 / m_fps );

	if ( m_pPixelsBuffer == NULL )
	{
		Warning( "[Capture] Couldn't allocate memory for pixels buffer\n" );
		return false;
	}

	m_sFilename = pszFilename;

	if ( !OpenPipe() )
	{
		free( (void *)m_pPixelsBuffer );
		return false;
	}

	m_bRecording = true;
	m_bFirstCapture = true;
	m_iCaptureFrameCount = 0;
	m_lastRecordTime = 0.0;

	CVar()->SetValue( "fps_max", (float)m_captureFps );
	CVar()->SetValue( "host_framerate", (float)sampleFrametime ); // m_frametime

	Msg( "[Capture] Started recording to file \"%s.mp4\"\n", pszFilename );
	return true;
}

//-----------------------------------------------------------------------------
// Stop capturing
//-----------------------------------------------------------------------------

bool CCapture::Stop( void )
{
	if ( !IsRecording() )
	{
		Warning( "[Capture] Not recording\n" );
		return false;
	}

	ClosePipe();

	free( (void *)m_pPixelsBuffer );

	CVar()->SetValue( "fps_max", 200 );
	CVar()->SetValue( "host_framerate", 0 );

	Msg( "[Capture] Stopped recording to file \"%s\"\n", m_sFilename.c_str() );

	m_bRecording = false;
	return true;
}

//-----------------------------------------------------------------------------
// Save pixels and send them to FFmpeg
//-----------------------------------------------------------------------------

void CCapture::SaveImage( void )
{
	DWORD dwBytesWritten;

	glReadPixels( 0, 0, m_iWidth, m_iHeight, GL_RGB, GL_UNSIGNED_BYTE, m_pPixelsBuffer );
	WriteFile( m_hWritePipe, m_pPixelsBuffer, m_nPixelsBufferSize, &dwBytesWritten, NULL );
}

//-----------------------------------------------------------------------------
// Open FFmpeg pipe
//-----------------------------------------------------------------------------

bool CCapture::OpenPipe()
{
	static char ffmpeg_args[ 2048 ];

	SECURITY_ATTRIBUTES sa = { sizeof( sa ) };
	sa.lpSecurityDescriptor = NULL;
	sa.bInheritHandle = TRUE;

	if ( !CreatePipe( &m_hReadPipe, &m_hWritePipe, &sa, m_nPixelsBufferSize ) )
	{
		Warning( "[Capture] Failed to create FFmpeg pipe\n" );
		return false;
	}

	SetHandleInformation( m_hWritePipe, HANDLE_FLAG_INHERIT, 0 );

	STARTUPINFOA si = { sizeof( si ) };
	si.dwFlags = STARTF_USESTDHANDLES;
	si.hStdInput = m_hReadPipe;
	si.hStdOutput = GetStdHandle( STD_OUTPUT_HANDLE );
	si.hStdError = GetStdHandle( STD_ERROR_HANDLE );

	ZeroMemory( &m_pi, sizeof( PROCESS_INFORMATION ) );

	snprintf( ffmpeg_args, M_ARRAYSIZE( ffmpeg_args ),
"ffmpeg -f rawvideo \
-pix_fmt rgb24 \
-video_size %dx%d \
-r %d \
-i pipe:0 \
-preset ultrafast \
-y -movflags +faststart \
-b:v 128k \
-c:v libx264 \
-crf 15 \
-vf vflip \
-color_primaries bt709 \
-color_trc bt709 \
-colorspace bt709 \
-color_range tv \
-chroma_sample_location center \
%s.mp4", m_iWidth, m_iHeight, int( m_captureFps ), m_sFilename.c_str() );

	if ( !CreateProcessA( NULL, ffmpeg_args, NULL, NULL, TRUE, 0, NULL, NULL, &si, &m_pi ) )
	{
		CloseHandle( m_hReadPipe );
		CloseHandle( m_hWritePipe );

		Warning( "[Capture] Failed to spawn FFmpeg\n" );
		return false;
	}

	return true;
}

//-----------------------------------------------------------------------------
// Close FFmpeg pipe
//-----------------------------------------------------------------------------

bool CCapture::ClosePipe()
{
	CloseHandle( m_hReadPipe );
	CloseHandle( m_hWritePipe );

	CloseHandle( m_pi.hProcess );
	CloseHandle( m_pi.hThread );

	return true;
}

//-----------------------------------------------------------------------------
// SCR_UpdateScreen
//-----------------------------------------------------------------------------

void CCapture::PostUpdateScreen( void )
{
	//static int testcounter = 0;
	//static char testbuffer[ 32 ];

	if ( !IsRecording() || !g_bPlayingbackDemo )
		return;

	if ( m_bFirstCapture )
	{
		m_bFirstCapture = false;
		m_lastRecordTime = *dbRealtime;

		//testcounter = 0;
		//snprintf( testbuffer, M_ARRAYSIZE( testbuffer ), "%s_%05d.bmp", m_sFilename.c_str(), 1 );
		//VID_TakeSnapshot( testbuffer );

		SaveImage();
	}
	//else
	else if ( *dbRealtime - m_lastRecordTime >= m_frametime )
	{
		//m_iCaptureFrameCount++;
		m_lastRecordTime = *dbRealtime;

		//if ( m_iCaptureFrameCount != m_iFpsMultiplier )
		//	return;
		//
		//m_iCaptureFrameCount = 0;

		SaveImage();

		//testcounter++;
		//snprintf( testbuffer, M_ARRAYSIZE( testbuffer ), "%s_%05d.bmp", m_sFilename.c_str(), testcounter + 1 );
		//VID_TakeSnapshot( testbuffer );
	}

	//SaveImage();
}

//-----------------------------------------------------------------------------
// Game frame
//-----------------------------------------------------------------------------

void CCapture::GameFrame( client_state_t state)
{
	if ( !g_bPlayingbackDemo && IsRecording() && !m_bFirstCapture )
	{
		Stop();
	}
}

//-----------------------------------------------------------------------------
// Is recording
//-----------------------------------------------------------------------------

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