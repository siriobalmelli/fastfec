#include <nmem.h>

#include <malloc.h>
#include <libgen.h> /* dirname */
#include <unistd.h>
#include <stdlib.h>
#include <getopt.h>

#include <zed_dbg.h>

/*	ncp	the nmem cp replacement

A version of 'cp' showing zero-copy I/O using nmem.
This is usually faster for larger files.

(c)2017 Sirio; Balmelli Analog & Digital
*/

int main(int argc, char **argv)
{
	int err_cnt = 0;

	struct nmem src = { 0 };
	struct nmem dst = { 0 };
	int piping[2] = { 0 };

	char *src_path = NULL;
	char *dst_path = NULL;
	char *dst_dir = NULL;

	/* TODO:
	-	replace with a full getopt parser
	-	implement '-v' and '-f' flag; error on other flags
	-	implement multiple source files into one destination dir
	*/
	int opt = 0;
	int opt_ind = 1;
	int force = 0;
	int verbose = 0;

	static struct option long_options[] = {
		{ "verbose",	no_argument,	0,	'v'},
		{ "force",	no_argument,	0,	'f'},
	};

	while ((opt = getopt_long(argc, argv, "vf", long_options, NULL)) != -1) {
		switch(opt)
		{
			case 'v':
				verbose = 1;
				opt_ind++;
				break;
			case 'f':
				force = 1;
				opt_ind++;
				break;
			default:
				fprintf(stderr, "Usage: %s SOURCE_FILE DEST_FILE\r\n", argv[0]);
				exit(EXIT_FAILURE);
		}
	}

	src_path = argv[opt_ind++];
	dst_path = argv[opt_ind++];

	if (verbose) {
		fprintf(stdout, "'%s' -> '%s'\r\n", src_path, dst_path);
	}
	/* open source and pipe */
	Z_die_if(
		nmem_file(src_path, &src)
		, "");
	Z_die_if(pipe(piping), "");
	/* open destination
	TODO: roll our own non-shit dirname() implementation
	*/
	size_t path_len = strlen(dst_path);
	Z_die_if(!(
		dst_dir = malloc(path_len)
		), "path_len %zu", path_len);
	memcpy(dst_dir, dst_path, path_len);
	Z_die_if(
		nmem_alloc(src.len, dirname(dst_dir), &dst)
		, "");

	/* splicings */
	for (size_t fd_sz, done_sz=0; done_sz < src.len; ) {
		fd_sz = nmem_out_splice(&src, done_sz, src.len-done_sz, piping[1]);
		for (size_t temp=0; fd_sz > 0; ) {
			Z_die_if(!(
				temp = nmem_in_splice(&dst, done_sz, fd_sz, piping[0])
				), "");
			done_sz += temp;
			fd_sz -= temp;
		}
	}

	/* Delete a possible existing file */
	if (force) {
		unlink(dst_path);
		errno = 0;
	}
	/* we're clean: close and deliver the file */
	nmem_free(&dst, dst_path);

out:
	nmem_free(&src, NULL);
	nmem_free(&dst, NULL);
	if (piping[0])
		close(piping[0]);
	if (piping[1])
		close(piping[1]);
	if (dst_dir)
		free(dst_dir);
	return err_cnt;
}
