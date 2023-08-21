#ifndef VISUAL_H
#define VISUAL_H

#ifdef _WIN32
#pragma once
#endif

#include <vector>

#include <base_feature.h>
#include <IHooks.h>
#include <math/vector.h>

#include "../game/class_table.h"

typedef float bone_matrix3x4_t[MAXSTUDIOBONES][3][4];

//-----------------------------------------------------------------------------
// Structs
//-----------------------------------------------------------------------------

struct display_sound_origin_t
{
	Vector origin;
	float time;
};

struct hitmarker_t
{
	Vector origin;
	float time;
};

//-----------------------------------------------------------------------------
// Visuals
//-----------------------------------------------------------------------------

class CVisual : public CBaseFeature
{
public:
	CVisual();

	virtual bool Load();
	virtual void PostLoad();

	virtual void Unload();

public:
	void Draw();
	void ProcessBones();
	bool StudioRenderModel();

	void OnVideoInit();
	void GameFrame();
	void OnHUDRedraw(float flTime);
	void V_CalcRefdef(struct ref_params_s *pparams);

	void ResetJumpSpeed();
	inline void SetDemoMessageSpeed( float flSpeed ) { m_flDemoMsgSpeed = flSpeed; }
	void AddSound(const Vector &vecOrigin);
	void AddHitmarker(const Vector &vecOrigin, float flStayTime);

	void CClient_SoundEngine__PlayFMODSoundPost( void *thisptr, int fFlags, int entindex, float *vecOrigin, int iChannel, const char *pszSample, float flVolume, float flAttenuation, int iUnknown, int iPitch, int iSoundIndex, float flOffset );

private:
	void ESP();
	void ShowSounds();
	void DrawHitmarkers();
	void DrawCrosshair();
	void ShowSpeed();
	void ShowGrenadeTimer();
	void ShowGrenadeTrajectory();
	void ShowARGrenadeTrajectory();
	void Lightmap();

	void DrawPlayerInfo_Default(int index, int iHealth, bool bIsEntityFriend, float top_mid_x, float top_mid_y, float bottom_mid_x, float bottom_mid_y);
	void DrawEntityInfo_Default(int index, class_info_t classInfo, float bottom_mid_x, float bottom_mid_y, int r, int g, int b);

	void DrawPlayerInfo_SAMP(int index, int iHealth, bool bDucking, bool bIsEntityFriend, Vector vecTop);
	void DrawEntityInfo_SAMP(int index, class_info_t classInfo, Vector vecTop, int r, int g, int b);
	
	void DrawPlayerInfo_L4D(int index, int iHealth, bool bDucking, bool bIsEntityFriend, Vector vecTop);
	void DrawEntityInfo_L4D(int index, class_info_t classInfo, Vector vecTop, int r, int g, int b);

	void DrawBox(bool bPlayer, bool bItem, int iHealth, int x, int y, int w, int h, int r, int g, int b);
	void DrawBones(int index, studiohdr_t *pStudioHeader);

private:
	std::vector<display_sound_origin_t> m_vSounds;
	std::vector<hitmarker_t> m_vHitMarkers;

	float m_flTime;

	float m_flPrevTime;
	float m_flFadeTime;
	float m_flJumpSpeed;
	float m_flDemoMsgSpeed;

	int m_clFadeFrom[3];

	bool m_bOnGround;

	void *m_pfnV_FadeAlpha;
	void *m_pfnEV_HLDM_PlayTextureSound;

	DetourHandle_t m_hV_FadeAlpha;
	DetourHandle_t m_hEV_HLDM_PlayTextureSound;
	//DetourHandle_t m_hHUD_PlayerMoveTexture;
	//DetourHandle_t m_hUserMsgHook_StartSound;
	DetourHandle_t m_hUserMsgHook_CreateBlood;
	DetourHandle_t m_hUserMsgHook_ScreenShake;
	DetourHandle_t m_hUserMsgHook_ScreenFade;

	int m_hHitMarkerTexture;

	int m_iScreenWidth;
	int m_iScreenHeight;
};

extern CVisual g_Visual;

#endif // VISUAL_H