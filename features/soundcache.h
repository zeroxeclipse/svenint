#ifndef SOUNDCACHE_H
#define SOUNDCACHE_H

#ifdef _WIN32
#pragma once
#endif

#include <base_feature.h>
#include <IDetoursAPI.h>

class CSoundcache : public CBaseFeature
{
public:
	CSoundcache();

	virtual bool Load();
	virtual void PostLoad();

	virtual void Unload();

public:


private:
	void *m_pfnCClient_SoundEngine__LoadSoundList;
	DetourHandle_t m_hCClient_SoundEngine__LoadSoundList;
	DetourHandle_t m_hNetMsgHook_ResourceList;

};

extern CSoundcache g_Soundcache;

#endif // SOUNDCACHE_H