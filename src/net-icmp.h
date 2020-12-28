#pragma once
#include "net.h"

namespace Net::Icmp
{
	enum class Type : uint8_t
	{
		EchoReply = 0,
		EchoRequest = 8,
	};

	struct Header
	{
		Type type;
		uint8_t code;
		uint16_t checksum;

		Header();
		Header(Type type, uint8_t code);

		constexpr static size_t SerializedLength()
		{
			return sizeof(type) + sizeof(code) + sizeof(checksum);
		}

		size_t Serialize(uint8_t* buffer, const size_t bufferSize) const;
		static size_t Deserialize(
			Header& out, const uint8_t* buffer, const size_t bufferSize);
	};

	struct EchoHeader
	{
		uint16_t identifier;
		uint16_t sequenceNumber;

		EchoHeader();
		EchoHeader(uint16_t identifier, uint16_t sequenceNumber);

		constexpr static size_t SerializedLength()
		{
			return sizeof(identifier) + sizeof(sequenceNumber);
		}

		size_t Serialize(uint8_t* buffer, const size_t bufferSize) const;
		static size_t Deserialize(
			EchoHeader& out, const uint8_t* buffer, const size_t bufferSize);
	};

	void SendEchoRequest(const Utils::MacAddress mac, const uint32_t ip);
	void HandlePacket(const uint8_t* buffer, const size_t bufferSize);
} // namespace Net::Icmp
