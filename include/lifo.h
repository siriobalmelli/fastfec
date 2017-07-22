#ifndef lifo_h_
#define lifo_h_

/*	Simple, very fast LIFO (stack)

The point is to very quickly push and pop "ID"s (integers, pointers) into a
	fast, memory-efficient LIFO (Last-In-First-Out).

This library calls malloc()/mmap() to alloc a LIFO,
	and if necessary grows by calling realloc()/mremap().
However, it NEVER shrinks the stack.

If you need a FIFO (queue):
-	Do you really NEED a queue?
	Reworking your algo to use a stack (LIFO) will be faster.
-	Can your queue have a maximum bound?
	If so a circular buffer is your best bet. Check out 'cbuf'.
-	Otherwise you have a choice between hash tables (e.g. Judy)
		or (gasp!) a linked list.

Thread-safety: NONE

(c) 2017, Sirio Balmelli - https://b-ad.ch
*/



/*	mmap
On systems which support it, use mmap() and mremap() directly.
*/
#ifdef __linux__
	#define _GNU_SOURCE
	#include <unistd.h>
	#include <sys/mman.h>
	#ifdef MAP_UNINITIALIZED
		#define LIFO_MMAP_FLAGS (MAP_PRIVATE | MAP_ANONYMOUS | MAP_UNINITIALIZED)
	#else
		#define LIFO_MMAP_FLAGS (MAP_PRIVATE | MAP_ANONYMOUS)
	#endif
#else
	#include <stdlib.h>
#endif



#include <stdint.h>
#include <stddef.h>  /* size_t */

#include <nonlibc.h>
#include <zed_dbg.h>



/*	Size of the 'tokens' being pushed and popped off this stack.
Allow caller to define their own type.
Otherwise, assume it should be able to hold a pointer or equivalent-sized uint.
*/
#ifndef LIFO_MEM_TYPE
	#define LIFO_MEM_TYPE uintptr_t
#endif

/*	Check function returns for LIFO_ERR where indicated.
*/
#define LIFO_ERR ((size_t)-1)

/*	How much with the stack grow by each time it is resized?
... in Bytes.
*/
#ifndef LIFO_GROW
	#ifdef LIFO_MMAP_FLAGS
		#define LIFO_GROW (4096 * 2048) /* anonymous 2M pages assumed */
	#else
		#define LIFO_GROW 1024 /* seems to get slower if this is higher? */
	#endif
#endif



/*	lifo
*/
struct lifo {
	size_t		next;		/* aka: 'cnt' ... it's an index */
	size_t		mem_len;	/* in Bytes */
	LIFO_MEM_TYPE	mem[];
};



/*
	public
*/
NLC_PUBLIC	__attribute__((warn_unused_result))
		struct lifo	*lifo_new();
NLC_PUBLIC	void		lifo_free(struct lifo *stk);
NLC_PUBLIC	__attribute__((warn_unused_result))
		struct lifo	*lifo_extend(struct lifo *stk);



/*	lifo_push()
*/
NLC_INLINE	size_t		lifo_push(struct lifo **stk, LIFO_MEM_TYPE push_this)
{
	struct lifo *sk = *stk;

	/* most of the time we do NOT need to extend the stack */
	if (__builtin_expect( !(sk->mem_len - (sk->next * sizeof(LIFO_MEM_TYPE))), 0))
		*stk = sk = lifo_extend(sk);

	sk->mem[sk->next] = push_this;
	return sk->next++;
}

/*	lifo_pop()
*/
NLC_INLINE	size_t		lifo_pop(struct lifo *stk, LIFO_MEM_TYPE *pop_here)
{
	if (stk->next) {
		*pop_here = stk->mem[--(stk->next)];
		return stk->next;
	}
	return LIFO_ERR;
}



#endif /* lifo_h_ */
