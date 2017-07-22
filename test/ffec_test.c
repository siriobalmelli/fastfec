/*	ffec_test.c

Test performance and correctness of ffec

(c) 2016 Sirio Balmelli
*/

/* override suppression of Z_inf with -DNDEBUG */
#include <zed_dbg.h>
#undef Z_LOG_LVL
#define Z_LOG_LVL (Z_err | Z_wrn | Z_inf)

#include <ffec.h>

#include <time.h> /* clock() */
#include <openssl/md5.h>
#include <nlc_urand.h>

#include <stdlib.h> /* atof() */

/*	who does symbol copying?
if set: copy symbol into matrix ourselves BEFORE calling ffec_decode_sym()
*/
//#define FFEC_TEST_COPY_SYM


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

	Z_log(Z_inf, "sym_len: %zu", sym_len);
	Z_log(Z_inf, "fec_ratio: %f", fec_ratio);
	Z_log(Z_inf, "original_sz: %zu", original_sz);
	Z_log(Z_inf, "N: %d", FFEC_N1_DEGREE);
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
		.fec_ratio = fec_ratio,	/* 10% FEC */
		.sym_len = sym_len	/* aka: packet size */
	};

	int err_cnt = 0;
	void *mem = NULL;
	uint32_t *next_esi = NULL;
	struct ffec_instance *fi_encode = NULL, *fi_decode = NULL;


	/*
		set up source memory region
	*/
	Z_die_if(!(
		mem = malloc(original_sz)
		), "");
	random_bytes(mem, original_sz);
	/* get a hash of the source */
	uint8_t src_hash[16], hash_check[16];
	MD5(mem, original_sz, src_hash);


	/*
		encode
	*/
	clock_t clock_enc = clock();
	Z_die_if(!(
		fi_encode = ffec_new(&fp, original_sz, mem, 0, 0)
		), "");
	ffec_encode(&fp, fi_encode);
	clock_enc = clock() - clock_enc;
	Z_log(Z_inf, "encode ELAPSED: %.2lfms", (double)clock_enc / CLOCKS_PER_SEC * 1000);

	/* invariant: encode must NOT alter the source region */
	MD5(mem, original_sz, hash_check);
	Z_die_if(memcmp(src_hash, hash_check, 16)
		, "");


	/*
		decode
	*/
	/* create random order into which to "send"(decode) symbols */
	next_esi = malloc(fi_encode->cnt.n * sizeof(uint32_t));
	ffec_esi_rand(fi_encode, next_esi, 0);

	clock_t clock_dec = clock();
	Z_die_if(!(
		fi_decode = ffec_new(&fp, original_sz, NULL,
					fi_encode->seeds[0],
					fi_encode->seeds[1])
		), "");
#ifdef DEBUG
      Z_die_if(ffec_mtx_cmp(fi_encode, fi_decode, &fp), "");
#endif

	/* Iterate through randomly ordered ESIs and decode for each.
	Break when decoder reports 0 symbols left to decode.
	*/
	uint32_t i;
	for (i=0; i < fi_decode->cnt.cols; i++) {
		Z_log(Z_in2, "submit ESI %d", next_esi[i]);

#ifdef FFEC_TEST_COPY_SYM
		/* copy symbol into matrix manually, give only ESI */
		memcpy(ffec_dec_sym(&fp, fi_decode, next_esi[i]),
				ffec_enc_sym(&fp, fi_encode, next_esi[i]), 
				fp.sym_len);
		if (!ffec_decode_sym(&fp, fi_decode,
				NULL,
				next_esi[i]))
			break;

#else
		/* the more usual case: give ffec a pointer and have it copy */
		if (!ffec_decode_sym(&fp, fi_decode,
				ffec_enc_sym(&fp, fi_encode, next_esi[i]),
				next_esi[i]))
			break;
#endif
	}
	clock_dec = clock() - clock_dec;


	/*
		verify
	decoded region must be bit-identical to source
	*/
	MD5(fi_decode->dec_source, original_sz, hash_check);
	Z_die_if(
		memcmp(src_hash, hash_check, 16)
		, "");


	/*
		report
	*/
	Z_log(Z_inf, "decode ELAPSED: %.2lfms", (double)clock_dec / CLOCKS_PER_SEC * 1000);
	Z_log(Z_inf, "decoded with k=%"PRIu32" < i=%"PRIu32" < n=%"PRIu32";\n\
\tinefficiency=%lf; channel loss tolerance=%.2lf%%; FEC=%.2lf%%\n\
\tsource size=%.4lf MiB, bitrates: enc=%"PRIu64"Mb/s, dec=%"PRIu64"Mb/s",
		/*k*/fi_decode->cnt.k, /*i*/i, /*n*/fi_decode->cnt.n,
		/*inefficiency*/(double)i / (double)fi_decode->cnt.k,
		/*channel loss tolerance*/((double)(fi_decode->cnt.n - i) / (double)fi_decode->cnt.n) * 100,
		/*FEC*/(fp.fec_ratio -1) * 100,
		/*source size*/(double)original_sz / (1024 * 1024),
		/*enc bitrate*/(uint64_t)((double)original_sz
			/ ((double)clock_enc / CLOCKS_PER_SEC)
			/ (1024 * 1024) * 8),
		/*dec bitrate*/(uint64_t)((double)original_sz
			/ ((double)clock_dec / CLOCKS_PER_SEC)
			/ (1024 * 1024) * 8));

out:
	free(mem);
	free(next_esi);
	ffec_free(fi_encode);
	ffec_free(fi_decode);
	return err_cnt;
}
