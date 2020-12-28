#pragma once
#include "net.h"

namespace Net::Icmp
{
	enum class Type : uint8_t
	{
		EchoReply = 0,
		EchoRequest = 8,
	};

	struct PacketHeader
	{
		Type type;
		uint8_t code;
		uint16_t checksum;

		PacketHeader();
		PacketHeader(Type type, uint8_t code);

		constexpr static size_t SerializedLength()
		{
			return sizeof(type) + sizeof(code) + sizeof(checksum);
		}

		size_t Serialize(uint8_t* buffer) const;
		static PacketHeader Deserialize(const uint8_t* buffer);
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

		size_t Serialize(uint8_t* buffer) const;
		static EchoHeader Deserialize(const uint8_t* buffer);
	};

	void SendEchoRequest(Utils::MacAddress mac, uint32_t ip);
	void HandlePacket(const uint8_t* buffer);
} // namespace Net::Icmp
