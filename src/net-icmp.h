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

	IcmpPacketHeader();
	IcmpPacketHeader(std::uint8_t type, std::uint8_t code);

	constexpr static std::size_t SerializedLength()
	{
		return sizeof(type) + sizeof(code) + sizeof(checksum);
	}

	std::size_t Serialize(uint8_t* buffer) const;
	static IcmpPacketHeader Deserialize(const uint8_t* buffer);
};

struct IcmpEchoHeader
{
	uint16_t identifier;
	uint16_t sequenceNumber;

	IcmpEchoHeader();
	IcmpEchoHeader(uint16_t identifier, uint16_t sequenceNumber);

	constexpr static size_t SerializedLength()
	{
		return sizeof(identifier) + sizeof(sequenceNumber);
	}

	size_t Serialize(uint8_t* buffer) const;
	static IcmpEchoHeader Deserialize(const uint8_t* buffer);
};
