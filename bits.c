/*	"bits" library

Collection of random functions for data manipulation or math.
*/

#include "bits.h"
#include <stdio.h> /* sscanf() */

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

/*	bin_2_hex_straight()
Writes byte_cnt bytes as (byte_cnt *2 +1) ascii hex digits to the mem in `*out`.
	(+1 because there's a '\0' at the end).
Does NOT check that enough mem in `out` exists.

Returns number of CHARACTERS written (should be `byte_cnt *2 +1`)

NOTE that we consider '*bin' to be a SEQUENTIAL FIELD of bytes,
	with LSB at the EARLIEST memory address.
If '*bin' would be an array of e.g.: uint32_t, then this function
	would output the WRONG result; use the uXX_2_hex() functions instead.
We output a single hex string, which is in "human" notation: MSB in front.
TODO: change 'bin' to const char *; update headers; grep all usage of the function and
	remove STUPID casts
*/
size_t bin_2_hex(const uint8_t *bin, char *hex, size_t byte_cnt)
{
	if (!bin || !hex || !byte_cnt)
		return 0;
	size_t hex_pos = 0;
	for (int i=byte_cnt-1; i >= 0; i--) {
		hex[hex_pos++] = syms[bin[i] >>4];
		hex[hex_pos++] = syms[bin[i] & 0xf];
	}
	hex[hex_pos++] = '\0'; /* end of string */

	return hex_pos; /* return number of hex CHARACTERS written */
}
/*	hex_2_bin()
Writes 'char_cnt' hex CHARACTERS as (char_cnt / 2) bytes in 'bin'.
Does NOT check that enough mem in 'bin' exists.
Does NOT expect or check for leading "0x" sequence.

Returns number of BYTES written.

NOTE: a hex string generated with bin_2_hex() will return the original sequence
	of bytes when reversed with hex_2_bin().
*/
size_t hex_2_bin(const char *hex, size_t char_cnt, uint8_t *bin)
{
	if (!bin || !hex || !char_cnt)
		return 0;

	/* get byte count */
	int bytes = (char_cnt >> 1) -1;
	/* work backwards */
	for (int i=0; bytes >=0; i+=2)
		sscanf(&hex[i], "%2hhx", &bin[bytes--]);

	return (char_cnt >> 1);
}

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

/*	next_pow2()
Returns next higher power of 2, or itself if already power of 2.
Shamelessly ripped off of an S/O thread.
	*/
uint32_t next_pow2(uint32_t x)
{
	if (!x)
		return 0;
	x--;
	x |= x >> 1;
	x |= x >> 2;
	x |= x >> 4;
	x |= x >> 8;
	x |= x >> 16;

	return x+1;
}

/*	next_mult32()
Returns `x` if `x` divides evenly into `mult`
Else returns next multiple of mult above x
*/
uint32_t next_mult32(uint32_t x, uint32_t mult)
{
	return ((x + (mult -1)) / mult) * mult;
}
uint64_t next_mult64(uint64_t x, uint64_t mult)
{
	return ((x + (mult -1)) / mult) * mult;
}

