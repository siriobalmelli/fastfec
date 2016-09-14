#include <unistd.h>

/* open() */
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>

#include "ffec.h"

#define SRC_DATA_LEN 64123456	/* 64 MB */


void unused()
{
	/* all pointers here */
	void *src=NULL, *src_compare=NULL, *repair=NULL, *scratch=NULL, *rcv_all=NULL;

	/* compute needed sizes */
	const struct ffec_params fp = {	.fec_ratio = 1.1,	/* 10% */
					.sym_len = 1280 };	/* largest for a 1500 MTU */
	struct ffec_sizes sizes;
	Z_die_if(
		ffec_calc_lengths(&fp, SRC_DATA_LEN, &sizes, decode)
		, "");

	/* allocate source memory and init */
	src = malloc(sizes.source_sz);
	src_compare = malloc(sizes.source_sz);
	Z_die_if(!src || !src_compare, "malloc()");
	int fd = open("/dev/urandom", O_RDONLY);
	size_t init = 0;
	while (init < sizes.source_sz)
		init += read(fd, src + init, sizes.source_sz - init);
	memcpy(src_compare, src, sizes.source_sz);
	Z_die_if(memcmp(src, src_compare, sizes.source_sz), "something basic is broken");

	/* other memory regions */
	repair = malloc(sizes.parity_sz);
	scratch = malloc(sizes.scratch_sz);
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
	Z_die_if(memcmp(src, src_compare, sizes.source_sz), "encoding must not alter the data");

	/* initialize decoder as a contiguous region */
	rcv_all = malloc(sizes.source_sz + sizes.parity_sz + sizes.scratch_sz);
	Z_die_if(!rcv_all, "malloc()");
	memset(rcv_all, 0xFE, sizes.source_sz); /* should have no dependency on existing memory contents */
	/* New instance for decoding.
	Note that 'seed' must be the same between the two.
	*/
	struct ffec_instance fi_dec;
	Z_die_if(
		ffec_init_instance_contiguous(&fp, &fi_dec, SRC_DATA_LEN, rcv_all, decode, fi_enc.seed)
		, "");

	/* run decode */
#if 0
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
#else
	unsigned int i;
	for (i=0; i < fi_enc.cnt.cols; i++) {
		if (!ffec_decode_sym(&fp, &fi_dec, 
				ffec_get_sym(&fp, &fi_enc, i),
				i))
			break;
	}
	Z_die_if(fi_dec.cnt.k_decoded != fi_dec.cnt.k, 
		"k_decoded %d != k %d", 
		fi_dec.cnt.k_decoded, fi_dec.cnt.k);
#endif

	Z_die_if(memcmp(src, rcv_all, SRC_DATA_LEN), "");
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
	if (rcv_all)
		free(rcv_all);

}

int main()
{
	int err_cnt = 0;

	struct ffec_params fp = {
		.fec_ratio = 1.1, /* 10% FEC */
		.sym_len = 1280
	};

	const unsigned int n=50000;

	/* 10 source symbols should equal 4 repair symbols */
	struct ffec_sizes fs;
	ffec_calc_lengths(&fp, fp.sym_len * n, &fs, decode);

	void *mem = malloc(fs.source_sz + fs.parity_sz + fs.scratch_sz);
	int fd = open("/dev/urandom", O_RDONLY);
	size_t init = 0;
	while (init < fs.source_sz)
		init += read(fd, fs.source_sz + (void*)init, fs.source_sz - init);

	clock_t clock_enc = clock();
	struct ffec_instance fi;
	ffec_init_instance_contiguous(&fp, &fi, fp.sym_len * n, mem, encode, 0xaf);
	ffec_encode(&fp, &fi);
	clock_enc = clock() - clock_enc;
	Z_inf(0, "encode ELAPSED: %lfs", (double)clock_enc / CLOCKS_PER_SEC * 1000);

	void *mem_dec = malloc(fs.source_sz + fs.parity_sz + fs.scratch_sz);
	clock_t clock_dec = clock();
	struct ffec_instance fi_dec;
	ffec_init_instance_contiguous(&fp, &fi_dec, fp.sym_len * n, mem_dec, decode, 0xaf);

	/* make linear collection of symbols to be decoded */
	uint32_t *next_esi = alloca(fi_dec.cnt.cols * sizeof(uint32_t));
	unsigned int i;
	for (i=0; i < fi_dec.cnt.cols; i++)
		next_esi[i] = i;
	/* seed a random number generator */
	long rand;
	Z_die_if( read(fd, &rand, sizeof(rand)) != sizeof(rand), "");
	struct drand48_data rand_d;
	srand48_r(rand, &rand_d);
	/* swap random collection of ESIs */
	uint32_t *a, *b;
	for (i=0; i < fi_dec.cnt.cols * 2; i++) {
		lrand48_r(&rand_d, &rand);
		a = &next_esi[rand % fi_dec.cnt.cols];
		lrand48_r(&rand_d, &rand);
		b = &next_esi[rand % fi_dec.cnt.cols];
		rand = a[0];
		a[0] = b[0];
		b[0] = rand;
	}
	/* iterate through randomly ordered ESIs and decode for each */
	for (i=0; i < fi_dec.cnt.cols; i++) {
		if (!ffec_decode_sym(&fp, &fi_dec, 
				ffec_get_sym(&fp, &fi, next_esi[i]),
				next_esi[i]))
			break;
	}
	clock_dec = clock() - clock_dec;
	Z_inf(0, "decode ELAPSED: %lfms", (double)clock_dec / CLOCKS_PER_SEC * 1000);
	Z_inf(0, "decoded with i=%d, n=%d; inefficiency=%lf; loss tolerance=%lf%%",
		i, fi_dec.cnt.n, 
		(double)i / (double)fi_dec.cnt.k,
		(double)(fi_dec.cnt.n - i) / (double)fi_dec.cnt.n * 100);
	Z_inf(0, "source size=%lfMB, bitrates: enc=%lfMbps, dec=%lfMbps",
		(double)fs.source_sz / (1024 * 1024),
		(double)fs.source_sz / ((double)clock_enc / CLOCKS_PER_SEC) / (1024 * 1024) * 8,
		(double)fs.source_sz / ((double)clock_dec / CLOCKS_PER_SEC) / (1024 * 1024) * 8);

	Z_die_if(memcmp(mem, mem_dec, fs.source_sz), "sum ting willy wong");
out:
	if (mem)
		free(mem);
	if (mem_dec)
		free(mem_dec);
	return err_cnt;
}
