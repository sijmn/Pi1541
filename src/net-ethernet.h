#pragma once
#include "net.h"

struct EthernetFrameHeader
{
	MacAddress macDestination;
	MacAddress macSource;
	std::uint16_t type;

	EthernetFrameHeader();
	EthernetFrameHeader(std::uint16_t type);
	EthernetFrameHeader(MacAddress macSource, uint16_t type);
	EthernetFrameHeader(MacAddress macDestination, MacAddress macSource, uint16_t type);

	constexpr static std::size_t SerializedLength()
	{
		return sizeof(macDestination) + sizeof(macSource) + sizeof(type);
	}

	std::size_t Serialize(uint8_t* buffer) const;
	static EthernetFrameHeader Deserialize(const uint8_t* buffer);
};
