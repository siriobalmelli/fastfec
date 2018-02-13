/*	ffec_rand.c

Everything apropos symbol shuffling/distribution:
-	shuffling symbols  prior to transmission
-	generating a matrix
-	getting a random Initialization Vector for encode
*/


#include <ffec_internal.h>


/*	ffec_esi_rand_()

Populate memory at 'esi_seq' with a randomly shuffled array of all the ESIs
	in 'fi' starting from 'esi_start' (set it to 0 to obtain all symbols).
Uses Knuth-Fisher-Yates.

NOTE: 'esi_seq' should have been allocated by the caller and should be
	of size >= '(fi->cnt.n - esi_start) * sizeof(uint32_t)'

This function is used e.g.: in determining the order in which to send symbols over the wire.
*/
void		ffec_esi_rand_	(const struct ffec_instance	*fi)
{
	/* derive seeds from existing instance */
	uint64_t seeds[2] = {
		(fi->seeds[1] >> 1) +1,
		(fi->seeds[0] << 1) +1
	};
	/* setup rng */
	struct pcg_state rnd;
	pcg_seed(&rnd, seeds[0], seeds[1]);

	/*
		is simultaneous assignment and K-F-Y shuffle possible?

	In generating a matrix, we currently assign ESIs sequentially,
		then do a pass of KFY shuffle.

	Here we instead move front-to-back, assigning 'i' to the current cell,
		then swapping it with any random previous cell
		(guaranteed to contain a valid ID).
	This removes one loop (one pass through the ESI array) entirely.

	TODO: Does this compromise the randomness of the shuffle?
	*/
	fi->esi_seq[0] = 0;
	fi->esi_seq[1] = 1;
	for (uint32_t i=2, rand, temp; i < fi->cnt.n; i++) {
		fi->esi_seq[i] = i;
		rand = pcg_rand_bound(&rnd, i);
		/* use temp var (not triple-XOR): avoid XORing a cell with itself */
		temp = fi->esi_seq[i];
		fi->esi_seq[i] = fi->esi_seq[rand];
		fi->esi_seq[rand] = temp;
	}

	Z_prn_buf(Z_in2, fi->esi_seq, fi->cnt.n, "randomized ESI sequence:");
}


/*	ffec_gen_matrix_()
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
void		ffec_gen_matrix_(struct ffec_instance	*fi)
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
	uint32_t temp;
	cell_cnt = fi->cnt.k * FFEC_N1_DEGREE;

	/* Perform all the randomization passes without assigning the cells to a row.
	NOTE that back-to-front is a faster pcg_rand_bound() than front-to-back.
	*/
	for (uint32_t z=0; z < FFEC_RAND_PASSES; z++) {
		for (i = cell_cnt -1, cell = &fi->cells[cell_cnt -1];
			i > 0;
			i--, cell--)
		{
			cell_b = &fi->cells[pcg_rand_bound(&fi->rng, i)];
			/* Use a temp variable instead of triple-XOR so that
				we don't worry about XORing a cell with itself.
			 */
			temp = cell_b->row_id;
			cell_b->row_id = cell->row_id;
			cell->row_id = temp;
		}
		/* Swap cell 0, which isn't touched by the above loop.
		NOTE: this swap makes it unsafe for us to have assigned cells to their
			rows in the above loop.
		*/
		cell = fi->cells;
		cell_b = &fi->cells[pcg_rand_bound(&fi->rng, cell_cnt-1)];
		temp = cell_b->row_id;
		cell_b->row_id = cell->row_id;
		cell->row_id = temp;
	}

	/* assign cells to rows */
	for (cell = &fi->cells[cell_cnt-1]; cell >= fi->cells; cell--)
		ffec_matrix_row_link(&fi->rows[cell->row_id], cell, fi->cells);
}
