/*
 * loader_kernel_ip5000_thread.c
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

/*
 * FLASH Command Set
 */
#define FLASH_FC_INST_CMD 0x00		// for SPI command only transaction
#define FLASH_FC_INST_WR 0x01		// for SPI write transaction
#define FLASH_FC_INST_RD 0x02		// for SPI read transaction

/*
 * Common commands
 */
#define FLASH_SPICMD_NULL 0x00		// invalid
#define FLASH_SPICMD_WRSR 0x01		// write status: FC_WRITE - 0(addr) - 0(dummy) - 1(data)
#define FLASH_SPICMD_PROGRAM 0x02	// write data: FC_WRITE - 3(addr) - 0(dummy) - N(data N=1 for SST)
#define FLASH_SPICMD_RD 0x03		// read data: FC_READ - 3(addr) - 0(dummy) - N(data)
#define FLASH_SPICMD_WRDI 0x04		// write disable: FC_CMD - 0(addr) - 0(dummy) - 0(data)
#define FLASH_SPICMD_RDSR 0x05		// read status: FC_READ - 0(addr) - 0(dummy) - 1(data)
#define FLASH_SPICMD_WREN 0x06		// write enable: FC_CMD - 0(addr) - 0(dummy) - 0(data)
#define FLASH_SPICMD_FAST_RD 0x0b	// fast read(not for ATMEL): FC_READ - 3(addr) - 1(dummy) - N(data)
#define FLASH_SPICMD_RDID 0x9f		// FC_READ - 0(addr) - 0(dummy) - 3(data id-dev-cap)
#define FLASH_SPICMD_PAGE_ERASE 0xd8	// sector/page erase: FC_CMD - 3(addr) - 0(dummy) - 0(data)
#define FLASH_SPICMD_CHIP_ERASE 0xc7	// whole chip erase: FC_CMD - 0(addr) - 0(dummy) - 0(data)

/*
 * SST specific commands
 */
#define FLASH_SPICMD_SST_AAI 0xad	// write data: FC_WRITE - 3(addr) - 0(dummy) - N(data N=2) SST only

/*
 * ATMEL specific commands
 */
#define FLASH_SPICMD_RDID_ATMEL 0x15	// FC_READ - 0(addr) - 0(dummy) - 2(data 0x1f-dev)
#define FLASH_SPICMD_PAGE_ERASE_ATMEL 0x52
#define FLASH_SPICMD_CHIP_ERASE_ATMEL 0x62

/*
 * Common status register bit definitions
 */
#define FLASH_STATUS_WIP (1 << 0)
#define FLASH_STATUS_WEL (1 << 1)
#define FLASH_STATUS_BP_MASK (7 << 2)
#define FLASH_STATUS_SRWD (1 << 7)

/*
 * Flash type
 */
#define EXTFLASH_TYPE_UNKNOWN 0
#define EXTFLASH_TYPE_AMD 0x01		// Include Spansion type
#define EXTFLASH_TYPE_ATMEL 0x1f	// Untested
#define EXTFLASH_TYPE_INTEL 0x89
#define EXTFLASH_TYPE_MXIC 0xc2
#define EXTFLASH_TYPE_SST 0xbf		// Untested
#define EXTFLASH_TYPE_ST 0x20
#define EXTFLASH_TYPE_WINBOND 0xef

static u8_t mem_flash_type = EXTFLASH_TYPE_UNKNOWN;

#define ALIGN_DOWN(v, a) ((v) & ~((a) - 1))
#define ALIGN_UP(v, a) (((v) + ((a) - 1)) & ~((a) - 1))

#define	FLASH_COMMAND_KICK_OFF(io)					\
	asm volatile (							\
	"	bset	(%0), #0, #%%bit("D(IO_XFL_INT_DONE)")	\n\t"	\
	"	jmpt.t	.+4					\n\t"	\
		:							\
		: "a"(&io->int_clr)					\
		: "cc"							\
	);								\
	asm volatile (							\
	"	bset	(%0), #0, #%%bit("D(IO_XFL_INT_START)")	\n\t"	\
		:							\
		: "a"(&io->int_set)					\
		: "cc"							\
	);

#define	FLASH_COMMAND_WAIT_FOR_COMPLETION(io)				\
	asm volatile (							\
	"	btst	(%0), #%%bit("D(IO_XFL_INT_DONE)")	\n\t"	\
	"	jmpeq.f	.-4					\n\t"	\
		:							\
		: "a"(&io->int_status)					\
		: "cc"							\
	);

#define	FLASH_COMMAND_EXEC(io)						\
	FLASH_COMMAND_KICK_OFF(io)					\
	FLASH_COMMAND_WAIT_FOR_COMPLETION(io)

#define EXTFLASH_WRITE_FIFO_SIZE 32
#define EXTFLASH_WRITE_BLOCK_SIZE 256				// Use limit of FLASH chip for max speed.
#define TEN_MICRO_SECONDS (12000000 * 10 / 1000000)

#define EXTFLASH_MAX_ACCESS_TIME 20
#define EXTFLASH_PAGE_SIZE 64*1024

/*
 * mem_flash_wait_until_complete()
 */
static void mem_flash_wait_until_complete(void)
{
	struct ubicom32_io_port *io = (struct ubicom32_io_port *)RA;

	do {
		/*
		 * Put a delay here to deal with flash programming problem.
		 */
		u32_t mptval = UBICOM32_IO_TIMER->mptval + TEN_MICRO_SECONDS;
		while (UBICOM32_IO_TIMER->mptval < mptval);

		io->ctl1 &= ~IO_XFL_CTL1_MASK;
		io->ctl1 |= IO_XFL_CTL1_FC_INST(FLASH_FC_INST_RD) | IO_XFL_CTL1_FC_DATA(1);
		io->ctl2 = IO_XFL_CTL2_FC_CMD(FLASH_SPICMD_RDSR);
		FLASH_COMMAND_EXEC(io);
	} while (io->status1 & FLASH_STATUS_WIP);
}

/*
 * mem_flash_write_next()
 */
static u32_t mem_flash_write_next(u32_t addr, u8_t *buf, u32_t length)
{
	DEBUG_ASSERT(length > 0, "invalid flash write size");

	struct ubicom32_io_port *io = (struct ubicom32_io_port *)RA;
	u32_t data_start = addr;
	u32_t data_end = addr + length;
	u32_t count;

	if (mem_flash_type == EXTFLASH_TYPE_SST) {
		/*
		 * Top limit address.
		 */
		u32_t block_start = ALIGN_DOWN(data_start, 2);
		u32_t block_end = block_start + 2;
		data_end = (data_end <= block_end) ? data_end : block_end;
		count = data_end - data_start;

		/*
		 * Transfer data to a buffer.
		 */
		union {
			u8_t byte[2];
			u16_t hword;
		} write_buf;

		for (u32_t i = 0, j = (data_start - block_start); i < (data_end - data_start); i++, j++) {
			write_buf.byte[j] = buf[i];
		}

		/*
		 * Write to flash.
		 */
		asm volatile (
		"	bset	(%0), #0, #%%bit("D(IO_PORTX_INT_FIFO_TX_RESET)")	\n\t"
		"	pipe_flush 0							\n\t"
			:
			: "a"(&io->int_set)
			: "cc"
		);
		asm volatile (
		"	move.4  (%0), %1						\n\t"
			:
			: "a"(&io->tx_fifo), "r"(write_buf.hword << 16)
		);

		/* Lock FLASH for write access. */
		io->ctl0 |= IO_XFL_CTL0_MCB_LOCK;

		/* Command: WREN */
		io->ctl1 &= ~IO_XFL_CTL1_MASK;
		io->ctl1 |= IO_XFL_CTL1_FC_INST(FLASH_FC_INST_CMD);
		io->ctl2 = IO_XFL_CTL2_FC_CMD(FLASH_SPICMD_WREN);
		FLASH_COMMAND_EXEC(io);

		/* Command: BYTE PROGRAM */
		io->ctl1 &= ~IO_XFL_CTL1_MASK;
		io->ctl1 |= IO_XFL_CTL1_FC_INST(FLASH_FC_INST_WR) | IO_XFL_CTL1_FC_DATA(2) | IO_XFL_CTL1_FC_ADDR;
		io->ctl2 = IO_XFL_CTL2_FC_CMD(FLASH_SPICMD_SST_AAI) | IO_XFL_CTL2_FC_ADDR(block_start);
		FLASH_COMMAND_EXEC(io);

		/* Command: WRDI */
		io->ctl1 &= ~IO_XFL_CTL1_MASK;
		io->ctl1 |= IO_XFL_CTL1_FC_INST(FLASH_FC_INST_CMD);
		io->ctl2 = IO_XFL_CTL2_FC_CMD(FLASH_SPICMD_WRDI);
		FLASH_COMMAND_EXEC(io);
	} else {
		/*
		 * Top limit address.
		 */
		u32_t block_start = ALIGN_DOWN(data_start, 4);
		u32_t block_end = block_start + EXTFLASH_WRITE_BLOCK_SIZE;

		/*
		 * The write block must be limited by FLASH internal buffer.
		 */
		u32_t block_end_align = ALIGN_DOWN(block_end, 256);
		block_end = (block_end_align > block_start) ? block_end_align : block_end;
		data_end = (data_end <= block_end) ? data_end : block_end;
		block_end = ALIGN_UP(data_end, 4);
		count = data_end - data_start;

		/*
		 * Write to flash.
		 */
		void *datap = buf;
		asm volatile (
		"	bset		(%1), #0, #%%bit("D(IO_PORTX_INT_FIFO_TX_RESET)")	\n\t"
		"	pipe_flush	0							\n\t"
		"										\n\t"
		"	.rept		"D(EXTFLASH_WRITE_FIFO_SIZE / 4)"			\n\t"
		"	move.1		d15, (%0)1++						\n\t"
		"	shmrg.1		d15, (%0)1++, d15					\n\t"
		"	shmrg.1		d15, (%0)1++, d15					\n\t"
		"	shmrg.1		d15, (%0)1++, d15					\n\t"
		"	move.4		(%2), d15						\n\t"
		"	.endr									\n\t"
			: "+a" (datap)
			: "a" (&io->int_set), "a"(&io->tx_fifo)
			: "cc", "d15"
		);

		/* Lock FLASH for write access. */
		io->ctl0 |= IO_XFL_CTL0_MCB_LOCK;

		/* Command: WREN */
		io->ctl1 &= ~IO_XFL_CTL1_MASK;
		io->ctl1 |= IO_XFL_CTL1_FC_INST(FLASH_FC_INST_CMD);
		io->ctl2 = IO_XFL_CTL2_FC_CMD(FLASH_SPICMD_WREN);
		FLASH_COMMAND_EXEC(io);

		/* Command: BYTE PROGRAM */
		io->ctl1 &= ~IO_XFL_CTL1_MASK;
		io->ctl1 |= IO_XFL_CTL1_FC_INST(FLASH_FC_INST_WR) | IO_XFL_CTL1_FC_DATA(block_end - block_start) | IO_XFL_CTL1_FC_ADDR;
		io->ctl2 = IO_XFL_CTL2_FC_CMD(FLASH_SPICMD_PROGRAM) | IO_XFL_CTL2_FC_ADDR(block_start);
		FLASH_COMMAND_KICK_OFF(io);

		s32_t extra_words = (s32_t)(block_end - block_start - EXTFLASH_WRITE_FIFO_SIZE) / 4;
		if (extra_words > 0) {
			asm volatile (
			"1:	cmpi		(%1), #"D(EXTFLASH_WRITE_FIFO_SIZE/4)"	\n\t"
			"	jmpge.s.f	1b					\n\t"
			"								\n\t"
			"	move.1		d15, (%0)1++				\n\t"
			"	shmrg.1		d15, (%0)1++, d15			\n\t"
			"	shmrg.1		d15, (%0)1++, d15			\n\t"
			"	shmrg.1		d15, (%0)1++, d15			\n\t"
			"	move.4		(%2), d15				\n\t"
			"	add.4		%3, #-1, %3				\n\t"
			"	jmpgt.t		1b					\n\t"
				: "+a" (datap)
				: "a" (&io->fifo_level), "a" (&io->tx_fifo), "d" (extra_words)
				: "cc", "d15"
			);
		}

		FLASH_COMMAND_WAIT_FOR_COMPLETION(io);
	}

	/*
	 * Complete.
	 */
	mem_flash_wait_until_complete();

	/* Unlock FLASH for cache access. */
	io->ctl0 &= ~IO_XFL_CTL0_MCB_LOCK;

	return count;
}

/*
 * mem_flash_write()
 */
static void mem_flash_write(u32_t addr, const void *src, u32_t length)
{
	DEBUG_TRACE("flash_write: 0x%lx 0x%lx", addr, length);

	/*
	 * Write data
	 */
	u8_t *ptr = (u8_t *)src;
	while (length) {
		u32_t count = mem_flash_write_next(addr, ptr, length);
		addr += count;
		ptr += count;
		length -= count;
	}
}

/*
 * mem_flash_erase_page()
 */
static void mem_flash_erase_page(u32_t addr)
{
	struct ubicom32_io_port *io = (struct ubicom32_io_port *)RA;

	/* Lock FLASH for write access. */
	io->ctl0 |= IO_XFL_CTL0_MCB_LOCK;

	/* Command: WREN */
	io->ctl1 &= ~IO_XFL_CTL1_MASK;
	io->ctl1 |= IO_XFL_CTL1_FC_INST(FLASH_FC_INST_CMD);
	io->ctl2 = IO_XFL_CTL2_FC_CMD(FLASH_SPICMD_WREN);
	FLASH_COMMAND_EXEC(io);

	/* Command: ERASE */
	io->ctl1 &= ~IO_XFL_CTL1_MASK;
	io->ctl1 |= IO_XFL_CTL1_FC_INST(FLASH_FC_INST_CMD) | IO_XFL_CTL1_FC_ADDR;
	if (mem_flash_type == EXTFLASH_TYPE_ATMEL) {
		io->ctl2 = IO_XFL_CTL2_FC_CMD(FLASH_SPICMD_PAGE_ERASE_ATMEL) | IO_XFL_CTL2_FC_ADDR(addr);
	} else {
		io->ctl2 = IO_XFL_CTL2_FC_CMD(FLASH_SPICMD_PAGE_ERASE) | IO_XFL_CTL2_FC_ADDR(addr);
	}
	FLASH_COMMAND_EXEC(io);

	mem_flash_wait_until_complete();

	/* invalidate the Dcache */
	mem_cache_invalidate_all(DCCR_BASE);

	/* Unlock FLASH for cache access. */
	io->ctl0 &= ~IO_XFL_CTL0_MCB_LOCK;
}

/*
 * mem_flash_erase()
 */
static void mem_flash_erase(u32_t addr, u32_t length)
{
	DEBUG_INFO("flash_erase: 0x%lx 0x%lx", addr, length);

	/*
	 * Adjust start address to nearest reverse page boundary.
	 * Calculate the endaddress to be the first address of the page
	 * just beyond this erase section of pages.
	 */
	u32_t endaddr = ALIGN_UP(addr + length, EXTFLASH_PAGE_SIZE);
	addr = ALIGN_DOWN(addr, EXTFLASH_PAGE_SIZE);

	/*
	 * Erase.
	 */
	while (addr < endaddr) {
		mem_flash_erase_page(addr);

		/*
		 * Test how much was erased as actual flash page at this address
		 * may be smaller than the expected page size.
		 */
		u32_t test_addr = addr;
		while (test_addr < endaddr) {
			u32_t v;

			/*
			 * Rapidly scan 32 bytes at a time and check they're all
			 * 0xff.  This check is a little crude but flash sectors
			 * are never as small as 32 bytes anyway!
			 */
			asm volatile (
			"	move.4	%1, (%0)4++	\n\t"
			"	and.4	%1, (%0)4++, %1	\n\t"
			"	and.4	%1, (%0)4++, %1	\n\t"
			"	and.4	%1, (%0)4++, %1	\n\t"
			"	and.4	%1, (%0)4++, %1	\n\t"
			"	and.4	%1, (%0)4++, %1	\n\t"
			"	and.4	%1, (%0)4++, %1	\n\t"
			"	and.4	%1, (%0)4++, %1	\n\t"
				: "+a" (test_addr), "=&d" (v)
				:
				: "cc"
			);

			if (v != 0xFFFFFFFF) {
				DEBUG_ERROR("erase failed at address 0x%lx, skipping", test_addr - 32);
				break;
			}
		}

		addr = test_addr;
	}
}

/*
 * loader_kernel_flash_thread_erase()
 */
static void loader_kernel_flash_thread_erase(void)
{
	mem_flash_erase(flash_state->access_addr, flash_state->length);

	/* invalidate the Dcache */
	mem_cache_invalidate_all(DCCR_BASE);
}

/*
 * loader_kernel_flash_thread_write()
 */
static void loader_kernel_flash_thread_write(void)
{
	u32_t addr = flash_state->access_addr;
	u32_t *datap = &loader_kernel_flash_write_buffer[flash_state->buffer][0];
	u32_t length = flash_state->length;

	/*
	 * Write data
	 */
	mem_flash_write(addr, datap, length);

	/* invalidate the Dcache */
	mem_cache_invalidate_all(DCCR_BASE);
}

/*
 * mem_flash_query()
 */
static u32_t mem_flash_query(void)
{
	/* Read FLASH ID */

	struct ubicom32_io_port *io = (struct ubicom32_io_port *)RA;
	io->ctl1 &= ~IO_XFL_CTL1_MASK;
	io->ctl1 |= IO_XFL_CTL1_FC_INST(FLASH_FC_INST_RD) | IO_XFL_CTL1_FC_DATA(3);
	io->ctl2 = IO_XFL_CTL2_FC_CMD(FLASH_SPICMD_RDID);
	FLASH_COMMAND_EXEC(io);
	u8_t type = (u8_t)(io->status1 >> 16);

	if ((type != EXTFLASH_TYPE_AMD) &&
			(type != EXTFLASH_TYPE_INTEL) &&
			(type != EXTFLASH_TYPE_MXIC) &&
			(type != EXTFLASH_TYPE_SST) &&
			(type != EXTFLASH_TYPE_ST) &&
			(type != EXTFLASH_TYPE_WINBOND)) {
		/* Try for ATMEL FLASH now */
		io->ctl1 &= ~IO_XFL_CTL1_MASK;
		io->ctl1 |= IO_XFL_CTL1_FC_INST(FLASH_FC_INST_RD) | IO_XFL_CTL1_FC_DATA(2);
		io->ctl2 = IO_XFL_CTL2_FC_CMD(FLASH_SPICMD_RDID_ATMEL);
		FLASH_COMMAND_EXEC(io);
		type = (u8_t)(io->status1 >> 8);
	}

	/* Return known supported type of FLASH only */
	u32_t flash_timing;
	switch (type) {
	case EXTFLASH_TYPE_AMD:		// (fast read / 50MHz clock / 100ns idle)
	case EXTFLASH_TYPE_INTEL:	// (fast read / 50MHz clock / 100ns idle)
	case EXTFLASH_TYPE_MXIC:	// (fast read / 50MHz clock / 100ns idle)
	case EXTFLASH_TYPE_SST:		// (fast read / 50MHz clock / 50ns idle)
	case EXTFLASH_TYPE_ST:		// (fast read / 50MHz clock / 100ns idle)
	case EXTFLASH_TYPE_WINBOND:	// (fast read / 50MHz clock / 100ns idle)
		/*
		 * Configure the flash timing with FC_CLK_WIDTH = Calculated value + 2
		 * (Done to fix the flash write issue that we are unable to root cause yet)
		 * and FC_CE_WAIT = 12
		 */
		flash_timing = IO_XFL_CTL0_FAST_VALUE(
			(((SYSTEM_FREQ / 1000000) * EXTFLASH_MAX_ACCESS_TIME - 1) / 1000 + 2),
			12);
		flash_timing = (flash_timing ^ io->ctl0) & IO_XFL_CTL0_MASK;
		asm volatile (" xor.4 (%0), (%0), %1" : : "a" (&io->ctl0), "d" (flash_timing) : "cc");
		return type;

	case EXTFLASH_TYPE_ATMEL:	// (normal read / 33MHz clock / 25ns idle)
		/* Configure the flash timing (normal read / 33MHz clock / 25ns idle) */
		flash_timing = IO_XFL_CTL0_VALUE(
			(((SYSTEM_FREQ / 1000000) * EXTFLASH_MAX_ACCESS_TIME - 1) / 1000 + 2),
			((100 - 1) / EXTFLASH_MAX_ACCESS_TIME + 2));
		flash_timing = (flash_timing ^ io->ctl0) & IO_XFL_CTL0_MASK;
		asm volatile (" xor.4 (%0), (%0), %1" : : "a"(&io->ctl0), "d"(flash_timing) : "cc");
		return type;

	default:
		return EXTFLASH_TYPE_UNKNOWN;
	}
}

/*
 * mem_flash_init()
 */
static void mem_flash_init(void)
{
	/*
	 * Query flash device.
	 */
	mem_flash_type = mem_flash_query();

	/*
	 * If the flash is Intel the BP bits will come up high. We need to drive them to zero
	 * by writing a 0 to the status register.
	 */
	if (mem_flash_type == EXTFLASH_TYPE_INTEL) {
		/*
		 * Command: WREN. This will let us write the Status Register.
		 */
		struct ubicom32_io_port *io = (struct ubicom32_io_port *)RA;

		io->ctl1 &= ~IO_XFL_CTL1_MASK;
		io->ctl1 |= IO_XFL_CTL1_FC_INST(FLASH_FC_INST_CMD);
		io->ctl2 = IO_XFL_CTL2_FC_CMD(FLASH_SPICMD_WREN);
		FLASH_COMMAND_EXEC(io);

		/*
		 * Write 0 to SR.
		 */
		io->ctl1 &= ~IO_XFL_CTL1_MASK;
		io->ctl1 |= IO_XFL_CTL1_FC_INST(FLASH_FC_INST_WR) | IO_XFL_CTL1_FC_DATA(1);
		io->ctl2 = IO_XFL_CTL2_FC_CMD(FLASH_SPICMD_WRSR);
		io->tx_fifo = 0;
		FLASH_COMMAND_EXEC(io);
	}
}

/*
 * loader_kernel_flash_thread()
 */
void loader_kernel_flash_thread(void)
{
	mem_flash_init();

	ubicom32_clear_interrupt(THREAD_INT(FLASH_THREAD));
	ubicom32_enable_interrupt(THREAD_INT(FLASH_THREAD));

	while (1) {
		thread_suspend();
		ubicom32_clear_interrupt(THREAD_INT(FLASH_THREAD));

		switch (flash_state->command) {
		case ERASE_FLASH:
			loader_kernel_flash_thread_erase();
			break;

		case WRITE_PGM_MEMORY:
			loader_kernel_flash_thread_write();
			break;
		}

		flash_state->command = 0;
		ubicom32_set_interrupt(THREAD_INT(MAIN_THREAD));
	}
}

