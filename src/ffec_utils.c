/*	ffec_utils.c

Non-essentials go here.
*/

#include <ffec.h>


/*	ffec_mtx_cmp()
Compare 2 FEC matrices, cells and rows - which should be identical
		(because their IDs are offsets, not absolute addresses ;)

NOTE that matrices are NO LONGER identical after decode has started,
	because decoding will remove cells from the rows.

returns 0 if identical
*/
int ffec_mtx_cmp(struct ffec_instance *enc, struct ffec_instance *dec, struct ffec_params *fp)
{
	int err_cnt = 0;
	/* verify seeds */
	Z_err_if(memcmp(enc->seeds, dec->seeds, sizeof(enc->seeds)),
		"FEC seeds mismatched: enc(0x%"PRIx64", 0x%"PRIx64") != dec(0x%"PRIx64", 0x%"PRIx64")",
		enc->seeds[0], enc->seeds[1], dec->seeds[0], dec->seeds[1]);

	/* verify matrix cells */
	if (memcmp(enc->cells, dec->cells, sizeof(struct ffec_cell) * enc->cnt.k * FFEC_N1_DEGREE))
	{
		Z_log(Z_err, "FEC matrices mismatched");
		uint32_t mismatch_cnt=0;
		for (uint32_t i=0; i < enc->cnt.k * FFEC_N1_DEGREE; i++) {
			if (memcmp(&enc->cells[i], &dec->cells[i], sizeof(enc->cells[0])))
				mismatch_cnt++;
		}
		printf("\nmismatch %d <= %d cells\n\n", mismatch_cnt, enc->cnt.n * FFEC_N1_DEGREE);
	}

	return err_cnt;
}
