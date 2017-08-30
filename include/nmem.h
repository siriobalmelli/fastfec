#ifndef nmem_h_
#define nmem_h_

/*	nmem.h		the Nonlibc Memory library

Library of functions to handle zero-copy I/O
*/

#define _GNU_SOURCE         /* See feature_test_macros(7) */
#include <fcntl.h> /* splice() */
#include <unistd.h> /* pipe() */
#include <sys/uio.h>
#include <sys/mman.h>
#include <sys/stat.h> /* umask() */

#include <sys/syscall.h>
#include <linux/memfd.h>

#include <stdint.h> /* uint{x}_t */
#include <nonlibc.h>

#define NMEM_PERMS (mode_t)(S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP)
#define NMEM_SPLICE_FLAGS ( SPLICE_F_GIFT | SPLICE_F_MOVE )

struct nmem {
	int32_t		fd;
	uint32_t	o_flags; /* only open() flags valid here */
union {
	struct {
		void	*mem;
		size_t	len;
	};
	struct iovec	iov;
};
};

/* TODO: speed comparisons:
For given block sizes:
-	malloc
-	open(O_TMPFILE)
-	memfd()
*/
NLC_PUBLIC int		nmem_file(char *path, struct nmem *out);
NLC_PUBLIC int		nmem_alloc(size_t len, const char *tmp_dir, struct nmem *out);
NLC_PUBLIC void		nmem_free(struct nmem *nm, char *deliver_path);

NLC_PUBLIC size_t	nmem_in_splice(struct nmem	*nm,
					size_t		offset,
					size_t		len,
					int		fd_pipe_from);

NLC_PUBLIC size_t	nmem_out_splice(struct nmem	*nm,
					size_t		offset,
					size_t		len,
					int		fd_pipe_to);

#endif /* nmem_h_ */
