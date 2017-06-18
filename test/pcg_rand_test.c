/*	pcg_rand_test.c

Test series for the pcg_rand set of functions.
This is a faster, better (more bits of randomity), easier-to-use (ok, admittedly that's subjective)
	alternative to rand() and company.

Melissa O'Neill developed PCG, I merely adapted it for the purposes of this library.
For all the 411, see 'pcg_rand.h'

(c)2017 Sirio Balmelli; https://b-ad.ch
*/

#include <pcg_rand.h>
#include <zed_dbg.h>


/*	rand()
Basic usage of pcg
*/
int rand()
{
	int err_cnt = 0;

	/* seed my RNG with the 'default' or 'static' seed */
	struct pcg_state pcg1;
	pcg_seed_static(&pcg1);
	uint32_t res1 = pcg_rand(&pcg1);

	/* reproducible results (using default constants) */
	struct pcg_state pcg2;
	pcg_seed(&pcg2, PCG_RAND_S1, PCG_RAND_S2);
	uint32_t res2 = pcg_rand(&pcg2);

	Z_err_if(res1 != res2, "rng not reproducible: %d != %d", res1, res2);

	return err_cnt;
}


/*	bounded_rand()

Test uniformity of randomly generated values with a certain bound:
	certain numbers shouldn't be more statistically likely to appear.
For more info on this, see <https://blog.codinghorror.com/the-danger-of-naivete/>

returns 0 on success
*/
int bounded_rand()
{
	int err_cnt = 0;
	const unsigned int bound = 7;		/* random numbers will be 0:(bound-1) */
	const unsigned int numiter = 4666797 * bound;	/* how many random nums to generate */

	/* seed my RNG with the 'default' or 'static' seed */
	struct pcg_state pcg;
	pcg_seed_static(&pcg);

	/* generate random numbers, log frequency of appearance */
	int hits[bound];
	memset(hits, 0x0, sizeof(hits));
	for (unsigned int i=0; i < numiter; i++)
		hits[pcg_rand_bound(&pcg, bound)]++;

	unsigned int lowest = -1, highest = 0;

	/* check each 'slot', verify there was an even frequency of appearance */
	for (unsigned int i=0; i < bound; i++) {
		if (hits[i] < lowest)
			lowest = hits[i];
		if (hits[i] > highest)
			highest = hits[i];
	}

	/* report findings */
	unsigned int expected = numiter / bound;
	double lowest_deviation = (expected - lowest) / (double)expected;
	double highest_deviation = (highest - expected) / (double)expected;

	/* TODO: this threshold is entirely subjective.
	Need some input/guidance on what's acceptable.
	*/
	Z_err_if(lowest_deviation > 0.001 || highest_deviation > 0.001, "bad algorithm");
	/* print info regardless */
	Z_inf(0, "%d rng calls bounded at %d: -%f <%d> +%f",
		numiter, bound, lowest_deviation, expected, highest_deviation);

	return err_cnt;
}


/*	main()

returns 0 on success
*/
int main()
{
	int err_cnt = 0;

	err_cnt += rand();
	err_cnt += bounded_rand();

	return err_cnt;
}
