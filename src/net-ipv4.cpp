#include <cassert>

#include "net-ipv4.h"
#include "net-ethernet.h"
#include "net-arp.h"
#include "net-icmp.h"
#include "net-udp.h"
#include "net-utils.h"

#include "debug.h"

namespace Net::Ipv4
{
	Header::Header() {}

	Header::Header(
		Protocol protocol, uint32_t sourceIp, uint32_t destinationIp, uint16_t totalLength
	) :
		version(4),
		ihl(5),
		dscp(0),
		ecn(0),
		totalLength(totalLength),
		identification(0),
		flags(0),
		fragmentOffset(0),
		ttl(64),
		protocol(protocol),
		headerChecksum(0),
		sourceIp(sourceIp),
		destinationIp(destinationIp)
	{}

	size_t Header::Serialize(uint8_t* buffer, const size_t bufferSize) const
	{
		if (bufferSize <= SerializedLength())
		{
			return 0;
		}

		size_t i = 0;
		buffer[i++] = version << 4 | ihl;
		buffer[i++] = dscp << 2 | ecn;
		buffer[i++] = totalLength >> 8;
		buffer[i++] = totalLength;
		buffer[i++] = identification >> 8;
		buffer[i++] = identification;
		buffer[i++] = (flags << 13 | fragmentOffset) >> 8;
		buffer[i++] = flags << 13 | fragmentOffset;
		buffer[i++] = ttl;
		buffer[i++] = static_cast<uint8_t>(protocol);

		// Zero the checksum before calculating it
		buffer[i++] = 0;
		buffer[i++] = 0 >> 8;

		buffer[i++] = sourceIp >> 24;
		buffer[i++] = sourceIp >> 16;
		buffer[i++] = sourceIp >> 8;
		buffer[i++] = sourceIp;
		buffer[i++] = destinationIp >> 24;
		buffer[i++] = destinationIp >> 16;
		buffer[i++] = destinationIp >> 8;
		buffer[i++] = destinationIp;

		uint16_t checksum = Net::Utils::InternetChecksum(buffer, i);
		buffer[10] = checksum;
		buffer[11] = checksum >> 8;

		return i;
	}

	size_t Header::Deserialize(
		Header& out, const uint8_t* buffer, const size_t bufferSize
	) {
		if (bufferSize <= SerializedLength())
		{
			return 0;
		}

		out.version = buffer[0] >> 4;
		out.ihl = buffer[0] & 0x0F;

		out.dscp = buffer[1] >> 2;
		out.ecn = buffer[1] & 0x03;

		out.totalLength = buffer[2] << 8 | buffer[3];
		out.identification = buffer[4] << 8 | buffer[5];

		out.flags = buffer[6] >> 5;
		out.fragmentOffset = (buffer[6] & 0x1F) << 8 | buffer[7];

		out.ttl = buffer[8];
		out.protocol = static_cast<Protocol>(buffer[9]);
		out.headerChecksum = buffer[10] << 8 | buffer[11];

		out.sourceIp = buffer[12] << 24 | buffer[13] << 16 | buffer[14] << 8 | buffer[15];
		out.destinationIp =
			buffer[16] << 24 | buffer[17] << 16 | buffer[18] << 8 | buffer[19];

		return 20;
	}

	void HandlePacket(
		const Ethernet::Header& ethernetHeader,
		const uint8_t* buffer,
		const size_t bufferSize
	) {
		Header header;
		const auto headerSize = Header::Deserialize(header, buffer, bufferSize);
		if (headerSize != Header::SerializedLength())
		{
			DEBUG_LOG(
				"Dropped IPv4 packet (invalid buffer size %lu, expected at least %lu)\r\n"
				bufferSize, headerSize
			);
			return;
		}

		// Update ARP table
		Arp::ArpTable.insert(
			std::make_pair(header.sourceIp, ethernetHeader.macSource));

		if (header.version != 4) return;
		if (header.ihl != 5) return; // Not supported
		if (header.destinationIp != Utils::Ipv4Address) return;
		if (header.fragmentOffset != 0) return; // TODO Support this

		if (header.protocol == Ipv4::Protocol::Icmp)
		{
			Icmp::HandlePacket(buffer, bufferSize - headerSize);
		}
		else if (header.protocol == Ipv4::Protocol::Udp)
		{
			Udp::HandlePacket(
				ethernetHeader, header, buffer + headerSize, bufferSize - headerSize);
		}
	}
} // namespace Net::Ipv4
