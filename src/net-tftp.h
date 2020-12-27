#pragma once
#include <cstdint>
#include <string>
#include <vector>

struct EthernetFrameHeader;
struct Ipv4Header;
struct UdpDatagramHeader;

namespace Net::Tftp {
	const size_t TFTP_BLOCK_SIZE = 512;

	enum class Opcode : uint16_t
	{
		ReadRequest = 1,
		WriteRequest = 2,
		Data = 3,
		Acknowledgement = 4,
		Error = 5,
	};

	struct TftpPacket
	{
		Opcode opcode;

		TftpPacket(Opcode opcode) : opcode(opcode) {}

		virtual size_t SerializedLength() const {
			return sizeof(opcode);
		}

		virtual size_t Serialize(uint8_t* buffer) const = 0;
	};

	struct TftpWriteReadRequestPacket : public TftpPacket
	{
		std::string filename;
		std::string mode;

		TftpWriteReadRequestPacket(const Opcode opcode);
		size_t SerializedLength() const override;
		size_t Serialize(uint8_t* buffer) const override;
		static TftpWriteReadRequestPacket Deserialize(const uint8_t* buffer);
	};

	struct TftpErrorPacket : public TftpPacket
	{
		uint16_t errorCode;
		std::string message;

		TftpErrorPacket();
		TftpErrorPacket(uint16_t errorCode, std::string message);
		size_t SerializedLength() const override;
		size_t Serialize(uint8_t* buffer) const override;
	};

	struct TftpAcknowledgementPacket : public TftpPacket
	{
		uint16_t blockNumber;

		TftpAcknowledgementPacket();
		TftpAcknowledgementPacket(uint16_t blockNumber);
		size_t SerializedLength() const override;
		size_t Serialize(uint8_t* buffer) const override;
	};

	struct TftpDataPacket : public TftpPacket
	{
		uint16_t blockNumber;
		std::vector<uint8_t> data;

		TftpDataPacket();
		size_t Serialize(uint8_t* buffer) const override;
		static size_t Deserialize(
			TftpDataPacket& out, const uint8_t* buffer, size_t length);
	};

	void HandlePacket(
		const EthernetFrameHeader ethernetReqHeader,
		const Ipv4Header ipv4ReqHeader,
		const UdpDatagramHeader udpReqHeader,
		const uint8_t* buffer
	);
}; // namespace Net::Tftp
