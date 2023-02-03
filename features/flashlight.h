#ifndef CUSTOM_FLASHLIGHT_H
#define CUSTOM_FLASHLIGHT_H

#ifdef _WIN32
#pragma once
#endif

#include <string>
#include <vector>

#include <IDetoursAPI.h>
#include <base_feature.h>

//-----------------------------------------------------------------------------
// Custom Flashlight
//-----------------------------------------------------------------------------

class CFlashlight : public CBaseFeature
{
public:
	CFlashlight();

	virtual bool Load();
	virtual void PostLoad();

	virtual void Unload();

private:
	void *m_pfnCL_PlayerFlashlight;
	DetourHandle_t m_hCL_PlayerFlashlight;
};

extern CFlashlight g_Flashlight;

#endif // CUSTOM_FLASHLIGHT_H