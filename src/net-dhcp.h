#pragma once
#include <functional>

#include "net-ethernet.h"
#include "net.h"

namespace Net::Dhcp
{
	enum class Opcode : uint8_t
	{
		BootRequest = 1,
		BootReply = 2,
	};

	struct Header
	{
		/// Message op code / message type. 1 = BOOTREQUEST, 2 = BOOTREPLY
		Opcode opcode;

		/// Hardware address type, see ARP section in "Assigned Numbers" RFC
		uint8_t hardwareAddressType;

		uint8_t hardwareAddressLength;

		/// Client sets to zero, optionally used by relay agents when booting via a
		/// relay agent.
		uint8_t hops;

		/// A random number chosen by the client, used by the client and server to
		/// associate messages and responses between a client and a server.
		uint32_t transactionId;

		/// Filled in by client, seconds elapsed since client began address acquisition
		/// or renewal process.
		uint16_t secondsElapsed;

		uint16_t flags;

		/// Only filled in if client is in BOUND, RENEW or REBINDING state and can
		/// respond to ARP requests.
		uint32_t clientIpAddress;

		/// 'your' (client) IP address.
		uint32_t yourIpAddress;

		/// IP address of next server to use in bootstrap; returned in DHCPOFFER,
		/// DHCPACK by server.
		uint32_t serverIpAddress;

		/// Relay agent IP address, used in booting via a relay agent.
		uint32_t relayIpAddress;

		std::array<uint8_t, 16> clientHardwareAddress;

		/// Optional server host name, null terminated string.
		std::array<uint8_t, 64> serverHostname;

		/// Boot file name, null terminated string; "generic" name or null in
		/// DHCPDISCOVER, fully qualified directory-path name in DHCPOFFER.
		std::array<uint8_t, 128> bootFile;

		/// Always 99, 130, 83, 99
		std::array<uint8_t, 4> magicValue;

		Header();
		Header(Opcode opcode, uint32_t transactionId);

		constexpr static size_t SerializedLength()
		{
			return sizeof(Opcode) + sizeof(hardwareAddressType) + sizeof(hardwareAddressLength) +
				sizeof(hops) + sizeof(transactionId) + sizeof(secondsElapsed) + sizeof(flags) +
				sizeof(clientIpAddress) + sizeof(yourIpAddress) + sizeof(serverIpAddress) +
				sizeof(relayIpAddress) + sizeof(clientHardwareAddress) + sizeof(serverHostname) +
				sizeof(bootFile) + sizeof(magicValue);
		}

		size_t Serialize(uint8_t* buffer, const size_t size) const;
		static size_t Deserialize(Header& out, const uint8_t* buffer, const size_t size);
	};

	void ObtainIp(std::function<void()>& callback);
	void HandlePacket(const Ethernet::Header& ethernetHeader, const uint8_t* buffer, size_t size);
} // namespace Net::Dhcp
