#pragma once
#include "net.h"

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
