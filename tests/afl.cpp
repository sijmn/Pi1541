#include <array>
#include <cassert>
#include <cstring>
#include <fstream>

#include <src/types.h>
#include <uspi.h>

static uint8_t uspiBuffer[USPI_FRAME_BUFFER_SIZE];
static size_t uspiBufferLength;

extern "C" int USPiEthernetAvailable(void)
{
	return 1;
}

extern "C" int USPiReceiveFrame(void* pBuffer, unsigned* pResultLength)
{
	memcpy(pBuffer, uspiBuffer, uspiBufferLength);
	*pResultLength = uspiBufferLength;
	return 1;
}

extern "C" int USPiSendFrame(const void* pBuffer, unsigned nLength)
{
	assert(nLength <= USPI_FRAME_BUFFER_SIZE);
	memcpy(uspiBuffer, pBuffer, nLength);
	uspiBufferLength = nLength;
	return nLength;
}

extern "C" void USPiGetMACAddress(unsigned char Buffer[6])
{
	for (size_t i = 0; i < 6; i++)
		Buffer[i] = i << 4;
}

extern "C" void MsDelay(unsigned) {}

#include <src/net-arp.h>

int main(int, char**)
{
	fread(uspiBuffer, 1, sizeof(uspiBuffer), stdin);
	Net::Ethernet::Header ethernetHeader;
	Net::Arp::HandlePacket(ethernetHeader, uspiBuffer, sizeof(uspiBuffer));

	return 0;
}
