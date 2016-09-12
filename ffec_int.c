#include "ffec_int.h"

#include <math.h>

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

	/* TODO: figure out the MINIMUM sizes here */
	fc->k = div_ceil(src_len, fp->sym_len);
	fc->n = ceil(fc->k * fp->fec_ratio);
	fc->p = fc->n - fc->k;

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
	unsigned int i, j;
	struct ffec_cell *cell;
	for (i=0; i < fi->cnt.k * FFEC_N1_DEGREE; i++)
		fi->cells[i].row_id = i % fi->cnt.rows;
	/* Swap just the IDs, since everything else in the matrix is 0s anyhow.
	Notice that we swap WHOLE COLUMNS. 
	This is to preclude multiple cells in the same column being part of the
		same row - they would XOR to 0 and likely affect alignment
		of universal dark matter, tempting the gods to anger.
	It's also faster (less random numbers to generate).
	And diagonals are cute.
	*/
	long rand48;
	struct ffec_cell *cell_b;
	for (i=0; i < fi->cnt.k; i++) {
		/* get 2 columns */
		lrand48_r(&fi->rand_buf, &rand48);
		cell = ffec_get_col_first(fi->cells, rand48 % fi->cnt.k);
		lrand48_r(&fi->rand_buf, &rand48);
		cell_b = ffec_get_col_first(fi->cells, rand48 % fi->cnt.k);
		/* swap all rows with each other */
		for (j=0; j < FFEC_N1_DEGREE; j++) {
			/* use rand48 as a scratchpad */
			rand48 = cell[j].row_id;
			cell[j].row_id = cell_b[j].row_id;
			cell_b[j].row_id = rand48;
		}
	}
	/* Walk through all cells again and add them into the linked list
		for their respective row.
	Also set their column IDs, which is necessary when decoding.
	*/
	for (i=0; i < fi->cnt.k * FFEC_N1_DEGREE; i++) {
		cell = &fi->cells[i];
		cell->col_id = i / FFEC_N1_DEGREE;
		ffec_matrix_row_link(&fi->rows[cell->row_id], cell);
	}


	/* Go through each parity column.
	Distribute FFEC_N1_DEGREE 1's in the column,
		where the first 2 ones are ALWAYS in a staircase pattern.
	*/
	uint32_t triangle_space;
	for (i=0; i < fi->cnt.p; i++) {
		/* get first cell in parity column */
		cell = ffec_get_col_first(fi->cells, fi->cnt.k + i);

		/* Get equation row.
		Because triangular matrix, the first 1 in this column
			MUST be the same-numbered row.
		AKA: this is the "edge" of the triangle.
		*/
		cell->row_id = i;
		ffec_matrix_row_link(&fi->rows[i], cell);

		/* FEC Staircase: next 1 goes one row BELOW,
			unless there IS no row below.
		Use 'triangle_space' to track availability of space
			"under the triangle".
		*/
		triangle_space = fi->cnt.p - i -1;
		if (triangle_space--) {
			cell[1].row_id = i+1;
			ffec_matrix_row_link(&fi->rows[i+1], &cell[1]);		
		}

		/* FEC Staircase: any next 1(s) go only on rows BELOW the current */
		unsigned int k;
		struct ffec_row *row;
		for (j=2; 
			j < FFEC_N1_DEGREE && triangle_space > 0; 
			j++, triangle_space--) 
		{
			/* Pick a random row UNDER the staircase which can
				be added to the equation row.
			(brute-force) verify that it hasn't already been picked
				for this column.
			*/
regen:
			lrand48_r(&fi->rand_buf, &rand48);
			cell[j].row_id = (rand48 % triangle_space) + i + j;
			for (k=2; k < j; k++) {
				if (cell[j].row_id == cell[k].row_id)
					goto regen;
			}
			
			/* add to row */
			row = &fi->rows[cell[j].row_id];
			ffec_matrix_row_link(row, &cell[j]);
		}
	}
}

