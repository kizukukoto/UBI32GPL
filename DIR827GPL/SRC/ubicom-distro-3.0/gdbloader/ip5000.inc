/*
 * ip5000.inc
 *
 * Copyright © 2005-2010 Ubicom Inc. <www.ubicom.com>.  All rights reserved.
 *
 * This file contains confidential information of Ubicom, Inc. and your use of
 * this file is subject to the Ubicom Software License Agreement distributed with
 * this file. If you are uncertain whether you are an authorized user or to report
 * any unauthorized use, please contact Ubicom, Inc. at +1-650-210-1500.
 * Unauthorized reproduction or distribution of this file is subject to civil and
 * criminal penalties.
 */

#if defined(__ASSEMBLY__)
	.macro	nops	quant
	.if	\quant > 0
	.rept	\quant
		nop
	.endr
	.endif
	.endm
#else
/*
 * Same macro as above just in C inline asm
 */
asm (
"	.macro	nops	quant		\n"
"	.if	\\quant > 0		\n"
"	.rept	\\quant			\n"
"		nop			\n"
"	.endr				\n"
"	.endif				\n"
"	.endm				\n"
);
#endif

/*
 * IP5000 and IP7000 takes 8 cycles for jmpt.f.
 * IP8000 takes 9 cycles for jmpt.f
 */
#if defined(__ASSEMBLY__)
	.macro	cycles	quant
	.if	(\quant) == 1
		nop
	.else
	.if	(((\quant) + 3) / 8) > 0
	.rept	(((\quant) + 3) / 8)
		jmpt.f		.+4
	.endr
	.endif
	.if	((((\quant) + 3) % 8) / 4) > 0
		jmpt.t		.+4
	.endif
	.endif
	.endm
#else
/*
 * Same macro as above just in C inline asm
 */
asm (
"						\n"
"	.macro	cycles	quant			\n"
"	.if	(\\quant) == 1			\n"
"		nop				\n"
"	.else					\n"
"	.if	(((\\quant) + 3) / 8) > 0	\n"
"	.rept	(((\\quant) + 3) / 8)		\n"
"		jmpt.f		.+4		\n"
"	.endr					\n"
"	.endif					\n"
"	.if	((((\\quant) + 3) % 8) / 4) > 0	\n"
"		jmpt.t		.+4		\n"
"	.endif					\n"
"	.endif					\n"
"	.endm					\n"
);
#endif

/*
 * IP5000 and IP7000 requires 11 cycles
 * IP8000 requires 12 cycles
 */
#if defined(__ASSEMBLY__)
	.macro	pipe_flush	cyc
		cycles		12 - (\cyc)
	.endm
#else
/*
 * Same macro as above just in C inline asm
 */
asm (
"	.macro	pipe_flush	cyc		\n"
"		cycles		12 - (\\cyc)	\n"
"	.endm					\n"
);
#endif

/*
 * IP5000 and IP7000 requires 5 cycles
 * IP8000 requires 6 cycles
 */
#if defined(__ASSEMBLY__)
	.macro	setcsr_flush	cyc
		cycles		6 - (\cyc)
	.endm
#else
/*
 * Same macro as above just in C inline asm
 */
asm (
"	.macro	setcsr_flush	cyc		\n"
"		cycles		6 - (\\cyc)	\n"
"	.endm					\n"
);
#endif
