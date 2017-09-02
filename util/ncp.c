#include <nmem.h>

#include <npath.h> /* n_dirname() */
#include <unistd.h>
#include <stdlib.h>
#include <getopt.h>

#include <zed_dbg.h>

/*	ncp	the nmem cp replacement

A version of 'cp' showing zero-copy I/O using nmem.
This is usually faster for larger files.

(c)2017 Sirio; Balmelli Analog & Digital
*/


/*
	global option flags
*/
static int force = 0;
static int verbose = 0;


/*	print_usage()
*/
void print_usage(char *pgm_name)
{
	fprintf(stderr, "Usage: %1$s [OPTION]... SOURCE_FILE DEST_FILE\n\
  or:  %1$s [OPTION]... SOURCE_FILE... DEST_DIR\n\
An analog of 'cp' using nmem(3) zero-copy I/O for speed.\n\
\n\
Options:\n\
\t-v, --verbose	:	list each file being copied\n\
\t-f, --force	:	overwrite destination file(s) if existing\n\
\t-h, --help	:	print usage and exit\n",
		pgm_name);
}


/*	cp()
Do copy operation
*/
int cp(const char *src_path, const char *dst_path, int *piping)
{
	int err_cnt = 0;

	struct nmem src = { 0 };
	struct nmem dst = { 0 };
	char *dst_dir = NULL;

	/* open source */
	Z_die_if(
		nmem_file(src_path, &src)
		, "");
	/* open destination */
	dst_dir = n_dirname(dst_path);
	Z_die_if(
		nmem_alloc(src.len, dst_dir, &dst)
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
	if (verbose)
		fprintf(stdout, "'%s' -> '%s'\n", src_path, dst_path);

out:
	nmem_free(&src, NULL);
	nmem_free(&dst, NULL);
	free(dst_dir);
	return err_cnt;
}

int main(int argc, char **argv)
{
	int err_cnt = 0;

	char *src_path = NULL;
	char *dst_path = NULL;

	/* use one pipe for all I/O */
	int piping[2] = { 0 };
	Z_die_if(pipe(piping), "");

	int opt = 0;
	static struct option long_options[] = {
		{ "verbose",	no_argument,	0,	'v'},
		{ "force",	no_argument,	0,	'f'},
		{ "help",	no_argument,	0,	'h'},
	};

	while ((opt = getopt_long_only(argc, argv, "vfh", long_options, NULL)) != -1) {
		switch(opt)
		{
			case 'v':
				verbose++;
				if (verbose >= 2)
					Z_log(Z_inf, "verbose");
				break;
			case 'f':
				force = 1;
				if (verbose >= 2)
					Z_log(Z_inf, "force");
				break;
			case 'h':
				print_usage(argv[0]);
				goto out;
			default:
				print_usage(argv[0]);
				Z_die("option '%c' invalid", opt);
		}
	}

	/* sanity check file arguments */
	int count = argc - optind;
	if (count < 2) {
		print_usage(argv[0]);
		Z_die("missing SOURCE_FILE or DEST_FILE");

	/* SOURCE_FILE -> DEST_FILE */
	} else if (count == 2) {
		src_path = argv[optind++];
		dst_path = argv[optind++];
		cp(src_path, dst_path, piping);

	/* SOURCE_FILE... -> DEST_DIR */
	} else {
		while (optind < argc -1) {
			src_path = argv[optind++];
			char *src_base = n_basename(src_path);
			dst_path = n_join(argv[argc-1], src_base);

			cp(src_path, dst_path, piping);
			free(src_base);
			free(dst_path); dst_path = NULL;
		}
	}

out:
	if (piping[0]) {
		close(piping[0]);
		close(piping[1]);
	}
	return err_cnt;
}
