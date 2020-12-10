#pragma once
#include "net.h"

struct Ipv4Header
{
	unsigned int version : 4;
	unsigned int ihl : 4;
	unsigned int dscp : 6;
	unsigned int ecn : 2;
	uint16_t totalLength;
	uint16_t identification;
	unsigned int flags : 3;
	unsigned int fragmentOffset : 13;
	uint8_t ttl;
	uint8_t protocol;
	uint16_t headerChecksum;
	uint32_t sourceIp;
	uint32_t destinationIp;

	Ipv4Header(
		uint8_t protocol, uint32_t sourceIp, uint32_t destinationIp, uint16_t totalLength
	) :
		version(4),
		ihl(5),
		dscp(0),
		ecn(0),
		totalLength(totalLength),
		identification(0),
		flags(0),
		fragmentOffset(0),
		ttl(64),
		protocol(protocol),
		headerChecksum(0),
		sourceIp(sourceIp),
		destinationIp(destinationIp)
	{
	}
	
	size_t Serialize(uint8_t* buffer)
	{
		size_t i = 0;

		buffer[i++] = version << 4 | ihl;
		buffer[i++] = dscp << 2 | ecn;
		buffer[i++] = totalLength >> 8;
		buffer[i++] = totalLength;
		buffer[i++] = identification >> 8;
		buffer[i++] = identification;
		buffer[i++] = (flags << 13 | fragmentOffset) >> 8;
		buffer[i++] = flags << 13 | fragmentOffset;
		buffer[i++] = ttl;
		buffer[i++] = protocol;

		headerChecksum = 0;
		buffer[i++] = headerChecksum;
		buffer[i++] = headerChecksum >> 8;

		buffer[i++] = sourceIp >> 24;
		buffer[i++] = sourceIp >> 16;
		buffer[i++] = sourceIp >> 8;
		buffer[i++] = sourceIp;
		buffer[i++] = destinationIp >> 24;
		buffer[i++] = destinationIp >> 16;
		buffer[i++] = destinationIp >> 8;
		buffer[i++] = destinationIp;

		headerChecksum = internetChecksum(buffer, i);
		buffer[10] = headerChecksum;
		buffer[11] = headerChecksum >> 8;

		return i;
	}
} __attribute__((packed));

template<class T>
struct Ipv4Packet
{
	Ipv4Header header;
	T payload;

	Ipv4Packet(uint8_t protocol, uint32_t sourceIp, uint32_t destinationIp, T payload) :
		header(protocol, sourceIp, destinationIp, sizeof(Ipv4Packet<T>)),
		payload(payload)
	{
	}

	size_t Serialize(uint8_t* buffer)
	{
		size_t i = 0;
		i += header.Serialize(buffer);
		i += payload.Serialize(buffer + i);
		return i;
	}
} __attribute__((packed));
