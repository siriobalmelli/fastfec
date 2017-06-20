#ifndef zed_dbg_h_
#define zed_dbg_h_

/*	zed_dbg.h	control-flow and print macros for C

The purpose of this library is to provide simple macros which abstract away
	repetitive control-flow and debug/warning/info print mechanics.

NOTE: it's a good idea to always include this library FIRST,
	before any other #include statement.


Credit to Zed A. Shaw:
	The ideas for this work (very loosely) derive from his
		debug macros in "learn C the hard way".
	Thanks.

The nondescriptive name 'zed_dbg' originally began as a tongue-in-cheek
	reference to Pulp Fiction.
It ended up sticking around in equal parts because the prefix 'Z' is both
	unique and minimalistic; but also because a whole bunch of code
	grew up using this library and I'm too lazy to go back and
	change all of it.
Oh - and it lends itself to all kinds of puns. Z_end_()

(c) 2014 Sirio Balmelli; https://b-ad.ch
*/

#include <errno.h>
#include <stdio.h>
#include <libgen.h>	/* basename() */
#include <string.h>
#include <nmath.h>	/* nm_bit_pos() */
#include <inttypes.h>	/* PRIu64 etc. for reliable printf across platforms */



/* Log levels as individual bits which can be set/unset independently */
	/* error - always print this one */
	#define Z_err 0x00
	/* info levels */
	#define Z_inf 0x01
	#define Z_in2 0x02
	#define Z_in3 0x04
	#define Z_in4 0x08
	/* warning levels */
	#define Z_wrn 0x10
	#define Z_wr2 0x20
	#define Z_wr3 0x40
	#define Z_wr4 0x80
/* print text corresponding to each log level */
static char *Z_log_txt[] = {
	"ERR",
	"INF", "IN2", "IN3", "IN4",
	"WRN", "WR2", "WR3", "WR4"
};

/* 'Z_LOG_' (log level) is a define (and not a static variable)
	so that debug prints can be removed by the preprocessor
	BEFORE compilation.

In order to get custom log levels for a file,
	define 'Z_LOG_LVL' before including this library.
*/
#ifndef Z_LOG_LVL
	#ifdef NDEBUG
		#define Z_LOG_LVL (Z_inf | Z_wrn | Z_err)
	#else
		#define Z_LOG_LVL (Z_err)
	#endif
#endif



/* Formatted print function.
Can be re-defined at compile-time to use other print facilities.
*/
#ifndef Z_PRN
#define Z_PRN fprintf
#endif

/*	Z_log_()
Call to predefined Z_PRN, with formatting helper.
NOTE: if 'errno' is set, it is printed and then RESET.
*/
#define	Z_log_(STREAM, LOG_LVL, M, ...) \
	do { \
		if (Z_LOG_LVL & LOG_LVL || LOG_LVL == Z_err) { \
			Z_PRN(STREAM, "[%s] %10s:%03d +%-15s\t:: " M "\n", \
				Z_log_txt[nm_bit_pos(LOG_LVL)], \
				basename(__FILE__), __LINE__, __FUNCTION__, ##__VA_ARGS__); \
			if (errno) { \
				Z_PRN(STREAM, "\t!errno: %d|%s!\n", errno, strerror(errno)); \
				errno = 0; \
			} \
		} \
	} while (0)

/*	Z_log_line()
Print a separator line
*/
#define Z_log_line() Z_PRN(stdout, "------------\n")
#define Z_lne Z_log_line


/*	Z_log()
Standard logging function.
Allows user to specify a log level (e.g.: 'Z_in3') which MUST
	be set for the log instruction to be compiled.
This is so that logging can be selectively enabled/disabled at compile time.
*/
#define Z_log(LOG_LVL, M, ...) Z_log_(stdout, LOG_LVL, M, ##__VA_ARGS__)



/* Global tracking of errors and warnings; as a catch-all.
Usually, a function will define and check one or both of these locally.
	*/
static int err_cnt = 0;
static int wrn_cnt = 0;

/*	Z_log_wrn()
Increment 'wrn_cnt' when logging a warning.
*/
#define Z_log_wrn(M, ...) do { \
			Z_log_(stderr, Z_wrn, M, ##__VA_ARGS__); \
			wrn_cnt++; \
			} while(0)

/*	Z_log_err()
Increment 'err_cnt' when logging an error.
*/
#define Z_log_err(M, ...) do { \
			Z_log_(stderr, Z_err, M, ##__VA_ARGS__); \
			err_cnt++; \
			} while(0)

/*	Z_die()
Log error, then goto 'out'
*/
#define Z_die(M, ...) do { \
			Z_log_(stderr, Z_err, M, ##__VA_ARGS__); \
			err_cnt++; \
			goto out; \
		} while(0)

/*	CONDITIONALS
*/
#define Z_wrn_if(A, M, ...) if (A) { Z_log_warn("(" #A ") " M, ##__VA_ARGS__); }
#define Z_err_if(A, M, ...) if (A) { Z_log_err("(" #A ") " M, ##__VA_ARGS__); }
#define Z_die_if(A, M, ...) if (A) { Z_die("(" #A ") " M, ##__VA_ARGS__); }



/*	Z_start_()
If we are using printf, set I/O to always be line-buffered:
	we likely want to be able to tail log files effectively.
*/
#if Z_PRN == fprintf
static void __attribute__ ((constructor)) Z_start_()
{
	setvbuf(stdout, NULL, _IOLBF, 0);
	setvbuf(stderr, NULL, _IOLBF, 0);
}
#endif



/*	Z_end_()

Report errors at program close.
Static because we want every translation unit to run this separately
*/
static void __attribute__ ((destructor)) Z_end_()
{
	if (err_cnt)
		Z_log_(stderr, Z_err, "%s; global err_cnt %d", __BASE_FILE__, err_cnt);
	if (wrn_cnt)
		Z_log_(stderr, Z_wrn, "%s; global wrn_cnt %d", __BASE_FILE__, wrn_cnt);
}



/*	Z_prn_buf()
Print a bunch of bytes, properly formatted.
*/
#define Z_prn_buf(BUF, LEN) \
	do { \
		char *b = (char*)BUF; \
		size_t __i; \
		for (__i = 0; __i < LEN; __i++) { \
			/* == "__i % 8" */ \
			if (!(__i & 0x7)) \
				Z_PRN(stdout, "\n"); \
			Z_PRN(stdout, "0x%hhx\t", b[__i]); \
		} \
		Z_PRN(stdout, "\n"); \
		Z_log_line(); \
	} while(0)

#endif
