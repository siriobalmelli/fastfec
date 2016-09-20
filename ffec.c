/*	ffec.h
The Fast (and barebones) FEC library.

This library implements a basic (and fast) LDGM staircase FEC.
For some background data and math, please see:

- Design, Evaluation and Comparison of Four Large Block FEC Codecs ...
	Vincent Roca, Christoph Neumann - 19 May 2006
	https://hal.inria.fr/inria-00070770/document
- Systems of Linear Equations: Gaussian Elimination
	http://www.sosmath.com/matrix/system1/system1.html


DESIGN
The design goal is a FEC codec optimized for:
	a. speed and memory footprint
	b. large objects: depending on 'fec_ratio' (see def. below),
		anything <=2GB should be OK.
	c. low fec_ratio's (i.e.: <1.2 ... aka <20%)
For these reasons it was chosen NOT to use gaussian elimination
	at the decoder.


THREAD-SAFETY : NONE


NOMENCLATURE

FEC		:	Forward Error Correction
			The practice of generating some additional data at the
				"sender", to be sent along with the original data,
				so that "receiver" can re-compute data lost
				in transit (as long as it is under a certain
				percentage).
LDGM		:	Low Density Generator Matrix
			Describes the method for implementing this type of FEC,
				where computations to be done on the data
				are represented in a "low density" (aka: sparse)
				matrix, which is also then used to GENERATE the
				FEC data (therefore: "generator").

symbol		:	A single "unit" of data in FEC.
			It is assumed that "symbol" is the payload of a single
				network packet, therefore <=9KB and >=1KB.
k		:	Number of "source" symbols.
			This is the data which must be recovered at the receiver.
n		:	Total number of symbols.
			This is "k" source symbols plus "p" parity symbols.
p		:	Number of "parity" symbols (created by XORing several source symbols together).
			AKA: "repair" symbols.
			p = n - k
esi		:	Encoding Symbol ID (zero-indexed).
fec_ratio	:	'n = fec_ratio * k'
			Example: "1.1" == "10% FEC"
inefficiency	:	"number of symbols needed to decode" / k
			This is always greater than 1, and denotes
				how many MORE symbols than source are needed
				to allow decoding.
			A perfect codec (e.g. reed-solomon) has inefficiency == 1,
				but the tradeoff of LDGM is (slight) inefficiency.


2016: Sirio Balmelli and Anthony Soenen
*/

/* /dev/urandom */
#ifndef _GNU_SOURCE
	#define _GNU_SOURCE
#endif
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <alloca.h>

#include "ffec_int.h"
#include "judyutil_L.h"

/*	ffec_calc_lengths()
Calculate the needed memory lengths which the caller must
	allocate so FEC can operate.
returns 0 if parameters seem copacetic.

NOTE: ffec_init_instance() will NOT perform these safety checks,
	as it assumes the caller has called ffec_calc_lengths() first
	AND CHECKED THE RESULT.
*/
int		ffec_calc_lengths(const struct ffec_params	*fp,
				size_t				src_len,
				struct ffec_sizes		*out,
				enum ffec_direction		dir)
{
	int err_cnt = 0;
	Z_die_if(!fp || !src_len, "args");

	int alignment = fp->sym_len % FFEC_SYM_ALIGN;
	Z_die_if(alignment,
		"requested sym_len %d not a multiple of %d",
		fp->sym_len, FFEC_SYM_ALIGN);

	/* get counts */
	struct ffec_counts fc;
	Z_die_if(
		ffec_calc_sym_counts(fp, src_len, &fc)
		, "");

	/* use counts to calc lengths */
	ffec_calc_lengths_int(fp, src_len, out, dir, &fc);
	/* sanity-check resulting sizes, keeping in mind that all the
		FEC math is in 32-bit.
	*/
	Z_die_if(out->scratch_sz + out->parity_sz + out->source_sz > UINT32_MAX -2,
		"proposed total memory area of %ld is out of range for this library",
		out->scratch_sz + out->parity_sz + out->source_sz);

out:
	return err_cnt;
}

/*	ffec_init_instance()
Requires a pointer to a struct fec_instance rather than allocating it.
This is to allow caller to decide its location.
Corollary: we must not assume 'fi' has been zeroed: we must set 
	EVERY variable.
*/
int		ffec_init_instance(const struct ffec_params	*fp,
				struct ffec_instance		*fi,
				size_t				src_len,
				void				*source,
				void				*parity,
				void				*scratch,
				enum ffec_direction		dir,
				uint64_t			seed1,
				uint64_t			seed2)
{
	int err_cnt = 0;
	Z_die_if(!fp || !fi || !src_len || !source, "args");

	/* calculate symbol sizes */
	Z_die_if(
		ffec_calc_sym_counts(fp, src_len, &fi->cnt)
		, "");
	/* calculate region sizes */
	struct ffec_sizes sz;
	ffec_calc_lengths_int(fp, src_len, &sz, dir, &fi->cnt);

	/* assign region pointers */
	fi->source = source;
	if (parity)
		fi->parity = parity;
	else
		fi->parity = fi->source + sz.source_sz;
	if (scratch)
		fi->scratch = scratch;
	else
		fi->scratch = fi->parity + sz.parity_sz;

	/* assign pointers into scratch region */
	/* note: fi->cells is a union with fi->scratch : no need to assign it */
	fi->rows = fi->scratch + ffec_len_cells(&fi->cnt);
	/* psums only when decoding */
	if (dir == decode)
		fi->psums = ((void*)fi->rows) + ffec_len_rows(&fi->cnt);
	else
		fi->psums = NULL;
	/* zero the scratch region, set row IDs */
	memset(fi->scratch, 0x0, sz.scratch_sz);
#ifdef FFEC_DEBUG
	/* initialize row IDs for use by debug prints */
	unsigned int i;
	for (i=0; i < fi->cnt.rows; i++)
		fi->rows[i].row_id = i;
#endif
	/* If encoding, the parity region will be zeroed by the encoder
		function.
	Otherwise, zero it now.
	*/
	if (dir == decode) {
		memset(fi->parity, 0x0, sz.parity_sz);
		memset(fi->psums, 0x0, fi->cnt.rows * fp->sym_len);
	}


	/* if no seed proposed, fish from /dev/urandom */
	if (!seed1 || !seed2) {
#if 0
	/* won't work until we update Ubuntu versions and move away from EGLIBC :O */
		while (syscall(SYS_getrandom, &seed, sizeof(seed), 0))
			usleep(100000);
#else
		int fd = 0;
		Z_die_if((fd = open("/dev/urandom", O_NOATIME | O_RDONLY)) < 1, "");
		while (read(fd, &fi->seeds, sizeof(fi->seeds)) != sizeof(fi->seeds))
			usleep(100000);
		close(fd);
#endif
	} else {
		fi->seeds[0] = seed1;
		fi->seeds[1] = seed2;
	}
	/* seed random number generator
	lrand48_r() will be used to generate random numbers between 0 - 2^31.
	*/
	ffec_rand_seed(&fi->rng, fi->seeds[0], fi->seeds[1]);

#ifdef FFEC_DEBUG
	/* print values for debug */
	Z_inf(0, "\n\tseeds=[0x%lx,0x%lx]\tcnt: .k=%d .n=%d .p=%d",
		fi->seeds[0], fi->seeds[1], fi->cnt.k, fi->cnt.n, fi->cnt.p);
#endif

	/* init the matrix */
	ffec_init_matrix(fi);

out:
	return err_cnt;
}

/*	ffec_encode()
Go through an entire block and generate its repair symbols.
The main emphasis is on SEQUENTIALLY accessing source symbols,
	limiting the pattern of random memory access to the repair symbols.
*/
uint32_t	ffec_encode	(const struct ffec_params	*fp, 
				struct ffec_instance		*fi)
{
	int err_cnt = 0;
	Z_die_if(!fi, "args");

	/* zero out all parity symbols */
	memset(fi->parity, 0x0, fi->cnt.p * fp->sym_len);

	int64_t i;
	uint32_t j;
	struct ffec_cell *cell;
	void *symbol;
	for (i=0; i < fi->cnt.cols; i++) {
		/* get source symbol
		Note that ffec_xor_into_symbol() issues prefetch instructions,
			don't duplicate that here.
		*/
		cell = ffec_get_col_first(fi->cells, i);
		symbol = ffec_get_sym(fp, fi, i);
#ifdef FFEC_DEBUG
		Z_inf(0, "enc(esi %ld) @0x%lx", 
			i, (uint64_t)symbol);
#endif
		for (j=0; j < FFEC_N1_DEGREE; j++) {
			/* avoid empty cells under the staircase */
			if (ffec_cell_test(&cell[j]))
				continue;
			/* Avoid XOR-ing parity symbol with itself */
			if (i - fi->cnt.k == cell[j].row_id)
				continue;
			/* XOR into parity symbol for that row */
			ffec_xor_into_symbol(symbol, 
					ffec_get_sym_p(fp, fi, cell[j].row_id),
					fp->sym_len);
#ifdef FFEC_DEBUG
					Z_inf(0, "xor(esi %ld) -> p%d @0x%lx", 
						i, cell[j].row_id, 
						(uint64_t)ffec_get_sym_p(fp, fi, cell[j].row_id));
#endif
		}
	}

out:
	return err_cnt;
}

/* this is because I need all relavant "recursion" state represented in 64b */
typedef struct {
	union {
		uint64_t index;
	struct {
		uint32_t esi;
		uint32_t row;
	};
	};
} __attribute__ ((packed)) ffec_esi_row_t;
/*	ffec_decode_sym()
Decode a symbol:
a. XOR it into the PartialSum for all of its rows.
b. Copy it into its final location in either "source" or "repair" regions.
Note that there is no advantage to splicing since we must read
	the symbol into cache for a.) anyways.
c. If any row is now left with only one symbol, the PartialSum of that
	row IS that symbol: "iteratively decode" by recursing.

Returns number of source symbols yet to receive/decode.
A return of '0' means "all source symbols decoded".
Returns '-1' on error.

Note on recursion:
This function, if simply calling itself, can (with large blocks) recurse 
	to a point where it stack overflows.
The solution is to use a state variable 'state', which MUST be NULL
	on first invocation of the function, and which is otherwise
	used as a pointer to a J1 array which into which can be queued
	the remaining "cnt==1" rows to be decoded.
*/
uint32_t	ffec_decode_sym	(const struct ffec_params	*fp,
				struct ffec_instance		*fi,
				void				*symbol,
				uint32_t			esi)
{
	/* state for "recursion" */
	struct j_array state;
	j_init_nomutex(&state);
	ffec_esi_row_t tmp;

	/* set up only only once */
	err_cnt = 0;
	Z_die_if(!fp || !fi || !symbol, "args");
	struct ffec_cell *cell = NULL;
	void *stack_sym = alloca(fp->sym_len);

recurse:

	/* if all source symbols have been decoded, bail */
	if (fi->cnt.k_decoded == fi->cnt.k)
		goto out;

	/* get column */
	cell = ffec_get_col_first(fi->cells, esi);
	/* if this cell has been unlinked, unwind the recusion stack */
	if (ffec_cell_test(cell))
		goto check_recurse;

	/* pull symbol onto stack */
	memcpy(stack_sym, symbol, fp->sym_len);
	/* push it to its destination */
	memcpy(ffec_get_sym(fp, fi, esi), stack_sym, fp->sym_len);
#ifdef FFEC_DEBUG
	Z_inf(0, "pull <-(esi %d) @0x%lx", 
		esi, (uint64_t)symbol);
	Z_inf(0, "push ->(esi %d) @0x%lx", 
		esi, (uint64_t)ffec_get_sym(fp, fi, esi));
#endif

	/* If it's a source symbol, log it. */
	if (esi < fi->cnt.k) {
		/* We may have just finished.
		Avoid extra work.
		*/
		if (++fi->cnt.k_decoded == fi->cnt.k)
			goto out;
	}

	/* get all rows */
	unsigned int j;
	struct ffec_row *n_rows[FFEC_N1_DEGREE];
	memset(&n_rows, 0x0, sizeof(n_rows));
	for (j=0; j < FFEC_N1_DEGREE; j++) {
		/* some cells have less 1's, like the bottom
			right of the pyramid.
		*/
		if (ffec_cell_test(&cell[j])) {
			n_rows[j] = NULL;
			break;
		} else {
			n_rows[j] = &fi->rows[cell[j].row_id];
		}
		/* XOR into psum, unless we don't have to because we're done
			decoding.
		*/
		if (n_rows[j]->cnt > 1) {
			ffec_xor_into_symbol(stack_sym, 
					ffec_get_psum(fp, fi, cell[j].row_id),
					fp->sym_len);
#ifdef FFEC_DEBUG
					Z_inf(0, "xor(esi %d) -> p%d @0x%lx", 
						esi, cell[j].row_id, 
						(uint64_t)ffec_get_psum(fp, fi, cell[j].row_id));
#endif
		}
		/* remove from row */
		ffec_matrix_row_unlink(n_rows[j], &cell[j]);
	}

	/* See if any row can now be solved.
	This is done in a separate loop so that we have already removed
		our symbol from ALL rows it belongs to
		before recursing.
	*/
	for (j=0; j < FFEC_N1_DEGREE; j++) {
		if (!n_rows[j])
			continue;
		if (n_rows[j]->cnt == 1) {
			cell = n_rows[j]->last;
			tmp.esi = cell->col_id;
			tmp.row = cell->row_id;
			j_enq_(&state, tmp.index);
		}
	}

check_recurse:
	/* If any rows are queued to be solved, "recurse".
	Notice that I am populating 'tmp' with the "row index"
		which was added into the array at some unknown
		past iteration.
	*/
	tmp.index = j_deq_(&state);
	if (tmp.index) {
		/* reset stack variables */
		symbol = ffec_get_psum(fp, fi, tmp.row);
		esi = tmp.esi;
		goto recurse;
	}

out:
	j_destroy_(&state);
	if (err_cnt)
		return -1;
	return fi->cnt.k - fi->cnt.k_decoded;
}
