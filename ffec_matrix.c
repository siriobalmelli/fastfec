/*	ffec_matrix.c

This is some 411 on the implementation of the LDGM matrix for ffec.
It is assumed that the reader understands the math behind the parity
	matrix being described.
See comments in "ffec.c" for background.

Each "1" in the matrix is represented by a "cell" structure.

All the cells are contiguous in memory, and since
	the number of 1's for each column is precisely FFEC_N1_DEGREE
	(known at compile time), there is no need to explicitly represent
	"columns", as we can index into the "cells" array directly.
The inline ffec_get_col_first() gets the first cell in a column,
	after which simple pointer increments will yield the next cells.

The matrix rows (equations) however ARE implemented as a linked list.
Each equation is represented by a "row" structure.

Certain factors made it possible to simplify the implementation, with resultant
	smaller "cell" and "row" structures:

We are ALWAYS accessing the matrix starting from a SYMBOL (==column), and not
	an equation.
Therefore, the cell must know what row and column it's in, and point to its
	"siblings" (other cells belonging to the same equation).
However, this means the row structure must only know HOW MANY cells it "has" 
	(equations unsolved), and point to one of them.

All rows are contiguous in memory, so a cell doesn't need a pointer to its
	row, simply the row ID, which it can then use as an index into the
	array of rows.

2016 Sirio Balmelli
*/

#include <string.h> /* memset() */

#include "ffec_matrix.h"

#ifdef FFEC_DEBUG
/*	ffec_matrix_row_cmp()
Returns 0 if rows identical
*/
int		ffec_matrix_row_cmp(	struct ffec_row		*a,
					struct ffec_row		*b)
{
	if (a->cnt != b->cnt)
		return 1;
	struct ffec_cell *cell_a = a->last;
	struct ffec_cell *cell_b = b->last;

	while (cell_a && cell_b)
	{
		if (ffec_matrix_cell_cmp(cell_a, cell_b))
			return 1;
		/* move one cell down the row */
		cell_a = cell_a->row_prev;
		cell_b = cell_b->row_prev;
	}
	/* if cell_b is not now ALSO NULL, this is bad */
	if (cell_b)
		return 1;

	/* all is good */
	return 0;
}

/*	ffec_matrix_cell_cmp()
Compare 2 cells, return non-zero if they differ.
*/
int		ffec_matrix_cell_cmp(struct ffec_cell		*a,
					struct ffec_cell	*b)
{
	if (a->col_id != b->col_id || a->row_id != b->row_id)
		return 1;
	return 0;
}

/* debug printing */
void		ffec_matrix_row_prn(struct ffec_row		*row)
{
	if (!row->last)
		Z_inf(0, "r[%02d].cnt=%d\t.last(NULL)", 
			row->row_id, row->cnt);
	else
		Z_inf(0, "r[%02d].cnt=%d\t.last(r%d,c%d) @ 0x%lx", 
			row->row_id, row->cnt, 
			row->last->row_id, row->last->col_id,
			(uint64_t)row->last);
}
#endif

/*	ffec_matrix_row_link()
Link 'new_cell' into the row 'row'.
There is no requirement that cells be ordered by physical memory location,
	so something like this would make logical sense:

row:	last	->	s2:	prev	->	s5
*/
void		ffec_matrix_row_link(struct ffec_row		*row, 
					struct ffec_cell	*new_cell)
{
#ifdef FFEC_DEBUG
	/* debug print: before */
	Z_inf(0, "r[%02d].cnt=%d\t++cell(r%02d,c%02d)", 
		row->row_id, row->cnt, new_cell->row_id, new_cell->col_id);
#endif
	row->cnt++;

	if (!row->last) {
		row->last = new_cell;
	} else {
		new_cell->row_prev = row->last;
		(new_cell->row_prev)->row_next = new_cell;
		row->last = new_cell;
	}

#ifdef FFEC_DEBUG
	/* debug print: after */
	ffec_matrix_row_prn(row);
#endif
}

/*	ffec_matrix_row_unlink()
Unlink 'cell' from 'row'.
Any "previous" cells are pointed to any "later" cells.
Counts in 'row' are updated.
'cell' is zeroed.
*/
void		ffec_matrix_row_unlink(struct ffec_row		*row, 
					struct ffec_cell	*cell)
{
#ifdef FFEC_DEBUG
	/* debug print: before */
	Z_inf(0, "r[%02d].cnt=%d\t--cell(r%02d,c%02d)", 
		row->row_id, row->cnt, cell->row_id, cell->col_id);
#endif
	row->cnt--;

	/* Look before us.
	If there is a cell there, link it to the one after us.
	*/
	if (cell->row_prev)
		(cell->row_prev)->row_next = cell->row_next;
	/* Look after us.
	If noone is after us, row considers us "last".
	*/
	if (!cell->row_next)
		row->last = cell->row_prev;
	else
		(cell->row_next)->row_prev = cell->row_prev;

	/* invalidate cell, so it won't be used again */
	ffec_cell_unset(cell);

#ifdef FFEC_DEBUG
	/* debug print: after */
	ffec_matrix_row_prn(row);
#endif
}
