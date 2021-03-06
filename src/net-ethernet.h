#pragma once
#include <array>

#include "net-utils.h"

namespace Net
{
	namespace Ethernet
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

			size_t Serialize(uint8_t* buffer, const size_t size) const;
			static size_t Deserialize(Header& out, const uint8_t* buffer, const size_t size);
		};
	} // namespace Ethernet
} // namespace Net
