#include <lifo.h>



/*	lifo_new()
*/
struct lifo	*lifo_new()
{
	struct lifo *ret = NULL;

#ifdef LIFO_MMAP_FLAGS
	Z_die_if((
		ret = mmap(NULL, LIFO_GROW, PROT_READ | PROT_WRITE, LIFO_MMAP_FLAGS, -1, 0)
		) == MAP_FAILED, "mmap %d fail", LIFO_GROW);
#else
	Z_die_if(!(ret = malloc(LIFO_GROW)),
		"malloc %d fail", LIFO_GROW);
#endif
	
	ret->next = 0;
	ret->mem_len = LIFO_GROW - sizeof(struct lifo);

out:
	return ret;
}



/*	lifo_free()
*/
void		lifo_free(struct lifo *stk)
{
#ifdef LIFO_MMAP_FLAGS
	if (stk && stk != MAP_FAILED)
		munmap(stk, stk->mem_len + sizeof(stk));
#else
	free(stk);
#endif
}



/*	lifo_extend()
*/
struct lifo	*lifo_extend(struct lifo *stk)
{
	stk->mem_len += LIFO_GROW;

#ifdef LIFO_MMAP_FLAGS
	Z_die_if((
		stk = mremap(stk, stk->mem_len - LIFO_GROW + sizeof(struct lifo),
				stk->mem_len + sizeof(struct lifo),
				MREMAP_MAYMOVE)
		) == MAP_FAILED,
		"mremap to %zu", stk->mem_len + sizeof(struct lifo));
#else
	Z_die_if(!(stk = realloc(stk, stk->mem_len + sizeof(struct lifo))),
		"realloc to %zu", stk->mem_len + sizeof(struct lifo));
#endif

out:
	return stk;
}
