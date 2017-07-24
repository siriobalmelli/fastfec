#include <zed_dbg.h>	/* include this FIRST! */
#include <stdlib.h>	/* malloc() free() */

/*	zed_dbg_test.c

Test and show usage for the print and control-flow macros in 'zed_dbg.h'

(c) 2017 Sirio Balmelli; https://b-ad.ch
*/



/*	Tell sanitizers not to freak out when we deliberately fail a malloc()
		as part of the tests below.
*/
const char *__asan_default_options()
{
	return "allocator_may_return_null=1";
}
const char *__tsan_default_options()
{
	return "allocator_may_return_null=1";
}



/*	basic_function()

In the majority of cases, a function returns an int,
	which should be 0 to indicate success.

We can easily follow this convention by declaring a local 'err_cnt' variable
	and using the Z macros to test for errors and print them
	if they occur; then just return 'err_cnt' which will take care of
	informing the caller if an error occurred.
*/
int basic_function()
{
	int err_cnt = 0;

	/* test for an error condition;
		print a message and increment error count if test evaluates to true.
	Note that readability and convention usually splits
		the test, error message and error message variables into 3 separate lines.
	*/
	Z_err_if(1 != 1,
		"this will never error; err_cnt is %d",
		err_cnt);

	Z_die_if(1, "error!");
	/* It would be an error to ever arrive here - the preceding
		error should have bailed us out by now.
	*/
	exit(1);

out:
	return err_cnt;
}



/*	Struct to illustrate usage in malloc_error()
*/
struct	malloc_err_example {
	void	*ptr_a;
	void	*ptr_b;
};

/*	malloc_error()

Show use of Z macros in a function which allocates memory.
Such a function usually returns a pointer to the allocated memory,
	NULL on error.

The goal is to graciously handle malloc() or other failures without risking
	a memory leak.
*/
struct malloc_err_example *malloc_error()
{
	/* Declare any values which will be used below 'out:'
		BEFORE any Z_die_if() calls.
	*/
	struct malloc_err_example *ret;

	/* Assign AND test in the same construct;
		give an error on fail.
	*/
	Z_die_if(( ret = malloc(sizeof(struct malloc_err_example)) ) == NULL,
		"could not malloc() %zu bytes",
		sizeof(struct malloc_err_example));

	/* Assign inner pointers - one of these allocs should fail
		because it's an insane amount of memory.
	*/
	Z_die_if(( ret->ptr_a = malloc(sizeof(uint64_t)) ) == NULL,
		"could not malloc() %zu bytes",
		sizeof(uint64_t));
	Z_die_if(( ret->ptr_b = malloc(1UL << (sizeof(long) * 8 - 1)) ) == NULL,
		"could not malloc() %lu (insane amount) bytes",
		1UL << (sizeof(long) * 8 - 1));


	/* successful return */
	return ret;
out:
	/* Bad things happened.
	Clean up any possible dirty resources (allocs, FDs, etc),
		then return failure value.

	NOTE: passing NULL to free() is perfectly permissible.
	By explicitly free()ing all pointers,
		we are certain never to leak.
	*/
	if (ret) {
		free(ret->ptr_a);
		free(ret->ptr_b);
	}
	free(ret);

	return NULL;
}



/*	sudden_death_()

Used as a hack to make a (conditional) Z_log function kill the program if
	it's evaluated.
This is the standard approach if you want
	conditional execution inside a Z_() function.
*/
int sudden_death_()
{
	exit(1);
}

/*	disabled_infos()

Disable the Z_log and Z_wrn info levels.
*/
void disabled_infos()
{
	Z_log(Z_inf, "this will print");
	Z_log(Z_wrn, "this will also print");

#undef Z_LOG_LVL
#define Z_LOG_LVL (Z_err)

	Z_log(Z_inf, "this should NOT print: %d", sudden_death_());
	Z_log(Z_wrn, "this should also NOT print: %d", sudden_death_());

#undef Z_LOG_LVL
#define Z_LOG_LVL (Z_err | Z_wrn | Z_inf)
}

/*	enabled_infos()

Enable additional logging levels.
*/
void enabled_infos()
{
	Z_log(Z_in2, "this will NOT print: %d", sudden_death_());
	Z_log(Z_wr2, "this will also NOT print: %d", sudden_death_());

#undef Z_LOG_LVL
#define Z_LOG_LVL (Z_err | Z_wrn | Z_inf | Z_wr2 | Z_in2)

	Z_log(Z_in2, "this should now print");
	Z_log(Z_wr2, "this should also print");

#undef Z_LOG_LVL
#define Z_LOG_LVL (Z_err | Z_wrn | Z_inf)
}



/*	main()
*/
int main()
{
	/* This will be incremented by any calls to
		Z_err(), Z_err_if(), Z_die(), Z_die_if()
	*/
	int err_cnt = 0;

	/* Print: the current function name as an "info" loglevel;
		'Z_inf' 'Z_wrn' and 'Z_err' levels are always enabled by default.
	*/
	Z_log(Z_inf, "current function: %s()", __FUNCTION__);

	/* Declare all variables which might be used in 'out:'
		BEFORE any Z_die_if() calls.

	Always initialize them to their "failure values".
	*/
	int res = 0;
	void *a_pointer = NULL;
	void *a_malloc = NULL;

	/* Show error handling when calling a "basic" function
		(which in the UNIX tradition returns 0 on success.
	Note we assign a return value AND test it at the same time.
	*/
	Z_die_if(( res = basic_function() ) != 1,
		"sum ting willy wong heah");
	/* printing an info */
	Z_log(Z_inf, "res is %d", res);

	/* Show error handling for a function which allocates
		(potentially multiple) memory regions.
	*/
	a_pointer = malloc_error();

	/* malloc (with safety check); then print a buffer */
	const size_t len = 125;
	Z_die_if((a_malloc = malloc(len)) == NULL,
		"len = %zu",
		len);
	Z_prn_buf(Z_inf, a_malloc, len,
		"printing buffer len %zu", len);

	/* show user-controlled enabling/disabling of prints */
	disabled_infos();
	enabled_infos();

out:
	/* Regardless of whether function succeeded or not,
		make sure all things are cleaned up.
	*/
	free(a_pointer); /* free(NULL) if perfectly OK */
	free(a_malloc);
	return err_cnt;;
}
