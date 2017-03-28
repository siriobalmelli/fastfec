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

TODO:
-	Encode a single symbol
-	Encode a range of symbols
-	Report decoding "critical path" ... aka: smallest amount of source symbols
		needed to decode.
	Simulate decode with "next missing source symbol" until decoding all source symbols.



2016: Sirio Balmelli and Anthony Soenen
*/

#ifndef _GNU_SOURCE
	#define _GNU_SOURCE	/* /dev/urandom; syscalls */
#endif
#include <unistd.h>
#include <sys/syscall.h>
#include <linux/random.h>

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

NOTE: ffec_init() will NOT perform these safety checks,
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
	Z_die_if(
		ffec_calc_lengths_int(fp, src_len, out, dir, &fc)
		, "");

out:
	return err_cnt;
}


/*	ffec_init()
Requires a pointer to a struct fec_instance rather than allocating it.
This is to allow caller to decide its location.
Corollary: we must not assume 'fi' has been zeroed: we must set
	EVERY variable.
*/
int		ffec_init(	const struct ffec_params	*fp,
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
	Z_die_if(
		ffec_calc_lengths_int(fp, src_len, &sz, dir, &fi->cnt)
		, "");

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
	/* zero the scratch region */
	memset(fi->scratch, 0x0, sz.scratch_sz);

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
#if 1
		/* requires Ubuntu 16.04, which eschews EGLIBC */
		while (syscall(SYS_getrandom, fi->seeds, sizeof(fi->seeds), 0) != sizeof(fi->seeds))
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
	pcg_rand_seed(&fi->rng, fi->seeds[0], fi->seeds[1]);

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

return 0 on success
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
/*	ffec_decode_sym_()
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

NOTE on recursion:
This function, if simply calling itself, can (with large blocks) recurse
	to a point where it stack overflows.
The solution is to allocate a j_array when entering the function,
	and then simulate recursion with a jump.

WARNING: if 'symbol' is NULL, we ASSUME it has already been copied to matrix memory
	and read it directly from ffec_get_sym(esi)

If 'j1_src_decoded' points somewhere, will append to a J1 array the ESI of all
	SOURCE symbols decoded (specifically: INCLUDING the symbol we were called to decode).
*/
uint32_t	ffec_decode_sym_	(const struct ffec_params	*fp,
					struct ffec_instance		*fi,
					void				*symbol,
					uint32_t			esi,
					void				**j1_src_decoded)
{
	/* state for "recursion" */
	struct j_array state;
	j_init_nomutex(&state);
	ffec_esi_row_t tmp;

	/* set up only only once */
	err_cnt = 0;
	Z_die_if(!fp || !fi, "args");
	struct ffec_cell *cell = NULL;
	void *curr_sym = NULL;

recurse:

	/* if all source symbols have been decoded, bail */
	if (fi->cnt.k_decoded == fi->cnt.k)
		goto out;

	/* get column */
	cell = ffec_get_col_first(fi->cells, esi);
	/* if this cell has been unlinked, unwind the recursion stack */
	if (ffec_cell_test(cell))
		goto check_recurse;

	/* point to symbol in matrix */
	curr_sym = ffec_get_sym(fp, fi, esi);
	/* if given a pointer, copy symbol from there into matrix */
	if (symbol && symbol != curr_sym) {
#ifdef FFEC_DEBUG
		Z_inf(0, "pull <-(esi %d) @0x%lx",
			esi, (uint64_t)symbol);
#endif
		memcpy(curr_sym, symbol, fp->sym_len);
	}
	/* ... otherwise assume it's already there */

#ifdef FFEC_DEBUG
	Z_inf(0, "decode (esi %d) @0x%lx",
		esi, (uint64_t)curr_sym);
#endif

	/* If it's a source symbol, log it. */
	if (esi < fi->cnt.k) {
		if (j1_src_decoded) {
			int rc;
			J1S(rc, *j1_src_decoded, esi);
		}
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
		/* if any cells are unset, avoid processing them
			and avoid processing their row.
		*/
		if (ffec_cell_test(&cell[j])) {
			n_rows[j] = NULL;
			continue;
		}
		n_rows[j] = &fi->rows[cell[j].row_id];
		/* XOR into psum, unless we don't have to because we're done
			decoding that row.
		*/
		if (n_rows[j]->cnt > 1) {
			ffec_xor_into_symbol(curr_sym,
					ffec_get_psum(fp, fi, cell[j].row_id),
					fp->sym_len);
#ifdef FFEC_DEBUG
					Z_inf(0, "xor(esi %d) -> p%d @0x%lx",
						esi, cell[j].row_id,
						(uint64_t)ffec_get_psum(fp, fi, cell[j].row_id));
#endif
		}
		/* remove from row */
		ffec_matrix_row_unlink(n_rows[j], &cell[j], fi->cells);
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
			cell = &fi->cells[n_rows[j]->c_last];
			tmp.esi = cell->c_me / FFEC_N1_DEGREE;
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


/*	ffec_esi_rand()

Populate memory at 'esi_seq' with a randomly shuffled array of all the ESIs
	in 'fi' starting from 'esi_start' (set it to 0 to obtain all symbols).
Uses Knuth-Fisher-Yates.

NOTE: 'esi_seq' should have been allocated by the caller and should be
	of size >= '(fi->cnt.n - esi_start) * sizeof(uint32_t)'

This function is used e.g.: in determining the order in which to send symbols over the wire.
*/
void		ffec_esi_rand	(const struct ffec_instance	*fi,
				uint32_t			*esi_seq,
				uint32_t			esi_start)
{
	/* derive seeds from existing instance */
	uint64_t seeds[2] = {
		(fi->seeds[1] >> 1) +1,
		(fi->seeds[0] << 1) +1
	};
	/* setup rng */
	struct pcg_rand_state rnd;
	pcg_rand_seed(&rnd, seeds[0], seeds[1]);

	/* write IDs sequentially */
	uint32_t limit = fi->cnt.n - esi_start;
	for (uint32_t i=0; i < limit; i++)
		esi_seq[i] = i + esi_start;

	/* Iterate and swap using XOR */
	uint32_t rand;
	uint32_t temp;
	for (uint32_t i=0; i < limit -1; i++) {
		rand = pcg_rand_bound(&rnd, limit -i) + i;
		/* use temp var and not triple-XOR so as to avoid XORing a cell with itself */
		temp = esi_seq[i];
		esi_seq[i] = esi_seq[rand];
		esi_seq[rand] = temp;
	}
#ifdef FFEC_DEBUG
	printf("randomized ESI sequence:\n");
	for (uint32_t i=0; i < fi->cnt.n; i++)
		printf("%d, ", esi_seq[i]);
	printf("\n");
#endif
}

/*	ffec_test_esi()
Returns 1 if 'esi' is already decoded, 0 otherwise.
*/
int		ffec_test_esi	(const struct ffec_instance	*fi,
				uint32_t			esi)
{
	/* if this cell has been unlinked, unwind the recursion stack */
	return ffec_cell_test(ffec_get_col_first(fi->cells, esi));
}


/*	ffec_mtx_cmp()
Compare 2 FEC matrices, cells and rows - which should be identical
		(because their IDs are offsets, not absolute addresses ;)

NOTE that matrices are NO LONGER identical after decode has started,
	because decoding will remove cells from the rows.

returns 0 if identical
*/
int ffec_mtx_cmp(struct ffec_instance *enc, struct ffec_instance *dec, struct ffec_params *fp)
{
	int err_cnt = 0;
	/* verify seeds */
	Z_err_if(memcmp(enc->seeds, dec->seeds, sizeof(enc->seeds)),
		"FEC seeds mismatched: enc(0x%lx, 0x%lx) != dec(0x%lx, 0x%lx)",
		enc->seeds[0], enc->seeds[1], dec->seeds[0], dec->seeds[1]);

	/* verify matrix cells */
	if (memcmp(enc->cells, dec->cells, sizeof(struct ffec_cell) * enc->cnt.k * FFEC_N1_DEGREE))
	{
		Z_err("FEC matrices mismatched");
		uint32_t mismatch_cnt=0;
		for (uint32_t i=0; i < enc->cnt.k * FFEC_N1_DEGREE; i++) {
			if (memcmp(&enc->cells[i], &dec->cells[i], sizeof(enc->cells[0])))
				mismatch_cnt++;
		}
		printf("\nmismatch %d <= %d cells\n\n", mismatch_cnt, enc->cnt.n * FFEC_N1_DEGREE);
	}

out:
	return err_cnt;
}


