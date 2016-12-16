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

size_t		u16_2_hex	(const uint16_t *u16, size_t u_cnt, char *hex);
size_t		u32_2_hex	(const uint32_t *u32, size_t u_cnt, char *hex);
size_t		u64_2_hex	(const uint64_t *u64, size_t u_cnt, char *hex);
size_t		bin_2_hex	(const unsigned char *bin, char *hex, size_t byte_cnt);

int		hex_2_u16	(const char *hex, size_t u_cnt, uint16_t *out);
int		hex_2_u32	(const char *hex, size_t u_cnt, uint32_t *out);
int		hex_2_u64	(const char *hex, size_t u_cnt, uint64_t *out);
size_t		hex_2_bin	(const char *hex, size_t char_cnt, unsigned char *bin);

/*
	math bits
*/
uint64_t	div_ceil	(uint64_t a, uint64_t b);
uint32_t	next_pow2	(uint32_t x);
uint32_t	next_mult32	(uint32_t x, uint32_t mult);
uint64_t	next_mult64	(uint64_t x, uint64_t mult);


/*	bits_pause_()
Proper handling for failed HLE
*/
static inline __attribute__((always_inline))	int bits_pause_()
{
	__asm__ ( "pause;" );
	return 0;
}


/*	"bits op" utilities.
Allows caller to utilize a 'bits_op' variable to signal:
	a.) Whether it is safe to begin an operation
	b.) How many operations are "in progress"

To init a new 'bits_op', set it to 'bits_op_init'.
Then, call _begin() and _end() on it to log operations as they start and finish.
Finally, call _kill() to:
	- mark 'op' as "no_start", after which all calls to _begin() will
		return "unsafe".
	- wait until all current operations have called _end().
	- mark 'op' as "unsafe" and return.
*/
typedef uint32_t bits_op;
#define bits_op_unsafe ((__typeof__(bits_op))-1)		/* ALL the bits are '1' if "unsafe" */
#define bits_op_nostart (0x1 << (sizeof(bits_op) * 8 - 1))	/* MSb is '1' if "nostart " OR "unsafe" */
#define bits_op_init ((__typeof__(bits_op))0)			/* No pending ops at init */

/*	bits_op_begin()
Try to add a new operation to 'op'.
Returns 'bits_op_unsafe' if 'op' is "no_start" or "unsafe",
	otherwise returns number of operations begun (including this one).
*/
static inline __attribute__((always_inline)) uint32_t bits_op_begin(bits_op *op)
{
	bits_op local;
	do {
		local = __atomic_load_n(op, __ATOMIC_ACQUIRE | __ATOMIC_HLE_ACQUIRE);
		if (local & bits_op_nostart)
			return bits_op_unsafe;
	} while(!__atomic_compare_exchange_n(op, &local, local+1, 1, 
			__ATOMIC_ACQUIRE | __ATOMIC_HLE_ACQUIRE, __ATOMIC_RELAXED)
		&& !pause()
		);

	/* how many ops active, including ours */
	return local+1;
}

/*	bits_op_end()
Marks an operation as completed.
All operations started with _begin() MUST be ended, or else _kill() will
	loop infinitely.
Returns number of operations still active.
*/
static inline __attribute__((always_inline)) uint32_t bits_op_end(bits_op *op)
{
	/* Assume that op_begin() WAS called before calling op_end, so don't
		try and sanity check for -1.
	*/
	bits_op ret = __atomic_sub_fetch(op, 1,
				__ATOMIC_ACQUIRE | __ATOMIC_HLE_ACQUIRE);
	/* only return number of operations, NOT any possible "nostart" flag */
	return ret & ~bits_op_nostart;
}

/*	bits_op_kill()
Marks 'op' as "nostart" and then loops until all operations have called _end(),
	then set 'op' to "unsafe" and returns.
*/
static inline __attribute__((always_inline)) void bits_op_kill(bits_op *op)
{
	uint32_t expect = bits_op_nostart;
	do {
		if (__atomic_fetch_or(op, bits_op_nostart,
				__ATOMIC_ACQUIRE | __ATOMIC_HLE_ACQUIRE) == -1)
			return;
	} while(!__atomic_compare_exchange_n(op, &expect, bits_op_unsafe, 1, 
			__ATOMIC_ACQUIRE | __ATOMIC_HLE_ACQUIRE, __ATOMIC_RELAXED)
		&& !pause()
		);
	return;
}

#endif
