#pragma once
#include "net.h"

namespace Net::Arp
{
	struct Ipv4ArpPacket
	{
		std::uint16_t hardwareType;
		std::uint16_t protocolType;
		std::uint8_t hardwareAddressLength;
		std::uint8_t protocolAddressLength;
		std::uint16_t operation;

		MacAddress senderMac;
		std::uint32_t senderIp;
		MacAddress targetMac;
		std::uint32_t targetIp;

		Ipv4ArpPacket();
		Ipv4ArpPacket(std::uint16_t operation);

		constexpr std::size_t SerializedLength() const
		{
			return
				sizeof(hardwareType) +
				sizeof(protocolType) +
				sizeof(hardwareAddressLength) +
				sizeof(protocolAddressLength) +
				sizeof(operation) +
				senderMac.size() +
				sizeof(senderIp) +
				targetMac.size() +
				sizeof(targetIp);
		}

		std::size_t Serialize(std::uint8_t* buffer);

		static Ipv4ArpPacket Deserialize(const uint8_t* buffer);
	};

	void HandlePacket(EthernetFrameHeader header, uint8_t* buffer);

	void SendPacket(
		ArpOperation operation,
		MacAddress targetMac,
		MacAddress senderMac,
		uint32_t senderIp,
		uint32_t targetIp
	);

	void SendRequest(
		MacAddress targetMac,
		MacAddress senderMac,
		uint32_t senderIp,
		uint32_t targetIp
	);

	void SendReply(
		MacAddress targetMac,
		MacAddress senderMac,
		uint32_t senderIp,
		uint32_t targetIp
	);

	void SendAnnouncement(MacAddress mac, uint32_t ip);

	extern std::unordered_map<std::uint32_t, MacAddress> ArpTable;
}; // namespace Net::Arp
