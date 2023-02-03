#ifndef THIRDPERSON_H
#define THIRDPERSON_H

#ifdef _WIN32
#pragma once
#endif

#include <math/vector.h>

class CThirdPerson
{
public:
	CThirdPerson();

public:
	void OnConfigLoad();

	bool OnKeyPress(int down, int keynum);

	void CreateMove(float frametime, struct usercmd_s *cmd, int active);
	void V_CalcRefdef(struct ref_params_s *pparams);

public:
	void ResetRollAxis();

private:
	void PM_NoClip(struct usercmd_s *cmd);
	void ClampViewAngles();

private:
	Vector m_vecViewAngles;
	float m_flSavedPitchAngle;
	bool m_bEditMode;
	bool m_bThirdPerson;
};

extern CThirdPerson g_ThirdPerson;

#endif // THIRDPERSON_H