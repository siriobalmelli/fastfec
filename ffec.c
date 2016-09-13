/* /dev/urandom */
#ifndef _GNU_SOURCE
	#define _GNU_SOURCE
#endif
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "ffec_int.h"

/*	ffec_calc_lengths()
Calculate the needed memory lengths which the caller must
	allocate so FEC can operate.
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
				uint32_t			seed)
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
	if (!seed) {
#if 0
	/* won't work until we update Ubuntu versions and move away from EGLIBC :O */
		while (syscall(SYS_getrandom, &seed, sizeof(seed), 0))
			usleep(100000);
#else
		int fd = 0;
		Z_die_if((fd = open("/dev/urandom", O_NOATIME | O_RDONLY)) < 1, "");
		while (read(fd, &seed, sizeof(seed)) != sizeof(seed))
			usleep(100000);
		close(fd);
#endif
	}
	fi->seed = seed;
	/* seed rand48
	lrand48_r() will be used to generate random numbers between 0 - 2^31.
	*/
	srand48_r(seed, &fi->rand_buf);

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

	unsigned int i, j;
	struct ffec_cell *cell;
	void *symbol;
	for (i=0; i < fi->cnt.cols; i++) {
		/* get source symbol
		Note that ffec_xor_into_symbol() issues prefetch instructions,
			don't duplicate that here.
		*/
		cell = ffec_get_col_first(fi->cells, i);
		symbol = ffec_get_sym(fp, fi, i);
		for (j=0; j < FFEC_N1_DEGREE; j++) {
			/* Avoid XOR-ing parity symbol with itself */
			if (i == cell[j].row_id)
				continue;
			/* XOR into parity symbol for that row */
			ffec_xor_into_symbol(symbol, 
					ffec_get_sym_p(fp, fi, cell[j].row_id),
					fp->sym_len);
		}
	}

out:
	return err_cnt;
}

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
*/
uint32_t	ffec_decode_sym	(const struct ffec_params	*fp,
				struct ffec_instance		*fi,
				void				*symbol,
				uint32_t			esi)
{
	err_cnt = 0;
	Z_die_if(!fp || !fi || !symbol, "args");

	/* get column */
	struct ffec_cell *cell = ffec_get_col_first(fi->cells, esi);
	/* If symbol has already been decoded, 
		or all source symbols already decoded,
		bail.
	*/
	if (!cell[0].row_id || fi->cnt.k_decoded == fi->cnt.k)
		goto out;

	/* pull symbol onto stack */
	void *stack_sym = alloca(fp->sym_len);
	memcpy(stack_sym, symbol, fp->sym_len);
	/* push it to its destination */
	memcpy(ffec_get_sym(fp, fi, esi), stack_sym, fp->sym_len);

	/* If it's a source symbol, log it. */
	if (esi < fi->cnt.k) {
		/* We may have just finished.
		Avoid extra work.
		*/
		if (++fi->cnt.k_decoded == fi->cnt.k)
			return 0;
	}

	/* get all rows */
	unsigned int j;
	struct ffec_row *n_rows[FFEC_N1_DEGREE];
	for (j=0; j < FFEC_N1_DEGREE; j++) {
		/* some cells have less 1's, like the bottom
			right of the pyramid.
		*/
		if (!cell[j].row_id)
			break;
		n_rows[j] = &fi->rows[cell[j].row_id];
		/* XOR into psum */
		ffec_xor_into_symbol(	stack_sym, 
					ffec_get_psum(fp, fi, cell[j].row_id),
					fp->sym_len);
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
			break;
		if (n_rows[j]->cnt == 1) {
			cell = n_rows[j]->first;
			ffec_decode_sym(fp, fi, 
					ffec_get_psum(fp, fi, cell->row_id),
					cell->col_id);
		}
	}

out:
	if (err_cnt)
		return -1;
	return fi->cnt.k - fi->cnt.k_decoded;
}
