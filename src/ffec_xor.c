/*	ffec_xor.c

Performance-critical XOR code
*/

#include <ffec.h>

/*	ffec_xor_into_symbol_()
XOR 2 symbols, in blocks of FFEC_SYM_ALIGN bytes.
*/
void	__attribute__((hot))
	__attribute__((regparm(3)))
#ifdef __GCC__
	__attribute__((optimize("O3")))
	__attribute__((optimize("unroll-loops")))
#endif
	ffec_xor_into_symbol_	(const void *from, void *to, uint32_t sym_len)
{
	uint32_t i;
	for (i=0; i < sym_len / FFEC_SYM_ALIGN; i++) {
		__builtin_prefetch(from + FFEC_SYM_ALIGN, 0, 0);
		__builtin_prefetch(to + FFEC_SYM_ALIGN, 1, 0);

/* AVX-specific: speeeeed */
#ifdef __AVX__
		asm (
			/* vlddqu replaced with vmovdqu for speed */
			"vmovdqu	0x0(%[src]),	%%ymm8;"
			"vmovdqu	0x0(%[dst]),	%%ymm0;"
			"vpxor		%%ymm0,		%%ymm8,		%%ymm0;"
			"vmovdqu	%%ymm0,		0x0(%[dst]);"

			"vmovdqu	0x20(%[src]),	%%ymm9;"
			"vmovdqu	0x20(%[dst]),	%%ymm1;"
			"vpxor		%%ymm1,		%%ymm9,		%%ymm1;"
			"vmovdqu	%%ymm1,		0x20(%[dst]);"

			"vmovdqu	0x40(%[src]),	%%ymm10;"
			"vmovdqu	0x40(%[dst]),	%%ymm2;"
			"vpxor		%%ymm2,		%%ymm10,	%%ymm2;"
			"vmovdqu	%%ymm2,		0x40(%[dst]);"

			"vmovdqu	0x60(%[src]),	%%ymm11;"
			"vmovdqu	0x60(%[dst]),	%%ymm3;"
			"vpxor		%%ymm3,		%%ymm11,	%%ymm3;"
			"vmovdqu	%%ymm3,		0x60(%[dst]);"

			"vmovdqu	0x80(%[src]),	%%ymm12;"
			"vmovdqu	0x80(%[dst]),	%%ymm4;"
			"vpxor		%%ymm4,		%%ymm12,	%%ymm4;"
			"vmovdqu	%%ymm4,		0x80(%[dst]);"

			"vmovdqu	0xA0(%[src]),	%%ymm13;"
			"vmovdqu	0xA0(%[dst]),	%%ymm5;"
			"vpxor		%%ymm5,		%%ymm13,	%%ymm5;"
			"vmovdqu	%%ymm5,		0xA0(%[dst]);"

			"vmovdqu	0xC0(%[src]),	%%ymm14;"
			"vmovdqu	0xC0(%[dst]),	%%ymm6;"
			"vpxor		%%ymm6,		%%ymm14,	%%ymm6;"
			"vmovdqu	%%ymm6,		0xC0(%[dst]);"

			"vmovdqu	0xE0(%[src]),	%%ymm15;"
			"vmovdqu	0xE0(%[dst]),	%%ymm7;"
			"vpxor		%%ymm7,		%%ymm15,	%%ymm7;"
			"vmovdqu	%%ymm7,		0xE0(%[dst]);"

			:						/* output */
			: [src] "r" (from), [dst] "r" (to)		/* input */
			/* TODO: how to prove that not adding "memory" as a clobber
				(which slows things down!) is SAFE?
			*/
			:						/* clobber */
		);

/* This is the non-AVX approach for portability.
Rely on compiler unrolling loop for speed.
*/
#else
		const uintmax_t *p_f = from;
		uintmax_t *p_t = to;
		#ifndef __GNUC__
			#pragma unroll (FFEC_SYM_ALIGN / sizeof(uintmax_t))
		#endif
		for (int i=0; i < FFEC_SYM_ALIGN / sizeof(uintmax_t); i++)
			p_t[i] ^= p_f[i];
#endif

		from += FFEC_SYM_ALIGN;
		to += FFEC_SYM_ALIGN;
	}
}
