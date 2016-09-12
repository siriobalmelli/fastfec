#ifndef ffec_matrix_h_
#define ffec_matrix_h_

#include <stdint.h>

/* parity matrix
Note that because all cells are stored contiguously in column order,
	columns are "implicit".
See the inline ffec_get_col_first().
*/
struct ffec_cell {
	uint32_t		col_id;
	uint32_t		row_id;
	struct ffec_cell	*row_prev;
	struct ffec_cell	*row_next;
	/* note that because cells are all contiguous in an array,
		'col_prev' will be immediately preceding in memory,
		and 'col_next' immediately following.
	*/
}__attribute__ ((packed));

struct ffec_row {
	uint32_t		cnt;	/* nr of linked cells */
	struct ffec_cell	*first;
	struct ffec_cell	*last;	/* for quick navigation when adding an item */
}__attribute__ ((packed));

void		ffec_matrix_row_link(struct ffec_row		*row, 
					struct ffec_cell	*new_cell);
void		ffec_matrix_row_unlink(struct ffec_row		*row, 
					struct ffec_cell	*cell);

#endif /* ffec_matrix_h_ */
