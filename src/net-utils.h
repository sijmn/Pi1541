#include <array>
#include <cstddef>
#include <cstdint>

namespace Net::Utils
{
	typedef std::array<uint8_t, 6> MacAddress;
	extern const MacAddress MacBroadcast;
	extern uint32_t Ipv4Address;

	uint32_t Crc32(const uint8_t* buffer, size_t size);
	uint16_t InternetChecksum(const void* data, size_t size);
	MacAddress GetMacAddress();
}; // namespace Net::Utils
