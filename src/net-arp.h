#pragma once
#include <unordered_map>
#include "net-ethernet.h"
#include "net-utils.h"

namespace Net::Arp
{
	using Net::Utils::MacAddress;

	enum ArpOperation {
		ARP_OPERATION_REQUEST = 1,
		ARP_OPERATION_REPLY = 2,
	};

	struct Ipv4ArpPacket
	{
		uint16_t hardwareType;
		uint16_t protocolType;
		uint8_t hardwareAddressLength;
		uint8_t protocolAddressLength;
		uint16_t operation;

		MacAddress senderMac;
		uint32_t senderIp;
		MacAddress targetMac;
		uint32_t targetIp;

		Ipv4ArpPacket();
		Ipv4ArpPacket(uint16_t operation);

		constexpr size_t SerializedLength() const
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

		size_t Serialize(uint8_t* buffer);

		static Ipv4ArpPacket Deserialize(const uint8_t* buffer);
	};

	void HandlePacket(Net::Ethernet::EthernetFrameHeader header, uint8_t* buffer);

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

	extern std::unordered_map<uint32_t, MacAddress> ArpTable;
}; // namespace Net::Arp
