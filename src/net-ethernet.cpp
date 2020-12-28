#include <cstring>
#include "net-ethernet.h"

namespace Net::Ethernet
{
	Header::Header()
	{}

	Header::Header(uint16_t type) :
		macDestination(Utils::MacBroadcast),
		macSource{0, 0, 0, 0, 0, 0},
		type(type)
	{}

	Header::Header(
		MacAddress macSource, uint16_t type
	) :
		macDestination(Utils::MacBroadcast),
		macSource(macSource),
		type(type)
	{}

	Header::Header(
		MacAddress macDestination, MacAddress macSource, uint16_t type
	) :
		macDestination(macDestination),
		macSource(macSource),
		type(type)
	{}

	size_t Header::Serialize(uint8_t* buffer) const
	{
		size_t i = 0;

		std::memcpy(buffer + i, macDestination.data(), macDestination.size());
		i += sizeof(macDestination);

		std::memcpy(buffer + i, macSource.data(), macSource.size());
		i += sizeof(macSource);

		buffer[i++] = type >> 8;
		buffer[i++] = type;

		return i;
	}

	Header Header::Deserialize(const uint8_t* buffer)
	{
		Header self;
		std::memcpy(self.macDestination.data(), buffer + 0, self.macDestination.size());
		std::memcpy(self.macSource.data(), buffer + 6, self.macSource.size());
		self.type = buffer[12] << 8 | buffer[13];
		return self;
	}
}; // namespace Net::Ethernet
