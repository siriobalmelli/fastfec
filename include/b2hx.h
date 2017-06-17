#ifndef b2hx_h_
#define b2hx_h_

/*	b2hx.h		Binary-to-Hexadecimal

Fast library to convert values from binary to hexadecimal format.

(c) 2016 Sirio Balmelli; https://b-ad.ch
*/

#include <stddef.h>
#include <stdint.h>


size_t		b2hx	(const unsigned char *bin, char *hex, size_t byte_cnt);
/* Big Endian */
size_t		b2hx_BE	(const unsigned char *bin, char *hex, size_t byte_cnt);


size_t		b2hx_u16(const uint16_t *u16, size_t u_cnt, char *hex);
size_t		b2hx_u32(const uint32_t *u32, size_t u_cnt, char *hex);
size_t		b2hx_u64(const uint64_t *u64, size_t u_cnt, char *hex);


#endif /* b2hx_h_ */
