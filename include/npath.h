#ifndef npath_h_
#define npath_h_

/*	npath.h		the Nonlibc paths library

A set of non-horrendous (that means you, libc) functions to handle
	string manipulation specific to paths and filenames.
*/

#include <stdint.h> /* uint{x}_t */

char	*n_dirname(const char *path);
char	*n_basename(const char *path);

#endif /* npath_h_ */
