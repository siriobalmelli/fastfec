#ifndef ffec_matrix_h_
#define ffec_matrix_h_

#include <stdint.h>
#include <nonlibc.h>



/* parity matrix
Note that because all cells are stored contiguously in column order,
	columns are "implicit".
See the inline ffec_get_col_first().
*/
struct ffec_cell {
	/* linked-list stuff: must be at top of struct.
	An "unset" cell is (c_prev == c_next && c_next == c_me)
	*/
	uint32_t		c_prev;
	uint32_t		c_next;
	uint32_t		c_me; /* to get col_id, divide by N1_DEGREE */

	uint32_t		row_id;
}__attribute__ ((packed));

/*	row
Note that this struct must be of IDENTICAL size to 'ffec_cell'
	so that we can index using IDs rather than pointers.
*/
struct ffec_row {
	/* linked list stuff */
	uint32_t		c_last;
	uint32_t		c_first;
	uint32_t		c_me;	/* only used for debug prints (and to pad this struct) */

	uint32_t		cnt;	/* nr of linked cells */
}__attribute__ ((packed));

/* operational things */
void		ffec_matrix_row_link(	struct ffec_row		*row,
					struct ffec_cell	*new_cell,
					struct ffec_cell	*base);
void		ffec_matrix_row_unlink(	struct ffec_row		*row,
					struct ffec_cell	*cell,
					struct ffec_cell	*base);

/*	ffec_cell_init()
*/
NLC_INLINE void	ffec_cell_init(struct ffec_cell *cell, uint32_t id)
{
	cell->c_prev = cell->c_next = cell->c_me = id;
}

/*	ffec_cell_test()
returns 0 if a cell is "set"
	(aka: hasn't already been solved out of its row equation).
*/
NLC_INLINE int		ffec_cell_test(struct ffec_cell *cell)
{
	return (cell->c_prev == cell->c_next && cell->c_next == cell->c_me);
}

#endif /* ffec_matrix_h_ */
