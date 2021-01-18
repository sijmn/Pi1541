#include <cassert>

#include "net-arp.h"
#include "net-ethernet.h"
#include "net-icmp.h"
#include "net-ipv4.h"
#include "net-udp.h"
#include "net-utils.h"

#include "debug.h"

#include "types.h"
#include <cstring>
#include <uspi.h>

namespace Net
{
	namespace Ipv4
	{
		Header::Header() {}

		Header::Header(
			Protocol protocol, uint32_t sourceIp, uint32_t destinationIp, uint16_t totalLength) :
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
		{
		}

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
			buffer[i++] = 0;

			buffer[i++] = sourceIp >> 24;
			buffer[i++] = sourceIp >> 16;
			buffer[i++] = sourceIp >> 8;
			buffer[i++] = sourceIp;
			buffer[i++] = destinationIp >> 24;
			buffer[i++] = destinationIp >> 16;
			buffer[i++] = destinationIp >> 8;
			buffer[i++] = destinationIp;

			uint16_t checksum = Net::Utils::InternetChecksum(buffer, i);
			buffer[10] = checksum >> 8;
			buffer[11] = checksum;

			return i;
		}

		size_t Header::Deserialize(Header& out, const uint8_t* buffer, const size_t bufferSize)
		{
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
			out.destinationIp = buffer[16] << 24 | buffer[17] << 16 | buffer[18] << 8 | buffer[19];

			return 20;
		}

		void HandlePacket(
			const Ethernet::Header& ethernetHeader, const uint8_t* buffer, const size_t bufferSize)
		{
			Header header;
			const auto headerSize = Header::Deserialize(header, buffer, bufferSize);
			if (headerSize != Header::SerializedLength())
			{
				DEBUG_LOG(
					"Dropped IPv4 header (invalid buffer size %u, expected at least %u)\r\n",
					bufferSize,
					headerSize);
				return;
			}
			DEBUG_LOG(
				"IPv4 { src=%08lx, dst=%08lx, len=%u, protocol=%u }\r\n",
				header.sourceIp,
				header.destinationIp,
				header.totalLength,
				static_cast<uint8_t>(header.protocol));
			if (bufferSize < header.totalLength)
			{
				DEBUG_LOG(
					"Dropped IPv4 packet (invalid buffer size %u, expected at least %u)\r\n",
					bufferSize,
					header.totalLength);
				return;
			}

			// Update ARP table
			Arp::ArpTable.insert(std::make_pair(header.sourceIp, ethernetHeader.macSource));

			if (header.version != 4)
			{
				DEBUG_LOG(
					"Dropped IPv4 packet (invalid header version %u, expected 4)\r\n",
					header.version);
				return;
			}
			if (header.ihl != 5)
			{
				// Not supported
				DEBUG_LOG("Dropped IPv4 packet (unsupported IHL %u, expected 5)\r\n", header.ihl);
				return;
			}
			if (header.destinationIp != Utils::Ipv4Address)
			{
				DEBUG_LOG(
					"Dropped IPv4 packet (invalid destination IP address %08lx)\r\n",
					header.destinationIp);
				return;
			}
			if (header.fragmentOffset != 0)
			{
				// TODO Support this
				DEBUG_LOG(
					"Dropped IPv4 packet (unexpected fragment offset %u, expected 0)\r\n",
					header.fragmentOffset);
				return;
			}

			if (header.protocol == Ipv4::Protocol::Icmp)
			{
				DEBUG_LOG("Ethernet -> IPv4 -> ICMP\r\n");
				Icmp::HandlePacket(
					ethernetHeader, header, buffer + headerSize, bufferSize - headerSize);
			}
			else if (header.protocol == Ipv4::Protocol::Udp)
			{
				DEBUG_LOG("Ethernet -> IPv4 -> UDP\r\n");
				Udp::HandlePacket(
					ethernetHeader, header, buffer + headerSize, bufferSize - headerSize);
			}
		}
	} // namespace Ipv4
} // namespace Net
