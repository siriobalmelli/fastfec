/*	ffec_inlines.h

Internal addressing and math inlines
*/

#ifndef ffec_inlines_h_
#define ffec_inlines_h_

#include <ffec.h>



/*	ffec_get_esi()
Given the address of a SOURCE symbol, calculate its ESI.

returns 'esi' or -1 on error.
*/
NLC_INLINE uint32_t	ffec_get_esi	(const struct ffec_params	*fp,
					const struct ffec_instance	*fi,
					void				*symbol)
{
	uint32_t ret = (uint64_t)(symbol - fi->source) / fp->sym_len;
	if (ret > fi->cnt.k)
		ret = -1;
	return ret;
}



/*
	memory location inlines:	esi -> symbol*
NOTE that source and parity regions are not necessarily contiguous.
*/

/*	ffec_get_sym_k()
*/
NLC_INLINE void	*ffec_get_sym_k		(const struct ffec_params	*fp,
					struct ffec_instance		*fi,
					uint32_t			k)
{
	return fi->source + (fp->sym_len * k);
}

/*	ffec_get_sym_p()
*/
NLC_INLINE void	*ffec_get_sym_p		(const struct ffec_params	*fp,
					struct ffec_instance		*fi,
					uint32_t			p)
{
	return fi->parity + (fp->sym_len * p);
}

/*	ffec_get_sym()
*/
NLC_INLINE void	*ffec_get_sym		(const struct ffec_params	*fp,
					struct ffec_instance		*fi,
					uint32_t			esi)
{
	if (esi < fi->cnt.k) {
		return ffec_get_sym_k(fp, fi, esi);
	} else {
		esi -= fi->cnt.k;
		return ffec_get_sym_p(fp, fi, esi);
	}
}

/*	ffec_get_psum()
*/
NLC_INLINE void	*ffec_get_psum		(const struct ffec_params	*fp,
					struct ffec_instance		*fi,
					uint32_t			row)
{
	return fi->psums + (fp->sym_len * row);
}



/*
	region lengths
*/

/*	ffec_len_cells()
*/
NLC_INLINE size_t	ffec_len_cells	(const struct ffec_counts *fc)
{
	return sizeof(struct ffec_cell) * fc->cols * FFEC_N1_DEGREE;
}

/*	ffec_len_rows()
*/
NLC_INLINE size_t	ffec_len_rows	(const struct ffec_counts *fc)
{
	return sizeof(struct ffec_row) * fc->rows;
}



/*	ffec_get_col_first()
Gets the first cell for 'col'.
The following FFEC_N1_DEGREE cells will be immediately following in memory.
*/
NLC_INLINE struct ffec_cell	*ffec_get_col_first(struct ffec_cell *cells, uint32_t col)
{
	return &cells[col * FFEC_N1_DEGREE];
}



#endif /* ffec_inlines_h_ */
