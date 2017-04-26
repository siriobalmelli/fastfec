/*
	NFEC	-	n-dimension FEC

------------------------ sample matrix ---------------------------------------------------------
source/page	:	4					// number of source symbols each "page"
parity/page	:	3

width		:	2					// nr. of pages in a matrix, overlap "depth"
page_cnt	:	width * 2 -1		:	3

num_source	:	page_cnt * sym_page	:	12	// total number of source symbols
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
*/
