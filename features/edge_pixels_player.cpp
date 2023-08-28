// Edge Pixels Player

#include <dbg.h>
#include <convar.h>
#include <IRender.h>
#include <ISvenModAPI.h>

#include "../patterns.h"
#include "../game/utils.h"
#include "../game/demo_message.h"
#include "../utils/xorstr.h"

#include "edge_pixels_player.h"

extern ref_params_t refparams;
extern bool g_bPlayingbackDemo;

//-----------------------------------------------------------------------------
// Declare Hooks
//-----------------------------------------------------------------------------

DECLARE_HOOK( void, __cdecl, CL_TempEntInit, void );

//-----------------------------------------------------------------------------
// Vars
//-----------------------------------------------------------------------------

constexpr int max_tents = 32768;

TEMPENTITY *gTempEnts = NULL;
TEMPENTITY **gpTempEntFree = NULL;

CEdgePixelsPlayer g_EdgePixelsPlayer;

//-----------------------------------------------------------------------------
// ConVars / ConCommands
//-----------------------------------------------------------------------------

ConVar sc_epp_play_in_demo( "sc_epp_play_in_demo", "1", FCVAR_CLIENTDLL );

CON_COMMAND( sc_epp_start, "" )
{
	if ( args.ArgC() > 10 )
	{
		bool bDemoPlayback = false;

		const char *filename = args[ 1 ];
		double width = atof( args[ 2 ] );
		double height = atof( args[ 3 ] );
		Vector vecPos( atof( args[ 4 ] ), atof( args[ 5 ] ), atof( args[ 6 ] ) );
		Vector vecAngles( atof( args[ 7 ] ), atof( args[ 8 ] ), atof( args[ 9 ] ) );
		int drawcalls = atoi( args[ 10 ] );

		if ( args.ArgC() > 11 )
			bDemoPlayback = !!atoi( args[ 11 ] );

		g_EdgePixelsPlayer.Start( filename, width, height, vecPos, vecAngles, drawcalls, bDemoPlayback );
	}
}

CON_COMMAND( sc_epp_stop, "" )
{
	g_EdgePixelsPlayer.Stop();
}

//-----------------------------------------------------------------------------
// CEdgePixelsPlayer implementation
//-----------------------------------------------------------------------------

CEdgePixelsPlayer::CEdgePixelsPlayer()
{
	m_bPlaying = false;
	m_bDemoPlayback = false;
	m_pFile = NULL;
	m_ulFileSize = 0;
	m_width = 0.0;
	m_height = 0.0;
	m_frametime = 0.0;
	m_lastPlayed = 0.0;
	m_iDrawCalls = 1;
	npc_moveto = NULL;
}

//-----------------------------------------------------------------------------
// Start playing
//-----------------------------------------------------------------------------

bool CEdgePixelsPlayer::Start( const char *pszFilename, double width, double height, const Vector &vecPos, const Vector &vecAngles, int iDrawCalls, bool bDemoPlayback /* = false */ )
{
	if ( pszFilename == NULL )
		return false;

	int tmp;

	m_width = height; // yeah
	m_height = width;
	m_vecPos = vecPos;
	m_vecAngles = vecAngles;
	m_iDrawCalls = V_max( 1, iDrawCalls );

	m_pFile = fopen( pszFilename, "rb" );

	if ( m_pFile == NULL )
		return false;

	fseek( m_pFile, 0, SEEK_END );
	m_ulFileSize = ftell( m_pFile );
	fseek( m_pFile, 0, SEEK_SET );

	m_sFilename = pszFilename;

	fread( &tmp, sizeof( int ), 1, m_pFile ); // width
	fread( &tmp, sizeof( int ), 1, m_pFile ); // height

	fread( &tmp, sizeof( int ), 1, m_pFile );
	m_frametime = 1.0 / static_cast<double>( tmp );
	m_lastPlayed = 0.0;

	m_drawAngles.clear();
	m_drawAnglesQueue.clear();

	m_bPlaying = true;
	m_bDemoPlayback = bDemoPlayback;

#if 0
	Msg( "pszFilename: %s\n", pszFilename );
	Msg( "width: %f\n", width );
	Msg( "height: %f\n", height );
	Msg( "vecPos: %.3f %.3f %.3f\n", VectorExpand( vecPos ) );
	Msg( "vecAngles: %.3f %.3f %.3f\n", VectorExpand( vecAngles ) );
	Msg( "iDrawCalls: %d\n", iDrawCalls );
	Msg( "frametime: %f\n", m_frametime );
#endif

	g_DemoMessage.WriteEdgePixelsPlayer( pszFilename, width, height, vecPos, vecAngles, iDrawCalls );
	return true;
}

//-----------------------------------------------------------------------------
// Stop playing
//-----------------------------------------------------------------------------

bool CEdgePixelsPlayer::Stop( void )
{
	if ( !m_bPlaying )
		return false;

	m_bPlaying = false;

	fclose( m_pFile );
	m_pFile = NULL;

	m_drawAngles.clear();
	m_drawAnglesQueue.clear();

	g_DemoMessage.WriteEdgePixelsPlayerStop();
	return true;
}

//-----------------------------------------------------------------------------
// CreateMove
//-----------------------------------------------------------------------------

void CEdgePixelsPlayer::CreateMove( float frametime, usercmd_t *cmd, int active )
{
	if ( m_bPlaying && m_pFile != NULL )
	{
		if ( feof( m_pFile ) || (unsigned long)ftell( m_pFile ) >= m_ulFileSize )
		{
			Stop();
			return;
		}

		Vector localPoint, worldPoint, vecDir, vecAngles, vecEyes;

		if ( !m_bDemoPlayback && !g_bPlayingbackDemo )
		{
			vecEyes = g_pPlayerMove->origin + g_pPlayerMove->view_ofs;
		}
		else
		{
			vecEyes = *(Vector *)( refparams.simorg ) + *(Vector *)( refparams.viewheight );
		}

	#if 0
		if ( m_iDrawCalls == 1 )
		{
			if ( !m_drawAngles.empty() )
			{
				for ( Vector &ang : m_drawAngles )
				{
					g_pEngineFuncs->SetViewAngles( ang );
					npc_moveto->function();
				}

				m_drawAngles.erase( m_drawAngles.begin() );
			}
		}
		else if ( !m_drawAnglesQueue.empty() )
	#else
		if ( !m_drawAnglesQueue.empty() )
	#endif
		{
			std::vector<Vector> &drawAngles = m_drawAnglesQueue[ 0 ];

			for ( Vector &ang : drawAngles )
			{
				g_pEngineFuncs->SetViewAngles( ang );
				npc_moveto->function();
			}

			m_drawAnglesQueue.erase( m_drawAnglesQueue.begin() );
		}

		if ( *dbRealtime - m_lastPlayed >= m_frametime )
		{
			m_lastPlayed = *dbRealtime;

			m_drawAngles.clear();
			m_drawAnglesQueue.clear();

			int pixels;
			fread( &pixels, sizeof( int ), 1, m_pFile );

			if ( pixels > 0 )
			{
				double ndc[ 2 ];
				float localSpaceToWorld[ 3 ][ 4 ];

				AngleMatrix( m_vecAngles, localSpaceToWorld );

				localSpaceToWorld[ 0 ][ 3 ] = m_vecPos.x;
				localSpaceToWorld[ 1 ][ 3 ] = m_vecPos.y;
				localSpaceToWorld[ 2 ][ 3 ] = m_vecPos.z;

				for ( int i = 0; i < pixels; i++ )
				{
					worldPoint.Zero();
					localPoint.Zero();

					fread( &ndc, sizeof( ndc ), 1, m_pFile );

					localPoint.x = static_cast<float>( ( 2.0 * ndc[ 0 ] - 1.0 ) * m_width );
					localPoint.y = static_cast<float>( ( 2.0 * ndc[ 1 ] - 1.0 ) * m_height );

					VectorTransform( localPoint, localSpaceToWorld, worldPoint );

					vecDir = worldPoint - vecEyes;
					VectorAngles( vecDir, vecAngles );
					vecAngles.x *= -1.f;
					vecAngles.z = 0.f;

					NormalizeAngles( vecAngles );

					m_drawAngles.push_back( vecAngles );
				}

				std::random_shuffle( m_drawAngles.begin(), m_drawAngles.end() );

				if ( m_iDrawCalls == 1 )
				{
					for ( int i = 0; i < pixels; i++ )
					{
						g_pEngineFuncs->SetViewAngles( m_drawAngles[ i ] );
						npc_moveto->function();
					}
				}
				else
				{
					int queued_draw_calls_cur = 0;
					int queued_draw_calls_pixels_delta = pixels / m_iDrawCalls;

					m_drawAnglesQueue.push_back( std::vector<Vector>() );

					for ( int i = 0; i < pixels; i++ )
					{
						if ( queued_draw_calls_cur * queued_draw_calls_pixels_delta <= i )
						{
							m_drawAnglesQueue.push_back( std::vector<Vector>() );
							queued_draw_calls_cur++;
						}

						m_drawAnglesQueue.back().push_back( m_drawAngles[ i ] );
					}

					std::random_shuffle( m_drawAnglesQueue.begin(), m_drawAnglesQueue.end() );

					std::vector<Vector> &drawAngles = m_drawAnglesQueue[ 0 ];

					for ( Vector &ang : drawAngles )
					{
						g_pEngineFuncs->SetViewAngles( ang );
						npc_moveto->function();
					}

					m_drawAnglesQueue.erase( m_drawAnglesQueue.begin() );
				}

				m_drawAngles.clear();
			}
		}

		vecDir = m_vecPos - vecEyes;
		VectorAngles( vecDir, vecAngles );
		vecAngles.x *= -1.f;
		vecAngles.z = 0.f;

		NormalizeAngles( vecAngles );
		g_pEngineFuncs->SetViewAngles( vecAngles );
	}

/*
static FILE *giga2_file = NULL;
static double giga2_width = 0.0;
static double giga2_height = 0.0;
static double giga2_framerate = 15.0;
static double giga2_last_play = 0.0;
static Vector giga2_pos;
static Vector giga2_ang;
static std::vector<std::vector<Vector>> drawQueue;

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

	if ( giga2_file != NULL )
	{
		if ( feof( giga2_file ) )
		{
			fclose( giga2_file );
			giga2_file = NULL;
			return;
		}

		if ( !drawQueue.empty() )
		{
			cmd_t *npc_moveto = CVar()->FindCmd( "npc_moveto" );

			std::vector<Vector> &angs = drawQueue[ 0 ];

			for ( Vector &ang : angs )
			{
				g_pEngineFuncs->SetViewAngles( ang );
				npc_moveto->function();
				//g_pEngineFuncs->ClientCmd( "npc_moveto" );
			}

			drawQueue.erase( drawQueue.begin() );
			g_pEngineFuncs->SetViewAngles( Vector( 0, 90, 0 ) );

			if ( drawQueue.empty() )
			{
				//CVar()->SetValue( "fps_max", (float)( 1.0 / ( ( ( 1.0 / 60.0 ) - 0.01 ) / 2.0 ) ) );
			}
		}
		else
		{
			g_pEngineFuncs->SetViewAngles( Vector( 0, 90, 0 ) );
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

			const int queued_draw_calls = 5;
			int queued_draw_calls_cur = 0;
			int queued_draw_calls_pixels_delta = pixels / queued_draw_calls;
			std::vector<Vector> worldPoints;

			drawQueue.push_back( std::vector<Vector>() );

			for ( int i = 0; i < pixels; i++ )
			{
				worldPoint.Zero();
				localPoint.Zero();

				fread( &ndc, sizeof( PixVec2 ), 1, giga2_file );

				localPoint.x = static_cast<float>( ( ndc.x * 2.0 - 1.0 ) * giga2_width );
				localPoint.y = static_cast<float>( ( ndc.y * 2.0 - 1.0 ) * giga2_height );

				VectorTransform( localPoint, localSpaceToWorld, worldPoint );

				vecDir = worldPoint - vecEyes;
				VectorAngles( vecDir, vecAngles );
				vecAngles.x *= -1.f;
				vecAngles.z = 0.f;

				worldPoints.push_back( vecAngles );
			}

			std::random_shuffle( worldPoints.begin(), worldPoints.end() );

			for ( int i = 0; i < pixels; i++ )
			{
				//g_pEffectsAPI->R_SparkShower( worldPoint );

				if ( queued_draw_calls_cur * queued_draw_calls_pixels_delta <= i )
				{
					queued_draw_calls_cur++;
					drawQueue.push_back( std::vector<Vector>() );
				}

				drawQueue.back().push_back( worldPoints[ i ] );

				//Render()->DrawBox( worldPoint, Vector( -2, -2, -2 ), Vector( 2, 2, 2 ), { 232, 0, 232, 150 }, 1.f );
			}

			std::random_shuffle( drawQueue.begin(), drawQueue.end() );

			//for ( std::vector<Vector> &angs : drawQueue )
			//{
			//	std::random_shuffle( angs.begin(), angs.end() );
			//}

			std::vector<Vector> &angs = drawQueue[ 0 ];

			for ( Vector &ang : angs )
			{
				g_pEngineFuncs->SetViewAngles( ang );
				npc_moveto->function();
				//g_pEngineFuncs->ClientCmd( "npc_moveto" );
			}

			//CVar()->SetValue( "fps_max", (float)( 1.0 / ( 0.01 / pixels ) ) );
			drawQueue.erase( drawQueue.begin() );

			//vecDir = giga2_pos - vecEyes;
			//VectorAngles( vecDir, vecAngles );
			//vecAngles.x *= -1.f;
			//vecAngles.z = 0.f;

			//g_pEngineFuncs->SetViewAngles( vecAngles );
			g_pEngineFuncs->SetViewAngles( Vector( 0, 90, 0 ) );
		}
	}
*/
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
#if 0
	bool bScanOK = true;

	auto fCL_TempEntInit = MemoryUtils()->FindPatternAsync( SvenModAPI()->Modules()->Hardware, Patterns::Hardware::CL_TempEntInit );

	if ( !( m_pfnCL_TempEntInit = fCL_TempEntInit.get() ) )
	{
		Warning( xs( "Couldn't find function \"CL_TempEntInit\"\n" ) );
		bScanOK = false;
	}

	if ( !bScanOK )
		return false;

	ud_t inst;

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
	//m_hCL_TempEntInit = DetoursAPI()->DetourFunction( m_pfnCL_TempEntInit, HOOKED_CL_TempEntInit, GET_FUNC_PTR( ORIG_CL_TempEntInit ) );

	npc_moveto = CVar()->FindCmd( "npc_moveto" );
}

void CEdgePixelsPlayer::Unload( void )
{
	//DetoursAPI()->RemoveDetour( m_hCL_TempEntInit );
}