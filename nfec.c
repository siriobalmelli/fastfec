/*
	NFEC	-	n-dimension FEC

------------------------ sample matrix ---------------------------------------------------------
source/page	:	4					// number of source symbols each "page"
parity/page	:	3

width		:	2					// nr. of pages in a matrix, overlap "depth"
page_cnt	:	width * 2 -1		:	3

num_source	:	page_cnt * source_page	:	12	// total number of source symbols
num_parity	:	page_cnt * parity_page	:	9

N		:	1					// Row-count per column.
								// NOTE: "1" in unrealistic but simple to portray.
cell_cnt	:	N * width		:	2	// how many matrix cells per source symbol

	s0 s1 s2 s3	s4 s5 s6 s7	s8 s9 s10 s11	p0 p1 p2	p3 p4 p5	p6 p7 p8
r0	1        1	      1      	                1  x  x
r1	   1		1        1   	                1  1  x
r2	      1		   1         	                x  1  1
                                                                        
                                                                        
r3			1        1	      1				1  x  x
r4			   1		1         1			1  1  x
r5			      1		   1				x  1  1
                                                                        
                                                                        
r6	      1        			1         1					1  x  x
r7	1				   1						1  1  x
r8	   1     1			      1						x  1  1
------------------------ repeats identically --------------------------------------------------


THOUGHTS: 
-	'seq_no' should probably be stored in each row?
-	UINT32_MAX should divide into 'page_cnt' evenly, so overflow doesn't explode things.
-	DO we need to distinguish between "solved" and "unused" states of a cell?
	For now, ASSUME we don't.
-	Randomize matrix "1" allocation only once.
-	Algo for staircase-assigning rows to each cell (before KFY random):
		Ps1	-> Pr0, Pr1		// PAGEsource1	-> PAGErepair0, PAGErepair1
		Ps2	-> Pr1, Pr2
		Ps3/0	-> Pr2, Pr3/0
-	Algo for recursion: single matrix "row" of cell IDs pending decode?
	This way NO MALLOC; and the same cell_id may be "set" (added to the row) more than once
		when decoding recursively without ill consequences.
-	Maintain 2 matrices and 2 memory buffers: the "current" and the "past".
	When pages drop off of "active" they go to "stale" and are
		re-requested (using simulation algo).
	When pages drop off of "stale" they are forgotten.

------------------------ another sample matrix ------------------------------------------------
source/page	:	10
parity/page	:	4

width		:	4
page_cnt	:	width * 2 -1		:	7

num_source	:	page_cnt * source_page	:	70
num_parity	:	page_cnt * parity_page	:	28

N		:	1					// fake, for simplicity's sake only
cell_cnt	:	N * width		:	4

	source symbols						repair symbols
	s0-9 | 10-19 | 20-29 | 30-39 | 40-49 | 50-59 | 60-69	p0-3 | 4-7 | 8-11 | 12-15 | 16-19 | 20-23 | 24-27
	[ ---- preseed ---- ]
r0-3	x	x	x	x				x
r4-7		x	x	x	x				x
r8-11			x	x	x	x				x
r12-15				x	x	x	x				x
r16-19	x				x	x	x					x
r20-23	x	x				x	x						x
r24-27	x	x	x				x							x
------------------------ repeats identically --------------------------------------------------

*/


#include <stdint.h>

struct nfec_params {
	/* classic FEC params FOR EACH PAGE */
	double		ratio;
	uint32_t	sym_len;

	uint32_t	N;			/* Number of matrix 1's per symbol */

	/* page-related params */
	uint32_t	width;			/* nr. pages in a single matrix; overlap depth */
	uint32_t	page_cnt;		/* nr. pages in the circular buffer */		
}__attribute__((packed));


#include <ffec_matrix.h>

struct nfec_buffer {
	/* memory */
	void			*source;
	void			*parity;
	union {
	void			*scratch;
	struct ffec_cell	*cells;		/* scratch region begins with all the cells */	
	};
	struct ffec_row		*rows;
	struct ffec_row		*dec_stack;	/* instead of using j1_arrays as a decode recursion stack */
	void			*psums;

	/* state */
	uint32_t		*k_decoded;
	uint64_t		seq_no;
}__attribute__((packed)); /* 64B */


#include <pcg_rand.h>

enum nfec_direction {
	nfec_encode,
	nfec_decode
};

struct nfec_instance {
	/* params */
	struct nfec_params	*par;
	enum nfec_direction	dir;

	/* RNG */
	struct pcg_rand_state	rng;

	/* per-page symbol counts */
	uint32_t		k;
	uint32_t		p;
	uint32_t		n;

	/* buffers */
	struct nfec_buffer	*active;
	struct nfec_buffer	*stale;
};


/*	nfec_instance()
Allocate a new nfec_instance, AND the buffer memory it uses for symbols.
*/
struct nfec_instance	*nfec_new(struct nfec_params *par, enum nfec_direction dir);
void			nfec_free(struct nfec_instance *ni);


