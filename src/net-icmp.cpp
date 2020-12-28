#include <cstring>

#include "net-icmp.h"

#include "types.h"
#include <uspi.h>

namespace Net::Icmp
{
	//
	// PacketHeader
	//
	PacketHeader::PacketHeader() {}

	PacketHeader::PacketHeader(Type type, uint8_t code) :
		type(type), code(code), checksum(0)
	{}

	size_t PacketHeader::Serialize(uint8_t* buffer) const
	{
		size_t i = 0;
		buffer[i++] = static_cast<uint8_t>(type);
		buffer[i++] = code;
		buffer[i++] = checksum;
		buffer[i++] = checksum >> 8;
		return i;
	}

	PacketHeader PacketHeader::Deserialize(const uint8_t* buffer)
	{
		PacketHeader self;
		self.type = static_cast<Type>(buffer[0]);
		self.code = buffer[1];
		self.checksum = buffer[2] << 8 | buffer[3];
		return self;
	}

	//
	// EchoHeader
	//
	EchoHeader::EchoHeader() : EchoHeader(0, 0) {}
	EchoHeader::EchoHeader(uint16_t identifier, uint16_t sequenceNumber) :
		identifier(identifier), sequenceNumber(sequenceNumber) {}

	size_t EchoHeader::Serialize(uint8_t* buffer) const
	{
		size_t i = 0;
		buffer[i++] = identifier >> 8;
		buffer[i++] = identifier;
		buffer[i++] = sequenceNumber >> 8;
		buffer[i++] = sequenceNumber;
		return i;
	}

	EchoHeader EchoHeader::Deserialize(const uint8_t* buffer)
	{
		EchoHeader self;
		self.identifier = buffer[0] << 8 | buffer[1];
		self.sequenceNumber = buffer[2] << 8 | buffer[3];
		return self;
	}

	void SendEchoRequest(Utils::MacAddress mac, uint32_t ip)
	{
		Icmp::PacketHeader icmpHeader(Icmp::Type::EchoRequest, 0);
		Icmp::EchoHeader pingHeader(0, 0);

		size_t ipv4TotalSize = Icmp::PacketHeader::SerializedLength() +
			Icmp::EchoHeader::SerializedLength() +
			Ipv4::Header::SerializedLength();
		Ipv4::Header ipv4Header(
			Ipv4::Protocol::Icmp, Utils::Ipv4Address, ip, ipv4TotalSize);

		Ethernet::Header ethernetHeader(
			mac, Utils::GetMacAddress(), Ethernet::EtherType::Ipv4);

		uint8_t buffer[USPI_FRAME_BUFFER_SIZE];
		size_t i = 0;

		i += ethernetHeader.Serialize(buffer + i);
		i += ipv4Header.Serialize(buffer + i);
		i += pingHeader.Serialize(buffer + i);
		i += icmpHeader.Serialize(buffer + 1);

		USPiSendFrame(buffer, i);
	}

	void HandlePacket(const uint8_t* buffer)
	{
		// TODO Don't re-parse the upper layers
		size_t requestSize = 0;
		const auto requestEthernetHeader =
			Ethernet::Header::Deserialize(buffer + requestSize);
		requestSize += requestEthernetHeader.SerializedLength();
		const auto requestIpv4Header = Ipv4::Header::Deserialize(buffer + requestSize);
		requestSize += requestIpv4Header.SerializedLength();
		const auto requestIcmpHeader =
			Icmp::PacketHeader::Deserialize(buffer + requestSize);
		requestSize += requestIcmpHeader.SerializedLength();

		if (requestIcmpHeader.type == Icmp::Type::EchoRequest)
		{
			const auto requestEchoHeader =
				Icmp::EchoHeader::Deserialize(buffer + requestSize);
			requestSize += requestEchoHeader.SerializedLength();

			const Icmp::PacketHeader responseIcmpHeader(
				Icmp::Type::EchoReply, 0);
			const Ipv4::Header responseIpv4Header(
				Ipv4::Protocol::Icmp,
				Utils::Ipv4Address,
				requestIpv4Header.sourceIp,
				requestIpv4Header.totalLength
			);
			const Ethernet::Header responseEthernetHeader(
				requestEthernetHeader.macSource,
				Utils::GetMacAddress(),
				Ethernet::EtherType::Ipv4
			);

			const auto payloadLength =
				requestIpv4Header.totalLength -
				requestIpv4Header.SerializedLength() -
				requestIcmpHeader.SerializedLength() -
				requestEchoHeader.SerializedLength();

			std::array<uint8_t, USPI_FRAME_BUFFER_SIZE> bufferResp;
			size_t respSize = 0;
			respSize += responseEthernetHeader.Serialize(bufferResp.data() + respSize);
			respSize += responseIpv4Header.Serialize(bufferResp.data() + respSize);
			respSize += responseIcmpHeader.Serialize(bufferResp.data() + respSize);
			std::memcpy(
				bufferResp.data() + respSize, buffer + requestSize, payloadLength);
			respSize += payloadLength;
			USPiSendFrame(bufferResp.data(), respSize);
		}
	}
} // namespace Net::Icmp
