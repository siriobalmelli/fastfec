#include <unistd.h>
#include <stdint.h>
#include <sys/mman.h>

#include <bits.h>
#include <sbfu.h>
#include <ffec.h>
#include <zed_dbg.h>

#ifndef _GNU_SOURCE
	#define _GNU_SOURCE	/* /dev/urandom; syscalls */
#endif
#include <unistd.h>
#include <sys/syscall.h>
#include <linux/random.h>

#include <Judy.h>

/*	multifec

A system for the increase of recovery probability by partially overlapping the 
	"data" or "k" symbol regions of multiple FEC matrices.

'page'		:	A block of source symbols, the unit in which source symbol
				memory is computed/handled.
			Must be a multiple of the system page size.

'width'		:	How many separate FEC matrices include the symbols in
				a page (conceptually, the "depth" of overlap).
			Width is ALSO the amount of pages which make up
				the "source" region of a FEC matrix.

'book'		:	A set of 'width' pages artificially concatenated into a contiguous
				virtual memory address space,
				and forming the "source" or "n" region of a FEC matrix.

'matrix'	:	A FEC matrix addressing a 'book' of source symbols,
				(which pages may overlap into other books),
				PLUS a trailing repair region which is unique to this matrix.

'span'		:	This is 'width * 2 -1' and is the size of all ringbuffer
				allocations (source regions, repair regions, FEC structs, etc).

'idx'		:	Index into the ringbuffer.
			This is 'seq % span'.

'seq' || 'seq_no' :	Monotonic sequence counter of FEC matrices,
				also used as the random seed for FEC generation.

MEMORY LAYOUT
	width	:	3
	span	:	width * 2 -1
	
seq		source_pages				repair region

0	Seed0	Seed1	Page0				R0
1		Seed1	Page0	Page1			R1
2			Page0	Page1	Page2		R2
3	Page3			Page1	Page2		R3
4	Page3	Page4			Page2		R4
[cycle repeats as `seq % span`]
5	Page3	Page4	Page5				R0
6		Page4	Page5	Page6			R1
7			Page5	Page6	Page7		R2
8	Page8			Page6	Page7		R3
9	Page8	Page9			Page7		R4

TODO:
-	Can we force the use of 2/4MB hugepages?
*/


/*
	a single book
*/
struct book {
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
	struct heap_mgmt	*hp;
};

/*
	heap management
*/
struct heap_mgmt {
	/* single mmap'ed ringbuffer holding all symbols */
	int			ring_fd;	
	struct iovec		the_one_ring;

	/* ring overlap factors */
	uint32_t		width;
	size_t			span;	/* `width *2 -1`
					Every array here is 'span' long,
						and we loop `% span`
					*/

	enum ffec_direction	dir;
	/* sym_len and fec_ratio */
	struct ffec_params fp;
	uint32_t syms_page;	/* Number of symbols per page.
				e.g. 2048 for a 2M hugepage @ 1024 sym_len
				*/

	/* matrices */
	uint64_t		seq_no;
	struct ffec_sizes	fs;
	struct book		*books;
};


/*	hp_pg()
returns the pagesize
*/
Z_INL_FORCE size_t hp_pg(struct heap_mgmt *hp)
{
	return hp->syms_page * hp->fp.sym_len;
}

/*	esi_walk()
Translate 'esi' (for a source symbol only) "forwards" or "backwards" by
	'walk' pages.
*/
Z_INL_FORCE uint32_t esi_walk(struct heap_mgmt *hp, uint32_t esi, int walk)
{
	return esi + (walk * hp->syms_page);
}

/*	book_data()
Get "data" region of book, which is ONLY the last page.
Everything else in this book is either seed data or pages from previous books
	(should not be changed directly).

NOTE: Returned address is in the underlying ringbuffer, NOT the assembled
	"source" region seen by 'bk->fi'.
*/
Z_INL_FORCE void *book_data(struct book *bk)
{
	return bk->hp->the_one_ring.iov_base + bk->data_offt;
}

/*	book_tx_esi()
How many ESIs are actually transmitted for a book?
*/
Z_INL_FORCE uint32_t book_tx_esi(struct book *bk)
{
	return bk->fi.cnt.n - bk->hp->syms_page * (bk->hp->width - 1);
}


/*	source_assemble()
Assemble (map) 'width' pages from the underlying ring (may roll over somewhere in the middle)
	into a contiguous virtual memory region.

Returns pointer to said region, NULL on error.
We ASSUME returned region must be munmap()ed, but valgrind doesn't complain if not (?!?)

NOTE: 'page_idx' means the FIRST page in the mapping,
	which will then extend 'width' pages.
'page_idx' is 'seq % span'

Populates 'last_offt' with the offset (in the underlying ringbuffer, not the
	the assembled memory region) of the "data" region,
	which callers may wish to splice() into.
*/
void *source_assemble(struct heap_mgmt *hp, uint64_t page_idx, off_t *last_offt)
{
	void *ret = NULL;

	int map_len = hp->span - page_idx;

	/* can mmap all in one shot */
	if (map_len >= hp->width) {
		Z_die_if((
			ret = mmap(NULL, hp->width * hp_pg(hp),
					PROT_READ | PROT_WRITE,
					MAP_SHARED,
					hp->ring_fd,
					page_idx * hp_pg(hp))
			) == MAP_FAILED, "");
		if (last_offt)
			*last_offt = (page_idx + hp->width -1) * hp_pg(hp);

	/* must split map into 2 operations to roll around end of ring */
	} else {
		Z_die_if((
			ret = mmap(NULL, hp_pg(hp) * map_len,
					PROT_READ | PROT_WRITE,
					MAP_SHARED,
					hp->ring_fd,
					page_idx * hp_pg(hp))
			) == MAP_FAILED, "");
		Z_die_if((
			/* requested virtual mem addr is contiguous to previous map */
			mmap(ret + hp_pg(hp) * map_len, 
				hp_pg(hp) * (hp->width - map_len),
				PROT_READ | PROT_WRITE,
				MAP_SHARED | MAP_FIXED,
				hp->ring_fd,
				/* we just rolled over, so offset is clearly 0 */
				0)
			) == MAP_FAILED, "");
		if (last_offt)
			*last_offt = (hp->width - map_len -1) * hp_pg(hp);
	}

	Z_inf(0, "assemble @0x%lx", (uint64_t)ret);
	return ret;
out:
	if (ret && ret != MAP_FAILED)
		munmap(ret, hp_pg(hp) * hp->width); /* unmap the maximum possible amount */
	return NULL;
}


/*	book_get()
Get pointer to book ONLY if seq_no matches.

Returns NULL on mismatch; the world may have moved on, you know - ka and all that.
*/
struct book *book_get(struct heap_mgmt *hp, uint64_t seq_no)
{
	struct book *ret = &hp->books[seq_no % hp->span];
	if (ret->seq_no != seq_no)
		ret = NULL;
	return ret;
}

/*	book_inval()
Free memory mapped by source_assemble().
*/
void book_inval(struct book *bk)
{
	bk->seq_no = -1;
	if (bk->fi.source) {
		munmap(bk->fi.source, hp_pg(bk->hp) * bk->hp->width);
		bk->fi.source = NULL;
	}
}

/*	book_next()
Invalidate the furthest book whose symbols we would clobber, at 'seq_no+width-1'
	(which is the same as 'seq_no-width', accounting for rollover
	across a set of 'span' books).
Initialize new book @'seq_no'.
'seq_no++'
*/
struct book *book_next(struct heap_mgmt *hp)
{
	/* inval the book we might now clobber */
	book_inval(&hp->books[(hp->seq_no + hp->width - 1) % hp->span]);

	/* init new book */
	struct book *ret = &hp->books[hp->seq_no % hp->span];
	ret->seq_no = hp->seq_no++;
	ret->fi.source = source_assemble(hp, ret->seq_no % hp->span, &ret->data_offt);
	ret->hp = hp;
	Z_die_if((
		ffec_init(&hp->fp, &ret->fi,
			hp_pg(hp) * hp->width,
			ret->fi.source, ret->fi.parity, ret->fi.scratch,
			hp->dir,
			/* use seq as the RNG seed for FFEC */
			PCG_RAND_S2, ret->seq_no)
		), "");

	/* if this is the first book (TX || RX): seed */
	if (!ret->seq_no) {
		pcg_randset(ret->fi.source, hp_pg(hp) * (hp->width -1),
				PCG_RAND_S1, PCG_RAND_S2);
		/* if decoding, mark symbols in FFEC matrix */
		if (hp->dir == decode) {
			for (uint32_t i=0; i < hp->syms_page * (hp->width -1); i++)
				ffec_decode_sym(&hp->fp, &ret->fi, NULL, i);
		}

	/* otherwise for RX/decode: check previous book for all symbols already decoded
		and mark them decoded here.
	*/
	} else if (hp->dir == decode) {
		struct book *prev = book_get(hp, ret->seq_no -1);
		for (uint32_t i=esi_walk(hp, 0, 1); i < prev->fi.cnt.n; i++) {
			if (ffec_test_esi(&prev->fi, i))
				ffec_decode_sym(&hp->fp, &ret->fi, NULL, esi_walk(hp, i, -1));
		}
	}

	/* NOTE: TX/encode expects user to prepare "data" and then call book_encode() */

	return ret;
out:
	return NULL;
}

/*	book_encode()
Generate repair symbols for 'bk'.

If 'data' is provided, splice() the "data" (last) page from there;
	otherwise assume it was written to directly by caller, using book_data().

Generate and return an array of uint32_t ESIs as the (randomized) order
	in which to send symbols.
Caller must free returned array.
Returns NULL on error.
*/
uint32_t *book_encode(struct book *bk, void *data)
{
	int err_cnt = 0;
	uint32_t *ret = NULL;
	Z_die_if(!bk || bk->hp->dir != encode, "sanity");

	// TODO: splicing
	Z_die_if(data, "splicing not implemented");

	err_cnt = ffec_encode(&bk->hp->fp, &bk->fi);

	/* alloc and return randomized sequence of ESIs */
	Z_die_if(!(
		ret = malloc(book_tx_esi(bk) * sizeof(uint32_t))
		), "");
	ffec_esi_rand(&bk->fi, ret, bk->hp->syms_page * (bk->hp->width - 1));
out:
	return ret;
}


/*	heap_alloc()
*/
int heap_alloc(struct heap_mgmt *hp, uint32_t width, uint32_t syms_page,
		enum ffec_direction dir, struct ffec_params fp,
		uint64_t seq_no)
{
	int err_cnt = 0;
	Z_die_if(!hp, "sanity");
	memset(hp, 0x0, sizeof(*hp));

	hp->width = width;
	hp->span = hp->width * 2 - 1;	/* every region/array is of length 'span' */
	hp->syms_page = syms_page;
	hp->seq_no = seq_no;

	/* ffec params */
	hp->fp.fec_ratio = fp.fec_ratio;
	hp->fp.sym_len = fp.sym_len;
	hp->dir = dir;
	Z_die_if((
		ffec_calc_lengths(&hp->fp, hp_pg(hp) * hp->width, &hp->fs, dir)
		), "");

	/* verify that quested size parameters give a multiple of pagesize */
	size_t pgsz = getpagesize();
	Z_die_if(hp_pg(hp) / pgsz * pgsz != hp_pg(hp),
		"proposed pagesize %ld %% %ld system pagesize = %ld",
		hp_pg(hp), pgsz, hp_pg(hp) % pgsz);
	/* mmap() will complain if syms_page < 1024 */
	Z_die_if(syms_page < 1024, "");

	/* one mmap to rule them all ... and at process end unmap them
	Physical layout:
		N = width * 2 -2
		src0	...	srcN	par0	...	parN	scr0	...	scrN
	oh, and make that map a multiple of the system pagesize
	 */
	hp->the_one_ring.iov_base = 0;
	hp->the_one_ring.iov_len = next_mult64((hp->fs.source_sz + hp->fs.parity_sz + hp->fs.scratch_sz)
							* hp->span,
						pgsz);
	Z_die_if((
		hp->ring_fd = sbfu_tmp_map(&hp->the_one_ring, "./")
		) < 1, "");
	Z_inf(0, "map ring @ 0x%lx", (uint64_t)hp->the_one_ring.iov_base);
		
	/* Allocate and invalidate book structs */
	hp->books = calloc(hp->span, sizeof(struct book));
	for (size_t i=0; i < hp->span; i++)
		book_inval(&hp->books[i]);

	/* Parity and scratch regions are immutable,
		set the first pointer and then cycle through successive ones to increment.
	 */
	hp->books[0].fi.parity = hp->the_one_ring.iov_base + hp->fs.source_sz * hp->span;
	hp->books[0].fi.scratch = hp->books[0].fi.parity + hp->fs.parity_sz * hp->span;
	for (int i = 1; i < hp->span; i++) {
		hp->books[i].fi.parity = hp->books[i-1].fi.parity + hp->fs.parity_sz;
		hp->books[i].fi.scratch = hp->books[i-1].fi.scratch + hp->fs.scratch_sz;
	}

out:
	return err_cnt;
}

/*	heap_free()
*/
void heap_free(struct heap_mgmt *hp)
{
	if (!hp)
		return;

	if (hp->books) {
		for (int i=0; i < hp->span; i++)
			book_inval(&hp->books[i]);
		free(hp->books);
	}

	if (hp->ring_fd > 0)
		sbfu_unmap(hp->ring_fd, &hp->the_one_ring);
	hp->ring_fd = 0;
}


/*	multi_decode_()
Decode all ESIs in j1_esi into the FEC matrix for 'bk';
	ASSUMING symbol(s) are already present in the source region of the matrix;
	gathering any additional ESIs generated by decoding;
	recursing where source symbols overlap with other matrices.

Append the address of any NEW symbols generated
	(aka: not present in 'j1_esi'; not known about by caller)
	to j1_out_sym.

NOTE: we free all memory pointed to by 'j1_esi', caller should forget about it.

WARNING: internal - not for library callers to use directly.
*/
void multi_decode_(	struct book	*bk,
			Pvoid_t		j1_esi,
			Pvoid_t		*j1_out_sym)
{
	int rc;
	struct heap_mgmt *hp = bk->hp;

	/* Decode all symbols in given J1 array.
	Store esi's of all symbols generated by decode.
	*/
	Pvoid_t	j1_decoded = NULL;
	Word_t esi = 0;
	J1F(rc, j1_esi, esi);
	while (rc) {
		/* See if this symbol has not already been recursed,
			and if missing then decode it.
		NOTE that _decode_sym_() will add ALL source ESIs decoded
			(including the current one) into 'j1_decoded'.
		*/
		J1T(rc, j1_decoded, esi);
		if (!rc) {
			if (!ffec_decode_sym_(&hp->fp, &bk->fi, NULL, esi, &j1_decoded))
				break;
		}
		/* check for next */
		J1N(rc, j1_esi, esi);
	}

	/* report the address of all NEW (not known about by caller) symbols decoded */
	esi = 0;
	J1F(rc, j1_decoded, esi);
	while (rc) {
		J1T(rc, j1_esi, esi);
		if (!rc)
			J1S(rc, *j1_out_sym, (Word_t)ffec_get_sym(&hp->fp, &bk->fi, esi));
		J1N(rc, j1_decoded, esi);
	}

	/*	example @ width=3
						(walk)
			P1			-2
			P1	P2		-1
	we here ->	P1	P2	P3	0 <- skip recursing into ourself
				P2	P3	1
					P3	2
	*/
	struct book *child;
	Pvoid_t j1_submit = NULL;

	/* recurse into matrices BEFORE us */
	for (int walk = 1 - hp->width; walk < 0; walk++) {
		/* skip invalid books */
		if (!(child = book_get(hp, bk->seq_no + walk)))
			continue;
		/* Take the highest source ESI for the child and shift it downwards
			by 'walk' to get the highest decoded ESI (our)
			which 'child' can decode.
		*/
		esi = esi_walk(hp, child->fi.cnt.n -1, walk);
		/* Now walk backward, populating 'j1_submit' for 'child' */
		J1L(rc, j1_decoded, esi);
		while (rc) {
			/* Notice that we walk 'esi' UP, since a low 'esi' for us
				is a higher 'esi' for an earlier book.
			*/
			J1S(rc, j1_submit, esi_walk(hp, esi, walk * -1))
			J1P(rc, j1_decoded, esi);
		}
		/* check if any symbols to submit, and if so recurse */
		J1C(esi, j1_submit, 0, -1);
		if (esi) {
			multi_decode_(child, j1_submit, j1_out_sym);
			j1_submit = NULL; /* multi_decode_() has freed it already */
		}
	}

	/* recurse into matrices AFTER us only if they are valid (already instantiated) */
	for (int walk = 1;
		bk->seq_no + walk < hp->seq_no && walk < hp->width; 
		walk++)
	{
		/* skip invalid books */
		if (!(child = book_get(hp, bk->seq_no + walk)))
			continue;
		/* Take ESI 0 for the child and shift it upward
			by 'walk' to get the lowest decoded ESI (for us)
			which 'child' can decode.
		*/
		esi = esi_walk(hp, 0, walk);
		/* Now walk forward, populating 'j1_submit' for 'child' */
		J1F(rc, j1_decoded, esi);
		while (rc) {
			/* Notice that we walk 'esi' DOWN, since a high 'esi' for us
				is a lower 'esi' for a later book.
			*/
			J1S(rc, j1_submit, esi_walk(hp, esi, walk * -1))
			J1N(rc, j1_decoded, esi);
		}
		/* check if any symbols to submit, and if so recurse */
		J1C(esi, j1_submit, 0, -1);
		if (esi) {
			multi_decode_(child, j1_submit, j1_out_sym);
			j1_submit = NULL; /* multi_decode_() has freed it already */
		}
	}

out:
	/* free up J1 array we were called with */
	J1FA(rc, j1_esi);
	/* free j1_decoded */
	J1FA(rc, j1_decoded);
}

/*	decode_single()
Submit a symbol for decoding.

Any NEW symbol addresses which have been decoded are added to 'j1_out_sym'.

Return symbols left to decode in matrix for 'seq_no',
	-1 on error.
*/
uint32_t decode_single(struct book	*bk,
		void			*symbol,
		uint32_t		esi,
		Pvoid_t			*j1_out_sym)
{
	/* bail if 'seq_no' doesn't yield a valid book, or if symbol is already decoded */
	if (!bk || ffec_test_esi(&bk->fi, esi))
		return -1;

	/* in case caller doesn't provide a J1 array for output */
	Pvoid_t j1_stack = NULL;
	if (!j1_out_sym)
		j1_out_sym = &j1_stack;

	/* get address of symbol in matrix, put it there, log it as a new symbol */
	int rc;
	void *mtx = ffec_get_sym(&bk->hp->fp, &bk->fi, esi);
	memcpy(mtx, symbol, bk->hp->fp.sym_len);	// TODO: becomes splice()
	J1S(rc, *j1_out_sym, (Word_t)mtx);

	/* sumbit for multidimensional decode */
	Pvoid_t j1_esi = NULL;
	J1S(rc, j1_esi, esi);
	multi_decode_(bk, j1_esi, j1_out_sym);

	/* if caller didn't ask to be notified, free J1 memory */
	if (j1_stack)
		J1FA(rc, j1_stack);
	/* return number of symbols left to decode */
	return bk->fi.cnt.k - bk->fi.cnt.k_decoded;
}


/*	page_rand()
*/
void page_rand(void *data, size_t len)
{
	/* get seeds from /dev/urandom */
	uint64_t seeds[2] = { 0 };
	while (syscall(SYS_getrandom, seeds, sizeof(seeds), 0) != sizeof(seeds))
		usleep(100000);
	/* populate page with random data */
	pcg_randset(data, len, seeds[0], seeds[1]);
}


/*	main()
*/
int main()
{
	const int syms_page = 1024;

	int err_cnt;
	int rc;
	struct heap_mgmt TX = { 0 };
	struct heap_mgmt RX = { 0 };

	struct book *bk_tx, *bk_rx;
	uint32_t *esi_seq = NULL;
	Pvoid_t j1_decoded = NULL;

	Z_die_if(
		heap_alloc(&TX, 2, syms_page, encode, 
			(struct ffec_params){ .sym_len = 1024, .fec_ratio = 1.01 },
			0)
		, "");
	Z_die_if(
		heap_alloc(&RX, 2, syms_page, decode, 
			(struct ffec_params){ .sym_len = 1024, .fec_ratio = 1.01 },
			0)
		, "");


	/* set up TX book and encode */
	Z_die_if(!(
		bk_tx = book_next(&TX)			/* new book @ TX (seeded automatically) */
		), "");
	page_rand(book_data(bk_tx), hp_pg(&TX));	/* generate data block */
	Z_die_if(!(
		esi_seq = book_encode(bk_tx, NULL)	/* encode, get randomized ESI sequence */
		), "");


	/* set up RX book */
	Z_die_if(!(
		bk_rx = book_next(&RX)			/* new book @ RX */
		), "");
	/* decode symbols until done */
	uint32_t i;
	for (i=0; i < book_tx_esi(bk_tx); i++) {
		if (!decode_single(bk_rx,
				ffec_get_sym(&bk_rx->hp->fp, &bk_rx->fi, esi_seq[i]),
				esi_seq[i],
				&j1_decoded))
			break;
	}
	// TODO: verify that ESIs are reported in j1_decoded
	/* verify matrices are identical */
	Z_die_if(
		ffec_mtx_cmp(&bk_tx->fi, &bk_rx->fi, &bk_tx->hp->fp)
		, "");
	/* verify memory is identical */
	Z_die_if(
		memcmp(bk_tx->fi.source, bk_rx->fi.source, bk_tx->hp->fs.source_sz)
		, "");

	/* print efficiency */
	Z_inf(0, "decoded with k=%d < i=%d < n=%d;\n\
\tinefficiency=%lf; loss tolerance=%.2lf%%",
		bk_rx->fi.cnt.k, i, bk_rx->fi.cnt.n,
		(double)i / (double)bk_rx->fi.cnt.k,
		((double)(bk_rx->fi.cnt.n - i) / (double)bk_rx->fi.cnt.n) * 100);

out:
	if (esi_seq)
		free(esi_seq);
	if (j1_decoded)
		J1FA(rc, j1_decoded);
	heap_free(&TX);
	heap_free(&RX);
	return err_cnt;
}
