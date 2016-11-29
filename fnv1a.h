/*	FNV1a hash algorithm - 64bit

2016 Sirio Balmelli and Anthony Soenen
*/
#ifndef fnv1a_h_
#define fnv1a_h_

#include <stdint.h>

uint64_t	fnv_hash_l(uint64_t *hash, void *data, uint64_t data_len);
uint32_t	fnv_hash(uint32_t *hash, void *data, uint64_t data_len);

#endif /* fnv1a_h_ */
