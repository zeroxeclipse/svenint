#ifndef __AUTOUPDATE_PROTOCOL__H
#define __AUTOUPDATE_PROTOCOL__H

#ifdef _WIN32
#pragma once
#endif

#define PROTOCOL_VERSION ( 1 )

// Error/result codes
#define AUResultCode_Bad				( -1 )
#define AUResultCode_OK					( 0 )
#define AUResultCode_NotConnected		( 1 )
#define AUResultCode_BadPlatform		( 2 )
#define AUResultCode_UpdateAvailable	( 3 )
#define AUResultCode_SocketError		( 4 )
#define AUResultCode_InvalidPacket		( 5 )
#define AUResultCode_InvalidPacketType	( 6 )

#define AUPacketHeader { 0xFF, 0xFF, 0x41, 0x55, 0x53, 0x76, 0x65, 0x6E, 0x49, 0x6E, 0x74 } // bFF bFF A U S v e n I n t

constexpr unsigned char __AUPacketHeader[] = AUPacketHeader;
constexpr unsigned int AUPacketHeaderSize = sizeof(__AUPacketHeader) / sizeof(__AUPacketHeader[0]);

enum AUPacketType
{
	PACKET_ESTABLISH_CONNECTION = 0, // [ int unused (0x4) ] -> [ int code (0x4) ]
	PACKET_DISCONNECT, // [ int code (0x4) ] -> [ void ]

	PACKET_RESPONSE, // [ int code (0x4) ] -> [ void ]

	PACKET_PLATFORM, // [ int platform (0x4) ] -> [ int code (0x4) ]
	PACKET_APP_VERSION, // [ app_version_t (sizeof app_version_t) ] -> [ int code (0x4) ]

	PACKET_QUERY_PROTOCOL_VERSION, // [ int version (0x4) ] -> ? <<<|>>> not used
	PACKET_QUERY_UPDATE, // [ void ] -> [ int size ], [ uint8_t encrypt_key[FLEXIBLE_SIZE], uint8_t app[FLEXIBLE_SIZE] ]

	PACKET_LAST = PACKET_QUERY_UPDATE
};

struct AUHeader
{
	unsigned char sequence[AUPacketHeaderSize];
};

struct AUPacket
{
	AUHeader header;
	AUPacketType type;
	unsigned int length; // length of further data
};

bool Protocol_InitPacket(struct AUPacket *packet, int type, int size);
bool Protocol_PacketIsValid(struct AUPacket *packet);

#endif