#ifndef ffec_h_
#define	ffec_h_

#include <stdint.h>
#include <unistd.h> /* usleep() */

#include "ffec_matrix.h"

#include "pcg_rand.h"
#include "zed_dbg.h"

#define	FFEC_N1_DEGREE 3		/* minimum number of 1s per matrix column */
#if (FFEC_N1_DEGREE < 2 || FFEC_N1_DEGREE > 5)
	#error "FEC degree out of safe bounds"
#endif

#define FFEC_SYM_ALIGN 256		/* Given in Bytes.
					Symbols must be a multiple of this size.
					Must itself be a multiple of 32, since we are using
						YMM registers to XOR.
					*/
#if (FFEC_SYM_ALIGN != 256)
	#error "XOR loop is hand-unrolled. this is bad"
#endif

/* Minimum number of symbols for proper operation. */
#define FFEC_MIN_K 7
#define	FFEC_MIN_P 3

#define FFEC_COLLISION_RETRY (FFEC_N1_DEGREE * 4)

enum ffec_direction {
	encode,
	decode
};

struct ffec_params {
	double		fec_ratio;	/* MUST be >1.0, SHOULD be <1.3,
						although it can be higher.
						*/
	uint32_t	sym_len;	/* Must be multiple of FFEC_SYM_ALIGN */

};
/* organizes necessary memory region sizes for the caller */
struct ffec_sizes {
	size_t		source_sz;
	size_t		parity_sz;
	size_t		scratch_sz;
};

/* symbol counts for a fec block */
struct ffec_counts {
	uint32_t			k;	/* number source symbols */
	union {
	uint32_t			n;	/* total number of symbols */
	uint32_t			cols;
	};
	union {
	uint32_t			p;	/* number of parity symbols */
	uint32_t			rows;
	};
	uint32_t			k_decoded;
}__attribute__ ((packed));

struct ffec_instance {
	uint64_t			seeds[2];
	struct pcg_rand_state		rng;
	struct ffec_counts		cnt;
	/* NO pointers allocated or deallocated by FEC.
	Caller is responsible to make sure they're properly sized -
		use ffec_calc_lengths().
	*/
	void				*source;
	void				*parity;
	union {
	void				*scratch; /* FEC does all work here */
	struct ffec_cell		*cells;	/* ... and the scratch region
							always begins with the
							array of cells.
						*/
	};
	/* references into 'scratch' */
	struct ffec_row			*rows;
	void				*psums; /* only on decode */
};


/* public functions */
int		ffec_calc_lengths(const struct ffec_params	*fp,
				size_t				src_len,
				struct ffec_sizes		*out,
				enum ffec_direction		dir);

int		ffec_init(	const struct ffec_params	*fp,
				struct ffec_instance		*fi,
				size_t				src_len,
				void				*source,
				void				*parity,
				void				*scratch,
				enum ffec_direction		dir,
				uint64_t			seed1,
				uint64_t			seed2);
/*	ffec_init_contiguous()
Caller crosses his heart and swears that 'memory':
	- begins with all the source symbols
	- has enough space for repair symbols and the scratch region
*/
Z_INL_FORCE int	ffec_init_contiguous(
				const struct ffec_params	*fp,
				struct ffec_instance		*fi,
				size_t				src_len,
				void				*memory,
				enum ffec_direction		dir,
				uint64_t			seed1,
				uint64_t			seed2)
{
	return ffec_init(fp, fi, src_len, memory, NULL, NULL, dir, seed1, seed2);
}

uint32_t	ffec_encode	(const struct ffec_params	*fp,
				struct ffec_instance		*fi);
uint32_t	ffec_decode_sym	(const struct ffec_params	*fp,
				struct ffec_instance		*fi,
				void				*symbol,
				uint32_t			esi);
void		ffec_esi_rand	(const struct ffec_instance	*fi,
				uint32_t			*esi_seq);


/* TODO: untested */
Z_INL_FORCE uint32_t	ffec_get_esi	(const struct ffec_params	*fp,
					const struct ffec_instance	*fi,
					void				*symbol)
{
	return (uint64_t)(symbol - fi->source) / fp->sym_len;
}
/* symbol locations */
Z_INL_FORCE void	*ffec_get_sym_k(const struct ffec_params	*fp,
					struct ffec_instance		*fi,
					uint32_t			k)
{
	return fi->source + (fp->sym_len * k);
}
Z_INL_FORCE void	*ffec_get_sym_p(const struct ffec_params	*fp,
					struct ffec_instance		*fi,
					uint32_t			p)
{
	return fi->parity + (fp->sym_len * p);
}
Z_INL_FORCE void	*ffec_get_sym	(const struct ffec_params	*fp,
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
Z_INL_FORCE void	*ffec_get_psum	(const struct ffec_params	*fp,
					struct ffec_instance		*fi,
					uint32_t			row)
{
	return fi->psums + (fp->sym_len * row);
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
					
#endif /* ffec_h_ */
