#include "net-arp.h"

Ipv4ArpPacket::Ipv4ArpPacket()
{}

Ipv4ArpPacket::Ipv4ArpPacket(std::uint16_t operation) :
    hardwareType(1), // Ethernet
    protocolType(ETHERTYPE_IPV4), // IPv4
    hardwareAddressLength(6),
    protocolAddressLength(4),
    operation(operation)
{}

std::size_t Ipv4ArpPacket::Serialize(std::uint8_t* buffer)
{
    buffer[0] = hardwareType >> 8;
    buffer[1] = hardwareType;
    buffer[2] = protocolType >> 8;
    buffer[3] = protocolType;
    buffer[4] = hardwareAddressLength;
    buffer[5] = protocolAddressLength;
    buffer[6] = operation >> 8;
    buffer[7] = operation;

    memcpy(buffer + 8, senderMac.data(), 6);

    buffer[14] = senderIp >> 24;
    buffer[15] = senderIp >> 16;
    buffer[16] = senderIp >> 8;
    buffer[17] = senderIp;

    memcpy(buffer + 18, targetMac.data(), 6);

    buffer[24] = targetIp >> 24;
    buffer[25] = targetIp >> 16;
    buffer[26] = targetIp >> 8;
    buffer[27] = targetIp;

    return 28;
}

// Static
Ipv4ArpPacket Ipv4ArpPacket::Deserialize(const uint8_t* buffer)
{
    Ipv4ArpPacket self;

    self.hardwareType = buffer[0] << 8 | buffer[1];
    self.protocolType = buffer[2] << 8 | buffer[3];
    self.hardwareAddressLength = buffer[4];
    self.protocolAddressLength = buffer[5];
    self.operation = buffer[6] << 8 | buffer[7];

    memcpy(self.senderMac.data(), buffer + 8, 6);
    self.senderIp =
        buffer[14] << 24 | buffer[15] << 16 | buffer[16] << 8 | buffer[17];
    memcpy(self.targetMac.data(), buffer + 18, 6);
    self.targetIp =
        buffer[24] << 24 | buffer[25] << 16 | buffer[26] << 8 | buffer[27];

    return self;
}
