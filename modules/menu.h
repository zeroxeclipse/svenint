#ifndef MENU_MODULE_H
#define MENU_MODULE_H

#ifdef _WIN32
#pragma once
#endif

#include <base_feature.h>

#include <IDetoursAPI.h>
#include <IMemoryUtils.h>

extern int g_iMenuState;

extern bool g_bMenuEnabled;
extern bool g_bMenuClosed;

extern float g_bMenuOpenTime;
extern float g_bMenuCloseTime;
extern float g_bMenuOpenTimePrev;
extern float g_bMenuCloseTimePrev;

//-----------------------------------------------------------------------------
// Menu feature
//-----------------------------------------------------------------------------

class CMenuModule : public CBaseFeature
{
public:
	CMenuModule();

	virtual bool Load();
	virtual void PostLoad();

	virtual void Unload();

public:
	void Draw();

private:
	void DrawLogo();
	void DrawMainTabs();
	void DrawMenuImage();
	void DrawStats();

	void DrawVisualsSubTabs();
	void DrawHUDSubTabs();
	void DrawUtilitySubTabs();
	void DrawConfigsSubTabs();
	void DrawSettingsSubTabs();

	void DrawVisualsTabContent();
	void DrawHUDTabContent();
	void DrawUtilityTabContent();
	void DrawTabConfigsContent();
	void DrawSettingsTabContent();

private:
	void *m_pfnwglSwapBuffers;
	void *m_pfnSetCursorPos;

	DetourHandle_t m_hwglSwapBuffers;
	DetourHandle_t m_hSetCursorPos;

	bool m_bThemeLoaded;

	bool m_bMenuSettings;
	bool m_bMenuConfig;

	bool m_bMenuAim;
	bool m_bMenuVisuals;
	bool m_bMenuHud;
	bool m_bMenuUtility;
};

extern CMenuModule g_MenuModule;

#endif // MENU_MODULE_H

//-----------------------------------------------------------------------------
// Menu obfuscation
//-----------------------------------------------------------------------------

class obfuscated_string
{
public:
	obfuscated_string(const char* str)
	{
		m_str = strdup(str);
	}

	~obfuscated_string()
	{
		if (m_str != NULL)
			free((void*)m_str);
	}

	operator char* () { return (char*)m_str; }
	operator const char* () { return m_str; }

	const char* m_str;
};
