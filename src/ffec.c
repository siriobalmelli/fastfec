/*	ffec.c
*/

#include <ffec.h>
#include <math.h> /* ceill() */
#include <nlc_urand.h>



/*	ffec_new()

Allocate a new ffec struct.

If 'src' is given we ASSUME this is an ENCODE struct,
	and will simply point to it (NOT copy, NOT splice).
This means caller MUST keep 'src' around for the lifetime of
	this ffec_instance (aka: until ffec_free() is called).
Otherwise, this is a DECODE struct and we will allocate 'src_len' memory
	for use in decoding.

'src_len' is the data length being encoded/decoded, and MUST be padded to
	a multiple of 'fp->sym_len'.
Failure to do so will throw an error.

'seed1' and 'seed2' are the PRNG seeds used to construct the matrix,
	if they are '0' we will read them from system entropy with nlc_urand().
They MUST be same at ENCODE and DECODE; the custom is to:
-	init ENCODE with 'seed1 = 0' and 'seed2 = 0'
-	transmit seeds to decoder
-	init DECODE with seeds from encoder
*/
struct ffec_instance	*ffec_new(const struct ffec_params	*fp,
					size_t			src_len,
					const void		*src,
					uint64_t		seed1,
					uint64_t		seed2)
{
	struct ffec_instance *ret = NULL;
	Z_die_if(!fp, "expecting ffec_params");
	Z_die_if(!src_len, "src_len mandatory");

	Z_die_if(!( 
		ret = calloc(1, sizeof(struct ffec_instance))
		), "calloc(1, %zu)", sizeof(struct ffec_instance));
	Z_die_if(!(
		ret->stk = stack_new()
		), "");


	/* if 'src' is passed; we ASSUME ENCODE */
	if (!src)
		ret->enc_source = src;
	ret->source_len = src_len;

	/* alignment */
	Z_die_if((fp->sym_len / FFEC_SYM_ALIGN * FFEC_SYM_ALIGN) != fp->sym_len,
		"requested sym_len %"PRIu32" not a multiple of %"PRIu32,
		fp->sym_len, FFEC_SYM_ALIGN);
	/* calculate symbol counts */
	Z_die_if(
		ffec_calc_sym_counts_(fp, ret->source_len, &ret->cnt)
		, "");
	/* calculate memory region sizes */
	Z_die_if(
		ffec_calc_lengths_(fp, ret)
		, "");


	/* DECODE: alloc ALL memory */
	if (!ret->enc_source) {
		size_t alloc = ret->source_len + ret->parity_len + ret->scratch_len;
		Z_die_if(!(
			ret->dec_source = malloc(alloc)
			), "alloc %zu", alloc);
		ret->parity = ret->dec_source + ret->source_len;

	/* ENCODE: alloc parity and scratch only */
	} else {
		size_t alloc = ret->parity_len + ret->scratch_len;
		Z_die_if(!(
			ret->parity = malloc(alloc)
			), "alloc %zu", alloc);
	}
	ret->scratch = ret->parity + ret->parity_len;


	/* Assign pointers into scratch region.
	NOTE: ret->cells is a union with ret->scratch : no need to assign it.
	*/
	ret->rows = ret->scratch + ffec_len_cells(&ret->cnt);
	/* psums only when decoding */
	if (ret->dec_source)
		ret->psums = ((void*)ret->rows) + ffec_len_rows(&ret->cnt);
	/* zero the scratch region */
	memset(ret->scratch, 0x0, ret->scratch_len);


	/* If encoding, the parity region will be zeroed by the encoder
		function.
	Otherwise, zero it now.
	*/
	if (ret->dec_source) {
		memset(ret->parity, 0x0, ret->parity_len);
		memset(ret->psums, 0x0, ret->cnt.rows * fp->sym_len);
	}


	/* if no seed proposed, fish from /dev/urandom */
	if (!seed1 || !seed2) {
		Z_die_if(nlc_urand(ret->seeds, sizeof(ret->seeds))
				!= sizeof(ret->seeds), "");
	} else {
		ret->seeds[0] = seed1;
		ret->seeds[1] = seed2;
	}
	/* seed random number generator
	lrand48_r() will be used to generate random numbers between 0 - 2^31.
	*/
	pcg_seed(&ret->rng, ret->seeds[0], ret->seeds[1]);


	/* print values for debug */
	Z_log(Z_in2, "\n\tseeds=[0x%"PRIu64",0x%"PRIu64"]\tcnt: .k=%"PRIu32" .n=%"PRIu32" .p=%"PRIu32,
		ret->seeds[0], ret->seeds[1], ret->cnt.k, ret->cnt.n, ret->cnt.p);


	/* init the matrix */
	ffec_gen_matrix_(ret);


	return ret;
out:
	ffec_free(ret);
	return NULL;
}



/*	ffec_free()
Any value returned from ffec_new() MUST be freed with this function.
*/
void			ffec_free(struct ffec_instance		*fi)
{
	if (!fi)
		return;

	/* symbol regions */
	if (fi->dec_source)
		free(fi->dec_source);
	else if (fi->parity)
		free(fi->parity);

	/* stack */
	stack_free(fi->stk);
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
		Z_log(Z_wrn, "k=%"PRIu64" < FFEC_MIN_K=%"PRIu32";  src_len=%zu, sym_len=%"PRIu32,
			t_k, FFEC_MIN_K, src_len, fp->sym_len);
		t_k = FFEC_MIN_K;
	}
	Z_wrn_if(t_k * fp->sym_len > src_len,
		"symbol math requires larger source region: %"PRIu64" > %zu (specified)",
		t_k * fp->sym_len, src_len);

	t_n = ceill(t_k * fp->fec_ratio);

	t_p = t_n - t_k;
	if (t_p < FFEC_MIN_P) {
		Z_log(Z_wrn, "p=%"PRIu64" < FFEC_MIN_P=%"PRIu32";  k=%"PRIu64", fec_ratio=%lf",
			t_p, FFEC_MIN_P, t_k, fp->fec_ratio);
		t_p = FFEC_MIN_P;
		t_n = t_p + t_k;
	}

	/* sanity check: matrix counters and iterators are all 32-bit */
	Z_die_if(
		t_n * FFEC_N1_DEGREE > (uint64_t)UINT32_MAX -2,
		"n=%"PRIu64" symbols is excessive for this implementation",
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



/*	ffec_calc_lengths_()
Calculates lengths based on symbol counts.
Internal function: no sanity checking performed.
*/
int		ffec_calc_lengths_(const struct ffec_params	*fp,
					struct ffec_instance	*fi)
{
	int err_cnt = 0;

	/* do all maths in 64-bit, so we can avoid overflow */
	uint64_t src, par, scr;
	src = (uint64_t)fi->cnt.k * fp->sym_len;
	par = (uint64_t)fi->cnt.p * fp->sym_len;
	scr = (uint64_t)ffec_len_cells(&fi->cnt) + ffec_len_rows(&fi->cnt);
	/* if decoding, scratch must have space for psums */
	if (!fi->enc_source)
		scr += fp->sym_len * fi->cnt.rows;

	/* combined size must not exceed UINT32_MAX */
	Z_die_if(src + par + scr > (uint64_t)UINT32_MAX -2,
		"cannot handle combined symbol space of %"PRIu64,
		src + par + scr);

	fi->source_len = src;
	fi->parity_len = par;
	fi->scratch_len = scr;

out:
	return err_cnt;
}
