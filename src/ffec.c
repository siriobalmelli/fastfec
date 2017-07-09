/*	ffec.c
*/

#include <ffec.h>
#include <math.h> /* ceill() */



/*	ffec_calc_lengths()
Calculate the needed memory lengths which the caller must
	allocate so FEC can operate.
returns 0 if parameters seem copacetic.

NOTE: ffec_init() will NOT perform these safety checks,
	as it assumes the caller has called ffec_calc_lengths() first
	AND CHECKED THE RESULT.
The reason for this approach is that the CALLER must allocate memory as desired:
	they must use ffec_calc_lengths() to know the required size.
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
		ffec_calc_sym_counts_(fp, src_len, &fc)
		, "");

	/* use counts to calc lengths */
	Z_die_if(
		ffec_calc_lengths_int_(fp, src_len, out, dir, &fc)
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
		ffec_calc_sym_counts_(fp, src_len, &fi->cnt)
		, "");
	/* calculate region sizes */
	struct ffec_sizes sz;
	Z_die_if(
		ffec_calc_lengths_int_(fp, src_len, &sz, dir, &fi->cnt)
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
		ffec_rand_seed_(fi->seeds);
	} else {
		fi->seeds[0] = seed1;
		fi->seeds[1] = seed2;
	}
	/* seed random number generator
	lrand48_r() will be used to generate random numbers between 0 - 2^31.
	*/
	pcg_seed(&fi->rng, fi->seeds[0], fi->seeds[1]);

#ifdef FFEC_DEBUG
	/* print values for debug */
	Z_log(Z_inf, "\n\tseeds=[0x%lx,0x%lx]\tcnt: .k=%d .n=%d .p=%d",
		fi->seeds[0], fi->seeds[1], fi->cnt.k, fi->cnt.n, fi->cnt.p);
#endif

	/* init the matrix */
	ffec_gen_matrix_(fi);

out:
	return err_cnt;
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





/*	ffec_calc_sym_counts_()
Calculate the symbol counts for a given source length and FEC params.
Populates them into 'fc', which must be allocated by the caller.

returns:
	0 if counts are valid 
	>0 if symbol math required some adjustment to values:
		caller is strongly advised to double-check sizes (including src_len!)
	<0 on failure (aka: will overflow internal math, cannot proceed)
*/
int		ffec_calc_sym_counts_(const struct ffec_params	*fp,
					size_t			src_len,
					struct ffec_counts	*fc)
{
	int err_cnt = 0;
	int wrn_cnt = 0;
	Z_die_if(!fp || !src_len || !fc, "args");
	/* temp 64-bit counters, to test for overflow */
	uint64_t t_k, t_n, t_p;

	t_k = nm_div_ceil(src_len, fp->sym_len);
	if (t_k < FFEC_MIN_K) {
		Z_log(Z_wrn, "k=%ld < FFEC_MIN_K=%d;	src_len=%ld, sym_len=%d",
			t_k, FFEC_MIN_K, src_len, fp->sym_len);
		t_k = FFEC_MIN_K;
	}
	Z_wrn_if(t_k * fp->sym_len > src_len,
		"symbol math requires larger source region: %ld > %ld (specified)",
		t_k * fp->sym_len, src_len);

	t_n = ceill(t_k * fp->fec_ratio);

	t_p = t_n - t_k;
	if (t_p < FFEC_MIN_P) {
		Z_log(Z_wrn, "p=%ld < FFEC_MIN_P=%d;	k=%ld, fec_ratio=%lf",
			t_p, FFEC_MIN_P, t_k, fp->fec_ratio);
		t_p = FFEC_MIN_P;
		t_n = t_p + t_k;
	}

	/* sanity check: matrix counters and iterators are all 32-bit */
	Z_die_if(
		t_n * FFEC_N1_DEGREE > (uint64_t)UINT32_MAX -2,
		"n=%ld symbols is excessive for this implementation",
		t_n);
	/* assign everything */
	fc->n = t_n;
	fc->k = t_k;
	fc->p = t_p;
	fc->k_decoded = 0; /* because common sense */

out:
	if (err_cnt)
		return 0 - err_cnt;
	return wrn_cnt;
}


/*	ffec_calc_lengths_int_()
Calculates lengths for callers who already have symbol counts.
Internal function, so no sanity checking performed.
*/
int		ffec_calc_lengths_int_(const struct ffec_params	*fp,
				size_t				src_len,
				struct ffec_sizes		*out,
				enum ffec_direction		dir,
				struct ffec_counts		*fc)
{
	int err_cnt = 0;

	/* do all maths in 64-bit, so we can avoid overflow */
	uint64_t src, par, scr;
	src = (uint64_t)fc->k * fp->sym_len;
	par = (uint64_t)fc->p * fp->sym_len;
	scr = (uint64_t)ffec_len_cells(fc) + ffec_len_rows(fc);
	/* if decoding, scratch must have space for psums */
	if (dir == decode)
		scr += fp->sym_len * fc->rows;

	/* combined size must not exceed UINT32_MAX */
	Z_die_if(src + par + scr > (uint64_t)UINT32_MAX -2,
		"cannot handle combined symbol space of %ld",
		src + par + scr);

	out->source_sz = src;
	out->parity_sz = par;
	out->scratch_sz = scr;

out:
	return err_cnt;
}
