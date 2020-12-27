#include <memory>

#include "ff.h"
#include "net-arp.h"
#include "net-ethernet.h"
#include "net-icmp.h"
#include "net-ipv4.h"
#include "net-udp.h"
#include "net-dhcp.h"
#include "net-tftp.h"
#include "net.h"
#include "types.h"

#include <uspi.h>
#include <uspios.h>

//
// IPv4
//
void HandleIpv4Packet(
	const Net::Ethernet::EthernetFrameHeader ethernetHeader,
	const uint8_t* buffer,
	const size_t size
) {
	const auto ipv4Header = Ipv4Header::Deserialize(buffer);
	const auto offset = Ipv4Header::SerializedLength();

	// Update ARP table
	Net::Arp::ArpTable.insert(
		std::make_pair(ipv4Header.sourceIp, ethernetHeader.macSource));

	if (ipv4Header.version != 4) return;
	if (ipv4Header.ihl != 5) return; // Not supported
	if (ipv4Header.destinationIp != Net::Utils::Ipv4Address) return;
	if (ipv4Header.fragmentOffset != 0) return; // TODO Support this

	if (ipv4Header.protocol == IP_PROTO_ICMP)
	{
		HandleIcmpFrame(buffer);
	}
	else if (ipv4Header.protocol == IP_PROTO_UDP)
	{
		HandleUdpDatagram(ethernetHeader, ipv4Header, buffer + offset, size - offset);
	}
}

//
// UDP
//
void HandleUdpDatagram(
	const Net::Ethernet::EthernetFrameHeader ethernetHeader,
	const Ipv4Header ipv4Header,
	const uint8_t* buffer,
	const size_t size
) {
	const auto udpHeader = UdpDatagramHeader::Deserialize(buffer);

	if (udpHeader.destinationPort == UDP_PORT_DHCP_CLIENT)
	{
		Net::Dhcp::HandlePacket(
			ethernetHeader,
			buffer + udpHeader.SerializedLength(),
			size - udpHeader.SerializedLength()
		);
	}
	else if (udpHeader.destinationPort == UDP_PORT_TFTP)
	{
		Net::Tftp::HandlePacket(
			ethernetHeader,
			ipv4Header,
			udpHeader,
			buffer + udpHeader.SerializedLength()
		);
	}
}

//
// ICMP
//
void SendIcmpEchoRequest(MacAddress mac, uint32_t ip)
{
	IcmpPacketHeader icmpHeader(8, 0);
	IcmpEchoHeader pingHeader(0, 0);

	size_t ipv4TotalSize = IcmpPacketHeader::SerializedLength() +
		IcmpEchoHeader::SerializedLength() +
		Ipv4Header::SerializedLength();
	Ipv4Header ipv4Header(1, Net::Utils::Ipv4Address, ip, ipv4TotalSize);

	Net::Ethernet::EthernetFrameHeader ethernetHeader(
		mac, Net::Utils::GetMacAddress(), Net::Ethernet::ETHERTYPE_IPV4);

	uint8_t buffer[USPI_FRAME_BUFFER_SIZE];
	size_t i = 0;

	i += ethernetHeader.Serialize(buffer + i);
	i += ipv4Header.Serialize(buffer + i);
	i += pingHeader.Serialize(buffer + i);
	i += icmpHeader.Serialize(buffer + 1);

	USPiSendFrame(buffer, i);
}

void HandleIcmpFrame(const uint8_t* buffer)
{
	// TODO Don't re-parse the upper layers
	size_t requestSize = 0;
	const auto requestEthernetHeader =
		Net::Ethernet::EthernetFrameHeader::Deserialize(buffer + requestSize);
	requestSize += requestEthernetHeader.SerializedLength();
	const auto requestIpv4Header = Ipv4Header::Deserialize(buffer + requestSize);
	requestSize += requestIpv4Header.SerializedLength();
	const auto requestIcmpHeader = IcmpPacketHeader::Deserialize(buffer + requestSize);
	requestSize += requestIcmpHeader.SerializedLength();

	if (requestIcmpHeader.type == ICMP_ECHO_REQUEST)
	{
		const auto requestEchoHeader =
			IcmpEchoHeader::Deserialize(buffer + requestSize);
		requestSize += requestEchoHeader.SerializedLength();

		const IcmpPacketHeader responseIcmpHeader(ICMP_ECHO_REPLY, 0);
		const Ipv4Header responseIpv4Header(
			IP_PROTO_ICMP,
			Net::Utils::Ipv4Address,
			requestIpv4Header.sourceIp,
			requestIpv4Header.totalLength
		);
		const Net::Ethernet::EthernetFrameHeader responseEthernetHeader(
			requestEthernetHeader.macSource,
			Net::Utils::GetMacAddress(),
			Net::Ethernet::ETHERTYPE_IPV4
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
		memcpy(bufferResp.data() + respSize, buffer + requestSize, payloadLength);
		respSize += payloadLength;
		USPiSendFrame(bufferResp.data(), respSize);
	}
}

//
// Helpers
//



bool FileUploaded = false;
