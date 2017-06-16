#include "bits.h"
#include "zed_dbg.h"

int main()
{
	int err_cnt = 0;

	const uint16_t n_16 = 0x1234;
	const char *hex_16 = "1234";
	const uint32_t n_32 = 0x12345678;
	const char *hex_32 = "12345678";
	const uint64_t n_64 = 0x1234567890123456;
	const char *hex_64 = "1234567890123456";

	char hex[33];
	Z_die_if(u16_2_hex(&n_16, 1, hex) != 5, "");
	Z_die_if(strcmp(hex, hex_16), "'%s' not expected '%s'", hex, hex_16);
	uint16_t cmp_16;
	Z_die_if(hex_2_u16(hex, 1, &cmp_16), "");
	Z_die_if(cmp_16 != n_16, "%hd != %hd", cmp_16, n_16);

	Z_die_if(u32_2_hex(&n_32, 1, hex) != 9, "");
	Z_die_if(strcmp(hex, hex_32), "'%s' not expected '%s'", hex, hex_32);
	uint32_t cmp_32;
	Z_die_if(hex_2_u32(hex, 1, &cmp_32), "");
	Z_die_if(cmp_32 != n_32, "%d != %d", cmp_32, n_32);

	Z_die_if(u64_2_hex(&n_64, 1, hex) != 17, "");
	Z_die_if(strcmp(hex, hex_64), "'%s' not expected '%s'", hex, hex_64);
	uint64_t cmp_64;
	Z_die_if(hex_2_u64(hex, 1, &cmp_64), "");
	Z_die_if(cmp_64 != n_64, "%ld != %ld", cmp_64, n_64);

	const unsigned char byte_field[] = { 0xa0, 0xb1, 0xc2, 0xd3, 0xe4, 0xf5, 0x06, 0x17,
			0x28, 0x39, 0x4a, 0x5b, 0x6c, 0x7d, 0x8e, 0x9f };
	Z_die_if(bin_2_hex(byte_field, hex, 16) != 33, "");
	unsigned char bytes_parse[16];
	Z_die_if(hex_2_bin(hex, 32, bytes_parse) != 16, "");
	Z_die_if(memcmp(byte_field, bytes_parse, 16), "");

out:
	return err_cnt;
}
