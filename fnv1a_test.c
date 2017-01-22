/*	fnv1a_test.c
Designed to test the performance of FNV1A (both 32-bit and 64-bit) 
	vs MD5 as an absolute.

NOTE that while it would be nice to see a CRC32 comparison in here
	(there was - and it was always beaten slightly by FNV32),
	this library was specifically developed so I didn't have to build
	and/or link against mhash, so MEH!
*/

#include "fnv1a.h"
#include "bits.h"

#include<sys/uio.h> /* struct iovec */
#include<openssl/md5.h>

#include <string.h>

#include "zed_dbg.h"

static const char *phrases[] = {	/**< for use in correctness testing */
	"the quick brown fox jumped over the lazy dog",
	"/the/enemy/gate/is/down",
	"\t{}[]*\\&^%$#@!",
	"eeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee",
	"The player formerly known as mousecop",
	"Dan Smith",
	"blaar"
};
#define phrase_cnt (sizeof(phrases) / sizeof(char *))

/**	equivalence()
Hashing an entire block of bytes at once should give an identical result
	vs. hashing one byte at a time, reusing the intermediate result
	each time.
Verify this.
*/
int equivalence()
{
	int err_cnt = 0;
	uint64_t i, j, line_len, hash_a, hash_b;
	for (i=0; i < phrase_cnt; i++) {
		line_len = strlen(phrases[i]);	
		hash_a = fnv_hash_l(0, (uint8_t *)phrases[i], line_len);

		j=0;
		hash_b = fnv_hash_l(NULL, (uint8_t *)&phrases[i][j++], 1);
		while (j < line_len)
			hash_b = fnv_hash_l(&hash_b, (uint8_t *)&phrases[i][j++], 1);

		Z_die_if(hash_a != hash_b, "operations not equivalent - i=%ld, j=%ld",
				i, j);
	}
out:
	return err_cnt;
}

#if 0 /**< don't want to have to build 'bits' against Judy */
#include "judyutil.h"
/**	collision_test()
*/
int collision_test(char *test_file)
{
	int err_cnt = 0;

	FILE *txt;
	Z_die_if(!(
		txt = fopen(test_file, "r")
		), "failed to open '%s'", test_file);
	size_t line_sz = 256;
	char *lineptr = malloc(line_sz);

	/* FNV */
	struct j_array j_fnv;
	j_init_nomutex(&j_fnv);
	uint64_t fnv;
	uint64_t fnv_coll=0;

	/* FNV32 */
	struct j_array j_fnv32;
	j_init_nomutex(&j_fnv32);
	uint32_t fnv32;
	uint64_t fnv32_coll=0;

	/* check against MD5 hash */
	struct js_array js_md5;
	js_init_nomutex(&js_md5);
	uint8_t md5[16];
	char md5_txt[33];
	struct iovec md5_iov;
	uint64_t md5_coll = 0;

	size_t line_len;
	uint64_t i=0;
	while ( (line_len = getline(&lineptr, &line_sz, txt)) != -1) {
		/* hash this and try adding to j_array to see if there is a collision */
		fnv = fnv_hash_l(NULL, (uint8_t *)lineptr, line_len); /* +1 because '\0' */
		if (j_add_(&j_fnv, fnv, line_len))
			fnv_coll++;

		/* FNV32 */
		fnv32 = fnv_hash(NULL, (uint8_t *)lineptr, line_len);
		if (j_add_(&j_fnv32, fnv32, line_len))
			fnv32_coll++;

		/* MD5 hash */
		md5_iov.iov_base = lineptr; md5_iov.iov_len = line_len;
		memset(md5, 0x0, sizeof(md5));
		MD5(md5_iov.iov_base, md5_iov.iov_len, md5);
		bin_2_hex(md5, md5_txt, 16);
		if (js_add_(&js_md5, (uint8_t*)md5_txt, line_len))
			md5_coll++;

		/* track lines read */
		i++;
	}

	Z_inf(0, "%ld lines. MD5 coll %ld; FNV64 coll %ld; FNV32 coll %ld", 
			i, md5_coll, fnv_coll, fnv32_coll);

out:
	return err_cnt;
}
#endif

/**	correctness()
@brief
Tests correctness of computed values against known good results
*/
int correctness()
{
	int err_cnt = 0;

	const uint64_t out_l[phrase_cnt] = {
		0x4fb124b03ec8f8f8,
		0x7814fb571359f23e,
		0xa8d4c7c3b9738aef,
		0xb47617d43071893b,
		0x400b51cb52c3929d,
		0x088a7d587bd339f3,
		0x4b64e9abbc760b0d
	};
	const uint32_t out[phrase_cnt] = {
		0x406d1fd8,
		0x45d2df9e,
		0x7e928eaf,
		0xc83e6efb,
		0x7b8e245d,
		0x0d2b7f73,
		0x6f93f02d
	};

	for (int i = 0; i < phrase_cnt; i++) {
		/* 64-bit */
		uint64_t res_l = fnv_hash_l(NULL, phrases[i], strlen(phrases[i]));
		Z_die_if(res_l != out_l[i], "i=%d; 0x%lx != 0x%lx", i, res_l, out_l[i]);
		/* 32-bit */
		uint32_t res = fnv_hash(NULL, phrases[i], strlen(phrases[i]));
		Z_die_if(res != out[i], "i=%d 0x%x != 0x%x", i, res, out[i]);
	}

out:
	return err_cnt;
}

/**	main()
*/
int main(int argc, char **argv)
{
	int err_cnt = 0;

	Z_die_if(equivalence(), "");
	Z_die_if(correctness(), "");
#if 0	/**< don't want to have judyutil as a dependency */
	Z_die_if(collision_test(argv[1]), "");
#endif

out:
	return err_cnt;
}
