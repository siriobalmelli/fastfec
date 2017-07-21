# FFEC
The Fast (and barebones) FEC library.

This library implements a basic (and fast) LDGM staircase FEC.
For some background data and math, please see:

- Design, Evaluation and Comparison of Four Large Block FEC Codecs ...
	Vincent Roca, Christoph Neumann - 19 May 2006
	https://hal.inria.fr/inria-00070770/document
- Systems of Linear Equations: Gaussian Elimination
	http://www.sosmath.com/matrix/system1/system1.html


## DESIGN
The design goal is a FEC codec optimized for:
	a. speed and memory footprint
	b. large objects: depending on 'fec_ratio' (see def. below),
		anything <=2GB should be OK.
	c. low fec_ratio's (i.e.: <1.2 ... aka <20%)
For these reasons it was chosen NOT to use gaussian elimination
	at the decoder.


##THREAD-SAFETY
NONE


##NOMENCLATURE

FEC		:	Forward Error Correction
			The practice of generating some additional data at the
				"sender", to be sent along with the original data,
				so that "receiver" can re-compute data lost
				in transit (as long as it is under a certain
				percentage).
LDGM		:	Low Density Generator Matrix
			Describes the method for implementing this type of FEC,
				where computations to be done on the data
				are represented in a "low density" (aka: sparse)
				matrix, which is also then used to GENERATE the
				FEC data (therefore: "generator").

symbol		:	A single "unit" of data in FEC.
			It is assumed that "symbol" is the payload of a single
				network packet, therefore <=9KB and >=1KB.
k		:	Number of "source" symbols.
			This is the data which must be recovered at the receiver.
n		:	Total number of symbols.
			This is "k" source symbols plus "p" parity symbols.
p		:	Number of "parity" symbols (created by XORing several source symbols together).
			AKA: "repair" symbols.
			p = n - k
esi		:	Encoding Symbol ID (zero-indexed).
fec_ratio	:	'n = fec_ratio * k'
			Example: "1.1" == "10% FEC"
inefficiency	:	"number of symbols needed to decode" / k
			This is always greater than 1, and denotes
				how many MORE symbols than source are needed
				to allow decoding.
			A perfect codec (e.g. reed-solomon) has inefficiency == 1,
				but the tradeoff of LDGM is (slight) inefficiency.

#BUGS
-	this should not fail:
```
admlocal@bad-t1:~/ffec$ build-release/test/ffec_test -o 2000000000 -f 1.9
[INF] ffec_test.c:092 +parse_opts     	:: sym_len: 1280
[INF] ffec_test.c:093 +parse_opts     	:: fec_ratio: 1.900000
[INF] ffec_test.c:094 +parse_opts     	:: original_sz: 2000000000
[INF] ffec_test.c:139 +main           	:: encode ELAPSED: 1279.20ms
[ERR]     ffec.c:251 +ffec_calc_lengths_	:: (src + par + scr > (uint64_t)UINT32_MAX -2) cannot handle combined symbol space of 5765000000
[ERR]     ffec.c:066 +ffec_new       	:: (ffec_calc_lengths_(fp, ret)) 
[ERR] ffec_test.c:159 +main           	:: (!( fi_decode = ffec_new(&fp, original_sz, NULL, fi_encode->seeds[0], fi_encode->seeds[1]) )) 
[ERR]  zed_dbg.h:218 +Z_end_         	:: ../src/ffec.c; global err_cnt 1
admlocal@bad-t1:~/ffec$ 
```

# TODO
-	check assembly: what does memory clobber of xor change?
-	validation -> benchmarking, aka pretty graphs
-	rename 'stack'; becomes nonlocking; release nonlibc
-	0-copy I/O in nonlibc; replace in ffec
-	64-bit performance (extremely large blocks?)

# IDEAS
-	split up seeds across many packets when transmitting:
		less overhead per-packet; depend on MANY of them arriving
