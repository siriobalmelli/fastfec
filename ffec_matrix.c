#include <string.h> /* memset() */

#include "ffec_matrix.h"

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
	row->cnt++;

	if (!row->first) {
		row->first = row->last = new_cell;
	} else {
		new_cell->row_prev = row->last;
		(new_cell->row_prev)->row_next = new_cell;
		row->last = new_cell;
	}
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
}
