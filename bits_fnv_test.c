#include "ffec_rand.h"
#include "bits_fnv1a.h"
#include "judyutil_L.h"

#include "mhash.h"

int main(int argc, char **argv)
{
	int err_cnt = 0;

	FILE *txt;
	Z_die_if(!(
		txt = fopen(argv[1], "r")
		), "failed to open '%s'", argv[1]);
	size_t line_sz = 256;
	char *lineptr = malloc(line_sz);

	struct j_array j_fnv, j_crc;
	j_init_nomutex(&j_fnv);
	j_init_nomutex(&j_crc);

	uint64_t hash;
	uint32_t crc32;
	MHASH h_crc32;	

	size_t line_len;
	uint64_t fnv_coll=0, crc_coll=0;
	uint64_t i=0;
	while ( (line_len = getline(&lineptr, &line_sz, txt)) != -1) {
		/* hash this and try adding to j_array to see if there is a collision */
		hash = bits_fnv_seed();
		bits_fnv_hash(&hash, (uint8_t *)lineptr, line_len); /* +1 because '\0' */
		if (j_add_(&j_fnv, hash, line_len))
			fnv_coll++;

		/* re-do this with crc32, as a baseline */
		h_crc32 = mhash_init(MHASH_CRC32); 
		mhash(h_crc32, lineptr, line_len);
		mhash_deinit(h_crc32, &crc32);
		if (j_add_(&j_crc, crc32, line_len))
			crc_coll++;
		/* track lines read */
		i++;
	}

	Z_inf(0, "%ld lines. FNV coll %ld; CRC coll %ld;", i, fnv_coll, crc_coll);
	Z_die_if(fnv_coll, "unexpected. check architecture");
out:
	return err_cnt;
}
