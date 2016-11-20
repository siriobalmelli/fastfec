#ifndef bits_h_
#define bits_h_

#include <stdint.h>
#include <unistd.h>

/* Atomically swap 'val' with '0' before running 'exec' on it.
Can be used e.g.: in check()->free() logic looking at shared variables.
*/
#define BITS_SWAP_EXEC(var, exec) { \
	typeof(var) bits_temp_; \
	bits_temp_ = (typeof(var))__atomic_exchange_n(&var, (typeof(var))0, __ATOMIC_RELAXED); \
	if (bits_temp_) { \
		exec(bits_temp_); \
		bits_temp_ = (typeof(var))0; \
	} \
}
/* Optionally, excecute 2 functions in sequence,
	e.g.: pthread_mutex_destroy() and  then free()
*/
#define BITS_SWAP_EXEC_2(var, exec1, exec2) { \
	typeof(var) bits_temp_; \
	bits_temp_ = (typeof(var))__atomic_exchange_n(&var, NULL, __ATOMIC_RELAXED); \
	if (bits_temp_) { \
		exec1(bits_temp_); \
		exec2(bits_temp_); \
		bits_temp_ = (typeof(var))0; \
	} \
}

size_t		bin_2_hex		(uint8_t *bin, char *hex, size_t byte_cnt);
uint64_t	div_ceil		(uint64_t a, uint64_t b);

#endif
