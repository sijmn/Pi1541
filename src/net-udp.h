#pragma once
#include <vector>

enum TftpOperation
{
	TFTP_OP_READ_REQUEST = 1,
	TFTP_OP_WRITE_REQUEST = 2,
	TFTP_OP_DATA = 3,
	TFTP_OP_ACKNOWLEDGEMENT = 4,
	TFTP_OP_ERROR = 5,
};

struct UdpDatagramHeader
{
	std::uint16_t sourcePort;
	std::uint16_t destinationPort;
	std::uint16_t length;
	std::uint16_t checksum;

	UdpDatagramHeader() {}

	UdpDatagramHeader(uint16_t sourcePort, uint16_t destinationPort, uint16_t length) :
		sourcePort(sourcePort),
		destinationPort(destinationPort),
		length(length),
		checksum(0)
	{}

	static constexpr size_t SerializedLength()
	{
		return
			sizeof(sourcePort) +
			sizeof(destinationPort) +
			sizeof(length) +
			sizeof(checksum);
	}

	size_t Serialize(uint8_t* buffer)
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

	static UdpDatagramHeader Deserialize(const uint8_t* buffer)
	{
		UdpDatagramHeader self;
		self.sourcePort = buffer[0] << 8 | buffer[1];
		self.destinationPort = buffer[2] << 8 | buffer[3];
		self.length = buffer[4] << 8 | buffer[5];
		self.checksum = buffer[6] << 8 | buffer[7];
		return self;
	}
} __attribute__((packed));

template <class T>
struct UdpDatagram
{
	UdpDatagramHeader header;
	T payload;

	UdpDatagram() {}

	UdpDatagram(uint16_t sourcePort, uint16_t destinationPort, T payload) :
		header(sourcePort, destinationPort, sizeof(UdpDatagram<T>)),
		payload(payload)
	{}
} __attribute__((packed));

struct TftpWriteReadRequestPacket
{
	uint16_t opcode;
	std::string filename;
	std::string mode;

	size_t SerializedLength() const {
		return sizeof(opcode) + filename.size() + 1 + mode.size() + 1;
	}

	size_t Serialize(uint8_t* buffer) const {
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
		TftpWriteReadRequestPacket self;
		size_t i = 0;

		self.opcode = buffer[i] << 8 | buffer[i + 1];
		i += 2;

		self.filename = reinterpret_cast<const char*>(buffer + i);
		i += self.filename.size() + 1;

		self.mode = reinterpret_cast<const char*>(buffer + i);
		i += self.mode.size() + 1;

		return self;
	}
};

struct TftpErrorPacket
{
	uint16_t opcode;
	uint16_t errorCode;
	std::string message;

	TftpErrorPacket() : opcode(TFTP_OP_ERROR) {}
	TftpErrorPacket(uint16_t errorCode, std::string message) :
		opcode(TFTP_OP_ERROR), errorCode(errorCode), message(message)
	{}

	constexpr size_t SerializedLength()
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

struct TftpAcknowledgementPacket
{
	uint16_t opcode;
	uint16_t blockNumber;

	TftpAcknowledgementPacket() : opcode(TFTP_OP_ACKNOWLEDGEMENT) {}

	TftpAcknowledgementPacket(uint16_t blockNumber) :
		opcode(TFTP_OP_ACKNOWLEDGEMENT), blockNumber(blockNumber)
	{}

	constexpr size_t SerializedLength()
	{
		return sizeof(opcode) + sizeof(blockNumber);
	}

	size_t Serialize(uint8_t* buffer) const
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
