/*
 * loader_kernel_ip5000.c
 *
 * Copyright © 2005-2010 Ubicom Inc. <www.ubicom.com>.  All rights reserved.
 *
 * This file contains confidential information of Ubicom, Inc. and your use of
 * this file is subject to the Ubicom Software License Agreement distributed with
 * this file. If you are uncertain whether you are an authorized user or to report
 * any unauthorized use, please contact Ubicom, Inc. at +1-408-789-2200.
 * Unauthorized reproduction or distribution of this file is subject to civil and
 * criminal penalties.
 */
#include "config.h"
#include "ip5000.h"
#include "loader_kernel_ip5000.h"

struct loader_kernel_flash_state_struct loader_kernel_flash_state;

static u32_t loader_kernel_flash_thread_stack[512 / 4];
u32_t loader_kernel_flash_write_buffer[2][FLASH_BUFFER_SIZE / 4];

/*
 * mailbox_read_avail()
 */
static bool_t mailbox_read_avail(void)
{
	return !(UBICOM32_ISD_MAILBOX->status & ISD_MAILBOX_STATUS_IN_EMPTY);
}

/*
 * mailbox_read()
 */
static u32_t mailbox_read(void)
{
	while (UBICOM32_ISD_MAILBOX->status & ISD_MAILBOX_STATUS_IN_EMPTY);

	u32_t v;
	asm volatile (
	"	move.4		%0, %1		\n\t"
		: "=r" (v)
		: "m" (*(u32_t *)&UBICOM32_ISD_MAILBOX->in)
	);
	return v;
}

/*
 * mailbox_read_array()
 */
static void mailbox_read_array(u32_t *arr, u32_t count)
{
	if (!count) {
		return;
	}

	u32_t *datap = arr;
	do {
		while (UBICOM32_ISD_MAILBOX->status & ISD_MAILBOX_STATUS_IN_EMPTY);

		asm volatile (
		"	move.4		(%0)4++, %1		\n\t"
			: "+a" (datap)
			: "m" (*(u32_t *)&UBICOM32_ISD_MAILBOX->in)
		);
	} while (--count);
}

/*
 * mailbox_write()
 */
static void mailbox_write(u32_t v)
{
	while (!(UBICOM32_ISD_MAILBOX->status & ISD_MAILBOX_STATUS_OUT_EMPTY));

	/*
	 * Why we need 77 cycles here is unclear but the original code had 7 pipe
	 * flushes and that's 77 cycles.  At least this way is more efficient!
	 */
	asm volatile (
	"	cycles		77	\n\t"
	"	move.4		%1, %0	\n\t"
		:
		: "r" (v), "m" (*(u32_t *)&UBICOM32_ISD_MAILBOX->out)
	);
}

/*
 * mailbox_write()
 */
static void mailbox_write_array(u32_t *arr, u32_t count)
{
	if (!count) {
		return;
	}

	u32_t *datap = arr;
	do {
		while (!(UBICOM32_ISD_MAILBOX->status & ISD_MAILBOX_STATUS_OUT_EMPTY));

		/*
		 * Why we need 77 cycles here is unclear but the original code had 7 pipe
		 * flushes and that's 77 cycles.  At least this way is more efficient!
		 */
		asm volatile (
		"	cycles		77	\n\t"
		"	move.4		%1, (%0)4++	\n\t"
			: "+a" (datap)
			: "m" (*(u32_t *)&UBICOM32_ISD_MAILBOX->out)
		);
	} while (--count);
}

/*
 * load_kernel_wait_for_flash_thread()
 */
static void load_kernel_wait_for_flash_thread(void)
{
	ubicom32_disable_interrupt(ISD_MAILBOX_INT);
	ubicom32_enable_interrupt(THREAD_INT(MAIN_THREAD));

	while (1) {
		ubicom32_clear_interrupt(THREAD_INT(MAIN_THREAD));
		if (flash_state->command == 0) {
			break;
		}
		thread_suspend();
	}

	ubicom32_disable_interrupt(THREAD_INT(MAIN_THREAD));
	ubicom32_enable_interrupt(ISD_MAILBOX_INT);
}

/*
 * load_kernel_thread_start
 */
static void load_kernel_thread_start(int thread, vector_t exec, void *sp)
{
	u32_t csr = (thread << 15) | (1 << 14);
	asm volatile (
	"	setcsr		%0			\n\t"
	"	setcsr_flush	0			\n\t"
	"	move.4		INT_MASK0, #0		\n\t"
	"	move.4		INT_MASK1, #0		\n\t"
	"	move.4		PC, %1			\n\t"
	"	move.4		SP, %2			\n\t"
	"	move.4		A0, #0			\n\t"
	"	setcsr		#0			\n\t"
	"	setcsr_flush	0			\n\t"
	"	or.4		MT_HPRI, MT_HPRI, %3	\n\t"
	"	move.4		MT_ACTIVE_SET, %3	\n\t"
	"	or.4		MT_EN, MT_EN, %3	\n\t"
		:
		: "d" (csr), "d" (exec), "d" (sp), "d" (1 << thread)
		: "cc"
	);
}

/*
 * loader_kernel_read_status()
 */
static void loader_kernel_read_status(void)
{
	mailbox_write(READ_STATUS_DONE);
	mailbox_write(0x00000000);
}

/*
 * loader_kernel_erase_flash()
 */
static void loader_kernel_erase_flash(void)
{
	u32_t addr = mailbox_read();
	u32_t length = mailbox_read();

	/*
	 * Wait for flash thread to finish previous transaction.
	 */
	load_kernel_wait_for_flash_thread();

	/*
	 * Start erase process.
	 */
	flash_state->access_addr = addr;
	flash_state->length = length;
	flash_state->command = ERASE_FLASH;
	ubicom32_set_interrupt(THREAD_INT(FLASH_THREAD));

	/*
	 * The old version of GDB has a short timeout on read operations
	 * so we actually need to wait for it to complete so as not cause
	 * a timeout if the next operation is a read.
	 */
	load_kernel_wait_for_flash_thread();

	/*
	 * Complete.
	 */
	mailbox_write(ERASE_FLASH_DONE);
	mailbox_write(0x00000000);
}

/*
 * loader_kernel_read_pgm_memory()
 */
static void loader_kernel_read_pgm_memory(void)
{
	u32_t addr = mailbox_read();
	u32_t length_words = mailbox_read();

	/*
	 * Wait for flash thread to finish previous transaction.
	 */
	load_kernel_wait_for_flash_thread();

	/*
	 * Accept request.
	 */
	mailbox_write(READ_PGM_MEMORY_DONE);
	mailbox_write_array((u32_t *)addr, length_words);
}

/*
 * loader_kernel_write_pgm_memory()
 */
static void loader_kernel_write_pgm_memory(void)
{
	u32_t addr = mailbox_read();
	u32_t length_words = mailbox_read();

	if ((addr & 0xf0000000) == 0x40000000) {
		/*
		 * This data is going to OCM directly.
		 */
		u32_t *datap = (u32_t *)addr;

		/*
		 * Transfer the data
		 */
		mailbox_read_array(datap, length_words);

		/* This data was destined to go PRAM directly and has already been sent. Nothing else to do.*/
		mailbox_write(WRITE_PGM_MEMORY_DONE);
		return;
	}

	/*
	 * Read data into inactive buffer.
	 */
	u32_t buffer = flash_state->buffer ^ 0x01;
	u32_t *datap = &loader_kernel_flash_write_buffer[buffer][0];

	mailbox_read_array(datap, length_words);

	/*
	 * Wait for flash thread to finish previous transaction.
	 */
	load_kernel_wait_for_flash_thread();

	/*
	 * Start write process.
	 */
	flash_state->buffer = buffer;
	flash_state->access_addr = addr;
	flash_state->length = length_words * 4;
	flash_state->command = WRITE_PGM_MEMORY;
	ubicom32_set_interrupt(THREAD_INT(FLASH_THREAD));

	/*
	 * Complete
	 */
	mailbox_write(WRITE_PGM_MEMORY_DONE);
}

/*
 * loader_kernel_crc_pgm_memory()
 */
static void loader_kernel_crc_pgm_memory(void)
{
	u32_t addr = mailbox_read();
	u32_t length_words = mailbox_read();

	/*
	 * Wait for flash thread to finish previous transaction.
	 */
	load_kernel_wait_for_flash_thread();

	/*
	 * Accept request.
	 */
	mailbox_write(CRC_PGM_MEMORY_DONE);

	/*
	 * Calculate CRC.
	 */
	u32_t crc;
	u32_t datap = addr;
	asm volatile (
	"	move.4		acc0_lo, #-1		\n\t"
	"						\n\t"
	"1:	crcgen		(%1)1++, %3		\n\t"
	"	crcgen		(%1)1++, %3		\n\t"
	"	crcgen		(%1)1++, %3		\n\t"
	"	crcgen		(%1)1++, %3		\n\t"
	"	add.4		%2, #-1, %2		\n\t"
	"	jmpne.w.t	1b			\n\t"
	"						\n\t"
	"	move.4		%0, acc0_lo		\n\t"
		: "=d" (crc), "+a" (datap), "+d" (length_words)
		: "d" (0xEDB88320)
		: "acc0_lo", "acc0_hi", "cc"
	);

	crc ^= 0xFFFFFFFF;
	mailbox_write(crc);

	/*
	 * Invalidate the Dcache
	 */
	mem_cache_invalidate_all(DCCR_BASE);
}

/*
 * loader_kernel()
 */
void loader_kernel(void)
{
	/*
	 * Init state.
	 */
	flash_state->command = 0;
	flash_state->buffer = 0;
	flash_state->erase_addr = 0;

	/*
	 * Invalidate the Dcache
	 */
	mem_cache_invalidate_all(DCCR_BASE);

	/*
	 * Main threads.
	 */
	load_kernel_thread_start(FLASH_THREAD, loader_kernel_flash_thread, STACK_ADDR_HIGH(loader_kernel_flash_thread_stack));

	ubicom32_clear_interrupt(ISD_MAILBOX_INT);
	ubicom32_enable_interrupt(ISD_MAILBOX_INT);

	/*
	 * Infinite loop processing mailbox commands.
	 */
	while (1) {
		thread_suspend();
		ubicom32_clear_interrupt(ISD_MAILBOX_INT);
		if (!mailbox_read_avail()) {
			continue;
		}

		u32_t mailbox_command = mailbox_read();
		switch (mailbox_command) {
		case READ_STATUS:
			loader_kernel_read_status();
			break;

		case ERASE_FLASH:
			loader_kernel_erase_flash();
			break;

		case READ_PGM_MEMORY:
			loader_kernel_read_pgm_memory();
			break;

		case WRITE_PGM_MEMORY:
			loader_kernel_write_pgm_memory();
			break;

		case CRC_PGM_MEMORY:
			loader_kernel_crc_pgm_memory();
			break;

		default:
			mailbox_write(BAD_COMMAND);
		}
	}
}

