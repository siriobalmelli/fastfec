#include "ffec_int.h"
#include "bits.h" /* div_ceil() */

#include <math.h>

#ifdef Z_BLK_LVL
#undef Z_BLK_LVL
#endif
#define Z_BLK_LVL 0
/* debug levels:
	2	:
	3	:
*/

#ifdef FFEC_DEBUG
void		ffec_xor_into_symbol	(void *from, void *to, uint32_t sym_len)
{
	uint64_t *i_fr = from;
	uint64_t *i_to  = to;
	uint32_t i;
	for (i=0; i < sym_len / sizeof(uint64_t); i++)
		i_to[i] ^= i_fr[i];
}
#else
/*	ffec_xor_symbols()
XOR 2 symbols.
*/
void __attribute__((hot)) __attribute__((optimize("O3")))
		ffec_xor_into_symbol	(void *from, void *to, uint32_t sym_len)
{
	uint32_t i;
	for (i=0; i < sym_len / FFEC_SYM_ALIGN; i++) {
		__builtin_prefetch(from + FFEC_SYM_ALIGN, 0, 0);
		__builtin_prefetch(to + FFEC_SYM_ALIGN, 0, 0);
		asm (
			/* vlddqu replaced with vmovdqu for speed */
			"vmovdqu	0x0(%[src]),	%%ymm8;"
			"vmovdqu	0x0(%[dst]),	%%ymm0;"
			"vpxor		%%ymm0,		%%ymm8,		%%ymm0;"
			"vmovdqu	%%ymm0,		0x0(%[dst]);"

			"vmovdqu	0x20(%[src]),	%%ymm9;"
			"vmovdqu	0x20(%[dst]),	%%ymm1;"
			"vpxor		%%ymm1,		%%ymm9,		%%ymm1;"
			"vmovdqu	%%ymm1,		0x20(%[dst]);"

			"vmovdqu	0x40(%[src]),	%%ymm10;"
			"vmovdqu	0x40(%[dst]),	%%ymm2;"
			"vpxor		%%ymm2,		%%ymm10,	%%ymm2;"
			"vmovdqu	%%ymm2,		0x40(%[dst]);"

			"vmovdqu	0x60(%[src]),	%%ymm11;"
			"vmovdqu	0x60(%[dst]),	%%ymm3;"
			"vpxor		%%ymm3,		%%ymm11,	%%ymm3;"
			"vmovdqu	%%ymm3,		0x60(%[dst]);"

			"vmovdqu	0x80(%[src]),	%%ymm12;"
			"vmovdqu	0x80(%[dst]),	%%ymm4;"
			"vpxor		%%ymm4,		%%ymm12,	%%ymm4;"
			"vmovdqu	%%ymm4,		0x80(%[dst]);"

			"vmovdqu	0xA0(%[src]),	%%ymm13;"
			"vmovdqu	0xA0(%[dst]),	%%ymm5;"
			"vpxor		%%ymm5,		%%ymm13,	%%ymm5;"
			"vmovdqu	%%ymm5,		0xA0(%[dst]);"

			"vmovdqu	0xC0(%[src]),	%%ymm14;"
			"vmovdqu	0xC0(%[dst]),	%%ymm6;"
			"vpxor		%%ymm6,		%%ymm14,	%%ymm6;"
			"vmovdqu	%%ymm6,		0xC0(%[dst]);"

			"vmovdqu	0xE0(%[src]),	%%ymm15;"
			"vmovdqu	0xE0(%[dst]),	%%ymm7;"
			"vpxor		%%ymm7,		%%ymm15,	%%ymm7;"
			"vmovdqu	%%ymm7,		0xE0(%[dst]);"

			:						/* output */
			: [src] "r" (from), [dst] "r" (to)		/* input */
			:						/* clobber */
		);
		from += FFEC_SYM_ALIGN;
		to += FFEC_SYM_ALIGN;
	}
}
#endif


/*	ffec_calc_sym_counts()
Calculate the symbol counts for a given source length and FEC params.
Populates them into 'fc', which must be allocated by the caller.

returns 0 if counts are valid (aka: will not overflow internal math).
*/
int		ffec_calc_sym_counts(const struct ffec_params	*fp,
					size_t			src_len,
					struct ffec_counts	*fc)
{
	int err_cnt = 0;
	Z_die_if(!fp || !src_len || !fc, "args");
	/* temp 64-bit counters, to test for overflow */
	uint64_t t_k, t_n, t_p;

	t_k = div_ceil(src_len, fp->sym_len);
	if (t_k < FFEC_MIN_K) {
		Z_wrn("k=%ld < FFEC_MIN_K=%d;	src_len=%ld, sym_len=%d",
			t_k, FFEC_MIN_K, src_len, fp->sym_len);
		t_k = FFEC_MIN_K;
	}

	t_n = ceill(t_k * fp->fec_ratio);

	t_p = t_n - t_k;
	if (t_p < FFEC_MIN_P) {
		Z_wrn("p=%ld < FFEC_MIN_P=%d;	k=%ld, fec_ratio=%lf",
			t_p, FFEC_MIN_P, t_k, fp->fec_ratio);
		t_p = FFEC_MIN_P;
		t_n = t_p + t_k;
	}

	/* sanity check: matrix counters and iterators are all 32-bit */
	Z_die_if(
		t_n * FFEC_N1_DEGREE > (uint64_t)UINT32_MAX -2,
		"n=%ld symbols is excessive for this implementation",
		t_n);
	/* assign everything */
	fc->n = t_n;
	fc->k = t_k;
	fc->p = t_p;
	fc->k_decoded = 0; /* because common sense */

out:
	return err_cnt;
}


/*	ffec_init_matrix()
Initialize the parity matrix; distribute all the source symbols into the rows (equations).

The rub is we want an EVEN distribution of 1s between the rows,
	WHILE ensuring every column has precisely FFEC_N1_DEGREE 1s.
Our tactic takes advantage of the fact that "columns" are implicit
		in the ordering of cells (which means they will always have FFEC_N1_DEGREE 1s),
		so we can:
	a.) assign rows to cells in diagonal fashion,
	b.) RANDOMLY SWAP CELLS between each other,
	c.) link each cell to its row.
*/
void		ffec_init_matrix	(struct ffec_instance	*fi)
{
	/*
		initialize cells and rows
	*/
	unsigned int i, j;
	struct ffec_cell *cell = fi->cells;
	/* Initialize cells for 'k' source symbols. */
	uint32_t cell_cnt = fi->cnt.k * FFEC_N1_DEGREE;
	for (i=0; i < cell_cnt; i++, cell++) {
		cell->row_id = i % fi->cnt.rows;
		ffec_cell_init(cell, i);
	}
	/* Initialize cells for 'n-k' repair symbols */
	cell_cnt += fi->cnt.p * FFEC_N1_DEGREE;
	for (; i < cell_cnt; i++, cell++)
		ffec_cell_init(cell, i);
	/* Initialize the rows which immediately follow cells in memory.
	Note that part of the matrix linked list trickery involves
		a 'row' being identically-sized to a 'cell' and having
		its "c_" columns be interchangeable.
	We expect the 'cnt' variable for the row to have already been
		memset to 0 along with the rest of the scratch space.
	*/
	cell_cnt += fi->cnt.rows;
	for (; i < cell_cnt; i++, cell++)
		ffec_cell_init(cell, i);


	/*
		generate parity
	
	Go through each parity column.
	Distribute FFEC_N1_DEGREE 1's in the column,
		all of them in a staircase pattern.

	This is done before the source symbol swap
		since the parity cells are more
		likely to still be in cache.
	*/
	for (i=0; i < fi->cnt.p; i++) {
		/* get first cell in parity column */
		cell = ffec_get_col_first(fi->cells, fi->cnt.k + i);
		/* walk column */
		for (j=0; j < FFEC_N1_DEGREE; j++, cell++) {
			/* staircase: there must be space left under the diagonal */
			if ( ((int64_t)fi->cnt.p -i -j) > 0) {
				cell->row_id = i + j;
				ffec_matrix_row_link(&fi->rows[cell->row_id], cell, fi->cells);
			}
		}
	}


	/*
		source symbol swap
	
	Swap row_id among cells in order to randomize XOR distribution.
	The algorithm used is 'Knuth-Fisher-Yates'.
	*/
	struct ffec_cell *cell_b;
	uint32_t retry_cnt = 0;
	for (i=0, cell = fi->cells, cell_cnt = fi->cnt.k * FFEC_N1_DEGREE;
		i < cell_cnt -1; /* -1 because last cell can't swap with anyone */
		i++, cell++)
	{
retry:
		cell_b = &fi->cells[pcg_rand_bound(&fi->rng, cell_cnt - i) + i];
		/* To swap, use an XOR trick rather than a temp variable. */
		cell->row_id ^= cell_b->row_id;
		cell_b->row_id ^= cell->row_id;
		cell->row_id ^= cell_b->row_id;
		/*
		This added complexity lies in making sure no other cells
			in this column contain the same row ID.
		This is to preclude multiple cells in the same column being
			part of the same equation - they would XOR to 0
			and likely affect alignment of universal dark matter,
			leading to Lorenz-Fitzgerald-Einstein disturbances
			and tempting the gods to anger.
		Simply go back through any previous cells in this column
			and compare, re-executing the swap if equal.
		I'm sure this violates the absolute "randomness" of the
			algorithm, but no better alternative is in sight.

		Also, there IS a small chance that we are stuck at the end
			and no amount of retries will get us a column without
			a double cell, so use "retry_cnt" to avoid infinite
			loops.
		To be fair, the algorithm DOES still work with double
			XOR of a symbol into the same column, but for obvious
			reasons it subtracts its own entropy and reduces
			our efficiency ... but OH F'ING WELL we TRIED.
		*/
		for (j=0, cell_b = cell-1;
			j < i % FFEC_N1_DEGREE;
			j++, cell_b--)
		{
			if (cell->row_id == cell_b->row_id
				&& retry_cnt++ > FFEC_COLLISION_RETRY)
			{
				goto retry;
			}
			retry_cnt=0;
		}
		/* successful swap! Link into row. */
		ffec_matrix_row_link(&fi->rows[cell->row_id], cell, fi->cells);
	}
	/* set last cell.
	It is recognized that the last cell COULD be a duplicate of any of the
		FFEC_N1_DEGREE cells before it, but no SIMPLE solution
		presents itself at this time.
	This is a literal "corner case" :P
	*/
	ffec_matrix_row_link(&fi->rows[cell->row_id], cell, fi->cells);
}


/*	ffec_calc_lengths_int()
Calculates lengths for callers who already have symbol counts.
Internal function, so no sanity checking performed.
*/
int		ffec_calc_lengths_int(const struct ffec_params	*fp,
				size_t				src_len,
				struct ffec_sizes		*out,
				enum ffec_direction		dir,
				struct ffec_counts		*fc)
{
	int err_cnt = 0;

	/* do all maths in 64-bit, so we can avoid overflow */
	uint64_t src, par, scr;
	src = (uint64_t)fc->k * fp->sym_len;
	par = (uint64_t)fc->p * fp->sym_len;
	scr = (uint64_t)ffec_len_cells(fc) + ffec_len_rows(fc);
	/* if decoding, scratch must have space for psums */
	if (dir == decode)
		scr += fp->sym_len * fc->rows;

	/* combined size must not exceed UINT32_MAX */
	Z_die_if(src + par + scr > (uint64_t)UINT32_MAX -2,
		"cannot handle combined symbol space of %ld",
		src + par + scr);

	out->source_sz = src;
	out->parity_sz = par;
	out->scratch_sz = scr;

out:
	return err_cnt;
}


#undef Z_BLK_LVL
#define Z_BLK_LVL 0
