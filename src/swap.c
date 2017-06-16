#include "swap.h"

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

/*	bin_2_hex()
Writes byte_cnt bytes as (byte_cnt *2 +1) ascii hex digits to the mem in `*out`.
	(+1 because trailing '\0').
Does NOT check that enough mem in `out` exists.

Returns number of CHARACTERS written (should be `byte_cnt *2 +1`)

NOTE that we consider '*bin' to be a SEQUENTIAL FIELD of bytes,
	with LSB at the EARLIEST memory address (little-endian).
If '*bin' would be an array of e.g.: uint32_t, then this function
	would output the WRONG result; use the uXX_2_hex() functions instead.
If '*bin' is big-endian, use bin_2_hex_BE() instead.

We output a single hex string, which is in "human" notation: MSB in front.

NOTE: 'bin' MUST be unsigned, else array indexing with bitshift yields
	negative values :O
*/
size_t bin_2_hex(const unsigned char *bin, char *hex, size_t byte_cnt)
{
	if (!bin || !hex || !byte_cnt)
		return 0;

	size_t hex_pos = 0;
	for (int i=byte_cnt-1; i >= 0; i--) {
		hex[hex_pos++] = syms[bin[i] >> 4];
		hex[hex_pos++] = syms[bin[i] & 0xf];
	}
	hex[hex_pos++] = '\0'; /* end of string */

	return hex_pos; /* return number of hex CHARACTERS written */
}
/*	bin_2_hex_BE()
Big-endian version of bin_2_hex() above.
*/
size_t		bin_2_hex_BE	(const unsigned char *bin, char *hex, size_t byte_cnt)
{
	if (!bin || !hex || !byte_cnt)
		return 0;

	size_t hex_pos = 0;
	for (int i=0; i < byte_cnt; i++) {
		hex[hex_pos++] = syms[bin[i] >> 4];
		hex[hex_pos++] = syms[bin[i] & 0xf];
	}
	hex[hex_pos++] = '\0'; /* end of string */

	return hex_pos; /* return number of hex CHARACTERS written */
}

/*	hex_2_u16()
returns 0 on success.
*/
int hex_2_u16(const char *hex, size_t u_cnt, uint16_t *out)
{
	for (int i=0; i < u_cnt; i++, hex += 8) {
		if (sscanf(hex, "%hx", &out[i]) != 1)
			return 1;
	}

	return 0;
}

/*	hex_2_u32()
returns 0 on success.
*/
int hex_2_u32(const char *hex, size_t u_cnt, uint32_t *out)
{
	for (int i=0; i < u_cnt; i++, hex += 8) {
		if (sscanf(hex, "%x", &out[i]) != 1)
			return 1;
	}

	return 0;
}

/*	hex_2_u64()
returns 0 on success.
*/
int hex_2_u64(const char *hex, size_t u_cnt, uint64_t *out)
{
	for (int i=0; i < u_cnt; i++, hex += 16) {
		if (sscanf(hex, "%lx", &out[i]) != 1)
			return 1;
	}

	return 0;
}

/*	hex_2_bin()
Writes 'char_cnt' hex CHARACTERS as (char_cnt / 2) bytes in 'bin'.
Does NOT check that enough mem in 'bin' exists.
Does NOT expect or check for leading "0x" sequence.

Returns number of BYTES written.

NOTE: a hex string generated with bin_2_hex() will return the original sequence
	of bytes when reversed with hex_2_bin().
*/
size_t hex_2_bin(const char *hex, size_t char_cnt, unsigned char *bin)
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
