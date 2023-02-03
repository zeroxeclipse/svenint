#ifndef ANTIAFK_H
#define ANTIAFK_H

#ifdef _WIN32
#pragma once
#endif

#include <math/vector.h>
#include <base_feature.h>
#include <IHooks.h>

class CAntiAFK : CBaseFeature
{
public:
	CAntiAFK();

	virtual bool Load();
	virtual void PostLoad();

	virtual void Unload();

	void Reset();

	void OnVideoInit();
	void CreateMove(float frametime, struct usercmd_s *cmd, int active);

	void OnEndLoading();
	void OnEnterToServer();
	void OnRespawn();

private:
	void AntiAFK(struct usercmd_s *cmd);
	void WalkAround(struct usercmd_s *cmd, int &delay, int &movement_button, const int delay_count);
	void RotateCamera();

	void OnDie();

private:
	bool m_bDead;
	bool m_bComingBackToAFKPoint;

	bool m_bWaitingForClientdata;
	bool m_bWaitingForRespawn;

	Vector2D m_vecAFKPoint; // 2D point only
	float m_flComingBackStartTime;

	DetourHandle_t m_hUserMsgHook_Health;
};

extern CAntiAFK g_AntiAFK;

#endif // ANTIAFK_H