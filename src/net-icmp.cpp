#include <cassert>
#include <cstdio>
#include <cstring>

#include "net-icmp.h"
#include "net-utils.h"

#include "debug.h"
#include "types.h"
#include <uspi.h>

namespace Net::Icmp
{
	//
	// Header
	//
	Header::Header() : type(static_cast<Type>(0)), code(0), checksum(0) {}
	Header::Header(Type type, uint8_t code) : type(type), code(code), checksum(0) {}

	size_t Header::Serialize(
		uint8_t* buffer, const size_t bufferSize, const uint8_t* data, const size_t dataSize) const
	{
		if (bufferSize < SerializedLength())
		{
			return 0;
		}

		size_t i = 0;
		buffer[i++] = static_cast<uint8_t>(type);
		buffer[i++] = code;
		buffer[i++] = 0;
		buffer[i++] = 0;

		std::memcpy(buffer + i, data, dataSize);
		i += dataSize;

		uint16_t checksum = Utils::InternetChecksum(buffer, i);
		buffer[2] = checksum >> 8;
		buffer[3] = checksum;

		return i;
	}

	size_t Header::Deserialize(Header& out, const uint8_t* buffer, const size_t bufferSize)
	{
		if (bufferSize < SerializedLength())
		{
			return 0;
		}

		out.type = static_cast<Type>(buffer[0]);
		out.code = buffer[1];
		out.checksum = buffer[2] << 8 | buffer[3];
		return 4;
	}

	//
	// EchoHeader
	//
	EchoHeader::EchoHeader() : EchoHeader(0, 0) {}
	EchoHeader::EchoHeader(uint16_t identifier, uint16_t sequenceNumber) :
		identifier(identifier), sequenceNumber(sequenceNumber)
	{
	}

	size_t EchoHeader::Serialize(uint8_t* buffer, const size_t bufferSize) const
	{
		if (bufferSize < SerializedLength())
		{
			return 0;
		}

		size_t i = 0;
		buffer[i++] = identifier >> 8;
		buffer[i++] = identifier;
		buffer[i++] = sequenceNumber >> 8;
		buffer[i++] = sequenceNumber;
		return i;
	}

	size_t EchoHeader::Deserialize(EchoHeader& out, const uint8_t* buffer, const size_t bufferSize)
	{
		if (bufferSize < SerializedLength())
		{
			return 0;
		}

		out.identifier = buffer[0] << 8 | buffer[1];
		out.sequenceNumber = buffer[2] << 8 | buffer[3];
		return 4;
	}

	void SendEchoRequest(Utils::MacAddress mac, uint32_t ip)
	{
		Icmp::Header icmpHeader(Icmp::Type::EchoRequest, 0);
		Icmp::EchoHeader pingHeader(0, 0);

		size_t ipv4TotalSize = Icmp::Header::SerializedLength() +
			Icmp::EchoHeader::SerializedLength() + Ipv4::Header::SerializedLength();
		Ipv4::Header ipv4Header(Ipv4::Protocol::Icmp, Utils::Ipv4Address, ip, ipv4TotalSize);

		Ethernet::Header ethernetHeader(mac, Utils::GetMacAddress(), Ethernet::EtherType::Ipv4);

		uint8_t buffer[USPI_FRAME_BUFFER_SIZE];
		size_t size = 0;

		const uint8_t emptyBuffer[1] = {};

		size += ethernetHeader.Serialize(buffer + size, sizeof(buffer) - size);
		size += ipv4Header.Serialize(buffer + size, sizeof(buffer) - size);
		size += pingHeader.Serialize(buffer + size, sizeof(buffer) - size);
		size += icmpHeader.Serialize(buffer + size, sizeof(buffer) - size, emptyBuffer, 0);

		const auto expectedSize = ethernetHeader.SerializedLength() +
			ipv4Header.SerializedLength() + pingHeader.SerializedLength() +
			icmpHeader.SerializedLength();
		assert(size == expectedSize);
		assert(size <= sizeof(buffer));

		USPiSendFrame(buffer, size);
	}

	static void handleEchoRequest(
		const Ethernet::Header& reqEthernetHeader,
		const Ipv4::Header& reqIpv4Header,
		const Icmp::Header& reqIcmpHeader,
		const uint8_t* reqBuffer,
		const size_t reqBufferSize)
	{
		const Ethernet::Header respEthernetHeader(
			reqEthernetHeader.macSource, Utils::GetMacAddress(), Ethernet::EtherType::Ipv4);
		const Ipv4::Header respIpv4Header(
			Ipv4::Protocol::Icmp,
			Utils::Ipv4Address,
			reqIpv4Header.sourceIp,
			reqIpv4Header.totalLength);
		Header respIcmpHeader(Type::EchoReply, 0);

		const auto payloadSize = reqIpv4Header.totalLength - reqIpv4Header.SerializedLength() -
			reqIcmpHeader.SerializedLength();

		DEBUG_LOG("payloadSize: %u\r\n", payloadSize);

		std::array<uint8_t, USPI_FRAME_BUFFER_SIZE> respBuffer;

		size_t respSize = 0;
		respSize += respEthernetHeader.Serialize(
			respBuffer.data() + respSize, respBuffer.size() - respSize);
		respSize +=
			respIpv4Header.Serialize(respBuffer.data() + respSize, respBuffer.size() - respSize);
		respSize += respIcmpHeader.Serialize(
			respBuffer.data() + respSize, respBuffer.size() - respSize, reqBuffer, payloadSize);

		const auto expectedRespSize = respEthernetHeader.SerializedLength() +
			respIpv4Header.SerializedLength() + respIcmpHeader.SerializedLength() + payloadSize;
		assert(respSize == expectedRespSize);
		assert(respSize <= respBuffer.size());

		USPiSendFrame(respBuffer.data(), respSize);
	}

	void HandlePacket(
		Ethernet::Header ethernetHeader,
		Ipv4::Header ipv4Header,
		const uint8_t* buffer,
		const size_t bufferSize)
	{
		Header icmpHeader;
		size_t headerSize = Icmp::Header::Deserialize(icmpHeader, buffer, bufferSize);
		if (headerSize != Icmp::Header::SerializedLength())
		{
			DEBUG_LOG(
				"Dropped ICMP packet (invalid buffer size %u, expected at least %u)\r\n",
				bufferSize,
				Icmp::Header::SerializedLength());
		}

		DEBUG_LOG("Got ICMP header type %u\r\n", static_cast<uint8_t>(icmpHeader.type));

		if (icmpHeader.type == Type::EchoRequest)
		{
			handleEchoRequest(
				ethernetHeader,
				ipv4Header,
				icmpHeader,
				buffer + headerSize,
				bufferSize - headerSize);
		}
	}
} // namespace Net::Icmp
