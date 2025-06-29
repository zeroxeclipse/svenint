// BSP Stuff

#include "bsp.h"

#include <gl/GL.h>

#include <IRender.h>
#include <IClient.h>
#include <IFileSystem.h>
#include <ISvenModAPI.h>

#include <dbg.h>
#include <convar.h>

#include <hl_sdk/common/com_model.h>

#include <istream>
#include <streambuf>
#include <unordered_map>
#include <algorithm>

#include "../game/draw_context.h"
#include "../game/drawing.h"
#include "../game/utils.h"
#include "../config.h"

//-----------------------------------------------------------------------------
// Structs
//-----------------------------------------------------------------------------

struct membuf : std::streambuf
{
	membuf(unsigned char *begin, int len)
	{
		this->setg((char *)begin, (char *)begin, (char *)begin + len);
	}
};

typedef std::unordered_map<std::string, std::string> EntityKeyValues;

//-----------------------------------------------------------------------------
// Vars
//-----------------------------------------------------------------------------

CBsp g_Bsp;

unsigned char *bsp = NULL;

static std::vector<EntityKeyValues> vEnts;
static std::vector<TriggerEntity> vTriggers;
static std::vector<MonsterSpawn> vMonsterSpawns;
static std::vector<FuncWall> vFuncWalls;

//-----------------------------------------------------------------------------
// ConVars / ConCommands
//-----------------------------------------------------------------------------

class CDrawTransModel : public IDrawContext
{
public:
	CDrawTransModel(int modelindex);

	virtual void	Draw(void);
	virtual bool	ShouldStopDraw(void);

	virtual const Vector &GetDrawOrigin(void) const;

private:
	int m_modelindex;
	Vector m_vecMidPoint;
};

CDrawTransModel::CDrawTransModel(int modelindex)
{
	m_modelindex = modelindex;

	bspheader_t *header = (bspheader_t *)bsp;

	lump_t *lump_models = &header->lumps[LUMP_MODELS];

	bspmodel_t *models = (bspmodel_t *)(bsp + lump_models->fileofs);
	int models_count = lump_models->filelen / sizeof(bspmodel_t);

	bspmodel_t *model = &models[m_modelindex];

	Vector vecOrigin = *(Vector *)model->origin;
	Vector vecMins = *(Vector *)model->mins;
	Vector vecMaxs = *(Vector *)model->maxs;

	m_vecMidPoint = vecOrigin + vecMins;
	m_vecMidPoint += ((vecOrigin + vecMaxs) - (vecOrigin + vecMins)) * 0.5f;
}

void CDrawTransModel::Draw()
{
	glEnable(GL_BLEND);
	glDisable(GL_ALPHA_TEST);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glDisable(GL_TEXTURE_2D);

	bspheader_t *header = (bspheader_t *)bsp;

	lump_t *lump_models = &header->lumps[LUMP_MODELS];
	lump_t *lump_nodes = &header->lumps[LUMP_NODES];
	lump_t *lump_faces = &header->lumps[LUMP_FACES];
	lump_t *lump_edges = &header->lumps[LUMP_EDGES];
	lump_t *lump_surfedges = &header->lumps[LUMP_SURFEDGES];
	lump_t *lump_vertexes = &header->lumps[LUMP_VERTEXES];

	bspmodel_t *models = (bspmodel_t *)(bsp + lump_models->fileofs);
	int models_count = lump_models->filelen / sizeof(bspmodel_t);

	bspnode_t *nodes = (bspnode_t *)(bsp + lump_nodes->fileofs);
	int nodes_count = lump_nodes->filelen / sizeof(bspnode_t);

	bspface_t *faces = (bspface_t *)(bsp + lump_faces->fileofs);
	int faces_count = lump_faces->filelen / sizeof(bspface_t);

	bspedge_t *edges = (bspedge_t *)(bsp + lump_edges->fileofs);
	int edges_count = lump_edges->filelen / sizeof(bspedge_t);

	uint32_t *surfedges = (uint32_t *)(bsp + lump_surfedges->fileofs);
	int surfedges_count = lump_surfedges->filelen / sizeof(uint32_t);

	bspvertex_t *vertexes = (bspvertex_t *)(bsp + lump_vertexes->fileofs);
	int vertexes_count = lump_vertexes->filelen / sizeof(bspvertex_t);

	bspmodel_t *model = &models[m_modelindex];

	glColor4f(1.f, 1.f, 1.f, 0.6f);

	for (int i = 0; i < model->numfaces; i++)
	{
		int facenum = model->firstface + i;
		bspface_t *face = &faces[facenum];

		glBegin(GL_TRIANGLE_FAN);
		//glBegin(GL_TRIANGLES);

		for (int j = 0; j < face->numedges; j++)
		{
			int32_t edgenum = surfedges[face->firstedge + j];
			bspedge_t *edge = &edges[abs(edgenum)];

			int edgevertexnum = (edgenum >= 0 ? 1 : 0);
			int vertexnum = edge->v[edgevertexnum];

			Vector vertex = *(Vector *)&vertexes[vertexnum];

			//Render()->DrawPoint(vertex, { 255, 255, 255, 255 }, 24.f, 10.f);
			glVertex3f(vertex.x, vertex.y, vertex.z);
		}

		glEnd();
	}

	glEnable(GL_TEXTURE_2D);

	glDisable(GL_BLEND);
	glDisable(GL_ALPHA_TEST);
}

bool CDrawTransModel::ShouldStopDraw()
{
	return false;
}

const Vector &CDrawTransModel::GetDrawOrigin() const
{
	return m_vecMidPoint;
}

CON_COMMAND(sc_ultra_test, "")
{
	if (args.ArgC() > 1)
	{
		int modelindex = atoi(args[1]);

		Render()->AddDrawContext( new CDrawTransModel(modelindex), 5.f );

		//model_t *pModel = g_pEngineStudio->GetModelByIndex( modelindex );

		//if (pModel == NULL)
		//	return;

		//Msg("[MODEL] type: %d\n", pModel->type);
		//Msg("[MODEL] numframes: %d\n", pModel->numframes);
		//Msg("[MODEL] synctype: %d\n", pModel->synctype);
		//Msg("[MODEL] flags: %d\n", pModel->flags);
		//Msg("[MODEL] radius: %d\n", pModel->radius);
		//Msg("[MODEL] firstmodelsurface: %d\n", pModel->firstmodelsurface);
		//Msg("[MODEL] nummodelsurfaces: %d\n", pModel->nummodelsurfaces);
		//Msg("[MODEL] numsubmodels: %d\n", pModel->numsubmodels);
		//Msg("[MODEL] numplanes: %d\n", pModel->numplanes);
		//Msg("[MODEL] numleafs: %d\n", pModel->numleafs);
		//Msg("[MODEL] numvertexes: %d\n", pModel->numvertexes);
		//Msg("[MODEL] numedges: %d\n", pModel->numedges);
		//Msg("[MODEL] numnodes: %d\n", pModel->numnodes);
		//Msg("[MODEL] numtexinfo: %d\n", pModel->numtexinfo);
		//Msg("[MODEL] numsurfaces: %d\n", pModel->numsurfaces);
		//Msg("[MODEL] numsurfedges: %d\n", pModel->numsurfedges);
		//Msg("[MODEL] numclipnodes: %d\n", pModel->numclipnodes);
		//Msg("[MODEL] nummarksurfaces: %d\n", pModel->nummarksurfaces);
		//Msg("[MODEL] numtextures: %d\n", pModel->numtextures);

		//for (int i = 0; i < MAX_MAP_HULLS; i++)
		//{
		//	Msg("[MODEL HULLS] hulls[%d].firstclipnode: %d\n", i, pModel->hulls[i].firstclipnode);
		//	Msg("[MODEL HULLS] hulls[%d].lastclipnode: %d\n", i, pModel->hulls[i].lastclipnode);
		//	Msg("[MODEL HULLS] hulls[%d].clip_mins: %.2f %.2f %.2f\n", i, VectorExpand( pModel->hulls[i].clip_mins ));
		//	Msg("[MODEL HULLS] hulls[%d].clip_maxs: %.2f %.2f %.2f\n\n", i, VectorExpand( pModel->hulls[i].clip_maxs ));
		//}

		//return;

		bspheader_t *header = (bspheader_t *)bsp;

		lump_t *lump_models = &header->lumps[LUMP_MODELS];
		lump_t *lump_nodes = &header->lumps[LUMP_NODES];
		lump_t *lump_faces = &header->lumps[LUMP_FACES];
		lump_t *lump_edges = &header->lumps[LUMP_EDGES];
		lump_t *lump_surfedges = &header->lumps[LUMP_SURFEDGES];
		lump_t *lump_vertexes = &header->lumps[LUMP_VERTEXES];

		bspmodel_t *models = (bspmodel_t *)(bsp + lump_models->fileofs);
		int models_count = lump_models->filelen / sizeof(bspmodel_t);

		bspnode_t *nodes = (bspnode_t *)(bsp + lump_nodes->fileofs);
		int nodes_count = lump_nodes->filelen / sizeof(bspnode_t);
		
		bspface_t *faces = (bspface_t *)(bsp + lump_faces->fileofs);
		int faces_count = lump_faces->filelen / sizeof(bspface_t);

		bspedge_t *edges = (bspedge_t *)(bsp + lump_edges->fileofs);
		int edges_count = lump_edges->filelen / sizeof(bspedge_t);
		
		uint32_t *surfedges = (uint32_t *)(bsp + lump_surfedges->fileofs);
		int surfedges_count = lump_surfedges->filelen / sizeof(uint32_t);

		bspvertex_t *vertexes = (bspvertex_t *)(bsp + lump_vertexes->fileofs);
		int vertexes_count = lump_vertexes->filelen / sizeof(bspvertex_t);

		bspedge_t *pivot_edge = &edges[0];

		// dump
		Msg("Model Index: %d\n", modelindex);

		bspmodel_t *model = &models[modelindex];

		Msg("Number of faces #%d\n", model->numfaces);

		Msg("origin: %.3f %.3f %.3f\n", VectorExpand(*(Vector *)model->origin));
		Msg("mins: %.3f %.3f %.3f\n", VectorExpand(*(Vector *)model->mins));
		Msg("maxs: %.3f %.3f %.3f\n", VectorExpand(*(Vector *)model->maxs));
		Msg("headnode: %d %d %d %d\n", model->headnode[0], model->headnode[1], model->headnode[2], model->headnode[3]);
		Msg("visleafs: %d\n", model->visleafs);
		Msg("firstface: %d\n", model->firstface);
		Msg("numfaces: %d\n", model->numfaces);

		//for (int n = 0; n < MAX_MAP_HULLS; n++)
		//{
		//	int nodenum = model->headnode[n];
		//	if (nodenum == 0)
		//		continue;

		//	bspnode_t *node = &nodes[nodenum];

		//	Msg("nodenum: %d\n", nodenum);
		//	Msg("planenum: %d\n", node->planenum);
		//	Msg("children[0]: %d\n", node->children[0]);
		//	Msg("children[1]: %d\n", node->children[1]);
		//	Msg("mins: %d %d %d\n", node->mins[0], node->mins[1], node->mins[2]);
		//	Msg("maxs: %d %d %d\n", node->maxs[0], node->maxs[1], node->maxs[2]);
		//	Msg("firstface: %d\n", node->firstface);
		//	Msg("numfaces: %d\n", node->numfaces);

		//	continue;

		//	for (int i = 0; i < node->numfaces; i++)
		//	{
		//		int facenum = node->firstface + i;
		//		bspface_t *face = &faces[facenum];

		//		Msg("Face #%d\n", facenum);
		//		Msg("Face side #%d\n", face->side);

		//		Msg("Number of sides #%d\n", face->numedges);

		//		for (int j = 0; j < face->numedges; j++)
		//		{
		//			int edgenum = face->firstedge + j;
		//			bspedge_t *edge = &edges[abs(edgenum)];

		//			int edgevertexnum = (edgenum >= 0 ? 1 : 0);
		//			int vertexnum = edge->v[edgevertexnum];

		//			Vector vertex = *(Vector *)&vertexes[vertexnum];
		//			////Vector vertex = *(Vector *)&vertexes[pivot_edge->v[edgevertexnum]] - *(Vector *)&vertexes[vertexnum];

		//			Msg("Edge vertex[%d] - %d (%.3f %.3f %.3f)\n", edgevertexnum, vertexnum, VectorExpand(vertex));

		//			Debug()->DrawPoint(vertex, { 255, 255, 255, 255 }, 24.f, 10.f);

		//			////Vector vertex1 = *(Vector *)&vertexes[edge->v[0]] - *(Vector *)&vertexes[pivot_edge->v[0]];
		//			////Vector vertex2 = *(Vector *)&vertexes[edge->v[1]] - *(Vector *)&vertexes[pivot_edge->v[1]];

		//			////Msg("Edge #%d\n", edgenum);
		//			////Msg("Edge vertex[0] - %d (%.3f %.3f %.3f)\n", edge->v[0], VectorExpand(vertex1));
		//			////Msg("Edge vertex[1] - %d (%.3f %.3f %.3f)\n", edge->v[1], VectorExpand(vertex2));

		//			////if ( edge->v[0] && edge->v[1] )
		//			////{
		//			////	bspvertex_t *vertex1 = &vertexes[edge->v[0]];
		//			////	bspvertex_t *vertex2 = &vertexes[edge->v[1]];

		//			////	Debug()->DrawPoint( vertex1->point, { 255, 255, 255, 255 }, 24.f, 10.f );
		//			////	Debug()->DrawPoint( vertex2->point, { 255, 255, 255, 255 }, 24.f, 10.f );
		//			////}
		//		}

		//		Msg("\n");
		//	}
		//}

		for (int i = 0; i < model->numfaces; i++)
		{
			int facenum = model->firstface + i;
			bspface_t *face = &faces[facenum];

			Msg("Face #%d\n", facenum);
			Msg("Face side #%d\n", face->side);

			Msg("Number of edges #%d\n", face->numedges);

			for (int j = 0; j < face->numedges; j++)
			{
				int32_t edgenum = surfedges[face->firstedge + j];
				//int edgenum = face->firstedge + j;
				bspedge_t *edge = &edges[abs(edgenum)];

				int edgevertexnum = (edgenum >= 0 ? 1 : 0);
				int vertexnum = edge->v[edgevertexnum];

				Vector vertex = *(Vector *)&vertexes[vertexnum];
				////Vector vertex = *(Vector *)&vertexes[pivot_edge->v[edgevertexnum]] - *(Vector *)&vertexes[vertexnum];

				Msg("Edge (%d) > vertex[%d] - %d (%.3f %.3f %.3f)\n", edgenum, edgevertexnum, vertexnum, VectorExpand(vertex));

				//Render()->DrawPoint( vertex, { 255, 255, 255, 255 }, 24.f, 10.f );
				Render()->DrawBox( vertex, Vector(-2, -2, -2), Vector(2, 2, 2), { 255, 255, 255, 127 }, 10.f );

				////Vector vertex1 = *(Vector *)&vertexes[edge->v[0]] - *(Vector *)&vertexes[pivot_edge->v[0]];
				////Vector vertex2 = *(Vector *)&vertexes[edge->v[1]] - *(Vector *)&vertexes[pivot_edge->v[1]];

				////Msg("Edge #%d\n", edgenum);
				////Msg("Edge vertex[0] - %d (%.3f %.3f %.3f)\n", edge->v[0], VectorExpand(vertex1));
				////Msg("Edge vertex[1] - %d (%.3f %.3f %.3f)\n", edge->v[1], VectorExpand(vertex2));

				////if ( edge->v[0] && edge->v[1] )
				////{
				////	bspvertex_t *vertex1 = &vertexes[edge->v[0]];
				////	bspvertex_t *vertex2 = &vertexes[edge->v[1]];

				////	Debug()->DrawPoint( vertex1->point, { 255, 255, 255, 255 }, 24.f, 10.f );
				////	Debug()->DrawPoint( vertex2->point, { 255, 255, 255, 255 }, 24.f, 10.f );
				////}
			}

			Msg("\n");
		}
	}
	else
	{
		for (int i = 0; i < MAX_MAP_MODELS; i++)
		{
			model_t *pModel = g_pEngineStudio->GetModelByIndex(i);

			if ( pModel == NULL )
				continue;

			Msg("Modelname: %s (%d)\n", pModel->name, i);
		}
	}
}

ConVar sc_bsp( "sc_bsp", "1", FCVAR_CLIENTDLL, "Allow parse of .bsp maps" );

//-----------------------------------------------------------------------------
// Utilities
//-----------------------------------------------------------------------------

static inline std::string &rtrim(std::string &s, const char *t)
{
	s.erase(s.find_last_not_of(t) + 1);
	return s;
}

static inline std::string &ltrim(std::string &s, const char *t)
{
	s.erase(0, s.find_first_not_of(t));
	return s;
}

static inline std::string &trim(std::string &s, const char *t)
{
	return ltrim(rtrim(s, t), t);
}

//-----------------------------------------------------------------------------
// BSP functions
//-----------------------------------------------------------------------------

void CBsp::OnParseServerInfo()
{
	if ( bsp != NULL )
	{
		free( (void *)bsp );
		bsp = NULL;
	}

	LoadBsp();
}

void CBsp::OnDisconnect()
{
	if ( bsp != NULL )
	{
		free( (void *)bsp );
		bsp = NULL;
	}
}

void CBsp::V_CalcRefdef()
{
	if ( g_Config.cvars.show_triggers )
		//if ( false )
	{
		for ( const TriggerEntity &trigger : vTriggers )
		{
			float r, g, b, a;
			bool bDraw = true;

			switch ( trigger.iType )
			{
			case TRIGGER_ONCE:
				if ( g_Config.cvars.show_trigger_once )
				{
					r = g_Config.cvars.trigger_once_color[ 0 ];
					g = g_Config.cvars.trigger_once_color[ 1 ];
					b = g_Config.cvars.trigger_once_color[ 2 ];
					a = g_Config.cvars.trigger_once_color[ 3 ];
				}
				else
				{
					bDraw = false;
				}

				break;

			case TRIGGER_MULTIPLE:
				if ( g_Config.cvars.show_trigger_multiple )
				{
					r = g_Config.cvars.trigger_multiple_color[ 0 ];
					g = g_Config.cvars.trigger_multiple_color[ 1 ];
					b = g_Config.cvars.trigger_multiple_color[ 2 ];
					a = g_Config.cvars.trigger_multiple_color[ 3 ];
				}
				else
				{
					bDraw = false;
				}

				break;

			case TRIGGER_HURT:
				if ( g_Config.cvars.show_trigger_hurt )
				{
					r = g_Config.cvars.trigger_hurt_color[ 0 ];
					g = g_Config.cvars.trigger_hurt_color[ 1 ];
					b = g_Config.cvars.trigger_hurt_color[ 2 ];
					a = g_Config.cvars.trigger_hurt_color[ 3 ];
				}
				else
				{
					bDraw = false;
				}

				break;

			case TRIGGER_HURT_HEAL:
				if ( g_Config.cvars.show_trigger_hurt_heal )
				{
					r = g_Config.cvars.trigger_hurt_heal_color[ 0 ];
					g = g_Config.cvars.trigger_hurt_heal_color[ 1 ];
					b = g_Config.cvars.trigger_hurt_heal_color[ 2 ];
					a = g_Config.cvars.trigger_hurt_heal_color[ 3 ];
				}
				else
				{
					bDraw = false;
				}

				break;

			case TRIGGER_PUSH:
				if ( g_Config.cvars.show_trigger_push )
				{
					r = g_Config.cvars.trigger_push_color[ 0 ];
					g = g_Config.cvars.trigger_push_color[ 1 ];
					b = g_Config.cvars.trigger_push_color[ 2 ];
					a = g_Config.cvars.trigger_push_color[ 3 ];
				}
				else
				{
					bDraw = false;
				}

				break;

			case TRIGGER_TELEPORT:
				if ( g_Config.cvars.show_trigger_teleport )
				{
					r = g_Config.cvars.trigger_teleport_color[ 0 ];
					g = g_Config.cvars.trigger_teleport_color[ 1 ];
					b = g_Config.cvars.trigger_teleport_color[ 2 ];
					a = g_Config.cvars.trigger_teleport_color[ 3 ];
				}
				else
				{
					bDraw = false;
				}

				break;

			case TRIGGER_CHANGELEVEL:
				if ( g_Config.cvars.show_trigger_changelevel )
				{
					r = g_Config.cvars.trigger_changelevel_color[ 0 ];
					g = g_Config.cvars.trigger_changelevel_color[ 1 ];
					b = g_Config.cvars.trigger_changelevel_color[ 2 ];
					a = g_Config.cvars.trigger_changelevel_color[ 3 ];
				}
				else
				{
					bDraw = false;
				}

				break;

			case TRIGGER_ANTIRUSH:
				if ( g_Config.cvars.show_trigger_antirush )
				{
					r = g_Config.cvars.trigger_antirush_color[ 0 ];
					g = g_Config.cvars.trigger_antirush_color[ 1 ];
					b = g_Config.cvars.trigger_antirush_color[ 2 ];
					a = g_Config.cvars.trigger_antirush_color[ 3 ];
				}
				else
				{
					bDraw = false;
				}

				break;
			}

			if ( bDraw )
			{
				DrawBox( trigger.vecOrigin, trigger.vecMins, trigger.vecMaxs, r, g, b, a, 4.f, g_Config.cvars.bsp_wireframe );

				if ( trigger.iType == TRIGGER_PUSH )
				{
					Vector vecEnd;
					float dist = ( trigger.vecMaxs - trigger.vecMins ).Length();

					VectorMA( trigger.vecMidPoint, dist * 0.75f, trigger.vecDirection, vecEnd );

					Render()->DrawLine( trigger.vecMidPoint, vecEnd, r, g, b, a, 10.f );
				}
			}
		}
	}

	if ( g_Config.cvars.show_walls )
	{
		for ( const FuncWall &funcWall : vFuncWalls )
		{
			DrawBox( funcWall.vecOrigin, funcWall.vecMins, funcWall.vecMaxs, 0.f, 1.f, 1.f, 0.25f, 4.f, g_Config.cvars.bsp_wireframe );
		}
	}
}

void CBsp::Draw()
{
	if ( g_Config.cvars.show_spawns )
	{
		for (const MonsterSpawn &monster : vMonsterSpawns)
		{
			Vector2D vecScreen;
			Vector vecOrigin = monster.vecOrigin;

			if ( UTIL_WorldToScreen(vecOrigin, vecScreen) )
			{
				pmtrace_t trace;

				Vector vecStart = Client()->GetOrigin() + Client()->GetViewOffset();

				g_pEventAPI->EV_SetTraceHull(PM_HULL_POINT);
				g_pEventAPI->EV_PlayerTrace(vecStart, vecOrigin, PM_WORLD_ONLY, -1, &trace);

				if (trace.fraction != 1.f)
				{
					continue;
				}

				g_Drawing.DrawString(g_hFontESP, vecScreen.x, vecScreen.y, 255, 255, 255, 255, FONT_ALIGN_CENTER, monster.szClassname);
			}
		}
	}

	if ( g_Config.cvars.show_triggers && g_Config.cvars.show_triggers_info )
	{
		for (const TriggerEntity &trigger : vTriggers)
		{
			Color clr;
			bool bDraw = true;

			switch (trigger.iType)
			{
			case TRIGGER_ONCE:
			case TRIGGER_MULTIPLE:
				bDraw = false;
				break;

			case TRIGGER_HURT:
				if ( g_Config.cvars.show_trigger_hurt )
				{
					clr.SetColor( g_Config.cvars.trigger_hurt_color[0],
								 g_Config.cvars.trigger_hurt_color[1],
								 g_Config.cvars.trigger_hurt_color[2],
								 1.f );
				}
				else
				{
					bDraw = false;
				}

				break;
				
			case TRIGGER_HURT_HEAL:
				if ( g_Config.cvars.show_trigger_hurt_heal )
				{
					clr.SetColor( g_Config.cvars.trigger_hurt_heal_color[0],
								 g_Config.cvars.trigger_hurt_heal_color[1],
								 g_Config.cvars.trigger_hurt_heal_color[2],
								 1.f );
				}
				else
				{
					bDraw = false;
				}

				break;

			case TRIGGER_PUSH:
				if ( g_Config.cvars.show_trigger_push )
				{
					clr.SetColor( g_Config.cvars.trigger_push_color[0],
								 g_Config.cvars.trigger_push_color[1],
								 g_Config.cvars.trigger_push_color[2],
								 1.f );
				}
				else
				{
					bDraw = false;
				}

				break;

			case TRIGGER_TELEPORT:
			case TRIGGER_CHANGELEVEL:
				break;

			case TRIGGER_ANTIRUSH:
				if ( g_Config.cvars.show_trigger_antirush )
				{
					clr.SetColor( g_Config.cvars.trigger_antirush_color[0],
								 g_Config.cvars.trigger_antirush_color[1],
								 g_Config.cvars.trigger_antirush_color[2],
								 1.f );
				}
				else
				{
					bDraw = false;
				}

				break;
			}

			if ( bDraw )
			{
				Vector2D vecScreen;
				Vector vecOrigin = trigger.vecMidPoint;

				if ( UTIL_WorldToScreen(vecOrigin, vecScreen) )
				{
					pmtrace_t trace;

					Vector vecStart = Client()->GetOrigin() + Client()->GetViewOffset();

					g_pEventAPI->EV_SetTraceHull(PM_HULL_POINT);
					g_pEventAPI->EV_PlayerTrace(vecStart, vecOrigin, PM_WORLD_ONLY, -1, &trace);

					if (trace.fraction != 1.f)
					{
						continue;
					}

					switch (trigger.iType)
					{
					case TRIGGER_ONCE:
					case TRIGGER_MULTIPLE:
						break;

					case TRIGGER_HURT:
						g_Drawing.DrawStringF(g_hFontESP, vecScreen.x, vecScreen.y, clr.r, clr.g, clr.b, 255, FONT_ALIGN_CENTER, "Damage: %d", trigger.iDamage);
						break;
				
					case TRIGGER_HURT_HEAL:
						g_Drawing.DrawStringF(g_hFontESP, vecScreen.x, vecScreen.y, clr.r, clr.g, clr.b, 255, FONT_ALIGN_CENTER, "Heal: %d", abs(trigger.iDamage));
						break;

					case TRIGGER_PUSH:
						g_Drawing.DrawStringF(g_hFontESP, vecScreen.x, vecScreen.y, clr.r, clr.g, clr.b, 255, FONT_ALIGN_CENTER, "Push Speed: %d", trigger.iSpeed);
						break;

					case TRIGGER_TELEPORT:
						g_Drawing.DrawStringF(g_hFontESP, vecScreen.x, vecScreen.y, clr.r, clr.g, clr.b, 255, FONT_ALIGN_CENTER, "Model: %d", trigger.iModel);
						break;

					case TRIGGER_CHANGELEVEL:
						break;

					case TRIGGER_ANTIRUSH:
						g_Drawing.DrawStringF(g_hFontESP, vecScreen.x, vecScreen.y, clr.r, clr.g, clr.b, 255, FONT_ALIGN_CENTER, "Percentage: %.1f %%", trigger.flPercentage * 100.f);
						break;
					}
				}
			}
		}
	}
}

void CBsp::OnRenderScene()
{
	if ( g_Config.cvars.show_triggers )
	{
		glDisable( GL_TEXTURE_2D );

		g_pTriangleAPI->RenderMode( kRenderTransAdd );
		g_pTriangleAPI->CullFace( TRI_NONE );

		for (const TriggerEntity &trigger : vTriggers)
		{
			float r, g, b, a;
			bool bDraw = true;

			switch (trigger.iType)
			{
			case TRIGGER_ONCE:
				if ( g_Config.cvars.show_trigger_once )
				{
					r = g_Config.cvars.trigger_once_color[0];
					g = g_Config.cvars.trigger_once_color[1];
					b = g_Config.cvars.trigger_once_color[2];
					a = g_Config.cvars.trigger_once_color[3];
				}
				else
				{
					bDraw = false;
				}

				break;
				
			case TRIGGER_MULTIPLE:
				if ( g_Config.cvars.show_trigger_multiple )
				{
					r = g_Config.cvars.trigger_multiple_color[0];
					g = g_Config.cvars.trigger_multiple_color[1];
					b = g_Config.cvars.trigger_multiple_color[2];
					a = g_Config.cvars.trigger_multiple_color[3];
				}
				else
				{
					bDraw = false;
				}

				break;

			case TRIGGER_HURT:
				if ( g_Config.cvars.show_trigger_hurt )
				{
					r = g_Config.cvars.trigger_hurt_color[0];
					g = g_Config.cvars.trigger_hurt_color[1];
					b = g_Config.cvars.trigger_hurt_color[2];
					a = g_Config.cvars.trigger_hurt_color[3];
				}
				else
				{
					bDraw = false;
				}

				break;
				
			case TRIGGER_HURT_HEAL:
				if ( g_Config.cvars.show_trigger_hurt_heal )
				{
					r = g_Config.cvars.trigger_hurt_heal_color[0];
					g = g_Config.cvars.trigger_hurt_heal_color[1];
					b = g_Config.cvars.trigger_hurt_heal_color[2];
					a = g_Config.cvars.trigger_hurt_heal_color[3];
				}
				else
				{
					bDraw = false;
				}

				break;

			case TRIGGER_PUSH:
				if ( g_Config.cvars.show_trigger_push )
				{
					r = g_Config.cvars.trigger_push_color[0];
					g = g_Config.cvars.trigger_push_color[1];
					b = g_Config.cvars.trigger_push_color[2];
					a = g_Config.cvars.trigger_push_color[3];
				}
				else
				{
					bDraw = false;
				}

				break;
				
			case TRIGGER_TELEPORT:
				if ( g_Config.cvars.show_trigger_teleport )
				{
					r = g_Config.cvars.trigger_teleport_color[0];
					g = g_Config.cvars.trigger_teleport_color[1];
					b = g_Config.cvars.trigger_teleport_color[2];
					a = g_Config.cvars.trigger_teleport_color[3];
				}
				else
				{
					bDraw = false;
				}

				break;

			case TRIGGER_CHANGELEVEL:
				if ( g_Config.cvars.show_trigger_changelevel )
				{
					r = g_Config.cvars.trigger_changelevel_color[0];
					g = g_Config.cvars.trigger_changelevel_color[1];
					b = g_Config.cvars.trigger_changelevel_color[2];
					a = g_Config.cvars.trigger_changelevel_color[3];
				}
				else
				{
					bDraw = false;
				}

				break;

			case TRIGGER_ANTIRUSH:
				if ( g_Config.cvars.show_trigger_antirush )
				{
					r = g_Config.cvars.trigger_antirush_color[0];
					g = g_Config.cvars.trigger_antirush_color[1];
					b = g_Config.cvars.trigger_antirush_color[2];
					a = g_Config.cvars.trigger_antirush_color[3];
				}
				else
				{
					bDraw = false;
				}

				break;
			}

			if ( !bDraw )
				continue;

			model_t *pModel = g_pEngineStudio->GetModelByIndex( trigger.iModel + 1 );

			if ( pModel == NULL )
				continue;

			msurface_t *pSurfaces = pModel->surfaces + pModel->firstmodelsurface;

			for (int i = 0; i < pModel->nummodelsurfaces; i++)
			{
				g_pTriangleAPI->Color4f(r, g, b, a);
				g_pTriangleAPI->Begin(TRI_POLYGON);

				for (int j = 0; j < pSurfaces[i].polys->numverts; j++)
				{
					g_pTriangleAPI->Vertex3fv( pSurfaces[i].polys->verts[j] );
				}

				g_pTriangleAPI->End();
			}
		}

		glEnable( GL_TEXTURE_2D );

		g_pTriangleAPI->RenderMode( kRenderNormal );
	}
}

void CBsp::LoadBsp()
{
	if ( bsp != NULL )
	{
		free( (void *)bsp );
		bsp = NULL;
	}

	vMonsterSpawns.clear();
	vTriggers.clear();
	vEnts.clear();

	if ( !sc_bsp.GetBool() )
		return;

	char mapname[MAX_PATH];

	strncpy(mapname, g_pEngineFuncs->GetLevelName(), MAX_PATH);
	mapname[MAX_PATH - 1] = 0;

	FileHandle_t hFile = g_pFileSystem->Open(mapname, "r", "GAME");

	if ( !hFile )
	{
		hFile = g_pFileSystem->Open(mapname, "r", "GAMEDOWNLOAD");
	}

	if ( hFile )
	{
		int file_len = g_pFileSystem->Size( hFile );
		bsp = (unsigned char *)malloc( file_len );

		if ( !bsp )
		{
			Sys_Error("Failed to allocate memory for a bsp file");
			return;
		}

		g_pFileSystem->Read( bsp, file_len, hFile );
		g_pFileSystem->Close( hFile );

		bspheader_t *header = (bspheader_t *)bsp;

		lump_t *lump_entities = &header->lumps[LUMP_ENTITIES];
		lump_t *lump_models = &header->lumps[LUMP_MODELS];

		bspmodel_t *models = (bspmodel_t *)(bsp + lump_models->fileofs);
		int models_count = lump_models->filelen / sizeof(bspmodel_t);

		if ( LoadEntsFromBsp(bsp, lump_entities) )
		{
			const std::string sClassname = "classname";
			const std::string sMonsterType = "monstertype";
			const std::string sModel = "model";
			const std::string sOrigin = "origin";
			const std::string sAngles = "angles";
			const std::string sMinHullSize = "minhullsize";
			const std::string sMaxHullSize = "maxhullsize";
			const std::string sPercentage = "m_flPercentage";
			const std::string sSpeed = "speed";
			const std::string sDamage = "damage";
			const std::string sDmg = "dmg";

			for (const EntityKeyValues &keyvalues : vEnts)
			{
				auto found_classname = keyvalues.find(sClassname);

				if ( found_classname != keyvalues.end() )
				{
				#define INVALID_TRIGGER (TriggerType)(-1)

					bool bEntSpawn = false;
					bool bFuncWall = false;
					TriggerType trigger_type = INVALID_TRIGGER;

					const std::string &classname = keyvalues.at(sClassname);

					if ( !classname.rfind("monster_", 0) )
					{
						auto stringEndsWith = [](std::string const &str, std::string const &suffix)
						{
							if ( str.length() < suffix.length() )
								return false;

							return std::equal(suffix.rbegin(), suffix.rend(), str.rbegin());
						};

						if ( !stringEndsWith(classname, "_dead") )
						{
							bEntSpawn = true;
						}
					}
					else if ( classname == "monstermaker" || classname == "squadmaker" || classname == "env_xenmaker" || classname == "info_player_deathmatch" )
					{
						bEntSpawn = true;
					}
					else if ( classname == "func_wall" )
					{
						bFuncWall = true;
					}
					else if ( classname == "trigger_once" )
					{
						trigger_type = TRIGGER_ONCE;
					}
					else if ( classname == "trigger_multiple" )
					{
						trigger_type = TRIGGER_MULTIPLE;
					}
					else if ( classname == "trigger_hurt" )
					{
						trigger_type = TRIGGER_HURT;
					}
					else if ( classname == "trigger_push" )
					{
						trigger_type = TRIGGER_PUSH;
					}
					else if ( classname == "trigger_teleport" )
					{
						trigger_type = TRIGGER_TELEPORT;
					}
					else if ( classname == "trigger_changelevel" )
					{
						trigger_type = TRIGGER_CHANGELEVEL;
					}
					else if ( classname == "trigger_once_mp" )
					{
						trigger_type = TRIGGER_ANTIRUSH;
					}

					if ( trigger_type != INVALID_TRIGGER )
					{
						TriggerEntity trigger;
						ZeroMemory( &trigger, sizeof(TriggerEntity) );

						trigger.iType = trigger_type;

						if ( trigger_type != TRIGGER_ANTIRUSH )
						{
							auto found_model = keyvalues.find(sModel);
							auto found_origin = keyvalues.find(sOrigin);
							auto found_angles = keyvalues.find(sAngles);

							if ( trigger_type == TRIGGER_HURT )
							{
								auto found_dmg = keyvalues.find(sDmg);
								auto found_damage = keyvalues.find(sDamage);

								if ( found_damage != keyvalues.end() )
								{
									trigger.iDamage = atoi( keyvalues.at(sDamage).c_str() );

									if ( trigger.iDamage < 0 )
									{
										trigger.iType = TRIGGER_HURT_HEAL;
									}
								}
								else if ( found_dmg != keyvalues.end() )
								{
									trigger.iDamage = atoi( keyvalues.at(sDmg).c_str() );

									if ( trigger.iDamage < 0 )
									{
										trigger.iType = TRIGGER_HURT_HEAL;
									}
								}
							}
							else if ( trigger_type == TRIGGER_PUSH )
							{
								auto found_speed = keyvalues.find(sSpeed);

								if ( found_speed != keyvalues.end() )
								{
									trigger.iSpeed = atoi( keyvalues.at(sSpeed).c_str() );
								}
							}

							if ( found_model != keyvalues.end() )
							{
								if ( keyvalues.at(sModel)[0] == '*' )
								{
									int iModelIndex = atoi( keyvalues.at(sModel).c_str() + 1 );

									if ( iModelIndex > 0 && iModelIndex < models_count )
									{
										bspmodel_t *model = &models[ iModelIndex ];

										trigger.iModel = iModelIndex;
									
										trigger.vecOrigin = model->origin;
										trigger.vecMins = model->mins;
										trigger.vecMaxs = model->maxs;
									}
								}
							}

							if ( found_origin != keyvalues.end() )
							{
								float x = 0.f;
								float y = 0.f;
								float z = 0.f;

								int nParamsRead = sscanf( keyvalues.at(sOrigin).c_str(), "%f %f %f", &x, &y, &z );

								if (nParamsRead >= 3)
								{
									trigger.vecOrigin.x += x;
									trigger.vecOrigin.y += y;
									trigger.vecOrigin.z += z;
								}
								else if (nParamsRead == 2)
								{
									trigger.vecOrigin.x += x;
									trigger.vecOrigin.y += y;
								}
								else if (nParamsRead == 1)
								{
									trigger.vecOrigin.x += x;
								}
							}

							if ( found_angles != keyvalues.end() )
							{
								Vector vecAngles;

								int nParamsRead = sscanf( keyvalues.at(sAngles).c_str(), "%f %f %f", &vecAngles.x, &vecAngles.y, &vecAngles.z );

								if (nParamsRead != 0)
								{
									AngleVectors(vecAngles, &trigger.vecDirection, NULL, NULL);
								}
							}
						}
						else // TRIGGER_ANTIRUSH
						{
							auto found_mins = keyvalues.find(sMinHullSize);
							auto found_maxs = keyvalues.find(sMaxHullSize);

							if ( found_mins == keyvalues.end() || found_maxs == keyvalues.end() ) // not found
								continue;
							
							float x = 0.f;
							float y = 0.f;
							float z = 0.f;

							// Mins
							int nParamsRead = sscanf( keyvalues.at(sMinHullSize).c_str(), "%f %f %f", &x, &y, &z );

							if (nParamsRead >= 3)
							{
								trigger.vecOrigin.x = x;
								trigger.vecOrigin.y = y;
								trigger.vecOrigin.z = z;
							}
							else if (nParamsRead == 2)
							{
								trigger.vecOrigin.x = x;
								trigger.vecOrigin.y = y;
							}
							else if (nParamsRead == 1)
							{
								trigger.vecOrigin.x = x;
							}
							
							// Maxs
							nParamsRead = sscanf( keyvalues.at(sMaxHullSize).c_str(), "%f %f %f", &x, &y, &z );

							if (nParamsRead >= 3)
							{
								trigger.vecMaxs.x = x - trigger.vecOrigin.x;
								trigger.vecMaxs.y = y - trigger.vecOrigin.y;
								trigger.vecMaxs.z = z - trigger.vecOrigin.z;
							}
							else if (nParamsRead == 2)
							{
								trigger.vecMaxs.x = x - trigger.vecOrigin.x;
								trigger.vecMaxs.y = y - trigger.vecOrigin.y;
							}
							else if (nParamsRead == 1)
							{
								trigger.vecMaxs.x = x - trigger.vecOrigin.x;
							}

							auto found_percentage = keyvalues.find(sPercentage);

							if ( found_percentage != keyvalues.end() )
							{
								trigger.flPercentage = (float)atof( keyvalues.at(sPercentage).c_str() );
							}
						}

						trigger.vecMidPoint = trigger.vecOrigin + trigger.vecMins;
						trigger.vecMidPoint += ((trigger.vecOrigin + trigger.vecMaxs) - (trigger.vecOrigin + trigger.vecMins)) * 0.5f;

						vTriggers.push_back( trigger );
					}
					else if ( bEntSpawn )
					{
						MonsterSpawn monster;
						ZeroMemory( &monster, sizeof(MonsterSpawn) );

						auto found_origin = keyvalues.find(sOrigin);

						if ( found_origin != keyvalues.end() )
						{
							float x = 0.f;
							float y = 0.f;
							float z = 0.f;

							int nParamsRead = sscanf( keyvalues.at(sOrigin).c_str(), "%f %f %f", &x, &y, &z );

							if (nParamsRead >= 3)
							{
								monster.vecOrigin.x = x;
								monster.vecOrigin.y = y;
								monster.vecOrigin.z = z;
							}
							else if (nParamsRead == 2)
							{
								monster.vecOrigin.x = x;
								monster.vecOrigin.y = y;
							}
							else if (nParamsRead == 1)
							{
								monster.vecOrigin.x = x;
							}

							//auto found_monstertype = keyvalues.find(sOrigin);

							if ( classname == "monstermaker" || classname == "squadmaker" || classname == "env_xenmaker" )
							{
								auto found_monstertype = keyvalues.find(sMonsterType);

								if ( found_origin == keyvalues.end() )
									continue;

								strncpy_s( monster.szClassname, keyvalues.at(sMonsterType).c_str(), keyvalues.at(sMonsterType).length() );
							}
							else
							{
								strncpy_s( monster.szClassname, classname.c_str(), classname.length() );
							}

							vMonsterSpawns.push_back( monster );
						}
					}
					else if ( bFuncWall )
					{
						FuncWall funcWall;
						ZeroMemory( &funcWall, sizeof( FuncWall ) );

						auto found_model = keyvalues.find( sModel );
						auto found_origin = keyvalues.find( sOrigin );

						if ( found_model != keyvalues.end() )
						{
							if ( keyvalues.at( sModel )[ 0 ] == '*' )
							{
								int iModelIndex = atoi( keyvalues.at( sModel ).c_str() + 1 );

								if ( iModelIndex > 0 && iModelIndex < models_count )
								{
									bspmodel_t *model = &models[ iModelIndex ];

									funcWall.vecOrigin = model->origin;
									funcWall.vecMins = model->mins;
									funcWall.vecMaxs = model->maxs;
								}
							}
						}

						if ( found_origin != keyvalues.end() )
						{
							float x = 0.f;
							float y = 0.f;
							float z = 0.f;

							int nParamsRead = sscanf( keyvalues.at( sOrigin ).c_str(), "%f %f %f", &x, &y, &z );

							if ( nParamsRead >= 3 )
							{
								funcWall.vecOrigin.x += x;
								funcWall.vecOrigin.y += y;
								funcWall.vecOrigin.z += z;
							}
							else if ( nParamsRead == 2 )
							{
								funcWall.vecOrigin.x += x;
								funcWall.vecOrigin.y += y;
							}
							else if ( nParamsRead == 1 )
							{
								funcWall.vecOrigin.x += x;
							}
						}

						vFuncWalls.push_back( funcWall );
					}
				}
			}

			vEnts.clear();
		}
	}
}

bool CBsp::LoadEntsFromBsp(unsigned char *bsp, lump_t *lump_entities)
{
	static const char *trim_chars = " \t\n\r\f\v";

	membuf sbuf(bsp + lump_entities->fileofs, lump_entities->filelen);
	FILE *file = fopen("sven_internal/last_entmap.ent", "w");

	std::istream buffer( &sbuf );
	std::string sLine;

	int nLine = 0;
	int mode = 0;

	while ( std::getline(buffer, sLine) )
	{
		nLine++;

		sLine = trim(sLine, trim_chars);

		if ( sLine.empty() || !sLine[0] )
			continue;

		if ( file != NULL )
			fprintf( file, "%s\n", sLine.c_str());

		if ( sLine[0] == '/' )
			continue;

		if (mode == 0)
		{
			if (sLine[0] == '{')
			{
				mode = 1;

				vEnts.push_back( EntityKeyValues() );
			}
			else
			{
				Warning("[LoadEntsFromBsp] Expected start of keyvalues at line %d\n", nLine);

				if ( file != NULL )
					fclose(file);

				return false;
			}
		}
		else if (mode == 1)
		{
			if (sLine[0] == '}')
			{
				mode = 0;
			}
			else if (sLine[0] == '{')
			{
				Warning("[LoadEntsFromBsp] Expected end of keyvalues at line %d\n", nLine);

				if ( file != NULL )
					fclose(file);

				return false;
			}
			else
			{
				std::string sKey, sValue;

				char *buffer = (char *)(sLine.c_str());
				char *key = buffer + 1;
				char *value = NULL;

				if ( *buffer != '\"' )
				{
					Warning("[LoadEntsFromBsp] Expected start of key at line %d\n", nLine);
					
					if ( file != NULL )
						fclose(file);
					
					return false;
				}

				if ( *key == '\"' )
					continue;

				buffer++;

				while (*buffer)
				{
					if (*buffer == '\"')
					{
						char ch = *buffer;
						*buffer = 0;

						sKey = key;

						*buffer = ch;

						buffer++;
						break;
					}

					buffer++;
				}

				while (*buffer)
				{
					if (*buffer == '\"')
					{
						buffer++;
						value = buffer;
						break;
					}

					buffer++;
				}

				if (value != NULL && *value != '\"')
				{
					while (*buffer)
					{
						if (*buffer == '\"')
						{
							char ch = *buffer;
							*buffer = 0;

							sValue = value;

							*buffer = ch;

							break;
						}

						buffer++;
					}
				}
				else
				{
					sValue = "";
				}

				vEnts.back()[sKey] = sValue;
			}
		}
	}

	if ( file != NULL )
		fclose(file);

	return true;
}

//-----------------------------------------------------------------------------
// Getters
//-----------------------------------------------------------------------------

const std::vector<TriggerEntity> &CBsp::GetTriggers( void ) const
{
	return vTriggers;
}

const std::vector<MonsterSpawn> &CBsp::GetSpawns( void ) const
{
	return vMonsterSpawns;
}

//-----------------------------------------------------------------------------
// BSP implementation
//-----------------------------------------------------------------------------

CBsp::CBsp()
{
}

bool CBsp::Load()
{
	return true;
}

void CBsp::PostLoad()
{
}

void CBsp::Unload()
{
}