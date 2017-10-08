#include <npath.h>
#include <zed_dbg.h>

static const char n_sep = '/'; /* TODO: unsure best way to make this cross-platform */

/*	invariants (taken from LibC's basename() and dirname() implementation

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


/*	n_path_join()
Return a newly allocated string which joins 'dir' and 'base' with the path separator.

NOTES:
This is the reciprocal of n_basename() and n_dirname() above;
	in certain corner cases it doesn't just concatenate
	'dirname' and 'basename', but accurately reproduces
	the original path string which would have formed them.
... it's actually NOT the reciprocal in the isolated case where the original
	path to be split had a trailing '/' - n_basename() discards it
	so there is NO way to know it was there.
See npath_test.c for details.
*/
char *n_join(const char *dir_name, const char *base_name)
{
	char *ret = NULL;
	size_t dir_len = 0, base_len = 0;

	Z_die_if(!base_name, "args");
	base_len = strlen(base_name);

	/* sanitize dir */
	if (dir_name) {
		dir_len = strlen(dir_name);
		/* corner cases: when dirname should be removed.
		P.S.: I know this could be a single if statement ... but it's legible as-is.
		*/
		if (dir_len <= 1) {
			if (dir_name[0] == '.')
				dir_len = 0;
			if (dir_name[0] == n_sep && base_name[0] == n_sep)
				dir_len = 0;
		}
	}


	Z_die_if(!(
		ret = malloc(dir_len + base_len + 2)
		), "len %zu", dir_len + base_len + 2);

	/* dir_name trails with a separator: omit the separator */
	if (dir_len && dir_name[dir_len -1] == n_sep) {
		sprintf(ret, "%s%s", dir_name, base_name);
	
	/* classical case: concatenate dir_name, separator and base_name */
	} else if (dir_len) {
		sprintf(ret, "%s%c%s", dir_name, n_sep, base_name);
	
	/* no dir_name: return base_name */
	} else {
		sprintf(ret, "%s", base_name);
	}

	return ret;
out:
	free(ret);
	return NULL;
}


/*	n_is_dir()
Returns 1 if path ends with the separator, otherwise 0
*/
int n_is_dir(const char *path)
{
	size_t ret = 0;
	while (path[ret] != '\0')
		ret++;

	if (path[ret-1] == n_sep)
		return 1;
	if (ret == 1 && path[0] == '.')
		return 1;
	return 0;
}
