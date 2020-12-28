#pragma once
#include <array>
#include "net-utils.h"

namespace Net::Ethernet
{
	using Utils::MacAddress;

	enum EtherType
	{
		ETHERTYPE_IPV4 = 0x0800,
		ETHERTYPE_ARP = 0x0806,
	};

	struct Header
	{
		MacAddress macDestination;
		MacAddress macSource;
		uint16_t type;

		Header();
		Header(uint16_t type);
		Header(MacAddress macSource, uint16_t type);
		Header(MacAddress macDestination, MacAddress macSource, uint16_t type);

		constexpr static size_t SerializedLength()
		{
			return sizeof(macDestination) + sizeof(macSource) + sizeof(type);
		}

		size_t Serialize(uint8_t* buffer) const;
		static Header Deserialize(const uint8_t* buffer);
	};
}; // namespace Net::Ethernet
