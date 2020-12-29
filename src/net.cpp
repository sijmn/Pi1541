#include "net.h"

#include "debug.h"
#include "options.h"
#include "types.h"
#include <uspi.h>
#include <uspios.h>

namespace Net
{
	static void postInitialize(unsigned int, void* parameter, void*);
	static void ipObtained();

	void Initialize(Options& options)
	{
		// Wait for ethernet to become available.
		while (!USPiEthernetAvailable())
		{
			MsDelay(500);
		}

		// Wait 3 seconds, then run postInitialize
		const auto optionsVoid = static_cast<void*>(&options);
		StartKernelTimer(3 * HZ, postInitialize, optionsVoid, nullptr);
	}

	void Update()
	{
		unsigned int bufferSize = 0;
		uint8_t buffer[USPI_FRAME_BUFFER_SIZE];
		if (!USPiEthernetAvailable() || !USPiReceiveFrame(buffer, &bufferSize))
		{
			return;
		}

		Ethernet::Header ethernetHeader;
		auto headerSize = Ethernet::Header::Deserialize(ethernetHeader, buffer, bufferSize);
		if (headerSize == 0 || headerSize != Ethernet::Header::SerializedLength())
		{
			DEBUG_LOG(
				"Dropped ethernet packet (invalid buffer size %lu, expected at least %lu)\r\n",
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

	static void postInitialize(unsigned int, void* parameter, void*)
	{
		DEBUG_LOG("Running network post-init\r\n");
		const auto options = static_cast<const Options*>(parameter);

		if (options->GetDHCPEnable())
		{
			std::function<void()> callback = ipObtained;
			Dhcp::ObtainIp(callback);
		}
		else
		{
			// Try parsing the IP address in the options.
			uint8_t ip[4];
			int scanned = sscanf(
				options->GetIPAddress(), "%hhu.%hhu.%hhu.%hhu", &ip[3], &ip[2], &ip[1], &ip[0]);

			if (scanned == 4)
			{
				DEBUG_LOG("Setting IP address %d.%d.%d.%d\r\n", ip[3], ip[2], ip[1], ip[0]);
				Utils::Ipv4Address = *reinterpret_cast<uint32_t*>(ip);
			}

			ipObtained();
		}
	}

	static void ipObtained()
	{
#ifdef DEBUG
		uint8_t* ip = reinterpret_cast<uint8_t*>(Utils::Ipv4Address);
		DEBUG_LOG("Obtained IP address %d.%d.%d.%d\r\n", ip[3], ip[2], ip[1], ip[0]);
#endif

		Arp::SendAnnouncement(Utils::GetMacAddress(), Utils::Ipv4Address);
	}
} // namespace Net
