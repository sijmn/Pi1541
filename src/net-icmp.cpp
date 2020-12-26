#include "net-icmp.h"

//
// IcmpPacketHeader
//
IcmpPacketHeader::IcmpPacketHeader() {}

IcmpPacketHeader::IcmpPacketHeader(std::uint8_t type, std::uint8_t code) :
    type(type), code(code), checksum(0)
{}

std::size_t IcmpPacketHeader::Serialize(uint8_t* buffer) const
{
    size_t i = 0;
    buffer[i++] = type;
    buffer[i++] = code;
    buffer[i++] = checksum;
    buffer[i++] = checksum >> 8;
    return i;
}

IcmpPacketHeader IcmpPacketHeader::Deserialize(const uint8_t* buffer)
{
    IcmpPacketHeader self;
    self.type = buffer[0];
    self.code = buffer[1];
    self.checksum = buffer[2] << 8 | buffer[3];
    return self;
}

//
// IcmpEchoHeader
//
IcmpEchoHeader::IcmpEchoHeader() : IcmpEchoHeader(0, 0) {}
IcmpEchoHeader::IcmpEchoHeader(uint16_t identifier, uint16_t sequenceNumber) :
    identifier(identifier), sequenceNumber(sequenceNumber) {}

size_t IcmpEchoHeader::Serialize(uint8_t* buffer) const
{
    size_t i = 0;
    buffer[i++] = identifier >> 8;
    buffer[i++] = identifier;
    buffer[i++] = sequenceNumber >> 8;
    buffer[i++] = sequenceNumber;
    return i;
}

IcmpEchoHeader IcmpEchoHeader::Deserialize(const uint8_t* buffer)
{
    IcmpEchoHeader self;
    self.identifier = buffer[0] << 8 | buffer[1];
    self.sequenceNumber = buffer[2] << 8 | buffer[3];
    return self;
}
