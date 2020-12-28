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

//
// IPv4
//
void HandleIpv4Packet(
	const Net::Ethernet::Header ethernetHeader,
	const uint8_t* buffer,
	const size_t size
);

//
// ICMP
//
void SendIcmpEchoRequest(Net::Utils::MacAddress mac, uint32_t ip);
void HandleIcmpFrame(const uint8_t* buffer);
