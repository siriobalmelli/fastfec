/*	nlc_urand_test.c

Validate use of nonlibc_urand.
Show use of nlc_urand() to initialize an RNG for the purposes of writing
	many random bytes to memory.
*/

#include <zed_dbg.h>
#include <nlc_urand.h>
#include <stdlib.h> /* malloc() */



/*	test_IV()

Test using nlc_urand() to get an Initialization Vector.
The desired property is it just works and DOESN'T BLOCK; thank you very much.
*/
int test_IV()
{
	int err_cnt = 0;

	const int test_iter = 100000;

	/* reasonably the LARGEST Initialization Vector I can imagine needing */
	uint64_t words[32];

	for (int i=0; i < test_iter; i++) {
		Z_die_if(nlc_urand(words, sizeof(words)) != sizeof(words),
			"we really expect this not to have blocked");
	}

out:
	return err_cnt;
}



#include <pcg_rand.h>

/*	test_big_random()

Show use of nlc_urand() to initialize an RNG (pcg in this case; from this same library),
	then generating many many bytes.
*/
int test_big_random()
{
	int err_cnt = 0;
	const size_t size = 20000000; /* 20MB */

	void *mem_a = NULL, *mem_b = NULL;
	Z_die_if(!( mem_a = calloc(1, size) ), "size %zu", size);
	Z_die_if(!( mem_b = calloc(1, size) ), "size %zu", size);

	uint64_t seeds[2];

	/* get seeds and generate random memory */	
	Z_die_if(nlc_urand(seeds, sizeof(seeds)) != sizeof(seeds), "");
	pcg_randset(mem_a, size, seeds[0], seeds[1]);
	Z_die_if(nlc_urand(seeds, sizeof(seeds)) != sizeof(seeds), "");
	pcg_randset(mem_b, size, seeds[0], seeds[1]);

	Z_die_if(!memcmp(mem_a, mem_b, size), "these should NEVER be identical");

out:
	free(mem_a);
	free(mem_b);
	return err_cnt;	
}



/*	main()
*/
int main()
{
	int err_cnt = 0;

	err_cnt += test_IV();
	err_cnt += test_big_random();

	return err_cnt;
}
