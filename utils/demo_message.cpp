#include "demo_message.h"

#include <ISvenModAPI.h>

#include <dbg.h>
#include <convar.h>
#include <messagebuffer.h>

#include "../features/visual.h"

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