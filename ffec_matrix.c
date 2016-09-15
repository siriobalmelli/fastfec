#include <string.h> /* memset() */

#include "ffec_matrix.h"

#ifdef Z_BLK_LVL
#undef Z_BLK_LVL
#endif
#define Z_BLK_LVL 0
/* debug levels:
	2	:	matrix operations
*/

/* debug printing inline */
Z_INL_FORCE void		ffec_matrix_row_prn(struct ffec_row	*row)
{
	if (!row->first && !row->last)
#ifdef DEBUG
		Z_inf(2, "r[%02d].cnt=%d\t.first(NULL)\t.last(NULL)", 
			row->row_id, row->cnt);
#else
		Z_inf(2, "row.cnt=%d\t.first(NULL)\t.last(NULL)",
				row->cnt);
#endif
	else
#ifdef DEBUG
		Z_inf(2, "r[%02d].cnt=%d\t.first(r%02d,c%02d)\t.last(r%d,c%d) @ 0x%lx", 
			row->row_id, row->cnt, 
			row->first->row_id, row->first->col_id,
			row->last->row_id, row->last->col_id,
			(uint64_t)row->last);
#else
		Z_inf(2, "row.cnt=%d\t.first(r%02d,c%02d)\t.last(r%d,c%d) @ 0x%lx", 	
			row->cnt, 
			row->first->row_id, row->first->col_id,
			row->last->row_id, row->last->col_id,
			(uint64_t)row->last);
#endif
}

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
	Z_inf(2, "r[%02d].cnt=%d\t++cell(r%02d,c%02d)", 
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

	/* debug print: after */
	ffec_matrix_row_prn(row);
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
	Z_inf(2, "r[%02d].cnt=%d\t--cell(r%02d,c%02d)", 
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

	/* zero cell */
	memset(cell, 0x0, sizeof(struct ffec_cell));

	/* debug print: after */
	ffec_matrix_row_prn(row);
}

#undef Z_BLK_LVL
#define Z_BLK_LVL 0
