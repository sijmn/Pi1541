#include <memory>

#include "ff.h"
#include "net-ethernet.h"
#include "net-ipv4.h"
#include "net-tftp.h"
#include "net-udp.h"
#include "net.h"
#include "types.h"

#include <uspi.h>

// TODO Allow multiple files open
static FIL outFile;
static bool shouldReboot = false;
static uint32_t currentBlockNumber = -1;

static std::unique_ptr<TftpPacket> handleTftpWriteRequest(const uint8_t* data)
{
	auto packet = TftpWriteReadRequestPacket::Deserialize(data);

	// TODO Implement netscii, maybe
	if (packet.mode != "octet")
	{
		return std::unique_ptr<TftpErrorPacket>(
			new TftpErrorPacket(0, "please use mode octet")
		);
	}

	currentBlockNumber = 0;

	// TODO Return to the original working directory.
	char workingDirectory[256];
	f_getcwd(workingDirectory, sizeof(workingDirectory));

	// Try opening the file
	auto separator = packet.filename.rfind('/', packet.filename.size());
	if (separator != std::string::npos)
	{
		auto path = "/" + packet.filename.substr(0, separator);
		f_chdir(path.c_str());
	}
	else
	{
		f_chdir("/");
		separator = 0;
	}

	// Open the output file.
	auto filename = packet.filename.substr(separator + 1);
	const auto result = f_open(&outFile, filename.c_str(), FA_CREATE_ALWAYS | FA_WRITE);

	std::unique_ptr<TftpPacket> response;
	if (result != FR_OK)
	{
		response = std::unique_ptr<TftpErrorPacket>(
			new TftpErrorPacket(0, "error opening target file")
		);
	}
	else
	{
		shouldReboot =
			packet.filename == "kernel.img" || packet.filename == "options.txt";
		response = std::unique_ptr<TftpAcknowledgementPacket>(
			new TftpAcknowledgementPacket(currentBlockNumber)
		);
	}

	// TODO Return to the original working directory here

	return response;
}

static std::unique_ptr<TftpPacket> handleTftpData(const uint8_t* data, size_t length)
{
	auto packet = TftpDataPacket::Deserialize(data, length);

	if (packet.blockNumber != currentBlockNumber + 1)
	{
		f_close(&outFile);
		return std::unique_ptr<TftpErrorPacket>(
			new TftpErrorPacket(0, "invalid block number")
		);
	}
	currentBlockNumber = packet.blockNumber;

	unsigned int bytesWritten;
	const auto result =
		f_write(&outFile, packet.data.data(), packet.data.size(), &bytesWritten);

	if (result != FR_OK || bytesWritten != packet.data.size())
	{
		f_close(&outFile);
		return std::unique_ptr<TftpErrorPacket>(new TftpErrorPacket(0, "io error"));
	}

	if (packet.data.size() < TFTP_BLOCK_SIZE)
	{
		// Close the file for the last packet.
		f_close(&outFile);
	}

	return std::unique_ptr<TftpAcknowledgementPacket>(
		new TftpAcknowledgementPacket(currentBlockNumber)
	);
}

void HandleTftpDatagram(
	const EthernetFrameHeader ethernetReqHeader,
	const Ipv4Header ipv4ReqHeader,
	const UdpDatagramHeader udpReqHeader,
	const uint8_t* data
) {
	const auto opcode = data[0] << 8 | data[1];
	std::unique_ptr<TftpPacket> response;
	bool last = false;

	if (opcode == TFTP_OP_WRITE_REQUEST)
	{
		response = handleTftpWriteRequest(data);
	}
	else if (opcode == TFTP_OP_DATA)
	{
		const auto length = udpReqHeader.length - UdpDatagramHeader::SerializedLength();
		response = handleTftpData(data, length);
	}
	else
	{
		response = std::unique_ptr<TftpErrorPacket>(
			new TftpErrorPacket(4, "not implemented yet")
		);
	}

	if (response != nullptr)
	{
		UdpDatagramHeader udpRespHeader(
			udpReqHeader.destinationPort,
			udpReqHeader.sourcePort,
			response->SerializedLength() + UdpDatagramHeader::SerializedLength()
		);
		Ipv4Header ipv4RespHeader(
			IP_PROTO_UDP,
			Ipv4Address,
			ipv4ReqHeader.sourceIp,
			udpRespHeader.length + Ipv4Header::SerializedLength()
		);
		EthernetFrameHeader ethernetRespHeader(
			ArpTable[ipv4RespHeader.destinationIp],
			GetMacAddress(),
			ETHERTYPE_IPV4
		);

		size_t i = 0;
		uint8_t buffer[USPI_FRAME_BUFFER_SIZE];
		i += ethernetRespHeader.Serialize(buffer + i);
		i += ipv4RespHeader.Serialize(buffer + i);
		i += udpRespHeader.Serialize(buffer + i);
		i += response->Serialize(buffer + i);
		USPiSendFrame(buffer, i);
	}

	if (last && shouldReboot)
	{
		// TODO eww
		extern void Reboot_Pi();
		Reboot_Pi();
	}
}

//
// TftpWriteReadRequestPacket
//
TftpWriteReadRequestPacket::TftpWriteReadRequestPacket(uint16_t opcode) :
	TftpPacket(opcode)
{}

size_t TftpWriteReadRequestPacket::SerializedLength() const
{
	return TftpPacket::SerializedLength() + filename.size() + 1 + mode.size() + 1;
}

size_t TftpWriteReadRequestPacket::Serialize(uint8_t* buffer) const
{
	size_t i = 0;
	buffer[i++] = opcode >> 8;
	buffer[i++] = opcode;

	i += filename.copy(reinterpret_cast<char*>(buffer + i), filename.size());
	buffer[i++] = 0;

	i += mode.copy(reinterpret_cast<char*>(buffer + i), mode.size());
	buffer[i++] = 0;

	return i;
}

TftpWriteReadRequestPacket TftpWriteReadRequestPacket::Deserialize(const uint8_t* buffer)
{
	size_t i = 0;

	TftpWriteReadRequestPacket self(buffer[i] << 8 | buffer[i + 1]);
	i += 2;

	self.filename = reinterpret_cast<const char*>(buffer + i);
	i += self.filename.size() + 1;

	self.mode = reinterpret_cast<const char*>(buffer + i);
	i += self.mode.size() + 1;

	return self;
}

//
// TftpErrorPacket
//
TftpErrorPacket::TftpErrorPacket() : TftpPacket(TFTP_OP_ERROR) {}
TftpErrorPacket::TftpErrorPacket(uint16_t errorCode, std::string message) :
	TftpPacket(TFTP_OP_ERROR), errorCode(errorCode), message(message)
{}

size_t TftpErrorPacket::SerializedLength() const
{
	return TftpPacket::SerializedLength() + sizeof(errorCode) + message.size() + 1;
}

size_t TftpErrorPacket::Serialize(uint8_t* buffer) const
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

//
// TftpAcknowledgementPacket
//
TftpAcknowledgementPacket::TftpAcknowledgementPacket() :
	TftpPacket(TFTP_OP_ACKNOWLEDGEMENT)
{}

TftpAcknowledgementPacket::TftpAcknowledgementPacket(uint16_t blockNumber) :
	TftpPacket(TFTP_OP_ACKNOWLEDGEMENT), blockNumber(blockNumber)
{}

size_t TftpAcknowledgementPacket::SerializedLength() const
{
	return TftpPacket::SerializedLength() + sizeof(blockNumber);
}

size_t TftpAcknowledgementPacket::Serialize(uint8_t* buffer) const
{
	size_t i = 0;
	buffer[i++] = opcode >> 8;
	buffer[i++] = opcode;
	buffer[i++] = blockNumber >> 8;
	buffer[i++] = blockNumber;
	return i;
}

//
// TftpDataPacket
//
TftpDataPacket::TftpDataPacket() : opcode(TFTP_OP_DATA) {}

TftpDataPacket TftpDataPacket::Deserialize(const uint8_t* buffer, size_t length)
{
	TftpDataPacket self;
	self.opcode = buffer[0] << 8 | buffer[1];
	self.blockNumber = buffer[2] << 8 | buffer[3];
	self.data = std::vector<uint8_t>(buffer + 4, buffer + length);
	return self;
}
