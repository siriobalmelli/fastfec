/*	ffec_decode.c

TODO:
-	Report decoding "critical path" ... aka: smallest amount of source symbols
		needed to decode.
-	Simulate decode with "next missing source symbol" until decoding all source symbols.
*/

#include <ffec.h>



/*	ffec_esi_row_t
all relavant recursion state represented in 64b
*/
typedef struct {
	union {
		uint64_t index;
	struct {
		uint32_t esi;
		uint32_t row;
	};
	};
} __attribute__((packed)) ffec_esi_row_t;



/*	ffec_decode_sym()
Decode a symbol:
a. XOR it into the PartialSum for all of its rows.
b. Copy it into its final location in either "source" or "repair" regions.
Note that there is no advantage to splicing since we must read
	the symbol into cache for a.) anyways.
c. If any row is now left with only one symbol, the PartialSum of that
	row IS that symbol: "iteratively decode" by recursing.

Returns number of source symbols yet to receive/decode.
A return of '0' means "all source symbols decoded".
Returns '-1' on error.

NOTE on recursion:
This function, if simply calling itself, can (with large blocks) recurse
	to a point where it stack overflows.
Also, recursion is SLOW: lots of stupid crud to push onto stack.
The solution is to use a simple 64-bit LIFO struct from nonlibc;
	it is preallocated in 'fi' ... we reuse it each time we're called.

WARNING: if 'symbol' is NULL, we ASSUME it has already been copied to matrix memory
	and read it directly from ffec_dec_sym(esi)
*/
uint32_t	ffec_decode_sym		(const struct ffec_params	*fp,
					struct ffec_instance		*fi,
					void				*symbol,
					uint32_t			esi)
{

	/* set up only only once */
	err_cnt = 0;
	Z_die_if(!fp || !fi, "args");

	ffec_esi_row_t tmp;
	struct ffec_cell *cell = NULL;
	void *curr_sym = NULL;

recurse:

	/* if all source symbols have been decoded, bail */
	if (fi->cnt.k_decoded == fi->cnt.k)
		goto out;

	/* get column */
	cell = ffec_get_col_first(fi->cells, esi);
	/* if this cell has been unlinked, unwind the recursion stack */
	if (ffec_cell_test(cell))
		goto check_recurse;

	/* point to symbol in matrix */
	curr_sym = ffec_dec_sym(fp, fi, esi);
	/* if given a pointer, copy symbol from there into matrix */
	if (symbol && symbol != curr_sym) {
		Z_log(Z_in2, "pull <-(esi %"PRIu32") @0x%"PRIxPTR,
			esi, (uintptr_t)symbol);
		memcpy(curr_sym, symbol, fp->sym_len);
	}
	/* ... otherwise assume it's already there */

	Z_log(Z_in2, "decode (esi %"PRIu32") @0x%"PRIxPTR,
		esi, (uintptr_t)curr_sym);

	/* If it's a source symbol, log it. */
	if (esi < fi->cnt.k) {
		/* We may have just finished.
		Avoid extra work.
		*/
		if (++fi->cnt.k_decoded == fi->cnt.k)
			goto out;
	}

	/* get all rows */
	struct ffec_row *n_rows[FFEC_N1_DEGREE];
	memset(&n_rows, 0x0, sizeof(n_rows));
	for (unsigned int j=0; j < FFEC_N1_DEGREE; j++) {
		/* if any cells are unset, avoid processing them
			and avoid processing their row.
		*/
		if (ffec_cell_test(&cell[j])) {
			n_rows[j] = NULL;
			continue;
		}
		n_rows[j] = &fi->rows[cell[j].row_id];
		/* XOR into psum, unless we don't have to because we're done
			decoding that row.
		*/
		if (n_rows[j]->cnt > 1) {
			ffec_xor_into_symbol_(curr_sym,
					ffec_get_psum(fp, fi, cell[j].row_id),
					fp->sym_len);
			Z_log(Z_in2, "xor(esi %"PRIu32") -> p%"PRIu32" @0x%"PRIxPTR,
				esi, cell[j].row_id,
				(uintptr_t)ffec_get_psum(fp, fi, cell[j].row_id));
		}
		/* remove from row */
		ffec_matrix_row_unlink(n_rows[j], &cell[j], fi->cells);
	}

	/* See if any row can now be solved.
	This is done in a separate loop so that we have already removed
		our symbol from ALL rows it belongs to
		before recursing.
	*/
	for (unsigned int j=0; j < FFEC_N1_DEGREE; j++) {
		if (!n_rows[j])
			continue;
		if (n_rows[j]->cnt == 1) {
			cell = &fi->cells[n_rows[j]->c_last];
			tmp.esi = cell->c_me / FFEC_N1_DEGREE;
			tmp.row = cell->row_id;
			stack_push(&fi->stk, tmp.index);
		}
	}

check_recurse:
	/* If any rows are queued to be solved, "recurse".
	Notice that I am populating 'tmp' with the "row index"
		which was added into the array at some unknown
		past iteration.
	*/
	if (stack_pop(fi->stk, &tmp.index) != STACK_ERR) {
		/* reset stack variables */
		symbol = ffec_get_psum(fp, fi, tmp.row);
		esi = tmp.esi;
		goto recurse;
	}

out:
	if (err_cnt)
		return -1;
	return fi->cnt.k - fi->cnt.k_decoded;
}
