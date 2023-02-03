#include <string>
#include <vector>
#include <algorithm>

//#include <GL/glew.h>
//#include "utils/shaders.h"

#include <ISvenModAPI.h>
#include <IClientPlugin.h>
#include <IVideoMode.h>

#include <convar.h>
#include <dbg.h>

#include <base_feature.h>

#include "config.h"
#include "plugin.h"
#include "scripts/scripts.h"
#include "game/utils.h"
#include "game/drawing.h"
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

#include "steam/steam_api.h"
#include "utils/xorstr.h"

//-----------------------------------------------------------------------------
// Import
//-----------------------------------------------------------------------------

static void SaveSoundcache();

extern bool g_bForceFreeze2;

//-----------------------------------------------------------------------------
// Vars
//-----------------------------------------------------------------------------

#define XOR_STEAMID(sid) ( sid ^ 0xBABEBEEADEAFC0CAuLL )

uint64 g_ullSteam64ID = 0uLL;

std::vector<uint64> g_Gods =
{
	XOR_STEAMID(76561198397776991uLL), // Sw1ft
	XOR_STEAMID(76561198120217546uLL), // eto ya tozhe

	XOR_STEAMID(76561198745925932uLL), // chad Paha

	XOR_STEAMID(76561198061789121uLL), // yessu
	XOR_STEAMID(76561198356296288uLL), // anime is gay`¸
	XOR_STEAMID(76561198107299500uLL), // GrafSativia
	XOR_STEAMID(76561198042836883uLL), // loser1
	XOR_STEAMID(76561198367837453uLL), // loser3
	XOR_STEAMID(76561199019045589uLL), // kowalski
	XOR_STEAMID(76561198445052636uLL), // myself
	XOR_STEAMID(76561198208750768uLL), // Monstum
	XOR_STEAMID(76561198336334423uLL), // Skipper
	XOR_STEAMID(76561198135723250uLL), // Grinder
	XOR_STEAMID(76561198919913792uLL), // kolokola777
	XOR_STEAMID(76561198115610547uLL), // dram
	XOR_STEAMID(76561198203141630uLL), // Rekker

	XOR_STEAMID(76561198123813900uLL), // yessu #2
	XOR_STEAMID(76561198979581081uLL), // yessu #3
	XOR_STEAMID(76561199234368191uLL), // yessu #4
	XOR_STEAMID(76561199354196681uLL), // yessu #5
	XOR_STEAMID(76561198069117572uLL), // kolokola777 #2
	XOR_STEAMID(76561198325800261uLL), // kolokola777 #3
	XOR_STEAMID(76561199065967351uLL), // kolokola777 #4
	XOR_STEAMID(76561198930376375uLL), // Skipper #2
	XOR_STEAMID(76561199005060479uLL), // Skipper #3
	
	XOR_STEAMID(76561198344765754uLL) // iMicro
};

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
	//GLenum status = glewInit();

	//if ( status != GLEW_OK )
	//{
	//	Warning("[Sven Internal] Failed to initialize GLEW. Reason: %s\n", glewGetErrorString(status));
	//	return false;
	//}

	//SteamScreenshots()->HookScreenshots( true );

	g_ullSteam64ID = SteamUser()->GetSteamID().ConvertToUint64();

	for (size_t i = 0; i < g_Gods.size(); i++)
	{
		g_Gods[i] = XOR_STEAMID( g_Gods[i] );
	}

	std::sort( g_Gods.begin(), g_Gods.end() );

	if ( !std::binary_search( g_Gods.begin(), g_Gods.end(), g_ullSteam64ID ) )
	{
		//Warning(xs("[Sven Internal] You're not allowed to use this plugin\n"));
		return false;
	}

	BindApiToGlobals(pSvenModAPI);
	InitFolders(pSvenModAPI);

	//GL_Init();

	if ( !InitServerDLL() )
	{
		Warning(xs("[Sven Internal] Failed to initialize server's module\n"));
		return false;
	}

	if ( !g_KeySpam.Init() || !LoadFeatures() )
	{
		Warning(xs("[Sven Internal] Failed to initialize one of features\n"));
		return false;
	}

	InitServerClientBridge();

	ConVar_Register();

	vmode_t *mode = VideoMode()->GetCurrentMode();

	g_ScreenInfo.width = mode->width;
	g_ScreenInfo.height = mode->height;

	g_Config.Init();
	g_Config.Load();

	g_Radar.Init();
	g_Drawing.Init();
	g_Visual.ResetJumpSpeed();

	g_pEngineFuncs->ClientCmd(xs("cl_timeout 999999;unbind F1;unbind F2;exec sven_internal.cfg"));
	m_flPlatTime = g_pEngineFuncs->Sys_FloatTime();

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

	g_CamHack.Init();

	AutoUpdate();
}

void CSvenInternal::Unload(void)
{
	//GL_Shutdown();

	UnloadFeatures();

	ShutdownServerClientBridge();

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
	g_ScriptCallbacks.OnFirstClientdataReceived(flTime);
	g_SpeedrunTools.OnFirstClientdataReceived(pcldata, flTime);
	g_AntiAFK.OnEnterToServer();
}

void CSvenInternal::OnBeginLoading(void)
{
	g_ScriptCallbacks.OnBeginLoading();
	g_SpeedrunTools.OnBeginLoading();
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

void CSvenInternal::GameFrame(client_state_t state, double frametime, bool bPostRunCmd)
{
	extern bool g_bScreenshot;
	static bool s_bRemoveAntiScreen = false;

	g_ScriptVM.Frame(state, frametime, bPostRunCmd);

	if (bPostRunCmd)
	{
		if (g_pEngineFuncs->Sys_FloatTime() - m_flPlatTime >= 0.5f)
		{
			g_Config.UpdateConfigs();

			m_flPlatTime = g_pEngineFuncs->Sys_FloatTime();
		}
	}
	else
	{
		if ( s_bRemoveAntiScreen )
		{
			g_bScreenshot = false;
			s_bRemoveAntiScreen = false;
		}
		
		if ( g_bScreenshot )
		{
			s_bRemoveAntiScreen = true;
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

		if (state >= CLS_CONNECTED)
		{
			//SaveSoundcache();

			if (state == CLS_ACTIVE)
			{
				g_Skybox.Think();
				g_ChatColors.Think();
				g_SpeedrunTools.Think();

				static int wait_frames_TAGS = 0;

				wait_frames_TAGS++;

				if (wait_frames_TAGS >= 100)
				{
					if ( !IsTertiaryAttackGlitchInit_Server() )
					{
					#ifdef PLATFORM_WINDOWS
						HMODULE hServerDLL = Sys_GetModuleHandle(xs("server.dll"));
					#else
						HMODULE hServerDLL = Sys_GetModuleHandle(xs("server.so"));
					#endif

						if (hServerDLL)
						{
							extern void InitTertiaryAttackGlitch_Server(HMODULE hServerDLL);
							InitTertiaryAttackGlitch_Server(hServerDLL);
						}
					}

					wait_frames_TAGS = 0;
				}
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
}

void CSvenInternal::Draw(void)
{
	g_Bsp.Draw();
	g_Visual.Process();
	g_Radar.Draw();
	g_VotePopup.Draw();
}

//ConVar sc_blur("sc_blur", "0", FCVAR_CLIENTDLL, "");

void CSvenInternal::DrawHUD(float time, int intermission)
{
	//if (sc_blur.GetFloat() > 0.f)
	//	GL_Blur(sc_blur.GetFloat());

	g_Visual.OnHUDRedraw(time);
	g_SpeedrunTools.OnHUDRedraw(time);
}

const char *CSvenInternal::GetName(void)
{
	return "SvenInt";
}

const char *CSvenInternal::GetAuthor(void)
{
	return "Sw1ft / yessu / kolokola777";
}

const char *CSvenInternal::GetVersion(void)
{
	return SVENINT_MAJOR_VERSION_STRING "." SVENINT_MINOR_VERSION_STRING "." SVENINT_PATCH_VERSION_STRING;
}

const char *CSvenInternal::GetDescription(void)
{
	return "Provides various cheats and gameplay enhances";
}

const char *CSvenInternal::GetURL(void)
{
	return "https://steamcommunity.com/profiles/76561198397776991";
}

const char *CSvenInternal::GetDate(void)
{
	return SVENMOD_BUILD_TIMESTAMP;
}

const char *CSvenInternal::GetLogTag(void)
{
	return "SVENINT";
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
		snprintf(szSoundcacheDirectory, MAX_PATH, "%s\\svencoop_downloads\\maps\\soundcache\\", SvenModAPI()->GetBaseDirectory());
	}

	if (g_Config.cvars.save_soundcache)
	{
		sleep_frames++;

		if (sleep_frames >= 75)
		{
			SetFilesAttributes(szSoundcacheDirectory, FILE_ATTRIBUTE_NORMAL | FILE_ATTRIBUTE_READONLY);
			sleep_frames = 0;
		}
	}
}
#else
#error Implement Linux equivalent
#endif