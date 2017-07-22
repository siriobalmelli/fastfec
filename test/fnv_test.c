/*	fnv_test.c
Designed to test the correctness of FNV1A (both 32-bit and 64-bit).

NOTE that while it would be nice to see a CRC32 comparison in here
	(there was - and it was always beaten slightly by FNV32),
	this library was specifically developed so I didn't have to build
	and/or link against mhash or zlib, so MEH!
*/

#include <nonlibc.h>
#include <fnv.h>
#include <zed_dbg.h>

#include <nlc_urand.h>
#include <pcg_rand.h>

#include <stdlib.h> /* malloc() */
#include <time.h> /* clock() */

static const char *phrases[] = {	/**< for use in correctness testing */
	"",
	"the quick brown fox jumped over the lazy dog",
	"/the/enemy/gate/is/down",
	"\t{}[]*\\&^%$#@!",
	"eeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee",
	"The player formerly known as mousecop",
	"Dan Smith",
	"blaar"
};
#define phrase_cnt (sizeof(phrases) / sizeof(char *))

static const uint64_t out64[phrase_cnt] = {
	0xcbf29ce484222325,
	0x4fb124b03ec8f8f8,
	0x7814fb571359f23e,
	0xa8d4c7c3b9738aef,
	0xb47617d43071893b,
	0x400b51cb52c3929d,
	0x088a7d587bd339f3,
	0x4b64e9abbc760b0d
};
static const uint32_t out32[phrase_cnt] = {
	0x811c9dc5,
	0x406d1fd8,
	0x45d2df9e,
	0x7e928eaf,
	0xc83e6efb,
	0x7b8e245d,
	0x0d2b7f73,
	0x6f93f02d
};



/*	equivalence()
Hashing an entire block of bytes at once should give an identical result
	vs. hashing one byte at a time, reusing the intermediate result
	each time.
Verify this.

returns 0 on success
*/
int equivalence()
{
	int err_cnt = 0;

	for (uint64_t i=0; i < phrase_cnt; i++) {
		/* hash entire phrase in a single operation */
		uint64_t line_len = strlen(phrases[i]);	
		uint64_t hash_a = fnv_hash64(0, (uint8_t *)phrases[i], line_len);

		/* initialize */
		uint64_t hash_b = fnv_hash64(NULL, NULL, 0);
		/* hash byte by byte */
		uint64_t j=0;
		while (j < line_len)
			hash_b = fnv_hash64(&hash_b, (uint8_t *)&phrases[i][j++], 1);

		/* compare */
		Z_err_if(hash_a != hash_b,
			"operations not equivalent : i %"PRIu64" != j %"PRIu64,
			i, j);
	}

	return err_cnt;
}



/*	correctness()
Tests correctness of computed values against known good results

returns 0 on success
*/
int correctness()
{
	int err_cnt = 0;

	for (uint_fast16_t i = 0; i < phrase_cnt; i++) {
		/* 64-bit */
		uint64_t res64 = fnv_hash64(NULL, phrases[i], strlen(phrases[i]));
		Z_err_if(res64 != out64[i], "i=%"PRIuFAST16"; 0x%"PRIx64" != 0x%"PRIx64, i, res64, out64[i]);
		/* 32-bit */
		uint32_t res32 = fnv_hash32(NULL, phrases[i], strlen(phrases[i]));
		Z_err_if(res32 != out32[i], "i=%"PRIuFAST16"; 0x%"PRIx32" != 0x%"PRIx32, i, res32, out32[i]);
	}

	return err_cnt;
}



/*	speed()

Run algo on a large chunk of data; check performance.
*/
int	speed()
{
	const size_t sz = 1UL << 20;

	int err_cnt = 0;
	void *large = NULL;

	/* alloc large block, seed with random data */
	Z_die_if(!(
		large = malloc(sz)
		), "malloc %zu", sz);
	uint64_t seeds[2];
	Z_die_if(nlc_urand(seeds, sizeof(seeds)) != sizeof(seeds), "");
	pcg_randset(large, sz, seeds[0], seeds[1]);

	/* run FNV64 */
	nlc_timing_start(fnv64);
	uint64_t res64 = fnv_hash64(NULL, large, sz);
	nlc_timing_stop(fnv64);
	Z_log(Z_inf, "fnv_hash64 on %zuB: %fs - %"PRIx64,
			sz, (double)fnv64 / CLOCKS_PER_SEC, res64);

	/* run FNV32 */
	nlc_timing_start(fnv32);
	uint32_t res32 = fnv_hash64(NULL, large, sz);
	nlc_timing_stop(fnv32);
	Z_log(Z_inf, "fnv_hash32 on %zuB: %fs - %"PRIx32,
			sz, (double)fnv32 / CLOCKS_PER_SEC, res32);

out:
	free(large);
	return err_cnt;
}



/*	main()
*/
int main()
{
	int err_cnt = 0;

	err_cnt += equivalence();
	err_cnt += correctness();
	err_cnt += speed();

	return err_cnt;
}
