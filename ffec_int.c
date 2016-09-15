#include "ffec_int.h"

#include <math.h>

#ifdef Z_BLK_LVL
#undef Z_BLK_LVL
#endif
#define Z_BLK_LVL 0
/* debug levels:
	2	:	
	3	:	column swaps
*/

#if 0
/* YMM intrinsics, for XOR */
#include "immintrin.h"

/*	ffec_xor_symbols()
xor 2 symbols.
make heavy use of SIMD instructions.
TODO: verify the resultant assembly is optimal.
*/
void __attribute__((hot)) __attribute__((optimize("O3"))) 
		ffec_xor_into_symbol	(void *from, void *to, uint32_t sym_len)
{
	__m256i *src_blk = from;
	__m256i *dst_blk = to;

	/* loop through symbol in blocks of 256B at one time */
	uint32_t i, loop_iter;
	for (i=0, loop_iter = sym_len / FFEC_SYM_ALIGN;
		i < loop_iter; 
		i++, src_blk += FFEC_SYM_ALIGN, dst_blk += FFEC_SYM_ALIGN) 
	{
		/* prefetch next block */
		__builtin_prefetch(src_blk + FFEC_SYM_ALIGN);
		/* xor */
		dst_blk[0] = _mm256_xor_si256(src_blk[0], dst_blk[0]);
		dst_blk[1] = _mm256_xor_si256(src_blk[1], dst_blk[1]);
		dst_blk[2] = _mm256_xor_si256(src_blk[2], dst_blk[2]);
		dst_blk[3] = _mm256_xor_si256(src_blk[3], dst_blk[3]);
		dst_blk[4] = _mm256_xor_si256(src_blk[4], dst_blk[4]);
		dst_blk[5] = _mm256_xor_si256(src_blk[5], dst_blk[5]);
		dst_blk[6] = _mm256_xor_si256(src_blk[6], dst_blk[6]);
		dst_blk[7] = _mm256_xor_si256(src_blk[7], dst_blk[7]);
	}
}
#else
/*	ffec_xor_symbols()
xor 2 symbols. stand-in for the above function using YMM registers.
*/
void __attribute__((hot)) __attribute__((optimize("O3"))) 
		ffec_xor_into_symbol	(void *from, void *to, uint32_t sym_len)
{
	/* turn "sym_len" into "word_count" 
		without pushing another variable onto the stack.
	uint64_t word_count = sym_len >> 3; // uint64_t is 8 bytes
		 */
	sym_len = sym_len / FFEC_SYM_ALIGN;
	uint64_t i = 0;
	for (; i < sym_len; i++)
		((uint64_t*)to)[i] ^= ((uint64_t*)from)[i];
}
#endif

/*	div_ceil(a, b)
Integer "ceiling" operation.
If 'b' divides evenly into 'a', returns 'a / b'.
Else, returns 'a / b + 1'.
TODO: optimize: turn into inline assembly, checking the remainder register
	and xor (see p.3-5 of Intel Optimization manual: 'setge' and family).
TODO: unify, along with other implementation of div_ceil() in fl_bk,
	into a "bits" library.
*/
uint64_t	div_ceil		(uint64_t a, uint64_t b)
{
	uint64_t ret = a / b;
	if ((ret * b) < a)
		ret++;
	return ret;
}

/*	ffec_calc_sym_counts()
Calculate the symbol counts for a given source length and FEC params.
*/
int		ffec_calc_sym_counts(const struct ffec_params	*fp,
					size_t			src_len,
					struct ffec_counts	*fc)
{
	int err_cnt = 0;
	Z_die_if(!fp || !src_len || !fc, "args");
	/* sanity check length */
	Z_warn_if(src_len > FFEC_SRC_EXCESS,
		"src_len %ld > recommended limit", src_len);

	fc->k = div_ceil(src_len, fp->sym_len);
	if (fc->k < FFEC_MIN_K) {
		Z_wrn("k=%d < FFEC_MIN_K=%d;	src_len=%ld, sym_len=%d",
			fc->k, FFEC_MIN_K, src_len, fp->sym_len);
		fc->k = FFEC_MIN_K;
	}

	fc->n = ceil(fc->k * fp->fec_ratio);

	fc->p = fc->n - fc->k;
	if (fc->p < FFEC_MIN_P) {
		Z_wrn("p=%d < FFEC_MIN_P=%d;	k=%d, fec_ratio=%lf",
			fc->p, FFEC_MIN_P, fc->k, fp->fec_ratio);
		fc->k = FFEC_MIN_K;
	}

	fc->k_decoded = 0; /* because common sense */
out:
	return err_cnt;
}

/*	ffec_init_matrix()
Initialize the parity matrix.
TODO: we probably don't have to link the rows when encoding.
*/
void		ffec_init_matrix	(struct ffec_instance	*fi)
{
	/* distribute all the source symbols into the rows (equations).
	The rub is we want an EVEN distribution of 1s between the rows,
		WHILE ensuring every column has precisely FFEC_N1_DEGREE 1s.
	The tactic takes advantage of the fact that "columns" are implicit
		in the ordering of cells (which means they will always have FFEC_N1_DEGREE 1s)
		so we can:
		a.) assign rows to cells in diagonal fashion,
		b.) RANDOMLY SWAP CELLS between each other,
		c.) link each cell to its row (assigned in a).
	*/
	unsigned int i;
	struct ffec_cell *cell;
	for (i=0; i < fi->cnt.k * FFEC_N1_DEGREE; i++)
		fi->cells[i].row_id = i % fi->cnt.rows;

	/* Swap the cells among each other in order to randomize the contents.
	Notice we swap just the row_id, since every other field
		identical at this stage.
	The algorithm used is 'Knuth-Fisher-Yates'.
	*/
	uint32_t temp;
	struct ffec_cell *cell_b;
	struct ffec_row *row;
	uint32_t cell_cnt = fi->cnt.k * FFEC_N1_DEGREE;
	unsigned int j;
	for (i=0, cell = fi->cells; 
		i < cell_cnt -1; /* -1 because last cell can't swap with anyone */
		i++, cell++)
	{
retry:
		cell_b = &fi->cells[ffec_rand_bound(&fi->rng, cell_cnt - i) + i];
		temp = cell->row_id;
		cell->row_id = cell_b->row_id;
		cell_b->row_id = temp;	
		/*
		This added complexity lies in making sure no other cells 
			in this column contain the same row ID.
		This is to preclude multiple cells in the same column being 
			part of the same equation - they would XOR to 0 
			and likely affect alignment of universal dark matter, 
			tempting the gods to anger.
		Simply go back through any previous cells in this column
			and compare, re-executing the swap if equal.
		I'm sure this violates the absolute "randomness" of the
			algorithm, but no better alternative is in sight.
		*/
		for (j=0, cell_b = cell-1;
			j < i % FFEC_N1_DEGREE; 
			j++, cell_b--) 
		{
			if (cell->row_id == cell_b->row_id)
				goto retry;
		}
		/* successful swap!
		We won't visit this cell again, so set its column ID
			and link it to the pertinent row.
		*/
		cell->col_id = i / FFEC_N1_DEGREE;
		row = &fi->rows[cell->row_id];
		ffec_matrix_row_link(row, cell);
	}
	/* set last cell.
	It is recognized that the last cell COULD be a duplicate of any of the
		FFEC_N1_DEGREE cells before it, but no SIMPLE solution
		presents itself at this time.
	It bears note that this is a literal "corner case".
	*/
	cell->col_id = i / FFEC_N1_DEGREE;
	row = &fi->rows[cell->row_id];
	ffec_matrix_row_link(row, cell);


	/* Go through each parity column.
	Distribute FFEC_N1_DEGREE 1's in the column,
		all of them in a staircase pattern.
	*/
	for (i=0; i < fi->cnt.p; i++) {
		/* get first cell in parity column */
		cell = ffec_get_col_first(fi->cells, fi->cnt.k + i);
		/* walk column */
		for (j=0; 
			j < FFEC_N1_DEGREE; 
			j++, cell++)
		{
			/* staircase: must be space left under the diagonal */
			if ( (fi->cnt.p - i -j) > 0) {
				cell->col_id = fi->cnt.k + i;
				cell->row_id = i + j;
				row = &fi->rows[cell->row_id];
				ffec_matrix_row_link(row, cell);
			} else {
				/* otherwise, explicitly "unset" the cell */
				ffec_cell_unset(cell);
			}
			
		}
	}
}

/*	ffec_calc_lengths_int()
Calculates lengths for callers who already have symbol counts.
Internal function, so no sanity checking performed.
*/
void		ffec_calc_lengths_int(const struct ffec_params	*fp,
				size_t				src_len,
				struct ffec_sizes		*out,
				enum ffec_direction		dir,
				struct ffec_counts		*fc)
{
		out->source_sz = fc->k * fp->sym_len;
		out->parity_sz = fc->p * fp->sym_len;
		out->scratch_sz =  ffec_len_cells(fc) + ffec_len_rows(fc);
		/* if decoding, scratch must have space for psums */
		if (dir == decode)
			out->scratch_sz += fp->sym_len * fc->rows;
}

#undef Z_BLK_LVL
#define Z_BLK_LVL 0
