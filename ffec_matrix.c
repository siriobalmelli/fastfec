/*	ffec_matrix.c

This is some 411 on the implementation of the LDGM matrix for ffec.
It is assumed that the reader understands the math behind the parity
	matrix being described.
See comments in "ffec.c" for background.

Each "1" in the matrix is represented by a "cell" structure.

All the cells are contiguous in memory, and since
	the number of 1's for each column is precisely FFEC_N1_DEGREE
	(known at compile time), there is no need to explicitly represent
	"columns", as we can index into the "cells" array directly.
The inline ffec_get_col_first() gets the first cell in a column,
	after which simple pointer increments will yield the next cells.

The matrix rows (equations) however ARE implemented as a linked list.
Each equation is represented by a "row" structure.


Linked list and ID FIELDS:

The basic issue here is that we want to cut in half the amount of space being
	used for linked-list pointers.
Also, we want to be able to memcpy an entire matrix (rows + cells) elsewhere
	and still be able to use it (decode simulation, etc).

Therefore, linked list "pointers" are actually offsets from the "base"
	address (address of first cell).
"base" must refer to the beginning of an array which contiguously contains
	all cells and then all rows.

We KNOW the entire array of rows AND columns WON'T ever be larger than
	UINT32_MAX, so we can use a uint32_t 'id' fields,
	rather than 64-bit pointers.

INVARIANT this approach only works if:
	- memory is arranged as "cells, THEN rows"
	- memory is ALWAYS CONTIGUOUS
	- sizeof(cell) == sizeof(row)

The counterintuitive part of this is that a row DOES have a "cell_id",
	which is "cell_cnt + row_id".
This is ONLY used so that linked list addition and removal operations
	can treat the row like just another "previous" or "next" cell
	without wasteful if clauses.


Here is a basic conceptual diagram which was of help in visualizing the matrix:

[matrix at start of decode]
		idx=2	idx=1	idx=2	idx=2	idx=2	idx=2	idx=1
		s1	s2	s3	s4	p1	p2	p3
idx=4	r1	1	0	1	1	1	0	0	psum=0
idx=4	r2	0	1	0	1	1	1	0	psum=0
idx=4	r3	1	0	1	0	0	1	1	psum=0


[after receipt of some symbols]
		idx=0	idx=1	idx=2	idx=0	idx=2	idx=0	idx=1
		s1	s2	s3	s4	p1	p2	p3
idx=2	r1	0	0	1	0	1	0	0	psum=s1^s4
idx=2	r2	0	1	0	0	1	0	0	psum=p2^s4
idx=2	r3	0	0	1	0	0	0	1	psum=s1^p2


2016 Sirio Balmelli
*/

#include "ffec_matrix.h"


#ifdef FFEC_MATRIX_DEBUG
#include <signal.h> /* raise() */
#include <stdlib.h> /* malloc() */
/* self-test code */
__attribute__((constructor)) void ffec_matrix_self_test()
{
	struct ffec_cell a_cell;
	ffec_cell_init(&a_cell, 42);
	Z_die_if(!ffec_cell_test(&a_cell), "expecting cell to be unset after init");
	
	Z_inf(0, "matrix self-test OK");
	return;
out:
	raise(SIGQUIT);
}

/*	ffec_matrix_row_prn()
Print a row including all its linked cells.
*/
void		ffec_matrix_row_prn(struct ffec_row		*row,
					struct ffec_cell	*base)
{
	printf("r[%d].cnt=%02d  %d", row->c_me, row->cnt, row->c_me);

	int i=0;
	/* for a row, "next" is "first" */
	struct ffec_cell *c_fwd = (struct ffec_cell*)row; 
	/* one extra iteration, print row again at the end */
	for (; i <= row->cnt; i++) {
		c_fwd = &base[c_fwd->c_next];
		printf(" -> %d", c_fwd->c_me);
	}
	printf("\n");
}
#endif


/*	ffec_matrix_row_link()
Add cell 'c_new' into the linked list of 'row'.
*/
void		ffec_matrix_row_link(	struct ffec_row		*row,
					struct ffec_cell	*c_new,
					struct ffec_cell	*base)
{
	/* we insert at "tail", so our "prev" is the existing "tail" */
	c_new->c_prev = row->c_last;
	/* and we are their "next" */
	base[c_new->c_prev].c_next = c_new->c_me;

	/* we are last, so row (head) is "next" */
	c_new->c_next = row->c_me;
	row->c_last = c_new->c_me;

	/* increment row (aka: "head") counter */
	row->cnt++;

#ifdef FFEC_MATRIX_DEBUG
	ffec_matrix_row_prn(row, base);
#endif
}

void		ffec_matrix_row_unlink(	struct ffec_row		*row,
					struct ffec_cell	*cell,
					struct ffec_cell	*base)
{
	/* we disappear */
	base[cell->c_prev].c_next = cell->c_next;
	base[cell->c_next].c_prev = cell->c_prev;

	/* unset (aka: init() but we already know 'c_me' */
	cell->c_prev = cell->c_next = cell->c_me;

	/* we count */
	row->cnt--;

#ifdef FFEC_MATRIX_DEBUG
	ffec_matrix_row_prn(row, base);
#endif
}
