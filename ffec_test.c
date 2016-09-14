#include <unistd.h>

/* open() */
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>

#include "ffec.h"

/* TODO: arguments to set 'fec_ratio' and 'original_sz' */
/* TODO: a script that runs several iterations of this program, each combination of:
	fec_ratio 1.02 -> 1.2 in 2% increments
	original_sz 500k, 1M, 2M, 5M, 10M, 40M, 80M, 100M, 150M, 200M, 500M
	... and 10x for each combination, with an average.
Data to be dumped to CSV so that we can analyze with OpenOffice.
*/

/* BASH:
for i in $(seq 1 6); do 
	dosomething $i & >something_$i.txt
done
wait
cat something_*.txt >result.txt
*/

int main()
{
	int err_cnt = 0;

	struct ffec_params fp = {
		.fec_ratio = 1.1, /* 10% FEC */
		.sym_len = 1280 /* aka: packet size */
	};
	unsigned int original_sz = 503927; /* is prime, to test things */

	/* 10 source symbols should equal 4 repair symbols */
	struct ffec_sizes fs;
	ffec_calc_lengths(&fp, original_sz, &fs, decode);

	void *mem = malloc(fs.source_sz + fs.parity_sz + fs.scratch_sz);
	int fd = open("/dev/urandom", O_RDONLY);
	size_t init = 0;
	while (init < fs.source_sz)
		init += read(fd, fs.source_sz + (void*)init, fs.source_sz - init);

	clock_t clock_enc = clock();
	struct ffec_instance fi;
	ffec_init_instance_contiguous(&fp, &fi, original_sz, mem, encode, 0xaf);
	ffec_encode(&fp, &fi);
	clock_enc = clock() - clock_enc;
	Z_inf(0, "encode ELAPSED: %lfs", (double)clock_enc / CLOCKS_PER_SEC * 1000);

	void *mem_dec = malloc(fs.source_sz + fs.parity_sz + fs.scratch_sz);
	clock_t clock_dec = clock();
	struct ffec_instance fi_dec;
	ffec_init_instance_contiguous(&fp, &fi_dec, original_sz, mem_dec, decode, 0xaf);

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
		((double)(fi_dec.cnt.n - i) / (double)fi_dec.cnt.n) * 100);
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
