// Radar

#ifndef RADAR_H
#define RADAR_H

#pragma once

#include <base_feature.h>

class CRadar : public CBaseFeature
{
public:
	CRadar();

	virtual void PostLoad();

	virtual void Unload();

	void Draw();

private:
	int m_hRadarRoundTexture;
};

extern CRadar g_Radar;

#endif // RADAR_H