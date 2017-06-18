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
NLC_INLINE	uint64_t nm_div_ceil(uint64_t a, uint64_t b)
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
NLC_INLINE	uint32_t nm_next_pow2(uint32_t x)
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

/*	nm_next_mult32()
Returns `x` if `x` divides evenly into `mult`
Else returns next multiple of mult above x
*/
NLC_INLINE	uint32_t nm_next_mult32(uint32_t x, uint32_t mult)
{
	return ((x + (mult -1)) / mult) * mult;
}
NLC_INLINE	uint64_t nm_next_mult64(uint64_t x, uint64_t mult)
{
	return ((x + (mult -1)) / mult) * mult;
}


#endif /* nmath_h_ */
