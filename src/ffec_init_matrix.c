#include <ffec.h>

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
	uint32_t temp;
	for (i=0, cell = fi->cells, cell_cnt = fi->cnt.k * FFEC_N1_DEGREE;
		i < cell_cnt -1; /* -1 because last cell can't swap with anyone */
		i++, cell++)
	{
#if FFEC_SWAP_NITPICK 
		/*
		Select a cell to swap with.
		Make sure no other cells in this column contain the same row ID,
			so as to preclude multiple cells in the same column being
			part of the same equation - they would XOR to 0
			and likely affect alignment of universal dark matter,
			leading to Lorenz-Fitzgerald-Einstein disturbances
			and tempting the gods to anger.

		The approach is to pick a random cell for swap and then
			go back through any previous cells in this column
			to verify it is different.
		Avoid infinite loops (like being in the last column with a double row_id)
			by using 'retry_cnt'.
		I'm sure this violates the absolute "randomness" of the
			algorithm, but no reasonable alternative is in sight.

		To be fair, the algorithm DOES still work with double
			XOR of a symbol into the same column, but for obvious
			reasons it subtracts its own entropy and reduces
			our efficiency ... OH F'ING WELL we TRIED.
		*/
		uint32_t retry_cnt = 0;
retry:
		cell_b = &fi->cells[pcg_rand_bound(&fi->rng, cell_cnt - i) + i];
		for (j = (i % FFEC_N1_DEGREE); j < i; j++) {
			if ( fi->cells[j].row_id == cell_b->row_id
				&& retry_cnt++ < FFEC_COLLISION_RETRY)
			{
				goto retry;
			}
		}
#else
		cell_b = &fi->cells[pcg_rand_bound(&fi->rng, cell_cnt - i) + i];
#endif
		/* Use a temp variable instead of triple-XOR so that
			we don't worry about XORing a cell with itself.
		 */
		temp = cell_b->row_id;
		cell_b->row_id = cell->row_id;
		cell->row_id = temp;
		/* Link into row. */
		ffec_matrix_row_link(&fi->rows[cell->row_id], cell, fi->cells);
	}
	/* Set last cell.
	It is recognized that the last cell COULD be a duplicate of any of the
		FFEC_N1_DEGREE cells before it, but no SIMPLE solution
		presents itself at this time.
	This is a literal "corner case" :P
	*/
	ffec_matrix_row_link(&fi->rows[cell->row_id], cell, fi->cells);
}
