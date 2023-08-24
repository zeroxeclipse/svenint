#ifndef MENU_MODULE_H
#define MENU_MODULE_H

#ifdef _WIN32
#pragma once
#endif

#define IMGUI_USE_GL3 ( 1 )
#define SVENINT_OBFUSCATE ( 1 )

#include <imgui.h>

#include <base_feature.h>
#include <IDetoursAPI.h>
#include <IMemoryUtils.h>

#include "opengl.h"

#include "../imgui_custom/imgui_custom.h"

extern int g_iMenuState;

extern bool g_bMenuEnabled;
extern bool g_bMenuClosed;

extern float g_flMenuOpenTime;
extern float g_flMenuCloseTime;

//-----------------------------------------------------------------------------
// Menu feature
//-----------------------------------------------------------------------------

class CMenuModule : public CBaseFeature
{
	friend DECLARE_FUNC(BOOL, APIENTRY, HOOKED_wglSwapBuffers, HDC hdc);

public:
	CMenuModule();

	virtual bool Load();
	virtual void PostLoad();

	virtual void Unload();

public:
	void Draw();
	void WindowStyle();

private:
	// Draw menu windows
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
	// Utilities
	void LoadTextures();
	bool LoadTextureFromFile(const char *filename, GLuint *out_texture, int *out_width, int *out_height);
	void DeleteTextures();

	void SelectCurrentFont();
	void LoadFonts();
	void LoadFont(int type, const void *pFont, int iFontSize, const float fontSizes[3]);
	void MergeIconsToCurrentFont();

	void ResetShaders();

private:
	CImGuiCustom ImGuiCustom;
	ImGuiStyle *m_pStyle;

	ImFont *m_pMenuFontDefault;
	ImFont *m_pMenuFontBig;
	ImFont *m_pMenuFontSmall;

	void *m_pfnwglSwapBuffers;
	void *m_pfnSetCursorPos;

	DetourHandle_t m_hwglSwapBuffers;
	DetourHandle_t m_hSetCursorPos;

	int m_iLogoWidth;
	int m_iLogoHeight;
	GLuint m_hLogoTex;

	int m_iMenuTexWidth;
	int m_iMenuTexHeight;
	GLuint m_hMenuTex;

	bool m_bMenuTexLoaded;

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

#ifdef SVENINT_OBFUSCATE

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

#else

typedef const char *obfuscated_string;

#endif
