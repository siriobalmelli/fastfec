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

#endif /* atop_h_ */
