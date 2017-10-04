#ifndef nonlibc_h_

/*	nonlibc.h	nonlibc's generic header file

This header file exports defines and macros to clean up ugly code and abstract
	away compiler- or platform-dependent functionality.

(c) 2017 Sirio Balmelli; https://b-ad.ch
*/



/*	compile-time checks (size, etc)
A macro to allow for compile-time checks where the CPP is missing info,
	such as sizeof().
Exploit the fact that a '0' bitfield throws a compiler error.
*/
#define NLC_ASSERT(name, expr) \
	struct name { unsigned int bitfield : (expr); }



/*	inlining!
Use this in header files when defining inline functions for use library callers.
The effect is the same as a macro, except these are actually legibile ;)
 */
#define	NLC_INLINE static inline __attribute__((always_inline))



/*	visibility!
Visibility is important for motives of cleanliness, performance and bug-catching.
The recommended approach is:
-	Declare ALL functions (even static helper functions not meant for use by caller)
		in header files.
-	Prefix each declaration with a PUBLIC/LOCAL macro as defined below.

And that's it.
You'll keep track of all your functions (and not lose "private/local" functions
	buried somewhere in a .c file), BUT the exported symbols of your library
	will be exactly and only that which you intend to export.
*/
#if (__GNUC__ >= 4) || defined(__clang__)
	#define NLC_PUBLIC __attribute__ ((visibility ("default")))
	#define NLC_LOCAL  __attribute__ ((visibility ("hidden")))
#else
	#define NLC_PUBLIC
	#define NLC_LOCAL
#endif



/*	benchmarking!
Use these macros to time segments of code, without:
	- being tricked by compiler or CPU reordering
	- dealing with a bunch of kludge

Calls clock(), which on libc can be obtained with:
*/
#include <time.h>

#define nlc_timing_tp2u64(tp) \
	((uint64_t)tp.tv_nsec + ((uint64_t)tp.tv_sec * 1000000000))

#define nlc_timing_start(timer_name) \
	struct timespec tp_##timer_name = { 0 }; \
	clock_gettime(CLOCK_MONOTONIC, &tp_##timer_name); \
	clock_t cpu_##timer_name = clock(); \
	__atomic_thread_fence(__ATOMIC_SEQ_CST);

#define nlc_timing_stop(timer_name) \
	__atomic_thread_fence(__ATOMIC_SEQ_CST); \
	cpu_##timer_name = clock() - cpu_##timer_name; \
	uint64_t wall_##timer_name = nlc_timing_tp2u64(tp_##timer_name); \
	clock_gettime(CLOCK_MONOTONIC, &tp_##timer_name); \
	wall_##timer_name = nlc_timing_tp2u64(tp_##timer_name) - wall_##timer_name;

/* calculate time in fractional seconds */
#define nlc_timing_cpu(timer_name) \
	((double)cpu_##timer_name / CLOCKS_PER_SEC) /* e.g.: with %.2lf printf() format */
#define nlc_timing_wall(timer_name) \
	((double)wall_##timer_name / 1000000000)


#define nonlibc_h_
#endif /* nonlibc_h_ */
