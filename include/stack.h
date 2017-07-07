#ifndef stack_h_
#define stack_h_

/*	Simple, very fast LIFO (stack)

If you need a FIFO (queue):
-	Do you really NEED a queue?
	Reworking your algo to use a stack (LIFO) will be faster.
-	Can your queue have a maximum bound?
	If so a circular buffer is your best bet. Check out 'cbuf'.
-	Otherwise you have a choice between hash tables (e.g. Judy)
		or (gasp!) a linked list.

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
		#define STACK_MMAP_FLAGS (MAP_PRIVATE | MAP_ANONYMOUS | MAP_UNINITIALIZED)
	#else
		#define STACK_MMAP_FLAGS (MAP_PRIVATE | MAP_ANONYMOUS)
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
Otherwise, assume it should be able to hold a pointer or equivalent unsigned int.
*/
#ifndef STACK_MEM_TYPE
	#define STACK_MEM_TYPE uintptr_t
#endif

/*	How much with the stack grow by each time it is resized?
... in Bytes.
*/
#ifndef STACK_GROW
	#ifdef STACK_MMAP_FLAGS
		#define STACK_GROW (4096 * 2048) /* anonymous 2M pages assumed */
	#else
		#define STACK_GROW 1024 /* seems to get slower if this is higher? */
	#endif
#endif



/*	stack_t
*/
struct stack_t {
	size_t		next;		/* aka: 'cnt' ... it's an index */
	size_t		mem_len;	/* in Bytes */
	STACK_MEM_TYPE	mem[];
};



/*
	public
*/
NLC_PUBLIC	struct stack_t	*stack_new();
NLC_PUBLIC	void		stack_free(struct stack_t *stk);
NLC_PUBLIC	struct stack_t	*stack_extend(struct stack_t *stk);



/*	stack_push()
*/
NLC_INLINE	size_t		stack_push(struct stack_t **stk, STACK_MEM_TYPE push_this)
{
	struct stack_t *sk = *stk;
	if (!(sk->mem_len - (sk->next * sizeof(STACK_MEM_TYPE))))
		*stk = sk = stack_extend(sk);

	sk->mem[sk->next] = push_this;
	return sk->next++;
}

/*	stack_pop()
*/
NLC_INLINE	size_t		stack_pop(struct stack_t *stk, STACK_MEM_TYPE *pop_here)
{
	if (stk->next)
		*pop_here = stk->mem[--(stk->next)];
	return stk->next;
}



#endif /* stack_h_ */
