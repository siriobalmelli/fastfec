/*	ffec_encode.c

TODO:
-	Encode a single symbol
-	Encode a range of symbols
*/

#include <ffec.h>



/*	ffec_encode()
Go through an entire block and generate its repair symbols.
The main emphasis is on SEQUENTIALLY accessing source symbols,
	limiting the pattern of random memory access to the repair symbols.

return 0 on success
*/
uint32_t	ffec_encode	(const struct ffec_params	*fp,
				struct ffec_instance		*fi)
{
	int err_cnt = 0;
	Z_die_if(!fi, "args");

	/* zero out all parity symbols */
	memset(fi->parity, 0x0, fi->cnt.p * fp->sym_len);

	int64_t i;
	uint32_t j;
	struct ffec_cell *cell;
	void *symbol;
	for (i=0; i < fi->cnt.cols; i++) {
		/* get source symbol
		Note that ffec_xor_into_symbol_() issues prefetch instructions,
			don't duplicate that here.
		*/
		cell = ffec_get_col_first(fi->cells, i);
		symbol = ffec_get_sym(fp, fi, i);
		Z_log(Z_in2, "enc(esi %ld) @0x%lx",
			i, (uint64_t)symbol);

		for (j=0; j < FFEC_N1_DEGREE; j++) {
			/* avoid empty cells under the staircase */
			if (ffec_cell_test(&cell[j]))
				continue;
			/* Avoid XOR-ing parity symbol with itself */
			if (i - fi->cnt.k == cell[j].row_id)
				continue;
			/* XOR into parity symbol for that row */
			ffec_xor_into_symbol_(symbol,
					ffec_get_sym_p(fp, fi, cell[j].row_id),
					fp->sym_len);
			Z_log(Z_in2, "xor(esi %ld) -> p%d @0x%lx",
				i, cell[j].row_id,
				(uint64_t)ffec_get_sym_p(fp, fi, cell[j].row_id));
		}
	}

out:
	return err_cnt;
}