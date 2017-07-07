#ifndef ffec_int_h_
#define ffec_int_h_

#include <ffec.h>

/* Whether to engage in complexities with the aim of making sure that the same ESI
	is never in a row more than once.

TODO: evaluate ACTUAL loss of (fec) inefficiency if we don't care about duplicate ESIs
	(not giving a fuck is universally faster for all algorithms)
 */
#define FFEC_SWAP_NITPICK 0

/* ffec_math.h */
void		ffec_xor_into_symbol	(void *from, void *to, uint32_t sym_len);
int		ffec_calc_sym_counts(const struct ffec_params	*fp,
					size_t			src_len,
					struct ffec_counts	*fc);
void		ffec_init_matrix	(struct ffec_instance	*fi);

int		ffec_calc_lengths_int(const struct ffec_params	*fp,
				size_t				src_len,
				struct ffec_sizes		*out,
				enum ffec_direction		dir,
				struct ffec_counts		*fc);

/*	ffec_get_col_first()
Gets the first cell for 'col'.
The following FFEC_N1_DEGREE cells will be immediately following in memory.
*/
NLC_INLINE struct ffec_cell	*ffec_get_col_first(struct ffec_cell *cells, uint32_t col)
{
	return &cells[col * FFEC_N1_DEGREE];
}

#endif /* ffec_int_h_ */