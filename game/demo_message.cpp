#include "demo_message.h"

#include <ISvenModAPI.h>

#include <dbg.h>
#include <convar.h>
#include <messagebuffer.h>

#include "../features/visual.h"
#include "../features/edge_pixels_player.h"

extern bool g_bPlayingbackDemo;

CDemoMessageHandler g_DemoMessage;

static CMessageBuffer demoMsg;
static char demobuffer[ 2048 ];

//-----------------------------------------------------------------------------
// ReadClientDLLMessage
//-----------------------------------------------------------------------------

bool CDemoMessageHandler::ReadClientDLLMessage( int size, unsigned char *buffer )
{
	CMessageBuffer msgBuffer;

	msgBuffer.Init( buffer, size, true );

	if ( msgBuffer.ReadLong() == TYPE_CUSTOM_MSG )
	{
		int msgType = msgBuffer.ReadShort();

		switch ( msgType )
		{
		case SVENINT_DEMOMSG_SEGMENT_INFO:
		{
			float flSegmentTime = msgBuffer.ReadFloat();

			ConColorMsg( { 255, 165, 0, 255 }, "> Finished segment in " );
			ConColorMsg( { 179, 255, 32, 255 }, msgBuffer.ReadString() );
			ConColorMsg( { 122, 200, 0, 255 }, " (%.6f) ", flSegmentTime );
			ConColorMsg( { 255, 165, 0, 255 }, "(map: %s)\n", msgBuffer.ReadString() );

			break;
		}

		case SVENINT_DEMOMSG_VELOMETER:
		{
			g_Visual.SetDemoMessageSpeed( msgBuffer.ReadFloat() );
			break;
		}
		
		case SVENINT_DEMOMSG_EDGE_PIXELS_PLAYER:
		{
			extern ConVar sc_epp_play_in_demo;

			if ( !sc_epp_play_in_demo.GetBool() )
				break;

			bool bStop = !!msgBuffer.ReadByte();

			if ( bStop )
			{
				g_EdgePixelsPlayer.Stop();
				break;
			}

			union
			{
				double m_dbl;
				struct
				{
					unsigned long low;
					unsigned long high;
				} m_llu;
			} dbl;

			std::string sFilename;
			double width, height;
			Vector vecPos, vecAngles;
			int drawcalls;

			sFilename = msgBuffer.ReadString();

			dbl.m_llu.low = msgBuffer.ReadLong();
			dbl.m_llu.high = msgBuffer.ReadLong();
			width = dbl.m_dbl;
			
			dbl.m_llu.low = msgBuffer.ReadLong();
			dbl.m_llu.high = msgBuffer.ReadLong();
			height = dbl.m_dbl;

			vecPos.x = msgBuffer.ReadFloat();
			vecPos.y = msgBuffer.ReadFloat();
			vecPos.z = msgBuffer.ReadFloat();

			vecAngles.x = msgBuffer.ReadFloat();
			vecAngles.y = msgBuffer.ReadFloat();
			vecAngles.z = msgBuffer.ReadFloat();

			drawcalls = msgBuffer.ReadLong();

			g_EdgePixelsPlayer.Start( sFilename.c_str(), width, height, vecPos, vecAngles, drawcalls, true );
			break;
		}

		default:
		{
			DevMsg( "[SvenInt] ReadClientDLLMessage: unrecognized message type\n" );
			break;
		}
		}

		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// WriteSegmentInfo
//-----------------------------------------------------------------------------

void CDemoMessageHandler::WriteSegmentInfo( float flTime, const char *pszFormattedTimer, const char *pszMapname )
{
	if ( g_bPlayingbackDemo )
		return;

	demoMsg.Init( demobuffer, M_ARRAYSIZE( demobuffer ) );

	demoMsg.WriteLong( TYPE_CUSTOM_MSG );
	demoMsg.WriteShort( SVENINT_DEMOMSG_SEGMENT_INFO );
	demoMsg.WriteFloat( flTime );
	demoMsg.WriteString( (char *)pszFormattedTimer );
	demoMsg.WriteString( (char *)pszMapname );

	demobuffer[ M_ARRAYSIZE( demobuffer ) - 1 ] = 0;

	g_pDemoAPI->WriteClientDLLMessage( demoMsg.GetBuffer()->cursize, demoMsg.GetBuffer()->data );
}

//-----------------------------------------------------------------------------
// WriteVelometerSpeed
//-----------------------------------------------------------------------------

void CDemoMessageHandler::WriteVelometerSpeed( float flSpeed )
{
/*
	if ( g_bPlayingbackDemo )
		return;

	demoMsg.Init( demobuffer, M_ARRAYSIZE( demobuffer ) );

	demoMsg.WriteLong( TYPE_CUSTOM_MSG );
	demoMsg.WriteShort( SVENINT_DEMOMSG_VELOMETER );
	demoMsg.WriteFloat( flSpeed );

	demobuffer[ M_ARRAYSIZE( demobuffer ) - 1 ] = 0;

	g_pDemoAPI->WriteClientDLLMessage( demoMsg.GetBuffer()->cursize, demoMsg.GetBuffer()->data );
*/
}

//-----------------------------------------------------------------------------
// WriteEdgePixelsPlayer
//-----------------------------------------------------------------------------

void CDemoMessageHandler::WriteEdgePixelsPlayer( const char *pszFilename, double width, double height, const Vector &vecPos, const Vector &vecAngles, int iDrawCalls )
{
	if ( g_bPlayingbackDemo )
		return;

	union
	{
		double m_dbl;
		struct
		{
			unsigned long low;
			unsigned long high;
		} m_llu;
	} dbl;

	demoMsg.Init( demobuffer, M_ARRAYSIZE( demobuffer ) );

	demoMsg.WriteLong( TYPE_CUSTOM_MSG );
	demoMsg.WriteShort( SVENINT_DEMOMSG_EDGE_PIXELS_PLAYER );

	demoMsg.WriteByte( 0 );
	demoMsg.WriteString( (char *)pszFilename );

	dbl.m_dbl = width;
	demoMsg.WriteLong( dbl.m_llu.low );
	demoMsg.WriteLong( dbl.m_llu.high );
	
	dbl.m_dbl = height;
	demoMsg.WriteLong( dbl.m_llu.low );
	demoMsg.WriteLong( dbl.m_llu.high );

	demoMsg.WriteFloat( vecPos.x );
	demoMsg.WriteFloat( vecPos.y );
	demoMsg.WriteFloat( vecPos.z );

	demoMsg.WriteFloat( vecAngles.x );
	demoMsg.WriteFloat( vecAngles.y );
	demoMsg.WriteFloat( vecAngles.z );

	demoMsg.WriteLong( iDrawCalls );

	demobuffer[ M_ARRAYSIZE( demobuffer ) - 1 ] = 0;

	g_pDemoAPI->WriteClientDLLMessage( demoMsg.GetBuffer()->cursize, demoMsg.GetBuffer()->data );
}

//-----------------------------------------------------------------------------
// WriteEdgePixelsPlayerStop
//-----------------------------------------------------------------------------

void CDemoMessageHandler::WriteEdgePixelsPlayerStop( void )
{
	if ( g_bPlayingbackDemo )
		return;

	demoMsg.Init( demobuffer, M_ARRAYSIZE( demobuffer ) );

	demoMsg.WriteLong( TYPE_CUSTOM_MSG );
	demoMsg.WriteShort( SVENINT_DEMOMSG_EDGE_PIXELS_PLAYER );

	demoMsg.WriteByte( 1 );
	
	demobuffer[ M_ARRAYSIZE( demobuffer ) - 1 ] = 0;

	g_pDemoAPI->WriteClientDLLMessage( demoMsg.GetBuffer()->cursize, demoMsg.GetBuffer()->data );
}