/*	"bits" library

Collection of random functions for data manipulation or math.
*/

#include "bits.h"

/*	bin_2_hex()
Writes byte_cnt bytes as (byte_cnt *2 +1) ascii hex digits to the mem in `*out`.
	(+1 because there's a '\0' at the end).
Does NOT check that enough mem in `out` exists.
returns number of CHARACTERS written (should be `byte_cnt *2 +1`)
	*/
size_t bin_2_hex(uint8_t *bin, char *hex, size_t byte_cnt)
{
	ssize_t i = 0;
	if (!bin || !hex || !byte_cnt)
		return 0;

	/* index into this to get the digit being written */
	static const char *syms = "0123456789abcdef";

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
