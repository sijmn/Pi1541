#include <cstring>
#include "net-ethernet.h"

namespace Net::Ethernet {
	EthernetFrameHeader::EthernetFrameHeader()
	{}

	EthernetFrameHeader::EthernetFrameHeader(std::uint16_t type) :
		macDestination(Net::Utils::MacBroadcast),
		macSource{0, 0, 0, 0, 0, 0},
		type(type)
	{}

	EthernetFrameHeader::EthernetFrameHeader(
		MacAddress macSource, uint16_t type
	) :
		macDestination(Net::Utils::MacBroadcast),
		macSource(macSource),
		type(type)
	{}

	EthernetFrameHeader::EthernetFrameHeader(
		MacAddress macDestination, MacAddress macSource, uint16_t type
	) :
		macDestination(macDestination),
		macSource(macSource),
		type(type)
	{}

	std::size_t EthernetFrameHeader::Serialize(uint8_t* buffer) const
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

	EthernetFrameHeader EthernetFrameHeader::Deserialize(const uint8_t* buffer)
	{
		EthernetFrameHeader self;
		std::memcpy(self.macDestination.data(), buffer + 0, self.macDestination.size());
		std::memcpy(self.macSource.data(), buffer + 6, self.macSource.size());
		self.type = buffer[12] << 8 | buffer[13];
		return self;
	}
}; // namespace Net::Ethernet
