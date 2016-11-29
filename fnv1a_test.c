/*	fnv1a_test.c
Designed to test the performance of FNV1A (both 32-bit and 64-bit) 
	vs MD5 as an absolute.

NOTE that while it would be nice to see a CRC32 comparison in here
	(there was - and it was always beaten slightly by FNV32),
	this library was specifically developed so I didn't have to build
	and/or link against mhash, so MEH!
*/

#include "fnv1a.h"
#include "judyutil.h"
#include "bits.h"

#include<sys/uio.h> /* struct iovec */
#include<openssl/md5.h>

/*	equivalence()
Hashing an entire block of bytes at once should give an identical result
	vs. hashing one byte at a time, reusing the intermediate result
	each time.
Verify this.
*/
int equivalence()
{
	int err_cnt = 0;
	static const char *phrases[] = {
		"the quick brown fox jumped over the lazy dog",
		"/the/enemy/gate/is/down",
		"\t{}[]*\\&^%$#@!",
		"eeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee",
		"The player formerly known as mousecop",
		"Dan Smith"
	};

	uint64_t i, j, line_len, hash_a, hash_b;
	for (i=0; i < sizeof(phrases) / sizeof(char *); i++) {
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

int test_js_double_insert()
{
	int err_cnt = 0;

	uint8_t str[] = "hello";
	struct js_array js_test;
	js_init_nomutex(&js_test);

	Z_die_if(js_add_(&js_test, str, 8), "no previous value: add() should return 0");
	Z_die_if(!js_add_(&js_test, str, 42), "collision!: add() should return non-zero");

out:
	js_destroy(&js_test);
	return err_cnt;
}

int main(int argc, char **argv)
{
	int err_cnt = 0;
	Z_die_if(equivalence(), "");
	Z_die_if(test_js_double_insert(), "");

	FILE *txt;
	Z_die_if(!(
		txt = fopen(argv[1], "r")
		), "failed to open '%s'", argv[1]);
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
