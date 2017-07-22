/*	nonlibc_urand.h

Hide away the annoyance of "correctly" getting urandom across various systems.
*/

#ifndef nonlibc_urand_h_
#define nonlibc_urand_h_

#include <nonlibc.h>
#include <zed_dbg.h>

#include <unistd.h>	/* read(); close() */
#include <fcntl.h>	/* open(); O_RDONLY */



/*	nlc_urand_open_()

The least-desirable (but most-portable) implementation:
	open() /dev/urandom and read()
*/
NLC_INLINE size_t nlc_urand_open(void *buf, size_t len)
{
	int fd = 0;
	size_t ret = -1;
	Z_die_if((
		fd = open("/dev/urandom", O_RDONLY)
		) < 1, "open(\"/dev/urandom\" fail");

	Z_die_if((
		ret = read(fd, buf, len)
		) != len, "ret %zu != len %zu", ret, len);
out:
	if (fd > 0)
		close(fd);
	return ret;
}



/*	define nlc_urand_() to call the proper implementation */
#if defined(__linux__)
	#include <linux/version.h>

	/* preference: use the proper syscall */
	#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,17,0)
		#ifdef HAVE_LINUX_GETRANDOM                                                  
			#include <linux/random.h>                                                   
			#define nlc_urand_(buf, len) \
				getrandom(buf, len, 0)
		#else                                                                        
			#include <sys/syscall.h>                                                    
			#define nlc_urand_(buf, len) \
				syscall(SYS_getrandom, buf, len, 0)
		#endif

	#endif     
/* TODO: BSD arc4random ? */
#endif

/* fallback: open() /dev/urandom */
#ifndef nlc_urand_
	#define nlc_urand_(buf, len) nlc_urand_open(buf, len)
#endif



/*	nlc_urand()
Get bytes from system pseudo-RNG; put them in '*buf'

Returns number of bytes written;
	may be less than requested: 0 OR -1 on error!

NOTE: 
Recommended usage is to use this function to get a SMALL amount of
	randomness from the system.
If you need a lot of random data it's preferrable (and much faster!) to
	use this function to initialize an RNG (e.g.: 'pcg_rand.h' included in this library);
	then run the RNG to get all the randomness you want.
Stated another way: use nlc_urand() just to get Initialization Vectors

NOTE:
This inline itself does nothing; it's here simply to enforce
	type correctness on the caller.
*/
NLC_INLINE	__attribute__((warn_unused_result))
		size_t	nlc_urand(void *buf, size_t len)
{
	return nlc_urand_(buf, len);
}



/* caller has no business with this hack */
#undef nlc_urand_

#endif /* nonlibc_urand_h_ */
