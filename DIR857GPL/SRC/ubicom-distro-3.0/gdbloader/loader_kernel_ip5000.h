/*
 * loader_kernel_ip5000.h
 *
 * Copyright © 2005-2010 Ubicom Inc. <www.ubicom.com>.  All rights reserved.
 *
 * This file contains confidential information of Ubicom, Inc. and your use of
 * this file is subject to the Ubicom Software License Agreement distributed with
 * this file. If you are uncertain whether you are an authorized user or to report
 * any unauthorized use, please contact Ubicom, Inc. at +1-408-789-2200.
 * Unauthorized reproduction or distribution of this file is subject to civil and
 * criminal penalties.
 *
 * $RCSfile: loader_kernel_ip5000.c,v $
 * $Date: 2009/02/12 22:52:28 $
 * $Revision: 1.5 $
 */

/*
 * ubicom32_isd_mailbox
 */
struct ubicom32_isd_mailbox {
	volatile u32_t in;
	volatile u32_t out;
	volatile u32_t status;
};

#define UBICOM32_ISD_MAILBOX ((struct ubicom32_isd_mailbox *)ISD_MAILBOX_BASE)

#define STACK_ADDR_HIGH(x) (void *)((u32_t)(x) + sizeof((x)) - sizeof(u32_t))

extern void loader_kernel_flash_thread(void);

/*
 * ubicom32_is_interrupt_set()
 */
static inline unsigned int ubicom32_is_interrupt_set(u8_t interrupt)
{
	u32_t ret;
	u32_t ibit = INT_BIT_MASK(interrupt);

	if (INT_REG(interrupt) == INT_REG(INT_CHIP(0, 0))) {
		asm volatile (
			"and.4	%0, "D(INT_STAT(INT_CHIP(0, 0)))", %1\n\t"
			: "=r" (ret)
			: "d" (ibit)
			: "cc"
		);

		return ret;
	}

	asm volatile (
		"and.4	%0, "D(INT_STAT(INT_CHIP(1, 0)))", %1\n\t"
		: "=r" (ret)
		: "d" (ibit)
		: "cc"
	);

	return ret;
}

/*
 * ubicom32_is_interrupt_enabled()
 */
static inline unsigned int ubicom32_is_interrupt_enabled(u8_t interrupt)
{
	u32_t ret;
	u32_t ibit = INT_BIT_MASK(interrupt);

	if (INT_REG(interrupt) == INT_REG(INT_CHIP(0, 0))) {
		asm volatile (
			"and.4	%0, "D(INT_MASK(INT_CHIP(0, 0)))", %1\n\t"
			: "=r" (ret)
			: "d" (ibit)
			: "cc"
		);

		return ret;
	}

	asm volatile (
		"and.4	%0, "D(INT_MASK(INT_CHIP(1, 0)))", %1\n\t"
		: "=r" (ret)
		: "d" (ibit)
		: "cc"
	);

	return ret;
}

/*
 * ubicom32_set_interrupt()
 */
static inline void ubicom32_set_interrupt(u8_t interrupt)
{
	u32_t ibit = INT_BIT_MASK(interrupt);

	if (INT_REG(interrupt) == INT_REG(INT_CHIP(0, 0))) {
		asm volatile (
			"move.4		"D(INT_SET(INT_CHIP(0, 0)))", %0\n\t"
			:
			: "r" (ibit)
		);

		return;
	}

	asm volatile (
		"move.4		"D(INT_SET(INT_CHIP(1, 0)))", %0\n\t"
		:
		: "r" (ibit)
	);
}

/*
 * ubicom32_clear_interrupt()
 */
static inline void ubicom32_clear_interrupt(u8_t interrupt)
{
	u32_t ibit = INT_BIT_MASK(interrupt);

	if (INT_REG(interrupt) == INT_REG(INT_CHIP(0, 0))) {
		asm volatile (
			"move.4		"D(INT_CLR(INT_CHIP(0, 0)))", %0\n\t"
			:
			: "r" (ibit)
		);

		return;
	}

	asm volatile (
		"move.4		"D(INT_CLR(INT_CHIP(1, 0)))", %0\n\t"
		:
		: "r" (ibit)
	);
}

/*
 * ubicom32_enable_interrupt()
 */
static inline void ubicom32_enable_interrupt(u8_t interrupt)
{
	u32_t ibit = INT_BIT_MASK(interrupt);

	if (INT_REG(interrupt) == INT_REG(INT_CHIP(0, 0))) {
		asm volatile (
			"or.4		"D(INT_MASK(INT_CHIP(0, 0)))", "D(INT_MASK(INT_CHIP(0, 0)))", %0\n\t"
			:
			: "d" (ibit)
			: "cc"
		);

		return;
	}

	asm volatile (
		"or.4		"D(INT_MASK(INT_CHIP(1, 0)))", "D(INT_MASK(INT_CHIP(1, 0)))", %0\n\t"
		:
		: "d" (ibit)
		: "cc"
	);
}

/*
 * ubicom32_disable_interrupt()
 */
static inline void ubicom32_disable_interrupt(u8_t interrupt)
{
	u32_t ibit = ~INT_BIT_MASK(interrupt);

	if (INT_REG(interrupt) == INT_REG(INT_CHIP(0, 0))) {
		asm volatile (
			"and.4		"D(INT_MASK(INT_CHIP(0, 0)))", "D(INT_MASK(INT_CHIP(0, 0)))", %0\n\t"
			:
			: "d" (ibit)
			: "cc"
		);

		return;
	}

	asm volatile (
		"and.4		"D(INT_MASK(INT_CHIP(1, 0)))", "D(INT_MASK(INT_CHIP(1, 0)))", %0\n\t"
		:
		: "d" (ibit)
		: "cc"
	);
}

/*
 * ubicom32_enable_global_interrupts()
 */
static inline void ubicom32_enable_global_interrupts(void)
{
	asm volatile (
		"bset		GLOBAL_CTRL, GLOBAL_CTRL, #%%bit("D(GLOBAL_CTRL_INT_EN)")"
		:
		:
		: "cc"
	);
}

/*
 * ubicom32_disable_global_interrupts()
 */
static inline void ubicom32_disable_global_interrupts(void)
{
	asm volatile (
		"bclr		GLOBAL_CTRL, GLOBAL_CTRL, #%%bit("D(GLOBAL_CTRL_INT_EN)")"
		:
		:
		: "cc"
	);
}

/*
 * Cache handling for UBICOM32
 */
static inline void mem_cache_invalidate_all(u32_t cc)
{
	asm volatile (
	"	bset	"D(CCR_CTRL)"(%0), "D(CCR_CTRL)"(%0), #"D(CCR_CTRL_RESET)"	\n\t"
	"	nop									\n\t"
	"	bclr	"D(CCR_CTRL)"(%0), "D(CCR_CTRL)"(%0), #"D(CCR_CTRL_RESET)"	\n\t"
	"	pipe_flush 0								\n\t"
		:
		: "a" (cc)
		: "cc"
	);
}

static inline void thread_suspend(void)
{
	asm volatile (
	"	suspend\n\t"
		:
		:
	);
}

/* Main processor kernel Debugger API defines */
#define BREAKPOINT_HIT		0x00000001

#define READ_STATUS 		0x00000002
#define READ_STATUS_DONE 	READ_STATUS

#define READ_DATA_MEMORY	0x00000003
#define READ_DATA_MEMORY_DONE 	READ_DATA_MEMORY

#define WRITE_DATA_MEMORY	0x00000004
#define WRITE_DATA_MEMORY_DONE 	WRITE_DATA_MEMORY

#define READ_REGISTERS 		0x00000005
#define READ_REGISTERS_DONE 	READ_REGISTERS

#define WRITE_REGISTERS 	0x00000006
#define WRITE_REGISTERS_DONE 	WRITE_REGISTERS

#define READ_PGM_MEMORY 	0x00000007
#define READ_PGM_MEMORY_DONE 	READ_PGM_MEMORY

#define WRITE_PGM_MEMORY 	0x00000008
#define WRITE_PGM_MEMORY_DONE 	WRITE_PGM_MEMORY

#define READ_FLASH_MEMORY 	0x00000009
#define READ_FLASH_MEMORY_DONE 	READ_FLASH_MEMORY

#define WRITE_FLASH_MEMORY 	0x0000000a
#define WRITE_FLASH_MEMORY_DONE WRITE_FLASH_MEMORY

#define ERASE_FLASH_MEMORY 	0x0000000b
#define ERASE_FLASH_MEMORY_DONE ERASE_FLASH_MEMORY

#define COPY_DATA_MEMORY 	0x0000000c
#define COPY_DATA_MEMORY_DONE 	COPY_DATA_MEMORY

#define STOP 			0x0000000d
#define STOP_DONE 		STOP

#define START 			0x0000000e
#define START_DONE 		START

#define SINGLE_STEP 		0x0000000f
#define SINGLE_STEP_DONE 	SINGLE_STEP

#define GET_COPRO_REG		0x00000010
#define GET_COPRO_REG_DONE	GET_COPRO_REG

#define SET_COPRO_REG		0x00000011
#define SET_COPRO_REG_DONE	SET_COPRO_REG

#define STOP_COPRO		0x00000012
#define STOP_COPRO_DONE		STOP_COPRO

#define START_COPRO		0x00000013
#define START_COPRO_DONE	START_COPRO

#define ATTACH_DEBUGGER		0x00000014
#define ATTACH_DEBUGGER_DONE	ATTACH_DEBUGGER

#define DETACH_DEBUGGER		0x00000015
#define DETACH_DEBUGGER_DONE	DETACH_DEBUGGER

#define GET_COPRO_TREGS		0x00000016
#define GET_COPRO_TREGS_DONE	GET_COPRO_TREGS

#define DEBUGGER_THREAD_NO	0x00000017
#define DEBUGGER_THREAD_NO_DONE DEBUGGER_THREAD_NO

#define DEBUGGER_DONT_DEBUG_MASK	0x00000018
#define DEBUGGER_DONT_DEBUG_MASK_DONE	DEBUGGER_DONT_DEBUG_MASK

#define GET_STATIC_BP_ADDRS	0x00000019
#define GET_STATIC_BP_ADDRS_DONE	GET_STATIC_BP_ADDRS

#define GET_THREAD_PCS		0x0000001a
#define GET_THREAD_PCS_DONE	GET_THREAD_PCS

#define ERASE_FLASH		0x0000001b
#define ERASE_FLASH_DONE	ERASE_FLASH

#define WHO_ARE_YOU		0x0000001c
#define WHO_ARE_YOU_DONE	WHO_ARE_YOU

#define PROTECT_SECTOR		0x0000001d
#define PROTECT_SECTOR_DONE	PROTECT_SECTOR

#define BAD_COMMAND		0x0000001e

#define GET_FLASH_DETAILS	0x0000001f
#define GET_FLASH_DETAILS_DONE GET_FLASH_DETAILS

#define DCAPT_ERROR		0x00000020
#define PARITY_ERROR		0x00000021

#define GET_VERSION 		0x00000022
#define GET_VERSION_DONE	GET_VERSION

#define UNPROTECT_SECTOR	0x00000023
#define UNPROTECT_SECTOR_DONE	UNPROTECT_SECTOR

#define CRC_PGM_MEMORY		0x00000024
#define CRC_PGM_MEMORY_DONE	CRC_PGM_MEMORY

#define JUMP_TO_PC		0x00000025
#define JUMP_TO_PC_DONE		JUMP_TO_PC

#define GET_SGL_STP_BUF		0x00000026
#define GET_SGL_STP_BUF_DONE	GET_SGL_STP_BUF

#define START_SENDING		0xffccdde2

#define DEBUG_ASSERT
#define DEBUG_INFO
#define DEBUG_TRACE
#define DEBUG_ERROR

#define MAIN_THREAD 0
#define FLASH_THREAD 1
#define THREAD_INT(thread) INT_CHIP(0, (thread))

#define FLASH_BUFFER_SIZE 4096

struct loader_kernel_flash_state_struct {
	volatile u32_t command;
	volatile u32_t buffer;
	volatile u32_t erase_addr;
	volatile u32_t access_addr;
	volatile u32_t length;
};

extern struct loader_kernel_flash_state_struct loader_kernel_flash_state;
#define flash_state (&loader_kernel_flash_state)

extern u32_t loader_kernel_flash_write_buffer[2][FLASH_BUFFER_SIZE / 4];

extern void loader_kernel_flash_thread(void);
