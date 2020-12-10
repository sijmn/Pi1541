#pragma once
#include "net.h"

struct IcmpPacketHeader
{
	std::uint8_t type;
	std::uint8_t code;
	std::uint16_t checksum;

	IcmpPacketHeader(std::uint8_t type, std::uint8_t code) :
		type(type), code(code), checksum(0)
	{
	}

	std::size_t Serialize(uint8_t* buffer) {
		size_t i = 0;
		buffer[i++] = type;
		buffer[i++] = code;
		buffer[i++] = checksum;
		buffer[i++] = checksum >> 8;
		return i;
	}
} __attribute__((packed));


template <class T>
struct IcmpPacket
{
	IcmpPacketHeader header;
	T payload;

	IcmpPacket(std::uint8_t type, std::uint8_t code, T payload) :
		header(type, code), payload(payload)
	{
	}

	std::size_t Serialize(uint8_t* buffer)
	{
		std::size_t i = 0;

		header.checksum = 0;
		i += header.Serialize(buffer);
		i += payload.Serialize(buffer + i);

		uint16_t checksum = internetChecksum(buffer, i);
		buffer[2] = checksum;
		buffer[3] = checksum >> 8;

		return i;
	}
} __attribute__((packed));

template <class T>
struct IcmpEchoRequest 
{
	uint16_t identifier;
	uint16_t sequenceNumber;
	T data;

	IcmpEchoRequest(T data) : identifier(0), sequenceNumber(0), data(data)
	{}

	size_t Serialize(uint8_t* buffer)
	{
		size_t i = 0;
		buffer[i++] = identifier >> 8;
		buffer[i++] = identifier;
		buffer[i++] = sequenceNumber >> 8;
		buffer[i++] = sequenceNumber;

		memcpy(buffer + i, &data, sizeof(T));
		i += sizeof(T);

		return i;
	}
} __attribute__((packed));
