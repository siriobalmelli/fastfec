#ifndef bits_h_
#define bits_h_

#include <stdint.h>
#include <unistd.h>

size_t		bin_2_hex		(uint8_t *bin, char *hex, size_t byte_cnt);
uint64_t	div_ceil		(uint64_t a, uint64_t b);

#endif
