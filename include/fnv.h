#ifndef fnv_h_
#define fnv_h_

/*	fnv.h		FNV1a hash algorithm

(c) 2016 Sirio Balmelli and Anthony Soenen; https://b-ad.ch
*/

#include <stdint.h>
#include <nonlibc.h>


NLC_PUBLIC uint64_t	fnv_hash64(uint64_t *hash, const void *data, uint64_t data_len);
NLC_PUBLIC uint32_t	fnv_hash32(uint32_t *hash, const void *data, uint64_t data_len);


#endif /* fnv_h_ */
