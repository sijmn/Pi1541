#include "net-udp.h"
#include "net-dhcp.h"
#include "net-tftp.h"

namespace Net::Udp
{
	Header::Header()
	{}

	Header::Header(
		Port sourcePort,
		Port destinationPort,
		uint16_t length
	) :
		sourcePort(sourcePort),
		destinationPort(destinationPort),
		length(length),
		checksum(0)
	{}

	size_t Header::Serialize(uint8_t* buffer) const
	{
		size_t i = 0;
		buffer[i++] = static_cast<uint16_t>(sourcePort) >> 8;
		buffer[i++] = static_cast<uint16_t>(sourcePort);
		buffer[i++] = static_cast<uint16_t>(destinationPort) >> 8;
		buffer[i++] = static_cast<uint16_t>(destinationPort);
		buffer[i++] = length >> 8;
		buffer[i++] = length;
		buffer[i++] = checksum >> 8;
		buffer[i++] = checksum;
		return i;
	}

	Header Header::Deserialize(const uint8_t* buffer)
	{
		Header self;
		self.sourcePort = static_cast<Port>(buffer[0] << 8 | buffer[1]);
		self.destinationPort = static_cast<Port>(buffer[2] << 8 | buffer[3]);
		self.length = buffer[4] << 8 | buffer[5];
		self.checksum = buffer[6] << 8 | buffer[7];
		return self;
	}

	void HandlePacket(
		const Ethernet::Header ethernetHeader,
		const Ipv4Header ipv4Header,
		const uint8_t* buffer,
		const size_t size
	) {
		const auto udpHeader = Header::Deserialize(buffer);

		if (udpHeader.destinationPort == Port::DhcpClient)
		{
			Dhcp::HandlePacket(
				ethernetHeader,
				buffer + udpHeader.SerializedLength(),
				size - udpHeader.SerializedLength()
			);
		}
		else if (udpHeader.destinationPort == Port::Tftp)
		{
			Tftp::HandlePacket(
				ethernetHeader,
				ipv4Header,
				udpHeader,
				buffer + udpHeader.SerializedLength()
			);
		}
	}
}; // namespace Net::Udp
