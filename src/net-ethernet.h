#pragma once
#include "net.h"

struct EthernetFrameHeader
{
	MacAddress macDestination;
	MacAddress macSource;
	std::uint16_t type;

	EthernetFrameHeader(std::uint16_t type) :
		macDestination{255, 255, 255, 255, 255, 255},
		macSource{0, 0, 0, 0, 0, 0},
		type(type)
	{
	}

	EthernetFrameHeader() : EthernetFrameHeader(0) {}

	std::size_t Serialize(uint8_t* buffer)
	{
		std::size_t i = 0;

		std::memcpy(buffer + i, macDestination.data(), macDestination.size());
		i += sizeof(macDestination);

		std::memcpy(buffer + i, macSource.data(), macSource.size());
		i += sizeof(macSource);

		buffer[i++] = type >> 8;
		buffer[i++] = type;

		return i;
	}

	static EthernetFrameHeader Deserialize(uint8_t* buffer)
	{
		EthernetFrameHeader self;
		memcpy(self.macDestination.data(), buffer + 0, self.macDestination.size());
		memcpy(self.macSource.data(), buffer + 6, self.macSource.size());
		self.type = buffer[12] << 8 | buffer[13];
		return self;
	}
} __attribute__((packed));


template <class T>
struct EthernetFrame
{
	EthernetFrameHeader header;
	T payload;
	std::uint32_t crc;

	EthernetFrame() {}

	EthernetFrame(std::uint16_t type, T payload) : header(type), payload(payload), crc(0)
	{
	}

	std::size_t Serialize(uint8_t* buffer)
	{
		std::size_t i = 0;
		i += header.Serialize(buffer);

		std::size_t payload_size = payload.Serialize(buffer + i);
		i += payload_size;

		// Pad data to 46 bytes
		for (; payload_size < 46; payload_size++) {
			buffer[i++] = 0;
		}

		crc = Crc32(buffer, i);

		buffer[i++] = crc;
		buffer[i++] = crc >> 8;
		buffer[i++] = crc >> 16;
		buffer[i++] = crc >> 24;

		return i;
	}

	static EthernetFrame<T> Deserialize(uint8_t* buffer)
	{
		EthernetFrame<T> self;

		self.header = EthernetFrameHeader::Deserialize(buffer);
		size_t i = sizeof(EthernetFrameHeader);

		// XXX Might want to base this on actual deserialized data, might not match.
		std::size_t payloadSize = sizeof(T);
		self.payload = T::Deserialize(buffer + i);

		// Skip the padding
		i += std::max(payloadSize, std::size_t{46});

		self.crc =
			buffer[i] << 24 | buffer[i + 1] << 16 | buffer[i + 2] << 8 | buffer[i + 3];
		i += 4;

		return self;
	}
} __attribute__((packed));
