#pragma once
#include <unordered_map>

#include "net-ethernet.h"
#include "net-utils.h"

namespace Net
{
	namespace Arp
	{
		enum Operation
		{
			ARP_OPERATION_REQUEST = 1,
			ARP_OPERATION_REPLY = 2,
		};

		struct Packet
		{
			uint16_t hardwareType;
			Ethernet::EtherType protocolType;
			uint8_t hardwareAddressLength;
			uint8_t protocolAddressLength;
			uint16_t operation;

			Utils::MacAddress senderMac;
			uint32_t senderIp;
			Utils::MacAddress targetMac;
			uint32_t targetIp;

			Packet();
			Packet(const uint16_t operation);

			constexpr static size_t SerializedLength()
			{
				return sizeof(hardwareType) + sizeof(protocolType) + sizeof(hardwareAddressLength) +
					sizeof(protocolAddressLength) + sizeof(operation) + sizeof(senderMac) +
					sizeof(senderIp) + sizeof(targetMac) + sizeof(targetIp);
			}

			size_t Serialize(uint8_t* buffer, const size_t bufferSize) const;
			size_t Deserialize(const uint8_t* buffer, const size_t bufferSize);
		};

		void
		HandlePacket(const Ethernet::Header header, const uint8_t* buffer, const size_t bufferSize);

		void SendPacket(
			const Operation operation,
			const Utils::MacAddress targetMac,
			const Utils::MacAddress senderMac,
			const uint32_t targetIp,
			const uint32_t senderIp);

		void SendRequest(
			const Utils::MacAddress targetMac,
			const Utils::MacAddress senderMac,
			const uint32_t targetIp,
			const uint32_t senderIp);

		void SendReply(
			const Utils::MacAddress targetMac,
			const Utils::MacAddress senderMac,
			const uint32_t targetIp,
			const uint32_t senderIp);

		void SendAnnouncement(const Utils::MacAddress mac, const uint32_t ip);

		extern std::unordered_map<uint32_t, Utils::MacAddress> ArpTable;
	} // namespace Arp
} // namespace Net
