#pragma once
#include <vector>
#include <string>
#include <cstdint>

struct UdpDatagramHeader
{
	uint16_t sourcePort;
	uint16_t destinationPort;
	uint16_t length;
	uint16_t checksum;

	UdpDatagramHeader() {}

	UdpDatagramHeader(uint16_t sourcePort, uint16_t destinationPort, uint16_t length) :
		sourcePort(sourcePort),
		destinationPort(destinationPort),
		length(length),
		checksum(0)
	{}

	static constexpr size_t SerializedLength()
	{
		return
			sizeof(sourcePort) +
			sizeof(destinationPort) +
			sizeof(length) +
			sizeof(checksum);
	}

	size_t Serialize(uint8_t* buffer)
	{
		size_t i = 0;
		buffer[i++] = sourcePort >> 8;
		buffer[i++] = sourcePort;
		buffer[i++] = destinationPort >> 8;
		buffer[i++] = destinationPort;
		buffer[i++] = length >> 8;
		buffer[i++] = length;
		buffer[i++] = checksum >> 8;
		buffer[i++] = checksum;
		return i;
	}

	static UdpDatagramHeader Deserialize(const uint8_t* buffer)
	{
		UdpDatagramHeader self;
		self.sourcePort = buffer[0] << 8 | buffer[1];
		self.destinationPort = buffer[2] << 8 | buffer[3];
		self.length = buffer[4] << 8 | buffer[5];
		self.checksum = buffer[6] << 8 | buffer[7];
		return self;
	}
} __attribute__((packed));

template <class T>
struct UdpDatagram
{
	UdpDatagramHeader header;
	T payload;

	UdpDatagram() {}

	UdpDatagram(uint16_t sourcePort, uint16_t destinationPort, T payload) :
		header(sourcePort, destinationPort, sizeof(UdpDatagram<T>)),
		payload(payload)
	{}
} __attribute__((packed));
