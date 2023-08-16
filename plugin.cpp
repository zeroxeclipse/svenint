#ifdef _CPPRTTI
#error "RTTI enabled"
#endif

#include <string>
#include <vector>
#include <algorithm>

#include <ISvenModAPI.h>
#include <IClientPlugin.h>
#include <IVideoMode.h>

#include <convar.h>
#include <dbg.h>

#include <base_feature.h>

#include "plugin.h"
#include "config.h"
#include "friends.h"

#include "scripts/scripts.h"
#include "game/utils.h"
#include "game/drawing.h"

#include "modules/opengl.h"
#include "modules/server.h"
#include "modules/patches.h"
#include "modules/server_client_bridge.h"

// Auto Update
#include "autoupdate/client/autoupdate.h"
#include "autoupdate/shared/au_utils.h"

// Features
#include "features/custom_vote_popup.h"
#include "features/speedrun_tools.h"
#include "features/antiafk.h"
#include "features/skybox.h"
#include "features/visual.h"
#include "features/radar.h"
#include "features/chat_colors.h"
#include "features/camhack.h"
#include "features/keyspam.h"
#include "features/bsp.h"
#include "features/input_manager.h"

#include "steam/steam_api.h"
#include "utils/security.hpp"
#include "utils/xorstr.h"

#pragma warning( disable : 4731)

//-----------------------------------------------------------------------------
// Import
//-----------------------------------------------------------------------------

static void SaveSoundcache();

extern bool g_bForceFreeze2;
extern bool g_bAutoUpdateInProcess;

//-----------------------------------------------------------------------------
// Vars
//-----------------------------------------------------------------------------

uint64 g_ullSteam64ID = 0uLL;
int g_hAutoUpdateThread = 0;

//-----------------------------------------------------------------------------
// SvenMod's plugin
//-----------------------------------------------------------------------------

class CSvenInternal : public IClientPlugin
{
public:
	virtual api_version_t GetAPIVersion();

	virtual bool Load(CreateInterfaceFn pfnSvenModFactory, ISvenModAPI *pSvenModAPI, IPluginHelpers *pPluginHelpers);

	virtual void PostLoad(bool bGlobalLoad);

	virtual void Unload(void);

	virtual bool Pause(void);

	virtual void Unpause(void);

	virtual void OnFirstClientdataReceived(client_data_t *pcldata, float flTime);

	virtual void OnBeginLoading(void);

	virtual void OnEndLoading(void);

	virtual void OnDisconnect(void);

	virtual void GameFrame(client_state_t state, double frametime, bool bPostRunCmd);

	virtual void Draw(void);

	virtual void DrawHUD(float time, int intermission);

	virtual const char *GetName(void);

	virtual const char *GetAuthor(void);

	virtual const char *GetVersion(void);

	virtual const char *GetDescription(void);

	virtual const char *GetURL(void);

	virtual const char *GetDate(void);

	virtual const char *GetLogTag(void);

private:
	void InitFolders(ISvenModAPI *pSvenModAPI);

private:
	float m_flPlatTime;

#if SECURITY_CHECKS
	float m_flAntiDebugTime;
#endif
};

CSvenInternal g_SvenInternal;
IClientPlugin *g_pClientPlugin = &g_SvenInternal;

//-----------------------------------------------------------------------------
// Implement plugin methods
//-----------------------------------------------------------------------------

//class CGameManager
//{
//private:
//	STEAM_CALLBACK(CGameManager, OnGameOverlayActivated, GameOverlayActivated_t);
//	STEAM_CALLBACK(CGameManager, OnScreenshotRequested, ScreenshotRequested_t);
//	STEAM_CALLBACK(CGameManager, OnScreenshotReady, ScreenshotReady_t);
//} g_GameManager;
//
//void CGameManager::OnGameOverlayActivated(GameOverlayActivated_t *pCallback)
//{
//	if ( pCallback->m_bActive )
//		Warning("Steam overlay now active\n");
//	else
//		Warning("Steam overlay now inactive\n");
//}
//
//void CGameManager::OnScreenshotRequested(ScreenshotRequested_t *pCallback)
//{
//	Warning("Steam screenshot requested\n");
//}
//
//void CGameManager::OnScreenshotReady(ScreenshotReady_t *pCallback)
//{
//	Warning("Steam screenshot ready\n");
//}

api_version_t CSvenInternal::GetAPIVersion()
{
	return SVENMOD_API_VER;
}

bool CSvenInternal::Load(CreateInterfaceFn pfnSvenModFactory, ISvenModAPI *pSvenModAPI, IPluginHelpers *pPluginHelpers)
{
	//SteamScreenshots()->HookScreenshots( true );

	if ( !GL_Init() )
	{
		Warning(xs("[Sven Internal] Failed to initialize OpenGL module\n"));
		return false;
	}

	// Print GL version
	const GLubyte *renderer = glGetString( GL_RENDERER );
	const GLubyte *vendor = glGetString( GL_VENDOR );
	const GLubyte *version = glGetString( GL_VERSION );
	const GLubyte *glslVersion = glGetString( GL_SHADING_LANGUAGE_VERSION );

	GLint major, minor;
	glGetIntegerv( GL_MAJOR_VERSION, &major );
	glGetIntegerv( GL_MINOR_VERSION, &minor );

	if ( CVar()->GetBoolFromCvar( xs( "developer" ) ) )
	{
		DevMsg( xs( "GL Vendor            : %s\n" ), vendor );
		DevMsg( xs( "GL Renderer          : %s\n" ), renderer );
		DevMsg( xs( "GL Version (string)  : %s\n" ), version );
		DevMsg( xs( "GL Version (integer) : %d.%d\n" ), major, minor );
		DevMsg( xs( "GLSL Version         : %s\n" ), glslVersion );
	}

#if SECURITY_CHECKS
	AntiDebug();
#endif

	g_ullSteam64ID = SteamUser()->GetSteamID().ConvertToUint64();

	for (size_t i = 0; i < g_Gods.size(); i++)
	{
		g_Gods[i] = XOR_STEAMID( g_Gods[i] );
	}

	std::sort( g_Gods.begin(), g_Gods.end() );

	if ( !std::binary_search( g_Gods.begin(), g_Gods.end(), g_ullSteam64ID ) )
	{
		//Warning(xs("[Sven Internal] You're not allowed to use this plugin\n"));
		security::utils::obfuscate_exit();
		return false;
	}

	BindApiToGlobals(pSvenModAPI);
	InitFolders(pSvenModAPI);

	if ( !g_ServerModule.Init() )
	{
		Warning(xs("[Sven Internal] Failed to initialize server's module\n"));
		return false;
	}

	if ( !g_KeySpam.Init() || !LoadFeatures() )
	{
		Warning(xs("[Sven Internal] Failed to initialize one of features\n"));
		return false;
	}

	g_ServerClientBridge.Init();

	ConVar_Register();

	vmode_t *mode = VideoMode()->GetCurrentMode();

	g_ScreenInfo.width = mode->width;
	g_ScreenInfo.height = mode->height;

	g_Config.Init();
	g_Config.Load();

	g_Drawing.Init();
	g_Visual.ResetJumpSpeed();

	g_pEngineFuncs->ClientCmd(xs("cl_timeout 999999;exec sven_internal.cfg"));

	m_flPlatTime = g_pEngineFuncs->Sys_FloatTime();
#if SECURITY_CHECKS
	m_flAntiDebugTime = g_pEngineFuncs->Sys_FloatTime();
#endif

	ConColorMsg({ 40, 255, 40, 255 }, xs("[Sven Internal] Successfully loaded\n"));

	if ( SvenModAPI()->GetClientState() == CLS_ACTIVE )
		g_ScriptVM.Init();

	return true;
}

void CSvenInternal::PostLoad(bool bGlobalLoad)
{
	if (bGlobalLoad)
	{
		
	}
	else
	{
		
	}

	PostLoadFeatures();

	g_ServerModule.PostInit();

	g_CamHack.Init();

	g_hAutoUpdateThread = AutoUpdate();
}

void CSvenInternal::Unload(void)
{
	if ( g_bAutoUpdateInProcess )
		CloseHandle( (HANDLE)g_hAutoUpdateThread );

	GL_Shutdown();

	g_ServerModule.Shutdown();

	UnloadFeatures();

	g_ServerClientBridge.Shutdown();

	ConVar_Unregister();

	AutoUpdate_ExtractAndLaunch();
}

bool CSvenInternal::Pause(void)
{
	Warning(xs("[Sven Internal] It is not allowed to pause the plugin\n"));
	return false;
}

void CSvenInternal::Unpause(void)
{
}

void CSvenInternal::OnFirstClientdataReceived(client_data_t *pcldata, float flTime)
{
	g_ullSteam64ID = SteamUser()->GetSteamID().ConvertToUint64();

	g_ScriptCallbacks.OnFirstClientdataReceived(flTime);
	g_SpeedrunTools.OnFirstClientdataReceived(pcldata, flTime);
	g_InputManager.OnFirstClientdataReceived();
	g_AntiAFK.OnEnterToServer();
}

void CSvenInternal::OnBeginLoading(void)
{
	g_ScriptCallbacks.OnBeginLoading();
	g_SpeedrunTools.OnBeginLoading();
	g_InputManager.OnBeginLoading();
}

void CSvenInternal::OnEndLoading(void)
{
	g_ScriptCallbacks.OnEndLoading();
	//g_SpeedrunTools.OnEndLoading();
	g_AntiAFK.OnEndLoading();
}

void CSvenInternal::OnDisconnect(void)
{
	g_ScriptCallbacks.OnDisconnect();
	g_ScriptVM.Shutdown();
}

#pragma optimize("", off)
void CSvenInternal::GameFrame(client_state_t state, double frametime, bool bPostRunCmd)
{
	extern bool g_bScreenshot;

	static bool s_bRemoveAntiScreen = false;
	static float s_flRemoveAntiScreenDuration = -1.f;

	float flPlatTime = g_pEngineFuncs->Sys_FloatTime();

	g_ScriptVM.Frame(state, frametime, bPostRunCmd);

	if ( bPostRunCmd )
	{
		if ( flPlatTime - m_flPlatTime >= 0.5f )
		{
			g_Config.UpdateConfigs();

			m_flPlatTime = flPlatTime;
		}

#if SECURITY_CHECKS
		if ( flPlatTime - m_flAntiDebugTime >= 5.0f )
		{
			// Check for debuggers or virtualization
			AntiDebug();

			__try
			{
				__asm
				{
					nop;
				}
			}
			__except ( EXCEPTION_EXECUTE_HANDLER )
			{
				exit( 555 );
				// decoy 
				__asm
				{
					xor ebx, ebx;
					mov ebx, 42;
					pop ebp;
					ret;
					int 3; 
					int 3; 
					int 3; 
					int 3; 
					int 3;
					int 3;
					int 3;
					push ebp;
					mov ebp, esp;
				}
			}

			m_flAntiDebugTime = flPlatTime;
		}
#endif
	}
	else
	{
		extern bool g_bPlayingbackDemo;

		g_bPlayingbackDemo = g_pDemoAPI->IsPlayingback();

		// Stupid anti screen decision
		if ( s_bRemoveAntiScreen )
		{
			if ( s_flRemoveAntiScreenDuration <= flPlatTime )
			{
				g_bScreenshot = false;
				s_bRemoveAntiScreen = false;
				s_flRemoveAntiScreenDuration = -1.f;
			}
		}
		else if ( g_bScreenshot )
		{
			s_bRemoveAntiScreen = true;
			s_flRemoveAntiScreenDuration = flPlatTime + 1.0f;
		}

		// Auto Update debugging messages
		if ( !g_AUDevMsgQueue.empty() )
		{
			for (size_t i = 0; i < g_AUDevMsgQueue.size(); i++)
			{
				DevMsg( g_AUDevMsgQueue[i].c_str() );
			}

			g_AUDevMsgQueue.clear();
		}
		
		if ( !g_AUColorMsgQueue.empty() )
		{
			for (size_t i = 0; i < g_AUColorMsgQueue.size(); i++)
			{
				ConColorMsg( g_AUColorMsgQueue[i].clr, g_AUColorMsgQueue[i].msg.c_str() );
			}

			g_AUColorMsgQueue.clear();
		}

		if ( state >= CLS_CONNECTED )
		{
			//SaveSoundcache();

			if ( state == CLS_ACTIVE )
			{
				g_Skybox.Think();
				g_ChatColors.Think();
				g_Visual.GameFrame();

				//static int wait_frames_TAGS = 0;

				//wait_frames_TAGS++;

				//if ( wait_frames_TAGS >= 100 )
				//{
				//	if ( !IsTertiaryAttackGlitchInit_Server() )
				//	{
				//	#ifdef PLATFORM_WINDOWS
				//		HMODULE hServerDLL = Sys_GetModuleHandle(xs("server.dll"));
				//	#else
				//		HMODULE hServerDLL = Sys_GetModuleHandle(xs("server.so"));
				//	#endif

				//		if ( hServerDLL )
				//		{
				//			extern void InitTertiaryAttackGlitch_Server(HMODULE hServerDLL);
				//			InitTertiaryAttackGlitch_Server(hServerDLL);
				//		}
				//	}

				//	wait_frames_TAGS = 0;
				//}
			}
			else
			{
				g_bForceFreeze2 = false;
			}
		}
		else
		{
			g_bForceFreeze2 = false;
		}
	}

	if ( state == CLS_ACTIVE )
	{
		g_SpeedrunTools.GameFrame( bPostRunCmd );
		g_InputManager.GameFrame( bPostRunCmd );
	}
}
#pragma optimize("", on)

void CSvenInternal::Draw(void)
{
	//g_Bsp.Draw();
	//g_SpeedrunTools.Draw();
	//g_Visual.Process();
	//g_Radar.Draw();
	//g_VotePopup.Draw();
}

void CSvenInternal::DrawHUD(float time, int intermission)
{
	g_Visual.OnHUDRedraw(time);
	g_SpeedrunTools.OnHUDRedraw(time);

	g_Bsp.Draw();
	g_SpeedrunTools.Draw();
	g_Visual.Draw();
	g_Radar.Draw();
	g_VotePopup.Draw();
}

FORCEINLINE void copy_obfuscated_str(const char *from, char *to, int size)
{
	memcpy(to, from, size);
}

static char svenint_name[ sizeof( xs("SvenInt")) ];
static char svenint_author[ sizeof(xs("Sw1ft / Reality")) ];
static char svenint_version[ sizeof(SVENINT_MAJOR_VERSION_STRING "." SVENINT_MINOR_VERSION_STRING "." SVENINT_PATCH_VERSION_STRING) ];
static char svenint_desc[ sizeof(xs("Provides various cheats and gameplay enhances")) ];
static char svenint_url[ sizeof( xs("https://steamcommunity.com/profiles/76561198397776991")) ];
static char svenint_date[ sizeof(SVENMOD_BUILD_TIMESTAMP) ];
static char svenint_tag[ sizeof(xs("SVENINT"))];

const char *CSvenInternal::GetName(void)
{
	copy_obfuscated_str(xs("SvenInt"), svenint_name, sizeof(svenint_name));

	return svenint_name;
}

const char *CSvenInternal::GetAuthor(void)
{
	copy_obfuscated_str(xs("Sw1ft / Reality"), svenint_author, sizeof(svenint_author));

	return svenint_author;
}

const char *CSvenInternal::GetVersion(void)
{
	copy_obfuscated_str(xs(SVENINT_MAJOR_VERSION_STRING "." SVENINT_MINOR_VERSION_STRING "." SVENINT_PATCH_VERSION_STRING), svenint_version, sizeof(svenint_version));

	return svenint_version;
}

const char *CSvenInternal::GetDescription(void)
{
	copy_obfuscated_str(xs("Provides various cheats and gameplay enhances"), svenint_desc, sizeof(svenint_desc));

	return svenint_desc;
}

const char *CSvenInternal::GetURL(void)
{
	copy_obfuscated_str(xs("https://steamcommunity.com/profiles/76561198397776991"), svenint_url, sizeof(svenint_url));

	return svenint_url;
}

const char *CSvenInternal::GetDate(void)
{
	copy_obfuscated_str(xs(SVENMOD_BUILD_TIMESTAMP), svenint_date, sizeof(svenint_date));

	return svenint_date;
}

const char *CSvenInternal::GetLogTag(void)
{
	copy_obfuscated_str(xs("SVENINT"), svenint_tag, sizeof(svenint_tag));

	return svenint_tag;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

void CSvenInternal::InitFolders(ISvenModAPI *pSvenModAPI)
{
	std::string sDir = pSvenModAPI->GetBaseDirectory();

#ifdef PLATFORM_WINDOWS
	if ( !CreateDirectory((sDir + xs("\\sven_internal\\")).c_str(), NULL) && GetLastError() != ERROR_ALREADY_EXISTS )
	{
		Warning(xs("[Sven Internal] Failed to create \"../sven_internal/\" directory\n"));
	}

	if ( !CreateDirectory((sDir + xs("\\sven_internal\\config\\")).c_str(), NULL) && GetLastError() != ERROR_ALREADY_EXISTS )
	{
		Warning(xs("[Sven Internal] Failed to create \"../sven_internal/config/\" directory\n"));
	}
	
	if ( !CreateDirectory((sDir + xs("\\sven_internal\\config\\shaders\\")).c_str(), NULL) && GetLastError() != ERROR_ALREADY_EXISTS )
	{
		Warning(xs("[Sven Internal] Failed to create \"../sven_internal/config/shaders/\" directory\n"));
	}

	if ( !CreateDirectory((sDir + xs("\\sven_internal\\message_spammer\\")).c_str(), NULL) && GetLastError() != ERROR_ALREADY_EXISTS )
	{
		Warning(xs("[Sven Internal] Failed to create \"../sven_internal/message_spammer/\" directory\n"));
	}

	if ( !CreateDirectory((sDir + xs("\\sven_internal\\input_manager\\")).c_str(), NULL) && GetLastError() != ERROR_ALREADY_EXISTS )
	{
		Warning(xs("[Sven Internal] Failed to create \"../sven_internal/input_manager/\" directory\n"));
	}
#else
#error Implement Linux equivalent
#endif

	sDir.clear();
}

//-----------------------------------------------------------------------------
// Export the plugin's interface
//-----------------------------------------------------------------------------

EXPOSE_SINGLE_INTERFACE_GLOBALVAR(CSvenInternal, IClientPlugin, CLIENT_PLUGIN_INTERFACE_VERSION, g_SvenInternal);

//-----------------------------------------------------------------------------
// Weirdo function
//-----------------------------------------------------------------------------

#ifdef PLATFORM_WINDOWS
static void SetFilesAttributes(const char *m_szFdPath, DWORD dwAttribute)
{
	HANDLE hFile;
	WIN32_FIND_DATAA FileInformation;

	char m_szPath[MAX_PATH];
	char m_szFolderInitialPath[MAX_PATH];
	char wildCard[MAX_PATH] = "\\*.*";

	strcpy(m_szPath, m_szFdPath);
	strcpy(m_szFolderInitialPath, m_szFdPath);
	strcat(m_szFolderInitialPath, wildCard);

	hFile = ::FindFirstFileA(m_szFolderInitialPath, &FileInformation);

	if (hFile != INVALID_HANDLE_VALUE)
	{
		do
		{
			if (FileInformation.cFileName[0] != '.')
			{
				strcpy(m_szPath, m_szFdPath);
				strcat(m_szPath, "\\");
				strcat(m_szPath, FileInformation.cFileName);

			#pragma warning(push)
			#pragma warning(push)
			#pragma warning(disable: 26450)
			#pragma warning(disable: 4307)

				if (!(FileInformation.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) && (FileInformation.nFileSizeHigh * (MAXDWORD + 1)) + FileInformation.nFileSizeLow > 0)
				{
					//it is a file
					::SetFileAttributesA(m_szPath, dwAttribute);
				}

			#pragma warning(pop)
			#pragma warning(pop)
			}
		} while (::FindNextFileA(hFile, &FileInformation) == TRUE);

		::FindClose(hFile);

		//DWORD dwError = ::GetLastError();
		//if (dwError == ERROR_NO_MORE_FILES)
		//{
		//	//Attributes successfully changed
		//}
	}
}

static int sleep_frames = 0;
static char szSoundcacheDirectory[MAX_PATH] = { 0 };

static void SaveSoundcache()
{
	if ( !*szSoundcacheDirectory )
	{
		snprintf(szSoundcacheDirectory, MAX_PATH, xs("%s\\svencoop_downloads\\maps\\soundcache\\"), SvenModAPI()->GetBaseDirectory());
	}

	if ( g_Config.cvars.save_soundcache )
	{
		sleep_frames++;

		if ( sleep_frames >= 75 )
		{
			SetFilesAttributes(szSoundcacheDirectory, FILE_ATTRIBUTE_NORMAL | FILE_ATTRIBUTE_READONLY);
			sleep_frames = 0;
		}
	}
}
#else
#error Implement Linux equivalent
#endif