#include <npath.h>
#include <zed_dbg.h>

static const char n_sep = '/'; /* TODO: unsure best way to make this cross-platform */

/*	invariants:

       path       dirname   basename
       /usr/lib   /usr      lib
       /usr/      /         usr
       usr        .         usr
       /          /         /
       .          .         .
       ..         .         ..
*/


/*	n_counts_()
Walk a 'path' string and return:
	'out_len'	:	Equivalent to strlen(path)
	'out_last_sep'	:	The position of the last separator
				which ISN'T THE TRAILING CHARACTER.
*/
void n_counts_(const char *path, size_t *out_len, size_t *out_last_sep)
{
	size_t len = 0, last_sep = 0;
	while (path[len] != '\0') {
		/* if separator is last character, ignore it */
		if (path[len] == n_sep && path[len+1] != '\0')
			last_sep = len;
		len++;
	}
	*out_len = len;
	*out_last_sep = last_sep;
}


/*	n_dirname()
Get get the "directory" component of 'path'.
Return it as a newly allocated string.
*/
char	*n_dirname(const char *path)
{
	char *ret = NULL;

	size_t len = 0, last_sep = 0;
	n_counts_(path, &len, &last_sep);

	/* Never encountered a separator,
		aka: all your corner cases are belong to us.
	*/
	if (!last_sep) {
		Z_die_if(!(
			ret = malloc(2)
			), "");
		if (path[0] == '/')
			sprintf(ret, "/");
		else
			sprintf(ret, ".");

	/* Alloc and copy UP TO (but not including) last separator encountered */
	} else {
		Z_die_if(!(
			ret = malloc(last_sep+1)
			), "len = %zu", last_sep+1);
		memcpy(ret, path, last_sep);
		ret[last_sep] = '\0';
	}

	return ret;
out:
	free(ret);
	return NULL;
}


/*	n_basename()
Get the "file" component of 'path'.
Return it as a newly allocated string.
*/
char	*n_basename(const char *path)
{
/*	invariants:

       path       dirname   basename
       /usr/lib   /usr      lib
       /usr/      /         usr
       usr        .         usr
       /          /         /
       .          .         .
       ..         .         ..
*/
	char *ret = NULL;

	size_t len = 0, last_sep = 0;
	n_counts_(path, &len, &last_sep);

	/* start copy at the character which follows 'last_sep',
		except where path is "/"
	*/
	if (last_sep || (path[0] == n_sep && len > 1))
		last_sep++;

	/* string will be at most this long */
	len -= last_sep;
	Z_die_if(!(
		ret = malloc(len+1)
		), "len %zu", len);

	memcpy(ret, &path[last_sep], len);

	/* clobber any trailing slashes */
	if (ret[len-1] == n_sep && len > 1)
		len--;
	ret[len] = '\0';

	return ret;
out:
	free(ret);
	return NULL;
}
