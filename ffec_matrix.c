#include <string.h> /* memset() */

#include "ffec_matrix.h"

#ifdef Z_BLK_LVL
#undef Z_BLK_LVL
#endif
#define Z_BLK_LVL 0
/* debug levels:
	2	:	matrix operations
*/

#ifdef DEBUG
/*	ffec_matrix_row_cmp()
Returns 0 if rows identical
*/
int		ffec_matrix_row_cmp(	struct ffec_row		*a,
					struct ffec_row		*b)
{
	if (a->cnt != b->cnt)
		return 1;
	struct ffec_cell *cell_a = a->first;
	struct ffec_cell *cell_b = b->first;

	while (cell_a && cell_b)
	{
		if (ffec_matrix_cell_cmp(cell_a, cell_b))
			return 1;
		/* move one cell down the row */
		cell_a = cell_a->row_next;
		cell_b = cell_b->row_next;
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
	if (!row->first && !row->last)
		Z_inf(0, "r[%02d].cnt=%d\t.first(NULL)\t.last(NULL)", 
			row->row_id, row->cnt);
	else
		Z_inf(0, "r[%02d].cnt=%d\t.first(r%02d,c%02d)\t.last(r%d,c%d) @ 0x%lx", 
			row->row_id, row->cnt, 
			row->first->row_id, row->first->col_id,
			row->last->row_id, row->last->col_id,
			(uint64_t)row->last);
}
#endif

/*	ffec_matrix_row_link()
Link 'new_cell' into the row 'row'.
There is no requirement that cells be ordered by physical memory location,
	so something like this would make logical sense:

row:	first	->	s5:	next	->	s2
	last	->	->	->	->	s2
*/
void		ffec_matrix_row_link(struct ffec_row		*row, 
					struct ffec_cell	*new_cell)
{
#ifdef DEBUG
	/* debug print: before */
	Z_inf(0, "r[%02d].cnt=%d\t++cell(r%02d,c%02d)", 
		row->row_id, row->cnt, new_cell->row_id, new_cell->col_id);
#endif
	row->cnt++;

	if (!row->first) {
		row->first = row->last = new_cell;
	} else {
		new_cell->row_prev = row->last;
		(new_cell->row_prev)->row_next = new_cell;
		row->last = new_cell;
	}

#ifdef DEBUG
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
#ifdef DEBUG
	/* debug print: before */
	Z_inf(0, "r[%02d].cnt=%d\t--cell(r%02d,c%02d)", 
		row->row_id, row->cnt, cell->row_id, cell->col_id);
#endif
	row->cnt--;

	/* Look before us 
	If noone is before us, it means the row considers us "first".
	*/
	if (!cell->row_prev)
		row->first = cell->row_next;
	else
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

#ifdef DEBUG
	/* debug print: after */
	ffec_matrix_row_prn(row);
#endif
}

#undef Z_BLK_LVL
#define Z_BLK_LVL 0
