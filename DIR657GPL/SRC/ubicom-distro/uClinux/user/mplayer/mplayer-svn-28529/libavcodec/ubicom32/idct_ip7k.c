/*
 * mpeg2_thread.c
 *
 * Copyright © 2000-2008 Ubicom Inc <www.ubicom.com>.  All rights reserved.
 *
 * This file contains confidential information of Ubicom, Inc. and your use of
 * this file is subject to the Ubicom Software License Agreement distributed with
 * this file. If you are uncertain whether you are an authorized user or to report
 * any unauthorized use, please contact Ubicom, Inc. at +1-408-789-2200.
 * Unauthorized reproduction or distribution of this file is subject to civil and
 * criminal penalties.
 *
 * $RCSfile: mpeg2_thread.c,v $
 * $Date: 2008/12/09 01:50:37 $
 * $Revision: 1.1.4.2 $
 */
#include "libavcodec/dsputil.h"

//#include "hddconst.h"

/*
 * This array contains the firmware that needs to be loaded into the idct block
 */
static uint32_t idct_instructions[] = {

	0x00000000,	//; WORD=0	nop				; addr 0 is not executed
	0x009b0b19,	//; WORD=1	movei	r11,	#2841		; W1	(constants below)
	0x009c0a74,	//; WORD=2	movei	r12,	#2676		; W2
	0x009d0968,	//; WORD=3	movei	r13,	#2408		; W3
	0x009f0649,	//; WORD=4	movei	r15,	#1609		; W5
	0x009a0454,	//; WORD=5	movei	r10,	#1108		; W6
	0x009e0235,	//; WORD=6	movei	r14,	#565		; W7
	//;start:
	0x00d10000,	//; WORD=7	cmovei	mar,	#0		; re-entry point, saves W loads, above
	0x00d20002,	//; WORD=8	cmovei	loop,	#2		; simd=4 => 8 values in 2 iterations
	//;row:
	0x02d03000,	//; WORD=9	ldhs	r0,	#0		; x0
	0x02d93080,	//; WORD=10	ldhs	r9,	#0x80		; q0 @ offset 128 from coef
	0x02405009,	//; WORD=11	mpys	r0,	r0,	r9	; x0 = x0 * q0
	0x02d43010,	//; WORD=12	ldhs	r4,	#16		; x4
	0x02d93090,	//; WORD=13	ldhs	r9,	#0x90		; q4
	0x02445409,	//; WORD=14	mpys	r4,	r4,	r9	; x4 = x4 * q4
	0x02d33020,	//; WORD=15	ldhs	r3,	#32		; x3
	0x02d930a0,	//; WORD=16	ldhs	r9,	#0xa0		; q3
	0x02435309,	//; WORD=17	mpys	r3,	r3,	r9	; x3 = x3 * q3
	0x02d73030,	//; WORD=18	ldhs	r7,	#48		; x7
	0x02d930b0,	//; WORD=19	ldhs	r9,	#0xb0		; q7
	0x02475709,	//; WORD=20	mpys	r7,	r7,	r9	; x7 = x7 * q7
	0x02d13008,	//; WORD=21	ldhs	r1,	#8		; x1, pre-op
	0x02d93088,	//; WORD=22	ldhs	r9,	#0x88		; q1
	0x02415109,	//; WORD=23	mpys	r1,	r1,	r9	; x1 = x1 * q1
	0x02d63018,	//; WORD=24	ldhs	r6,	#24		; x6
	0x02d93098,	//; WORD=25	ldhs	r9,	#0x98		; q6
	0x02465609,	//; WORD=26	mpys	r6,	r6,	r9	; x6 = x6 * q6
	0x02d23028,	//; WORD=27	ldhs	r2,	#40		; x2
	0x02d930a8,	//; WORD=28	ldhs	r9,	#0xa8		; q2
	0x02425209,	//; WORD=29	mpys	r2,	r2,	r9	; x2 = x2 * q2
	0x02d53038,	//; WORD=30	ldhs	r5,	#56		; x5
	0x02d930b8,	//; WORD=31	ldhs	r9,	#0xb8		; q5
	0x02455509,	//; WORD=32	mpys	r5,	r5,	r9	; x5 = x5 * q5
	0x03237004,	//; WORD=33	pipeswap r0, r4, r3, r7		; prepare for 4 parallel rows first 1/2
	0x03225106,	//; WORD=34	pipeswap r1, r6, r2, r5		; prepare for 4 parallel rows 2nd 1/2
	0x0230000b,	//; WORD=35	lsl	r0,	r0,	#11	; fix x0: x0 = x0 << 11
	0x0231010b,	//; WORD=36	lsl	r1,	r1,	#11	; fix x1: x1 = x1 << 11
	0x02500080,	//; WORD=37	addi	r0,	r0,	#128	; x0 = (x0 << 11) + 128
	0x02490405,	//; WORD=38	add	r9,	r4,	r5	;; /* First Stage */ tmp = x4 + x5
	0x02485e09,	//; WORD=39	mpys	r8,	r14,	r9	; x8 = W7 * (x4 + x5)
	0x02491b0e,	//; WORD=40	sub	r9,	r11,	r14	; tmp = (W1 - W7)
	0x02445904,	//; WORD=41	mpys	r4,	r9,	r4	; intermed: x4 = (W1 - W7) * x4)
	0x02490b0e,	//; WORD=42	add	r9,	r11,	r14	; tmp = (W1 + W7)
	0x02440804,	//; WORD=43	add	r4,	r8,	r4	; x4 = x8 + (W1 - W7) * x4
	0x02455905,	//; WORD=44	mpys	r5,	r9,	r5	; intermed: x5 = (W1 + W7) * x5
	0x02490607,	//; WORD=45	add	r9,	r6,	r7	; tmp = (x6 + x7)
	0x02451805,	//; WORD=46	sub	r5,	r8,	r5	; x5 = x8 - (W1 + W7) * x5
	0x02485d09,	//; WORD=47	mpys	r8,	r13,	r9	; x8 = W3 * (x6 + x7)
	0x02491d0f,	//; WORD=48	sub	r9,	r13,	r15	; tmp = (W3 - W5)
	0x02465906,	//; WORD=49	mpys	r6,	r9,	r6	; intermed: x6 = (W3 - W5) * x6
	0x02490d0f,	//; WORD=50	add	r9,	r13,	r15	; tmp = (W3 + W5)
	0x02461806,	//; WORD=51	sub	r6,	r8,	r6	; x6 = x8 - (W3 - W5) * x6
	0x02475907,	//; WORD=52	mpys	r7,	r9,	r7	; intermed: x7 = (W3 + W5) * x7
	0x02471807,	//; WORD=53	sub	r7,	r8,	r7	; x7 = x8 - (W3 + W5) * x7
	0x03080001,	//; WORD=54	addsub	r8, r0, r0, r1		; /* Second Stage */ x8=x0+x1; x0 -= x1
	0x02490c0a,	//; WORD=55	add	r9,	r12,	r10	; tmp = (W2 + W6)
	0x02410302,	//; WORD=56	add	r1,	r3,	r2	; intermed: x1 = (x3 + x2)
	0x02425902,	//; WORD=57	mpys	r2,	r9,	r2	; intermed: x2 = (W2 + W6) * x2
	0x02415a01,	//; WORD=58	mpys	r1,	r10,	r1	; x1 = W6 * (x3 + x2)
	0x02491c0a,	//; WORD=59	sub	r9,	r12,	r10	; tmp = (W2 - W6)
	0x02421102,	//; WORD=60	sub	r2,	r1,	r2	; x2 = x1 - (W2 + W6) * x2
	0x02435903,	//; WORD=61	mpys	r3,	r9,	r3	; intermed: x3 = (W2 - W6) * x3
	0x02430103,	//; WORD=62	add	r3,	r1,	r3	; x3 = x1 + (W2 - W6) * x3
	0x03014406,	//; WORD=63	addsub	r1, r4, r4, r6		; x1 = x4 + x6; x4 = x4 - x6
	0x03065507,	//; WORD=64	addsub	r6, r5, r5, r7		; x6 = x5 + x7; x5 = x5 - x7
	//; start of original code *****************************
	// /* Third Stage */
	0x03078803,	//; WORD=65	addsub	r7, r8, r8, r3		; x7=x8+x3; x8 -= x3
	0x03030002,	//; WORD=66	addsub	r3, r0, r0, r2		; x3 = x0 + x2; x0 = x0 - x2
	//;;;;;;;; UBICOM_MPY_CHANGE ;;;;;;;;;;;;;
	0x03094405,	//; WORD=67	addsub	r9, r4, r4, r5		; intermed: tmp=x4+x5; x4=x4-x5
	//;;;;;;;;; start surgery
	//;	mpysi	r2,	r9,	#181	; intermed: x2 = 181 * (x4 + x5)
	//;	mpysi	r4,	r4,	#181	; intermed: x4 = 181 * (x4 - x5)
	//;	addi	r2,	r2,	#128	; intermed: x2 = (181 * (x4 + x5) + 128)
	//;	addi	r4,	r4,	#128	; intermed: x4 = (181 * (x4 - x5) + 128)
	//; implement 32 bit multipy of r2 and #181
	0x019000d2,	//; WORD=68	call	#210			; r9 * 181: r9 has init and return value
	0x02520980,	//; WORD=69	addi	r2,	r9,	#128	; intermed: x2 = (181 * (x4 + x5) + 128)
	//; implement 32 bit multipy of r4 and #181
	0x02091404,	//; WORD=70	or	r9,	r4,	r4	; copy r4, i.e. tmp = (x4 - x5)
	0x019000d2,	//; WORD=71	call	#210			; r9 * 181: r9 has init and return value
	0x02540980,	//; WORD=72	addi	r4,	r9,	#128	; intermed: x4 = (181 * (x4 - x5) + 128)
	//; restore constants
	0x009a0454,	//; WORD=73	movei	r10,	#1108		; W6
	0x009e0235,	//; WORD=74	movei	r14,	#565		; W7
	//;;;;;;;;;;; end surgery
	0x02322208,	//; WORD=75	asr	r2,	r2,	#8	; x2 = (181*(x4+x5)+128)>>8
	0x02342408,	//; WORD=76	asr	r4,	r4,	#8	; x4 = (181*(x4+x5)+128)>>8
	//;;;;;;;; end of UBICOM_MPY_CHANGE ;;;;;;;;;;;;;
	// /* Fourth stage */
	0x03071701,	//; WORD=77	addsub	r7, r1, r7, r1		; intermed: x7=x7+x1; x1=x7-x1
	0x03032302,	//; WORD=78	addsub	r3, r2, r3, r2		; intermed: x3=x3+x2; x2=x3-x2
	0x03004004,	//; WORD=79	addsub	r0, r4, r0, r4		; intermed: x0=x0+x4; x4=x0-x4
	0x03086806,	//; WORD=80	addsub	r8, r6, r8, r6		; intermed: x8=x8+x6; x6=x8-x6
	0x02372708,	//; WORD=81	asr	r7,	r7,	#8	; x7 = (x7 + x1) >> 8
	0x02332308,	//; WORD=82	asr	r3,	r3,	#8	; x3 = (x3 + x2) >> 8
	0x02302008,	//; WORD=83	asr	r0,	r0,	#8	; x0 = (x0 + x4) >> 8
	0x02382808,	//; WORD=84	asr	r8,	r8,	#8	; x8 = (x8 + x6) >> 8
	0x02362608,	//; WORD=85	asr	r6,	r6,	#8	; x6 = (x8 - x6) >> 8
	0x02342408,	//; WORD=86	asr	r4,	r4,	#8	; x4 = (x0 - x4) >> 8
	0x02322208,	//; WORD=87	asr	r2,	r2	#8	; x2 = (x3 - x2) >> 8
	0x02312108,	//; WORD=88	asr	r1,	r1,	#8	; x1 = (x7 - x1) >> 8
	0x03208703,	//; WORD=89	pipeswap r7, r3, r0, r8		; prepare data for stores
	0x03221604,	//; WORD=90	pipeswap r6, r4, r2, r1
	0x02f71000,	//; WORD=91	sth	r7,	#0		; (x7 + x1) >> 8
	0x02f31010,	//; WORD=92	sth	r3	#16		; (x3 + x2) >> 8
	0x02f01020,	//; WORD=93	sth	r0,	#32		; (x0 + x4) >> 8
	0x02f81030,	//; WORD=94	sth	r8,	#48		; (x8 + x6) >> 8
	0x02f61008,	//; WORD=95	sth	r6,	#8		; (x8 - x6) >> 8
	0x02f41018,	//; WORD=96	sth	r4,	#24		; (x0 - x4) >> 8
	0x02f21028,	//; WORD=97	sth	r2,	#40		; (x3 - x2) >> 8
	0x02f11038,	//; WORD=98	sth	r1	#56		; (x7 - x1) >> 8
	//; end of original code *****************************
	0x00f10040,	//; WORD=99	cinc	mar	#64		; set base addr for next iteration
	0x01d00009,	//; WORD=100	decjmpnz	row
	0x00d10000,	//; WORD=101	cmovei	mar,	#0		; /* col iDCT */
	0x00d20002,	//; WORD=102	cmovei	loop,	#2		; simd=4 => 8 values in 2 iterations
	//;col:
	0x02d03000,	//; WORD=103	ldhs	r0,	#0		; x0 pre-op
	0x02d43010,	//; WORD=104	ldhs	r4,	#16		; x4
	0x02d33020,	//; WORD=105	ldhs	r3,	#32		; x3
	0x02d73030,	//; WORD=106	ldhs	r7,	#48		; x7
	0x02d13040,	//; WORD=107	ldhs	r1,	#64		; x1 pre-op
	0x02d63050,	//; WORD=108	ldhs	r6,	#80		; x6
	0x02d23060,	//; WORD=109	ldhs	r2,	#96		; x2
	0x02d53070,	//; WORD=110	ldhs	r5,	#112		; x5
	0x02300008,	//; WORD=111	lsl	r0,	r0,	#8	; intermed: x = (x0 << 8)
	0x00992000,	//; WORD=112	movei	r9,	#8192		; tmp = 8192
	0x02310108,	//; WORD=113	lsl	r1,	r1,	#8	; x1 << 8
	0x02400009,	//; WORD=114	add	r0,	r0,	r9	; x0 = (x0 << 8) + 8192
	// /* First Stage */
	//; expand terms of multiply
	0x02495e04,	//; WORD=115	mpys	r9,	r14,	r4	; intermed: tmp = W7 * x4
	0x02485e05,	//; WORD=116	mpys	r8,	r14,	r5	; intermed: x8 = W7 * x5
	0x02480809,	//; WORD=117	add	r8,	r8,	r9	; intermed: x8 = W7 * (x4 + x5)
	//; end of expand multiply
	0x02580804,	//; WORD=118	addi	r8,	r8,	#4	; x8 = W7 * (x4 + x5) + 4
	0x02491b0e,	//; WORD=119	sub	r9,	r11,	r14	; tmp = (W1 - W7)
	0x02445904,	//; WORD=120	mpys	r4,	r9,	r4	; intermed: x4 = (W1 - W7) * x4
	0x02440804,	//; WORD=121	add	r4,	r8,	r4	; intermed: x4 = (x8 + (W1 - W7) * x4)
	0x02342403,	//; WORD=122	asr	r4,	r4,	#3	; x4 = (x8 + (W1 - W7) * x4) >> 3
	0x02490b0e,	//; WORD=123	add	r9,	r11,	r14	; tmp = (W1 + W7)
	0x02455905,	//; WORD=124	mpys	r5,	r9,	r5	; intermed: x5 = (W1 + W7) * x5
	0x02451805,	//; WORD=125	sub	r5,	r8,	r5	; intermed: x5 = (x8 - (W1 + W7) * x5)
	0x02352503,	//; WORD=126	asr	r5,	r5,	#3	; x5 = (x8 - (W1 + W7) * x5) >> 3
	//; expand terms of multiply
	0x02495d06,	//; WORD=127	mpys	r9,	r13,	r6	; tmp = W3 * x6
	0x02485d07,	//; WORD=128	mpys	r8,	r13,	r7	; intermed: x8 = W3 * x7
	0x02480809,	//; WORD=129	add	r8,	r8,	r9	; intermed: x8 = W3 (x6 + x7)
	//; end of expand multiply
	0x02580804,	//; WORD=130	addi	r8,	r8,	#4	; x8 = W3 * (x6 + x7) + 4
	0x02491d0f,	//; WORD=131	sub	r9,	r13,	r15	; tmp = (W3 - W5)
	0x02465906,	//; WORD=132	mpys	r6,	r9,	r6	; intermed: x6 = (W3 - W5) * x6
	0x02490d0f,	//; WORD=133	add	r9,	r13,	r15	; tmp = (W3 + W5)
	0x02461806,	//; WORD=134	sub	r6,	r8,	r6	; intermed: x6 = (x8 - (W3 - W5) * x6)
	0x02475907,	//; WORD=135	mpys	r7,	r9,	r7	; intermed: x7 = (W3 + W5) * x7
	0x02362603,	//; WORD=136	asr	r6,	r6,	#3	; x6 = (x8 - (W3 - W5) * x6) >> 3
	0x02471807,	//; WORD=137	sub	r7,	r8,	r7	; intermed: x7 = (x8 - (W3 + W5) * x7)
	0x02372703,	//; WORD=138	asr	r7,	r7,	#3	; x7 = (x8 - (W3 + W5) * x7) >> 3
	0x03080001,	//; WORD=139	addsub	r8, r0, r0, r1		; /* Second Stage */ x8=x0+x1; x0 -= x1
	//; expand terms of multiply
	0x02495a03,	//; WORD=140	mpys	r9,	r10,	r3	; tmp = W6 * x3
	0x02415a02,	//; WORD=141	mpys	r1,	r10,	r2	; intermed: x1 = W6 * x2
	0x02410901,	//; WORD=142	add	r1,	r9,	r1	; intermed: x1 = W6 * (x3 + x2)
	//; end of expand multiply
	0x02490c0a,	//; WORD=143	add	r9,	r12,	r10	; tmp = (W2 + W6)
	0x02510104,	//; WORD=144	addi	r1,	r1,	#4	; x1 = W6 * (x3 + x2) + 4
	0x02425902,	//; WORD=145	mpys	r2,	r9,	r2	; intermed: x2 = (W2 + W6) * x2
	0x02491c0a,	//; WORD=146	sub	r9,	r12,	r10	; tmp = (W2 - W6)
	0x02421102,	//; WORD=147	sub	r2,	r1,	r2	; intermed: x2 = (x1 - (W2 + W6) * x2)
	0x02435903,	//; WORD=148	mpys	r3,	r9,	r3	; intermed: x3 = (W2 - W6) * x3
	0x02322203,	//; WORD=149	asr	r2,	r2,	#3	; x2 = (x1 - (W2 + W6) * x2) >> 3
	0x02430103,	//; WORD=150	add	r3,	r1,	r3	; intermed: x3 = (x1 + (W2 - W6) * x3)
	0x03014406,	//; WORD=151	addsub	r1, r4, r4, r6		; x1 = x4 + x6; x4 = x4 - x6
	0x02332303,	//; WORD=152	asr	r3,	r3,	#3	; x3 = (x1 + (W2 - W6) * x3) >> 3
	0x03065507,	//; WORD=153	addsub	r6, r5, r5, r7		; x6 = x5 + x7; x5 = x5 - x7
	//; start of original code *****************************
	0x03078803,	//; WORD=154	addsub	r7, r8, r8, r3		; /* Third Stage */ x7=x8+x3; x8 -= x3
	0x03030002,	//; WORD=155	addsub	r3, r0, r0, r2		; x3 = x0 + x2; x0 = x0 - x2
	0x03094405,	//; WORD=156	addsub	r9, r4, r4, r5		; intermed: tmp = x4 + x5; x4 = x4 - x5
	//;;;;;;;;; start surgery
	//;	mpysi	r2,	r9,	#181	; intermed: x2 = (181 * (x4 + x5))
	//;	mpysi	r4,	r4,	#181	; intermed: x4 = (181 * (x4 - x5))
	//;	addi	r2,	r2,	#128	; intermed: x2 = (181 * (x4 + x5) + 128)
	//;	addi	r4,	r4,	#128	; intermed: x4 = (181 * (x4 - x5) + 128)
	//; implement 32 bit multipy of r2 and #181
	0x019000d2,	//; WORD=157	call	#210			; 181 * r9: r9 has init and return value
	0x02520980,	//; WORD=158	addi	r2,	r9,	#128	; intermed: x2 = (181 * (x4 + x5) + 128)
	//; implement 32 bit multipy of r4 and #181
	0x02091404,	//; WORD=159	or	r9,	r4,	r4	; copy r4, i.e. tmp = (x4 - x5)
	0x019000d2,	//; WORD=160	call	#210			; 181 * r9: r9 has init and return value
	0x02540980,	//; WORD=161	addi	r4,	r9,	#128	; intermed: x4 = (181 * (x4 - x5) + 128)
	//; restore constants
	0x009a0454,	//; WORD=162	movei	r10,	#1108		; W6
	0x009e0235,	//; WORD=163	movei	r14,	#565		; W7
	//;;;;;;;;;;; end surgery
	0x02322208,	//; WORD=164	asr	r2,	r2,	#8	; x2 = (181 * (x4 + x5) + 128) >> 8
	0x02342408,	//; WORD=165	asr	r4,	r4,	#8	; x4 = (181 * (x4 - x5) + 128) >> 8
	0x03071701,	//; WORD=166	addsub	r7, r1, r7, r1		; /* Fourth Stage */ intermed: x7 = x7 + x1; x1 = x7 - x1
	0x03032302,	//; WORD=167	addsub	r3, r2, r3, r2		; intermed: x3 = x3 + x2; x2 = x3 - x2
	0x03004004,	//; WORD=168	addsub	r0, r4, r0, r4		; intermed: x0 = x0 + x4; x4 = x0 - x4
	0x03086806,	//; WORD=169	addsub	r8, r6, r8, r6		; intermed: x8 = x8 + x6; x6 = x8 - x6
	0x0237270e,	//; WORD=170	asr	r7,	r7,	#14	; x7 = (x7 + x1) >> 14
	0x0233230e,	//; WORD=171	asr	r3,	r3,	#14	; x3 = (x3 + x2) >> 14
	0x0230200e,	//; WORD=172	asr	r0,	r0,	#14	; x0 = (x0 + x4) >> 14
	0x0238280e,	//; WORD=173	asr	r8,	r8,	#14	; x8 = (x8 + x6) >> 14
	0x0236260e,	//; WORD=174	asr	r6,	r6,	#14	; x6 = (x8 - x6) >> 14
	0x0234240e,	//; WORD=175	asr	r4,	r4,	#14	; x4 = (x0 - x4) >> 14
	0x0232220e,	//; WORD=176	asr	r2,	r2,	#14	; x2 = (x3 - x2) >> 14
	0x0231210e,	//; WORD=177	asr	r1,	r1,	#14	; x1 = (x7 - x1) >> 14
	0x02773708,	//; WORD=178	sat	r7,	r7,	#8	; SatSigned9b(x7 + x1) >> 14
	0x02733308,	//; WORD=179	sat	r3,	r3,	#8	; SatSigned9b(x3 + x2) >> 14
	0x02703008,	//; WORD=180	sat	r0,	r0,	#8	; SatSigned9b(x0 + x4) >> 14
	0x02783808,	//; WORD=181	sat	r8,	r8,	#8	; SatSigned9b(x8 + x6) >> 14
	0x02763608,	//; WORD=182	sat	r6,	r6,	#8	; SatSigned9b(x8 - x6) >> 14
	0x02743408,	//; WORD=183	sat	r4,	r4,	#8	; SatSigned9b(x0 - x4) >> 14
	0x02723208,	//; WORD=184	sat	r2,	r2,	#8	; SatSigned9b(x3 - x2) >> 14
	0x02713108,	//; WORD=185	sat	r1,	r1,	#8	; SatSigned9b(x7 - x1) >> 14
	0x02f71000,	//; WORD=186	sth	r7,	#0		; pi16Clip((x7 + x1) >> 14)
	0x02f31010,	//; WORD=187	sth	r3,	#16		; pi16Clip((x3 + x2) >> 14)
	0x02f01020,	//; WORD=188	sth	r0,	#32		; pi16Clip((x0 + x4) >> 14)
	0x02f81030,	//; WORD=189	sth	r8,	#48		; pi16Clip((x8 + x6) >> 14)
	0x02f61040,	//; WORD=190	sth	r6,	#64		; pi16Clip((x8 - x6) >> 14)
	0x02f41050,	//; WORD=191	sth	r4,	#80		; pi16Clip((x0 - x4) >> 14)
	0x02f21060,	//; WORD=192	sth	r2,	#96		; pi16Clip((x3 - x2) >> 14)
	0x02f11070,	//; WORD=193	sth	r1,	#112		; pi16Clip((x7 - x1) >> 14)
	//; end of original code *****************************
	0x00f10008,	//; WORD=194	cinc	mar,	#8		; set base addr for next iteration
	0x01d00067,	//; WORD=195	decjmpnz	col
	0x00d10000,	//; WORD=196	cmovei	mar,	#0		; superflous; prevents nasty surprises
	0x00400000,	//; WORD=197	int				; done
	0x00300001,	//; WORD=198	stop	#1			; return code = 1
	0x01700007,	//; WORD=199	jmp	start			; set up for next round
	0x00000000,	//; WORD=200	.org 210
	0x00000000,	//; WORD=201	.org 210
	0x00000000,	//; WORD=202	.org 210
	0x00000000,	//; WORD=203	.org 210
	0x00000000,	//; WORD=204	.org 210
	0x00000000,	//; WORD=205	.org 210
	0x00000000,	//; WORD=206	.org 210
	0x00000000,	//; WORD=207	.org 210
	0x00000000,	//; WORD=208	.org 210
	0x00000000,	//; WORD=209	.org 210
	//;.org 210:
	//;mpy_16x32:
	//; implement multiplication of a 32-bit number by constant #181
	//; assumes
	//;	r9 has the input value and updates it to the product
	//;	r14 has the mask value 0xffff
	//; uses (corrupts) r10, which should be restored at calling point
	0x009effff,	//; WORD=210	movei	r14,	#0xffff		; 16 bit mask
	0x023a2910,	//; WORD=211	asr	r10,	r9,	#16	; r10 has upper 16 bits
	0x0209290e,	//; WORD=212	and	r9,	r9,	r14	; r9 = r9 & 0xffff; so r9 has lower 16b
	0x025a5ab5,	//; WORD=213	mpysi	r10,	r10,	#181
	0x025949b5,	//; WORD=214	mpyui	r9,	r9,	#181	; lower bits multiplied UNsigned!
	0x023a0a10,	//; WORD=215	lsl	r10,	r10,	#16	; shift upper 16 * 181
	0x02490a09,	//; WORD=216	add	r9,	r10,	r9	; r9 now has result
	0x01000000,	//; WORD=217	ret
	0x00000000,	//; WORD=218	.org	220
	0x00000000,	//; WORD=219	.org	220
	//;.org	220:
	//;service_routine:
	//; clear the mar register
	0x00400000,	//; WORD=220	int
	0x00300002,	//; WORD=221	stop	2
	//cmovei	mar, #128
	//; increment the service routine counter
	//ldhu	r0, #1
	//addi	r0, r0, #1
	//sth	r0, #1
	//; nops replaces debug routine for now
	0x00000000,	//; WORD=222	nop
	0x00000000,	//; WORD=223	nop
	0x00000000,	//; WORD=224	nop
	0x00000000,	//; WORD=225	nop
	0x00000000,	//; WORD=226	nop
	0x00000000,	//; WORD=227	nop
	0x00400000,	//; WORD=228	int
	0x00300002,	//; WORD=229	stop	2

	//; pad with nops to 256 total values
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000
    };

/*
 * struct to access idct command registers
 */
typedef struct idct_ctrl_port {
	uint32_t status;
		// bits:
		//	busy : 4
		//	code : 3:0
	uint32_t cmd;
		// bits:
		//	valid : 7
		//	mem   : 6:4
		//		0 => reg
		//		1 => inst
		//		2 => data
		//		4 => PC register
		//		5 => LOOP register
		//		6 => MAR register
		//		7 => RET register
	uint32_t addr;
	uint32_t write_data;
	uint32_t read_data;
} idct_ctrl_port_t, *idct_ctrl_port_p;

#define IDCT_ENGINE_BASE_ADDRESS 0x01000b00
uint32_t *hw_idct_base = (uint32_t *)(IDCT_ENGINE_BASE_ADDRESS);
uint32_t *hw_idct_data_base = (uint32_t *)(IDCT_ENGINE_BASE_ADDRESS + 0x100);


#define S(arg) #arg
#define D(arg) S(arg)
#define INT(reg, bit) (((reg) << 5) | (bit))
#define INT_REG(interrupt) (((interrupt) >> 5) * 4)
#define INT_SET(interrupt) 0x0114 + INT_REG(interrupt)
#define INT_CLR(interrupt) 0x0124 + INT_REG(interrupt)
#define INT_STAT(interrupt) 0x0104 + INT_REG(interrupt)
#define INT_MASK(interrupt) 0x00C0 + INT_REG(interrupt)
#define INT_BIT(interrupt) ((interrupt) & 0x1F)
#define INT_BIT_MASK(interrupt) (1 << INT_BIT(interrupt))

void ubicom32_clear_interrupt(unsigned char interrupt)
{
	uint32_t ibit = INT_BIT_MASK(interrupt);

	if (INT_REG(interrupt) == INT_REG(INT(0, 0))) {
		asm volatile (
			"move.4		"D(INT_CLR(INT(0, 0)))", %0\n\t"
			:
			: "r" (ibit)
		);

		return;
	}

	asm volatile (
		"move.4		"D(INT_CLR(INT(1, 0)))", %0\n\t"
		:
		: "r" (ibit)
	);
}

unsigned char ubicom32_is_interrupt_set(unsigned char interrupt)
{
	uint32_t ibit = INT_BIT_MASK(interrupt);

	if (INT_REG(interrupt) == INT_REG(INT(0, 0))) {
		uint32_t flags;
		asm volatile (
			"move.4		%0, "D(INT_STAT(INT(0, 0)))"\n\t"
			: "=r" (flags)
		);

		return (unsigned char)((flags & ibit) != 0);
	}

	uint32_t flags;
	asm volatile (
		"move.4		%0, "D(INT_STAT(INT(1, 0)))"\n\t"
		: "=r" (flags)
	);

	return (unsigned char)((flags & ibit) != 0);
}

void ubicom32_enable_interrupt(unsigned char interrupt)
{
	uint32_t ibit = INT_BIT_MASK(interrupt);

	if (INT_REG(interrupt) == INT_REG(INT(0, 0))) {
		asm volatile (
			"or.4		"D(INT_MASK(INT(0, 0)))", "D(INT_MASK(INT(0, 0)))", %0\n\t"
			:
			: "d" (ibit)
		);

		return;
	}

	asm volatile (
		"or.4		"D(INT_MASK(INT(1, 0)))", "D(INT_MASK(INT(1, 0)))", %0\n\t"
		:
		: "d" (ibit)
	);
}

#define OCP_BASE	0x01000000
#define OCP_GENERAL	0x000
#define GENERAL_CFG_BASE (OCP_BASE + OCP_GENERAL)
#define GEN_CLK_SERDES_SEL 0x18	/* IP7000 only */
/*
 * load_idct_firmware
 */
void load_idct_firmware(void)
{
	idct_ctrl_port_p idct_ctrl = (idct_ctrl_port_p) hw_idct_base;
	uint32_t *quant_ptr = (uint32_t *) (hw_idct_data_base + 32);
	int i;
	uint32_t status;
	uint32_t cmd;

	*(uint32_t *)(GENERAL_CFG_BASE + GEN_CLK_SERDES_SEL) |= (3 << 16);

//	printf("Loading IDCT firmware\n");
	for ( i = 0; i < 256; i++ ) {
		idct_ctrl->write_data	= idct_instructions[i];
		idct_ctrl->addr		= i;
		idct_ctrl->cmd		= 0x92; // {valid,inst,write}
	} // for instr addr

	// load default quantization values (of 1) for mpeg
	// 64 16-bit quant values, packed 2 per 32-bit word
	for ( i = 0; i < 32; i++ ) {
		quant_ptr[i]	= 0x00010001;
	} // for quant values

	cmd = idct_ctrl->cmd;			// read cmd valid bit
	status = idct_ctrl->status;

	ubicom32_enable_interrupt(INT(1, 10));

//	printf("cmd = %lx status = %lx\n", cmd, status);
}


#define IDCT_ENGINE_BASE_ADDRESS 0x01000b00
void ff_ip7k_idct(DCTELEM *block) //DCTELEM *block)
{

#if 0
    ff_simple_idct(block);
#else

	uint32_t *idct_base = (uint32_t *)(IDCT_ENGINE_BASE_ADDRESS);
	uint32_t *idct_data_base = (uint32_t *)(IDCT_ENGINE_BASE_ADDRESS + 0x100);
    uint32_t int_reg0, int_reg1;
	/*
	* Load 32 words from Y into idct block.
	*/
#if 1
	memcpy(idct_data_base, block, 4*32);
#else
	asm volatile (
		"move.4 a1, %1\n\t"
		".rep 32\n\t"
		"move.4	0(%0), (a1)4++\n\t"
		".endr\n\t"
		::"a" (idct_data_base), "a" (block): "a1");
#endif

	/*
	 * trigger the IDCT block and wait until it is done
	 */

/*	__asm__ volatile ("move.4  4(%0), %1 \n\t" :: "a" (idct_base), "r" (0x84));

    __asm__ volatile (
        "move.4  %0, int_mask0   \n\t"
        "move.4  %1, int_mask1   \n\t"
        "move.4  int_mask0, #0   \n\t"
        "move.4  int_mask1, %2   \n\t"
        :       "=r" (int_reg0), "=r" (int_reg1)
        :       "d"(1<<10)
    );
*/


	ubicom32_clear_interrupt(INT(1,10));
	asm volatile ("move.4  4(%0), %1 \n\t" :: "a" (idct_base), "r" (0x84));
	while (ubicom32_is_interrupt_set(INT(1,10)) == 0);
	ubicom32_clear_interrupt(INT(1,10));
    
    __asm__ volatile (
        "move.4  int_mask0, %0   \n\t"
        "move.4  int_mask1, %1   \n\t"
        :
        :       "r" (int_reg0), "r" (int_reg1)
    );

#if 1
	memcpy(block, idct_data_base, 4*32);
#else
	asm volatile (
		".rep 32\n\t"
		"move.4	(%1)4++, (%0)\n\t"
		".endr\n\t"
		::"a" (idct_data_base), "a" (block));
#endif
#endif
}

