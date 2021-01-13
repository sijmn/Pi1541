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

void TestNetArpPacket()
{
	constexpr auto expectedSize = Arp::Packet::SerializedLength();
	const auto packet = Arp::Packet(Arp::ARP_OPERATION_REQUEST);

	uint8_t buffer[expectedSize];
	packet.Serialize(buffer, sizeof(buffer));

	Arp::Packet deserialized;
	const auto size = deserialized.Deserialize(buffer, sizeof(buffer));
	TEST_CHECK(size == expectedSize);

	TEST_CHECK(packet.hardwareType == deserialized.hardwareType);
	TEST_CHECK(packet.protocolType == deserialized.protocolType);
	TEST_CHECK(packet.hardwareAddressLength == deserialized.hardwareAddressLength);
	TEST_CHECK(packet.protocolAddressLength == deserialized.protocolAddressLength);
	TEST_CHECK(packet.operation == deserialized.operation);
	TEST_CHECK(packet.senderMac == deserialized.senderMac);
	TEST_CHECK(packet.senderIp == deserialized.senderIp);
	TEST_CHECK(packet.targetMac == deserialized.targetMac);
	TEST_CHECK(packet.targetIp == deserialized.targetIp);
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
