#ifndef mfec_h_
#define mfec_h_

#include <stdint.h>
#include <sys/mman.h>

#include <bits.h>
#include <sbfu.h>
#include <ffec.h>
#include <zed_dbg.h>

#include <Judy.h>

#include <stdlib.h> /* mkostemp() */

/*
	a single book
*/
struct mfec_bk {
	uint64_t		seq_no;
	struct ffec_instance	fi;		/* The 'parity' and 'scratch'
							pointers are set independent of FEC,
							and point into the ring.
						"recycle" them by passing them
							to ffec_init() every time.
						*/
	off_t			data_offt;	/* Offset of last ("data") page
							in underlying ringbuffer.
						Used for memcpy() and splice() operations.
						NOTE: NO relationship to assembled "source" region
							used by 'fi' above.
						*/
	struct mfec_hp		*hp;

	/* ESI propagation tracking */
	Pvoid_t			j1_pend;
	Pvoid_t			j1_done;
};

/*
	heap management
*/
struct mfec_hp {
	/* single mmap'ed ringbuffer holding all symbols */
	int			ring_fd;	
	struct iovec		the_one_ring;

	/* ring overlap factors */
	uint64_t		width;
	size_t			span;	/* `width *2 -1`
					Every array here is 'span' long,
						and we loop `% span`
					*/

	enum ffec_direction	dir;	/* encode/decode */
	/* sym_len and fec_ratio */
	struct ffec_params	fp;
	uint64_t		syms_page;/* Number of symbols per page.
					e.g. 2048 for a 2M hugepage @ 1024 sym_len
					*/

	/* matrices */
	uint64_t		seq_no;
	struct ffec_sizes	fs;
	struct mfec_bk		*books;
};

/*	mfec_pg()
returns the pagesize
*/
Z_INL_FORCE size_t mfec_pg(struct mfec_hp *hp)
{
	return hp->syms_page * hp->fp.sym_len;
}

/*	mfec_esi_walk()
Translate 'esi' (for a source symbol only) "forwards" or "backwards" by
	'walk' pages.
Be sure to check the return value for '<0' or '>fi.cnt.k'
*/
int64_t	mfec_esi_walk(struct mfec_hp *hp, uint32_t esi, int walk)
{
	return (int64_t)esi + ((int64_t)walk * hp->syms_page);
}

/*	mfec_bk_data()
Get "data" region of book, which is ONLY the last page.
Everything else in this book is either seed data or pages from previous books
	(should not be changed directly).

NOTE: Returned address is in the underlying ringbuffer, NOT the assembled
	"source" region seen by 'bk->fi'.
*/
Z_INL_FORCE void *mfec_bk_data(struct mfec_bk *bk)
{
	return bk->hp->the_one_ring.iov_base + bk->data_offt;
}

/*	mfec_bk_txesi_cnt()
How many ESIs are actually transmitted for a book?
*/
Z_INL_FORCE uint32_t mfec_bk_txesi_cnt(struct mfec_bk *bk)
{
	return bk->fi.cnt.n - bk->hp->syms_page * (bk->hp->width - 1);
}


void		*mfec_source_map(struct mfec_hp *hp, uint64_t page_idx, off_t *last_offt);

void		mfec_bk_inval	(struct mfec_bk *bk);
struct mfec_bk	*mfec_bk_get	(struct mfec_hp *hp, uint64_t seq_no);
struct mfec_bk	*mfec_bk_next	(struct mfec_hp *hp);

uint32_t	*mfec_encode	(struct mfec_bk *bk, void *data);
uint64_t	mfec_sym_propagate(struct mfec_bk *bk, uint32_t	esi);
uint32_t	mfec_decode	(struct mfec_bk	*bk, void *symbol, uint32_t esi);

int		mfec_hp_init	(struct mfec_hp *hp, uint32_t width, uint64_t syms_page,
				enum ffec_direction dir, struct ffec_params fp,
				uint64_t seq_no);
void		mfec_hp_clean	(struct mfec_hp *hp);
#endif /* mfec_h_ */
