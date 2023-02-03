#ifndef CHAT_COLORS_H
#define CHAT_COLORS_H

#ifdef _WIN32
#pragma once
#endif

#include <stdint.h>

#include <IDetoursAPI.h>

#include <data_struct/hashtable.h>
#include <steamtypes.h>
#include <base_feature.h>

class CChatColors : public CBaseFeature
{
public:
	CChatColors();

	virtual bool Load();
	virtual void PostLoad();

	virtual void Unload();

public:
	void OnVideoInit();
	void Think();

	float *GetRainbowColor();

	void LoadPlayers();
	int *FindPlayerInList(int playerIndex);

private:
	enum token_state
	{
		TOKEN_READ_FAILED = -1,
		TOKEN_READ_OK = 0,
		TOKEN_READ_EMPTY_BUFFER,
		TOKEN_READ_STEAMID,
		TOKEN_READ_ASSIGNMENT,
		TOKEN_READ_TEAM_NUMBER
	};

	token_state ReadToken(uint64_t *steamID, int *teamnum, const char *pszBuffer);

private: // rainbow stuff
	void UpdateRainbowColor();

	void HSL2RGB(float h, float s, float l, /* Out: */ float &r, float &g, float &b);
	float Hue2RGB(float p, float q, float t);

private:
	CHashTable<uint64, int> m_HashTable;

	void *m_pfnGetClientColor;

	DetourHandle_t m_hGetClientColor;

	float m_flRainbowDelta;
	float m_flRainbowColor[3];
	float m_flRainbowUpdateTime;
};

extern CChatColors g_ChatColors;

#endif // CHAT_COLORS_H