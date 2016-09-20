/*	FNV1a hash algorithm - 64bit

2016 Sirio Balmelli and Anthony Soenen
*/
#ifndef bits_fnv1a_h_
#define bits_fnv1a_h_

#include <stdint.h>
#include "zed_dbg.h"

#define FNV_OFFT_BASIS 14695981039346656037UL
#define FNV_PRIME 1099511628211UL

Z_INL_FORCE uint64_t	bits_fnv_seed()
{
	return FNV_OFFT_BASIS;
}

Z_INL_FORCE void	bits_fnv_hash(uint64_t *hash, uint8_t *data, size_t data_len)
{
	uint64_t i;
	for (i=0; i < data_len; i++) {
		*hash ^= data[i];
		*hash *= FNV_PRIME;
	}
}

#endif /* bits_fnv1a_h_ */
