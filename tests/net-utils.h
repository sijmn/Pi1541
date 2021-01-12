void TestNetUtilsInternetChecksum()
{
	{
		const char buffer[] = {1, 2, 3, 4, 5, 6, 7, 8};
		uint16_t checksum = Net::Utils::InternetChecksum(buffer, sizeof(buffer));
		TEST_CHECK(checksum == 61419);
	}

	{
		const char buffer[] = {1, 2, 3, 4, 5, 6, 7};
		uint16_t checksum = Net::Utils::InternetChecksum(buffer, sizeof(buffer));
		TEST_CHECK(checksum == 61427);
	}

	{
		const char buffer[] = {};
		uint16_t checksum = Net::Utils::InternetChecksum(buffer, sizeof(buffer));
		TEST_CHECK(checksum == 65535);
	}
}

void TestNetUtilsCrc32()
{
	{
		const uint8_t buffer[] = {1, 2, 3, 4, 5, 6, 7, 8};
		uint32_t checksum = Net::Utils::Crc32(buffer, sizeof(buffer));
		TEST_CHECK(checksum == 1070237893);
	}

	{
		const uint8_t buffer[] = {};
		uint32_t checksum = Net::Utils::Crc32(buffer, sizeof(buffer));
		TEST_CHECK(checksum == 0);
	}
}

void TestNetUtilsGetMacAddress()
{
	auto mac = Net::Utils::GetMacAddress();
	auto expected = Net::Utils::MacAddress{0x00, 0x10, 0x20, 0x30, 0x40, 0x50};
	TEST_CHECK(mac == expected);
}
