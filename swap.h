#ifndef swap_h_
#define swap_h_

/*	swap.h		byte swapping library

(c) 2016-2017 Sirio Balmelli; https://b-ad.ch
*/

#include <stdint.h>
#include <stdio.h> /* sscanf() */



/*
	NOTE: all functions without a '_BE' assume a Little-Endian byte order
*/
size_t		u16_2_hex	(const uint16_t *u16, size_t u_cnt, char *hex);
size_t		u32_2_hex	(const uint32_t *u32, size_t u_cnt, char *hex);
size_t		u64_2_hex	(const uint64_t *u64, size_t u_cnt, char *hex);
size_t		bin_2_hex	(const unsigned char *bin, char *hex, size_t byte_cnt);
size_t		bin_2_hex_BE	(const unsigned char *bin, char *hex, size_t byte_cnt);

int		hex_2_u16	(const char *hex, size_t u_cnt, uint16_t *out);
int		hex_2_u32	(const char *hex, size_t u_cnt, uint32_t *out);
int		hex_2_u64	(const char *hex, size_t u_cnt, uint64_t *out);
size_t		hex_2_bin	(const char *hex, size_t char_cnt, unsigned char *bin);



#endif /* swap_h_ */
