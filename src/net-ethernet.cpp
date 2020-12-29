#include <cstring>

#include "net-ethernet.h"

namespace Net::Ethernet
{
	Header::Header() {}

	Header::Header(EtherType type) :
		macDestination(Utils::MacBroadcast), macSource{0, 0, 0, 0, 0, 0}, type(type)
	{
	}

	Header::Header(MacAddress macSource, EtherType type) :
		macDestination(Utils::MacBroadcast), macSource(macSource), type(type)
	{
	}

	Header::Header(MacAddress macDestination, MacAddress macSource, EtherType type) :
		macDestination(macDestination), macSource(macSource), type(type)
	{
	}

	size_t Header::Serialize(uint8_t* buffer, const size_t size) const
	{
		if (size < SerializedLength())
		{
			return 0;
		}

		size_t i = 0;

		std::memcpy(buffer + i, macDestination.data(), macDestination.size());
		i += sizeof(macDestination);

		std::memcpy(buffer + i, macSource.data(), macSource.size());
		i += sizeof(macSource);

		buffer[i++] = static_cast<uint16_t>(type) >> 8;
		buffer[i++] = static_cast<uint16_t>(type);

		return i;
	}

	size_t Header::Deserialize(Header& out, const uint8_t* buffer, const size_t size)
	{
		if (size < SerializedLength())
		{
			return 0;
		}

		std::memcpy(out.macDestination.data(), buffer + 0, out.macDestination.size());
		std::memcpy(out.macSource.data(), buffer + 6, out.macSource.size());
		out.type = static_cast<EtherType>(buffer[12] << 8 | buffer[13]);
		return 14;
	}
} // namespace Net::Ethernet
