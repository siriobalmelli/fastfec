/*	ffec_test.c

Test performance and correctness of ffec

(c) 2016 Sirio Balmelli
*/

#include <ffec.h>

#include <time.h> /* clock() */
#include <nonlibc.h>

#include <fnv.h>
#include <nlc_urand.h>

#include <stdlib.h> /* atof() */


/*	defaults:
5MB region into 1280B symbols @ 10% FEC
*/
double fec_ratio = 1.1;
size_t original_sz = 5000960;
size_t sym_len = 1280;


/*	random_bytes()
Fills a region with random bytes.
Faster (and less secure) than reading them all from /dev/urandom.
*/
void random_bytes(void *region, size_t size)
{
	/* get seeds from /dev/urandom */
	uint64_t seeds[2] = { 0 };

	while (nlc_urand(seeds, sizeof(seeds)) != sizeof(seeds))
		usleep(10000);
	pcg_randset(region, size, seeds[0], seeds[1]);
}



/*	print_usage()
*/
void print_usage(char *pgm_name)
{
	fprintf(stderr,
"usage:\n\
%s	[-f <fec_ratio>] [-o <original_sz>] [-s <sym_len>] [-h]\n\
\n\
fec_ratio	:	a fractional ratio >1.0 && <2.0\n\
		default: 1.1\n\
original_sz	:	data size in B\n\
		default: 5000960 (5MB)\n\
sym_len		:	size of FEC symbols, in B. Must be a multiple of 256\n\
		default: 1280\n",
		pgm_name);
}

/*	parse_opts()
*/
void parse_opts(int argc, char **argv)
{
	int opt;
	extern char* optarg; /* used by getopt to point to arg values given */
	while ((opt = getopt(argc, argv, "f:o:s:h")) != -1) {
		switch (opt) {
			case 'f':
				fec_ratio = atof(optarg);
				break;
			case 'o':
				original_sz = atol(optarg);
				break;
			case 's':
				sym_len = atol(optarg);
				break;
			default:
				print_usage(argv[0]);
				exit(1);
		}
	}

	NB_inf("sym_len: %zu", sym_len);
	NB_inf("fec_ratio: %f", fec_ratio);
	NB_inf("original_sz: %zu", original_sz);
	NB_inf("N: %d", FFEC_N1_DEGREE);
	NB_inf("FFEC_RAND_PASSES: %d", FFEC_RAND_PASSES);
}



/*	main()
*/
int main(int argc, char **argv)
{
	/*
		parameters
	*/
	parse_opts(argc, argv);
	struct ffec_params fp = {
		.fec_ratio = fec_ratio,	/* 1.1 == 10% FEC */
		.sym_len = sym_len	/* aka: packet size */
	};

	int err_cnt = 0;
	void *mem = NULL;
	struct ffec_instance *fi_enc = NULL, *fi_dec = NULL;


	/*
		set up source memory region
	*/
	NB_die_if(!(
		mem = malloc(original_sz)
		), "");
	random_bytes(mem, original_sz);
	/* get a hash of the source */
	uint64_t src_hash = fnv_hash64(NULL, mem, original_sz);


	/*
		encode
	*/
	nlc_timing_start(clock_enc);
		NB_die_if(!(
			fi_enc = ffec_new(&fp, original_sz, mem, 0, 0)
			), "");
		ffec_encode(&fp, fi_enc);
	nlc_timing_stop(clock_enc);
	NB_inf("encode ELAPSED: %.2lfms", nlc_timing_wall(clock_enc) * 1000);

	/* invariant: encode must NOT alter the source region */
	NB_die_if(src_hash != fnv_hash64(NULL, mem, original_sz), "");


	/*
		decode
	*/

	nlc_timing_start(clock_dec);
		NB_die_if(!(
			fi_dec = ffec_new(&fp, original_sz, NULL,
						fi_enc->seeds[0],
						fi_enc->seeds[1])
			), "");
#ifdef DEBUG
		NB_die_if(ffec_mtx_cmp(fi_enc, fi_dec, &fp), "");
#endif

		/* Iterate through randomly ordered ESIs and decode for each.
		Break when decoder reports 0 symbols left to decode.
		*/
		uint32_t i=0;
		for (; i < fi_dec->cnt.n; i++)
			if (!ffec_decode_sym(&fp, fi_dec, ffec_enc_seq(&fp, fi_enc, i)))
				break;
	nlc_timing_stop(clock_dec);

	/*
		verify
	decoded region must be bit-identical to source
	*/
	NB_die_if(src_hash != fnv_hash64(NULL, fi_dec->dec_source, original_sz), "");


	/*
		report
	*/
	NB_inf("decode ELAPSED: %.2lfms", nlc_timing_wall(clock_dec) * 1000);
	NB_inf("decoded with k=%"PRIu32" < i=%"PRIu32" < n=%"PRIu32";\n\
\tinefficiency=%lf; channel loss tolerance=%.2lf%%; FEC=%.2lf%%\n\
\tsource size=%.4lf MiB, bitrates: enc=%"PRIu64"Mb/s, dec=%"PRIu64"Mb/s",
		/*k*/fi_dec->cnt.k, /*i*/i, /*n*/fi_dec->cnt.n,
		/*inefficiency*/(double)i / (double)fi_dec->cnt.k,
		/*channel loss tolerance*/((double)(fi_dec->cnt.n - i) / (double)fi_dec->cnt.n) * 100,
		/*FEC*/(fp.fec_ratio -1) * 100,
		/*source size*/(double)original_sz / (1024 * 1024),
		/*enc bitrate*/(uint64_t)((double)original_sz
			/ nlc_timing_wall(clock_enc)
			/ (1024 * 1024) * 8),
		/*dec bitrate*/(uint64_t)((double)original_sz
			/ nlc_timing_wall(clock_dec)
			/ (1024 * 1024) * 8));

die:
	free(mem);
	ffec_free(fi_enc);
	ffec_free(fi_dec);
	return err_cnt;
}
