#include "net.h"

#include "debug.h"
#include "options.h"
#include "types.h"
#include <uspi.h>
#include <uspios.h>

extern "C"
{
#include "rpiHardware.h"
}

namespace Net
{
	static uint32_t postInitializeTime = 0;
	static Options* options;

	static void postInitialize();
	static void ipObtained();

	void Initialize(Options& options)
	{
		// Wait for ethernet to become available.
		DEBUG_LOG("Waiting for ethernet\r\n");
		while (!USPiEthernetAvailable())
		{
			MsDelay(500);
		}

		// Wait 3 seconds, then run postInitialize
		DEBUG_LOG("Scheduled post-init\r\n");
		Net::options = &options;
		postInitializeTime = read32(ARM_SYSTIMER_CLO) + 30000;
	}

	void HandlePacket(const uint8_t* buffer, const size_t bufferSize)
	{
		Ethernet::Header ethernetHeader;
		auto headerSize = Ethernet::Header::Deserialize(ethernetHeader, buffer, bufferSize);
		if (headerSize == 0 || headerSize != Ethernet::Header::SerializedLength())
		{
			DEBUG_LOG(
				"Dropped ethernet packet (invalid buffer size %u, expected at least %u)\r\n",
				headerSize,
				Ethernet::Header::SerializedLength());
			return;
		}

		switch (ethernetHeader.type)
		{
		case Ethernet::EtherType::Arp:
			Arp::HandlePacket(ethernetHeader, buffer + headerSize, bufferSize - headerSize);
			break;
		case Ethernet::EtherType::Ipv4:
			Ipv4::HandlePacket(ethernetHeader, buffer + headerSize, bufferSize - headerSize);
			break;
		}
	}

	void Update()
	{
		if (postInitializeTime && read32(ARM_SYSTIMER_CLO) > postInitializeTime)
		{
			postInitialize();
			postInitializeTime = 0;
		}

		unsigned int bufferSize = 0;
		uint8_t buffer[USPI_FRAME_BUFFER_SIZE];
		if (!USPiReceiveFrame(buffer, &bufferSize))
		{
			return;
		}

		HandlePacket(buffer, sizeof(buffer));
	}

	static void postInitialize()
	{
		DEBUG_LOG("Running network post-init\r\n");

		if (options->GetDHCPEnable())
		{
			DEBUG_LOG("DHCP enabled, trying to obtain IP\r\n");
			std::function<void()> callback = ipObtained;
			Dhcp::ObtainIp(callback);
		}
		else
		{
			// Try parsing the IP address in the options.
			unsigned int ip[4];
			int scanned =
				sscanf(options->GetIPAddress(), "%u.%u.%u.%u", &ip[0], &ip[1], &ip[2], &ip[3]);

			if (scanned == 4)
			{
				DEBUG_LOG("Setting IP address %u.%u.%u.%u\r\n", ip[0], ip[1], ip[2], ip[3]);
				Utils::Ipv4Address = 0;
				for (int i = 0; i < 4; i++)
				{
					Utils::Ipv4Address <<= 8;
					Utils::Ipv4Address |= ip[i];
				}

				ipObtained();
			}
			else
			{
				DEBUG_LOG("Invalid IP address '%s'\r\n", options->GetIPAddress());
			}
		}
	}

	static void ipObtained()
	{
		DEBUG_LOG(
			"Obtained IP address %ld.%ld.%ld.%ld\r\n",
			Utils::Ipv4Address >> 24,
			(Utils::Ipv4Address >> 16) & 0xFF,
			(Utils::Ipv4Address >> 8) & 0xFF,
			Utils::Ipv4Address & 0xFF);
		Arp::SendAnnouncement(Utils::GetMacAddress(), Utils::Ipv4Address);
	}
} // namespace Net
