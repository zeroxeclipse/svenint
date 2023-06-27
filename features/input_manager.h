#ifndef INPUT_MANAGER_H
#define INPUT_MANAGER_H

#ifdef _WIN32
#pragma once
#endif

#include <vector>
#include <string>

#include <IDetoursAPI.h>
#include <base_feature.h>
#include <data_struct/hash.h>
#include <generichash.h>

#include <hl_sdk/common/usercmd.h>

//-----------------------------------------------------------------------------
// Macro definitions
//-----------------------------------------------------------------------------

#define IM_FILE_HEADER		( 0x4D49 ) // IM
#define IM_FILE_VERSION		( 3 )
#define IM_FILE_PATH		( "sven_internal/input_manager/" )
#define IM_FRAME_INFO_SIZE	( sizeof im_frame_info_t )
#define IM_FRAME_INPUTS_SIZE ( sizeof im_frame_inputs_t )

#define IM_PRINT(str, ...) Msg( str, __VA_ARGS__ ); Utils()->PrintChatText( str, __VA_ARGS__ )
#define IM_MSG(str, ...) Msg( str, __VA_ARGS__ )
#define IM_WARNING(str, ...) Warning( str, __VA_ARGS__ )

//-----------------------------------------------------------------------------
// Type declarations
//-----------------------------------------------------------------------------

typedef const char *im_cstring_t;

//-----------------------------------------------------------------------------
// States
//-----------------------------------------------------------------------------

typedef enum
{
	IM_NONE = 0,
	IM_RECORDING,
	IM_PLAYINGBACK
} im_state_t;

//-----------------------------------------------------------------------------
// Structures of a single input frame
//-----------------------------------------------------------------------------

struct im_frame_info_t
{
	float			origin[ 3 ];
	float			velocity[ 3 ];
};

struct im_frame_inputs_t
{
	float			realviewangles[ 3 ];
	float			viewangles[ 3 ];
	float			forwardmove;
	float			sidemove;
	float			upmove;
	unsigned short	buttons;
	unsigned char	impulse;
	unsigned char	weaponselect;
};

struct im_frame_t
{
	im_frame_info_t			info;
	im_frame_inputs_t		inputs;

	const char				*commands;
};

//-----------------------------------------------------------------------------
// Inputs context
//-----------------------------------------------------------------------------

class CInputContext
{
public:
	CInputContext();

	void					Init( const char *pszFilename );
	void					Clear( void );

	bool					LoadFromFile( void );
	bool					SaveToFile( void );

	void					Split( void );

	void					GotoFrame( int iFrame );
	void					ForwardFrames( int iFrames );
	void					BackwardFrames( int iFrames );

	void					IncrementFramesCounter( void );

	im_frame_t				&CurrentFrame( void );
	im_frame_t				&FrameBuffer( void );
	std::vector<im_frame_t> &Frames( void );
	int						FrameCounter( void ) const;

	int						Version( void ) const;
	const char				*FileName( void ) const;

public:
	// Callbacks
	void					RecordInput( float frametime, usercmd_t *cmd, int active );
	void					PlaybackInput( float frametime, usercmd_t *cmd, int active );

private:
	bool					ParseFile( FILE *hStream, int iVersion, int &iErrorCode );

private:
	int						m_iVersion;

	std::vector<im_frame_t> m_frames;
	im_frame_t				m_FrameBuffer;
	int						m_iCurrentFrame;

	std::string				m_sFileName;
	std::string				m_sFilePath;

	bool					m_bSavedInfos;
};

//-----------------------------------------------------------------------------
// CInputManager
//-----------------------------------------------------------------------------

class CInputManager : public CBaseFeature
{
public:
	CInputManager();

	// CBaseFeature
	virtual bool	Load( void ) override;
	virtual void	PostLoad( void ) override;
	virtual void	Unload( void ) override;

public:
	// Main interface
	bool			Record( const char *pszFilename );
	bool			Playback( const char *pszFilename );
	bool			Split( void );
	bool			Stop( bool bAutoStop = false );

	void			Goto( int iFrame );
	void			Forward( int iFrames );
	void			Backward( int iFrames );

	void			RecordCommand( const char *pszCommand );

	bool			IsInAction( void ) const;
	bool			IsRecording( void ) const;
	bool			IsPlayingback( void ) const;

	int				GetCurrentFrame( void ) const;

public:
	// Callbacks
	void			CreateMove( float frametime, usercmd_t *cmd, int active );
	void			GameFrame( bool bPostRunCmd );
	void			OnBeginLoading( void );
	void			OnFirstClientdataReceived( void );
	void			OnVideoInit( void );

	void			SavedInputsSignal( bool bSaved );

	// Callbacks for hooked functions
	void			OnCbuf_AddText( const char *pszCommand );
	void			OnServerCmd( const char *pszCommand );

private:
	int					m_state;
	bool				m_bSavedInputs;
	bool				m_bForceViewAngles;

	CInputContext		m_InputContext;

	std::string			m_sQueuedCommands;
	CHash<im_cstring_t> m_WhitelistedCommands;

	DetourHandle_t		m_hCbuf_AddText;
	DetourHandle_t		m_hServerCmd;
	void				*m_pfnCbuf_AddText;
	void				*m_pfnServerCmd;
};

extern CInputManager g_InputManager;

#endif // INPUT_MANAGER_H