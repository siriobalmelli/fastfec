/***
* zed_dbg.h
* by: Sirio Balmelli, 2014
* http://b-ad.ch
*
* credit to Zed A. Shaw
* the ideas for this work (very loosely) derive from his
* debug macros in "learn C the hard way"
* thanks
*/
#ifndef zed_dbg_h_
#define zed_dbg_h_

#include <errno.h>
#include <stdio.h>
#include <string.h>

 /* POSIX-style basename */
#include <libgen.h>
#include <string.h>

/* a common define seen in library headers */
#define	Z_INL_FORCE static inline __attribute__((always_inline))

#ifndef Z_STR_OK
#define Z_STR_OK stdout
#endif
#ifndef Z_STR_ERR
#define Z_STR_ERR stderr
#endif

/* Global log level. We probably want to see Z_LOG_LVL as a -D in the Makefile */
#ifndef Z_LOG_LVL
#define Z_LOG_LVL 1
#endif
/* Z_inf levels:
	0 = (always print) stats, ending err_cnt
	1 = signals, thread start/end
	2 = job/file/session start/end
	3 = object creation/destruction, counter details
	4 = detailed prints, interesting conditionals
	5 = nitpick-du-jour
	----- [above this, specific subsystems] ----
	6 = threading
	7 = shard
	8 = judy
	9 = [satan?]
TODO: this scheme needs to change into a bitmask-based idea, rather than sequential ;)
 */


/* block-scope log level (supersedes global level)
	we may see sections of code nestled between Z_BLK_LVL blocks
	*/
#ifndef Z_BLK_LVL
#define Z_BLK_LVL 0
#endif
/* put this at the start of any block to force the debug level */
/*
#ifdef Z_BLK_LVL
#undef Z_BLK_LVL
#endif
#define Z_BLK_LVL 2
*/
/* put this at the footer of any such section */
/*
#undef Z_BLK_LVL
#define Z_BLK_LVL 0
*/

/* conditional execution based on loglevel */
#define Z_DO(LVL, BLOCK) \
	do { \
		if ((Z_LOG_LVL > Z_BLK_LVL ? Z_LOG_LVL : Z_BLK_LVL) >= LVL) { \
			BLOCK \
		} \
	} while(0)

/* redefine this for database inserts, for example? */
#ifndef Z_PRN
#define Z_PRN fprintf
#endif

/* call to (we assume) fprintf */
#define	_log(STREAM, T, M, ...) \
	do { \
		if (errno) { \
			Z_PRN(STREAM, "["T"] %10s:%03d -!%d|%s!- [%s] :: " M "\n", \
				basename(__FILE__), __LINE__, errno, strerror(errno), \
				__FUNCTION__, ##__VA_ARGS__); \
			errno = 0; \
		} else { \
			Z_PRN(STREAM, "["T"] %10s:%03d +%-15s\t:: " M "\n", \
				basename(__FILE__), __LINE__, __FUNCTION__, ##__VA_ARGS__); \
		} \
	} while (0)

#define Z_log_line() Z_PRN(Z_STR_OK, "------------\n")
#define Z_lne Z_log_line

/* conditional logging, based on log level */
#define Z_inf(LVL, M, ...) Z_DO(LVL, \
				if (LVL) \
					_log(Z_STR_OK, "I:" #LVL, M, ##__VA_ARGS__); \
				else \
					_log(Z_STR_OK, "INF", M, ##__VA_ARGS__); \
				)
#define Z_log_info(M, ...) { Z_inf(0, M, ##__VA_ARGS__); }

#define Z_log_warn(M, ...) do { \
			_log(Z_STR_OK, "WRN", M, ##__VA_ARGS__); \
			wrn_cnt++; \
			} while(0)
#define Z_wrn Z_log_warn

/* global definitions
	in case a function does not care to override it locally
	*/
static int err_cnt = 0;
static int wrn_cnt = 0;

#define Z_log_err(M, ...) do { \
			_log(Z_STR_ERR, "ERR", M, ##__VA_ARGS__); \
			err_cnt++; \
			} while(0)
#define Z_err Z_log_err

/*	Set I/O to always be line-buffered:
If we are writing to files we likely want to be able to tail them effectively.
	*/
static void __attribute__ ((constructor)) Z_start_()
{
	setvbuf(stdout, NULL, _IOLBF, 0);
	setvbuf(stderr, NULL, _IOLBF, 0);
}

/* report errors at program close
	static because we want every library to run this separately
	*/
static void __attribute__ ((destructor)) Z_end_()
{
	if (err_cnt)
		_log(Z_STR_ERR, __BASE_FILE__, "err_cnt == %d", err_cnt);
	if (wrn_cnt)
		_log(Z_STR_OK, __BASE_FILE__, "wrn_cnt == %d", wrn_cnt);
}

#define Z_bail(M, ...) do {\
			_log(Z_STR_OK, "BAIL", M, ##__VA_ARGS__); \
			wrn_cnt++; \
			goto out; \
		} while (0)

#define Z_die(M, ...) do { \
			_log(Z_STR_ERR, "DIE", M, ##__VA_ARGS__); \
			err_cnt++; \
			goto out; \
		} while(0)

/* function checks */
#define Z_warn_if(A, M, ...) if (A) { Z_log_warn("(" #A ") " M, ##__VA_ARGS__); }
#define Z_err_if(A, M, ...) if (A) { Z_log_err("(" #A ") " M, ##__VA_ARGS__); }
#define Z_bail_if(A, M, ...) if (A) { Z_bail("(" #A ") " M, ##__VA_ARGS__); }
#define Z_die_if(A, M, ...) if (A) { Z_die("(" #A ") " M, ##__VA_ARGS__); }

#define Z_prn_buf(BUF, LEN) \
	do { \
		char *b = (char*)BUF; \
		size_t __i; \
		for (__i = 0; __i < LEN; __i++) { \
			/* == "__i % 8" */ \
			if (!(__i & 0x7)) \
				Z_PRN(Z_STR_OK, "\n"); \
			Z_PRN(Z_STR_OK, "0x%hhx\t", b[__i]); \
		} \
		Z_PRN(Z_STR_OK, "\n"); \
		Z_log_line(); \
	} while(0)

/*	EXAMPLES
 */
/* this is a one-liner that prints function name:
	Z_inf(9, "%s()", __FUNCTION__);
	*/

#endif
