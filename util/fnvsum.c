/*	fnvsum.c

A command-line application, on the pattern of md5sum,
	which applies the 64-bit FNV1a algorithm to the named filed
	(or standard input if blank).

(c) 2017 Sirio Balmelli; https://b-ad.ch
*/

#include <zed_dbg.h>
#include <fnv.h>

/* stat() */
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h> /* open() */
#include <sys/mman.h> /* mmap() */



/*	do_stdin()
Hash the contents of stdin; until EOF or errno do us part.
*/
int	do_stdin()
{
	static const size_t buf_sz = 1024; /* reasonable assumption on most systems? */
	uint8_t buf[buf_sz];

	uint64_t hash = fnv_hash64(NULL, NULL, 0); /* initialize hash */

	size_t size;
	/* hash full buffer blocks */
	while ((size = fread(buf, sizeof(buf[0]), buf_sz, stdin)) == buf_sz)
		hash = fnv_hash64(&hash, buf, buf_sz);
	/* hash trailing blocks.
	Note that '0' is a valid size to hash; it does nothing.
	*/
	hash = fnv_hash64(&hash, buf, size);

	fprintf(stdout, "%"PRIx64"  -\n", hash);
	return 0;
}



/*	do_file()
Hash a file, print the results;
*/
int do_file(const char *file)
{
	int err_cnt = 0;
	int fd = -1;
	void *map = NULL;

	/* a '-' file is handled as stdin */
	if (file[0] == '-')
		return do_stdin();

	/* stat file; sanity */
	struct stat st;
	Z_die_if( stat(file, &st),
		"error stat()ing '%s'", file);
	Z_die_if(!S_ISREG(st.st_mode),
		"'%s' not a regular file", file);
	
	/* init hash */
	uint64_t hash = fnv_hash64(NULL, NULL, 0);

	if (st.st_size) {
		/* open file */
		Z_die_if((
			fd = open(file, O_RDONLY)
			) == -1, "open() '%s'", file);
		/* map it */
		Z_die_if((
			map = mmap(NULL, st.st_size, PROT_READ, MAP_SHARED, fd, 0)
			) == MAP_FAILED, "mmap() '%s'", file);
	}

	/* hash and print */
	hash = fnv_hash64(NULL, map, st.st_size);
	fprintf(stdout, "%"PRIx64"  %s\n", hash, file);

out:
	if (map && map != MAP_FAILED)
		munmap(map, st.st_size);
	if (fd != -1)
		close(fd);
	return err_cnt;
}



/*	print_usage()
*/
void print_usage(char *pgm_name)
{
	fprintf(stderr,
"usage:\n\
	%s [OPTIONS] [FILE]\n\
\n\
Calculate the 64-bit FNV1a hash of FILE;\n\
	read from standard input if no FILE or when FILE is '-'\n\
\n\
-h | -?  :  print usage details\n",
		pgm_name);
}



/*	main()
*/
int main(int argc, char **argv)
{
	int err_cnt = 0;

	/* no args means "hash from stdint" */
	if (argc == 1)
		return do_stdin();

	/* 'help' flag only valid BEFORE any files.
		... pattern matching from the ghetto ;)
	*/
	if (argv[1][0] == '-' && (argv[1][1] == 'h' || argv[1][1] == '?')) {
		print_usage(argv[0]);
		return 0;
	}

	/* otherwise, look for files */
	while (argc > 1)
		err_cnt += do_file(argv[--argc]);

	return err_cnt;
}
