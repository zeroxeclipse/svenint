// Drawing

#pragma warning(disable : 4244)

#include <stdio.h>
#include <Windows.h>
#include <gl/GL.h>

#include <math/mathlib.h>

#include <IVGUI.h>
#include <IUtils.h>
#include <ISvenModAPI.h>

#include "utils.h"
#include "drawing.h"

//-----------------------------------------------------------------------------
// Macro definitions
//-----------------------------------------------------------------------------

//#define swap(a, b) s_swap_buffer = a; a = b; b = s_swap_buffer

//-----------------------------------------------------------------------------
// Vars
//-----------------------------------------------------------------------------

CDrawing g_Drawing;

vgui::HFont g_hFontESP;
vgui::HFont g_hFontESP2;
vgui::HFont g_hFontSpeedometer;
vgui::HFont g_hFontVotePopup;
//vgui::HFont MENU;

static char s_szBuffer[4096];
static wchar_t s_wszBuffer[4096];

static int s_swap_buffer = 0;

//-----------------------------------------------------------------------------
// CDrawing implementations
//-----------------------------------------------------------------------------

CDrawing::CDrawing()
{
	m_iNumberWidth = 0;
	m_iNumberHeight = 0;

	m_iSpriteCount = 0;
	m_pSpriteList = NULL;

	memset(m_NumberSprites, 0, sizeof(VHSPRITE) * 10);
	memset(m_NumberSpriteRects, 0, sizeof(Rect) * 10);
	memset(m_NumberSpritePointers, 0, sizeof(client_sprite_t *) * 10);
}

void CDrawing::Init()
{
	if (SvenModAPI()->GetClientState() == CLS_ACTIVE)
	{
		InitSprites();
	}
}

void CDrawing::SetupFonts()
{
	//g_pVGUI->Surface()->AddGlyphSetToFont(g_hFontESP = g_pVGUI->Surface()->CreateFont(), "Choktoff", 12, FW_BOLD, NULL, NULL, vgui::ISurface::FONTFLAG_DROPSHADOW, 0, 0);
	//g_pVGUI->Surface()->AddGlyphSetToFont(g_hFontESP2 = g_pVGUI->Surface()->CreateFont(), "Choktoff", 26, FW_SEMIBOLD, NULL, NULL, vgui::ISurface::FONTFLAG_DROPSHADOW, 0, 0);
	//g_pVGUI->Surface()->AddGlyphSetToFont(g_hFontSpeedometer = g_pVGUI->Surface()->CreateFont(), "Choktoff", 38, FW_MEDIUM, NULL, NULL, vgui::ISurface::FONTFLAG_DROPSHADOW, 0, 0);
	g_pVGUI->Surface()->AddGlyphSetToFont(g_hFontESP = g_pVGUI->Surface()->CreateFont(), "Verdana", 12, FW_BOLD, NULL, NULL, vgui::ISurface::FONTFLAG_DROPSHADOW, 0, 0);
	g_pVGUI->Surface()->AddGlyphSetToFont(g_hFontESP2 = g_pVGUI->Surface()->CreateFont(), "Verdana", 26, FW_SEMIBOLD, NULL, NULL, vgui::ISurface::FONTFLAG_DROPSHADOW, 0, 0);
	g_pVGUI->Surface()->AddGlyphSetToFont(g_hFontSpeedometer = g_pVGUI->Surface()->CreateFont(), "Verdana", 38, FW_MEDIUM, NULL, NULL, vgui::ISurface::FONTFLAG_DROPSHADOW, 0, 0);
	g_pVGUI->Surface()->AddGlyphSetToFont(g_hFontVotePopup = g_pVGUI->Surface()->CreateFont(), "Lucida-Console", 20, FW_EXTRABOLD, NULL, NULL, vgui::ISurface::FONTFLAG_NONE, 0, 0);
	//g_pSurface->AddGlyphSetToFont(MENU = g_pSurface->CreateFont(), "Arial", 14, FW_BOLD, NULL, NULL, FONTFLAG_NONE, 0, 0); //Main font
}

void CDrawing::InitSprites()
{
	static char szSpritePath[MAX_PATH];

	int iSpriteResolution = (g_pUtils->GetScreenWidth() < 640) ? 320 : 640;

	if ( !m_pSpriteList )
	{
		m_pSpriteList = g_pEngineFuncs->SPR_GetList( (char *)("sprites/hud.txt"), &m_iSpriteCount );

		if ( m_pSpriteList )
		{
			for (int i = 0; i < m_iSpriteCount; i++)
			{
				client_sprite_t *pSprite = m_pSpriteList + i;

				char num = *(pSprite->szName + 7);

				if (pSprite->iRes == iSpriteResolution && strstr(pSprite->szName, "number_") == pSprite->szName && *(pSprite->szName + 8) == '\0' && num >= '0' && num <= '9')
				{
					int digit = int(num - '0');

					m_NumberSpritePointers[digit] = pSprite;
					m_NumberSpriteRects[digit] = pSprite->rc;

					snprintf(szSpritePath, sizeof(szSpritePath), "sprites/%s.spr", pSprite->szSprite);
					m_NumberSprites[digit] = g_pEngineFuncs->SPR_Load( szSpritePath );

					if ( !digit )
					{
						m_iNumberWidth = pSprite->rc.right - pSprite->rc.left;
						m_iNumberHeight = pSprite->rc.bottom - pSprite->rc.top;
					}
				}
			}
		}
	}
	else
	{
		for (int i = 0; i < 10; i++)
		{
			client_sprite_t *pSprite = m_NumberSpritePointers[i];

			snprintf(szSpritePath, sizeof(szSpritePath), "sprites/%s.spr", pSprite->szSprite);
			m_NumberSprites[i] = g_pEngineFuncs->SPR_Load( szSpritePath );
		}
	}
}

void CDrawing::OnVideoInit()
{
	InitSprites();
}

//-----------------------------------------------------------------------------
// Sprite handlers
//-----------------------------------------------------------------------------

int CDrawing::DrawDigit(int digit, int x, int y, int r, int g, int b)
{
	if (digit >= 0 && digit <= 9)
	{
		g_pEngineFuncs->SPR_Set( m_NumberSprites[digit], r, g, b );
		g_pEngineFuncs->SPR_DrawAdditive( 0, x, y, &m_NumberSpriteRects[digit] );
	}

	return m_iNumberWidth;
}

int CDrawing::DrawDigit(int digit, int x, int y, int r, int g, int b, FontAlignFlags_t alignment)
{
	ApplyTextAlignment(alignment, x, y, m_iNumberWidth, m_iNumberHeight);
	return DrawDigit(digit, x, y, r, g, b);
}

int CDrawing::DrawNumber(int number, int x, int y, int r, int g, int b, FontAlignFlags_t alignment, int fieldMinWidth /* = 1 */)
{
	bool bNegative = false;

	if (number < 0)
	{
		if (number == -2147483648)
		{
			number = 0;
		}
		else
		{
			number = abs(number);
			bNegative = true;
		}
	}

	int i;
	int c = 0;
	int digits[10] = { 0 };

	for (i = 0; i < 10; ++i)
	{
		if ( !number )
			break;

		digits[i] = number % 10;
		number /= 10;

		c++;
	}

	ApplyTextAlignment(alignment, x, y, (fieldMinWidth >= c ? fieldMinWidth : c) * m_iNumberWidth, m_iNumberHeight);

	if (bNegative)
	{
		DrawLine(x - m_iNumberWidth, y + m_iNumberHeight / 2, x, y + m_iNumberHeight / 2, r, g, b, 255);
	}

	for (; fieldMinWidth > 10; --fieldMinWidth)
	{
		DrawDigit(0, x, y, r, g, b);
		x += m_iNumberWidth;
	}

	if (fieldMinWidth > i)
		i = fieldMinWidth;

	for (int j = i; j > 0; --j)
	{
		DrawDigit(digits[j - 1], x, y, r, g, b);
		x += m_iNumberWidth;
	}

	return x;
}

int CDrawing::GetNumberSpriteWidth()
{
	return m_iNumberWidth;
}

int CDrawing::GetNumberSpriteHeight()
{
	return m_iNumberHeight;
}

//-----------------------------------------------------------------------------
// Figures
//-----------------------------------------------------------------------------

void CDrawing::DrawCircle3D(Vector &position, float points, float radius, int r, int g, int b, int a)
{
	float step = (float)M_PI * 2.0f / points;

	for (float a = 0; a < (M_PI * 2.0f); a += step)
	{
		Vector start(radius * cosf(a) + position.x, radius * sinf(a) + position.y, position.z);
		Vector end(radius * cosf(a + step) + position.x, radius * sinf(a + step) + position.y, position.z);

		Vector start2d, end2d;
		if ( !UTIL_WorldToScreen(start, start2d) || !UTIL_WorldToScreen(end, end2d) )
			return;

		DrawLine(start2d.x, start2d.y, end2d.x, end2d.y, r, g, b, a);
	}
}

void CDrawing::DrawCircle(float position[2], float points, float radius, int r, int g, int b, int a)
{
	float step = (float)M_PI * 2.0f / points;

	float start[2], end[2];

	for (float x = 0; x < (M_PI * 2.0f); x += step)
	{
		start[0] = radius * cosf(x) + position[0];
		start[1] = radius * sinf(x) + position[1];
		end[0] = radius * cosf(x + step) + position[0];
		end[1] = radius * sinf(x + step) + position[1];
		DrawLine(start[0], start[1], end[0], end[1], r, g, b, a);
	}
}

void CDrawing::DrawPlayerBox(int x, int y, int w, int h, int r, int g, int b, int a)
{
	int x0 = x - w;
	int y0 = y;
	int x1 = x + w;
	int y1 = y + h;

	//if (x1 < x0)
	//	swap(x1, x0);

	//if (y1 < y0)
	//	swap(y1, y0);

	g_pVGUI->Surface()->DrawSetColor(r, g, b, a);
	g_pVGUI->Surface()->DrawOutlinedRect(x0, y0, x1, y1);

	//if (cvar.esp_box_outline) 
	if (1) 
	{
		g_pVGUI->Surface()->DrawSetColor(0, 0, 0, a);
		g_pVGUI->Surface()->DrawOutlinedRect(x0 - 1, y0 - 1, x1 + 1, y1 + 1);
		g_pVGUI->Surface()->DrawOutlinedRect(x0 + 1, y0 + 1, x1 - 1, y1 - 1);
	}
}

void CDrawing::DrawRect(int x, int y, int w, int h, int r, int g, int b, int a)
{
	int x0 = x;
	int y0 = y;
	int x1 = x + w;
	int y1 = y + h;

	//if (x1 < x0)
	//	swap(x1, x0);

	//if (y1 < y0)
	//	swap(y1, y0);

	g_pVGUI->Surface()->DrawSetColor(r, g, b, a);
	g_pVGUI->Surface()->DrawFilledRect(x0, y0, x1, y1);
}

void CDrawing::DrawOutlinedRect(int x, int y, int w, int h, int r, int g, int b, int a)
{
	int x0 = x;
	int y0 = y;
	int x1 = x + w;
	int y1 = y + h;

	//if (x1 < x0)
	//	swap(x1, x0);

	//if (y1 < y0)
	//	swap(y1, y0);

	g_pVGUI->Surface()->DrawSetColor(r, g, b, a);
	g_pVGUI->Surface()->DrawOutlinedRect(x0, y0, x1, y1);
}

void CDrawing::DrawLine(int x0, int y0, int x1, int y1, int r, int g, int b, int a)
{
	g_pVGUI->Surface()->DrawSetTextColor(r, g, b, a);
	g_pVGUI->Surface()->DrawLine(x0, y0, x1, y1);
}

void CDrawing::FillArea(int x, int y, int w, int h, int r, int g, int b, int a)
{
	glPushMatrix();

		glLoadIdentity();

		glDisable(GL_TEXTURE_2D);
		glEnable(GL_BLEND);

		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glColor4ub(r, g, b, a);

		glBegin(GL_QUADS);
			glVertex2i(x, y);
			glVertex2i(x + w, y);
			glVertex2i(x + w, y + h);
			glVertex2i(x, y + h);
		glEnd();

		glDisable(GL_BLEND);
		glEnable(GL_TEXTURE_2D);

	glPopMatrix();
}

void CDrawing::DrawBox(int x, int y, int w, int h, int r, int g, int b, int a)
{
	g_pVGUI->Surface()->DrawSetColor(r, g, b, a);
	g_pVGUI->Surface()->DrawOutlinedRect(x - w, y - h, x + w, y + h);
}

void CDrawing::Box(int x, int y, int w, int h, int lw, int r, int g, int b, int a)
{
	FillArea(x, y, w, lw, r, g, b, a);
	FillArea(x + w - lw, y + lw, lw, h - lw, r, g, b, a);
	FillArea(x, y + lw, lw, h - lw, r, g, b, a);
	FillArea(x + lw, y + h - lw, w - lw * 2, lw, r, g, b, a);
}

void CDrawing::BoxOutline(float x, float y, float w, float h, float lw, BYTE r, BYTE g, BYTE b, BYTE a)
{
	Box(x, y, w, h, lw, r, g, b, a);
	Box(x - 1, y - 1, w + 2, h + 2, 1, 0, 0, 0, a);
	Box(x + lw, y + lw, w - lw * 2, h - lw * 2, 1, 0, 0, 0, a);
}

void CDrawing::DrawCoalBox(int x, int y, int w, int h, int lw, int r, int g, int b, int a)
{
	int iw = w / 4;
	int ih = h / 1;

	DrawLine(x, y, x + iw, y, r, g, b, a);			// left
	DrawLine(x + w - iw, y, x + w, y, r, g, b, a);			// right
	DrawLine(x, y, x, y + ih, r, g, b, a);				// top left
	DrawLine(x + w, y, x + w, y + ih, r, g, b, a);	// top right
													// bottom
	DrawLine(x, y + h, x + iw, y + h, r, g, b, a);			// left
	DrawLine(x + w - iw, y + h, x + w, y + h, r, g, b, a);	// right
	DrawLine(x, y + h - ih, x, y + h, r, g, b, a);			// bottom left
	DrawLine(x + w, y + h - ih, x + w, y + h, r, g, b, a);	// bottom right
}

void CDrawing::DrawOutlineCoalBox(int x, int y, int w, int h, int  lw, BYTE r, BYTE g, BYTE b, BYTE a)
{
	int iw = w / 4;
	int ih = h / 4;

	DrawCoalBox(x - 1, y - 1, w, h, lw, 0, 0, 0, a);
	DrawCoalBox(x + 1, y + 1, w, h, lw, 0, 0, 0, a);
	DrawCoalBox(x - 1, y - 1, w, h, lw, 0, 0, 0, a);
	DrawCoalBox(x + 1, y + 1, w, h, lw, 0, 0, 0, a);

	DrawCoalBox(x - 1, y - 1, w, h, lw, 0, 0, 0, a);
	DrawCoalBox(x + 1, y + 1, w, h, lw, 0, 0, 0, a);
	DrawCoalBox(x - 1, y - 1, w, h, lw, 0, 0, 0, a);
	DrawCoalBox(x + 1, y + 1, w, h, lw, 0, 0, 0, a);


	DrawCoalBox(x, y, w, h, iw, r, g, b, a);
}

void CDrawing::BoxCorner(int x, int y, int w, int h, int lw, BYTE r, BYTE g, BYTE b, BYTE a)
{
	FillArea(x, y, w / 4, lw, r, g, b, a);
	FillArea(x + w - w / 4, y, w / 4, lw, r, g, b, a);
	FillArea(x, y + lw, lw, h / 4 - lw, r, g, b, a);
	FillArea(x, y + h - h / 4, lw, h / 4, r, g, b, a);
	FillArea(x + w - lw, y + lw, lw, h / 4 - lw, r, g, b, a);
	FillArea(x + w - lw, y + h - h / 4, lw, h / 4, r, g, b, a);
	FillArea(x + lw, y + h - lw, w / 4 - lw, lw, r, g, b, a);
	FillArea(x + w - w / 4, y + h - lw, w / 4 - lw, lw, r, g, b, a);
}

void CDrawing::BoxCornerOutline(int x, int y, int w, int h, int lw, BYTE r, BYTE g, BYTE b, BYTE a)
{
	BoxCorner(x - 1, y + 1, w, h, lw, 0, 0, 0, a);
	BoxCorner(x - 1, y - 1, w, h, lw, 0, 0, 0, a);
	BoxCorner(x + 1, y + 1, w, h, lw, 0, 0, 0, a);
	BoxCorner(x + 1, y - 1, w, h, lw, 0, 0, 0, a);

	BoxCorner(x, y, w, h, lw, r, g, b, a);
}

//-----------------------------------------------------------------------------
// Draw a texture
//-----------------------------------------------------------------------------

void CDrawing::DrawTexture(int id, int x0, int y0, int x1, int y1, int r, int g, int b, int a)
{
	g_pVGUI->Surface()->DrawSetColor(r, g, b, a);
	g_pVGUI->Surface()->DrawSetTexture(id);
	g_pVGUI->Surface()->DrawTexturedRect(x0, y0, x1, y1);
}

//-----------------------------------------------------------------------------
// Draw a formatted string
//-----------------------------------------------------------------------------

void CDrawing::DrawStringF(vgui::HFont font, int x, int y, int r, int g, int b, int a, FontAlignFlags_t alignment, const char *pszString, ...)
{
	va_list va_alist;
	va_start(va_alist, pszString);
	vsnprintf(s_szBuffer, sizeof(s_szBuffer), pszString, va_alist);
	va_end(va_alist);
	MultiByteToWideChar(CP_UTF8, 0, s_szBuffer, 256, s_wszBuffer, 256);

	int width, height;
	g_pVGUI->Surface()->GetTextSize(font, s_wszBuffer, width, height);

	ApplyTextAlignment(alignment, x, y, width, height);

	g_pVGUI->Surface()->DrawSetTextFont(font);
	g_pVGUI->Surface()->DrawSetTextColor(r, g, b, a);
	g_pVGUI->Surface()->DrawSetTextPos(x, y - height / 2);
	g_pVGUI->Surface()->DrawPrintText(s_wszBuffer, wcslen(s_wszBuffer));
}

void CDrawing::DrawStringExF(vgui::HFont font, int x, int y, int r, int g, int b, int a, int &width, int &height, FontAlignFlags_t alignment, const char *pszString, ...)
{
	va_list va_alist;
	va_start(va_alist, pszString);
	vsnprintf(s_szBuffer, sizeof(s_szBuffer), pszString, va_alist);
	va_end(va_alist);
	MultiByteToWideChar(CP_UTF8, 0, s_szBuffer, 256, s_wszBuffer, 256);

	g_pVGUI->Surface()->GetTextSize(font, s_wszBuffer, width, height);

	ApplyTextAlignment(alignment, x, y, width, height);

	g_pVGUI->Surface()->DrawSetTextFont(font);
	g_pVGUI->Surface()->DrawSetTextColor(r, g, b, a);
	g_pVGUI->Surface()->DrawSetTextPos(x, y - height / 2);
	g_pVGUI->Surface()->DrawPrintText(s_wszBuffer, wcslen(s_wszBuffer));
}

void CDrawing::DrawStringACPF(vgui::HFont font, int x, int y, int r, int g, int b, int a, FontAlignFlags_t alignment, const char *pszString, ...)
{
	va_list va_alist;
	va_start(va_alist, pszString);
	vsnprintf(s_szBuffer, sizeof(s_szBuffer), pszString, va_alist);
	va_end(va_alist);
	MultiByteToWideChar(CP_ACP, 0, s_szBuffer, 256, s_wszBuffer, 256);

	int width, height;
	g_pVGUI->Surface()->GetTextSize(font, s_wszBuffer, width, height);

	ApplyTextAlignment(alignment, x, y, width, height);

	g_pVGUI->Surface()->DrawSetTextFont(font);
	g_pVGUI->Surface()->DrawSetTextColor(r, g, b, a);
	g_pVGUI->Surface()->DrawSetTextPos(x, y - height / 2);
	g_pVGUI->Surface()->DrawPrintText(s_wszBuffer, wcslen(s_wszBuffer));
}

void CDrawing::DrawWideStringF(vgui::HFont font, int x, int y, int r, int g, int b, int a, FontAlignFlags_t alignment, const wchar_t *pwszString, ...)
{
	va_list va_alist;
	va_start(va_alist, pwszString);
	vswprintf(s_wszBuffer, sizeof(s_wszBuffer) / sizeof(wchar_t), pwszString, va_alist);
	va_end(va_alist);

	int width, height;
	g_pVGUI->Surface()->GetTextSize(font, s_wszBuffer, width, height);

	ApplyTextAlignment(alignment, x, y, width, height);

	g_pVGUI->Surface()->DrawSetTextFont(font);
	g_pVGUI->Surface()->DrawSetTextColor(r, g, b, a);
	g_pVGUI->Surface()->DrawSetTextPos(x, y - height / 2);
	g_pVGUI->Surface()->DrawPrintText(s_wszBuffer, wcslen(s_wszBuffer));
}

//-----------------------------------------------------------------------------
// Draw a string
//-----------------------------------------------------------------------------

void CDrawing::DrawString(vgui::HFont font, int x, int y, int r, int g, int b, int a, FontAlignFlags_t alignment, const char *pszString)
{
	MultiByteToWideChar(CP_UTF8, 0, pszString, 256, s_wszBuffer, 256);

	int width, height;
	g_pVGUI->Surface()->GetTextSize(font, s_wszBuffer, width, height);

	ApplyTextAlignment(alignment, x, y, width, height);

	g_pVGUI->Surface()->DrawSetTextFont(font);
	g_pVGUI->Surface()->DrawSetTextColor(r, g, b, a);
	g_pVGUI->Surface()->DrawSetTextPos(x, y - height / 2);
	g_pVGUI->Surface()->DrawPrintText(s_wszBuffer, wcslen(s_wszBuffer));
}

void CDrawing::DrawStringEx(vgui::HFont font, int x, int y, int r, int g, int b, int a, int &width, int &height, FontAlignFlags_t alignment, const char *pszString)
{
	MultiByteToWideChar(CP_UTF8, 0, pszString, 256, s_wszBuffer, 256);

	g_pVGUI->Surface()->GetTextSize(font, s_wszBuffer, width, height);

	ApplyTextAlignment(alignment, x, y, width, height);

	g_pVGUI->Surface()->DrawSetTextFont(font);
	g_pVGUI->Surface()->DrawSetTextColor(r, g, b, a);
	g_pVGUI->Surface()->DrawSetTextPos(x, y - height / 2);
	g_pVGUI->Surface()->DrawPrintText(s_wszBuffer, wcslen(s_wszBuffer));
}

void CDrawing::DrawStringACP(vgui::HFont font, int x, int y, int r, int g, int b, int a, FontAlignFlags_t alignment, const char *pszString)
{
	MultiByteToWideChar(CP_ACP, 0, pszString, 256, s_wszBuffer, 256);

	int width, height;
	g_pVGUI->Surface()->GetTextSize(font, s_wszBuffer, width, height);

	ApplyTextAlignment(alignment, x, y, width, height);

	g_pVGUI->Surface()->DrawSetTextFont(font);
	g_pVGUI->Surface()->DrawSetTextColor(r, g, b, a);
	g_pVGUI->Surface()->DrawSetTextPos(x, y - height / 2);
	g_pVGUI->Surface()->DrawPrintText(s_wszBuffer, wcslen(s_wszBuffer));
}


void CDrawing::DrawWideString(vgui::HFont font, int x, int y, int r, int g, int b, int a, FontAlignFlags_t alignment, const wchar_t *pwszString)
{
	int width, height;
	g_pVGUI->Surface()->GetTextSize(font, pwszString, width, height);

	ApplyTextAlignment(alignment, x, y, width, height);

	g_pVGUI->Surface()->DrawSetTextFont(font);
	g_pVGUI->Surface()->DrawSetTextColor(r, g, b, a);
	g_pVGUI->Surface()->DrawSetTextPos(x, y - height / 2);
	g_pVGUI->Surface()->DrawPrintText(pwszString, wcslen(pwszString));
}

//-----------------------------------------------------------------------------
// Misc. draw methods
//-----------------------------------------------------------------------------

void CDrawing::DrawCrosshair(int x, int y, int r, int g, int b, int a, int iSize /* = 10 */, int iGap /* = 4 */, int iThickness /* = 2 */)
{
	int thickness = iThickness - 1;

	// Left
	g_Drawing.FillArea(x - iGap - iSize + 1, y - (thickness / 2), iSize, iThickness, r, g, b, a);

	// Right
	g_Drawing.FillArea(x + iGap, y - (thickness / 2), iSize, iThickness, r, g, b, a);

	// Up
	g_Drawing.FillArea(x - (thickness / 2), y - iGap - iSize + 1, iThickness, iSize, r, g, b, a);

	// Down
	g_Drawing.FillArea(x - (thickness / 2), y + iGap, iThickness, iSize, r, g, b, a);
}

void CDrawing::DrawDot(int x, int y, int r, int g, int b, int a, int iThickness /* = 2 */)
{
	int thickness = iThickness - 1;

	g_Drawing.FillArea(x - (thickness / 2), y - (thickness / 2), iThickness, iThickness, r, g, b, a);
}

void CDrawing::DrawCrosshairShadow(int x, int y, int r, int g, int b, int a, int iSize /* = 10 */, int iGap /* = 4 */, int iThickness /* = 2 */, int iShadowThickness /* = 1 */)
{
	int thickness = iThickness - 1;

	// Left
	g_Drawing.FillArea(x - iGap - iSize + 1 - iShadowThickness + (iShadowThickness / 2),
					   y - (thickness / 2) - iShadowThickness + (iShadowThickness / 2),
					   iSize + iShadowThickness,
					   iThickness + iShadowThickness,
					   r, g, b, a);

	// Right
	g_Drawing.FillArea(x + iGap - iShadowThickness + (iShadowThickness / 2),
					   y - (thickness / 2) - iShadowThickness + (iShadowThickness / 2),
					   iSize + iShadowThickness,
					   iThickness + iShadowThickness,
					   r, g, b, a);

	// Up
	g_Drawing.FillArea(x - (thickness / 2) - iShadowThickness + (iShadowThickness / 2),
					   y - iGap - iSize + 1 - iShadowThickness + (iShadowThickness / 2),
					   iThickness + iShadowThickness,
					   iSize + iShadowThickness,
					   r, g, b, a);

	// Down
	g_Drawing.FillArea(x - (thickness / 2) - iShadowThickness + (iShadowThickness / 2),
					   y + iGap - iShadowThickness + (iShadowThickness / 2),
					   iThickness + iShadowThickness,
					   iSize + iShadowThickness,
					   r, g, b, a);
}

void CDrawing::DrawDotShadow(int x, int y, int r, int g, int b, int a, int iThickness /* = 2 */, int iShadowThickness /* = 1 */)
{
	int thickness = iThickness - 1;

	g_Drawing.FillArea(x - (thickness / 2) - iShadowThickness + (iShadowThickness / 2),
					   y - (thickness / 2) - iShadowThickness + (iShadowThickness / 2),
					   iThickness + iShadowThickness,
					   iThickness + iShadowThickness,
					   r, g, b, a);
}