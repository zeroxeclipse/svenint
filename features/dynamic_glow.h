// Dynamic Glow

#pragma once

class CDynamicGlow
{
public:
	void OnHUDRedraw();
	void OnAddEntityPost(int is_visible, int type, struct cl_entity_s *ent, const char *modelname);

private:
	void CreateDynamicLight(int entindex, float *vOrigin, float *pColor24, float flRadius, float flDecay, float flDieTime);
};

extern CDynamicGlow g_DynamicGlow;