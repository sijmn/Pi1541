#include <cassert>
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
		size_t size = 0;

		size += ethernetHeader.Serialize(buffer + size, sizeof(buffer) - size);
		size += ipv4Header.Serialize(buffer + size);
		size += pingHeader.Serialize(buffer + size);
		size += icmpHeader.Serialize(buffer + 1);

		const auto expectedSize =
			ethernetHeader.SerializedLength() +
			ipv4Header.SerializedLength() +
			pingHeader.SerializedLength() +
			icmpHeader.SerializedLength();
		assert(size == expectedSize);
		assert(size <= sizeof(buffer));

		USPiSendFrame(buffer, size);
	}

	static void handleEchoRequest(
		const Ethernet::Header& reqEthernetHeader,
		const Ipv4::Header& reqIpv4Header,
		const Icmp::PacketHeader& reqIcmpHeader,
		const uint8_t* reqBuffer,
		const size_t reqBufferSize
	) {
		const auto reqEchoHeader = Icmp::EchoHeader::Deserialize(reqBuffer);
		const auto reqHeaderSize = reqEchoHeader.SerializedLength();

		const Icmp::PacketHeader respIcmpHeader(Icmp::Type::EchoReply, 0);
		const Ipv4::Header respIpv4Header(
			Ipv4::Protocol::Icmp,
			Utils::Ipv4Address,
			reqIpv4Header.sourceIp,
			reqIpv4Header.totalLength
		);
		const Ethernet::Header respEthernetHeader(
			reqEthernetHeader.macSource,
			Utils::GetMacAddress(),
			Ethernet::EtherType::Ipv4
		);

		const auto payloadLength =
			reqIpv4Header.totalLength -
			reqIpv4Header.SerializedLength() -
			reqIcmpHeader.SerializedLength() -
			reqEchoHeader.SerializedLength();

		std::array<uint8_t, USPI_FRAME_BUFFER_SIZE> respBuffer;

		size_t respSize = 0;
		respSize += respEthernetHeader.Serialize(
			respBuffer.data() + respSize, respBuffer.size() - respSize);
		respSize += respIpv4Header.Serialize(respBuffer.data() + respSize);
		respSize += respIcmpHeader.Serialize(respBuffer.data() + respSize);
		std::memcpy(
			respBuffer.data() + respSize,
			reqBuffer + reqHeaderSize,
			payloadLength
		);
		respSize += payloadLength;

		const auto expectedRespSize =
			respEthernetHeader.SerializedLength() +
			respIpv4Header.SerializedLength() +
			respIcmpHeader.SerializedLength() +
			payloadLength;
		assert(respSize == expectedRespSize);
		assert(respSize <= respBuffer.size());

		USPiSendFrame(respBuffer.data(), respSize);
	}

	void HandlePacket(const uint8_t* buffer, const size_t bufferSize)
	{
		// TODO Don't re-parse the upper layers
		size_t headerSize = 0;

		Ethernet::Header ethernetHeader;
		headerSize += Ethernet::Header::Deserialize(
			ethernetHeader, buffer + headerSize, bufferSize - headerSize);

		const auto ipv4Header = Ipv4::Header::Deserialize(buffer + headerSize);
		headerSize += ipv4Header.SerializedLength();

		const auto icmpHeader = Icmp::PacketHeader::Deserialize(buffer + headerSize);
		headerSize += icmpHeader.SerializedLength();

		const auto expectedHeaderSize =
			ethernetHeader.SerializedLength() +
			ipv4Header.SerializedLength() +
			icmpHeader.SerializedLength();
		assert(headerSize == expectedHeaderSize);

		if (icmpHeader.type == Icmp::Type::EchoRequest)
		{
			handleEchoRequest(
				ethernetHeader,
				ipv4Header,
				icmpHeader,
				buffer + headerSize,
				bufferSize - headerSize
			);
		}
	}
} // namespace Net::Icmp
