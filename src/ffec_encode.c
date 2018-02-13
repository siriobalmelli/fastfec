/*	ffec_encode.c

TODO:
-	Encode a single symbol
-	Encode a range of symbols
*/

#include <ffec_internal.h>


/*	ffec_sym_p_()
Get address of a parity symbol.
NOTE: this NOT 'esi', this is 'esi - k'
*/
NLC_INLINE void	*ffec_sym_p_		(const struct ffec_params	*fp,
					const struct ffec_instance	*fi,
					uint32_t			p)
{
	return fi->parity + (fp->sym_len * p);
}

/*	ffec_sym_n_()
Get address of any sumbol.
Encode-only: symbol may be in caller-controlled source region;
	or may be in our parity region.
*/
NLC_INLINE const void	*ffec_sym_n_	(const struct ffec_params	*fp,
					const struct ffec_instance	*fi,
					uint32_t			esi)
{
	if (esi < fi->cnt.k) {
		return fi->enc_source + (fp->sym_len * esi);
	} else {
		esi -= fi->cnt.k;
		return (const void *)ffec_sym_p_(fp, fi, esi);
	}
}


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

	for (int64_t i=0; i < fi->cnt.cols; i++) {
		/* get source symbol
		Note that ffec_xor_into_symbol_() issues prefetch instructions,
			don't duplicate that here.
		*/
		struct ffec_cell *cell = ffec_get_col_first(fi->cells, i);
		const void *symbol = ffec_sym_n_(fp, fi, i);
		Z_log(Z_in2, "enc(esi %"PRIu64") @0x%"PRIxPTR,
			i, (uintptr_t)symbol);

		for (uint32_t j=0; j < FFEC_N1_DEGREE; j++) {
			/* avoid empty cells under the staircase */
			if (ffec_cell_test(&cell[j]))
				continue;
			/* Avoid XOR-ing parity symbol with itself */
			if (i - fi->cnt.k == cell[j].row_id)
				continue;
			/* XOR into parity symbol for that row */
			ffec_xor_into_symbol_(symbol,
					ffec_sym_p_(fp, fi, cell[j].row_id),
					fp->sym_len);
			Z_log(Z_in2, "xor(esi %"PRId64") -> p%"PRIu32" @0x%"PRIxPTR,
				i, cell[j].row_id,
				(uintptr_t)ffec_sym_p_(fp, fi, cell[j].row_id));
		}
	}

out:
	return err_cnt;
}
