#include <nmem.h>

//#define Z_LOG_LVL (Z_inf | Z_wrn | Z_err | Z_in2)
#include <zed_dbg.h>

#include <npath.h>
#include <fcntl.h>
#include <sys/param.h> /* MAXPATHLEN */

/*	nmem_bsd.c	BSD-specific implementation for nmem
*/


int		nmem_alloc(size_t len, const char *tmp_dir, struct nmem *out)
{
	int err_cnt = 0;
	char *tmpfile = NULL;
	Z_die_if(!len || !out, "args");
	out->len = len;

	if (!tmp_dir)
		tmp_dir = "/tmp";
	tmpfile = n_join(tmp_dir, ".temp_XXXXXX");
	

	/* open an fd */
	out->o_flags = 0;
	Z_die_if((
		out->fd = mkstemp(tmpfile)
		) == -1, "failed to open temp file in %s", tmp_dir);
	/* TODO: must figure out how to unlink from filesystem for security
		... while still being able to link INTO filesystem at 'nmem_free'
		below. !
	Z_die_if((
		unlink(tmpfile)
		) == -1, "unlink %s", tmpfile);
	*/

	/* size and map */
	Z_die_if(ftruncate(out->fd, out->len), "len=%ld", out->len);
	Z_die_if((
		out->mem = mmap(NULL, out->len, PROT_READ | PROT_WRITE, MAP_SHARED, out->fd, 0)
		) == MAP_FAILED, "map sz %zu fd %"PRId32, out->len, out->fd);

	return 0;
out:
	free(tmpfile);
	nmem_free(out, NULL);
	return err_cnt;
}

void		nmem_free(struct nmem *nm, const char *deliver_path)
{
	if (!nm)
		return;

	/* unmap */
	if (nm->mem && nm->mem != MAP_FAILED)
		munmap(nm->mem, nm->len);
	nm->mem = NULL;

	/* deliver if requested */
	if (deliver_path) {
		char path[MAXPATHLEN];
		Z_die_if((
			fcntl(nm->fd, F_GETPATH, path)
			) == -1, "");		
		Z_die_if((
			rename(path, deliver_path)
			) == -1, "%s -> %s", path, deliver_path);
	}

out:
	/* close */
	if (nm->fd != -1)
		close(nm->fd);
	nm->fd = -1;
}

size_t	nmem_in_splice(struct nmem	*nm,
     			size_t		offset,
     			size_t		len,
     			int		fd_pipe_from)
{
	Z_die_if(!nm || fd_pipe_from < 1, "args");

	/* Owing to DARWIN's inevitable, demoralizing behavior of BLOCKING when
		write()ing to a pipe (and by contagion, I'm going to ASSUME read() as well);
		regardless of a previously successful fcntl() setting O_NONBLOCK!
	TBH I won't care until someone yells about this code being slooow,
		which is likely never as who even USES Darwin for serious computing??

	In case you're wondering, this is however STILL better than
		a poke in the eye with a sharp memcpy().
	*/
	if (len > PIPE_BUF)
		len = PIPE_BUF;
	Z_log(Z_in2, "len %zu; offt %zu", len, offset);

	ssize_t ret = read(fd_pipe_from, nm->mem + offset, len);
	Z_die_if(ret < 0, "len %zu", len);
	return ret;
out:
	return 0;
}

size_t	nmem_out_splice(struct nmem	*nm,
			size_t		offset,
			size_t		len,
			int		fd_pipe_to)
{
	Z_die_if(!nm || fd_pipe_to < 1, "args");

	/* because DARWIN, see above */
	if (len > PIPE_BUF)
		len = PIPE_BUF;
	Z_log(Z_in2, "len %zu; offt %zu", len, offset);

	ssize_t ret = write(fd_pipe_to, nm->mem + offset, len);
	Z_die_if(ret < 0, "len %zu", len);
	return ret;
out:
	return 0;
}
