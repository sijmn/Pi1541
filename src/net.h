#pragma once
#include <cstdint>
#include <cstring>
#include <algorithm>
#include <array>
#include <unordered_map>

#include "net-arp.h"
#include "net-ethernet.h"
#include "net-ipv4.h"
#include "net-utils.h"

enum UdpPort {
	UDP_PORT_DHCP_SERVER = 67,
	UDP_PORT_DHCP_CLIENT = 68,
	UDP_PORT_TFTP = 69,  // nice
};

//
// IPv4
//
void HandleIpv4Packet(
	const Net::Ethernet::EthernetFrameHeader ethernetHeader,
	const uint8_t* buffer,
	const size_t size
);

//
// UDP
//
void HandleUdpDatagram(
	const Net::Ethernet::EthernetFrameHeader ethernetHeader,
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

extern bool FileUploaded;
