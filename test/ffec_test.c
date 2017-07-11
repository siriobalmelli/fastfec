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
	printf(
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
}



/*	main()
*/
int main(int argc, char **argv)
{
	parse_opts(argc, argv);

	int err_cnt = 0;
	void *mem = NULL, *mem_dec = NULL;
	uint32_t *next_esi = NULL;

	/*
		parameters, lengths
	*/
	struct ffec_params fp = {
		.fec_ratio = fec_ratio,	/* 10% FEC */
		.sym_len = sym_len	/* aka: packet size */
	};
	struct ffec_sizes fs;
	Z_die_if(
		ffec_calc_lengths(&fp, original_sz, &fs, decode),
		"fp.fec_ratio %f; fp.sym_len %"PRIu32"; original_sz %zu",
		fp.fec_ratio, fp.sym_len, original_sz);

	/*
		set up source memory region
	*/
	Z_die_if(!(
		mem = calloc(1, fs.source_sz + fs.parity_sz + fs.scratch_sz)
		), "");
	random_bytes(mem, fs.source_sz + fs.parity_sz + fs.scratch_sz);
	/* get a hash of the source */
	uint8_t src_hash[16], hash_check[16];
	MD5(mem, fs.source_sz, src_hash);

	/*
		istantiate FEC, encode
	*/
	clock_t clock_enc = clock();
	struct ffec_instance fi;
	Z_die_if(
		ffec_init_contiguous(&fp, &fi, original_sz, mem,
						encode, 0, 0)
		, "");
	ffec_encode(&fp, &fi);
	clock_enc = clock() - clock_enc;
	Z_log(Z_inf, "encode ELAPSED: %.2lfms", (double)clock_enc / CLOCKS_PER_SEC * 1000);

	/* invariant: encode must NOT alter the source region */
	MD5(mem, fs.source_sz, hash_check);
	Z_die_if(
		memcmp(src_hash, hash_check, 16)
		, "");

	/*
		set up decoding memory region
	*/
	Z_die_if(!(
		mem_dec = calloc(1, fs.source_sz + fs.parity_sz + fs.scratch_sz)
		), "");
	/* create random order into which to "send"(decode) symbols */
	next_esi = malloc(fi.cnt.n * sizeof(uint32_t));
	ffec_esi_rand(&fi, next_esi, 0);


	/*
		instantiate FEC, decode
	*/
	clock_t clock_dec = clock();
	struct ffec_instance fi_dec;
	Z_die_if(
		ffec_init_contiguous(&fp, &fi_dec, original_sz, mem_dec,
						decode, fi.seeds[0], fi.seeds[1])
		, "");
	uint32_t i;
#ifdef DEBUG
      Z_die_if(ffec_mtx_cmp(&fi, &fi_dec, &fp), "");
#endif

	/* Iterate through randomly ordered ESIs and decode for each.
	Break when decoder reports 0 symbols left to decode.
	*/
	for (i=0; i < fi_dec.cnt.cols; i++) {
		Z_log(Z_in2, "submit ESI %d", next_esi[i]);

#ifdef FFEC_TEST_COPY_SYM
		/* copy symbol into matrix manually, give only ESI */
		memcpy(ffec_get_sym(&fp, &fi_dec, next_esi[i]),
				ffec_get_sym(&fp, &fi, next_esi[i]), 
				fp.sym_len);
		if (!ffec_decode_sym(&fp, &fi_dec,
				NULL,
				next_esi[i]))
			break;

#else
		/* the more usual case: give ffec a pointer and it copy */
		if (!ffec_decode_sym(&fp, &fi_dec,
				ffec_get_sym(&fp, &fi, next_esi[i]),
				next_esi[i]))
			break;
#endif
	}
	clock_dec = clock() - clock_dec;

	/*
		verify
	decoded region must be bit-identical to source
	*/
	MD5(mem_dec, fs.source_sz, hash_check);
	Z_die_if(
		memcmp(src_hash, hash_check, 16)
		, "");

	/*
		report
	*/
	Z_log(Z_inf, "decode ELAPSED: %.2lfms", (double)clock_dec / CLOCKS_PER_SEC * 1000);
	Z_log(Z_inf, "decoded with k=%"PRIu32" < i=%"PRIu32" < n=%"PRIu32";\n\
\tinefficiency=%lf; loss tolerance=%.2lf%%; FEC=%.2lf%%\n\
\tsource size=%.4lf MiB, bitrates: enc=%"PRIu64"Mb/s, dec=%"PRIu64"Mb/s",
		/*k*/fi_dec.cnt.k, /*i*/i, /*n*/fi_dec.cnt.n,
		/*inefficiency*/(double)i / (double)fi_dec.cnt.k,
		/*loss tolerance*/((double)(fi_dec.cnt.n - i) / (double)fi_dec.cnt.n) * 100,
		/*FEC*/(fp.fec_ratio -1) * 100,
		/*source size*/(double)fs.source_sz / (1024 * 1024),
		/*enc bitrate*/(uint64_t)((double)fs.source_sz
			/ ((double)clock_enc / CLOCKS_PER_SEC)
			/ (1024 * 1024) * 8),
		/*dec bitrate*/(uint64_t)((double)fs.source_sz
			/ ((double)clock_dec / CLOCKS_PER_SEC)
			/ (1024 * 1024) * 8));

out:
	free(mem);
	free(mem_dec);
	free(next_esi);
	return err_cnt;
}
