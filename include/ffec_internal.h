#ifndef ffec_internal_h_
#define ffec_internal_h_

#include <ffec.h>

/*	ffec_get_col_first()
Gets the first cell for 'col'.
The following FFEC_N1_DEGREE cells will be immediately following in memory.
*/
NLC_INLINE struct ffec_cell	*ffec_get_col_first(struct ffec_cell *cells, uint32_t col)
{
	return &cells[col * FFEC_N1_DEGREE];
}

#endif /* ffec_internal_h_ */
