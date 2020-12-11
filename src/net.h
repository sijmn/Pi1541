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

typedef std::array<uint8_t, 6> MacAddress;

//
// ARP
//
void HandleArpFrame(uint8_t* buffer);
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

//
// IPv4
//
void HandleIpv4Frame(const uint8_t* buffer);

//
// UDP
//
struct EthernetFrameHeader;
struct UdpDatagramHeader;
struct Ipv4Header;

void HandleUdpFrame(const uint8_t* buffer);

void HandleTftpDatagram(
	const EthernetFrameHeader ethernetReqHeader,
	const Ipv4Header ipv4ReqHeader,
	const UdpDatagramHeader udpReqHeader,
	const uint8_t* buffer
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
extern const uint32_t Ipv4Address;

extern bool FileUploaded;
extern std::unordered_map<std::uint32_t, MacAddress> ArpTable;
