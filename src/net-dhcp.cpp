#include <cassert>
#include <cstring>
#include <random>

#include "net-dhcp.h"
#include "net-ethernet.h"
#include "net-ipv4.h"
#include "net-udp.h"

#include "debug.h"
#include "types.h"
#include <uspi.h>

namespace Net
{
	namespace Dhcp
	{
		Header::Header() {}

		Header::Header(Opcode opcode, uint32_t transactionId) :
			opcode(opcode),
			hardwareAddressType(1), // Ethernet
			hops(0),
			transactionId(transactionId),
			secondsElapsed(0),
			flags(0), // TODO assumption
			clientIpAddress(0),
			yourIpAddress(0),
			serverIpAddress(0),
			relayIpAddress(0),
			clientHardwareAddress{0},
			serverHostname{0},
			bootFile{0},
			magicValue{99, 130, 83, 99}
		{
			const auto mac = Utils::GetMacAddress();
			hardwareAddressLength = mac.size();
			std::memcpy(clientHardwareAddress.data(), mac.data(), mac.size());
		}

		size_t Header::Serialize(uint8_t* buffer, const size_t size) const
		{
			if (size < Header::SerializedLength())
			{
				return 0;
			}

			size_t i = 0;
			buffer[i++] = static_cast<uint8_t>(opcode);
			buffer[i++] = hardwareAddressType;
			buffer[i++] = hardwareAddressLength;
			buffer[i++] = hops;
			buffer[i++] = transactionId >> 24;
			buffer[i++] = transactionId >> 16;
			buffer[i++] = transactionId >> 8;
			buffer[i++] = transactionId;
			buffer[i++] = secondsElapsed >> 8;
			buffer[i++] = secondsElapsed;
			buffer[i++] = flags >> 8;
			buffer[i++] = flags;
			buffer[i++] = clientIpAddress >> 24;
			buffer[i++] = clientIpAddress >> 16;
			buffer[i++] = clientIpAddress >> 8;
			buffer[i++] = clientIpAddress;
			buffer[i++] = yourIpAddress >> 24;
			buffer[i++] = yourIpAddress >> 16;
			buffer[i++] = yourIpAddress >> 8;
			buffer[i++] = yourIpAddress;
			buffer[i++] = relayIpAddress >> 24;
			buffer[i++] = relayIpAddress >> 16;
			buffer[i++] = relayIpAddress >> 8;
			buffer[i++] = relayIpAddress;

			std::memcpy(buffer + i, clientHardwareAddress.data(), clientHardwareAddress.size());
			i += clientHardwareAddress.size();

			std::memcpy(buffer + i, serverHostname.data(), serverHostname.size());
			i += serverHostname.size();

			std::memcpy(buffer + i, bootFile.data(), bootFile.size());
			i += bootFile.size();

			std::memcpy(buffer + i, magicValue.data(), magicValue.size());
			i += magicValue.size();

			return i;
		}

		size_t Header::Deserialize(Header& out, const uint8_t* buffer, const size_t size)
		{
			if (size < SerializedLength())
			{
				return 0;
			}

			out.opcode = static_cast<Opcode>(buffer[0]);
			out.hardwareAddressType = buffer[1];
			out.hardwareAddressLength = buffer[2];
			out.hops = buffer[3];
			out.transactionId = buffer[4] << 24 | buffer[5] << 16 | buffer[6] << 8 | buffer[7];
			out.secondsElapsed = buffer[8] << 8 | buffer[9];
			out.flags = buffer[10] << 8 | buffer[11];
			out.clientIpAddress =
				buffer[12] << 24 | buffer[13] << 16 | buffer[14] << 8 | buffer[15];
			out.yourIpAddress = buffer[16] << 24 | buffer[17] << 16 | buffer[18] << 8 | buffer[19];
			out.serverIpAddress =
				buffer[20] << 24 | buffer[21] << 16 | buffer[22] << 8 | buffer[23];
			out.relayIpAddress = buffer[24] << 24 | buffer[25] << 16 | buffer[26] << 8 | buffer[27];

			std::memcpy(
				out.clientHardwareAddress.data(), buffer + 28, out.clientHardwareAddress.size());
			std::memcpy(out.serverHostname.data(), buffer + 44, out.serverHostname.size());
			std::memcpy(out.bootFile.data(), buffer + 108, out.bootFile.size());
			std::memcpy(out.magicValue.data(), buffer + 236, out.magicValue.size());

			assert(SerializedLength() == 240);
			return 240;
		}

		static uint32_t transactionId;
		static std::vector<uint32_t> offeredIpAddresses;
		static std::vector<uint32_t> serverIpAddresses;
		static std::vector<Utils::MacAddress> serverMacAddresses;
		static bool serverSelected;

		void sendRequest(
			uint32_t clientIpAddress, Utils::MacAddress serverMacAddress, uint32_t serverIpAddress)
		{
			const Header dhcpHeader(Opcode::BootRequest, transactionId);

			size_t udpLength = dhcpHeader.SerializedLength() + Udp::Header::SerializedLength();
			const Udp::Header udpHeader(Udp::Port::DhcpClient, Udp::Port::DhcpServer, udpLength);

			size_t ipv4Length = udpLength + Ipv4::Header::SerializedLength();
			const Ipv4::Header ipv4Header(
				Ipv4::Protocol::Udp, clientIpAddress, serverIpAddress, ipv4Length);
			const Ethernet::Header ethernetHeader(
				serverMacAddress, Utils::GetMacAddress(), Ethernet::EtherType::Ipv4);

			uint8_t buffer[USPI_FRAME_BUFFER_SIZE];
			size_t size = 0;
			size += ethernetHeader.Serialize(buffer + size, sizeof(buffer) - size);
			size += ipv4Header.Serialize(buffer + size, sizeof(buffer) - size);
			size += udpHeader.Serialize(buffer + size, sizeof(buffer) - size);
			size += dhcpHeader.Serialize(buffer + size, sizeof(buffer) - size);

			const auto expectedSize = ethernetHeader.SerializedLength() +
				ipv4Header.SerializedLength() + udpHeader.SerializedLength() +
				dhcpHeader.SerializedLength();
			assert(size == expectedSize);
			assert(size <= sizeof(buffer));

			USPiSendFrame(buffer, size);
		}

		void discoverTimerHandler(unsigned int, void* callbackVoid, void*)
		{
			if (transactionId == 0 || offeredIpAddresses.empty())
			{
				// TODO retry every minute or so?
				return;
			}

			// Select the first IP address
			Utils::Ipv4Address = offeredIpAddresses[0];

			// Send DHCP Requests to every server with that IP address.
			for (size_t i = 0; i < serverIpAddresses.size(); i++)
			{
				sendRequest(Utils::Ipv4Address, serverMacAddresses[i], serverIpAddresses[i]);
			}

			// Run the callback indicating an IP has been obtained
			if (callbackVoid != nullptr)
			{
				const auto& callback = *static_cast<std::function<void()>*>(callbackVoid);
				callback();
			}
		}

		void sendDiscover()
		{
			transactionId = std::rand();
			offeredIpAddresses.clear();
			const Header dhcpHeader(Opcode::BootRequest, transactionId);

			size_t udpLength = dhcpHeader.SerializedLength() + Udp::Header::SerializedLength();
			const Udp::Header udpHeader(Udp::Port::DhcpClient, Udp::Port::DhcpServer, udpLength);

			size_t ipv4Length = udpLength + Ipv4::Header::SerializedLength();
			const Ipv4::Header ipv4Header(Ipv4::Protocol::Udp, 0, 0xFFFFFFFF, ipv4Length);
			const Ethernet::Header ethernetHeader(
				Utils::GetMacAddress(), Ethernet::EtherType::Ipv4);

			uint8_t buffer[USPI_FRAME_BUFFER_SIZE];
			size_t size = 0;

			size += ethernetHeader.Serialize(buffer + size, sizeof(buffer) - size);
			size += ipv4Header.Serialize(buffer + size, sizeof(buffer) - size);
			size += udpHeader.Serialize(buffer + size, sizeof(buffer) - size);
			size += dhcpHeader.Serialize(buffer + size, sizeof(buffer) - size);

			const auto expectedSize = ethernetHeader.SerializedLength() +
				ipv4Header.SerializedLength() + udpHeader.SerializedLength() +
				dhcpHeader.SerializedLength();
			assert(size == expectedSize);
			assert(size <= sizeof(buffer));

			USPiSendFrame(buffer, size);
		}

		void ObtainIp(std::function<void()>& callback)
		{
			sendDiscover();

			// Wait three seconds for responses
			// const auto callbackVoid = static_cast<void*>(&callback);
			// StartKernelTimer(3 * HZ, discoverTimerHandler, callbackVoid, nullptr);
		}

		static void
		handleOfferPacket(const Ethernet::Header ethernetHeader, const Header dhcpHeader)
		{
			offeredIpAddresses.push_back(dhcpHeader.yourIpAddress);
			serverIpAddresses.push_back(dhcpHeader.serverIpAddress);
			serverMacAddresses.push_back(ethernetHeader.macSource);
		}

		static void handleAckPacket(const Ethernet::Header ethernetHeader, const Header dhcpHeader)
		{
			Utils::Ipv4Address = dhcpHeader.yourIpAddress;

			// TODO Schedule handler for end of lease.

			transactionId = 0;
			offeredIpAddresses.clear();
			serverIpAddresses.clear();
			serverMacAddresses.clear();
			serverSelected = false;
		}

		void
		HandlePacket(const Ethernet::Header& ethernetHeader, const uint8_t* buffer, size_t size)
		{
			Header header;
			const auto dhcpSize = Header::Deserialize(header, buffer, size);
			if (dhcpSize != Header::SerializedLength())
			{
				DEBUG_LOG(
					"Dropped DHCP packet (invalid buffer size %u, expected %u)\r\n",
					size,
					Header::SerializedLength());
				return;
			}

			if (header.opcode != Opcode::BootReply)
				return;
			if (header.hardwareAddressType != 1)
				return;
			if (header.hardwareAddressLength != 6)
				return;
			if (header.transactionId != transactionId)
				return;

			if (!serverSelected)
			{
				handleOfferPacket(ethernetHeader, header);
			}
			else
			{
				handleAckPacket(ethernetHeader, header);
			}
		}
	} // namespace Dhcp
} // namespace Net
