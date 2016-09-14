#ifndef ffec_int_h_
#define ffec_int_h_

#include "ffec.h"

/* ffec_math.h */
void		ffec_xor_into_symbol	(void *from, void *to, uint32_t sym_len);
uint64_t	div_ceil		(uint64_t a, uint64_t b);
int		ffec_calc_sym_counts(const struct ffec_params	*fp,
					size_t			src_len,
					struct ffec_counts	*fc);
void		ffec_init_matrix	(struct ffec_instance	*fi);

void		ffec_calc_lengths_int(const struct ffec_params	*fp,
				size_t				src_len,
				struct ffec_sizes		*out,
				enum ffec_direction		dir,
				struct ffec_counts		*fc);
int		ffec_cell_memcmp(struct ffec_cell *cell);

/*	ffec_get_col_first()
Gets the first cell for 'col'.
The following FFEC_N1_DEGREE cells will be immediately following in memory.
*/
Z_INL_FORCE struct ffec_cell	*ffec_get_col_first(struct ffec_cell *cells, uint32_t col)
{
#if 0
	struct ffec_cell *ret = &cells[col * FFEC_N1_DEGREE];
	Z_inf(0, "col %d => cell(r%d, c%d)\t@ 0x%lx", 
		col, ret->row_id, ret->col_id, (uint64_t)ret);
	return ret;
#else
	return &cells[col * FFEC_N1_DEGREE];
#endif
}


/* region lengths */
Z_INL_FORCE size_t	ffec_len_cells	(const struct ffec_counts *fc)
{
	return sizeof(struct ffec_cell) * fc->cols * FFEC_N1_DEGREE;
}
Z_INL_FORCE size_t	ffec_len_rows	(const struct ffec_counts *fc)
{
	return sizeof(struct ffec_row) * fc->rows;
}
#endif /* ffec_int_h_ */
