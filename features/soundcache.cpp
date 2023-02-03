// Soundcache

#ifdef _WIN32
#include <Windows.h>
#endif

#include <ISvenModAPI.h>
#include <IMemoryUtils.h>
#include <IEngineClient.h>
#include <IFileSystem.h>

#include <convar.h>
#include <dbg.h>

#include <hl_sdk/common/protocol.h>

#include "soundcache.h"

#include "../patterns.h"
#include "../config.h"

//-----------------------------------------------------------------------------
// Declare Hooks
//-----------------------------------------------------------------------------

DECLARE_CLASS_HOOK(bool, CClient_SoundEngine__LoadSoundList, void *thisptr);

//-----------------------------------------------------------------------------
// Vars
//-----------------------------------------------------------------------------

CSoundcache g_Soundcache;

NetMsgHookFn ORIG_NetMsgHook_ResourceList = NULL;

static char mapname_buffer[MAX_PATH];
static char szMapName[MAX_PATH];

static char szSoundcacheDir[MAX_PATH];
static char szServerSoundcacheDir[MAX_PATH];
static char szServerSoundcacheFolder[MAX_PATH];

bool bHasSoundcache = false;

//-----------------------------------------------------------------------------
// ConCommands, CVars..
//-----------------------------------------------------------------------------

ConVar sc_autosave_soundcache("sc_autosave_soundcache", "1", 0, "Automatically save soundcache");

CON_COMMAND(sc_clear_soundcache, "Automatically save soundcache for different servers")
{
	FileFindHandle_t findHandleDir;

	std::string sDirectory = SvenModAPI()->GetBaseDirectory();
	const char *pszFileName = g_pFileSystem->FindFirst("maps\\soundcache\\", &findHandleDir, "GAMEDOWNLOAD");

	sDirectory += "\\svencoop_downloads\\maps\\soundcache\\";

	while (pszFileName)
	{
		std::string sFilePath = sDirectory + pszFileName;

		if ( !g_pFileSystem->FindIsDirectory(findHandleDir) )
		{
			SetFileAttributesA( sFilePath.c_str(), FILE_ATTRIBUTE_NORMAL );
		}

		DeleteFileA( sFilePath.c_str() );

		pszFileName = g_pFileSystem->FindNext( findHandleDir );
	}

	g_pFileSystem->FindClose( findHandleDir );
}

//-----------------------------------------------------------------------------
// Hooks
//-----------------------------------------------------------------------------

void HOOKED_NetMsgHook_ResourceList(void)
{
	if ( !sc_autosave_soundcache.GetBool() )
		return ORIG_NetMsgHook_ResourceList();

	netadr_t addr;
	int port;

	char mapname_buffer[MAX_PATH];

	char *pszMapName = mapname_buffer;
	char *pszExt = NULL;

	strncpy(mapname_buffer, g_pEngineFuncs->GetLevelName(), MAX_PATH);

	// maps/<mapname>.bsp to <mapname>
	while (*pszMapName)
	{
		if (*pszMapName == '/')
		{
			pszMapName++;
			break;
		}

		pszMapName++;
	}

	pszExt = pszMapName;

	while (*pszExt)
	{
		if (*pszExt == '.')
		{
			*pszExt = 0;
			break;
		}

		pszExt++;
	}

	g_pEngineClient->GetServerAddress(&addr);
	port = (addr.port << 8) | (addr.port >> 8); // it's swapped

	strncpy(szMapName, pszMapName, MAX_PATH);

	snprintf(szSoundcacheDir, MAX_PATH, "maps\\soundcache\\%s.txt", szMapName);
	snprintf(szServerSoundcacheDir, MAX_PATH, "maps\\soundcache\\%hhu.%hhu.%hhu.%hhu %hu\\%s.txt", addr.ip[0], addr.ip[1], addr.ip[2], addr.ip[3], port, szMapName);
	snprintf(szServerSoundcacheFolder, MAX_PATH, "maps\\soundcache\\%hhu.%hhu.%hhu.%hhu %hu", addr.ip[0], addr.ip[1], addr.ip[2], addr.ip[3], port);

	// Uh we have the soundcache that wasn't deleted
	if ( g_pFileSystem->FileExists(szSoundcacheDir) )
	{
		std::string sDirectory = SvenModAPI()->GetBaseDirectory();

		SetFileAttributesA((sDirectory + "\\svencoop_downloads\\" + szSoundcacheDir).c_str(), FILE_ATTRIBUTE_NORMAL); // WinAPI
		//DeleteFileA(szFullSoundcacheDir);
		g_pFileSystem->RemoveFile(szSoundcacheDir, "GAMEDOWNLOAD");
	}

	// If client already has the saved soundcache then create a dummy file so we won't download anything..
	if ( g_pFileSystem->FileExists(szServerSoundcacheDir) )
	{
		FileHandle_t hFile = g_pFileSystem->Open(szSoundcacheDir, "a+", "GAMEDOWNLOAD"); // dummy

		if ( hFile )
		{
			g_pFileSystem->Close(hFile);
		}

		bHasSoundcache = true;
	}
	else
	{
		bHasSoundcache = false;
	}

	ORIG_NetMsgHook_ResourceList();
}

DECLARE_CLASS_FUNC(bool, HOOKED_CClient_SoundEngine__LoadSoundList, void *thisptr)
{
	if ( !sc_autosave_soundcache.GetBool() )
		return ORIG_CClient_SoundEngine__LoadSoundList(thisptr);

	if ( g_pFileSystem->FileExists(szSoundcacheDir) )
	{
		std::string sDirectory = SvenModAPI()->GetBaseDirectory();
		std::string sSoundcacheDir = sDirectory + "\\svencoop_downloads\\" + szSoundcacheDir;
		std::string sServerSoundcacheDir = sDirectory + "\\svencoop_downloads\\" + szServerSoundcacheDir;

		SetFileAttributesA(sSoundcacheDir.c_str(), FILE_ATTRIBUTE_NORMAL);

		if ( !CreateDirectoryA((sDirectory + "\\svencoop_downloads\\" + szServerSoundcacheFolder).c_str(), NULL) && GetLastError() != ERROR_ALREADY_EXISTS )
		{
			Warning("Failed to create directory \"..\\%s\"\n", szServerSoundcacheFolder);
			return ORIG_CClient_SoundEngine__LoadSoundList(thisptr);
		}

		if ( bHasSoundcache )
		{
			g_pFileSystem->RemoveFile(szSoundcacheDir, "GAMEDOWNLOAD");

			if ( CopyFileA(sServerSoundcacheDir.c_str(), sSoundcacheDir.c_str(), true) )
			{
				Msg("Used saved soundcache for the current map \"%s\"\n", szMapName);
			}
			else
			{
				Warning("Failed to replace soundcache for the current map \"%s\"\n", szMapName);
			}
		}
		else
		{
			if ( CopyFileA(sSoundcacheDir.c_str(), sServerSoundcacheDir.c_str(), true) )
			{
				Msg("Saved soundcache for the current map \"%s\"\n", szMapName);
			}
			else
			{
				Warning("Failed to save soundcache for the current map \"%s\"\n", szMapName);
			}
		}
	}
	else
	{
		Warning("Unable to find soundcache for the current map \"%s\"\n", szMapName);
	}

	return ORIG_CClient_SoundEngine__LoadSoundList(thisptr);
}

//-----------------------------------------------------------------------------
// Init
//-----------------------------------------------------------------------------

CSoundcache::CSoundcache()
{
	m_hCClient_SoundEngine__LoadSoundList = 0;
}

bool CSoundcache::Load()
{
	m_pfnCClient_SoundEngine__LoadSoundList = MemoryUtils()->FindPattern( SvenModAPI()->Modules()->Client, Patterns::Client::CClient_SoundEngine__LoadSoundList );

	if ( !m_pfnCClient_SoundEngine__LoadSoundList )
	{
		Warning("Couldn't find function \"CClient_SoundEngine::LoadSoundList\"\n");
		return false;
	}

	return true;
}

void CSoundcache::PostLoad()
{
	m_hNetMsgHook_ResourceList = Hooks()->HookNetworkMessage( SVC_RESOURCELIST, HOOKED_NetMsgHook_ResourceList, &ORIG_NetMsgHook_ResourceList );
	m_hCClient_SoundEngine__LoadSoundList = DetoursAPI()->DetourFunction( m_pfnCClient_SoundEngine__LoadSoundList, HOOKED_CClient_SoundEngine__LoadSoundList, GET_FUNC_PTR(ORIG_CClient_SoundEngine__LoadSoundList) );
}

void CSoundcache::Unload()
{
	DetoursAPI()->RemoveDetour( m_hNetMsgHook_ResourceList );
	DetoursAPI()->RemoveDetour( m_hCClient_SoundEngine__LoadSoundList );
}


/*
	ud_t inst;

	m_pfnCL_ParseConsistencyInfo = MemoryUtils()->FindPattern( SvenModAPI()->Modules()->Hardware, Patterns::Hardware::CL_ParseConsistencyInfo );

	if ( !m_pfnCL_ParseConsistencyInfo )
	{
		Warning("Couldn't find function \"CL_ParseConsistencyInfo\"\n");
		return false;
	}

	DEFINE_PATTERN(ResourceList_Offset, "A1 ? ? ? ? 85 C0 74 ? 83 3D ? ? ? ? ? 74 ? 89 86 84 00 00 00");
	DEFINE_PATTERN(dummy_resource_list, "C7 86 80 00 00 00 ? ? ? ? EB ?");

	void *pResourceList_Offset = MemoryUtils()->FindPattern( SvenModAPI()->Modules()->Hardware, ResourceList_Offset );

	void *pResourceListDummy_Offset = MemoryUtils()->FindPatternWithin( SvenModAPI()->Modules()->Hardware,
																		   dummy_resource_list,
																		   pResourceList_Offset,
																		   (unsigned char *)pResourceList_Offset + 72 );

	if ( !pResourceList_Offset )
	{
		Warning("Failed to locate resource list\n");
		return false;
	}

	if ( !pResourceListDummy_Offset )
	{
		Warning("Failed to locate dummy resource list\n");
		return false;
	}

	MemoryUtils()->InitDisasm(&inst, pResourceList_Offset, 32, 16);

	if ( MemoryUtils()->Disassemble(&inst) )
	{
		if (inst.mnemonic == UD_Imov && inst.operand[0].type == UD_OP_REG && inst.operand[0].base == UD_R_EAX && inst.operand[1].type == UD_OP_MEM)
		{
			g_pResourceList = reinterpret_cast<resource_t *>(inst.operand[1].lval.udword);
		}
	}

	if ( !g_pResourceList )
	{
		Warning("Failed to get resource list\n");
		return false;
	}

	MemoryUtils()->InitDisasm(&inst, pResourceListDummy_Offset, 32, 16);

	if (MemoryUtils()->Disassemble(&inst))
	{
		if (inst.mnemonic == UD_Imov && inst.operand[0].type == UD_OP_MEM && inst.operand[0].base == UD_R_ESI && inst.operand[0].offset == 32 && inst.operand[1].type == UD_OP_IMM)
		{
			g_pResourceListDummy = reinterpret_cast<resource_t *>(inst.operand[1].lval.udword);
		}
	}

	if ( !g_pResourceListDummy )
	{
		Warning("Failed to get dummy resource list\n");
		return false;
	}
*/