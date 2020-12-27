#pragma once
#include <vector>

const size_t TFTP_BLOCK_SIZE = 512;

enum TftpOperation
{
	TFTP_OP_READ_REQUEST = 1,
	TFTP_OP_WRITE_REQUEST = 2,
	TFTP_OP_DATA = 3,
	TFTP_OP_ACKNOWLEDGEMENT = 4,
	TFTP_OP_ERROR = 5,
};

struct TftpPacket
{
	uint16_t opcode;

	TftpPacket(uint16_t opcode) : opcode(opcode) {}

	virtual size_t SerializedLength() const {
		return sizeof(opcode);
	}

	virtual size_t Serialize(uint8_t* buffer) const = 0;
};

struct TftpWriteReadRequestPacket : public TftpPacket
{
	std::string filename;
	std::string mode;

	TftpWriteReadRequestPacket(uint16_t opcode);
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

struct TftpDataPacket
{
	uint16_t opcode;
	uint16_t blockNumber;
	std::vector<uint8_t> data;

	TftpDataPacket();
	static size_t Deserialize(
		TftpDataPacket& out, const uint8_t* buffer, size_t length);
};
