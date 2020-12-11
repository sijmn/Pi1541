#pragma once
#include <cstdint>
#include <cstring>
#include <algorithm>
#include <array>
#include <unordered_map>

enum EtherType {
	ETHERTYPE_IPV4 = 0x0800,
	ETHERTYPE_ARP = 0x0806,
};

enum ArpOperation {
	ARP_OPERATION_REQUEST = 1,
	ARP_OPERATION_REPLY = 2,
};

typedef std::array<std::uint8_t, 6> MacAddress;


void HandleArpFrame(std::uint8_t* buffer);
void SendArpPacket(ArpOperation operation,
					MacAddress targetMac,
					MacAddress senderMac,
					uint32_t senderIp,
					uint32_t targetIp);
void SendArpRequest(MacAddress targetMac,
					MacAddress senderMac,
					uint32_t senderIp,
					uint32_t targetIp);
void SendArpReply(MacAddress targetMac,
					MacAddress senderMac,
					uint32_t senderIp,
					uint32_t targetIp);
void SendArpAnnouncement(MacAddress mac, uint32_t ip);

void SendIcmpEchoRequest(MacAddress mac, uint32_t ip);

std::uint32_t Crc32(const std::uint8_t* buffer, std::size_t size);
std::uint16_t InternetChecksum(const void* data, std::size_t size);
MacAddress GetMacAddress();

extern MacAddress MacBroadcast;
extern std::unordered_map<std::uint32_t, MacAddress> ArpTable;

struct UdpDatagramHeader
{
	std::uint16_t sourcePort;
	std::uint16_t destinationPort;
	std::uint16_t length;
	std::uint16_t checksum;

	UdpDatagramHeader() {}

	UdpDatagramHeader(uint16_t sourcePort, uint16_t destinationPort, uint16_t length) :
		sourcePort(sourcePort), destinationPort(destinationPort), length(length)
	{}

	size_t Serialize(uint8_t* buffer)
	{
		size_t i = 0;
		buffer[i++] = sourcePort >> 8;
		buffer[i++] = sourcePort;
		buffer[i++] = destinationPort >> 8;
		buffer[i++] = destinationPort;
		buffer[i++] = length >> 8;
		buffer[i++] = length;
		buffer[i++] = checksum >> 8;
		buffer[i++] = checksum;
		return i;
	}

	UdpDatagramHeader Deserialize(const uint8_t* buffer)
	{
		UdpDatagramHeader self;
		self.sourcePort = buffer[0] << 8 | buffer[1];
		self.destinationPort = buffer[2] << 8 | buffer[3];
		self.length = buffer[4] << 8 | buffer[5];
		self.checksum = buffer[6] << 8 | buffer[7];
		return self;
	}
};

template <class T>
struct UdpDatagram
{
	UdpDatagramHeader header;
	T payload;

	UdpDatagram() {}

	UdpDatagram(uint16_t sourcePort, uint16_t destinationPort, T payload) :
		header(sourcePort, destinationPort, sizeof(UdpDatagram<T>)),
		payload(payload)
	{}
};
