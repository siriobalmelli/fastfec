#ifndef ffec_h_
#define	ffec_h_

/* nonlibc */
#include <nonlibc.h>
#include <zed_dbg.h>
#include <pcg_rand.h>
#include <nmath.h> /* nm_div_ceil() */

#define STACK_MEM_TYPE uint64_t
#include <stack.h>

#include <stdint.h>

#include <ffec_matrix.h>



/*	N1 degree
Minimum number of 1s per matrix column.
*/
#ifndef FFEC_N1_DEGREE
	#define	FFEC_N1_DEGREE 3
#endif
#if (FFEC_N1_DEGREE < 2 || FFEC_N1_DEGREE > 5)
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

/*	Collision retry
Whether to engage in complexities at matrix init time; with the aim of
	making sure that the same ESI is never in a row more than once.

TODO: evaluate ACTUAL loss of (fec) inefficiency if we don't care about duplicate ESIs
	(not giving a fuck is universally faster for all algorithms).
 */
//#define FFEC_COLLISION_RETRY (FFEC_N1_DEGREE * 4)



/*	ffec_direction
*/
enum ffec_direction {
	encode,
	decode
};

/*	ffec_params
Caller sets these and passes them to ffec; must be the same at encode and decode ends.
*/
struct ffec_params {
	double		fec_ratio;	/* MUST be >1.0, SHOULD be <1.3,
						although it can be higher.
					*/
	uint32_t	sym_len;	/* Must be multiple of FFEC_SYM_ALIGN */
};

/*	ffec_sizes
Organizes necessary memory region sizes for the caller.
This is important as ffec does NO memory allocation;
	must be handled by caller.
*/
struct ffec_sizes {
	size_t		source_sz;
	size_t		parity_sz;
	size_t		scratch_sz;
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



/*
	ffec.c
*/
NLC_PUBLIC	int	ffec_calc_lengths(const struct ffec_params	*fp,
					size_t				src_len,
					struct ffec_sizes		*out,
					enum ffec_direction		dir);

NLC_PUBLIC	int	ffec_init	(const struct ffec_params	*fp,
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
	- is large enough for repair symbols and the scratch region
*/
NLC_INLINE int		ffec_init_contiguous(
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

NLC_PUBLIC	int	ffec_test_esi		(const struct ffec_instance *fi,
						uint32_t		esi);

NLC_LOCAL	int	ffec_calc_sym_counts_(const struct ffec_params	*fp,
						size_t			src_len,
						struct ffec_counts	*fc);

NLC_LOCAL	int	ffec_calc_lengths_int_(const struct ffec_params	*fp,
						size_t			src_len,
						struct ffec_sizes	*out,
						enum ffec_direction	dir,
						struct ffec_counts	*fc);


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
						void				*symbol,
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
			ffec_xor_into_symbol_	(void *from, void *to, uint32_t sym_len);



/*
	ffec_utils.c
*/
NLC_PUBLIC	int	ffec_mtx_cmp	(struct ffec_instance		*enc,
					struct ffec_instance		*dec,
					struct ffec_params		*fp);



#endif /* ffec_h_ */
