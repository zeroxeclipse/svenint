// Edge Pixels Player

#include <dbg.h>
#include <convar.h>
#include <IRender.h>
#include <ISvenModAPI.h>

#include "../patterns.h"
#include "../game/utils.h"
#include "../utils/xorstr.h"

#include "edge_pixels_player.h"

//-----------------------------------------------------------------------------
// Declare Hooks
//-----------------------------------------------------------------------------

DECLARE_HOOK( void, __cdecl, CL_TempEntInit, void );

//-----------------------------------------------------------------------------
// Vars
//-----------------------------------------------------------------------------

constexpr int max_tents = 8192;

TEMPENTITY *gTempEnts = NULL;
TEMPENTITY **gpTempEntFree = NULL;

CEdgePixelsPlayer g_EdgePixelsPlayer;

//-----------------------------------------------------------------------------
// ConVars / ConCommands
//-----------------------------------------------------------------------------

static std::vector<Vector> points;
static int points_it = 0;
static int points_it2 = 0;
static int circle_points = 1;
static int clock_points = 1;
static double clock_angle = 0.f;
static Vector clock_pos;
static Vector clock_ang;

CON_COMMAND( sc_giga_test, "" )
{
	if ( args.ArgC() < 10 )
		return;

	float localSpaceToWorld[ 3 ][ 4 ];

	Vector vecPoint;
	points_it = 0;

	float radius = 60.f;

	circle_points = atoi( args[ 1 ] );
	clock_points = atoi( args[ 2 ] );
	clock_angle = atof( args[ 3 ] );
	clock_pos = Vector( atof( args[ 4 ] ), atof( args[ 5 ] ), atof( args[ 6 ] ) );
	clock_ang = Vector( atof( args[ 7 ] ), atof( args[ 8 ] ), atof( args[ 9 ] ) );

	float delta_angle = 2.0 * M_PI / circle_points;

	CVar()->SetValue( "fps_max", 10 * ( circle_points + clock_points ) );
	points_it2 = 0;

	for ( float angle = 0.f; angle < 2.0 * M_PI; angle += delta_angle )
	{
		vecPoint.Zero();

		vecPoint.x = cos( angle );
		vecPoint.y = sin( angle );

		vecPoint *= radius;

		points.push_back( vecPoint );
	}

	float delta_radius = radius / clock_points;

	for ( float r = 0.f; r < radius; r += delta_radius )
	{
		vecPoint.Zero();

		vecPoint.x = cos( clock_angle );
		vecPoint.y = sin( clock_angle );

		vecPoint *= r;

		points.push_back( vecPoint );
	}

	AngleMatrix( clock_ang, localSpaceToWorld );

	localSpaceToWorld[ 0 ][ 3 ] = clock_pos.x;
	localSpaceToWorld[ 1 ][ 3 ] = clock_pos.y;
	localSpaceToWorld[ 2 ][ 3 ] = clock_pos.z;

	for ( const Vector &point : points )
	{
		Vector worldPoint;
		VectorTransform( point, localSpaceToWorld, worldPoint );

		*(Vector *)&point = worldPoint;

		//Render()->DrawBox( worldPoint, Vector( -2, -2, -2 ), Vector( 2, 2, 2 ), { 232, 0, 232, 150 }, 5.f );
	}
}

CON_COMMAND( sc_giga_test_stop, "" )
{
	points.clear();
	points_it = 0;
}

static FILE *giga2_file = NULL;
static double giga2_width = 0.0;
static double giga2_height = 0.0;
static double giga2_framerate = 15.0;
static double giga2_last_play = 0.0;
static Vector giga2_pos;
static Vector giga2_ang;

CON_COMMAND( sc_giga_test2, "" )
{
	if ( args.ArgC() < 9 )
		return;

	int tmp;

	giga2_width = atof( args[ 1 ] );
	giga2_height = atof( args[ 2 ] );
	giga2_pos = Vector( atof( args[ 3 ] ), atof( args[ 4 ] ), atof( args[ 5 ] ) );
	giga2_ang = Vector( atof( args[ 6 ] ), atof( args[ 7 ] ), atof( args[ 8 ] ) );

	giga2_file = fopen( "ba.bin", "rb" );

	fread( &tmp, sizeof( int ), 1, giga2_file );
	fread( &tmp, sizeof( int ), 1, giga2_file );

	fread( &tmp, sizeof( int ), 1, giga2_file );
	giga2_framerate = 1.0 / static_cast<double>( tmp );

	giga2_last_play = 0.0;

	Msg( "%f\n", 1.0 / giga2_framerate );
}

CON_COMMAND( sc_giga_test2_stop, "" )
{
	if ( giga2_file == NULL )
		return;

	fclose( giga2_file );
	giga2_file = NULL;
}

//-----------------------------------------------------------------------------
// CEdgePixelsPlayer implementation
//-----------------------------------------------------------------------------

CEdgePixelsPlayer::CEdgePixelsPlayer()
{
}

//-----------------------------------------------------------------------------
// CreateMove
//-----------------------------------------------------------------------------

void CEdgePixelsPlayer::CreateMove( float frametime, usercmd_t *cmd, int active )
{
	if ( giga2_file != NULL )
	{
		if ( feof( giga2_file ) )
		{
			fclose( giga2_file );
			giga2_file = NULL;
			return;
		}

		if ( *dbRealtime - giga2_last_play >= giga2_framerate )
		{
			Render()->DrawClear();
			giga2_last_play = *dbRealtime;

			int pixels;
			fread( &pixels, sizeof( int ), 1, giga2_file );

			if ( pixels == 0 )
				return;

			struct PixVec2
			{
				double x;
				double y;
			};

			PixVec2 ndc;
			Vector localPoint, worldPoint, vecDir, vecAngles;
			Vector vecEyes = g_pPlayerMove->origin + g_pPlayerMove->view_ofs;
			float localSpaceToWorld[ 3 ][ 4 ];

			AngleMatrix( giga2_ang, localSpaceToWorld );

			localSpaceToWorld[ 0 ][ 3 ] = giga2_pos.x;
			localSpaceToWorld[ 1 ][ 3 ] = giga2_pos.y;
			localSpaceToWorld[ 2 ][ 3 ] = giga2_pos.z;

			cmd_t *npc_moveto = CVar()->FindCmd( "npc_moveto" );

			for ( int i = 0; i < pixels; i++ )
			{
				worldPoint.Zero();
				localPoint.Zero();

				fread( &ndc, sizeof( PixVec2 ), 1, giga2_file );

				localPoint.x = static_cast<float>( ( ndc.x * 2.0 - 1.0 ) * giga2_width );
				localPoint.y = static_cast<float>( ( ndc.y * 2.0 - 1.0 ) * giga2_height );

				VectorTransform( localPoint, localSpaceToWorld, worldPoint );

				//g_pEffectsAPI->R_SparkShower( worldPoint );

				vecDir = worldPoint - vecEyes;
				VectorAngles( vecDir, vecAngles );
				vecAngles.x *= -1.f;
				vecAngles.z = 0.f;

				g_pEngineFuncs->SetViewAngles( vecAngles );
				npc_moveto->function();

				//Render()->DrawBox( worldPoint, Vector( -2, -2, -2 ), Vector( 2, 2, 2 ), { 232, 0, 232, 150 }, 1.f );
			}

			//vecDir = giga2_pos - vecEyes;
			//VectorAngles( vecDir, vecAngles );
			//vecAngles.x *= -1.f;
			//vecAngles.z = 0.f;

			//g_pEngineFuncs->SetViewAngles( vecAngles );
			g_pEngineFuncs->SetViewAngles( Vector( 0, 90, 0 ) );
		}
	}

	if ( !points.empty() )
	{
		if ( points_it >= (int)points.size() )
		{
			points_it = 0;
			points.clear();
			points_it2++;

			float localSpaceToWorld[ 3 ][ 4 ];

			Vector vecPoint;

			float radius = 60.f;
			float delta_angle = 2.0 * M_PI / circle_points;

			clock_angle += ( 2.0 * M_PI * ( 1.0 / double( ( circle_points + clock_points ) ) ) );

			for ( float angle = 0.f; angle < 2.0 * M_PI; angle += delta_angle )
			{
				vecPoint.Zero();

				vecPoint.x = cos( angle );
				vecPoint.y = sin( angle );

				vecPoint *= radius;

				points.push_back( vecPoint );
			}

			float delta_radius = radius / clock_points;

			for ( float r = 0.f; r < radius; r += delta_radius )
			{
				vecPoint.Zero();

				vecPoint.x = cos( clock_angle );
				vecPoint.y = sin( clock_angle );

				vecPoint *= r;

				points.push_back( vecPoint );
			}

			AngleMatrix( clock_ang, localSpaceToWorld );

			localSpaceToWorld[ 0 ][ 3 ] = clock_pos.x;
			localSpaceToWorld[ 1 ][ 3 ] = clock_pos.y;
			localSpaceToWorld[ 2 ][ 3 ] = clock_pos.z;

			for ( const Vector &point : points )
			{
				Vector worldPoint;
				VectorTransform( point, localSpaceToWorld, worldPoint );

				*(Vector *)&point = worldPoint;

				//Render()->DrawBox( worldPoint, Vector( -2, -2, -2 ), Vector( 2, 2, 2 ), { 232, 0, 232, 150 }, 5.f );
			}
		}

		Vector vecPoint = points[ points_it ];
		Vector vecEyes = g_pPlayerMove->origin + g_pPlayerMove->view_ofs;

		Vector vecAngles;
		Vector vecDir = vecPoint - vecEyes;

		VectorAngles( vecDir, vecAngles );
		vecAngles.z = 0.f;

		//char command_buffer[ 64 ];
		//snprintf( command_buffer, M_ARRAYSIZE( command_buffer ), "setang %f %f 0;npc_moveto", vecAngles.x, vecAngles.y );
		//g_pEngineFuncs->ClientCmd( command_buffer );

		g_pEngineFuncs->SetViewAngles( vecAngles );
		g_pEngineFuncs->ClientCmd( "npc_moveto" );

		points_it++;
	}
}

//-----------------------------------------------------------------------------
// CL_TempEntInit hook
//-----------------------------------------------------------------------------

DECLARE_FUNC( void, __cdecl, HOOKED_CL_TempEntInit, void )
{
	ORIG_CL_TempEntInit();

	if ( gTempEnts == NULL || gpTempEntFree == NULL )
		return;

	memset( gTempEnts, 0, max_tents * sizeof( TEMPENTITY ) );

	for ( int i = 0; i < max_tents - 1; i++ )
	{
		gTempEnts[ i ].next = &gTempEnts[ i + 1 ];
		//gTempEnts[ i ].entity.trivial_accept = 0;
	}

	gTempEnts[ max_tents - 1 ].next = NULL;

	*gpTempEntFree = gTempEnts;
}

//-----------------------------------------------------------------------------
// CEdgePixelsPlayer feature implementation
//-----------------------------------------------------------------------------

bool CEdgePixelsPlayer::Load( void )
{
	ud_t inst;
	bool bScanOK = true;

	auto fCL_TempEntInit = MemoryUtils()->FindPatternAsync( SvenModAPI()->Modules()->Hardware, Patterns::Hardware::CL_TempEntInit );

	if ( !( m_pfnCL_TempEntInit = fCL_TempEntInit.get() ) )
	{
		Warning( xs( "Couldn't find function \"CL_TempEntInit\"\n" ) );
		bScanOK = false;
	}

	if ( !bScanOK )
		return false;

#if 0
	MemoryUtils()->InitDisasm( &inst, m_pfnCL_TempEntInit, 32, 80 );

	while ( MemoryUtils()->Disassemble( &inst ) )
	{
		if ( inst.mnemonic == UD_Imov && inst.operand[ 0 ].type == UD_OP_MEM && inst.operand[ 1 ].type == UD_OP_IMM )
		{
			gpTempEntFree = reinterpret_cast<TEMPENTITY **>( inst.operand[ 0 ].lval.udword );
			break;
		}
	}

	if ( gpTempEntFree != NULL )
	{
		gTempEnts = (TEMPENTITY *)malloc( max_tents * sizeof( TEMPENTITY ) );

		if ( gTempEnts != NULL )
		{
			memset( gTempEnts, 0, max_tents * sizeof( TEMPENTITY ) );

			for ( int i = 0; i < max_tents - 1; i++ )
			{
				gTempEnts[ i ].next = &gTempEnts[ i + 1 ];
				//gTempEnts[ i ].entity.trivial_accept = 0;
			}

			gTempEnts[ max_tents - 1 ].next = NULL;

			*gpTempEntFree = gTempEnts;
		}
		else
		{
			Warning( xs( "Failed to allocate memory for \"gTempEnts\"\n" ) );
		}
	}
	else
	{
		Warning( xs( "Couldn't find variable \"gpTempEntFree\"\n" ) );
	}
#endif

	return true;
}

void CEdgePixelsPlayer::PostLoad( void )
{
	m_hCL_TempEntInit = DetoursAPI()->DetourFunction( m_pfnCL_TempEntInit, HOOKED_CL_TempEntInit, GET_FUNC_PTR( ORIG_CL_TempEntInit ) );
}

void CEdgePixelsPlayer::Unload( void )
{
	DetoursAPI()->RemoveDetour( m_hCL_TempEntInit );
}