#pragma once
#include <cstddef>
#include <cstdint>

#include "net-ethernet.h"

namespace Net
{
	namespace Ipv4
	{
		enum class Protocol : uint8_t
		{
			Icmp = 1,
			Tcp = 6,
			Udp = 17,
		};

		struct Header
		{
			uint8_t version;
			uint8_t ihl;
			uint8_t dscp;
			uint8_t ecn;
			uint16_t totalLength;
			uint16_t identification;
			uint8_t flags;
			uint16_t fragmentOffset;
			uint8_t ttl;
			Protocol protocol;
			uint16_t headerChecksum;
			uint32_t sourceIp;
			uint32_t destinationIp;

			Header();
			Header(
				Protocol protocol, uint32_t sourceIp, uint32_t destinationIp, uint16_t totalLength);

			static constexpr size_t SerializedLength()
			{
				// Hardcoded because of bitfields.
				return 20;
			}

			size_t Serialize(uint8_t* buffer, const size_t bufferSize) const;
			static size_t Deserialize(Header& out, const uint8_t* buffer, const size_t bufferSize);
		};

		void HandlePacket(
			const Ethernet::Header& ethernetHeader, const uint8_t* buffer, const size_t bufferSize);
	} // namespace Ipv4
} // namespace Net
