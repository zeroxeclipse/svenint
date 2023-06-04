#ifndef DEMO_MESSAGES_H
#define DEMO_MESSAGES_H

#ifdef _WIN32
#pragma once
#endif

#define TYPE_CUSTOM_MSG ( -1 )

typedef enum
{
	SVENINT_DEMOMSG_SEGMENT_INFO = 0,
	SVENINT_DEMOMSG_VELOMETER
} demo_message_t;

//-----------------------------------------------------------------------------
// CDemoMessageHandler
//-----------------------------------------------------------------------------

class CDemoMessageHandler
{
public:
	bool ReadClientDLLMessage( int size, unsigned char *buffer );

public:
	void WriteSegmentInfo( float flTime, const char *pszFormattedTimer, const char *pszMapname );
	void WriteVelometerSpeed( float flSpeed );
};

extern CDemoMessageHandler g_DemoMessage;

#endif // DEMO_MESSAGES_H