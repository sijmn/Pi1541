#pragma once
#include "net.h"

enum IcmpType
{
	ICMP_ECHO_REPLY = 0,
	ICMP_ECHO_REQUEST = 8,
};

struct IcmpPacketHeader
{
	std::uint8_t type;
	std::uint8_t code;
	std::uint16_t checksum;

	IcmpPacketHeader() {}

	IcmpPacketHeader(std::uint8_t type, std::uint8_t code) :
		type(type), code(code), checksum(0)
	{}

	constexpr static std::size_t SerializedLength()
	{
		return sizeof(type) + sizeof(code) + sizeof(checksum);
	}

	std::size_t Serialize(uint8_t* buffer) const
	{
		size_t i = 0;
		buffer[i++] = type;
		buffer[i++] = code;
		buffer[i++] = checksum;
		buffer[i++] = checksum >> 8;
		return i;
	}

	static IcmpPacketHeader Deserialize(const uint8_t* buffer)
	{
		IcmpPacketHeader self;
		self.type = buffer[0];
		self.code = buffer[1];
		self.checksum = buffer[2] << 8 | buffer[3];
		return self;
	}
};

struct IcmpEchoHeader
{
	uint16_t identifier;
	uint16_t sequenceNumber;

	IcmpEchoHeader() : IcmpEchoHeader(0, 0) {}
	IcmpEchoHeader(uint16_t identifier, uint16_t sequenceNumber) :
		identifier(identifier), sequenceNumber(sequenceNumber) {}

	constexpr static size_t SerializedLength()
	{
		return sizeof(identifier) + sizeof(sequenceNumber);
	}

	size_t Serialize(uint8_t* buffer)
	{
		size_t i = 0;
		buffer[i++] = identifier >> 8;
		buffer[i++] = identifier;
		buffer[i++] = sequenceNumber >> 8;
		buffer[i++] = sequenceNumber;
		return i;
	}

	static IcmpEchoHeader Deserialize(const uint8_t* buffer)
	{
		IcmpEchoHeader self;
		self.identifier = buffer[0] << 8 | buffer[1];
		self.sequenceNumber = buffer[2] << 8 | buffer[3];
		return self;
	}
};
