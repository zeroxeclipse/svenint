#pragma warning(disable : 26812)

#include "au_protocol.h"

#include <string.h>

unsigned char g_PacketHeader[] = AUPacketHeader;

bool Protocol_InitPacket(struct AUPacket *packet, int type, int length)
{
	memcpy( &packet->header, g_PacketHeader, AUPacketHeaderSize );
	
	packet->type = (AUPacketType)type;
	packet->length = length;

	return true;
}

bool Protocol_PacketIsValid(struct AUPacket *packet)
{
	return ( memcmp(&packet->header, g_PacketHeader, AUPacketHeaderSize) == 0 ) && ( packet->type >= 0 && packet->type <= PACKET_LAST );
}