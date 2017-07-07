#ifndef atop_h_
#define atop_h_

/*	atop.h		ATomic OPerations

These are NOT yet rigorously tested.
Please keep an eye out for unusual behavior and let me know right away.
Thank you, Sirio

(c) 2016-2017 Sirio Balmelli; https://b-ad.ch
*/

#include <stdint.h>
#include <unistd.h>	/* pause() */



/*	ATOP_SWAP_EXEC

Atomically exchange the value in 'var' with 'swap';
	if the exchanged value is different from 'swap', pass it to 'exec'.
Can be used e.g.: in check()->free() logic looking at shared variables.

var	:	variable name
swap	:	value e.g. '0' or 'NULL'
exec	:	function pointer e.g. 'free'
*/
#define ATOP_SWAP_EXEC(var, swap, exec) { \
	typeof(var) bits_temp_; \
	bits_temp_ = (typeof(var))__atomic_exchange_n(&var, (typeof(var))swap, __ATOMIC_ACQUIRE); \
	if (bits_temp_ != ((typeof(var))swap)) { \
		exec(bits_temp_); \
		bits_temp_ = (typeof(var))swap; \
	} \
}



/*	"transaction" utilities.
Allows caller to utilize a lock-free 'atop_txn' variable to signal:
	a.) Whether it is safe to begin a "transaction" (area of code)
	b.) How many operations are "in progress"

To init a new 'atop_txn', set it to 'atop_txn_init'.
Then, call _begin() and _end() on it to log operations as they start and finish.
Finally, call _kill() to:
	- mark 'op' as "no_start", after which all calls to _begin() will
		return "unsafe".
	- spinlock until all current operations have called _end().
	- mark 'op' as "unsafe" and return.
*/
typedef int atop_txn;
#define atop_txn_unsafe ((__typeof__(atop_txn))-1)		/* ALL the bits are '1' if "unsafe" */
#define atop_txn_nostart (0x1 << (sizeof(atop_txn) * 8 - 1))	/* MSb is '1' if "nostart " OR "unsafe" */
#define atop_txn_init ((__typeof__(atop_txn))0)			/* No pending ops at init */

/*	atop_txn_begin()
Try to add a new operation to 'op'.
Returns 'atop_txn_unsafe' if 'op' is "no_start" or "unsafe",
	otherwise returns number of operations begun (including this one).
*/
static inline __attribute__((always_inline))	atop_txn atop_txn_begin(atop_txn *op)
{
	atop_txn local;
	do {
		local = __atomic_load_n(op, __ATOMIC_ACQUIRE);
		if (local & atop_txn_nostart)
			return atop_txn_unsafe;
	} while(!__atomic_compare_exchange_n(op, &local, local+1, 1, 
			__ATOMIC_ACQUIRE, __ATOMIC_ACQUIRE)
		&& !pause()
		);

	/* how many ops active, including ours */
	return local+1;
}

/*	atop_txn_end()
Marks an operation as completed.
All operations started with _begin() MUST be ended, or else _kill() will
	loop infinitely.
Returns number of operations still active.
*/
static inline __attribute__((always_inline))	atop_txn atop_txn_end(atop_txn *op)
{
	/* Assume that op_begin() WAS called before calling op_end, so don't
		try and sanity check for -1.
	*/
	atop_txn ret = __atomic_sub_fetch(op, 1, __ATOMIC_ACQUIRE);
	/* only return number of operations, NOT any possible "nostart" flag */
	return ret & ~atop_txn_nostart;
}

/*	atop_txn_kill()
Marks 'op' as "nostart" and then spinlocks until all operations have called _end(),
	then sets 'op' to "unsafe" and returns.
*/
static inline __attribute__((always_inline))	void atop_txn_kill(atop_txn *op)
{
	atop_txn expect = atop_txn_nostart;
	do {
		if (__atomic_fetch_or(op, atop_txn_nostart, __ATOMIC_ACQUIRE) == -1)
			return;
	} while(!__atomic_compare_exchange_n(op, &expect, atop_txn_unsafe, 1, 
			__ATOMIC_ACQUIRE, __ATOMIC_ACQUIRE)
		&& !pause()
		);
	return;
}


#endif /* atop_h_ */
