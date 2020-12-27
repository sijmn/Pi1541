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

enum UdpPort {
	UDP_PORT_DHCP_SERVER = 67,
	UDP_PORT_DHCP_CLIENT = 68,
	UDP_PORT_TFTP = 69,  // nice
};

typedef std::array<uint8_t, 6> MacAddress;

struct EthernetFrameHeader;
struct UdpDatagramHeader;
struct Ipv4Header;

//
// IPv4
//
void HandleIpv4Packet(
	const EthernetFrameHeader ethernetHeader, const uint8_t* buffer, const size_t size);

//
// UDP
//
void HandleUdpDatagram(
	const EthernetFrameHeader ethernetHeader,
	const Ipv4Header ipv4Header,
	const uint8_t* buffer,
	const size_t size
);

//
// ICMP
//
void SendIcmpEchoRequest(MacAddress mac, uint32_t ip);
void HandleIcmpFrame(const uint8_t* buffer);

//
// Helpers
//
std::uint32_t Crc32(const std::uint8_t* buffer, std::size_t size);
std::uint16_t InternetChecksum(const void* data, std::size_t size);
MacAddress GetMacAddress();

extern const MacAddress MacBroadcast;
extern uint32_t Ipv4Address;

extern bool FileUploaded;
