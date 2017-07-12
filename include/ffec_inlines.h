/*	ffec_inlines.h

Internal addressing and math inlines
*/

#ifndef ffec_inlines_h_
#define ffec_inlines_h_

#include <ffec.h>



/*
	addressing inlines
*/

/*	ffec_sym_p()

Get address of a parity symbol.
NOTE: this NOT 'esi', this is 'esi - k'
*/
NLC_INLINE void	*ffec_sym_p		(const struct ffec_params	*fp,
					const struct ffec_instance	*fi,
					uint32_t			p)
{
	return fi->parity + (fp->sym_len * p);
}

/*	ffec_enc_sym()
Encode-only: symbol may be in caller-controlled source region;
	or may be in our parity region.
*/
NLC_INLINE void	*ffec_enc_sym		(const struct ffec_params	*fp,
					const struct ffec_instance	*fi,
					uint32_t			esi)
{
	if (esi < fi->cnt.k) {
		return (void *)fi->enc_source + (fp->sym_len * esi);
	} else {
		esi -= fi->cnt.k;
		return ffec_sym_p(fp, fi, esi);
	}
}

/*	ffec_dec_sym()
Decode-only: get any ESI (they are all contiguous: faster)
*/
NLC_INLINE void	*ffec_dec_sym		(const struct ffec_params	*fp,
					const struct ffec_instance	*fi,
					uint32_t			esi)
{
	return fi->dec_source + (fp->sym_len * esi);
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
