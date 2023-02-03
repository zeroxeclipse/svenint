#ifndef CUSTOM_VOTE_POPUP_H
#define CUSTOM_VOTE_POPUP_H

#ifdef _WIN32
#pragma once
#endif

#include <base_feature.h>
#include <IHooks.h>

typedef enum
{
	VOTEKILL = 1,
	VOTEKICK,
	VOTEBAN,
	VOTEMAP
} votetype_t;

class CUserVotePopup : public CBaseFeature
{
public:
	CUserVotePopup();

	virtual bool Load();
	virtual void PostLoad();

	virtual void Unload();

public:
	void Draw();

	bool OnVoteStart(const char *pszUserMsg, int iSize, void *pBuffer);
	bool OnVoteEnd(const char *pszUserMsg, int iSize, void *pBuffer);

	void OnVideoInit();
	void OnKeyPress(int bKeyDown, int nKey);

private:
	void ValidateMessage(const char **pszMessage);

private:
	DetourHandle_t m_hUserMsgHook_VoteMenu;
	DetourHandle_t m_hUserMsgHook_EndVote;

	votetype_t m_voteType;

	wchar_t m_wszVoteTarget[128];
	wchar_t m_wszVoteMessage[128];
	wchar_t m_wszVoteYes[128];
	wchar_t m_wszVoteNo[128];
	
	char m_szVoteTarget[128];

	bool m_bShowPopup;
	bool m_bVoteStarted;
};

extern CUserVotePopup g_VotePopup;

#endif