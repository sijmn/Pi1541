#include "net-ipv4.h"
#include "net-utils.h"

Ipv4Header::Ipv4Header() {}

Ipv4Header::Ipv4Header(
	uint8_t protocol, uint32_t sourceIp, uint32_t destinationIp, uint16_t totalLength
) :
	version(4),
	ihl(5),
	dscp(0),
	ecn(0),
	totalLength(totalLength),
	identification(0),
	flags(0),
	fragmentOffset(0),
	ttl(64),
	protocol(protocol),
	headerChecksum(0),
	sourceIp(sourceIp),
	destinationIp(destinationIp)
{}

size_t Ipv4Header::Serialize(uint8_t* buffer) const
{
	size_t i = 0;

	buffer[i++] = version << 4 | ihl;
	buffer[i++] = dscp << 2 | ecn;
	buffer[i++] = totalLength >> 8;
	buffer[i++] = totalLength;
	buffer[i++] = identification >> 8;
	buffer[i++] = identification;
	buffer[i++] = (flags << 13 | fragmentOffset) >> 8;
	buffer[i++] = flags << 13 | fragmentOffset;
	buffer[i++] = ttl;
	buffer[i++] = protocol;

	// Zero the checksum before calculating it
	buffer[i++] = 0;
	buffer[i++] = 0 >> 8;

	buffer[i++] = sourceIp >> 24;
	buffer[i++] = sourceIp >> 16;
	buffer[i++] = sourceIp >> 8;
	buffer[i++] = sourceIp;
	buffer[i++] = destinationIp >> 24;
	buffer[i++] = destinationIp >> 16;
	buffer[i++] = destinationIp >> 8;
	buffer[i++] = destinationIp;

	uint16_t checksum = Net::Utils::InternetChecksum(buffer, i);
	buffer[10] = checksum;
	buffer[11] = checksum >> 8;

	return i;
}

Ipv4Header Ipv4Header::Deserialize(const uint8_t* buffer)
{
	Ipv4Header self;
	self.version = buffer[0] >> 4;
	self.ihl = buffer[0] & 0x0F;

	self.dscp = buffer[1] >> 2;
	self.ecn = buffer[1] & 0x03;

	self.totalLength = buffer[2] << 8 | buffer[3];
	self.identification = buffer[4] << 8 | buffer[5];

	self.flags = buffer[6] >> 5;
	self.fragmentOffset = (buffer[6] & 0x1F) << 8 | buffer[7];

	self.ttl = buffer[8];
	self.protocol = buffer[9];
	self.headerChecksum = buffer[10] << 8 | buffer[11];

	self.sourceIp = buffer[12] << 24 | buffer[13] << 16 | buffer[14] << 8 | buffer[15];
	self.destinationIp =
		buffer[16] << 24 | buffer[17] << 16 | buffer[18] << 8 | buffer[19];

	return self;
}
