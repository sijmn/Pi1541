#include <memory>

#include "ff.h"
#include "net-arp.h"
#include "net-ethernet.h"
#include "net-ipv4.h"
#include "net-tftp.h"
#include "net-udp.h"
#include "net.h"

#include "types.h"
#include <uspi.h>

namespace Net::Tftp
{
	// TODO Allow multiple files open
	static FIL outFile;
	static bool shouldReboot = false;
	static uint32_t currentBlockNumber = -1;

	static std::unique_ptr<Packet> handleTftpWriteRequest(const uint8_t* data)
	{
		auto packet = WriteReadRequestPacket::Deserialize(data);

		// TODO Implement netscii, maybe
		if (packet.mode != "octet")
		{
			return std::unique_ptr<ErrorPacket>(
				new ErrorPacket(0, "please use mode octet")
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

		std::unique_ptr<Packet> response;
		if (result != FR_OK)
		{
			response = std::unique_ptr<ErrorPacket>(
				new ErrorPacket(0, "error opening target file")
			);
		}
		else
		{
			shouldReboot =
				packet.filename == "kernel.img" || packet.filename == "options.txt";
			response = std::unique_ptr<AcknowledgementPacket>(
				new AcknowledgementPacket(currentBlockNumber)
			);
		}

		// TODO Return to the original working directory here

		return response;
	}

	static std::unique_ptr<Packet> handleTftpData(const uint8_t* buffer, size_t size)
	{
		DataPacket packet;
		const auto tftpSize = DataPacket::Deserialize(packet, buffer, size);
		if (size == 0)
		{
			// TODO log
			return nullptr;
		}

		if (packet.blockNumber != currentBlockNumber + 1)
		{
			f_close(&outFile);
			return std::unique_ptr<ErrorPacket>(
				new ErrorPacket(0, "invalid block number")
			);
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
			new AcknowledgementPacket(currentBlockNumber)
		);
	}

	void HandlePacket(
		const Ethernet::Header ethernetReqHeader,
		const Ipv4Header ipv4ReqHeader,
		const Udp::Header udpReqHeader,
		const uint8_t* data
	) {
		const auto opcode = static_cast<Opcode>(data[0] << 8 | data[1]);
		std::unique_ptr<Packet> response;
		bool last = false;

		if (opcode == Opcode::WriteRequest)
		{
			response = handleTftpWriteRequest(data);
		}
		else if (opcode == Opcode::Data)
		{
			const auto length = udpReqHeader.length - Udp::Header::SerializedLength();
			response = handleTftpData(data, length);
		}
		else
		{
			response = std::unique_ptr<ErrorPacket>(
				new ErrorPacket(4, "not implemented yet")
			);
		}

		if (response != nullptr)
		{
			Udp::Header udpRespHeader(
				udpReqHeader.destinationPort,
				udpReqHeader.sourcePort,
				response->SerializedLength() + Udp::Header::SerializedLength()
			);
			Ipv4Header ipv4RespHeader(
				IP_PROTO_UDP,
				Utils::Ipv4Address,
				ipv4ReqHeader.sourceIp,
				udpRespHeader.length + Ipv4Header::SerializedLength()
			);
			Ethernet::Header ethernetRespHeader(
				Arp::ArpTable[ipv4RespHeader.destinationIp],
				Utils::GetMacAddress(),
				Ethernet::ETHERTYPE_IPV4
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
	// WriteReadRequestPacket
	//
	WriteReadRequestPacket::WriteReadRequestPacket(const Opcode opcode) :
		Packet(opcode)
	{}

	size_t WriteReadRequestPacket::SerializedLength() const
	{
		return Packet::SerializedLength() + filename.size() + 1 + mode.size() + 1;
	}

	size_t WriteReadRequestPacket::Serialize(uint8_t* buffer) const
	{
		size_t i = 0;
		buffer[i++] = static_cast<uint16_t>(opcode) >> 8;
		buffer[i++] = static_cast<uint16_t>(opcode);

		i += filename.copy(reinterpret_cast<char*>(buffer + i), filename.size());
		buffer[i++] = 0;

		i += mode.copy(reinterpret_cast<char*>(buffer + i), mode.size());
		buffer[i++] = 0;

		return i;
	}

	WriteReadRequestPacket WriteReadRequestPacket::Deserialize(const uint8_t* buffer)
	{
		size_t i = 0;

		const auto opcode = static_cast<Opcode>(buffer[i] << 8 | buffer[i + 1]);
		WriteReadRequestPacket self(opcode);
		i += 2;

		self.filename = reinterpret_cast<const char*>(buffer + i);
		i += self.filename.size() + 1;

		self.mode = reinterpret_cast<const char*>(buffer + i);
		i += self.mode.size() + 1;

		return self;
	}

	//
	// ErrorPacket
	//
	ErrorPacket::ErrorPacket() : Packet(Opcode::Error) {}
	ErrorPacket::ErrorPacket(uint16_t errorCode, std::string message) :
		Packet(Opcode::Error), errorCode(errorCode), message(message)
	{}

	size_t ErrorPacket::SerializedLength() const
	{
		return Packet::SerializedLength() + sizeof(errorCode) + message.size() + 1;
	}

	size_t ErrorPacket::Serialize(uint8_t* buffer) const
	{
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
	AcknowledgementPacket::AcknowledgementPacket() :
		Packet(Opcode::Acknowledgement)
	{}

	AcknowledgementPacket::AcknowledgementPacket(uint16_t blockNumber) :
		Packet(Opcode::Acknowledgement), blockNumber(blockNumber)
	{}

	size_t AcknowledgementPacket::SerializedLength() const
	{
		return Packet::SerializedLength() + sizeof(blockNumber);
	}

	size_t AcknowledgementPacket::Serialize(uint8_t* buffer) const
	{
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
	DataPacket::DataPacket() : Packet(Opcode::Data), blockNumber(0)
	{}

	size_t DataPacket::Serialize(uint8_t* buffer) const
	{
		size_t i = 0;
		buffer[i++] = static_cast<uint16_t>(opcode) >> 8;
		buffer[i++] = static_cast<uint16_t>(opcode);
		buffer[i++] = blockNumber >> 8;
		buffer[i++] = blockNumber;

		std::memcpy(buffer + i, data.data(), data.size());
		i += data.size();

		return i;
	}

	size_t DataPacket::Deserialize(
		DataPacket& out, const uint8_t* buffer, size_t size
	) {
		if (size < sizeof(opcode) + sizeof(blockNumber)) {
			return 0;
		}

		out.opcode = static_cast<Opcode>(buffer[0] << 8 | buffer[1]);
		out.blockNumber = buffer[2] << 8 | buffer[3];
		out.data = std::vector<uint8_t>(buffer + 4, buffer + size);
		return size;
	}
}; // namespace Net::Tftp
