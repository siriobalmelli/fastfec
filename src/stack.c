#include <stack.h>



/*	stack_new()
*/
struct stack_t	*stack_new()
{
	struct stack_t *ret = NULL;

#ifdef STACK_MMAP_FLAGS
	Z_die_if((
		ret = mmap(NULL, STACK_GROW, PROT_READ | PROT_WRITE, STACK_MMAP_FLAGS, -1, 0)
		) == MAP_FAILED, "mmap %d fail", STACK_GROW);
#else
	Z_die_if(!(ret = malloc(STACK_GROW)),
		"malloc %d fail", STACK_GROW);
#endif
	
	ret->next = 0;
	ret->mem_len = STACK_GROW - sizeof(struct stack_t);

out:
	return ret;
}



/*	stack_free()
*/
void		stack_free(struct stack_t *stk)
{
#ifdef STACK_MMAP_FLAGS
	if (stk && stk != MAP_FAILED)
		munmap(stk, stk->mem_len + sizeof(stk));
#else
	free(stk);
#endif
}



/*	stack_extend()
*/
struct stack_t	*stack_extend(struct stack_t *stk)
{
	stk->mem_len += STACK_GROW;

#ifdef STACK_MMAP_FLAGS
	Z_die_if((
		stk = mremap(stk, stk->mem_len - STACK_GROW + sizeof(struct stack_t),
				stk->mem_len + sizeof(struct stack_t),
				MREMAP_MAYMOVE)
		) == MAP_FAILED,
		"mremap to %zu", stk->mem_len + sizeof(struct stack_t));
#else
	Z_die_if(!(stk = realloc(stk, stk->mem_len + sizeof(struct stack_t))),
		"realloc to %zu", stk->mem_len + sizeof(struct stack_t));
#endif

out:
	return stk;
}
