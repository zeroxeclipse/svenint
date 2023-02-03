// Model Manager

#include <stdlib.h>
#include <time.h>

#include <dbg.h>
#include <convar.h>
#include <messagebuffer.h>

#include <ISvenModAPI.h>
#include <IMemoryUtils.h>
#include <IHooks.h>
#include <IUtils.h>

#include <hl_sdk/common/protocol.h>
#include <hl_sdk/common/netmsg.h>

#include "models_manager.h"

#include "../config.h"
#include "../patterns.h"

//-----------------------------------------------------------------------------
// Declare hooks
//-----------------------------------------------------------------------------

DECLARE_HOOK(struct model_s *, __cdecl, SetupPlayerModel, int index);

//-----------------------------------------------------------------------------
// Imports
//-----------------------------------------------------------------------------

extern IClientPlugin *g_pClientPlugin;

//-----------------------------------------------------------------------------
// Hash Map Functions
//-----------------------------------------------------------------------------

static bool MM_HashMap_Compare(const uint64 &a, const uint64 &b) { return a == b; }
static unsigned int MM_HashMap_Hash(const uint64 &a) { return HashKey((unsigned char *)&a, sizeof(uint64)); }

//-----------------------------------------------------------------------------
// Structures
//-----------------------------------------------------------------------------

struct player_model_replacement_info_s
{
	uint64 steamid;
	int random_model;
	bool model_replaced;
};

//-----------------------------------------------------------------------------
// Vars
//-----------------------------------------------------------------------------

constexpr int UserInfo_ModelOffset = 592;

CModelsManager g_ModelsManager;
CMessageBuffer UpdateUserInfoBuffer;

std::string g_ReplacePlayerModel;
char g_szReplacePlayerModelBuffer[64] = { 0 };

unsigned char *g_pUserInfo = NULL;
NetMsgHookFn ORIG_NetMsgHook_UpdateUserInfo = NULL;

player_model_replacement_info_s g_PlayerModelReplacementInfo[MAXCLIENTS];

//-----------------------------------------------------------------------------
// Parse stuff
//-----------------------------------------------------------------------------

#define PARSE_COMMENT_PREFIX ";#"
#define PARSE_STRIP_CHARS (" \t\n")

#define PARSE_STRIP_CHARS_LEN (sizeof(PARSE_STRIP_CHARS) - 1)
#define PARSE_COMMENT_PREFIX_LEN (sizeof(PARSE_COMMENT_PREFIX) - 1)

static int UTIL_contains_chars(char ch, const char *chars, size_t length)
{
	for (size_t i = 0; i < length; ++i)
	{
		if (chars[i] == ch)
			return 1;
	}

	return 0;
}

static char *UTIL_lstrip(char *str)
{
	while (*str && UTIL_contains_chars(*str, PARSE_STRIP_CHARS, PARSE_STRIP_CHARS_LEN))
		++str;

	return str;
}

static void UTIL_rstrip(char *str)
{
	char *end = str + strlen(str) - 1;

	if (end < str)
		return;

	while (end >= str && UTIL_contains_chars(*end, PARSE_STRIP_CHARS, PARSE_STRIP_CHARS_LEN))
	{
		*end = '\0';
		--end;
	}
}

static void UTIL_remove_comment(char *str)
{
	while (*str && !UTIL_contains_chars(*str, PARSE_COMMENT_PREFIX, PARSE_COMMENT_PREFIX_LEN))
		++str;

	if (*str)
		*str = '\0';
}

//-----------------------------------------------------------------------------
// ConCommands
//-----------------------------------------------------------------------------

CON_COMMAND(sc_mm_reload_random_models, "Reload a list of random models from the file \"../sven_internal/models_manager/random_models.txt\"")
{
	g_ModelsManager.ReloadRandomModels();
}

CON_COMMAND(sc_mm_reload_target_players, "Reload a list of target players from the file \"../sven_internal/models_manager/target_players.txt\"")
{
	g_ModelsManager.ReloadTargetPlayers();
}

CON_COMMAND(sc_mm_reload_ignored_players, "Reload a list of ignored players from the file \"../sven_internal/models_manager/ignored_players.txt\"")
{
	g_ModelsManager.ReloadIgnoredPlayers();
}

//-----------------------------------------------------------------------------
// Hooks
//-----------------------------------------------------------------------------

void HOOKED_NetMsgHook_UpdateUserInfo(void)
{
	CNetMessageParams *params = Utils()->GetNetMessageParams();

	UpdateUserInfoBuffer.Init( params->buffer, params->readcount, params->badread );

	int index = UpdateUserInfoBuffer.ReadByte();
	const char *pszModel = (const char *)(&g_pUserInfo[UserInfo_ModelOffset * index]);

	ORIG_NetMsgHook_UpdateUserInfo();

	//g_ModelsManager.UpdatePlayerModel(index);

	g_ModelsManager.ResetPlayerInfo(index);
}

DECLARE_FUNC(struct model_s *, __cdecl, HOOKED_SetupPlayerModel, int index)
{
	//if ( index == g_pPlayerMove->player_index )
	//{
	//	if ( g_Config.cvars.replace_model_on_self )
	//	{
	//		const char *pszModel = (const char *)(&g_pUserInfo[UserInfo_ModelOffset * index]);
	//		memcpy( (char *)pszModel, g_ReplacePlayerModel.c_str(), g_ReplacePlayerModel.length() + 1 );
	//	}
	//}
	//else if ( g_Config.cvars.replace_players_models )
	//{
	//	const char *pszModel = (const char *)(&g_pUserInfo[UserInfo_ModelOffset * index]);
	//	memcpy( (char *)pszModel, g_ReplacePlayerModel.c_str(), g_ReplacePlayerModel.length() + 1 );
	//}

	g_ModelsManager.UpdatePlayerModel(index);

	auto result = ORIG_SetupPlayerModel(index);

	return result;
}

//-----------------------------------------------------------------------------
// CModelsManager implementations
//-----------------------------------------------------------------------------

CModelsManager::CModelsManager() : m_TargetPlayers(15), m_IgnorePlayers(15, MM_HashMap_Compare, MM_HashMap_Hash)
{
	static char s_szDefaultPlayerModel[] = "player";

	srand( time(0) );

	g_ReplacePlayerModel = s_szDefaultPlayerModel;
	memcpy( g_szReplacePlayerModelBuffer, s_szDefaultPlayerModel, sizeof(s_szDefaultPlayerModel) );

	ResetPlayersInfo();
}

void CModelsManager::OnConfigLoad()
{
	if (g_Config.cvars.replace_model != NULL)
	{
		g_ReplacePlayerModel.assign( g_Config.cvars.replace_model );
		memcpy( g_szReplacePlayerModelBuffer, g_Config.cvars.replace_model, strlen(g_Config.cvars.replace_model) + 1 );

		free( (void *)g_Config.cvars.replace_model );
		g_Config.cvars.replace_model = NULL;
	}
}

void CModelsManager::OnVideoInit()
{
	ResetPlayersInfo();
}

void CModelsManager::ResetPlayersInfo()
{
	memset( g_PlayerModelReplacementInfo, 0, sizeof(player_model_replacement_info_s) * MAXCLIENTS );
}

void CModelsManager::ResetLocalPlayerInfo()
{
	//memset( &g_PlayerModelReplacementInfo[index], 0, sizeof(player_model_replacement_info_s) );

	g_PlayerModelReplacementInfo[g_pPlayerMove->player_index].steamid = 0uLL;
	g_PlayerModelReplacementInfo[g_pPlayerMove->player_index].random_model = 0;
	g_PlayerModelReplacementInfo[g_pPlayerMove->player_index].model_replaced = false;
}

void CModelsManager::ResetPlayerInfo(int index)
{
	//memset( &g_PlayerModelReplacementInfo[index], 0, sizeof(player_model_replacement_info_s) );

	g_PlayerModelReplacementInfo[index].model_replaced = false;
}

void CModelsManager::CheckPlayerInfo(int index)
{
	uint64 steamid = g_pPlayerUtils->GetSteamID(index + 1);

	if (steamid != g_PlayerModelReplacementInfo[index].steamid)
	{
		g_PlayerModelReplacementInfo[index].steamid = steamid;
		g_PlayerModelReplacementInfo[index].random_model = 0;
		g_PlayerModelReplacementInfo[index].model_replaced = false;
	}
}

void CModelsManager::UpdatePlayerModel(int index)
{
	CheckPlayerInfo(index);

	uint64 *pIgnoreSteamID = NULL;
	std::string *pTargetModel = NULL;

	if ( g_Config.cvars.replace_specified_players_models )
	{
		pTargetModel = m_TargetPlayers.Find(g_PlayerModelReplacementInfo[index].steamid);

		if ( pTargetModel != NULL )
		{
			if ( !g_PlayerModelReplacementInfo[index].model_replaced )
			{
				const char *pszModel = (const char *)(&g_pUserInfo[UserInfo_ModelOffset * index]);

				memcpy( (char *)pszModel, (*pTargetModel).c_str(), (*pTargetModel).length() + 1 );

				g_PlayerModelReplacementInfo[index].random_model = 0;
				g_PlayerModelReplacementInfo[index].model_replaced = true;

				return;
			}
		}
	}

	if ( g_Config.cvars.dont_replace_specified_players_models )
	{
		pIgnoreSteamID = m_IgnorePlayers.Find(g_PlayerModelReplacementInfo[index].steamid);

		if ( pIgnoreSteamID != NULL )
			return;
	}

	if ( g_Config.cvars.replace_players_models_with_randoms && !m_RandomModels.empty() )
	{
		if ( index != g_pPlayerMove->player_index || (index == g_pPlayerMove->player_index && g_Config.cvars.replace_model_on_self) )
		{
			if ( !g_PlayerModelReplacementInfo[index].model_replaced )
			{
				if ( pIgnoreSteamID == NULL && pTargetModel == NULL )
				{
					int model_index = rand() % m_RandomModels.size();
					
					if (g_PlayerModelReplacementInfo[index].random_model != 0)
						model_index = g_PlayerModelReplacementInfo[index].random_model = (g_PlayerModelReplacementInfo[index].random_model - 1);
					else
						model_index = rand() % m_RandomModels.size();

					const char *pszModel = (const char *)(&g_pUserInfo[UserInfo_ModelOffset * index]);

					memcpy( (char *)pszModel, m_RandomModels[model_index].c_str(), m_RandomModels[model_index].length() + 1 );

					g_PlayerModelReplacementInfo[index].random_model = model_index + 1;
					g_PlayerModelReplacementInfo[index].model_replaced = true;

					return;
				}
			}
		}
	}

	if ( g_Config.cvars.replace_players_models )
	{
		if ( index != g_pPlayerMove->player_index || (index == g_pPlayerMove->player_index && g_Config.cvars.replace_model_on_self) )
		{
			if ( !g_PlayerModelReplacementInfo[index].model_replaced )
			{
				if ( pIgnoreSteamID == NULL && pTargetModel == NULL )
				{
					const char *pszModel = (const char *)(&g_pUserInfo[UserInfo_ModelOffset * index]);

					memcpy( (char *)pszModel, g_ReplacePlayerModel.c_str(), g_ReplacePlayerModel.length() + 1 );

					g_PlayerModelReplacementInfo[index].random_model = 0;
					g_PlayerModelReplacementInfo[index].model_replaced = true;
				}
			}
		}
	}
}

void CModelsManager::ReloadRandomModels()
{
	static char szBuffer[512];
	FILE *file = fopen("sven_internal/models_manager/random_models.txt", "r");

	if (file)
	{
		int nLine = 0;

		m_RandomModels.clear();

		while (fgets(szBuffer, sizeof(szBuffer), file))
		{
			nLine++;

			char *buffer = UTIL_lstrip(szBuffer);
			UTIL_remove_comment(buffer);
			UTIL_rstrip(buffer);

			if ( !*buffer )
				continue;

			m_RandomModels.push_back( std::string(buffer) );
		}

		ResetPlayersInfo();
		fclose(file);
	}
	else
	{
		Warning("[Models Manager] Missing file \"sven_internal/models_manager/random_models.txt\"\n");
	}
}

void CModelsManager::ReloadTargetPlayers()
{
	static char szBuffer[512];
	static char szParameterBuffer[512];

	FILE *file = fopen("sven_internal/models_manager/target_players.txt", "r");

	if (file)
	{
		int nLine = 0;

		m_TargetPlayers.Clear();

		while (fgets(szBuffer, sizeof(szBuffer), file))
		{
			nLine++;

			char *buffer = UTIL_lstrip(szBuffer);
			UTIL_remove_comment(buffer);
			UTIL_rstrip(buffer);

			if ( !*buffer )
				continue;

			char *key = strtok(buffer, "=");

			if ( !key || !*key )
			{
				Warning("Key of a parameter is empty (line: %d)\n", nLine);
				continue;
			}

			char *value = strtok(NULL, "=");

			if ( !value || !*value )
			{
				Warning("Value of a parameter is empty (line: %d)\n", nLine);
				continue;
			}

			UTIL_rstrip(key);
			UTIL_rstrip(value);
			value = UTIL_lstrip(value);

			uint64 steamid = atoll(key);

			if (steamid != 0uLL)
			{
				memcpy(szParameterBuffer, value, strlen(value) + 1);
				szParameterBuffer[(sizeof(szParameterBuffer) / sizeof(*szParameterBuffer)) - 1] = 0;

				m_TargetPlayers.Insert( steamid, std::string(szParameterBuffer) );
			}
			else
			{
				Warning("Invalid Steam64 ID at line %d\n", nLine);
			}
		}

		ResetPlayersInfo();
		fclose(file);
	}
	else
	{
		Warning("[Models Manager] Missing file \"sven_internal/models_manager/target_players.txt\"\n");
	}
}

void CModelsManager::ReloadIgnoredPlayers()
{
	static char szBuffer[512];
	FILE *file = fopen("sven_internal/models_manager/ignored_players.txt", "r");

	if (file)
	{
		int nLine = 0;

		m_IgnorePlayers.Clear();

		while (fgets(szBuffer, sizeof(szBuffer), file))
		{
			nLine++;

			char *buffer = UTIL_lstrip(szBuffer);
			UTIL_remove_comment(buffer);
			UTIL_rstrip(buffer);

			if ( !*buffer )
				continue;

			uint64 steamid = atoll(buffer);

			if (steamid != 0uLL)
			{
				m_IgnorePlayers.Insert(steamid);
			}
			else
			{
				Warning("Invalid Steam64 ID at line %d\n", nLine);
			}
		}

		ResetPlayersInfo();
		fclose(file);
	}
	else
	{
		Warning("[Models Manager] Missing file \"sven_internal/models_manager/ignored_players.txt\"\n");
	}
}

//-----------------------------------------------------------------------------
// Initialize
//-----------------------------------------------------------------------------

bool CModelsManager::Load()
{
	ud_t inst;

	void *pUserInfo_Offset = MemoryUtils()->FindPatternWithin(SvenModAPI()->Modules()->Hardware, Patterns::Hardware::UserInfo_Offset,
															  g_pEngineStudio->SetupPlayerModel,
															  (unsigned char *)g_pEngineStudio->SetupPlayerModel + 72);

	if ( !pUserInfo_Offset )
	{
		Warning("Failed to locate userinfo\n");
		return false;
	}

	MemoryUtils()->InitDisasm(&inst, pUserInfo_Offset, 32, 8);

	do
	{
		if (inst.mnemonic == UD_Ilea && inst.operand[0].type == UD_OP_REG && inst.operand[0].base == UD_R_EDI &&
			inst.operand[1].type == UD_OP_MEM && inst.operand[1].base == UD_R_EAX && inst.operand[1].offset == 32)
		{
			g_pUserInfo = reinterpret_cast<unsigned char *>(inst.operand[1].lval.udword);
			break;
		}

	} while ( MemoryUtils()->Disassemble(&inst) );

	
	if ( !g_pUserInfo )
	{
		Warning("Failed to get userinfo\n");
		return false;
	}

	if ( SvenModAPI()->GetClientVersion()->version != 5025 )
	{
		PluginLogWarning(g_pClientPlugin, "Validate userinfo's model offset\n");
	}

	return true;
}

void CModelsManager::PostLoad()
{
	m_hNetMsgHook_UpdateUserInfo = Hooks()->HookNetworkMessage( SVC_UPDATEUSERINFO, HOOKED_NetMsgHook_UpdateUserInfo, &ORIG_NetMsgHook_UpdateUserInfo );
	m_hSetupPlayerModel = DetoursAPI()->DetourFunction( g_pEngineStudio->SetupPlayerModel, HOOKED_SetupPlayerModel, GET_FUNC_PTR(ORIG_SetupPlayerModel) );

	ReloadRandomModels();
	ReloadTargetPlayers();
	ReloadIgnoredPlayers();
}

void CModelsManager::Unload()
{
	DetoursAPI()->RemoveDetour( m_hNetMsgHook_UpdateUserInfo );
	DetoursAPI()->RemoveDetour( m_hSetupPlayerModel );
}