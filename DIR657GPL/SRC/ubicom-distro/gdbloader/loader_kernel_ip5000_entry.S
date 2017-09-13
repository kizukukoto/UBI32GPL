;
; loader_kernel_ip5000_entry.S
;
; Copyright © 2004-2009 Ubicom Inc. <www.ubicom.com>.  All rights reserved.
;
; This file contains confidential information of Ubicom, Inc. and your use of
; this file is subject to the Ubicom Software License Agreement distributed with
; this file. If you are uncertain whether you are an authorized user or to report
; any unauthorized use, please contact Ubicom, Inc. at +1-650-210-1500.
; Unauthorized reproduction or distribution of this file is subject to civil and
; criminal penalties.
;
; $RCSfile: loader_kernel_ip5000_entry.S,v $
; $Date: 2007/01/30 22:33:47 $
; $Revision: 1.2 $
;

#include "config.h"
#include "ip5000.h"

	.sect	.entry,"ax"
	.global	start_vector
	.func	start_vector,start_vector

start_vector:
; Configure PLLs
	moveai		A0, #%hi(GENERAL_CFG_BASE)
	moveai		A1, #%hi(TIMER_BASE)

	lsr.4		D1, CHIP_ID, #(16+2)
	jmpne		__pll_config_for_jupiter

__pll_config_for_mars_ans_ares:
	; Setup D0 with the timer delay value
	#if ((1000 * CORE_PLL_NR_VALUE) > (250 * (GEN_CLK_MPT_FREQ / 1000000)))
	movei		D0, #1000 * CORE_PLL_NR_VALUE
	#else
	movei		D0, #250 * (GEN_CLK_MPT_FREQ / 1000000)
	#endif

	; Enable global interrupt & enable Real Time Timer interrupt
	bset		global_ctrl, global_ctrl, #0
	bset		INT_MASK0, #0, #INT_BIT(REALTIME_TIMER_INT)
	move.4		INT_MASK1, #0

	;Ensure clock source set to non-PLL
	bclr		GEN_CLK_CORE_CFG(A0), GEN_CLK_CORE_CFG(A0), #%bit(GEN_CLK_PLL_SELECT)
	bclr		GEN_CLK_IO_CFG(A0), GEN_CLK_IO_CFG(A0), #%bit(GEN_CLK_PLL_SELECT)
	bclr		GEN_CLK_DDR_CFG(A0), GEN_CLK_DDR_CFG(A0), #%bit(GEN_CLK_PLL_SELECT)
	;Put PLL in reset state
	bset		GEN_CLK_CORE_CFG(A0), GEN_CLK_CORE_CFG(A0), #%bit(GEN_CLK_PLL_RESET)
	bset		GEN_CLK_IO_CFG(A0), GEN_CLK_IO_CFG(A0), #%bit(GEN_CLK_PLL_RESET)
	bset		GEN_CLK_DDR_CFG(A0), GEN_CLK_DDR_CFG(A0), #%bit(GEN_CLK_PLL_RESET)
	;Configure PLL - Set the forward divider for Core Clock to divide by 2
	movei		GEN_CLK_CORE_CFG+0(A0), #%hi(GEN_CORE_PLL_CFG_VALUE | GEN_CLK_PLL_RESET | (1 << GEN_CLK_PLL_SLOWDOWN))
	movei		GEN_CLK_CORE_CFG+2(A0), #%lo(GEN_CORE_PLL_CFG_VALUE | GEN_CLK_PLL_RESET | (1 << GEN_CLK_PLL_SLOWDOWN))
	call		A5, __pll_delay

	;Take PLL out of reset state
	bclr		GEN_CLK_CORE_CFG(A0), GEN_CLK_CORE_CFG(A0), #%bit(GEN_CLK_PLL_RESET)
	call		A5, __pll_delay

	;Switch to PLL
	bset		GEN_CLK_CORE_CFG(A0), GEN_CLK_CORE_CFG(A0), #%bit(GEN_CLK_PLL_SELECT)
	call		A5, __pll_delay

	;Set Core pll to normal speed
	bclr		GEN_CLK_CORE_CFG(A0), GEN_CLK_CORE_CFG(A0), #GEN_CLK_PLL_SLOWDOWN
	call		A5, __pll_delay
	jmpt.t		__memory_initialization

__pll_config_for_jupiter:
	; Setup D0 with the timer delay value
	#if ((1000 * CORE_PLL_NR_VALUE) > (250 * (GEN_CLK_MPT_FREQ_JUPITER / 1000000)))
	movei		D0, #1000 * CORE_PLL_NR_VALUE
	#else
	movei		D0, #250 * (GEN_CLK_MPT_FREQ_JUPITER / 1000000)
	#endif

	; Enable global interrupt & enable Real Time Timer interrupt
	bset		global_ctrl, global_ctrl, #0
	bset		INT_MASK0, #0, #INT_BIT(REALTIME_TIMER_INT)
	move.4		INT_MASK1, #0
	;move.4		INT_MASK2, #0

	;Ensure clock source set to non-PLL
	bclr		GEN_CLK_CORE_CFG(A0), GEN_CLK_CORE_CFG(A0), #%bit(GEN_CLK_PLL_SELECT)
	bclr		GEN_CLK_IO_CFG(A0), GEN_CLK_IO_CFG(A0), #%bit(GEN_CLK_PLL_SELECT)
	bclr		GEN_CLK_DDR_CFG(A0), GEN_CLK_DDR_CFG(A0), #%bit(GEN_CLK_PLL_SELECT)
	;Put PLL in reset state
	bset		GEN_CLK_CORE_CFG(A0), GEN_CLK_CORE_CFG(A0), #%bit(GEN_CLK_PLL_RESET)
	bset		GEN_CLK_IO_CFG(A0), GEN_CLK_IO_CFG(A0), #%bit(GEN_CLK_PLL_RESET)
	bset		GEN_CLK_DDR_CFG(A0), GEN_CLK_DDR_CFG(A0), #%bit(GEN_CLK_PLL_RESET)
	;Configure PLL - Set the forward divider for Core Clock to divide by 2
	movei		GEN_CLK_CORE_CFG+0(A0), #%hi(GEN_CORE_PLL_CFG_VALUE_JUPITER | GEN_CLK_PLL_RESET | (1 << GEN_CLK_PLL_SLOWDOWN_JUPITER))
	movei		GEN_CLK_CORE_CFG+2(A0), #%lo(GEN_CORE_PLL_CFG_VALUE_JUPITER | GEN_CLK_PLL_RESET | (1 << GEN_CLK_PLL_SLOWDOWN_JUPITER))
	call		A5, __pll_delay

	;Take PLL out of reset state
	bclr		GEN_CLK_CORE_CFG(A0), GEN_CLK_CORE_CFG(A0), #%bit(GEN_CLK_PLL_RESET)
	call		A5, __pll_delay

	;Switch to PLL
	bset		GEN_CLK_CORE_CFG(A0), GEN_CLK_CORE_CFG(A0), #%bit(GEN_CLK_PLL_SELECT)
	call		A5, __pll_delay

	;Set Core pll to normal speed
	bclr		GEN_CLK_CORE_CFG(A0), GEN_CLK_CORE_CFG(A0), #GEN_CLK_PLL_SLOWDOWN_JUPITER
	call		A5, __pll_delay

__memory_initialization:
	; Disable interrupts
	move.4		INT_MASK0, #0
	bclr		global_ctrl, global_ctrl, #0

; Setup all onchip memory bank0 and bank1 as code
	moveai		A0, #%hi(OCMC_BASE)
	move.4		OCMC_BANK_MASK(A0), #1
	pipe_flush	0

; Pre-initialize data mem
	moveai		A1, #%hi(__data_run_end)
	lea.4		D0, %lo(__data_run_end)(A1)
	moveai		A1, #%hi(__data_load_begin)
	lea.4		A1, %lo(__data_load_begin)(A1)
	moveai		A2, #%hi(__data_run_begin)
	lea.4		A2, %lo(__data_run_begin)(A2)
	jmpt.t		2f
1:	move.4		(A2)4++, (A1)4++
2:	sub.4		#0, A2, D0
	jmpne.t		1b

; Clear bss mem
	moveai		A1, #%hi(__bss_end)
	lea.4		D0, %lo(__bss_end)(A1)
	moveai		A2, #%hi(__bss_begin)
	lea.4		A2, %lo(__bss_begin)(A2)
	jmpt.t		2f
1:	move.4		(A2)4++, #0
2:	sub.4		#0, A2, D0
	jmpne.t		1b

; Call loader kernel
	moveai		SP, #%hi(OCMEND)
	move.4		A0, #0
	move.4		MT_DBG_ACTIVE_SET, #-1
	move.4		MT_HPRI, #1
	move.4		INT_CLR0, #-1
	move.4		INT_CLR1, #-1
	;move.4		INT_CLR2, #-1
	move.4		INT_MASK0, #0
	move.4		INT_MASK1, #0
	;move.4		INT_MASK2, #0
	bset		global_ctrl, global_ctrl, #0
	call		A5, loader_kernel

; Cleanup
	bclr		global_ctrl, global_ctrl, #0
	move.4		INT_MASK0, #0
	move.4		INT_MASK1, #0
	;move.4		INT_MASK2, #0
	move.4		INT_CLR0, #-1
	move.4		INT_CLR1, #-1
	;move.4		INT_CLR2, #-1
	move.4		MT_DBG_ACTIVE_CLR, #-2
	bkpt #-1

;
; __pll_delay()
;
__pll_delay:
	add.4		TIMER_RTCOM(A1), TIMER_MPTVAL(A1), D0
	move.4		INT_CLR0, #-1
	add.4		TIMER_RTCOM(A1), TIMER_MPTVAL(A1), D0
	suspend
	calli		A5, 0(A5)

	.endfunc

	.sect	.codeloader,"ax"
	.global	mem_read_in
	.func	mem_read_in,mem_read_in
	;; Fast data write sequence
	;; d0 contains number of words to be moved from mailbox
	;; a0 Points to mailbox
	;; a1 Points to memory to be written to
mem_read_in:
3:	btst 8(a0), #30		; Bit test IN EMPTY bit. If set then stay put
	jmpne.t 3b

	;; We have data in mbox fifo
	move.4 (a1)4++, (a0)

	;; Decrement the counter
	add.4	d0, #-1, d0
	jmpne.t	3b
4:	jmpt.t 4b
	.endfunc

	.global mem_read_out
	.func mem_read_out, mem_read_out
	;; d0 contais number of words to be moved to mailbox
	;; a0 Points to mailbox
	;; a1 Points to memory to be read
mem_read_out:
3:	btst 8(a0), #29		; Bit test OUT FULL bit. If set then stay put
	jmpne.t 3b

	;; We have space in mbox fifo
	move.4 4(a0), (a1)4++

	;; Decrement the counter
	add.4	d0, #-1, d0
	jmpne.t	3b
4:	jmpt.t 4b
	.endfunc
