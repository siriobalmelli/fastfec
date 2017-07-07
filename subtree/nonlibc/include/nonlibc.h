#ifndef nonlibc_h_

/*	nonlibc.h	nonlibc's generic header file

This header file exports defines and macros to clean up ugly code and abstract
	away compiler- or platform-dependent functionality.

(c) 2017 Sirio Balmelli; https://b-ad.ch
*/


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
#if __GNUC__ >= 4
	#define NLC_PUBLIC __attribute__ ((visibility ("default")))
	#define NLC_LOCAL  __attribute__ ((visibility ("hidden")))
#else
	#define NLC_PUBLIC
	#define NLC_LOCAL
#endif

#define nonlibc_h_
#endif /* nonlibc_h_ */
