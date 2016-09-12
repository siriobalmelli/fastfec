#include <unistd.h>

#include "ffec.h"

#define SRC_DATA_LEN 64123456	/* 64 MB */

int main()
{
	int err_cnt = 0;
	/* all pointers here */
	void *src=NULL, *src_compare=NULL, *repair=NULL, *scratch=NULL, *repair_dec=NULL;

	/* compute needed sizes */
	const struct ffec_params fp = {	.fec_ratio = 1.1,	/* 10% */
					.sym_len = 1280 };	/* largest for a 1500 MTU */
	size_t src_sz, repair_sz, scratch_sz;
	Z_die_if(
		ffec_calc_lengths(&fp, SRC_DATA_LEN, &src_sz, &repair_sz, &scratch_sz, decode)
		, "");

	/* allocate source memory */
	src = malloc(src_sz);
	src_compare = malloc(src_sz);
	Z_die_if(!src || !src_compare, "malloc()");
	memcpy(src_compare, src, src_sz);
	Z_die_if(memcmp(src, src_compare, src_sz), "something basic is broken");
	/* other memory regions */
	repair = malloc(repair_sz);
	scratch = malloc(scratch_sz);
	Z_die_if(!repair || !scratch, "malloc()");

	/* fec_instance on the stack; initialize it */
	struct ffec_instance fi_enc;
	Z_die_if(
		ffec_init_instance(&fp, &fi_enc, SRC_DATA_LEN, src, repair, scratch, encode, 0)
		, "");
	/* encode */
	Z_die_if(
		ffec_encode(&fp, &fi_enc)
		, "");
	Z_die_if(memcmp(src, src_compare, src_sz), "encoding must not alter the data");

	repair_dec = malloc(repair_sz);
	Z_die_if(!repair_dec, "malloc()");
	/* New instance for decoding.
	Note that 'seed' must be the same between the two.
	*/
	struct ffec_instance fi_dec;
	memset(src, 0xFE, src_sz); /* verify that FEC is overwriting */
	Z_die_if(
		ffec_init_instance(&fp, &fi_dec, SRC_DATA_LEN, src_compare, repair_dec, scratch, decode, fi_enc.seed)
		, "");

	/* run decode */
	unsigned int i=0;
	long rand;
	struct drand48_data rand_d;
	srand48_r(1234, &rand_d);
	do {
		lrand48_r(&rand_d, &rand);	
		rand %= fi_dec.cnt.cols;
		i++;
	} while  (
		ffec_decode_sym(&fp, &fi_dec, 
				ffec_get_sym(&fp, &fi_enc, rand),
				rand)
	);

	Z_die_if(memcmp(src, src_compare, src_sz), "");
	Z_inf(0, "%d iterations on random decode, from %d source symbols",
		i, fi_enc.cnt.cols);

out:
	if (src)
		free(src);
	if (src_compare)
		free(src_compare);
	if (repair)
		free(repair);
	if (scratch)
		free(scratch);
	if (repair_dec)
		free(repair_dec);

	return err_cnt;
}
