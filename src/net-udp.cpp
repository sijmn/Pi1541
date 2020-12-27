#include "net-udp.h"

UdpDatagramHeader::UdpDatagramHeader()
{}

UdpDatagramHeader::UdpDatagramHeader(
    uint16_t sourcePort,
    uint16_t destinationPort,
    uint16_t length
) :
    sourcePort(sourcePort),
    destinationPort(destinationPort),
    length(length),
    checksum(0)
{}

size_t UdpDatagramHeader::Serialize(uint8_t* buffer) const
{
    size_t i = 0;
    buffer[i++] = sourcePort >> 8;
    buffer[i++] = sourcePort;
    buffer[i++] = destinationPort >> 8;
    buffer[i++] = destinationPort;
    buffer[i++] = length >> 8;
    buffer[i++] = length;
    buffer[i++] = checksum >> 8;
    buffer[i++] = checksum;
    return i;
}

UdpDatagramHeader UdpDatagramHeader::Deserialize(const uint8_t* buffer)
{
    UdpDatagramHeader self;
    self.sourcePort = buffer[0] << 8 | buffer[1];
    self.destinationPort = buffer[2] << 8 | buffer[3];
    self.length = buffer[4] << 8 | buffer[5];
    self.checksum = buffer[6] << 8 | buffer[7];
    return self;
}