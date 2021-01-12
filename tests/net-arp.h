void TestNetArpPacket()
{
	using namespace Net::Arp;

	constexpr auto size = Packet::SerializedLength();
	auto packet = Packet(ARP_OPERATION_REQUEST);

	uint8_t buffer[size];
	packet.Serialize(buffer, sizeof(buffer));

	Packet deserialized;
	deserialized.Deserialize(buffer, sizeof(buffer));

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
