#include <zed_dbg.h>
#include <atop.h>
#include <stdlib.h>	/* malloc() / free() */

/*	atop_test.c

Demonstration for the "atomic operations" family of functions.

TODO:
-	add threads to ATOP_SWAP_EXEC test
-	write tests for atop_txn_() stuff

(c) 2017 Sirio Balmelli; https://b-ad.ch
*/



/*	ATOP_SWAP_EXEC

Show ATOP using a pretend-library implementation
	which uses a 'struct libw_widget' object.
*/
struct libw_widget {
	int	fd;	/* file descriptor (result of e.g. libc call) */
	/*
		other widgety things go here
	*/
};

/*	libw_free()
Free a widget, perform all cleanup actions.
*/
void		libw_free(struct libw_widget *wt)
{
	/* Tolerate being called with NULL, a la free() */
	if (!wt)
		return;
	/* Call close on fd if it's open (aka: not in error state); but only once */
	ATOP_SWAP_EXEC(wt->fd, -1, close);
	/* free struct alloc */
	free(wt);
}

/*	libw_new()
New widget object.
*/
struct libw_widget *libw_new(void *random_arg)
{
	/* return value pre-initialized to error state */
	struct libw_widget *ret = NULL;

	/* check arg sanity */
	Z_die_if(!random_arg, "expecting non-NULL function arg");

	/* fail on bad malloc() */
	Z_die_if((ret = malloc(sizeof(struct libw_widget))) == NULL,
		"alloc size %zu",
		sizeof(struct libw_widget));

	// widget defaults in error (closed) state
	ret->fd = -1;
	/*
		Code to try and open fd, do other operations.
		All statements checked for success with a Z_die_if().
	*/


	/* success - return an object */
	return ret;
out:
	/* failure - clean up gracefully */
	libw_free(ret);
	return NULL;
}



struct libw_widget *wt_a = NULL;
struct libw_widget *wt_b = NULL;

/*	main()
*/
int main()
{
	int err_cnt = 0;

	/* instantiate objects */
	char useless_array[8] = { 0 };
	/* this one will succeed */
	wt_a = libw_new(useless_array);
	/* this one will fail */
	wt_b = libw_new(NULL);

	/*
		do threaded things with widgets here
	*/

	/* safe cleanup, regardless of status */
	ATOP_SWAP_EXEC(wt_a, NULL, libw_free);
	ATOP_SWAP_EXEC(wt_b, NULL, libw_free);

	return err_cnt;
}
