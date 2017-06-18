#ifndef hx2b_h_
#define hx2b_h_

/*	hx2b.h		Hexadecimal-to-Binary

Parse hexadecimal strings into binary integers or bytefields.

(c) 2016-2017 Sirio Balmelli; https://b-ad.ch
*/

#include <nonlibc.h>
#include <stdint.h>
#include <string.h> /* memset() */

/* internal things */
NLC_LOCAL	const char *hex_burn_leading_(const char *hex);
NLC_LOCAL	uint8_t hex_parse_nibble_(const char *hex);


/* use this one for single bytes and long sequences */
NLC_PUBLIC	size_t hx2b(const char *hex, uint8_t *out, size_t bytes);


NLC_INLINE uint8_t	hx2b_u8		(const char *hex)
{
	#pragma GCC diagnostic ignored "-Wuninitialized"
		uint8_t ret;
	hx2b(hex, &ret, sizeof(ret));
	return ret;
}

NLC_INLINE uint16_t	hx2b_u16	(const char *hex)
{
	union bits__ {
		uint16_t	u16;
		uint8_t		u8[2];
	};
	#pragma GCC diagnostic ignored "-Wuninitialized"
		union bits__ ret;
	hx2b(hex, ret.u8, sizeof(uint16_t));
	return ret.u16;
}

NLC_INLINE uint32_t	hx2b_u32	(const char *hex)
{
	union bits__ {
		uint32_t	u32;
		uint8_t		u8[4];
	};
	#pragma GCC diagnostic ignored "-Wuninitialized"
		union bits__ ret;
	hx2b(hex, ret.u8, sizeof(uint32_t));
	return ret.u32;
}

NLC_INLINE uint64_t	hx2b_u64	(const char *hex)
{
	union bits__ {
		uint64_t	u64;
		uint8_t		u8[8];
	};
	#pragma GCC diagnostic ignored "-Wuninitialized"
		union bits__ ret;
	hx2b(hex, ret.u8, sizeof(uint64_t));
	return ret.u64;
}


#endif /* hx2b_h_ */
