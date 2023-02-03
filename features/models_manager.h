#ifndef MODELS_MANAGER_H
#define MODELS_MANAGER_H

#ifdef _WIN32
#pragma once
#endif

#include <string>
#include <vector>

#include <IDetoursAPI.h>
#include <base_feature.h>

#include <data_struct/hashtable.h>
#include <data_struct/hash.h>

//-----------------------------------------------------------------------------
// Models Manager
//-----------------------------------------------------------------------------

class CModelsManager : public CBaseFeature
{
public:
	CModelsManager();

	virtual bool Load();
	virtual void PostLoad();

	virtual void Unload();

public:
	void OnConfigLoad();
	void OnVideoInit();

	void ReloadRandomModels();
	void ReloadTargetPlayers();
	void ReloadIgnoredPlayers();

	void ResetPlayersInfo();
	void ResetLocalPlayerInfo();
	void ResetPlayerInfo(int index);
	void CheckPlayerInfo(int index);
	void UpdatePlayerModel(int index);

private:
	std::vector<std::string> m_RandomModels;

	CHashTable<uint64, std::string> m_TargetPlayers;
	CHash<uint64> m_IgnorePlayers;

	DetourHandle_t m_hNetMsgHook_UpdateUserInfo;
	DetourHandle_t m_hSetupPlayerModel;
};

extern CModelsManager g_ModelsManager;
extern std::string g_ReplacePlayerModel;
extern char g_szReplacePlayerModelBuffer[64];

#endif // MODELS_MANAGER_H