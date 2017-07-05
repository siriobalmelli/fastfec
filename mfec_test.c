

#include <stdint.h>
#include <sys/mman.h>

#ifndef _GNU_SOURCE
	#define _GNU_SOURCE	/* /dev/urandom; syscalls */
#endif
#include <unistd.h>
#include <sys/syscall.h>
#include <linux/random.h>

#include <mfec.h>

/*	page_rand()
Utility to generate random source data
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


/* program defaults */
double fec_ratio = 1.02;
int syms_page = 2048; /* this makes a 2M (hopefully one-day-to-be) hugepage */
int sym_len = 1024;
int width = 4;

/*	test_single()
Test single encode->decode
*/
int test_single()
{
	int err_cnt;
	int rc;
	struct mfec_hp TX = { 0 };
	struct mfec_hp RX = { 0 };

	struct mfec_bk *bk_tx, *bk_rx;
	uint32_t *esi_seq = NULL;

	Z_die_if(
		mfec_hp_init(&TX, width, syms_page, encode, 
			(struct ffec_params){ .sym_len = sym_len, .fec_ratio = fec_ratio },
			0)
		, "");
	Z_die_if(
		mfec_hp_init(&RX, width, syms_page, decode, 
			(struct ffec_params){ .sym_len = sym_len, .fec_ratio = fec_ratio },
			0)
		, "");


	/* set up TX book and encode */
	Z_die_if(!(
		bk_tx = mfec_bk_next(&TX)		/* new book @ TX (seeded automatically) */
		), "");
	page_rand(mfec_bk_data(bk_tx), mfec_pg(&TX));	/* generate data block */
	Z_die_if(!(
		esi_seq = mfec_encode(bk_tx, NULL)	/* encode, get randomized ESI sequence */
		), "");


	/* set up RX book */
	Z_die_if(!(
		bk_rx = mfec_bk_next(&RX)		/* new book @ RX */
		), "");
	/* decode symbols until done */
	uint64_t i;
	for (i=0; i < mfec_bk_txesi_cnt(bk_tx); i++) {
		if (!mfec_decode(bk_rx,
				ffec_get_sym(&bk_tx->hp->fp, &bk_tx->fi, esi_seq[i]),
				esi_seq[i]))
			break;
	}

	/* verify memory is identical */
	Z_die_if(
		memcmp(bk_tx->fi.source, bk_rx->fi.source, bk_tx->hp->fs.source_sz)
		, "");

	/* print efficiency */
	Z_log(Z_inf, "decoded with pg=%ld < i=%ld < (pg+p)=%ld;\n\
\tinefficiency=%lf; loss tolerance=%.2lf%%; FEC=%.2lf%%",
		/*pg*/TX.syms_page, /*i*/i, /*pg+p*/TX.syms_page + bk_tx->fi.cnt.p,
		/*inefficiency*/(double)i / (double)TX.syms_page,
		/*loss tolerance*/((double)(TX.syms_page + bk_tx->fi.cnt.p - i)
			/ ((double)TX.syms_page + bk_tx->fi.cnt.p)) * 100,
		/*FEC*/(bk_tx->hp->fp.fec_ratio -1) * 100 * TX.width);

out:
	if (esi_seq)
		free(esi_seq);
	mfec_hp_clean(&TX);
	mfec_hp_clean(&RX);
	return err_cnt;
}

/*	test_circular()
Test multiple single page encode->decode cycles,
	so as to verify circular buffer mechanics,
	and show decode efficiency as it evolves with time (should stay the same).
*/
int test_circular()
{
	int err_cnt;
	int rc;
	struct mfec_hp TX = { 0 };
	struct mfec_hp RX = { 0 };

	struct mfec_bk *bk_tx, *bk_rx;
	uint32_t *esi_seq = NULL;

	/* Instantiate a single instance of TX and RX;
		we will use single pages from these over and over to
		verify circular buffer mechanics.
	*/
	Z_die_if(
		mfec_hp_init(&TX, width, syms_page, encode,
			(struct ffec_params){ .sym_len = sym_len, .fec_ratio = fec_ratio },
			0)
		, "");
	Z_die_if(
		mfec_hp_init(&RX, width, syms_page, decode,
			(struct ffec_params){ .sym_len = sym_len, .fec_ratio = fec_ratio },
			0)
		, "");

	/* Loop for 'span' pages, each span being 'width' wide
		 and execute encode/decode cycle */
	for (int s = 0; s < TX.span * 2; s++) {

		/* set up TX book and encode */
		Z_die_if(!(
			bk_tx = mfec_bk_next(&TX)		/* new book @ TX (seeded automatically) */
			), "");
		page_rand(mfec_bk_data(bk_tx), mfec_pg(&TX));	/* generate data block */
		Z_die_if(!(
			esi_seq = mfec_encode(bk_tx, NULL)	/* encode, get randomized ESI sequence */
			), "");


		/* set up RX book */
		Z_die_if(!(
			bk_rx = mfec_bk_next(&RX)		/* new book @ RX */
			), "");
		/* decode symbols until done */
		uint64_t i;
		for (i=0; i < mfec_bk_txesi_cnt(bk_tx); i++) {
			if (!mfec_decode(bk_rx,
					ffec_get_sym(&bk_tx->hp->fp, &bk_tx->fi, esi_seq[i]),
					esi_seq[i]))
				break;
		}

		/* verify memory is identical */
		Z_die_if(
			memcmp(bk_tx->fi.source, bk_rx->fi.source, bk_tx->hp->fs.source_sz)
			, "s=%d", s);

#if 0
		/* print efficiency */
		Z_log(Z_inf, "decoded with pg=%ld < i=%ld < (pg+p)=%ld;\n\
			\tinefficiency=%lf; loss tolerance=%.2lf%%; FEC=%.2lf%%",
			/*pg*/TX.syms_page, /*i*/i, /*pg+p*/TX.syms_page + bk_tx->fi.cnt.p,
			/*inefficiency*/(double)i / (double)TX.syms_page,
			/*loss tolerance*/((double)(TX.syms_page + bk_tx->fi.cnt.p - i)
				/ ((double)TX.syms_page + bk_tx->fi.cnt.p)) * 100,
			/*FEC*/(bk_tx->hp->fp.fec_ratio -1) * 100 * TX.width);
#endif

		/* Clean up after each iteration */
		if (esi_seq)
			free(esi_seq);
	}

/* Clean up only if there was an error.*/
out:
	if (esi_seq)
		free(esi_seq);
	mfec_hp_clean(&TX);
	mfec_hp_clean(&RX);
	return err_cnt;
}

/*	test_multi()
Test 'width' encode->decode simultaneously
*/
int test_multi()
{
	int err_cnt;
	int rc;
	struct mfec_hp TX = { 0 };
	struct mfec_hp RX = { 0 };

	size_t arr_sz = sizeof(void*) * width;
	struct mfec_bk **bk_tx = alloca(arr_sz);
	struct mfec_bk **bk_rx = alloca(arr_sz);
	uint32_t **esi_seq = alloca(arr_sz);
	memset(bk_tx, 0x0, arr_sz);
	memset(bk_rx, 0x0, arr_sz);
	memset(esi_seq, 0x0, arr_sz);

	Z_die_if(
		mfec_hp_init(&TX, width, syms_page, encode, 
			(struct ffec_params){ .sym_len = sym_len, .fec_ratio = fec_ratio },
			0)
		, "");
	Z_die_if(
		mfec_hp_init(&RX, width, syms_page, decode, 
			(struct ffec_params){ .sym_len = sym_len, .fec_ratio = fec_ratio },
			0)
		, "");

	/* generate all TX blocks simulatenously */
	for (uint32_t w=0; w < width; w++) {
		/* set up TX book and encode */
		Z_die_if(!(
			bk_tx[w] = mfec_bk_next(&TX)		/* new book @ TX (seeded automatically) */
			), "");
		page_rand(mfec_bk_data(bk_tx[w]), mfec_pg(&TX));/* generate data block */
		Z_die_if(!(
			esi_seq[w] = mfec_encode(bk_tx[w], NULL)/* encode, get randomized ESI sequence */
			), "");
	}

	/* set up all RX books before decoding anything */
	for (uint32_t w=0; w < width; w++) {
		/* set up RX book */
		Z_die_if(!(
			bk_rx[w] = mfec_bk_next(&RX)		/* new book @ RX */
			), "");
	}

	/* decode symbols round-robin across all books until done */
	uint64_t i;
	for (i=0; i < mfec_bk_txesi_cnt(bk_tx[0]); i++) {
		int done_cnt=0;
		for (int j=0; j < width; j++) {
			struct mfec_bk *bkTX = mfec_bk_get(&TX, j);
			struct mfec_bk *bkRX = mfec_bk_get(&RX, j);
			Z_die_if(!bkTX || !bkRX, "can't get books for seq %d", j);
			if (!mfec_decode(bkRX,
					ffec_get_sym(&bkTX->hp->fp, &bkTX->fi, esi_seq[j][i]),
					esi_seq[j][i]))
				done_cnt++;
		}
		if (done_cnt == width)
			break;
	}

	for (int j=0; j < width; j++) {
		/* verify memory is identical */
		Z_die_if(
			memcmp(bk_tx[j]->fi.source, bk_rx[j]->fi.source, bk_tx[j]->hp->fs.source_sz)
			, "j=%d", j);
	}

	/* print efficiency */
	Z_log(Z_inf, "decoded (average over %d width) pg=%ld < i=%ld < (pg+p)=%ld;\n\
\tinefficiency=%lf; loss tolerance=%.2lf%%; FEC=%.2lf%%",
		/*width*/width,
		/*pg*/TX.syms_page, /*i*/i, /*pg+p*/TX.syms_page + bk_rx[0]->fi.cnt.p,
		/*inefficiency*/(double)i / (double)TX.syms_page,
		/*loss tolerance*/((double)(TX.syms_page + bk_rx[0]->fi.cnt.p - i)
			/ ((double)TX.syms_page + bk_rx[0]->fi.cnt.p)) * 100,
		/*FEC*/(bk_tx[0]->hp->fp.fec_ratio -1) * 100 * TX.width);

out:
	for (uint32_t w=0; w < width; w++)
		if (esi_seq[w])
			free(esi_seq[w]);
	mfec_hp_clean(&TX);
	mfec_hp_clean(&RX);
	return err_cnt;
}

/*	test_multi_seq_drop()
Decode backwards from the last book, in sequence - to simulate loss of contiguous pages.
Show how early the first pages can be decoded.

"how early can we decode 0 and 1?"
0	1	2
	1	2	3
		2	3	4

*/
int test_multi_seq_drop()
{
	int err_cnt;
	int rc;
	struct mfec_hp TX = { 0 };
	struct mfec_hp RX = { 0 };

	size_t arr_sz = sizeof(void*) * width;
	struct mfec_bk **bk_tx = alloca(arr_sz);
	struct mfec_bk **bk_rx = alloca(arr_sz);
	uint32_t **esi_seq = alloca(arr_sz);
	memset(bk_tx, 0x0, arr_sz);
	memset(bk_rx, 0x0, arr_sz);
	memset(esi_seq, 0x0, arr_sz);

	Z_die_if(
		mfec_hp_init(&TX, width, syms_page, encode,
			(struct ffec_params){ .sym_len = sym_len, .fec_ratio = fec_ratio },
			0)
		, "");
	Z_die_if(
		mfec_hp_init(&RX, width, syms_page, decode,
			(struct ffec_params){ .sym_len = sym_len, .fec_ratio = fec_ratio },
			0)
		, "");

	/* generate all TX blocks simulatenously */
	for (uint32_t w=0; w < width; w++) {
		/* set up TX book and encode */
		Z_die_if(!(
			bk_tx[w] = mfec_bk_next(&TX)		/* new book @ TX (seeded automatically) */
			), "");
		page_rand(mfec_bk_data(bk_tx[w]), mfec_pg(&TX));/* generate data block */
		Z_die_if(!(
			esi_seq[w] = mfec_encode(bk_tx[w], NULL)/* encode, get randomized ESI sequence */
			), "");
	}

	/* set up all RX books before decoding anything */
	for (uint32_t w=0; w < width; w++) {
		/* set up RX book */
		Z_die_if(!(
			bk_rx[w] = mfec_bk_next(&RX)		/* new book @ RX */
			), "");
	}

	/* decode symbols round-robin across all books until done */
	uint64_t i;
	for (i=mfec_bk_txesi_cnt(bk_tx[0]); i > 0; i--) {
		int done_cnt=0;
		for (int j=width-1; j == 0; j--) {
			struct mfec_bk *bkTX = mfec_bk_get(&TX, j);
			struct mfec_bk *bkRX = mfec_bk_get(&RX, j);
			Z_die_if(!bkTX || !bkRX, "can't get books for seq %d", j);
			if (!mfec_decode(bkRX,
					ffec_get_sym(&bkTX->hp->fp, &bkTX->fi, esi_seq[j][i]),
					esi_seq[j][i]))
				done_cnt++;
		}
		if (done_cnt == width)
			break;

		for (int j=width-1; j == 0; j--) {
			/* verify memory is identical */
			Z_die_if(
				memcmp(bk_tx[j]->fi.source, bk_rx[j]->fi.source, bk_tx[j]->hp->fs.source_sz)
				, "j=%d", j);
		}
	}

	/* print efficiency */
	Z_log(Z_inf, "decoded (average over %d width) pg=%ld < i=%ld < (pg+p)=%ld;\n\
\tinefficiency=%lf; loss tolerance=%.2lf%%; FEC=%.2lf%%",
		/*width*/width,
		/*pg*/TX.syms_page, /*i*/i, /*pg+p*/TX.syms_page + bk_rx[0]->fi.cnt.p,
		/*inefficiency*/(double)i / (double)TX.syms_page,
		/*loss tolerance*/((double)(TX.syms_page + bk_rx[0]->fi.cnt.p - i)
			/ ((double)TX.syms_page + bk_rx[0]->fi.cnt.p)) * 100,
		/*FEC*/(bk_tx[0]->hp->fp.fec_ratio -1) * 100 * TX.width);

out:
	for (uint32_t w=0; w < width; w++)
		if (esi_seq[w])
			free(esi_seq[w]);
	mfec_hp_clean(&TX);
	mfec_hp_clean(&RX);
	return err_cnt;
}


/*	print_usage()
*/
void print_usage(char *pgm_name)
{
	printf(
"usage:\n\
%s	[-f <fec_ratio>] [-p <syms_page>] [-s <sym_len>] [-w <width>] [-h]\n\
\n\
fec_ratio	:	a fractional ratio >1.0 && <2.0\n\
		default: 1.02\n\
syms_page	:	number of symbols making up one \"page\" in multi-fec.\n\
		default: 2048\n\
sym_len		:	size of FEC symbols, in B. Must be a divisble by 4096 (pagesize)\n\
		default: 1024\n\
width		:	number of pages in a single matrix; number of matrices into\n\
				which a page overlaps.\n\
		default: 4\n",
		pgm_name);
}

/*	parse_opts()
*/
void parse_opts(int argc, char **argv)
{
	int opt;
	extern char* optarg; /* used by getopt to point to arg values given */
	while ((opt = getopt(argc, argv, "f:p:s:w:h")) != -1) {
		switch (opt) {
			case 'f':
				fec_ratio = atof(optarg);
				break;
			case 'p':
				syms_page = atol(optarg);
				break;
			case 's':
				sym_len = atol(optarg);
				break;
			case 'w':
				width = atol(optarg);
				break;
			default:
				print_usage(argv[0]);
				exit(1);
		}
	}
	printf("fec_ratio: %.2f\r\n", fec_ratio);
	printf("syms_page: %d\r\n", syms_page);
	printf("sym_len: %d\r\n", sym_len);
	printf("width: %d\r\n", width);
}

/*	main()
*/
int main(int argc, char **argv)
{
	parse_opts(argc, argv);
	/* adjust FEC ratio to width */
	fec_ratio = 1.0 + ((fec_ratio -1.0) / width);
	Z_log(Z_inf, "effective fec_ratio adjusted to %.4f to account for width %d",
		fec_ratio, width);
	Z_log(Z_inf, "effective region length (width * pg): %ldMB",
			(uint64_t)syms_page * sym_len * width / 1024 / 1024);

	int err_cnt;
	Z_log(Z_inf, "---- single-matrix test ----");
	err_cnt += test_single();

	Z_log(Z_inf, "---- single-circular matrix test ----");
	err_cnt += test_circular();

	Z_log(Z_inf, "---- multi test ----");
	err_cnt += test_multi();

	Z_log(Z_inf, "---- multi seq drop test ----");
	err_cnt += test_multi_seq_drop();
	return err_cnt;
}
