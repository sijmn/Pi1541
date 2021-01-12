#include "acutest.h"
#include <cstring>
#include <src/net-arp.h>

extern "C" void USPiSendFrame(const void* pBuffer, unsigned nLength) {}

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
	{"Net::Arp::Packet", TestNetArpPacket},
	{nullptr, nullptr},
};
