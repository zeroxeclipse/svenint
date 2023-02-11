// Custom Vote Popup

#include <regex>

#include "custom_vote_popup.h"

#include "../game/utils.h"
#include "../game/drawing.h"

#include "../config.h"

#include <IVGUI.h>

#include <hl_sdk/engine/APIProxy.h>
#include <keydefs.h>
#include <dbg.h>
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

static const wchar_t *s_wszUndefined = L"N/A";

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

static const wchar_t *s_wszVoteKeys[256] =
{
	s_wszUndefined, // 0
	s_wszUndefined, // 1
	s_wszUndefined, // 2
	s_wszUndefined, // 3
	s_wszUndefined, // 4
	s_wszUndefined, // 5
	s_wszUndefined, // 6
	s_wszUndefined, // 7
	s_wszUndefined, // 8
	L"TAB", // 9
	s_wszUndefined, // 10
	s_wszUndefined, // 11
	s_wszUndefined, // 12
	L"Enter", // 13
	s_wszUndefined, // 14
	s_wszUndefined, // 15
	s_wszUndefined, // 16
	s_wszUndefined, // 17
	s_wszUndefined, // 18
	s_wszUndefined, // 19
	s_wszUndefined, // 20
	s_wszUndefined, // 21
	s_wszUndefined, // 22
	s_wszUndefined, // 23
	s_wszUndefined, // 24
	s_wszUndefined, // 25
	s_wszUndefined, // 26
	L"Esc", // 27
	s_wszUndefined, // 28
	s_wszUndefined, // 29
	s_wszUndefined, // 30
	s_wszUndefined, // 31
	L"Space", // 32
	s_wszUndefined, // 33
	s_wszUndefined, // 34
	s_wszUndefined, // 35
	s_wszUndefined, // 36
	s_wszUndefined, // 37
	s_wszUndefined, // 38
	s_wszUndefined, // 39
	s_wszUndefined, // 40
	s_wszUndefined, // 41
	s_wszUndefined, // 42
	s_wszUndefined, // 43
	s_wszUndefined, // 44
	s_wszUndefined, // 45
	s_wszUndefined, // 46
	s_wszUndefined, // 47
	L"0", // 48
	L"1", // 49
	L"2", // 50
	L"3", // 51
	L"4", // 52
	L"5", // 53
	L"6", // 54
	L"7", // 55
	L"8", // 56
	L"9", // 57
	s_wszUndefined, // 58
	s_wszUndefined, // 59
	s_wszUndefined, // 60
	s_wszUndefined, // 61
	s_wszUndefined, // 62
	s_wszUndefined, // 63
	s_wszUndefined, // 64
	L"A", // 65
	L"B", // 66
	L"C", // 67
	L"D", // 68
	L"E", // 69
	L"F", // 70
	L"G", // 71
	L"H", // 72
	L"I", // 73
	L"J", // 74
	L"K", // 75
	L"L", // 76
	L"M", // 77
	L"N", // 78
	L"O", // 79
	L"P", // 80
	L"Q", // 81
	L"R", // 82
	L"S", // 83
	L"T", // 84
	L"U", // 85
	L"V", // 86
	L"W", // 87
	L"X", // 88
	L"Y", // 89
	L"Z", // 90
	s_wszUndefined, // 91
	s_wszUndefined, // 92
	s_wszUndefined, // 93
	s_wszUndefined, // 94
	s_wszUndefined, // 95
	s_wszUndefined, // 96
	L"a", // 97
	L"b", // 98
	L"c", // 99
	L"d", // 100
	L"e", // 101
	L"f", // 102
	L"g", // 103
	L"h", // 104
	L"i", // 105
	L"j", // 106
	L"k", // 107
	L"l", // 108
	L"m", // 109
	L"n", // 110
	L"o", // 111
	L"p", // 112
	L"q", // 113
	L"r", // 114
	L"s", // 115
	L"t", // 116
	L"u", // 117
	L"v", // 118
	L"w", // 119
	L"x", // 120
	L"y", // 121
	L"z", // 122
	s_wszUndefined, // 123
	s_wszUndefined, // 124
	s_wszUndefined, // 125
	s_wszUndefined, // 126
	L"Backspace", // 127
	L"Up Arrow", // 128
	L"Down Arrow", // 129
	L"Left Arrow", // 130
	L"Right Arrow", // 131
	L"Alt", // 132
	L"Ctrl", // 133
	L"Shift", // 134
	L"F1", // 135
	L"F2", // 136
	L"F3", // 137
	L"F4", // 138
	L"F5", // 139
	L"F6", // 140
	L"F7", // 141
	L"F8", // 142
	L"F9", // 143
	L"F10", // 144
	L"F11", // 145
	L"F12", // 146
	L"Insert", // 147
	L"Delete", // 148
	L"Page Down", // 149
	L"Page Up", // 150
	L"Home", // 151
	L"End", // 152
	s_wszUndefined, // 153
	s_wszUndefined, // 154
	s_wszUndefined, // 155
	s_wszUndefined, // 156
	s_wszUndefined, // 157
	s_wszUndefined, // 158
	s_wszUndefined, // 159
	L"Numpad Home", // 160
	L"Numpad Up Arrow", // 161
	L"Numpad Page Up", // 162
	L"Numpad Left Arrow", // 163
	L"Numpad 5", // 164
	L"Numpad Right Arrow", // 165
	L"Numpad End", // 166
	L"Numpad Down Arrow", // 167
	L"Numpad Page Down", // 168
	L"Numpad Enter", // 169
	L"Numpad Insert", // 170
	L"Numpad Delete", // 171
	L"Numpad Slash", // 172
	L"Numpad Minus", // 173
	L"Numpad Plus", // 174
	L"Numpad Capslock", // 175
	L"Numpad Multiply", // 176
	L"Numpad Win", // 177
	s_wszUndefined, // 178
	s_wszUndefined, // 179
	s_wszUndefined, // 180
	s_wszUndefined, // 181
	s_wszUndefined, // 182
	s_wszUndefined, // 183
	s_wszUndefined, // 184
	s_wszUndefined, // 185
	s_wszUndefined, // 186
	s_wszUndefined, // 187
	s_wszUndefined, // 188
	s_wszUndefined, // 189
	s_wszUndefined, // 190
	s_wszUndefined, // 191
	s_wszUndefined, // 192
	s_wszUndefined, // 193
	s_wszUndefined, // 194
	s_wszUndefined, // 195
	s_wszUndefined, // 196
	s_wszUndefined, // 197
	s_wszUndefined, // 198
	s_wszUndefined, // 199
	s_wszUndefined, // 200
	s_wszUndefined, // 201
	s_wszUndefined, // 202
	L"Joy 1", // 203
	L"Joy 2", // 204
	L"Joy 3", // 205
	L"Joy 4", // 206
	L"Aux 1", // 207
	L"Aux 2", // 208
	L"Aux 3", // 209
	L"Aux 4", // 210
	L"Aux 5", // 211
	L"Aux 6", // 212
	L"Aux 7", // 213
	L"Aux 8", // 214
	L"Aux 9", // 215
	L"Aux 10", // 216
	L"Aux 11", // 217
	L"Aux 12", // 218
	L"Aux 13", // 219
	L"Aux 14", // 220
	L"Aux 15", // 221
	L"Aux 16", // 222
	L"Aux 17", // 223
	L"Aux 18", // 224
	L"Aux 19", // 225
	L"Aux 20", // 226
	L"Aux 21", // 227
	L"Aux 22", // 228
	L"Aux 23", // 229
	L"Aux 24", // 230
	L"Aux 25", // 231
	L"Aux 26", // 232
	L"Aux 27", // 233
	L"Aux 28", // 234
	L"Aux 29", // 235
	L"Aux 30", // 236
	L"Aux 31", // 237
	L"Aux 32", // 238
	L"Mouse Wheel Down", // 239
	L"Mouse Wheel Up", // 240
	s_wszUndefined, // 241
	s_wszUndefined, // 242
	s_wszUndefined, // 243
	s_wszUndefined, // 244
	s_wszUndefined, // 245
	s_wszUndefined, // 246
	s_wszUndefined, // 247
	s_wszUndefined, // 248
	s_wszUndefined, // 249
	s_wszUndefined, // 250
	s_wszUndefined, // 251
	s_wszUndefined, // 252
	s_wszUndefined, // 253
	s_wszUndefined, // 254
	L"Pause", // 255
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

	if ( m_bShowPopup )
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
		if ( pszVoteMessage )
		{
			std::cmatch match;
			std::regex regex_vote_target("\"(.*)\"");

			if ( std::regex_search(pszVoteMessage, match, regex_vote_target) )
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
		if ( pszVoteMessage )
			vgui::localize()->ConvertANSIToUnicode(pszVoteMessage, m_wszVoteMessage, sizeof(m_wszVoteMessage));

		break;
	}
	}

	if ( pszVoteYes )
	{
		int iYesKey = g_Config.cvars.vote_popup_yes_key;

		AssertFatal( iYesKey >= 0 && iYesKey <= 255 );

		vgui::localize()->ConvertANSIToUnicode(pszVoteYes, wszBuffer, sizeof(wszBuffer));
		vgui::localize()->ConstructString(m_wszVoteYes, sizeof(m_wszVoteYes), (wchar_t *)L"%s1: %s2", 2, s_wszVoteKeys[iYesKey], wszBuffer);

		m_wszVoteYes[M_ARRAYSIZE(m_wszVoteYes) - 1] = 0;
	}

	if ( pszVoteNo )
	{
		int iNoKey = g_Config.cvars.vote_popup_no_key;

		AssertFatal( iNoKey >= 0 && iNoKey <= 255 );

		vgui::localize()->ConvertANSIToUnicode(pszVoteNo, wszBuffer, sizeof(wszBuffer));
		vgui::localize()->ConstructString(m_wszVoteNo, sizeof(m_wszVoteNo), (wchar_t *)L"%s1: %s2", 2, s_wszVoteKeys[iNoKey], wszBuffer);

		m_wszVoteNo[M_ARRAYSIZE(m_wszVoteNo) - 1] = 0;
	}

	// is it really needed?
	m_wszVoteMessage[M_ARRAYSIZE(m_wszVoteMessage) - 1] = 0;

	if ( pszVoteMessage )
		free((void *)pszVoteMessage);
	if ( pszVoteYes )
		free((void *)pszVoteYes);
	if ( pszVoteNo )
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
	if ( m_bShowPopup && bKeyDown )
	{
		if ( nKey == g_Config.cvars.vote_popup_yes_key )
		{
			g_pEngineFuncs->ClientCmd("voteyes\n");
			m_bShowPopup = false;

		#if DEBUG_VOTE_POPUP
			Msg("[Vote] Pressed F1\n");
		#endif
		}
		else if ( nKey == g_Config.cvars.vote_popup_no_key )
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