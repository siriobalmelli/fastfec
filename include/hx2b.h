#ifndef hx2b_h_
#define hx2b_h_

/*	hx2b.h		Hexadecimal-to-Binary

Parse hexadecimal strings into binary integers or bytefields.

(c) 2016-2017 Sirio Balmelli; https://b-ad.ch
*/

#include <stddef.h>
#include <stdint.h>
#include <string.h> /* memset() */


/* use this one for single bytes and long sequences */
size_t		hx2b	(const char *hex, uint8_t *out, size_t bytes);

// TODO: proper inline macros
inline uint8_t	hx2b_u8		(const char *hex)
{
	#pragma GCC diagnostic ignored "-Wuninitialized"
		uint8_t ret;
	hx2b(hex, &ret, sizeof(ret));
	return ret;
}
inline uint16_t	hx2b_u16	(const char *hex)
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
inline uint32_t	hx2b_u32	(const char *hex)
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
inline uint64_t	hx2b_u64	(const char *hex)
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
