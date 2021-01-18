#include "acutest.h"
#include <cassert>
#include <cstring>
#include <src/types.h>
#include <uspi.h>

#include <src/net-arp.h>
#include <src/net-ipv4.h>

uint8_t uspiBuffer[USPI_FRAME_BUFFER_SIZE];

extern "C" int USPiSendFrame(const void* pBuffer, unsigned nLength)
{
	assert(nLength <= USPI_FRAME_BUFFER_SIZE);
	memcpy(uspiBuffer, pBuffer, nLength);
	return nLength;
}

extern "C" void USPiGetMACAddress(unsigned char Buffer[6])
{
	for (size_t i = 0; i < 6; i++)
	{
		Buffer[i] = i << 4;
	}
}

#include "net-arp.h"
#include "net-utils.h"

TEST_LIST = {
	{"Net::Utils::InternetChecksum", TestNetUtilsInternetChecksum},
	{"Net::Utils::Crc32", TestNetUtilsCrc32},
	{"Net::Utils::GetMacAddress", TestNetUtilsGetMacAddress},
	{"Net::Arp::PacketSerializeDeserialize", TestNetArpPacketSerializeDeserialize},
	{"Net::Arp::SendPacket", TestNetArpSendPacket},
	{"Net::Arp::SendRequest", TestNetArpSendRequest},
	{"Net::Arp::SendReply", TestNetArpSendReply},
	{"Net::Arp::SendAnnouncement", TestNetArpSendAnnouncement},
	{"Net::Arp::HandlePacket Invalid", TestNetArpHandlePacketInvalid},
	{nullptr, nullptr},
};
