#pragma once
#include "net.h"

enum IpProtocols
{
	IP_PROTO_ICMP = 1,
	IP_PROTO_TCP = 6,
	IP_PROTO_UDP = 17,
};

struct Ipv4Header
{
	uint8_t version;
	uint8_t ihl;
	uint8_t dscp;
	uint8_t ecn;
	uint16_t totalLength;
	uint16_t identification;
	uint8_t flags;
	uint16_t fragmentOffset;
	uint8_t ttl;
	uint8_t protocol;
	uint16_t headerChecksum;
	uint32_t sourceIp;
	uint32_t destinationIp;

	Ipv4Header();
	Ipv4Header(
		uint8_t protocol,
		uint32_t sourceIp,
		uint32_t destinationIp,
		uint16_t totalLength
	);

	static constexpr size_t SerializedLength()
	{
		// Hardcoded because of bitfields.
		return 20;
	}

	size_t Serialize(uint8_t* buffer) const;
	static Ipv4Header Deserialize(const uint8_t* buffer);
};
