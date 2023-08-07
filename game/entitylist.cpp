// Entity List

#include <dbg.h>
#include <IPlayerUtils.h>

#include <hl_sdk/common/r_studioint.h>
#include <hl_sdk/engine/APIProxy.h>
#include <hl_sdk/engine/studio.h>

#include "utils.h"
#include "entitylist.h"

#include "../config.h"
#include "../features/visual.h"

extern bool g_bPlayingbackDemo;

//-----------------------------------------------------------------------------
// Vars
//-----------------------------------------------------------------------------

CEntityList g_EntityList;
CEntity gEnts[ MY_MAXENTS + 1 ];

//-----------------------------------------------------------------------------
// Entity List
//-----------------------------------------------------------------------------

CEntityList::CEntityList()
{
	memset( gEnts, 0, sizeof( CEntity ) * ( MY_MAXENTS + 1 ) );
}

void CEntityList::Update()
{
	if ( !g_bPlayingbackDemo &&
		 !g_Config.cvars.esp &&
		 !g_Config.cvars.radar &&
		 !g_Config.cvars.aimbot &&
		 !g_Config.cvars.silent_aimbot &&
		 !g_Config.cvars.ragebot &&
		 !g_Config.cvars.st_show_entity_info &&
		 !g_Config.cvars.st_show_revive_area_info &&
		 !g_Config.cvars.st_show_revive_info &&
		 !g_Config.cvars.st_show_revive_boost_info &&
		 !g_Config.cvars.st_player_hulls )
		return;

	static float vScreen[ 2 ];

	cl_entity_s *pLocal = g_pEngineFuncs->GetLocalPlayer();
	cl_entity_s *pViewModel = g_pEngineFuncs->GetViewModel();

	int nLocalPlayer = pLocal->index;

	for ( register int i = 1; i <= MY_MAXENTS; ++i )
	{
		gEnts[ i ].m_bValid = false;

		//if ( i == nLocalPlayer )
		//	continue;

		const char *pszModelName;
		const char *pszSlashLastOccur;

		model_t *pModel = NULL;
		studiohdr_t *pStudioHeader = NULL;

		cl_entity_t *pEntity = g_pEngineFuncs->GetEntityByIndex( i );

		if ( pEntity == NULL || pEntity->curstate.messagenum < pLocal->curstate.messagenum || pEntity == pViewModel )
			continue;

		if ( ( pModel = pEntity->model ) == NULL || *pModel->name != 'm' )
			continue;

		if ( pszSlashLastOccur = strrchr( pModel->name, '/' ) )
			pszModelName = pszSlashLastOccur + 1;
		else
			continue;

		if ( !( pStudioHeader = (studiohdr_t *)g_pEngineStudio->Mod_Extradata( pModel ) ) || pStudioHeader->numhitboxes == 0 )
			continue;

		if ( pEntity->player )
		{
			float flHealth = gEnts[ i ].m_flHealth = g_pPlayerUtils->GetHealth( i );

			gEnts[ i ].m_bAlive = ( flHealth > 0.f || flHealth < -1.f );

			gEnts[ i ].m_classInfo.id = CLASS_PLAYER;
			gEnts[ i ].m_classInfo.flags = 0;

			if ( flHealth < -1.f )
			{
				gEnts[ i ].m_bFriend = false;
				gEnts[ i ].m_bEnemy = true;
				gEnts[ i ].m_bNeutral = false;
			}
			else
			{
				gEnts[ i ].m_bFriend = true;
				gEnts[ i ].m_bEnemy = false;
				gEnts[ i ].m_bNeutral = false;
			}
		}
		else
		{
			gEnts[ i ].m_bAlive = true;

			if ( pEntity->curstate.renderfx != (int)kRenderFxDeadPlayer )
			{
				gEnts[ i ].m_classInfo = GetEntityClassInfo( pszModelName );

				if ( IsEntityClassCorpse( gEnts[ i ].m_classInfo, pEntity->curstate.solid ) || IsEntityClassTrash( gEnts[ i ].m_classInfo ) )
					continue;
			}
			else
			{
				gEnts[ i ].m_classInfo.id = CLASS_DEAD_PLAYER;
				gEnts[ i ].m_classInfo.flags = FL_CLASS_NEUTRAL;
			}

			gEnts[ i ].m_bItem = IsEntityClassItem( gEnts[ i ].m_classInfo );
			gEnts[ i ].m_bFriend = IsEntityClassFriend( gEnts[ i ].m_classInfo );
			gEnts[ i ].m_bEnemy = IsEntityClassEnemy( gEnts[ i ].m_classInfo );
			gEnts[ i ].m_bNeutral = IsEntityClassNeutral( gEnts[ i ].m_classInfo );

			if ( gEnts[ i ].m_bItem )
			{
				if ( gEnts[ i ].m_classInfo.id == CLASS_ITEM_GRENADE )
				{
					if ( pEntity->curstate.solid == SOLID_BBOX )
					{
						if ( pEntity->curstate.sequence == 0 && pEntity->curstate.movetype == MOVETYPE_STEP )
						{
							gEnts[ i ].m_bNeutral = true;
							gEnts[ i ].m_bEnemy = false;
						}
						else
						{
							gEnts[ i ].m_bEnemy = true;
						}
					}
				}
			}
			else if ( pEntity->curstate.solid == SOLID_NOT )
			{
				switch ( pEntity->curstate.movetype )
				{
				case MOVETYPE_STEP:
				case MOVETYPE_FLY:
				case MOVETYPE_TOSS:
					if ( gEnts[ i ].m_bFriend || gEnts[ i ].m_bEnemy )
						continue;

					break;
				}
			}
		}

		gEnts[ i ].m_pEntity = pEntity;
		gEnts[ i ].m_pStudioHeader = pStudioHeader;

		gEnts[ i ].m_vecVelocity = pEntity->curstate.origin - pEntity->prevstate.origin;
		gEnts[ i ].m_frametime = pEntity->curstate.animtime - pEntity->prevstate.animtime;

		gEnts[ i ].m_bPlayer = pEntity->player;
		gEnts[ i ].m_bDucked = pEntity->curstate.usehull;
		gEnts[ i ].m_bVisible = UTIL_WorldToScreen( pEntity->curstate.origin + pEntity->curstate.mins +
													( ( pEntity->curstate.origin + pEntity->curstate.maxs ) - ( pEntity->curstate.origin + pEntity->curstate.mins ) ) * 0.5f, vScreen );

		if ( gEnts[ i ].m_classInfo.id == CLASS_OBJECT_CP )
		{
			gEnts[ i ].m_vecOrigin = pEntity->curstate.origin;

			gEnts[ i ].m_vecMins.Zero();
			gEnts[ i ].m_vecMaxs.Zero();
		}
		else
		{
			gEnts[ i ].m_vecOrigin = pEntity->curstate.origin;

			gEnts[ i ].m_vecMins = pEntity->curstate.mins;
			gEnts[ i ].m_vecMaxs = pEntity->curstate.maxs;
		}

		gEnts[ i ].m_bValid = true;
	}
}

void CEntityList::UpdateHitboxes( int index )
{
	extern bone_matrix3x4_t *g_pBoneTransform;

	if ( !g_Config.cvars.esp && !g_Config.cvars.radar && !g_Config.cvars.aimbot && !g_Config.cvars.silent_aimbot && !g_Config.cvars.ragebot )
		return;

	if ( !gEnts[ index ].m_bValid )
		return;

	Assert( gEnts[ index ].m_pStudioHeader->numhitboxes < MAXHITBOXES );

	Vector vecHitbox;

	Vector vecFrameVelocity = gEnts[ index ].m_vecVelocity * gEnts[ index ].m_frametime;
	mstudiobbox_t *pHitbox = (mstudiobbox_t *)( (byte *)gEnts[ index ].m_pStudioHeader + gEnts[ index ].m_pStudioHeader->hitboxindex );

	for ( int i = 0; i < gEnts[ index ].m_pStudioHeader->numhitboxes; i++ )
	{
		Vector vecMid = ( pHitbox[ i ].bbmin + pHitbox[ i ].bbmax ) * 0.5f;

		VectorTransform( vecMid, ( *g_pBoneTransform )[ pHitbox[ i ].bone ], vecHitbox );

		gEnts[ index ].m_rgHitboxes[ i ] = vecHitbox + vecFrameVelocity;
	}
}

CEntity *CEntityList::GetList()
{
	return gEnts;
}