#pragma once
#include <vector>
#include <string>
#include <cstdint>
#include "net-ethernet.h"
#include "net-ipv4.h"

namespace Net::Udp
{
	enum class Port : uint16_t
	{
		DhcpServer = 67,
		DhcpClient = 68,
		Tftp = 69, // nice
	};

	struct Header
	{
		Port sourcePort;
		Port destinationPort;
		uint16_t length;
		uint16_t checksum;

		Header();
		Header(Port sourcePort, Port destinationPort, uint16_t length);

		static constexpr size_t SerializedLength()
		{
			return
				sizeof(sourcePort) +
				sizeof(destinationPort) +
				sizeof(length) +
				sizeof(checksum);
		}

		size_t Serialize(uint8_t* buffer) const;
		static Header Deserialize(const uint8_t* buffer);
	};

	void HandlePacket(
		const Ethernet::Header ethernetHeader,
		const Ipv4::Header ipv4Header,
		const uint8_t* buffer,
		const size_t size
	);
} // namespace Net::Udp
