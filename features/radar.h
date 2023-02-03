// Radar

#ifndef RADAR_H
#define RADAR_H

#pragma once

class CRadar
{
public:
	CRadar();

	void Init();
	void Draw();

private:
	int m_iRadarRoundTexture;
};

extern CRadar g_Radar;

#endif // RADAR_H