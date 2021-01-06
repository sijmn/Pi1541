#include "net-udp.h"
#include "net-dhcp.h"
#include "net-tftp.h"

#include "debug.h"

namespace Net::Udp
{
	Header::Header() {}

	Header::Header(Port sourcePort, Port destinationPort, uint16_t length) :
		sourcePort(sourcePort), destinationPort(destinationPort), length(length), checksum(0)
	{
	}

	size_t Header::Serialize(uint8_t* buffer, const size_t bufferSize) const
	{
		if (bufferSize < SerializedLength())
		{
			return 0;
		}

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

	size_t Header::Deserialize(const uint8_t* buffer, const size_t bufferSize)
	{
		if (bufferSize < Header::SerializedLength())
		{
			return 0;
		}

		sourcePort = static_cast<Port>(buffer[0] << 8 | buffer[1]);
		destinationPort = static_cast<Port>(buffer[2] << 8 | buffer[3]);
		length = buffer[4] << 8 | buffer[5];
		checksum = buffer[6] << 8 | buffer[7];
		return 8;
	}

	void HandlePacket(
		const Ethernet::Header ethernetHeader,
		const Ipv4::Header ipv4Header,
		const uint8_t* buffer,
		const size_t bufferSize)
	{
		Header udpHeader;
		const auto headerSize = udpHeader.Deserialize(buffer, bufferSize);
		if (headerSize == 0 || headerSize != udpHeader.SerializedLength())
		{
			DEBUG_LOG(
				"Dropped UDP header (invalid buffer size %u, expected at least %u)\r\n",
				bufferSize,
				Header::SerializedLength());
			return;
		}
		if (udpHeader.length <= bufferSize)
		{
			DEBUG_LOG(
				"Dropped UDP packet (invalid buffer size %u, expected at least %u)\r\n",
				bufferSize,
				udpHeader.length);
			return;
		}

		if (udpHeader.destinationPort == Port::DhcpClient)
		{
			Dhcp::HandlePacket(
				ethernetHeader,
				buffer + udpHeader.SerializedLength(),
				bufferSize - udpHeader.SerializedLength());
		}
		else if (udpHeader.destinationPort == Port::Tftp)
		{
			Tftp::HandlePacket(
				ethernetHeader,
				ipv4Header,
				udpHeader,
				buffer + udpHeader.SerializedLength(),
				bufferSize - udpHeader.SerializedLength());
		}
	}
} // namespace Net::Udp
