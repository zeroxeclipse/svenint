#ifndef FIRSTPERSON_ROAMING_H
#define FIRSTPERSON_ROAMING_H

#ifdef _WIN32
#pragma once
#endif

#include <math/vector.h>
#include <hl_sdk/common/cl_entity.h>

class CFirstPersonRoaming
{
public:
	CFirstPersonRoaming();

	bool StudioRenderModel();

	void V_CalcRefdef(struct ref_params_s *pparams);
	cl_entity_t *GetTargetPlayer();
	Vector &GetLerpViewAngles();

private:
	void GetPlayerViewAngles(Vector &vOutput);

private:
	int m_iTarget;
	int m_iSpectatorMode;

	cl_entity_t *m_pTarget;
	Vector m_vPrevAngles;
};

extern CFirstPersonRoaming g_FirstPersonRoaming;

#endif // FIRSTPERSON_ROAMING_H