#include <string>
#include <fstream>

#define TEST_NO_MAIN
#include "acutest.h"

#include <src/net-arp.h>
#include <src/types.h>
#include <uspi.h>

#include "common.h"
#include "net-arp.h"

using namespace Net;

static const auto targetMac = Utils::MacAddress{1, 2, 3, 4, 5, 6};
static const auto senderMac = Utils::MacAddress{11, 12, 13, 14, 15, 16};
static const uint32_t targetIp = 0xCAFE0000;
static const uint32_t senderIp = 0x00C0FFEE;

static void netArpCheckSentPacket(
	const uint16_t operation, const Utils::MacAddress targetMac, const uint32_t targetIp)
{
	// Check if SendPacket set the ethernet header correctly
	Ethernet::Header ethernetHeader;
	auto size = Ethernet::Header::Deserialize(ethernetHeader, uspiBuffer, USPI_FRAME_BUFFER_SIZE);
	TEST_CHECK(size == Ethernet::Header::SerializedLength());

	TEST_CHECK(ethernetHeader.macSource == senderMac);
	TEST_CHECK(ethernetHeader.macDestination == targetMac);

	// Check if the ARP packet fields were set correctly.
	Arp::Packet packet;
	size = packet.Deserialize(
		uspiBuffer + Ethernet::Header::SerializedLength(), USPI_FRAME_BUFFER_SIZE - size);
	TEST_CHECK(size == Arp::Packet::SerializedLength());

	TEST_CHECK(packet.hardwareType == 1);
	TEST_CHECK(packet.protocolType == Ethernet::EtherType::Ipv4);
	TEST_CHECK(packet.hardwareAddressLength == 6);
	TEST_CHECK(packet.protocolAddressLength == 4);
	TEST_CHECK(packet.operation == operation);
	TEST_CHECK(packet.senderMac == senderMac);
	TEST_CHECK(packet.senderIp == senderIp);
	TEST_CHECK(packet.targetMac == targetMac);
	TEST_CHECK(packet.targetIp == targetIp);
}

void TestNetArpPacketSerializeDeserialize()
{
	constexpr auto expectedSize = Arp::Packet::SerializedLength();
	const Arp::Packet packet(Arp::ARP_OPERATION_REQUEST);

	// Serialize
	uint8_t buffer[expectedSize];
	auto size = packet.Serialize(buffer, expectedSize);
	TEST_CHECK(size == expectedSize);

	// Deserialize
	Arp::Packet deserialized;
	size = deserialized.Deserialize(buffer, expectedSize);
	TEST_CHECK(size == expectedSize);

	// Check if the packet was deserialized correctly
	TEST_CHECK(packet.hardwareType == deserialized.hardwareType);
	TEST_CHECK(packet.protocolType == deserialized.protocolType);
	TEST_CHECK(packet.hardwareAddressLength == deserialized.hardwareAddressLength);
	TEST_CHECK(packet.protocolAddressLength == deserialized.protocolAddressLength);
	TEST_CHECK(packet.operation == deserialized.operation);
	TEST_CHECK(packet.senderMac == deserialized.senderMac);
	TEST_CHECK(packet.senderIp == deserialized.senderIp);
	TEST_CHECK(packet.targetMac == deserialized.targetMac);
	TEST_CHECK(packet.targetIp == deserialized.targetIp);

	// Check serialization and deserialization with a too small buffer
	TEST_CHECK(packet.Serialize(buffer, expectedSize - 1) == 0);
	TEST_CHECK(deserialized.Deserialize(buffer, expectedSize - 1) == 0);
}

void TestNetArpSendPacket()
{
	Arp::SendPacket(Arp::ARP_OPERATION_REQUEST, targetMac, senderMac, targetIp, senderIp);
	netArpCheckSentPacket(Arp::ARP_OPERATION_REQUEST, targetMac, targetIp);
}

void TestNetArpSendRequest()
{
	Arp::SendRequest(targetMac, senderMac, targetIp, senderIp);
	netArpCheckSentPacket(Arp::ARP_OPERATION_REQUEST, targetMac, targetIp);
}

void TestNetArpSendReply()
{
	Arp::SendReply(targetMac, senderMac, targetIp, senderIp);
	netArpCheckSentPacket(Arp::ARP_OPERATION_REPLY, targetMac, targetIp);
}

void TestNetArpSendAnnouncement()
{
	Arp::SendAnnouncement(senderMac, senderIp);
	netArpCheckSentPacket(Arp::ARP_OPERATION_REPLY, Utils::MacBroadcast, senderIp);
}

void loadFrame(const std::string path)
{
	std::ifstream stream(path);
	stream.read(reinterpret_cast<char*>(uspiBuffer), sizeof(uspiBuffer));
}

void TestNetArpHandlePacketInvalid()
{
	Ethernet::Header ethernetHeader;

	std::array<uint8_t, USPI_FRAME_BUFFER_SIZE> bufferRef = {};
	auto buffer = bufferRef;

	Arp::HandlePacket(ethernetHeader, buffer.data(), Arp::Packet::SerializedLength() - 1);
	TEST_CHECK(buffer == bufferRef);

	Arp::Packet reference(Arp::ARP_OPERATION_REQUEST);
	reference.targetIp = Utils::Ipv4Address;

	{
		auto packet = reference;
		packet.hardwareType = 2;
		const auto size = packet.Serialize(bufferRef.data(), bufferRef.size());

		buffer = bufferRef;
		Arp::HandlePacket(ethernetHeader, buffer.data(), size);
		TEST_CHECK(buffer == bufferRef);
	}

	{
		auto packet = reference;
		packet.protocolType = static_cast<Ethernet::EtherType>(1337);
		const auto size = packet.Serialize(bufferRef.data(), bufferRef.size());

		buffer = bufferRef;
		Arp::HandlePacket(ethernetHeader, buffer.data(), size);
		TEST_CHECK(buffer == bufferRef);
	}

	{
		auto packet = reference;
		packet.targetIp = 0xFEEDFEED;
		const auto size = packet.Serialize(bufferRef.data(), bufferRef.size());

		buffer = bufferRef;
		Arp::HandlePacket(ethernetHeader, buffer.data(), size);
		TEST_CHECK(buffer == bufferRef);
	}

	{
		auto packet = reference;
		packet.operation = static_cast<Arp::Operation>(31337);
		const auto size = packet.Serialize(bufferRef.data(), bufferRef.size());

		buffer = bufferRef;
		Arp::HandlePacket(ethernetHeader, buffer.data(), size);
		TEST_CHECK(buffer == bufferRef);
	}
}

void TestNetArpHandlePacketRequest()
{
	Ethernet::Header ethernetHeader;
	
	std::array<uint8_t, USPI_FRAME_BUFFER_SIZE> buffer;
	Arp::Packet reference(Arp::ARP_OPERATION_REQUEST);
	reference.targetIp = Utils::Ipv4Address;
	
	{
		auto packet = reference;
		const auto size = packet.Serialize(buffer.data(), buffer.size());
		const auto bufferRef = buffer;
		
		Arp::HandlePacket(ethernetHeader, buffer.data(), size);

		// Check if we've got a proper response
		TEST_CHECK(buffer != bufferRef);
	}
}
