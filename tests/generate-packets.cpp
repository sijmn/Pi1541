#include <algorithm>
#include <fstream>
#include <iostream>
#include <src/net-arp.h>

#include <src/types.h>
#include <uspi.h>

using namespace Net;

extern "C" int USPiSendFrame(const void* pBuffer, unsigned nLength)
{
	return std::min(nLength, static_cast<unsigned>(USPI_FRAME_BUFFER_SIZE));
}

extern "C" void USPiGetMACAddress(unsigned char Buffer[6]) {}

static void generateNetArpPacketValid()
{
	std::array<uint8_t, Arp::Packet::SerializedLength()> buffer;
	std::ofstream stream("inputs/net-arp-packet-valid.bin");
	const Utils::MacAddress sourceMac{10, 11, 12, 13, 14, 15};
	const Utils::MacAddress destMac{20, 21, 22, 23, 24, 25};

	Arp::Packet packet(Arp::Operation::ARP_OPERATION_REQUEST);
	packet.senderMac = sourceMac;
	packet.senderIp = Utils::Ipv4Address;
	packet.targetMac = destMac;
	packet.targetIp = Utils::Ipv4Address;
	packet.Serialize(buffer.data(), buffer.size());
	stream.write(reinterpret_cast<char*>(buffer.data()), buffer.size());

	std::cerr << "Generated Net::Arp::Packet (valid)" << std::endl;
}

int main(int, char**)
{
	Utils::Ipv4Address = 0xC0FFEEEE;
	generateNetArpPacketValid();
	return 0;
}
