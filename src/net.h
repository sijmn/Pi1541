#pragma once
#include <cstdint>
#include <cstring>
#include <algorithm>
#include <array>

enum EtherType {
	ETHERTYPE_IPV4 = 0x0800,
	ETHERTYPE_ARP = 0x0806,
};

enum ArpOperation {
	ARP_OPERATION_REQUEST = 1,
	ARP_OPERATION_REPLY = 2,
};

typedef std::array<std::uint8_t, 6> MacAddress;

std::uint32_t crc32(const std::uint8_t* buffer, std::size_t size);
std::uint16_t internetChecksum(const void* data, std::size_t size);

struct UdpDatagramHeader
{
	std::uint16_t sourcePort;
	std::uint16_t destinationPort;
	std::uint16_t length;
	std::uint16_t checksum;
};
