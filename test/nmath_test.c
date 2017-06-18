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
	Z_err_if(nm_div_ceil(42, 7) != 6, "%ld", nm_div_ceil(42, 7));
	/* divides unevenly: expect division result +1 */
	Z_err_if(nm_div_ceil(10, 6) != 2, "%ld", nm_div_ceil(10, 6));
	/* divides into 0: expect 1 */
	Z_err_if(nm_div_ceil(5, 9) != 1, "%ld", nm_div_ceil(5, 9));

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
	Z_err_if(nm_next_pow2(0) != 0, "%d", nm_next_pow2(0));
	Z_err_if(nm_next_pow2(1) != 1, "%d", nm_next_pow2(1));
	Z_err_if(nm_next_pow2(64) != 64, "%d", nm_next_pow2(64));

	/* non-power-of-2 values should be promoted to the next power of 2 */
	Z_err_if(nm_next_pow2(33) != 64, "%d", nm_next_pow2(33));
	Z_err_if(nm_next_pow2(63) != 64, "%d", nm_next_pow2(63));
	
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
	Z_err_if(nm_next_mult32(503, 503) != 503, "%d", nm_next_mult32(503, 503));	
	Z_err_if(nm_next_mult64(3, 3) != 3, "%ld", nm_next_mult64(3, 3));	
	Z_err_if(nm_next_mult64(42, 42) != 42, "%ld", nm_next_mult64(42, 42));	

	/* divides evenly (prime, odd, even) */
	Z_err_if(nm_next_mult32(503 * 2, 503) != 503 * 2, "%d", nm_next_mult32(503 * 2, 503));	
	Z_err_if(nm_next_mult64(3 * 3, 3) != 3 * 3, "%ld", nm_next_mult64(3 * 3, 3));	
	Z_err_if(nm_next_mult64(42 * 4, 42) != 42 * 4, "%ld", nm_next_mult64(42 * 4, 42));	

	/* divides unevenly (prime, odd, even) */
	Z_err_if(nm_next_mult32(503 +1, 503) != 503 * 2, "%d", nm_next_mult32(503 +1, 503));	
	Z_err_if(nm_next_mult64(3 +2, 3) != 3 * 2, "%ld", nm_next_mult64(3 +2, 3));	
	Z_err_if(nm_next_mult64(42 +3, 42) != 42 * 2, "%ld", nm_next_mult64(42 +3, 42));	

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

	return err_cnt;
}
