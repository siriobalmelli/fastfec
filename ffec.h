/*	ffec.h
The Fast (and barebones) FEC library.

This library implements a basic (and fast) LDGM staircase FEC.
For some background data and match, please see:

- Design, Evaluation and Comparison of FOur Large Block FEC Codecs ...
	Vincent Roca, Christoph Neumann - 19 May 2006

DESIGN
The design goal is a FEC codec optimized for:
	a. speed and memory footprint
	b. large objects (10s of MBs)
	c. low fec_ratio's (i.e.: <1.2 ... aka <20%)
For these reasons it was chosen NOT to use gaussian elimination
	at the decoder.

NOMENCLATURE

symbol		:	A single "unit" of data in FEC.
			It is assumed that "symbol" is the payload of a single
				network packet, therefore <=9KB and >=1KB.
			A symbol MUST be a multiple of 256B (2048b) because
				we rely on parallelized AVX2 instructions for XOR.
k		:	Number of "source" symbols.
			This is the data which must be recovered at the receiver.
n		:	Total number of symbols.
p		:	Number of "parity" symbols (created by XORing several source symbols together).
			AKA: "repair" symbols.
			p = n - k
esi		:	Encoding Symbol ID.
fec_ratio	:	'n = fec_ratio * k'
			Example: "1.1" == "10% FEC"

2016: Sirio Balmelli and Anthony Soenen

TODO:
	- handle extremely small object
*/
#ifndef ffec_h_
#define	ffec_h_

#include <stdint.h>
#include <stdlib.h> /* rand48_r() */
#include <unistd.h> /* usleep() */

#include "ffec_matrix.h"

#include "zed_dbg.h"

#define	FFEC_N1_DEGREE 3		/* minimum number of 1s per matrix column */
#if (FFEC_N1_DEGREE < 2 || FFEC_N1_DEGREE > 5)
	#error "FEC degree out of safe bounds"
#endif

#define FFEC_SYM_ALIGN 256		/* Given in Bytes.
					Symbols are a multiple of this size.
					Must be a multiple of 32, since we are using
						YMM registers to xor.
					*/

#define	FFEC_SRC_EXCESS 268435456	/* Discourage FEC'ing of blocks
						>256MB in size.
					*/


enum ffec_direction {
	encode,
	decode
};

struct ffec_params {
	double		fec_ratio;	/* MUST be >1.0, SHOULD be <1.3 */
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
	uint32_t			seed;
	struct drand48_data		rand_buf;
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

int		ffec_init_instance(const struct ffec_params	*fp,
				struct ffec_instance		*fi,
				size_t				src_len,
				void				*source,
				void				*parity,
				void				*scratch,
				enum ffec_direction		dir,
				uint32_t			seed);
/*	ffec_init_instance_contiguous()
Caller crosses his heart and swears that 'memory':
	- begins with all the source symbols
	- has enough space for repair symbols and the scratch region
*/
Z_INL_FORCE int	ffec_init_instance_contiguous(
				const struct ffec_params	*fp,
				struct ffec_instance		*fi,
				size_t				src_len,
				void				*memory,
				enum ffec_direction		dir,
				uint32_t			seed)
{
	return ffec_init_instance(fp, fi, src_len, memory, NULL, NULL, dir, seed);
}

uint32_t	ffec_encode	(const struct ffec_params	*fp, 
				struct ffec_instance		*fi);
uint32_t	ffec_decode_sym	(const struct ffec_params	*fp,
				struct ffec_instance		*fi,
				void				*symbol,
				uint32_t			esi);

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
#endif /* ffec_h_ */
