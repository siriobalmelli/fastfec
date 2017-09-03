#ifndef npath_h_
#define npath_h_

/*	npath.h		the Nonlibc paths library

Replacement for libc's frightening 'dirname' and 'basename' functions;
	which MAY or may NOT return the path,
	may or may not clobber said path ... sacre bleu!

These functions always return a newly allocated string which can be passed
	to free.
They are also reasonably efficient: they only traverse the string once.
*/

#include <nonlibc.h>
#include <stddef.h> /* size_t */
#include <stdio.h> /* sprintf() */
#include <stdlib.h> /* malloc() */

NLC_LOCAL void n_counts_(const char *path, size_t *out_len, size_t *out_last_sep);

NLC_PUBLIC char	*n_dirname(const char *path);
NLC_PUBLIC char	*n_basename(const char *path);
NLC_PUBLIC char *n_join(const char *dir_name, const char *base_name);

NLC_PUBLIC int	n_is_dir(const char *path);

#endif /* npath_h_ */
