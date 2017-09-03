#include <nmem.h>
#include <zed_dbg.h>

/*	nmem.c		platform-independent portions of nmem implementation
*/


/*	nmem_file()
Map a file at 'path' read-only.
Populate '*out'.
Returns 0 on success.
*/
int		nmem_file(const char *path, struct nmem *out)
{
	int err_cnt = 0;
	Z_die_if(!path || !out, "args");

	/* open source file
	This requires that mmap () protection also be read-only.
	*/
	out->o_flags = O_RDONLY;
	Z_die_if((
		out->fd = open(path, out->o_flags)
		) == -1, "fd %d; open %s", out->fd, path);

	/* get length */
	Z_die_if((
		out->len = lseek(out->fd, 0, SEEK_END)
		) == -1, "SEEK_END of '%s' gives %ld", path, out->len);

	/* mmap file */
	Z_die_if((
		out->mem = mmap(NULL, out->len, PROT_READ, MAP_PRIVATE, out->fd, 0)
		) == MAP_FAILED, "map %s sz %zu fd %"PRId32, path, out->len, out->fd);

	return 0;
out:
	nmem_free(out, NULL);
	return err_cnt;
}

