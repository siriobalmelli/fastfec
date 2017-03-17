#ifndef pcg_rand_h_
#define	pcg_rand_h_

/* pcg_rand.c
RNG for the ffec library.

This code is taken directly from the PCG library,
Copyright 2014 Melissa O'Neill <oneill@pcg-random.org>.

The code was obtained under the APACHE 2.0 license, which is
	available here:
https://www.apache.org/licenses/LICENSE-2.0.txt

It has been stripped down for simplicity, and turned into
	a pure-inline library for performance reasons.
There are no global, thread-unsafe RNG facilities provided.

In case this software is redistributed, it is done so under APACHE 2.0 and
	is "AS-IS", with no warranties whatever.
Seriously, it might be malicious code. Use it at your own risk.
If you can't read and understand it, just don't use it ;)

Sirio Balmelli, 2016
*/

#include <stdint.h> /* uint{x}_t */
#include "zed_dbg.h"

/* Internals are opaque (private). Don't access directly. */
struct pcg_rand_state {
	uint64_t state;		/* RNG state. All values are possible. */
	uint64_t inc;		/* Controls which RNG sequence (stream) is
					selected.
				Must *always* be odd.
				*/
}__attribute__ ((packed));

Z_INL_FORCE uint32_t	pcg_rand(struct pcg_rand_state *rng)
{
	uint64_t oldstate = rng->state;
	/* Advance internal state */
	rng->state = oldstate * 6364136223846793005ULL + rng->inc;
	/* Calculate output function (XSH RR), uses old state for max ILP */
	uint32_t xorshifted = ((oldstate >> 18u) ^ oldstate) >> 27u;
	uint32_t rot = oldstate >> 59u;
	return (xorshifted >> rot) | (xorshifted << ((-rot) & 31));
}

Z_INL_FORCE uint32_t	pcg_rand_bound(struct pcg_rand_state	*rng,
					uint32_t		bound)
{
	/* [sirio: dark magic goes here]:
	To avoid bias, we need to make the range of the RNG a multiple of
		bound, which we do by dropping output less than a threshold.
	A naive scheme to calculate the threshold would be to do
			`uint32_t threshold = 0x100000000ull % bound;`
		but 64-bit div/mod is slower than 32-bit div/mod
		(especially on 32-bit platforms).
	In essence, we do
			`uint32_t threshold = (0x100000000ull-bound) % bound;`
	because this version will calculate the same modulus,
		but the LHS value is less than 2^32.
	*/
	uint32_t threshold = -bound % bound;
	/* Uniformity guarantees that this loop will terminate.
	In practice, it should usually terminate quickly; on average
		(assuming all bounds are equally likely), 82.25% of the time,
		we can expect it to require just one iteration.
	In the worst case, someone passes a bound of 2^31 + 1 (i.e., 2147483649),
		which invalidates almost 50% of the range.
	In practice, bounds are typically small and only a tiny amount
		of the range is eliminated.
	*/
	uint32_t r;
	do {
		r = pcg_rand(rng);
	} while (r < threshold);
	return r % bound;
}

/*	pcg_rand_seed()
Seed a random number generator.
In actual fact, 'seed1' is the "init state" and 'seed2' the "init sequence",
	but to highlight usage, they are just "seeds".
For reproducible sequences, use the same pair of seeds across initializations.
*/
Z_INL_FORCE void	pcg_rand_seed(	struct pcg_rand_state	*rng,
					uint64_t		seed1,
					uint64_t		seed2)
{
	rng->state = 0u;
	rng->inc = (seed2 << 1) | 1u; /* must be odd */
	pcg_rand(rng);
	rng->state += seed1;
	pcg_rand(rng);
}

/* make static seeds available to callers if desired */
#define PCG_RAND_S1 0x853c49e6748fea9bULL
#define PCG_RAND_S2 0xda3e39cb94b95bdbULL

/*	pcg_rand_seed_static()
Seeds an rng with the recommended static constants.
*/
Z_INL_FORCE void	pcg_rand_seed_static(struct pcg_rand_state *rng)
{
	pcg_rand_seed(rng, PCG_RAND_S1, PCG_RAND_S2);
}

/*	pcg_randset()
Fill an area of memory with random bytes.
*/
Z_INL_FORCE void	pcg_randset(void *mem, size_t len, uint64_t seed1, uint64_t seed2)
{
	/* setup rng */
	struct pcg_rand_state rnd_state;
	pcg_rand_seed(&rnd_state, seed1, seed2);
	/* write data */
	uint32_t *word = mem;
	for (size_t i=0; i < len / sizeof(uint32_t); i++)
		*(word++) = pcg_rand(&rnd_state);
	/* trailing bytes */
	uint8_t *byte = (uint8_t *)word;
	for (int i=0; i < len % sizeof(uint32_t); i++)
		byte[i] = pcg_rand(&rnd_state);
}

#endif /* pcg_rand_h_ */
