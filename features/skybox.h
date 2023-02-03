#ifndef SKYBOX_H
#define SKYBOX_H

#ifdef _WIN32
#pragma once
#endif

#include <base_feature.h>
#include <IDetoursAPI.h>

class CSkybox : public CBaseFeature
{
public:
	CSkybox();

	virtual bool Load();
	virtual void PostLoad();

	virtual void Unload();

public:
	void OnConfigLoad();

	void OnVideoInit();

	void Think();

	void Replace(const char *pszSkyboxName);
	void Reset();

	void SaveOriginalSkybox(const char *pszSkyboxName);

private:
	void *m_pfnR_LoadSkyboxInt;
	DetourHandle_t m_hR_LoadSkyboxInt;

	bool m_bSkyboxReplaced;

	char m_szSkyboxName[128] = { 0 };
	char m_szCurrentSkyboxName[128] = { 0 };
	char m_szOriginalSkyboxName[128] = { 0 };

	float m_flNextThinkTime;
};

extern CSkybox g_Skybox;

#endif // SKYBOX_H