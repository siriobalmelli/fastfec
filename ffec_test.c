#include <unistd.h>

/* open() */
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <unistd.h>
#include <stdlib.h> /* atof */

//#define DEBUG
#include "ffec.h"

/* to verify integrity*/
#include "fmd5.h"

void print_usage()
{
	printf("ffec_test expects 2 arguments:\r\n" 
		"- fec_ratio (-f) - a ratio expressed in type double\r\n"
		"- original_sz (-o) - a size in Mb expressed as unsigned int\r\n"
		"- example: ffec_test.exe -f 1.1 -o 503927\r\n");
}

int main(int argc, char **argv)
{
	int err_cnt = 0;

	/* We expect 2 args, fec_ratio and original_sz 
		fec_ratio will be passed with the -f flag
		original_sz will be passed with the -o flag
	*/	
	double fec_ratio = 0;	
	unsigned int original_sz = 0;

	int opt;
	extern char* optarg; /* used by getopt to point to arg values given */
	while ((opt = getopt(argc, argv, "f:o:")) != -1) {
		switch (opt) {
			case 'f':
				fec_ratio = atof(optarg);
				printf("fec_ratio: %f\r\n", fec_ratio);
				break;

			case 'o': 
				original_sz = atoi(optarg);
				printf("original_sz: %d\r\n", original_sz);	
				break;

			default: 
				print_usage();
				return 0; 
		}
	}
	/* didn't get args? */
	if (!fec_ratio || !original_sz) {
		print_usage();
		return 0;
	}

	void *mem = NULL, *mem_dec = NULL;
	uint32_t *next_esi = NULL;
	/*
		parameters, lengths
	*/
	struct ffec_params fp = {
		.fec_ratio = fec_ratio, /* 10% FEC */
		.sym_len = 1280 /* aka: packet size */
	};
	struct ffec_sizes fs;
	Z_die_if(
		ffec_calc_lengths(&fp, original_sz, &fs, decode)
		, "");

	/*
		set up source memory region
	*/
	Z_die_if(!(
		mem = calloc(1, fs.source_sz + fs.parity_sz + fs.scratch_sz)
		), "");
	int fd;
	/* fill with random data */	
	Z_die_if((
		fd = open("/dev/urandom", O_RDONLY)
		) < 1, "");
	size_t init = 0, temp;
	while (init < fs.source_sz) {
		Z_die_if((
			temp = read(fd, fs.source_sz + (void*)init, fs.source_sz - init)
			) < 0, "");
		init += temp;
	}
	/* get a hash of the source */
	uint8_t src_hash[16], hash_check[16];
	struct iovec hash_iov = { .iov_len = fs.source_sz, .iov_base = mem };
	Z_die_if(
		fmd5_hash(&hash_iov, src_hash)
		, "");

	/*
		istantiate FEC, encode
	*/
	clock_t clock_enc = clock();
	struct ffec_instance fi;
	Z_die_if(
		ffec_init_instance_contiguous(&fp, &fi, original_sz, mem, 
						encode, 0, 0)
		, "");
	ffec_encode(&fp, &fi);
	clock_enc = clock() - clock_enc;
	Z_inf(0, "encode ELAPSED: %.2lfms", (double)clock_enc / CLOCKS_PER_SEC * 1000);
	/* invariant: encode must NOT alter the source region */
	Z_die_if(
		fmd5_hash(&hash_iov, hash_check)
		, "");
	Z_die_if(
		memcmp(src_hash, hash_check, 16)
		, "");

	/*
		malloc destination region.
		make random array of symbols to be decoded.
	*/
	Z_die_if(!(
		mem_dec = calloc(1, fs.source_sz + fs.parity_sz + fs.scratch_sz)
		), "");
	/* make linear collection of symbols to be decoded */
	next_esi = calloc(1, fi.cnt.cols * sizeof(uint32_t));
	unsigned int i;
	for (i=0; i < fi.cnt.cols; i++)
		next_esi[i] = i;
	/* seed a random number generator */
	struct ffec_rand_state rnd;
	ffec_rand_seed_static(&rnd);
	/* swap random collection of ESIs */
	uint32_t rand;
	for (i=0; i < fi.cnt.cols -1; i++) {
		temp = next_esi[i];
		rand = next_esi[ffec_rand_bound(&rnd, fi.cnt.cols -i) + i];
		next_esi[i] = next_esi[rand];
		next_esi[rand] = temp;
	}

	/*
		decode
	*/
	clock_t clock_dec = clock();
	struct ffec_instance fi_dec;
	Z_die_if(
		ffec_init_instance_contiguous(&fp, &fi_dec, original_sz, mem_dec, 
				decode, fi.seeds[0], fi.seeds[1])
		, "");
#ifdef DEBUG
	/* compare matrix rows */
	for (i=0; i < fi.cnt.rows; i++) {
		if (ffec_matrix_row_cmp(&fi.rows[i], &fi_dec.rows[i])) {
			Z_wrn("row %d differs", i);
			ffec_matrix_row_prn(&fi.rows[i]);
			ffec_matrix_row_prn(&fi_dec.rows[i]);
			printf("\n");
		}
	}
	/* compare matrix columns */
	for (i=0; i < fi.cnt.n * FFEC_N1_DEGREE; i++) {
		if (ffec_matrix_cell_cmp(&fi.cells[i], &fi_dec.cells[i]))
			Z_wrn("cell %d differs", i);
	}
#endif
	/* iterate through randomly ordered ESIs and decode for each */
	for (i=0; i < fi_dec.cnt.cols; i++) {
		/* stop decoding when decoder reports 0 symbols left to decode */
		if (!ffec_decode_sym(&fp, &fi_dec, 
				ffec_get_sym(&fp, &fi, next_esi[i]),
				next_esi[i]))
			break;
	}
	clock_dec = clock() - clock_dec;

	/*
		verify
	decoded region must be bit-identical to source
	*/
	hash_iov.iov_base = mem_dec;
	Z_die_if(
		fmd5_hash(&hash_iov, hash_check)
		, "");
	Z_die_if(
		memcmp(src_hash, hash_check, 16)
		, "");

	/*
		report
	*/
	Z_inf(0, "decode ELAPSED: %.2lfms", (double)clock_dec / CLOCKS_PER_SEC * 1000);
	Z_inf(0, "decoded with k=%d < i=%d < n=%d;\n\
\tinefficiency=%lf; loss tolerance=%.2lf%%\n\
\tsource size=%.4lf MB, bitrates: enc=%ldMb/s, dec=%ldMb/s",
		fi_dec.cnt.k, i, fi_dec.cnt.n, 
		(double)i / (double)fi_dec.cnt.k,
		((double)(fi_dec.cnt.n - i) / (double)fi_dec.cnt.n) * 100,
		(double)fs.source_sz / (1024 * 1024),
		(uint64_t)((double)fs.source_sz / ((double)clock_enc / CLOCKS_PER_SEC) 
			/ (1024 * 1024) * 8),
		(uint64_t)((double)fs.source_sz / ((double)clock_dec / CLOCKS_PER_SEC) 
			/ (1024 * 1024) * 8));

out:
	if (mem)
		free(mem);
	if (mem_dec)
		free(mem_dec);
	if (next_esi)
		free(next_esi);
	return err_cnt;
}
