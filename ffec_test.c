#include <unistd.h>

/* open() */
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <unistd.h>
#include <stdlib.h> /* atof */

#include "ffec.h"

#define OWN_RAND

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
	size_t original_sz = 0;

	int opt;
	extern char* optarg; /* used by getopt to point to arg values given */
	while ((opt = getopt(argc, argv, "f:o:")) != -1) {
		switch (opt) {
			case 'f':
				fec_ratio = atof(optarg);
				printf("fec_ratio: %f\r\n", fec_ratio);
				break;

			case 'o':
				original_sz = atol(optarg);
				printf("original_sz: %ld\r\n", original_sz);
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
#ifdef OWN_RAND
	//init our own PRNG lib here with urandom as a seed,
	//pass its results into temp.
	size_t init = 0, temp;
	uint64_t seeds[2];
	struct pcg_rand_state rnd_state;
	Z_die_if((
		read(fd, seeds, sizeof(seeds))
		) != sizeof(seeds), "");
	Z_inf(0, "Seed1: 0x%lx", seeds[0]);
	Z_inf(0, "Seed2: 0x%lx", seeds[1]);
	//seed random number generator from dev/urandom seeds
	pcg_rand_seed(&rnd_state, seeds[0], seeds[1]);
#endif

	while (init < fs.source_sz) {
#ifdef OWN_RAND
		((uint8_t*)mem)[init] = pcg_rand(&rnd_state);
		init += sizeof(uint32_t);
#else
		Z_die_if((
		          temp = read(fd, mem + init, fs.source_sz - init)
			) < 0, "");
                init += temp;
#endif
	}
	close(fd);

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
#ifdef FFEC_DEBUG
		/* force random number sequence for debugging purposes */
		ffec_init_instance_contiguous(&fp, &fi, original_sz, mem,
						encode, PCG_RAND_S1, PCG_RAND_S2)
#else
		ffec_init_instance_contiguous(&fp, &fi, original_sz, mem,
						encode, 0, 0)
#endif
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
	struct pcg_rand_state rnd;
	pcg_rand_seed_static(&rnd);
	/* swap random collection of ESIs */
	uint32_t rand;
	for (i=0; i < fi.cnt.cols -1; i++) {
		temp = next_esi[i];
		rand = next_esi[pcg_rand_bound(&rnd, fi.cnt.cols -i) + i];
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
#ifdef FFEC_DEBUG
	/* compare matrix cells and rows - which should be identical
		(because their IDs are offsets, not absolute addresses ;)
	*/
	Z_die_if(memcmp(fi.cells, fi_dec.cells, fi.cnt.n * FFEC_N1_DEGREE + fi.cnt.rows), "bad");
	/* print symbol addresses */
	for (i=0; i < fi.cnt.n; i++) {
		Z_inf(0, "src: esi %d @ 0x%lx", i, (uint64_t)ffec_get_sym(&fp, &fi, i));
		Z_inf(0, "dst: esi %d @ 0x%lx", i, (uint64_t)ffec_get_sym(&fp, &fi_dec, i));
	}
	for (i=0; i < fi.cnt.p; i++)
		Z_inf(0, "dst psum row %d @ 0x%lx",
			i, (uint64_t)ffec_get_psum(&fp, &fi_dec, i));
#endif
	/* iterate through randomly ordered ESIs and decode for each */
	for (i=0; i < fi_dec.cnt.cols; i++) {
#ifdef FFEC_DEBUG
		Z_inf(0, "submit ESI %d", next_esi[i]);
#endif
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
#ifdef FFEC_DEBUG
	for (i=0; i < fi.cnt.k; i++)
		Z_warn_if(memcmp(ffec_get_sym(&fp, &fi, i), ffec_get_sym(&fp, &fi_dec, i), fp.sym_len),
			"symbol %d differs", i);
#endif

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
