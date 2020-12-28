#pragma once
#include <unordered_map>
#include "net-ethernet.h"
#include "net-utils.h"

namespace Net::Arp
{
	enum Operation
	{
		ARP_OPERATION_REQUEST = 1,
		ARP_OPERATION_REPLY = 2,
	};

	struct Packet
	{
		uint16_t hardwareType;
		uint16_t protocolType;
		uint8_t hardwareAddressLength;
		uint8_t protocolAddressLength;
		uint16_t operation;

		Utils::MacAddress senderMac;
		uint32_t senderIp;
		Utils::MacAddress targetMac;
		uint32_t targetIp;

		Packet();
		Packet(uint16_t operation);

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

		static Packet Deserialize(const uint8_t* buffer);
	};

	void HandlePacket(Ethernet::Header header, uint8_t* buffer);

	void SendPacket(
		Operation operation,
		Utils::MacAddress targetMac,
		Utils::MacAddress senderMac,
		uint32_t senderIp,
		uint32_t targetIp
	);

	void SendRequest(
		Utils::MacAddress targetMac,
		Utils::MacAddress senderMac,
		uint32_t senderIp,
		uint32_t targetIp
	);

	void SendReply(
		Utils::MacAddress targetMac,
		Utils::MacAddress senderMac,
		uint32_t senderIp,
		uint32_t targetIp
	);

	void SendAnnouncement(Utils::MacAddress mac, uint32_t ip);

	extern std::unordered_map<uint32_t, Utils::MacAddress> ArpTable;
}; // namespace Net::Arp
