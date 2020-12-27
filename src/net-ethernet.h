#pragma once
#include <array>
#include "net-utils.h"

using Net::Utils::MacAddress;

namespace Net::Ethernet
{
	enum EtherType
	{
		ETHERTYPE_IPV4 = 0x0800,
		ETHERTYPE_ARP = 0x0806,
	};

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
}; // namespace Net::Ethernet
