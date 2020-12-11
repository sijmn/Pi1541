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

	virtual size_t SerializedLength() const = 0;
	virtual size_t Serialize(uint8_t* buffer) const = 0;
};

struct TftpWriteReadRequestPacket : public TftpPacket
{
	std::string filename;
	std::string mode;

	TftpWriteReadRequestPacket(uint16_t opcode) : TftpPacket(opcode) {}

	constexpr size_t SerializedLength() override {
		return sizeof(opcode) + filename.size() + 1 + mode.size() + 1;
	}

	size_t Serialize(uint8_t* buffer) const override {
		size_t i = 0;
		buffer[i++] = opcode >> 8;
		buffer[i++] = opcode;

		i += filename.copy(reinterpret_cast<char*>(buffer + i), filename.size());
		buffer[i++] = 0;

		i += mode.copy(reinterpret_cast<char*>(buffer + i), mode.size());
		buffer[i++] = 0;

		return i;
	}

	static TftpWriteReadRequestPacket Deserialize(const uint8_t* buffer) {
		size_t i = 0;

		TftpWriteReadRequestPacket self(buffer[i] << 8 | buffer[i + 1]);
		i += 2;

		self.filename = reinterpret_cast<const char*>(buffer + i);
		i += self.filename.size() + 1;

		self.mode = reinterpret_cast<const char*>(buffer + i);
		i += self.mode.size() + 1;

		return self;
	}
};

struct TftpErrorPacket : public TftpPacket
{
	uint16_t errorCode;
	std::string message;

	TftpErrorPacket() : TftpPacket(TFTP_OP_ERROR) {}
	TftpErrorPacket(uint16_t errorCode, std::string message) :
		TftpPacket(TFTP_OP_ERROR), errorCode(errorCode), message(message)
	{}

	constexpr size_t SerializedLength() const override
	{
		return sizeof(opcode) + sizeof(errorCode) + message.size() + 1;
	}

	size_t Serialize(uint8_t* buffer) const
	{
		size_t i = 0;
		buffer[i++] = opcode >> 8;
		buffer[i++] = opcode;
		buffer[i++] = errorCode >> 8;
		buffer[i++] = errorCode;
		
		i += message.copy(reinterpret_cast<char*>(buffer + i), message.size());
		buffer[i++] = 0;

		return i;
	}
};

struct TftpAcknowledgementPacket : public TftpPacket
{
	uint16_t blockNumber;

	TftpAcknowledgementPacket() : TftpPacket(TFTP_OP_ACKNOWLEDGEMENT) {}

	TftpAcknowledgementPacket(uint16_t blockNumber) :
		TftpPacket(TFTP_OP_ACKNOWLEDGEMENT), blockNumber(blockNumber)
	{}

	constexpr size_t SerializedLength() override
	{
		return sizeof(opcode) + sizeof(blockNumber);
	}

	size_t Serialize(uint8_t* buffer) const override
	{
		size_t i = 0;
		buffer[i++] = opcode >> 8;
		buffer[i++] = opcode;
		buffer[i++] = blockNumber >> 8;
		buffer[i++] = blockNumber;
		return i;
	}
};

struct TftpDataPacket
{
	uint16_t opcode;
	uint16_t blockNumber;
	std::vector<uint8_t> data;

	TftpDataPacket() : opcode(TFTP_OP_DATA) {}

	static TftpDataPacket Deserialize(const uint8_t* buffer, size_t length)
	{
		TftpDataPacket self;
		self.opcode = buffer[0] << 8 | buffer[1];
		self.blockNumber = buffer[2] << 8 | buffer[3];
		self.data = std::vector<uint8_t>(buffer + 4, buffer + length);
		return self;
	}
};
