#include <cassert>
#include <cstring>

#include "net-arp.h"
#include "net-ethernet.h"

#include "debug.h"
#include "types.h"
#include <uspi.h>

namespace Net::Arp
{
	Packet::Packet() {}

	Packet::Packet(const uint16_t operation) :
		hardwareType(1), // Ethernet
		protocolType(Ethernet::EtherType::Ipv4),
		hardwareAddressLength(6),
		protocolAddressLength(4),
		operation(operation)
	{
	}

	size_t Packet::Serialize(uint8_t* buffer, const size_t bufferSize)
	{
		if (bufferSize < SerializedLength())
		{
			return 0;
		}

		buffer[0] = hardwareType >> 8;
		buffer[1] = hardwareType;
		buffer[2] = static_cast<uint16_t>(protocolType) >> 8;
		buffer[3] = static_cast<uint16_t>(protocolType);
		buffer[4] = hardwareAddressLength;
		buffer[5] = protocolAddressLength;
		buffer[6] = operation >> 8;
		buffer[7] = operation;

		memcpy(buffer + 8, senderMac.data(), 6);

		buffer[14] = senderIp >> 24;
		buffer[15] = senderIp >> 16;
		buffer[16] = senderIp >> 8;
		buffer[17] = senderIp;

		memcpy(buffer + 18, targetMac.data(), 6);

		buffer[24] = targetIp >> 24;
		buffer[25] = targetIp >> 16;
		buffer[26] = targetIp >> 8;
		buffer[27] = targetIp;

		return 28;
	}

	// Static
	size_t Packet::Deserialize(const uint8_t* buffer, const size_t bufferSize)
	{
		if (bufferSize < SerializedLength())
		{
			return 0;
		}

		hardwareType = buffer[0] << 8 | buffer[1];
		protocolType = static_cast<Ethernet::EtherType>(buffer[2] << 8 | buffer[3]);
		hardwareAddressLength = buffer[4];
		protocolAddressLength = buffer[5];
		operation = buffer[6] << 8 | buffer[7];

		memcpy(senderMac.data(), buffer + 8, 6);
		senderIp = buffer[14] << 24 | buffer[15] << 16 | buffer[16] << 8 | buffer[17];
		memcpy(targetMac.data(), buffer + 18, 6);
		targetIp = buffer[24] << 24 | buffer[25] << 16 | buffer[26] << 8 | buffer[27];

		return 28;
	}

	void SendPacket(
		const Operation operation,
		const Utils::MacAddress targetMac,
		const Utils::MacAddress senderMac,
		const uint32_t targetIp,
		const uint32_t senderIp)
	{
		Packet arpPacket(operation);
		arpPacket.targetMac = targetMac;
		arpPacket.senderMac = senderMac;
		arpPacket.targetIp = targetIp;
		arpPacket.senderIp = senderIp;

		Ethernet::Header ethernetHeader(senderMac, targetMac, Ethernet::EtherType::Arp);

		uint8_t buffer[USPI_FRAME_BUFFER_SIZE];
		size_t size = 0;
		size += ethernetHeader.Serialize(buffer + size, sizeof(buffer) - size);
		size += arpPacket.Serialize(buffer + size, sizeof(buffer) - size);

		const auto expectedSize = ethernetHeader.SerializedLength() + arpPacket.SerializedLength();
		assert(size == expectedSize);
		assert(size <= sizeof(buffer));

		USPiSendFrame(buffer, size);
	}

	void SendRequest(
		const Utils::MacAddress targetMac,
		const Utils::MacAddress senderMac,
		const uint32_t targetIp,
		const uint32_t senderIp)
	{
		SendPacket(ARP_OPERATION_REQUEST, targetMac, senderMac, targetIp, senderIp);
	}

	void SendReply(
		const Utils::MacAddress targetMac,
		const Utils::MacAddress senderMac,
		const uint32_t targetIp,
		const uint32_t senderIp)
	{
		SendPacket(ARP_OPERATION_REPLY, targetMac, senderMac, targetIp, senderIp);
	}

	void SendAnnouncement(const Utils::MacAddress mac, const uint32_t ip)
	{
		SendReply(Utils::MacBroadcast, mac, ip, ip);
	}

	void HandlePacket(
		const Ethernet::Header ethernetHeader, const uint8_t* buffer, const size_t bufferSize)
	{
		const auto macAddress = Utils::GetMacAddress();

		Packet arpPacket;
		size_t arpSize = arpPacket.Deserialize(buffer, bufferSize);
		if (arpSize == 0 || arpSize != arpPacket.SerializedLength())
		{
			DEBUG_LOG(
				"Dropped ARP packet (invalid buffer size %lu, expected %lu)\r\n",
				bufferSize,
				arpPacket.SerializedLength());
			return;
		}

		if (arpPacket.hardwareType != 1 || arpPacket.protocolType != Ethernet::EtherType::Ipv4 ||
			arpPacket.targetIp != Utils::Ipv4Address)
		{
			// Might want to disable because of spamminess
			DEBUG_LOG("Dropped ARP packet (invalid parameters)\r\n");
			return;
		}

		switch (arpPacket.operation)
		{
		case ARP_OPERATION_REQUEST:
			SendReply(arpPacket.senderMac, macAddress, arpPacket.senderIp, Utils::Ipv4Address);
			break;

		case ARP_OPERATION_REPLY:
			ArpTable.insert(std::make_pair(arpPacket.senderIp, arpPacket.senderMac));
			break;

		default:
			DEBUG_LOG("Dropped ARP packet (invalid operation %d)\r\n", arpPacket.operation);
			break;
		}
	}

	std::unordered_map<uint32_t, Utils::MacAddress> ArpTable;
} // namespace Net::Arp
