// Chat Colors

#include <dbg.h>
#include <convar.h>

#include <ISvenModAPI.h>
#include <IMemoryUtils.h>
#include <IPlayerUtils.h>

#include <hl_sdk/cl_dll/hud.h>

#include "chat_colors.h"

#include "../config.h"
#include "../patterns.h"
#include "../game/utils.h"

extern CHudBaseTextBlock *g_pHudText;

//-----------------------------------------------------------------------------
// Declare hooks
//-----------------------------------------------------------------------------

DECLARE_HOOK(float *, __cdecl, GetClientColor, int);

//-----------------------------------------------------------------------------
// Vars
//-----------------------------------------------------------------------------

CChatColors g_ChatColors;

//-----------------------------------------------------------------------------
// Utilities for parsing files
//-----------------------------------------------------------------------------

#define CC_COMMENT_PREFIX ";#"
#define CC_STRIP_CHARS (" \t\n")

#define CC_STRIP_CHARS_LEN (sizeof(CC_STRIP_CHARS) - 1)
#define CC_COMMENT_PREFIX_LEN (sizeof(CC_COMMENT_PREFIX) - 1)

static int cc_contains_chars(char ch, const char *chars, size_t length)
{
	for (size_t i = 0; i < length; ++i)
	{
		if (chars[i] == ch)
			return 1;
	}

	return 0;
}

static char *cc_lstrip(char *str)
{
	while (*str && cc_contains_chars(*str, CC_STRIP_CHARS, CC_STRIP_CHARS_LEN))
		++str;

	return str;
}

static void cc_rstrip(char *str)
{
	char *end = str + strlen(str) - 1;

	if (end < str)
		return;

	while (end >= str && cc_contains_chars(*end, CC_STRIP_CHARS, CC_STRIP_CHARS_LEN))
	{
		*end = '\0';
		--end;
	}
}

static char *cc_strip(char *str)
{
	char *result = cc_lstrip(str);
	cc_rstrip(result);
	return result;
}

static void cc_remove_comment(char *str)
{
	while (*str && !cc_contains_chars(*str, CC_COMMENT_PREFIX, CC_COMMENT_PREFIX_LEN))
		++str;

	if (*str)
		*str = '\0';
}

static bool cc_is_assignment(char c)
{
	return c == ':';
}

static bool cc_is_whitespace(char c)
{
	return c == ' ' || c == '\t';
}

static bool cc_is_digit(char c)
{
	return c >= '0' && c <= '9';
}

static bool cc_is_forbidden_symbol(char c)
{
	return !cc_is_whitespace(c) && !cc_is_digit(c) && !cc_is_assignment(c);
}

//-----------------------------------------------------------------------------
// ConCommands
//-----------------------------------------------------------------------------

CON_COMMAND_EXTERN_NO_WRAPPER(sc_chat_colors_reload,
				 ConCommand_ChatColorsLoadPlayers,
				 "Loads list of Steam64 ID's from file \"sven_internal/chat_colors_players.txt\"")
{
	g_ChatColors.LoadPlayers();
}

//-----------------------------------------------------------------------------
// Hooks
//-----------------------------------------------------------------------------

DECLARE_FUNC(float *, __cdecl, HOOKED_GetClientColor, int playerIndex)
{
	if (g_Config.cvars.enable_chat_colors && playerIndex > 0)
	{
		int nTeamNumber = g_pPlayerUtils->GetTeamNumber(playerIndex);
		int *nUserColorNumber = g_ChatColors.FindPlayerInList(playerIndex);

		if (nUserColorNumber)
		{
			switch (*nUserColorNumber)
			{
			case 0:
				return g_ChatColors.GetRainbowColor();
				
			case 1:
				return g_Config.cvars.chat_color_one;

			case 2:
				return g_Config.cvars.chat_color_two;

			case 3:
				return g_Config.cvars.chat_color_three;

			case 4:
				return g_Config.cvars.chat_color_four;

			case 5:
				return g_Config.cvars.chat_color_five;

			case 6:
				return g_Config.cvars.chat_color_six;
			}
		}
		else if (nTeamNumber < 1 || nTeamNumber > 4)
		{
			return g_Config.cvars.player_name_color;
		}
	}

	return ORIG_GetClientColor(playerIndex);
}

//-----------------------------------------------------------------------------
// CChatColors implementations
//-----------------------------------------------------------------------------

CChatColors::CChatColors() : m_HashTable(15)
{
	m_pfnGetClientColor = NULL;
	m_hGetClientColor = 0;

	m_flRainbowDelta = 0.0f;

	m_flRainbowColor[0] = 1.0f;
	m_flRainbowColor[1] = 0.0f;
	m_flRainbowColor[2] = 0.0f;

	m_flRainbowUpdateTime = -1.0f;
}

void CChatColors::OnVideoInit()
{
	m_flRainbowUpdateTime = -1.0f;
}

void CChatColors::Think()
{
	UpdateRainbowColor();
}

void CChatColors::LoadPlayers()
{
	static char szBuffer[512];
	FILE *file = fopen("sven_internal/chat_colors_players.txt", "r");

	if (file)
	{
		int nLine = 0;
		token_state state = TOKEN_READ_OK;

		m_HashTable.Clear();

		while (fgets(szBuffer, sizeof(szBuffer), file))
		{
			nLine++;

			uint64_t steamID = 0;
			int colornum = 0;

			// Format >> ( STEAMID64 : COLOR_NUMBER )
			if ( (state = ReadToken(&steamID, &colornum, szBuffer)) == TOKEN_READ_OK )
			{
				if (colornum < 0 || colornum > 6)
				{
					Warning("[Chat Colors] Invalid color number!\n");
				}
				else
				{
					if ( steamID & 76561197960265728 ) // Check if we have a valid steam id
						m_HashTable.Insert(steamID, colornum);
					else
						Warning("[Chat Colors] Trying to insert invalid Steam ID!\n");
				}

			#ifdef _DEBUG
				Msg(">> %llu : %d\n", steamID, colornum);
			#endif
			}
			else if (state == TOKEN_READ_FAILED)
			{
				Warning("[Chat Colors] Failed to parse file \"sven_internal/chat_colors_players.txt\" at line %d\n", nLine);
				Warning("[Chat Colors] Follow this format >> STEAM64 ID : COLOR_NUMBER\n");
				break;
			}
		}

		if (state != TOKEN_READ_FAILED)
		{
			//Msg("[Chat Colors] Successfully parsed file 'sven_internal/chat_colors_players.txt'\n");
		}

		fclose(file);
	}
	else
	{
		Warning("[Chat Colors] Missing file \"sven_internal/chat_colors_players.txt\"\n");
	}
}

CChatColors::token_state CChatColors::ReadToken(uint64_t *steamID, int *colornum, const char *pszBuffer)
{
	int read_state = TOKEN_READ_STEAMID;

	char *buffer = cc_lstrip((char *)pszBuffer);
	cc_remove_comment(buffer);
	cc_rstrip(buffer);

	if (!*buffer) // empty
		return TOKEN_READ_EMPTY_BUFFER;

	bool bFoundSteamID = false;

	char *pszCurrentPosition = buffer;
	char *pszColorNumber = NULL;

	for (int i = 0; i < 3; i++)
	{
		if (read_state == TOKEN_READ_STEAMID)
		{
			while (*pszCurrentPosition)
			{
				if ( cc_is_forbidden_symbol(*pszCurrentPosition) )
				{
					Msg("[Chat Colors] Unrecognized symbol\n");
					return TOKEN_READ_FAILED;
				}

				if ( cc_is_whitespace(*pszCurrentPosition) || cc_is_assignment(*pszCurrentPosition) )
				{
					bool bAssignment = cc_is_assignment(*pszCurrentPosition);
					*pszCurrentPosition = 0;

					*steamID = (uint64_t)strtoll(buffer, NULL, 10);

					read_state = TOKEN_READ_ASSIGNMENT;

					if ( bAssignment )
						*pszCurrentPosition = ':';
					else
						pszCurrentPosition++;

					break;
				}

				pszCurrentPosition++;
			}

			if (!*pszCurrentPosition)
			{
				Msg("[Chat Colors] Failed to find Steam ID and an assignment symbol\n");
				goto EOF_REACHED;
			}
		}
		else if (read_state == TOKEN_READ_ASSIGNMENT)
		{
			while (*pszCurrentPosition)
			{
				if ( cc_is_forbidden_symbol(*pszCurrentPosition) )
				{
					Msg("[Chat Colors] Unrecognized symbol\n");
					return TOKEN_READ_FAILED;
				}

				if ( cc_is_digit(*pszCurrentPosition) )
				{
					Msg("[Chat Colors] Unexpectedly found a digit instead of an assignment symbol\n");
					return TOKEN_READ_FAILED;
				}

				if ( cc_is_assignment(*pszCurrentPosition) )
				{
					pszCurrentPosition++;

					read_state = TOKEN_READ_TEAM_NUMBER;

					break;
				}

				pszCurrentPosition++;
			}

			if (!*pszCurrentPosition)
			{
				Msg("[Chat Colors] Failed to find an assignment symbol and team number\n");
				goto EOF_REACHED;
			}
		}
		else if (read_state == TOKEN_READ_TEAM_NUMBER)
		{
			while (*pszCurrentPosition)
			{
				if ( cc_is_forbidden_symbol(*pszCurrentPosition) )
				{
					Msg("[Chat Colors] Unrecognized symbol\n");
					return TOKEN_READ_FAILED;
				}

				if ( cc_is_assignment(*pszCurrentPosition) )
				{
					Msg("[Chat Colors] Found repeated assignment symbol\n");
					return TOKEN_READ_FAILED;
				}

				if ( pszColorNumber && cc_is_whitespace(*pszCurrentPosition) )
				{
					Msg("[Chat Colors] Found white space symbol after team number\n");
					return TOKEN_READ_FAILED;
				}

				if ( cc_is_digit(*pszCurrentPosition) )
				{
					if (!pszColorNumber)
						pszColorNumber = pszCurrentPosition;
				}

				pszCurrentPosition++;
			}

			if (pszColorNumber)
			{
				*colornum = atoi(pszColorNumber);
				return TOKEN_READ_OK;
			}
			else
			{
				Msg("[Chat Colors] Failed to find team number\n");
				break;
			}
		}
	}

EOF_REACHED:
	Msg("[Chat Colors] Reached end of file without result\n");
	return TOKEN_READ_FAILED;
}

int *CChatColors::FindPlayerInList(int playerIndex)
{
	return m_HashTable.Find( g_pPlayerUtils->GetSteamID(playerIndex) );
}

float *CChatColors::GetRainbowColor()
{
	return m_flRainbowColor;
}

void CChatColors::UpdateRainbowColor()
{
	if (g_pEngineFuncs->GetClientTime() < m_flRainbowUpdateTime)
		return;

	HSL2RGB( m_flRainbowDelta, g_Config.cvars.chat_rainbow_saturation, g_Config.cvars.chat_rainbow_lightness, m_flRainbowColor[0], m_flRainbowColor[1], m_flRainbowColor[2] );

	m_flRainbowDelta += g_Config.cvars.chat_rainbow_hue_delta;

	while (m_flRainbowDelta > 1.0f)
		m_flRainbowDelta -= 1.0f;

	m_flRainbowUpdateTime = g_pEngineFuncs->GetClientTime() + g_Config.cvars.chat_rainbow_update_delay;
}

void CChatColors::HSL2RGB(float h, float s, float l, float &r, float &g, float &b)
{
	if (s == 0.f)
	{
		r = g = b = l;
		return;
	}

	float q = l < 0.5f ? l * (1.f + s) : l + s - l * s;
	float p = 2.f * l - q;

	r = Hue2RGB(p, q, h + (1.f / 3.f));
	g = Hue2RGB(p, q, h);
	b = Hue2RGB(p, q, h - (1.f / 3.f));
}

float CChatColors::Hue2RGB(float p, float q, float t)
{
	if (t < 0.f)
		t += 1.f;

	if (t > 1.f)
		t -= 1.f;

	if (t < 1.f / 6.f)
		return p + (q - p) * 6.f * t;

	if (t < 1.f / 2.f)
		return q;

	if (t < 2.f / 3.f)
		return p + (q - p) * ((2.f / 3.f) - t) * 6.f;

	return p;
}

//-----------------------------------------------------------------------------
// Initialize
//-----------------------------------------------------------------------------

bool CChatColors::Load()
{
	m_pfnGetClientColor = MemoryUtils()->FindPattern( SvenModAPI()->Modules()->Client, Patterns::Client::GetClientColor );

	if ( !m_pfnGetClientColor )
	{
		Warning("Couldn't find function GetClientColor\n");
		return false;
	}

	return true;
}

void CChatColors::PostLoad()
{
	m_hGetClientColor = DetoursAPI()->DetourFunction( m_pfnGetClientColor, HOOKED_GetClientColor, GET_FUNC_PTR(ORIG_GetClientColor) );

	LoadPlayers();
}

void CChatColors::Unload()
{
	DetoursAPI()->RemoveDetour( m_hGetClientColor );

	if ( g_pHudText )
	{
		// Flush the text block otherwise we will get crash because of shared color pointers
		g_pHudText->FlushText();
	}
}