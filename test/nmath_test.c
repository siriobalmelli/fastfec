/*	nmath_test.c

Test and demonstration routines for the nmath set of functions.
These are annoying little math problems that didn't seem to have
	adequate solutions in existing libraries.
*/

#include <nmath.h>
#include <zed_dbg.h>


/*	test_nm_div_ceil()

64-bit integer "ceiling" operation.
If 'b' divides evenly into 'a', returns 'a / b'.
Else, returns 'a / b + 1'.
*/
int test_nm_div_ceil()
{
	int err_cnt = 0;

	/* divides evenly */
	Z_err_if(nm_div_ceil(42, 7) != 6, "%"PRIu64, nm_div_ceil(42, 7));
	/* divides unevenly: expect division result +1 */
	Z_err_if(nm_div_ceil(10, 6) != 2, "%"PRIu64, nm_div_ceil(10, 6));
	/* divides into 0: expect 1 */
	Z_err_if(nm_div_ceil(5, 9) != 1, "%"PRIu64, nm_div_ceil(5, 9));

	return err_cnt;
}


/*	test_nm_next_pow2()

Returns next higher power of 2, or itself if already power of 2.
*/
int test_nm_next_pow2()
{
	int err_cnt = 0;

	/* A valid power of 2 should return itself.
	Note that 0 and 1 are actually valid powers of 2 ;)
	*/
	Z_err_if(nm_next_pow2_32(0) != 0, "%"PRIu32, nm_next_pow2_32(0));
	Z_err_if(nm_next_pow2_32(1) != 1, "%"PRIu32, nm_next_pow2_32(1));
	Z_err_if(nm_next_pow2_32(64) != 64, "%"PRIu32, nm_next_pow2_32(64));

	/* non-power-of-2 values should be promoted to the next power of 2 */
	Z_err_if(nm_next_pow2_32(33) != 64, "%"PRIu32, nm_next_pow2_32(33));
	Z_err_if(nm_next_pow2_32(63) != 64, "%"PRIu32, nm_next_pow2_32(63));
	
	/* Repeat checks for 64-bit version.
	*/
	Z_err_if(nm_next_pow2_64(0) != 0, "%"PRIu64, nm_next_pow2_64(0));
	Z_err_if(nm_next_pow2_64(1) != 1, "%"PRIu64, nm_next_pow2_64(1));
	Z_err_if(nm_next_pow2_64(64) != 64, "%"PRIu64, nm_next_pow2_64(64));
	Z_err_if(nm_next_pow2_64(33) != 64, "%"PRIu64, nm_next_pow2_64(33));
	Z_err_if(nm_next_pow2_64(63) != 64, "%"PRIu64, nm_next_pow2_64(63));

	/* check very large numbers */
	Z_err_if(nm_next_pow2_64(UINT64_MAX >> 1) != 0x8000000000000000,
			"%"PRIu64, nm_next_pow2_64(UINT64_MAX >> 1));
	Z_err_if(nm_next_pow2_64(UINT64_MAX) != 0, "%"PRIu64, nm_next_pow2_64(UINT64_MAX));

	return err_cnt;
}


/*	test_nm_next_mult()

Returns `x` if `x` divides evenly into `mult`
Else returns next multiple of mult above x
*/
int test_nm_next_mult()
{
	int err_cnt = 0;

	/* exact (prime, odd, even) */
	Z_err_if(nm_next_mult32(503, 503) != 503, "%"PRIu32, nm_next_mult32(503, 503));	
	Z_err_if(nm_next_mult64(3, 3) != 3, "%"PRIu64, nm_next_mult64(3, 3));	
	Z_err_if(nm_next_mult64(42, 42) != 42, "%"PRIu64, nm_next_mult64(42, 42));	

	/* divides evenly (prime, odd, even) */
	Z_err_if(nm_next_mult32(503 * 2, 503) != 503 * 2, "%"PRIu32, nm_next_mult32(503 * 2, 503));	
	Z_err_if(nm_next_mult64(3 * 3, 3) != 3 * 3, "%"PRIu64, nm_next_mult64(3 * 3, 3));	
	Z_err_if(nm_next_mult64(42 * 4, 42) != 42 * 4, "%"PRIu64, nm_next_mult64(42 * 4, 42));	

	/* divides unevenly (prime, odd, even) */
	Z_err_if(nm_next_mult32(503 +1, 503) != 503 * 2, "%"PRIu32, nm_next_mult32(503 +1, 503));	
	Z_err_if(nm_next_mult64(3 +2, 3) != 3 * 2, "%"PRIu64, nm_next_mult64(3 +2, 3));	
	Z_err_if(nm_next_mult64(42 +3, 42) != 42 * 2, "%"PRIu64, nm_next_mult64(42 +3, 42));	

	return err_cnt;
}


/*	test_nm_bit_pos()

Returns the index (1-based!) of lowest set bit in 'uint'.
If no bits are set, returns 0.
*/
int test_nm_bit_pos()
{
	int err_cnt = 0;

	/* no bits set should return 0 */
	Z_err_if(nm_bit_pos(0x0) != 0, "%"PRIuFAST8, nm_bit_pos(0x0));
	/* first bit set for both instances */
	Z_err_if(nm_bit_pos(0x1) != 1, "%"PRIuFAST8, nm_bit_pos(0x1));
	Z_err_if(nm_bit_pos(0x3) != 1, "%"PRIuFAST8, nm_bit_pos(0x3));
	/* MSb set */
	Z_err_if(nm_bit_pos(0x8000000000000000) != 64,
		"%"PRIuFAST8, nm_bit_pos(0x8000000000000000));

	/* example: use nm_bit_pos() to index into an array of prints */
	enum en {
		en_none = 0x0,
		en_one,
		en_two
	};
	const char *prints[] = {
		"en_none", "en_one", "en_two"
	};
	Z_err_if(strcmp("en_one", prints[nm_bit_pos(en_one)]), "");

	/* example: compute the shift necessary to multiply by a variable which
		is a power of 2.
	*/
	size_t pow2 = 0x80;
	Z_err_if((size_t)1 << (nm_bit_pos(pow2) -1) != pow2,
		"%zu != %zu", (size_t)1 << (nm_bit_pos(pow2) -1), pow2);

	return err_cnt;
}


/*	main()
*/
int main()
{
	int err_cnt = 0;

	err_cnt += test_nm_div_ceil();
	err_cnt += test_nm_next_pow2();
	err_cnt += test_nm_next_mult();
	err_cnt += test_nm_bit_pos();

	return err_cnt;
}
