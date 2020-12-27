#include "net-arp.h"
#include "net-ethernet.h"

#include "types.h"
#include <uspi.h>

namespace Net::Arp
{
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

    void SendPacket(
        ArpOperation operation,
        MacAddress targetMac,
        MacAddress senderMac,
        uint32_t targetIp,
        uint32_t senderIp)
    {
        Ipv4ArpPacket arpPacket(operation);
        arpPacket.targetMac = targetMac;
        arpPacket.senderMac = senderMac;
        arpPacket.targetIp = targetIp;
        arpPacket.senderIp = senderIp;

        EthernetFrameHeader ethernetHeader(senderMac, targetMac, ETHERTYPE_ARP);

        uint8_t buffer[USPI_FRAME_BUFFER_SIZE];
        size_t size = 0;
        size += ethernetHeader.Serialize(buffer + size);
        size += arpPacket.Serialize(buffer + size);
        USPiSendFrame(buffer, size);
    }

    void SendRequest(
        MacAddress targetMac, MacAddress senderMac, uint32_t targetIp, uint32_t senderIp)
    {
        SendPacket(ARP_OPERATION_REQUEST, targetMac, senderMac, targetIp, senderIp);
    }

    void SendReply(
        MacAddress targetMac, MacAddress senderMac, uint32_t targetIp, uint32_t senderIp)
    {
        SendPacket(ARP_OPERATION_REPLY, targetMac, senderMac, targetIp, senderIp);
    }

    void SendAnnouncement(MacAddress mac, uint32_t ip)
    {
        SendReply(MacBroadcast, mac, ip, ip);
    }

    void HandlePacket(const EthernetFrameHeader ethernetHeader, uint8_t* buffer)
    {
        const auto macAddress = GetMacAddress();
        const auto arpPacket = Ipv4ArpPacket::Deserialize(buffer);

        if (
            arpPacket.hardwareType == 1 &&
            arpPacket.protocolType == ETHERTYPE_IPV4 &&
            arpPacket.operation == ARP_OPERATION_REQUEST &&
            arpPacket.targetIp == Ipv4Address)
        {
            SendReply(arpPacket.senderMac, macAddress, arpPacket.senderIp, Ipv4Address);
        }

        else if (
            arpPacket.hardwareType == 1 &&
            arpPacket.protocolType == ETHERTYPE_IPV4 &&
            arpPacket.operation == ARP_OPERATION_REPLY &&
            arpPacket.targetIp == Ipv4Address &&
            arpPacket.targetMac == macAddress)
        {
            ArpTable.insert(std::make_pair(arpPacket.senderIp, arpPacket.senderMac));
        }
    }

    std::unordered_map<std::uint32_t, MacAddress> ArpTable;
}; // namespace Net::Arp
