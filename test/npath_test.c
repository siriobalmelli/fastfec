#include <npath.h>
#include <zed_dbg.h>

char *tests[] = {
	"/a",
	"/a/really/long/path",
	"/usr/lib",
	"/usr/",
	"usr",
	"/",
	".",
	".."
};

char *res_dirname[] = {
	"/",
	"/a/really/long",
	"/usr",
	"/",
	".",
	"/",
	".",
	"."
};

char *res_basename[] = {
	"a",
	"path",
	"lib",
	"usr",
	"usr",
	"/",
	".",
	".."
};


/*	main()
*/
int main()
{
	int err_cnt = 0;
	char *d_name = NULL, *b_name = NULL;

	for (int i=0; i < sizeof(tests)/sizeof(char*); i++) {
		d_name = n_dirname(tests[i]);
		b_name = n_basename(tests[i]);
		Z_die_if(strcmp(d_name, res_dirname[i]),
			"\n\texpected: n_dirname(%s) -> %s\n\treturned: %s",
			tests[i], res_dirname[i], d_name);
		Z_die_if(strcmp(b_name, res_basename[i]),
			"\n\texpected: n_basename(%s) -> %s\n\treturned: %s",
			tests[i], res_basename[i], b_name);

		free(d_name); free(b_name);
		d_name = b_name = NULL;
	}

out:
	free(d_name);
	free(b_name);
	return err_cnt;
}
