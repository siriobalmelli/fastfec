#ifndef nmath_h_
#define nmath_h_

/*	nmath.h		Nonlibc MATH

All the annoying little math snippets which aren't elsewere;
	purely as inlines.

(c) 2016-2017 Sirio Balmelli; https://b-ad.ch
*/

#include <stdint.h>
#include <nonlibc.h>


/*	nm_div_ceil(a, b)
64-bit integer "ceiling" operation.
If 'b' divides evenly into 'a', returns 'a / b'.
Else, returns 'a / b + 1'.
*/
NLC_INLINE	__attribute__((pure))
		uint64_t nm_div_ceil(uint64_t a, uint64_t b)
{
	uint64_t ret = a / b;
	if ((ret * b) < a)
		ret++;
	return ret;
}


/*	nm_next_pow2()
Returns next higher power of 2, or itself if already power of 2.
Shamelessly ripped off of an S/O thread.
*/
NLC_INLINE	__attribute__((pure))
		uint32_t nm_next_pow2_32(uint32_t x)
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
NLC_INLINE	__attribute__((pure))
		uint64_t nm_next_pow2_64(uint64_t x)
{
	if (!x)
		return 0;
	x--;
	x |= x >> 1;
	x |= x >> 2;
	x |= x >> 4;
	x |= x >> 8;
	x |= x >> 16;
	x |= x >> 32;

	return x+1;
}


/*	nm_next_mult32()
Returns `x` if `x` divides evenly into `mult`
Else returns next multiple of mult above x
*/
NLC_INLINE	__attribute__((pure))
		uint32_t nm_next_mult32(uint32_t x, uint32_t mult)
{
	return ((x + (mult -1)) / mult) * mult;
}
NLC_INLINE	__attribute__((pure))
		uint64_t nm_next_mult64(uint64_t x, uint64_t mult)
{
	return ((x + (mult -1)) / mult) * mult;
}


/*	nm_bit_pos()
Returns the index (1-based!) of lowest set bit in 'uint'.
If no bits are set, returns 0.

NOTE the index is 1-based (so that we return 0 in case no bits are set,
	as opposed to e.g. -1: this makes using the output of this
	call as an array index relatively safe).
1-based index means that 0x80 returns '8', NOT '7'.
*/
NLC_INLINE	__attribute__((pure))
		uint_fast8_t nm_bit_pos(uint64_t uint)
{
	unsigned int ret = 0;
	while (ret++ < (sizeof(uint) * 8)) {
		if (uint & 0x1)
			return ret;
		uint >>= 1;
	}
	return 0;
}


#endif /* nmath_h_ */
