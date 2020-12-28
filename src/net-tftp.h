#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include "net-udp.h"

namespace Net::Tftp
{
	const size_t TFTP_BLOCK_SIZE = 512;

	enum class Opcode : uint16_t
	{
		ReadRequest = 1,
		WriteRequest = 2,
		Data = 3,
		Acknowledgement = 4,
		Error = 5,
	};

	struct Packet
	{
		Opcode opcode;

		Packet();
		Packet(Opcode opcode);
		virtual size_t SerializedLength() const = 0;
		virtual size_t Serialize(uint8_t* buffer, const size_t bufferSize) const = 0;
	};

	struct WriteReadRequestPacket : public Packet
	{
		std::string filename;
		std::string mode;

		WriteReadRequestPacket();
		WriteReadRequestPacket(const Opcode opcode);
		size_t SerializedLength() const override;
		size_t Serialize(uint8_t* buffer, const size_t bufferSize) const override;
		size_t Deserialize(const uint8_t* buffer, const size_t bufferSize);
	};

	struct ErrorPacket : public Packet
	{
		uint16_t errorCode;
		std::string message;

		ErrorPacket();
		ErrorPacket(uint16_t errorCode, std::string message);
		size_t SerializedLength() const override;
		size_t Serialize(uint8_t* buffer, const size_t bufferSize) const override;
	};

	struct AcknowledgementPacket : public Packet
	{
		uint16_t blockNumber;

		AcknowledgementPacket();
		AcknowledgementPacket(uint16_t blockNumber);
		size_t SerializedLength() const override;
		size_t Serialize(uint8_t* buffer, const size_t bufferSize) const override;
	};

	struct DataPacket : public Packet
	{
		uint16_t blockNumber;
		std::vector<uint8_t> data;

		DataPacket();
		size_t SerializedLength() const override;
		size_t Serialize(uint8_t* buffer, const size_t bufferSize) const override;
		size_t Deserialize(const uint8_t* buffer, const size_t bufferSize);
	};

	void HandlePacket(
		const Ethernet::Header ethernetReqHeader,
		const Ipv4::Header ipv4ReqHeader,
		const Udp::Header udpReqHeader,
		const uint8_t* data,
		const size_t dataSize
	);
} // namespace Net::Tftp
