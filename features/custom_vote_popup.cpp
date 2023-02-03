// Custom Vote Popup

#include <regex>

#include "custom_vote_popup.h"

#include "../game/utils.h"
#include "../game/drawing.h"

#include "../config.h"

#include <IVGUI.h>

#include <hl_sdk/engine/APIProxy.h>
#include <keydefs.h>
#include <messagebuffer.h>

#define DEBUG_VOTE_POPUP 0

//-----------------------------------------------------------------------------
// Struct declarations
//-----------------------------------------------------------------------------

struct vote_message_s
{
	wchar_t *wszMsg;
	int wide;
	int tall;
};

//-----------------------------------------------------------------------------
// Vars
//-----------------------------------------------------------------------------

CUserVotePopup g_VotePopup;

CMessageBuffer VoteMenuBuffer;

UserMsgHookFn ORIG_UserMsgHook_VoteMenu = NULL;
UserMsgHookFn ORIG_UserMsgHook_EndVote = NULL;

static const wchar_t *s_wszDefaultVoteMessage = L"Unknown vote";
static const wchar_t *s_wszDefaultVoteYes = L"F1: Vote Yes";
static const wchar_t *s_wszDefaultVoteNo = L"F2: Vote No";

static const wchar_t *s_wszVoteFormats[5] =
{
	NULL,
	L"Kill player: %s1?",
	L"Kick player: %s1?",
	L"Ban player: %s1?",
	L"Change map to %s1?"
};

//-----------------------------------------------------------------------------
// Hooked User Messages
//-----------------------------------------------------------------------------

int UserMsgHook_VoteMenu(const char *pszUserMsg, int iSize, void *pBuffer)
{
	if ( g_Config.cvars.vote_popup && g_VotePopup.OnVoteStart(pszUserMsg, iSize, pBuffer) )
		return 1;

	return ORIG_UserMsgHook_VoteMenu(pszUserMsg, iSize, pBuffer);
}

int UserMsgHook_EndVote(const char *pszUserMsg, int iSize, void *pBuffer)
{
	if ( g_VotePopup.OnVoteEnd(pszUserMsg, iSize, pBuffer) )
		return 1;

	return ORIG_UserMsgHook_EndVote(pszUserMsg, iSize, pBuffer);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

CUserVotePopup::CUserVotePopup() : m_szVoteTarget(), m_wszVoteTarget(), m_wszVoteMessage(), m_wszVoteYes(), m_wszVoteNo()
{
	m_hUserMsgHook_VoteMenu = 0;
	m_hUserMsgHook_EndVote = 0;

	m_voteType = votetype_t(-1);

	m_bVoteStarted = false;
	m_bShowPopup = false;
}

bool CUserVotePopup::Load()
{
	return true;
}

void CUserVotePopup::PostLoad()
{
	m_hUserMsgHook_VoteMenu = Hooks()->HookUserMessage( "VoteMenu", UserMsgHook_VoteMenu, &ORIG_UserMsgHook_VoteMenu );
	m_hUserMsgHook_EndVote = Hooks()->HookUserMessage( "EndVote", UserMsgHook_EndVote, &ORIG_UserMsgHook_EndVote );
}

void CUserVotePopup::Unload()
{
	Hooks()->UnhookUserMessage( m_hUserMsgHook_VoteMenu );
	Hooks()->UnhookUserMessage( m_hUserMsgHook_EndVote );
}

void CUserVotePopup::Draw()
{
	static vote_message_s rgVoteMessages[3] =
	{
		{ NULL, 0, 0 },
		{ NULL, 0, 0 },
		{ NULL, 0, 0 }
	};

	if (m_bShowPopup)
	{
		// Init output text
		if (*m_wszVoteMessage)
			rgVoteMessages[0].wszMsg = m_wszVoteMessage;
		else
			rgVoteMessages[0].wszMsg = (wchar_t *)s_wszDefaultVoteMessage;

		if (*m_wszVoteYes)
			rgVoteMessages[1].wszMsg = m_wszVoteYes;
		else
			rgVoteMessages[1].wszMsg = (wchar_t *)s_wszDefaultVoteYes;

		if (*m_wszVoteNo)
			rgVoteMessages[2].wszMsg = m_wszVoteNo;
		else
			rgVoteMessages[2].wszMsg = (wchar_t *)s_wszDefaultVoteNo;


		int nWidestText = 0, nTextTall = 0;

		int widthBase = int(g_ScreenInfo.width * g_Config.cvars.vote_popup_width_frac);
		int heightBase = int(g_ScreenInfo.height * g_Config.cvars.vote_popup_height_frac);

		int nPopupWidth = g_Config.cvars.vote_popup_width_size;
		int nPopupHeight = g_Config.cvars.vote_popup_height_size;

		// Get text size
		for (int i = 0; i < 3; i++)
		{
			vgui::surface()->GetTextSize(g_hFontVotePopup, rgVoteMessages[i].wszMsg, rgVoteMessages[i].wide, rgVoteMessages[i].tall);

			// Find widest text
			if (nWidestText < rgVoteMessages[i].wide)
				nWidestText = rgVoteMessages[i].wide;
		}

		// Distance from border of popup to text
		int nBorderWidthPixels = g_Config.cvars.vote_popup_w_border_pix;
		int nWidthShiftPixels = nBorderWidthPixels;

		// Auto size popup's width
		if (nPopupWidth - (nWidthShiftPixels + nWidestText) <= nWidthShiftPixels)
		{
			nPopupWidth = nWidestText + 2 * nWidthShiftPixels;
		}

		// Calc height offsets for texts and a line
		int nBorderHeightPixels = g_Config.cvars.vote_popup_h_border_pix;
		int nHeightShiftPixels = nBorderHeightPixels; // for m_wszVoteMessage

		int nLineHeightOffset = nHeightShiftPixels + rgVoteMessages[0].tall + int(nBorderHeightPixels * 1.5f); // increase distance between vote message and a line by 50 %

		int nVoteYesHeightOffset = nLineHeightOffset + int(nBorderHeightPixels * 2.0f) + 2;
		int nVoteNoHeightOffset = nVoteYesHeightOffset + int(nBorderHeightPixels * 2.0f) + rgVoteMessages[1].tall;

		// Auto size popup's height
		if (nPopupHeight - (nVoteNoHeightOffset + rgVoteMessages[2].tall + nBorderHeightPixels) <= nBorderHeightPixels)
		{
			nPopupHeight = nVoteNoHeightOffset + rgVoteMessages[2].tall + nBorderHeightPixels;
		}

		// Draw it finally
		g_Drawing.DrawRect(widthBase, heightBase, nPopupWidth, nPopupHeight, 0, 0, 0, 200);
		g_Drawing.DrawRect(widthBase + nWidthShiftPixels, heightBase + nLineHeightOffset, nPopupWidth - 2 * nWidthShiftPixels, 2, 100, 100, 100, 255);

		// Info message
		g_Drawing.DrawWideString(g_hFontVotePopup, widthBase + nWidthShiftPixels, heightBase + nHeightShiftPixels, 232, 232, 232, 255, FONT_ALIGN_LEFT_BOT, rgVoteMessages[0].wszMsg);

		// Yes message
		g_Drawing.DrawWideString(g_hFontVotePopup, widthBase + nWidthShiftPixels, heightBase + nVoteYesHeightOffset, 232, 232, 232, 255, FONT_ALIGN_LEFT_BOT, rgVoteMessages[1].wszMsg);

		// No message
		g_Drawing.DrawWideString(g_hFontVotePopup, widthBase + nWidthShiftPixels, heightBase + nVoteNoHeightOffset, 232, 232, 232, 255, FONT_ALIGN_LEFT_BOT, rgVoteMessages[2].wszMsg);
	}
}

void CUserVotePopup::ValidateMessage(const char **pszMessage)
{
	if ( !(**pszMessage) )
	{
		*pszMessage = NULL;
	}
	else
	{
		*pszMessage = strdup(*pszMessage);
	}
}

bool CUserVotePopup::OnVoteStart(const char *pszUserMsg, int iSize, void *pBuffer)
{
	if (m_bVoteStarted)
		return false;

	VoteMenuBuffer.Init(pBuffer, iSize, true);
	VoteMenuBuffer.BeginReading();

	wchar_t wszBuffer[256];

	const char *pszVoteMessage;
	const char *pszVoteYes;
	const char *pszVoteNo;

	// reset everything
	*m_wszVoteTarget = 0;
	*m_wszVoteMessage = 0;
	*m_wszVoteYes = 0;
	*m_wszVoteNo = 0;

	m_voteType = (votetype_t)VoteMenuBuffer.ReadByte();

	pszVoteMessage = VoteMenuBuffer.ReadString();
	ValidateMessage(&pszVoteMessage);

	pszVoteYes = VoteMenuBuffer.ReadString();
	ValidateMessage(&pszVoteYes);
	
	pszVoteNo = VoteMenuBuffer.ReadString();
	ValidateMessage(&pszVoteNo);

#if DEBUG_VOTE_POPUP
	Msg("[Vote] Started\n");
	Msg("[Vote] Type: %d\n", m_voteType);
	Msg("[Vote] Message: %s\n", pszVoteMessage);
	Msg("[Vote] Yes: %s\n", pszVoteYes);
	Msg("[Vote] No: %s\n", pszVoteNo);
#endif

	switch (m_voteType)
	{
	case VOTEKILL:
	case VOTEKICK:
	case VOTEBAN:
	case VOTEMAP:
	{
		if (pszVoteMessage)
		{
			std::cmatch match;
			std::regex regex_vote_target("\"(.*)\"");

			if (std::regex_search(pszVoteMessage, match, regex_vote_target))
			{
				strcpy_s(m_szVoteTarget, sizeof(m_szVoteTarget), match[1].str().c_str());

			#if DEBUG_VOTE_POPUP
				Msg("[Vote] Target: %s\n", m_szVoteTarget);
			#endif

				vgui::localize()->ConvertANSIToUnicode(m_szVoteTarget, m_wszVoteTarget, sizeof(m_wszVoteTarget));
				vgui::localize()->ConstructString(m_wszVoteMessage, sizeof(m_wszVoteMessage), (wchar_t *)s_wszVoteFormats[m_voteType], 1, m_wszVoteTarget);
			}
			else // just for a case
			{
				vgui::localize()->ConvertANSIToUnicode(pszVoteMessage, m_wszVoteMessage, sizeof(m_wszVoteMessage));
			}
		}

		break;
	}

	default: // custom vote
	{
		if (pszVoteMessage)
			vgui::localize()->ConvertANSIToUnicode(pszVoteMessage, m_wszVoteMessage, sizeof(m_wszVoteMessage));

		break;
	}
	}

	if (pszVoteYes)
	{
		vgui::localize()->ConvertANSIToUnicode(pszVoteYes, wszBuffer, sizeof(wszBuffer));
		vgui::localize()->ConstructString(m_wszVoteYes, sizeof(m_wszVoteYes), (wchar_t *)L"F1: %s1", 1, wszBuffer);

		m_wszVoteYes[(sizeof(m_wszVoteYes) / sizeof(wchar_t)) - 1] = 0;
	}

	if (pszVoteNo)
	{
		vgui::localize()->ConvertANSIToUnicode(pszVoteNo, wszBuffer, sizeof(wszBuffer));
		vgui::localize()->ConstructString(m_wszVoteNo, sizeof(m_wszVoteNo), (wchar_t *)L"F2: %s1", 1, wszBuffer);

		m_wszVoteNo[(sizeof(m_wszVoteNo) / sizeof(wchar_t)) - 1] = 0;
	}

	// is it really needed?
	m_wszVoteMessage[(sizeof(m_wszVoteMessage) / sizeof(wchar_t)) - 1] = 0;

	if (pszVoteMessage)
		free((void *)pszVoteMessage);
	if (pszVoteYes)
		free((void *)pszVoteYes);
	if (pszVoteNo)
		free((void *)pszVoteNo);

	m_bVoteStarted = true;
	m_bShowPopup = true;

	// Play a satisfying L4D sound
	g_pEngineFuncs->PlaySoundByName("sven_internal/beep_synthtone01.wav", 1.0f);

	return true;
}

bool CUserVotePopup::OnVoteEnd(const char *pszUserMsg, int iSize, void *pBuffer)
{
#if DEBUG_VOTE_POPUP
	Msg("[Vote] Ended\n");
#endif

	m_bVoteStarted = false;
	m_bShowPopup = false;

	return false;
}

void CUserVotePopup::OnVideoInit()
{
	m_bVoteStarted = false;
	m_bShowPopup = false;
}

void CUserVotePopup::OnKeyPress(int bKeyDown, int nKey)
{
	if (m_bShowPopup && bKeyDown)
	{
		if (nKey == K_F1)
		{
			g_pEngineFuncs->ClientCmd("voteyes\n");
			m_bShowPopup = false;

		#if DEBUG_VOTE_POPUP
			Msg("[Vote] Pressed F1\n");
		#endif
		}
		else if (nKey == K_F2)
		{
			g_pEngineFuncs->ClientCmd("voteno\n");
			m_bShowPopup = false;

		#if DEBUG_VOTE_POPUP
			Msg("[Vote] Pressed F2\n");
		#endif
		}

		if ( !m_bShowPopup )
			g_pEngineFuncs->PlaySoundByName("sven_internal/menu_click01.wav", 1.0f);
	}
}