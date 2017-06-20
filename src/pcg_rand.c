#include <pcg_rand.h>


/*	pcg_rand()
Returns the next value in the sequence which state is stored in 'rng'
	(updating 'rng' in the process).

NOTE that 'rng' MUST be initialized before calling this function.
*/
uint32_t	pcg_rand(struct pcg_state *rng)
{
	uint64_t oldstate = rng->state;
	/* Advance internal state */
	rng->state = oldstate * 6364136223846793005ULL + rng->inc;
	/* Calculate output function (XSH RR), uses old state for max ILP */
	uint32_t xorshifted = ((oldstate >> 18u) ^ oldstate) >> 27u;
	uint32_t rot = oldstate >> 59u;
	return (xorshifted >> rot) | (xorshifted << ((-rot) & 31));
}


/*	pcg_randset()
Fill an area of memory with random bytes.
*/
void		pcg_randset(void *mem, size_t len, uint64_t seed1, uint64_t seed2)
{
	/* setup rng */
	struct pcg_state rnd_state;
	pcg_seed(&rnd_state, seed1, seed2);
	/* write 32-bit words */
	uint32_t *word = mem;
	mem += (len / sizeof(uint32_t) * sizeof(uint32_t));
	while ((void *)word < mem)
		*(word++) = pcg_rand(&rnd_state);
	/* trailing bytes */
	uint8_t *byte = (uint8_t *)word;
	mem += (len % sizeof(uint32_t));
	while ((void *)byte < mem)
		*(byte++) = pcg_rand(&rnd_state);
}
