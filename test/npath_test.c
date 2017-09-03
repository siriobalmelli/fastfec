#include <npath.h>
#include <zed_dbg.h>

char *tests[] = {
	"/a",
	"/usr/lib",
	"/usr/",
	"usr",
	"/",
	".",
	"..",
	"a/subdir"
};

char *res_dirname[] = {
	"/",
	"/usr",
	"/",
	".",
	"/",
	".",
	".",
	"a"
};

char *res_basename[] = {
	"a",
	"lib",
	"usr",
	"usr",
	"/",
	".",
	"..",
	"subdir"
};

char *res_join[] = {
	"/a",
	"/usr/lib",
	"/usr",		/* This is the only change vs. tests[] above:
				there is NO way to know it had a trailing '/'
			*/
	"usr",
	"/",
	".",
	"..",
	"a/subdir"
};

/*	test_basedir()
Test basename and dirname and join
*/
int test_basedir()
{
	int err_cnt = 0;

	char *d_name = NULL, *b_name = NULL, *join = NULL;

	for (int i=0; i < sizeof(tests)/sizeof(char*); i++) {
		d_name = n_dirname(tests[i]);
		Z_die_if(strcmp(d_name, res_dirname[i]),
			"\n\texpected: n_dirname(%s) -> %s\n\treturned: %s",
			tests[i], res_dirname[i], d_name);

		b_name = n_basename(tests[i]);
		Z_die_if(strcmp(b_name, res_basename[i]),
			"\n\texpected: n_basename(%s) -> %s\n\treturned: %s",
			tests[i], res_basename[i], b_name);

		join = n_join(d_name, b_name);
		Z_die_if(strcmp(join, res_join[i]),
			"\n\texpected: n_join(%s, %s) -> %s\n\treturned: %s",
			d_name, b_name, res_join[i], join);

		free(d_name); free(b_name); free(join);
		join = d_name = b_name = NULL;
	}

out:
	free(d_name);
	free(b_name);
	free(join);
	return err_cnt;
}


/*	test_isdir()
*/
int test_isdir()
{
	int err_cnt = 0;

	/* these are directories */
	Z_err_if(!n_is_dir("/"), "");
	Z_err_if(!n_is_dir("./"), "");
	Z_err_if(!n_is_dir("."), "");
	Z_err_if(!n_is_dir("/hello/"), "");

	/* these are not */
	Z_err_if(n_is_dir("hello"), "");
	Z_err_if(n_is_dir("/hello"), "");

	return err_cnt;
}


/*	main()
*/
int main()
{
	int err_cnt = 0;

	err_cnt += test_basedir();
	err_cnt += test_isdir();

	return err_cnt;
}
