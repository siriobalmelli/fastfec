/*	"bits" library

Collection of random functions for data manipulation or math.
*/

#include "bits.h"
/* index into this for bin_2_hex conversions */
static const char *syms = "0123456789abcdef";
union bits__{
	uint64_t u64;
	uint32_t u32;
	uint16_t u16;
	uint8_t u8[8];
};

size_t	u16_2_hex(const uint16_t *u16, size_t u_cnt, char *hex)
{
	int j=0;
	union bits__ tmp;
	for (int i=0; i < u_cnt; i++) {
		tmp.u16 = __builtin_bswap16(u16[i]);
		hex[j++]= syms[tmp.u8[0] >>4];
		hex[j++]= syms[tmp.u8[0] & 0xf];
		hex[j++]= syms[tmp.u8[1] >>4];
		hex[j++]= syms[tmp.u8[1] & 0xf];
	}
	hex[j++] = '\0';
	return j;
}

size_t	u32_2_hex(const uint32_t *u32, size_t u_cnt, char *hex)
{
	int j = 0;
	union bits__ tmp;
	for (int i=0; i < u_cnt; i++) {
		tmp.u32 = __builtin_bswap32(u32[i]);
		hex[j++]= syms[tmp.u8[0] >>4];
		hex[j++]= syms[tmp.u8[0] & 0xf];
		hex[j++]= syms[tmp.u8[1] >>4];
		hex[j++]= syms[tmp.u8[1] & 0xf];
		hex[j++]= syms[tmp.u8[2] >>4];
		hex[j++]= syms[tmp.u8[2] & 0xf];
		hex[j++]= syms[tmp.u8[3] >>4];
		hex[j++]= syms[tmp.u8[3] & 0xf];
	}
	hex[j++] = '\0';
	return j;
}

size_t	u64_2_hex(const uint64_t *u64, size_t u_cnt, char *hex)
{
	int j = 0;
	union bits__ tmp;
	for (int i=0; i < u_cnt; i++) {
		tmp.u64 = __builtin_bswap64(u64[i]);
		hex[j++]= syms[tmp.u8[0] >>4];
		hex[j++]= syms[tmp.u8[0] & 0xf];
		hex[j++]= syms[tmp.u8[1] >>4];
		hex[j++]= syms[tmp.u8[1] & 0xf];
		hex[j++]= syms[tmp.u8[2] >>4];
		hex[j++]= syms[tmp.u8[2] & 0xf];
		hex[j++]= syms[tmp.u8[3] >>4];
		hex[j++]= syms[tmp.u8[3] & 0xf];
		hex[j++]= syms[tmp.u8[4] >>4];
		hex[j++]= syms[tmp.u8[4] & 0xf];
		hex[j++]= syms[tmp.u8[5] >>4];
		hex[j++]= syms[tmp.u8[5] & 0xf];
		hex[j++]= syms[tmp.u8[6] >>4];
		hex[j++]= syms[tmp.u8[6] & 0xf];
		hex[j++]= syms[tmp.u8[7] >>4];
		hex[j++]= syms[tmp.u8[7] & 0xf];
	}
	hex[j++] = '\0';
	return j;
}

#if 1
/*	bin_2_hex()
Writes byte_cnt bytes as (byte_cnt *2 +1) ascii hex digits to the mem in `*out`.
	(+1 because there's a '\0' at the end).
Does NOT check that enough mem in `out` exists.
returns number of CHARACTERS written (should be `byte_cnt *2 +1`)
	*/
size_t bin_2_hex(const uint8_t *bin, char *hex, size_t byte_cnt)
{
	ssize_t i = 0;
	if (!bin || !hex || !byte_cnt)
		return 0;

	/* we swap byte order because of the endianness of Intel boxes */
	size_t hex_pos = byte_cnt *2;
	hex[hex_pos--] = '\0'; /* end of string */
	//for (; i < byte_cnt; i++) {
	for (i=byte_cnt-1; i >= 0; i--) {
		hex[hex_pos--] = syms[bin[i] & 0xf];
		hex[hex_pos--] = syms[bin[i] >>4];
	}

	return byte_cnt*2+1; /* return number of hex CHARACTERS written */
}
#endif

/*	div_ceil(a, b)
64-bit integer "ceiling" operation.
If 'b' divides evenly into 'a', returns 'a / b'.
Else, returns 'a / b + 1'.
*/
uint64_t	div_ceil		(uint64_t a, uint64_t b)
{
	uint64_t ret = a / b;
	if ((ret * b) < a)
		ret++;
	return ret;
}
