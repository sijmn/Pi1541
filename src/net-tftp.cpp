#include <cassert>
#include <cstring>
#include <memory>

#include "net-arp.h"
#include "net-ethernet.h"
#include "net-ipv4.h"
#include "net-tftp.h"
#include "net-udp.h"
#include "net.h"

#include "debug.h"
#include "ff.h"
#include "types.h"
#include <uspi.h>

namespace Net::Tftp
{
	// TODO Allow multiple files open
	static FIL outFile;
	static bool shouldReboot = false;
	static uint32_t currentBlockNumber = -1;

	Packet::Packet() : opcode(static_cast<Opcode>(0)) {}
	Packet::Packet(const Opcode opcode) : opcode(opcode) {}

	static std::unique_ptr<Packet>
	handleTftpWriteRequest(const uint8_t* data, const size_t dataSize)
	{
		WriteReadRequestPacket packet;
		const auto size = packet.Deserialize(data, dataSize);
		if (size == 0)
		{
			DEBUG_LOG(
				"Dropped TFTP packet (invalid buffer size %lu, expected at least %lu)\r\n",
				dataSize,
				sizeof(WriteReadRequestPacket::opcode) + 2)
			return nullptr;
		}

		// TODO Implement netscii, maybe
		if (packet.mode != "octet")
		{
			const auto pointer = new ErrorPacket(0, "please use mode octet");
			return std::unique_ptr<ErrorPacket>(pointer);
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

		std::unique_ptr<Packet> response;
		if (result != FR_OK)
		{
			response =
				std::unique_ptr<ErrorPacket>(new ErrorPacket(0, "error opening target file"));
		}
		else
		{
			shouldReboot = packet.filename == "kernel.img" || packet.filename == "options.txt";
			response = std::unique_ptr<AcknowledgementPacket>(
				new AcknowledgementPacket(currentBlockNumber));
		}

		// TODO Return to the original working directory here

		return response;
	}

	static std::unique_ptr<Packet> handleTftpData(const uint8_t* buffer, size_t size)
	{
		DataPacket packet;
		const auto tftpSize = packet.Deserialize(buffer, size);
		if (tftpSize == 0)
		{
			DEBUG_LOG(
				"Dropped TFTP data packet (invalid buffer size %lu, expected at least %lu)\r\n",
				size,
				sizeof(packet.opcode) + sizeof(packet.blockNumber))
			return nullptr;
		}

		if (packet.blockNumber != currentBlockNumber + 1)
		{
			f_close(&outFile);
			return std::unique_ptr<ErrorPacket>(new ErrorPacket(0, "invalid block number"));
		}
		currentBlockNumber = packet.blockNumber;

		unsigned int bytesWritten;
		const auto result =
			f_write(&outFile, packet.data.data(), packet.data.size(), &bytesWritten);

		if (result != FR_OK || bytesWritten != packet.data.size())
		{
			f_close(&outFile);
			return std::unique_ptr<ErrorPacket>(new ErrorPacket(0, "io error"));
		}

		if (packet.data.size() < TFTP_BLOCK_SIZE)
		{
			// Close the file for the last packet.
			f_close(&outFile);
		}

		return std::unique_ptr<AcknowledgementPacket>(
			new AcknowledgementPacket(currentBlockNumber));
	}

	void HandlePacket(
		const Ethernet::Header ethernetReqHeader,
		const Ipv4::Header ipv4ReqHeader,
		const Udp::Header udpReqHeader,
		const uint8_t* reqBuffer,
		const size_t reqBufferSize)
	{
		const auto opcode = static_cast<Opcode>(reqBuffer[0] << 8 | reqBuffer[1]);
		std::unique_ptr<Packet> response;

		const auto payloadSize = udpReqHeader.length - udpReqHeader.SerializedLength();
		if (reqBufferSize < payloadSize)
		{
			DEBUG_LOG(
				"Dropped TFTP packet (invalid buffer size %lu, expected at least %lu)\r\n",
				reqBufferSize,
				payloadSize);
		}

		if (opcode == Opcode::WriteRequest)
		{
			response = handleTftpWriteRequest(reqBuffer, payloadSize);
		}
		else if (opcode == Opcode::Data)
		{
			response = handleTftpData(reqBuffer, payloadSize);
		}
		else
		{
			response = std::unique_ptr<ErrorPacket>(new ErrorPacket(4, "not implemented yet"));
		}

		if (response != nullptr)
		{
			Udp::Header udpRespHeader(
				udpReqHeader.destinationPort,
				udpReqHeader.sourcePort,
				response->SerializedLength() + Udp::Header::SerializedLength());
			Ipv4::Header ipv4RespHeader(
				Ipv4::Protocol::Udp,
				Utils::Ipv4Address,
				ipv4ReqHeader.sourceIp,
				udpRespHeader.length + Ipv4::Header::SerializedLength());
			Ethernet::Header ethernetRespHeader(
				Arp::ArpTable[ipv4RespHeader.destinationIp],
				Utils::GetMacAddress(),
				Ethernet::EtherType::Ipv4);

			size_t size = 0;
			uint8_t buffer[USPI_FRAME_BUFFER_SIZE];
			size += ethernetRespHeader.Serialize(buffer + size, sizeof(buffer) - size);
			size += ipv4RespHeader.Serialize(buffer + size, sizeof(buffer) - size);
			size += udpRespHeader.Serialize(buffer + size, sizeof(buffer) - size);
			size += response->Serialize(buffer + size, sizeof(buffer) - size);

			const auto expectedSize = ethernetRespHeader.SerializedLength() +
				ipv4RespHeader.SerializedLength() + udpRespHeader.SerializedLength() +
				response->SerializedLength();
			assert(size == expectedSize);
			assert(size <= sizeof(buffer));

			USPiSendFrame(buffer, size);
		}

		// TODO Reboot the Pi when a system file was received
	}

	//
	// WriteReadRequestPacket
	//
	WriteReadRequestPacket::WriteReadRequestPacket() : Packet() {}
	WriteReadRequestPacket::WriteReadRequestPacket(const Opcode opcode) : Packet(opcode) {}

	size_t WriteReadRequestPacket::SerializedLength() const
	{
		return sizeof(opcode) + filename.size() + 1 + mode.size() + 1;
	}

	size_t WriteReadRequestPacket::Serialize(uint8_t* buffer, const size_t bufferSize) const
	{
		if (bufferSize < SerializedLength())
		{
			return 0;
		}

		size_t i = 0;
		buffer[i++] = static_cast<uint16_t>(opcode) >> 8;
		buffer[i++] = static_cast<uint16_t>(opcode);

		i += filename.copy(reinterpret_cast<char*>(buffer + i), filename.size());
		buffer[i++] = 0;

		i += mode.copy(reinterpret_cast<char*>(buffer + i), mode.size());
		buffer[i++] = 0;

		return i;
	}

	size_t WriteReadRequestPacket::Deserialize(const uint8_t* buffer, const size_t bufferSize)
	{
		// Can't use SerializedLength here, as it's variable.
		// Check for each field instead.
		size_t i = 0;

		if (sizeof(Opcode) >= bufferSize - i)
			return 0;
		opcode = static_cast<Opcode>(buffer[i] << 8 | buffer[i + 1]);
		i += 2;

		const char* filenameStr = reinterpret_cast<const char*>(buffer + i);
		if (std::strlen(filenameStr) + 1 >= bufferSize - i)
			return 0;
		filename = std::string(filenameStr);
		i += filename.size() + 1;

		const char* modeStr = reinterpret_cast<const char*>(buffer + i);
		if (std::strlen(modeStr) + 1 >= bufferSize - i)
			return 0;
		mode = std::string(modeStr);
		i += mode.size() + 1;

		return i;
	}

	//
	// ErrorPacket
	//
	ErrorPacket::ErrorPacket() : Packet(Opcode::Error) {}
	ErrorPacket::ErrorPacket(uint16_t errorCode, std::string message) :
		Packet(Opcode::Error), errorCode(errorCode), message(message)
	{
	}

	size_t ErrorPacket::SerializedLength() const
	{
		return sizeof(opcode) + sizeof(errorCode) + message.size() + 1;
	}

	size_t ErrorPacket::Serialize(uint8_t* buffer, const size_t bufferSize) const
	{
		if (bufferSize < SerializedLength())
		{
			return 0;
		}

		size_t i = 0;
		buffer[i++] = static_cast<uint16_t>(opcode) >> 8;
		buffer[i++] = static_cast<uint16_t>(opcode);
		buffer[i++] = errorCode >> 8;
		buffer[i++] = errorCode;

		i += message.copy(reinterpret_cast<char*>(buffer + i), message.size());
		buffer[i++] = 0;

		return i;
	}

	//
	// AcknowledgementPacket
	//
	AcknowledgementPacket::AcknowledgementPacket() : Packet(Opcode::Acknowledgement) {}

	AcknowledgementPacket::AcknowledgementPacket(uint16_t blockNumber) :
		Packet(Opcode::Acknowledgement), blockNumber(blockNumber)
	{
	}

	size_t AcknowledgementPacket::SerializedLength() const
	{
		return sizeof(opcode) + sizeof(blockNumber);
	}

	size_t AcknowledgementPacket::Serialize(uint8_t* buffer, const size_t bufferSize) const
	{
		if (bufferSize < SerializedLength())
		{
			return 0;
		}

		size_t i = 0;
		buffer[i++] = static_cast<uint16_t>(opcode) >> 8;
		buffer[i++] = static_cast<uint16_t>(opcode);
		buffer[i++] = blockNumber >> 8;
		buffer[i++] = blockNumber;
		return i;
	}

	//
	// DataPacket
	//
	DataPacket::DataPacket() : Packet(Opcode::Data), blockNumber(0) {}

	size_t DataPacket::SerializedLength() const
	{
		return sizeof(opcode) + sizeof(blockNumber) + data.size();
	}

	size_t DataPacket::Serialize(uint8_t* buffer, const size_t bufferSize) const
	{
		if (bufferSize <= SerializedLength())
		{
			return 0;
		}

		size_t i = 0;
		buffer[i++] = static_cast<uint16_t>(opcode) >> 8;
		buffer[i++] = static_cast<uint16_t>(opcode);
		buffer[i++] = blockNumber >> 8;
		buffer[i++] = blockNumber;

		std::memcpy(buffer + i, data.data(), data.size());
		i += data.size();

		return i;
	}

	size_t DataPacket::Deserialize(const uint8_t* buffer, const size_t bufferSize)
	{
		if (bufferSize < sizeof(opcode) + sizeof(blockNumber))
		{
			return 0;
		}

		opcode = static_cast<Opcode>(buffer[0] << 8 | buffer[1]);
		blockNumber = buffer[2] << 8 | buffer[3];
		data = std::vector<uint8_t>(buffer + 4, buffer + bufferSize);
		return bufferSize;
	}
} // namespace Net::Tftp
