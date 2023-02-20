#ifdef _WIN32
#pragma once
#endif

class CMenuColors
{
public:
	void OnVideoInit();
	void Think();

	float m_flRainbowDelta;
	float m_flRainbowColor[3];
	float m_flRainbowUpdateTime;

private: 
	void UpdateRainbowColor();

	void HSL2RGB(float h, float s, float l, /* Out: */ float& r, float& g, float& b);
	float Hue2RGB(float p, float q, float t);
};

extern CMenuColors g_MenuColors;