#ifndef MENU_MODULE_H
#define MENU_MODULE_H

#ifdef _WIN32
#pragma once
#endif

#include <base_feature.h>

#include <IDetoursAPI.h>
#include <IMemoryUtils.h>

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
	void DrawWindowAim();
	void DrawWindowVisuals();
	void DrawWindowHUD();
	void DrawWindowUtility();
	void DrawWindowConfig();
	void DrawWindowSettings();

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

#endif // MENU_MODULE_H