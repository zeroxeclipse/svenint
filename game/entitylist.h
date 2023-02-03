// Entity List

#ifndef ENTITY_LIST_H
#define ENTITY_LIST_H

#ifdef _WIN32
#pragma once
#endif

#include <hl_sdk/common/cl_entity.h>
#include <hl_sdk/common/com_model.h>
#include <hl_sdk/engine/studio.h>
#include <math/vector.h>

#include "class_table.h"

#define MY_MAXENTS 4096
#define MAXHITBOXES 128

//-----------------------------------------------------------------------------
// Entity Class
//-----------------------------------------------------------------------------

class CEntity
{
public:
	bool m_bValid;
	bool m_bPlayer;
	bool m_bItem;
	bool m_bAlive;
	bool m_bDucked;
	bool m_bVisible;
	bool m_bFriend;
	bool m_bEnemy;
	bool m_bNeutral;

	cl_entity_t *m_pEntity;
	studiohdr_t *m_pStudioHeader;

	Vector m_vecOrigin;
	Vector m_vecVelocity;

	Vector m_vecMins;
	Vector m_vecMaxs;

	float m_frametime;
	float m_flHealth;

	class_info_t m_classInfo;

	Vector m_rgHitboxes[MAXHITBOXES];
};

//-----------------------------------------------------------------------------
// Entity List
//-----------------------------------------------------------------------------

class CEntityList
{
public:
	CEntityList();

	void Update();
	void UpdateHitboxes(int index);

	CEntity *GetList();
	inline int GetMaxEntities() { return MY_MAXENTS; }
};

extern CEntityList g_EntityList;

#endif // ENTITY_LIST_H