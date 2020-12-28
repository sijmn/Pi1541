#pragma once
#include <array>
#include "net-utils.h"

namespace Net::Ethernet
{
	using Utils::MacAddress;

	enum class EtherType : uint16_t
	{
		Ipv4 = 0x0800,
		Arp = 0x0806,
	};

	struct Header
	{
		MacAddress macDestination;
		MacAddress macSource;
		EtherType type;

		Header();
		Header(EtherType type);
		Header(MacAddress macSource, EtherType type);
		Header(MacAddress macDestination, MacAddress macSource, EtherType type);

		constexpr static size_t SerializedLength()
		{
			return sizeof(macDestination) + sizeof(macSource) + sizeof(type);
		}

		size_t Serialize(uint8_t* buffer) const;
		static Header Deserialize(const uint8_t* buffer);
	};
} // namespace Net::Ethernet
