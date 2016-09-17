#ifndef ffec_matrix_h_
#define ffec_matrix_h_

#include <stdint.h>
#include "zed_dbg.h"

/* parity matrix
Note that because all cells are stored contiguously in column order,
	columns are "implicit".
See the inline ffec_get_col_first().
*/
struct ffec_cell {
	uint32_t		col_id;
	uint32_t		row_id;
	struct ffec_cell	*row_prev;	/* An unset cell has this as '-1',
						Note that NULL is a valid value
							for a cell at the head of a row.
						*/
	struct ffec_cell	*row_next;
	/* note that because cells are all contiguous in an array,
		'col_prev' will be immediately preceding in memory,
		and 'col_next' immediately following.
	*/
}__attribute__ ((packed));

struct ffec_row {
#ifdef DEBUG
	uint32_t		row_id;	/* only used for debug prints */
#endif 
	uint32_t		cnt;	/* nr of linked cells */
	struct ffec_cell	*first;
	struct ffec_cell	*last;	/* for quick navigation when adding an item */
}__attribute__ ((packed));

#ifdef DEBUG
/* debug/printing functions */
int		ffec_matrix_row_cmp(	struct ffec_row		*a,
					struct ffec_row		*b);
int		ffec_matrix_cell_cmp(	struct ffec_cell	*a,
					struct ffec_cell	*b);
void		ffec_matrix_row_prn(struct ffec_row		*row);
#endif

/* operational things */
void		ffec_matrix_row_link(struct ffec_row		*row, 
					struct ffec_cell	*new_cell);
void		ffec_matrix_row_unlink(struct ffec_row		*row, 
					struct ffec_cell	*cell);

/*	ffec_cell_unset()
Unset cell.
Note that this is a "one-way" unsetting, we don't anticipate
	relinking this cell, ever.
We simply link all cells when the matrix is created, and then
	unlink them one by one as the relevant symbols are solved
	into their equations.
*/
Z_INL_FORCE void	ffec_cell_unset(struct ffec_cell *cell)
{
	memset(cell, 0x0, sizeof(struct ffec_cell));
	cell->row_prev = (void *)-1;
}

/*	ffec_cell_test()
returns 0 if a cell is "set" 
	(aka: hasn't already been solved out of its row equation).
*/
Z_INL_FORCE int		ffec_cell_test(struct ffec_cell *cell)
{
	return (cell->row_prev == (void*)-1);
}

#endif /* ffec_matrix_h_ */
