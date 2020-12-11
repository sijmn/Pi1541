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
} __attribute__((packed));


template <class T>
struct IcmpPacket
{
	IcmpPacketHeader header;
	T payload;

	IcmpPacket() {}

	IcmpPacket(std::uint8_t type, std::uint8_t code) :
		header(type, code)
	{}

	IcmpPacket(std::uint8_t type, std::uint8_t code, T payload) :
		header(type, code), payload(payload)
	{}

	std::size_t Serialize(uint8_t* buffer)
	{
		std::size_t i = 0;

		header.checksum = 0;
		i += header.Serialize(buffer);
		i += payload.Serialize(buffer + i);

		uint16_t checksum = InternetChecksum(buffer, i);
		buffer[2] = checksum;
		buffer[3] = checksum >> 8;

		return i;
	}

	static IcmpPacket<T> Deserialize(const uint8_t* buffer)
	{
		IcmpPacket<T> self;
		self.header = IcmpPacketHeader::Deserialize(buffer);
		self.payload = T::Deserialize(buffer + sizeof(IcmpPacketHeader));
		return self;
	}
} __attribute__((packed));

template <class T>
struct IcmpEchoRequest
{
	uint16_t identifier;
	uint16_t sequenceNumber;
	T data;

	IcmpEchoRequest() {}

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

	static IcmpEchoRequest<T> Deserialize(const uint8_t* buffer)
	{
		IcmpEchoRequest self;
		self.identifier = buffer[0] << 8 | buffer[1];
		self.sequenceNumber = buffer[2] << 8 | buffer[3];
		memcpy(self.data, buffer + 4, sizeof(T));
		return self;
	}
} __attribute__((packed));

template <>
struct IcmpEchoRequest<void>
{
	uint16_t identifier;
	uint16_t sequenceNumber;

	IcmpEchoRequest() : identifier(0), sequenceNumber(0)
	{}

	size_t Serialize(uint8_t* buffer)
	{
		size_t i = 0;
		buffer[i++] = identifier >> 8;
		buffer[i++] = identifier;
		buffer[i++] = sequenceNumber >> 8;
		buffer[i++] = sequenceNumber;
		return i;
	}

	static IcmpEchoRequest Deserialize(const uint8_t* buffer)
	{
		IcmpEchoRequest self;
		self.identifier = buffer[0] << 8 | buffer[1];
		self.sequenceNumber = buffer[2] << 8 | buffer[3];
		return self;
	}
} __attribute__((packed));

template <class T>
struct IcmpEchoResponse
{
	uint16_t identifier;
	uint16_t sequenceNumber;
	T data;

	IcmpEchoResponse() {}

	IcmpEchoResponse(T data) : identifier(0), sequenceNumber(0), data(data)
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

	static IcmpEchoResponse<T> Deserialize(const uint8_t* buffer)
	{
		IcmpEchoResponse self;
		self.identifier = buffer[0] << 8 | buffer[1];
		self.sequenceNumber = buffer[2] << 8 | buffer[3];
		memcpy(self.data, buffer + 4, sizeof(T));
		return self;
	}
} __attribute__((packed));
