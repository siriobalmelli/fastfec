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

	char hex[32];
	Z_die_if(u16_2_hex(&n_16, 1, hex) != 5, "");
	Z_die_if(strcmp(hex, hex_16), "'%s' not expected '%s'", hex, hex_16);

	Z_die_if(u32_2_hex(&n_32, 1, hex) != 9, "");
	Z_die_if(strcmp(hex, hex_32), "'%s' not expected '%s'", hex, hex_32);

	Z_die_if(u64_2_hex(&n_64, 1, hex) != 17, "");
	Z_die_if(strcmp(hex, hex_64), "'%s' not expected '%s'", hex, hex_64);

out:
	return err_cnt;
}
