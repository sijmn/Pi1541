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

	UdpDatagramHeader();
	UdpDatagramHeader(uint16_t sourcePort, uint16_t destinationPort, uint16_t length);

	static constexpr size_t SerializedLength()
	{
		return
			sizeof(sourcePort) +
			sizeof(destinationPort) +
			sizeof(length) +
			sizeof(checksum);
	}

	size_t Serialize(uint8_t* buffer);
	static UdpDatagramHeader Deserialize(const uint8_t* buffer);
};
