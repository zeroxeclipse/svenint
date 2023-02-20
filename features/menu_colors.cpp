#include <ISvenModAPI.h>
#include "menu_colors.h"
#include "../config.h"

CMenuColors g_MenuColors;

//-----------------------------------------------------------------------------
// Vars
//-----------------------------------------------------------------------------

float m_flRainbowDelta = -1.0f;
float m_flRainbowColor[3] = { 0,0,0 };
float m_flRainbowUpdateTime = -1.0f;

//-----------------------------------------------------------------------------
// Functions
//-----------------------------------------------------------------------------

void CMenuColors::OnVideoInit()
{
	m_flRainbowUpdateTime = -1.0f;
}

void CMenuColors::Think()
{
	UpdateRainbowColor();
}

void CMenuColors::UpdateRainbowColor()
{
	if (g_pEngineFuncs->GetClientTime() < m_flRainbowUpdateTime)
		return;

	HSL2RGB(m_flRainbowDelta, g_Config.cvars.menu_rainbow_saturation, g_Config.cvars.menu_rainbow_lightness, m_flRainbowColor[0], m_flRainbowColor[1], m_flRainbowColor[2]);

	m_flRainbowDelta += g_Config.cvars.menu_rainbow_hue_delta;

	while (m_flRainbowDelta > 1.0f)
		m_flRainbowDelta -= 1.0f;

	m_flRainbowUpdateTime = g_pEngineFuncs->GetClientTime() + g_Config.cvars.menu_rainbow_update_delay;
}

void CMenuColors::HSL2RGB(float h, float s, float l, float& r, float& g, float& b)
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

float CMenuColors::Hue2RGB(float p, float q, float t)
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
