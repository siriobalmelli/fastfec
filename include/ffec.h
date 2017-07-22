#ifndef ffec_h_
#define	ffec_h_

/* nonlibc */
#include <nonlibc.h>
#include <zed_dbg.h>
#include <pcg_rand.h>
#include <nmath.h> /* nm_div_ceil() */

#define LIFO_MEM_TYPE uint64_t
#include <lifo.h>

#include <stdint.h>
#include <stdlib.h> /* calloc(); free() */

#include <ffec_matrix.h>



/*	N1 degree
Minimum number of 1s per matrix column.
*/
#ifndef FFEC_N1_DEGREE
	#define	FFEC_N1_DEGREE 3
#endif
#if (FFEC_N1_DEGREE < 2 || FFEC_N1_DEGREE > 7)
	#error "FEC degree out of safe bounds"
#endif

/*	Alignment!
Symbols must be a multiple of this size.
	(see ffec_xor.c: we are using YMM registers if possible or at least an unrolled loop)
Given in Bytes.
*/
#define FFEC_SYM_ALIGN 256
#if (FFEC_SYM_ALIGN != 256)
	#error "XOR loop is hand-unrolled. Only 256B alignment supported at this time."
#endif

/* Minimum number of symbols for proper operation
*/
#define FFEC_MIN_K 7
#define	FFEC_MIN_P 3

/*	Source matrix randomization passes.
Number of randomization passes executed
	when perfoming the source symbol swap step of matrix init.
*/
#ifndef FFEC_RAND_PASSES
	#define FFEC_RAND_PASSES 2
#endif
#if (FFEC_RAND_PASSES  < 1 || FFEC_RAND_PASSES  > 5)
	#error "ridiculous randomization registered!"
#endif

/*	Collision retry
Whether to engage in complexities at matrix init time; with the aim of
	making sure that the same ESI is never in a row more than once.

TODO: evaluate ACTUAL loss of (fec) inefficiency if we don't care about duplicate ESIs
	(not giving a fuck is universally faster for all algorithms).

NOTE: the code for this has been totally removed; look for it in the Git tree if interested.
 */
//#define FFEC_COLLISION_RETRY (FFEC_N1_DEGREE * 4)



/*	ffec_params
Caller sets these and passes them to ffec; must be the same at encode and decode ends.
*/
struct ffec_params {
	double		fec_ratio;	/* MUST be >1.0, SHOULD be <1.3,
						although it can be higher.
					*/
	uint32_t	sym_len;	/* Must be multiple of FFEC_SYM_ALIGN */
};

/*	ffec_counts
Symbol counts for a fec block.
*/
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

/*	ffec_instance
Caller holds this; passes a reference to it in nearly all calls to ffec.
*/
struct ffec_instance {
	uint64_t			seeds[2];
	struct pcg_state		rng;
	struct ffec_counts		cnt;

	/* These pointers are allocated and deallocated as a single memory
		region by ffec.
	Caller is responsible for calling ffec_free() on this struct.
	*/
	size_t				source_len;
	const void			*enc_source;	/* enc_ OR dec_ is NULL; that's how
								to tell whether ENCODE/DECODE.
							*/
	void				*dec_source;	

	size_t				parity_len;
	void				*parity;

	size_t				scratch_len;
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

	/* recursion stack|lifo; only on decode */
	struct lifo			*stk;
};



/*
	ffec.c
*/
NLC_PUBLIC	struct ffec_instance	*ffec_new(const struct ffec_params *fp,
						size_t			src_len,
						const void		*src,
						uint64_t		seed1,
						uint64_t		seed2);

NLC_PUBLIC	void			ffec_free(struct ffec_instance	*fi);


NLC_PUBLIC	int	ffec_test_esi		(const struct ffec_instance *fi,
						uint32_t		esi);

NLC_LOCAL	int	ffec_calc_sym_counts_	(const struct ffec_params *fp,
						size_t			src_len,
						struct ffec_counts	*fc);

NLC_LOCAL	int	ffec_calc_lengths_	(const struct ffec_params *fp,
						struct ffec_instance	*fi);


/*
	ffec_inlines.h
Must be available to callers, but extra-kludgy and unfit for
	direct inclusion in this file.
*/
#include <ffec_inlines.h>



/*
	ffec_encode.c
*/
NLC_PUBLIC	uint32_t	ffec_encode	(const struct ffec_params	*fp,
						struct ffec_instance		*fi);

/*
	ffec_decode.c
*/
NLC_PUBLIC	uint32_t	ffec_decode_sym	(const struct ffec_params	*fp,
						struct ffec_instance		*fi,
						const void			*symbol,
						uint32_t			esi);



/*
	ffec_rand.c
*/
NLC_PUBLIC	void	ffec_esi_rand	(const struct ffec_instance	*fi,
					uint32_t			*esi_seq,
					uint32_t			esi_start);

NLC_LOCAL	void	ffec_gen_matrix_(struct ffec_instance		*fi);



/*
	ffec_xor.c
*/
NLC_LOCAL	void	__attribute__((regparm(3)))
			ffec_xor_into_symbol_	(const void *from, void *to, uint32_t sym_len);



/*
	ffec_utils.c
*/
NLC_PUBLIC	int	ffec_mtx_cmp	(struct ffec_instance		*enc,
					struct ffec_instance		*dec,
					struct ffec_params		*fp);



#endif /* ffec_h_ */
