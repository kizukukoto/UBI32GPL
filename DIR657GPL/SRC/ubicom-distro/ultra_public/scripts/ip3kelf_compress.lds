/*
 * ip3kelf.ld
 *	Linker script for ip3k processor
 *
 *	This file must be run through the c pre-processor before use:
 *	gcc -E -C -P -Iconfig -x c ip3kelf.ld -o linker_script.ld
 *
 * Copyright © 2002-2009 Ubicom Inc. <www.ubicom.com>.  All rights reserved.
 *
 * This file contains confidential information of Ubicom, Inc. and your use of
 * this file is subject to the Ubicom Software License Agreement distributed with
 * this file. If you are uncertain whether you are an authorized user or to report
 * any unauthorized use, please contact Ubicom, Inc. at +1-650-210-1500.
 * Unauthorized reproduction or distribution of this file is subject to civil and
 * criminal penalties.
 */

#include <config.inc>
#include <ip3k/ip3000.h>
#include <ipHAL.inc>
#include <ocm_size.h>

#define ALIGN(var, n) (((var) + ((n) - 1)) & ~((n) - 1))

ENTRY (__start_vector)

MEMORY {
	mem_flash (wx) : ORIGIN = FLASHSTART, LENGTH = EXTFLASH_TOTAL_SIZE
	mem_codeloader (wx) : ORIGIN = CODELOADER_BEGIN, LENGTH = CODELOADER_SIZE
	mem_os_syscall (wx) : ORIGIN = OS_SYSCALL_BEGIN, LENGTH = OS_SYSCALL_SIZE
	mem_ocm (wx) : ORIGIN = OS_SYSCALL_END, LENGTH = OCMSIZE
	mem_sdram (w) : ORIGIN = SDRAMSTART, LENGTH = EXTMEM_TOTAL_SIZE
}

SECTIONS {
	/*
	 * Alignement rules:
	 *
	 * All sections will leave the end pointers aligned to 4 bytes (the
	 * theory being that code often assumes that pointers are aligned to
	 * 32-bits).
	 *
	 * If alignment of more than 4 bytes is required for a start pointer
	 * then it is up to that section to apply the required alignment.
	 */

	/*
	 * Physical memory region markers.
	 */
	__flash_begin = FLASHSTART;
	__flash_end = __flash_begin;
	__flash_limit = __flash_begin + EXTFLASH_TOTAL_SIZE;

	__ocm_begin = OCMSTART; /* This must be OCMSTART */
	__ocm_limit = __ocm_begin + OCMSIZE;

	__sdram_end = SDRAMSTART;
/*	__ipos_begin = __sdram_end; */
	__sdram_begin = SDRAMSTART;
	__sdram_limit = SDRAMSTART + EXTMEM_TOTAL_SIZE;
	__sdram_coredump_begin = SDRAMSTART;
	__sdram_coredump_limit = SDRAMSTART + EXTMEM_COREDUMP_SIZE;

	__os_begin = __sdram_limit;

	/*
	 * Filemedia (needed for coredumps)
         */
	__filemedia_begin_addr = __flash_begin;
	__filemedia_end_addr = __flash_begin + EXTFLASH_TOTAL_SIZE;
	__filemedia_page_size = EXTFLASH_PAGE_SIZE;

	__image_begin_addr = __flash_end;		
	.image __image_begin_addr : {
		KEEP (* (.image))
	} > mem_flash
	__image_end_addr = ALIGN(__image_begin_addr + SIZEOF(.image), 32);

	/*
	 * Application
	 */
	__main_load_begin = __flash_end;

	/*
	 * flash
	 */
	__flash_entry_load_begin = __sdram_end;
	__flash_entry_run_begin = __sdram_end;

	.flash_entry __flash_entry_run_begin : AT (__flash_entry_load_begin) {
		KEEP (* (.flash_entry))
		* (.flash_entry.*)
	        * (.pll_ddr_flash_entry.*)
	        * (.watchdog_flash_entry.*)
	} > mem_sdram

	__flash_entry_load_end = ALIGN(__flash_entry_load_begin + SIZEOF(.flash_entry), 4);
	__flash_entry_run_end = ALIGN(__flash_entry_run_begin + SIZEOF(.flash_entry), 4);
	__sdram_end = __flash_entry_load_end;

	/*
	 * IP5000 fast code loader has to occupy the lowest OCM address
	 */
	__codeloader_run_begin = CODELOADER_BEGIN;
	.codeloader __codeloader_run_begin : {
		* (.codeloader*)
		KEEP (* (.codeloader*))
	} > mem_codeloader
	__codeloader_run_end = ALIGN(__codeloader_run_begin + SIZEOF(.codeloader), 4);

	/*
	 * .os_syscall - on-chip syscall reservation
	 */
	__os_syscall_start = OS_SYSCALL_BEGIN;
	__os_syscall_end = OS_SYSCALL_END;

	/*
	 * .ocm_text - on-chip text segment.
	 */
	__ocm_text_load_begin = __sdram_end;
	__ocm_text_run_begin  = __os_syscall_end;

	.ocm_text __ocm_text_run_begin : AT (__ocm_text_load_begin) {
		* (.ocm_text)
		* (.ocm_text.*)

#define LINKER_OCM_TEXT_RULES 1
#include <UbicomSDK.ldi>
#undef LINKER_OCM_TEXT_RULES
	} > mem_ocm

	__ocm_text_load_end = ALIGN(__ocm_text_load_begin + SIZEOF(.ocm_text), 0x100);
	__ocm_text_run_end = ALIGN(__ocm_text_run_begin + SIZEOF(.ocm_text), 0x100);
	__sdram_end = __ocm_text_load_end;
	__ocm_end = __ocm_text_run_end;

	/*
	 * .text - off-chip text segment
	 */
	__text_load_begin = __sdram_end;
	__text_run_begin = __sdram_end;

	.text __text_run_begin : AT (__text_load_begin) {
		* (.text)
		* (.text.*)
	} > mem_sdram

	__text_load_end = ALIGN(__text_load_begin + SIZEOF(.text), 32);
	__text_run_end = ALIGN(__text_run_begin + SIZEOF(.text), 32);
	__sdram_end = __text_load_end;
	__sdram_end = __text_run_end;

	/*
	 * Start of OCM data.
	 */
	__data_begin = ALIGN(__ocm_end, OCM_SECTOR_SIZE);

	/*
	 * .ocm_stack - on-chip stacks.
	 */
#if APP_OCM_DATA_SIZE
	__ocm_stack_run_begin = OCMEND - APP_OCM_DATA_SIZE;
#else
	__ocm_stack_run_begin = __data_begin;
#endif

	.ocm_stack __ocm_stack_run_begin : AT (__ocm_stack_run_begin ) {
		/*
		 * Locate the gdb stack logically above the system thread stacks
		 * as a potential overflow area if we don't care about post crash
		 * debugging else logically at the bottom of the stacks.
		 */
#if defined(WATCHDOG_ENABLED)
		* (.ocm_stack.gdb_stack)
#endif
		* (.ocm_stack.system_thread_stacks)
		* (.ocm_stack)
		* (.ocm_stack.*)
#if !defined(WATCHDOG_ENABLED)
		* (.ocm_stack.gdb_stack)
#endif
	} > mem_ocm

	__ocm_stack_run_end = ALIGN(__ocm_stack_run_begin + SIZEOF(.ocm_stack), 4);
	__ocm_end = __ocm_stack_run_end;

	/*
	 * .ocm_data - on-chip initialized data segment.
	 */
	__ocm_data_load_begin = __sdram_end;
	__ocm_data_run_begin = __ocm_end;

	.ocm_data __ocm_data_run_begin : AT (__ocm_data_load_begin) {
		* (.ocm_data)
		* (.ocm_data.*)

#define LINKER_OCM_DATA_RULES 1
#include <UbicomSDK.ldi>
#undef LINKER_OCM_DATA_RULES
	} > mem_ocm

	__ocm_data_load_end = ALIGN(__ocm_data_load_begin + SIZEOF(.ocm_data), 4);
	__ocm_data_run_end = ALIGN(__ocm_data_run_begin + SIZEOF(.ocm_data), 4);
	__sdram_end = __ocm_data_load_end;
	__ocm_end = __ocm_data_run_end;

	/*
	 * .data - off-chip initialized data segment.
	 */
	__data_load_begin = __sdram_end;
	__data_run_begin = __sdram_end;

	.data __data_run_begin : AT (__data_load_begin) {
		* (.data)
		* (.data.*)
	} > mem_sdram

	__data_load_end = ALIGN(__data_load_begin + SIZEOF(.data), 4);
	__data_run_end  = ALIGN(__data_run_begin  + SIZEOF(.data), 4);
	__sdram_end = __data_load_end;
	__sdram_end = __data_run_end;

	/*
	 * .ocm_bss - on-chip uninitialized data segment.
	 */
	__ocm_bss_run_begin = __ocm_end;

	.ocm_bss __ocm_bss_run_begin : AT (__ocm_bss_run_begin ) {
		* (.ocm_bss)
		* (.ocm_bss.*)

#define LINKER_OCM_BSS_RULES 1
#include <UbicomSDK.ldi>
#undef LINKER_OCM_BSS_RULES
	} > mem_ocm

	__ocm_bss_run_end = ALIGN(__ocm_bss_run_begin + SIZEOF(.ocm_bss), 4);
	__ocm_end = __ocm_bss_run_end;

	/*
	 * flash
	 */
	__flash_load_begin = __sdram_end;
	__flash_run_begin = __sdram_end;

	.flash __flash_run_begin : AT (__flash_load_begin) {
		* (.flash)
		* (.flash.*)
	} > mem_sdram

	__flash_load_end = ALIGN(__flash_load_begin + SIZEOF(.flash), 4);
	__flash_run_end = ALIGN(__flash_run_begin + SIZEOF(.flash), 4);
	__sdram_end = __flash_load_end;
	__sdram_end = __flash_run_end;

	/*
	 * .ocm_rodata - on-chip initialized read-only data segment.
	 */
	__ocm_rodata_load_begin = __sdram_end;
	__ocm_rodata_run_begin = __ocm_end;

	.ocm_rodata __ocm_rodata_run_begin : AT (__ocm_rodata_load_begin) {
		* (.ocm_strings)
		* (.ocm_strings.*)
		* (.ocm_rodata)
		* (.ocm_rodata.*)
		* (.ocm_data.rodata)
		* (.ocm_data.rodata*)

#define LINKER_OCM_RODATA_RULES 1
#include <UbicomSDK.ldi>
#undef LINKER_OCM_RODATA_RULES
	} > mem_ocm
	__ocm_rodata_load_end = ALIGN(__ocm_rodata_load_begin + SIZEOF(.ocm_rodata), 0x100);
	__ocm_rodata_run_end = ALIGN(__ocm_rodata_run_begin + SIZEOF(.ocm_rodata), 0x100);
	__sdram_end = __ocm_rodata_load_end;
	__ocm_end = __ocm_rodata_run_end;

	/*
	 * .rodata
	 */
	__rodata_load_begin = __sdram_end;
	__rodata_run_begin  = __sdram_end;

	.rodata __rodata_run_begin : AT (__rodata_load_begin) {
		* (.strings)
		* (.strings.*)
		* (.rodata)
		* (.rodata.*)
		* (.data.rodata)
		* (.data.rodata*)
	} > mem_sdram

	__rodata_load_end = ALIGN(__rodata_load_begin + SIZEOF(.rodata), 4);
	__rodata_run_end = ALIGN(__rodata_run_begin + SIZEOF(.rodata), 4);
	__sdram_end = __rodata_load_end;
	__sdram_end = __rodata_run_end;

	/*
	 * .ocm_heap - on-chip heap.
	 */
	__ocm_heap_run_begin = ALIGN(__ocm_end, 4);
	__ocm_heap_run_end = __ocm_heap_run_begin + (HEAP_OCM_HEAP_KB * 1024);
	__ocm_end = __ocm_heap_run_end;

	__sdram_end = ALIGN(__sdram_end, 4);

	__filesystem_section_begin = SDRAMSTART;
	__filesystem_section_end = ALIGN(__sdram_end, 4);

	/*
	 * .bss - off-chip uninitialized data segment.
	 */
	__bss_run_begin = __sdram_end;

	.bss __bss_run_begin : AT (__bss_run_begin ) {
		* (.bss)
		* (.bss.*)
	} > mem_sdram

	__bss_run_end = ALIGN(__bss_run_begin + SIZEOF(.bss), 4);
	__sdram_end = __bss_run_end;

	/*
	 * .heap - off-chip heap.
	 */
	__heap_run_begin = ALIGN(__sdram_end, CACHE_LINE_SIZE);
	__heap_run_end = __sdram_limit;
	__sdram_end = __heap_run_end;

	/*
	 * Used for the build system.
	 */
__main_load_end = __flash_end;

	/*
	 * Flash protection
	 */
	__extflash_base_addr = __flash_begin;
	__extflash_read_range_min = __flash_begin;
	__extflash_read_range_max = __flash_limit;
	__extflash_write_range_min = __flash_begin;
	__extflash_write_range_max = __flash_limit;
	__extflash_erase_range_min = __flash_begin;
	__extflash_erase_range_max = __flash_limit;

	/*
	 * Debug
	 */

	.comment 0 : {
		* (.comment)
	}

	/*
	 * DWARF debug sections.
	 * Symbols in the DWARF debugging sections are relative to the beginning
	 * of the section so we begin them at 0.
	 */

	/*
	 * DWARF 1
	 */
	.debug 0 : {
		* (.debug)
	}

	.line 0 : {
		* (.line)
	}

	/*
	 * GNU DWARF 1 extensions
	 */
	.debug_srcinfo 0 : {
		* (.debug_srcinfo)
	}

	.debug_sfnames 0 : {
		* (.debug_sfnames)
	}

	/*
	 * DWARF 1.1 and DWARF 2
	 */
	.debug_aranges 0 : {
		* (.debug_aranges)
	}

	.debug_pubnames 0 : {
		* (.debug_pubnames)
	}

	/*
	 *DWARF 2
	 */
	.debug_info 0 : {
		* (.debug_info)
	}

	.debug_abbrev 0 : {
		* (.debug_abbrev)
	}

	.debug_line 0 : {
		* (.debug_line)
	}

	.debug_frame 0 : {
		* (.debug_frame)
	}

	.debug_str 0 : {
		* (.debug_str)
	}

	.debug_loc 0 : {
		* (.debug_loc)
	}

	.debug_macinfo 0 : {
		* (.debug_macinfo)
	}

	/*
	 * SGI/MIPS DWARF 2 extensions
	 */
	.debug_weaknames 0 : {
		* (.debug_weaknames)
	}

	.debug_funcnames 0 : {
		* (.debug_funcnames)
	}

	.debug_typenames 0 : {
		* (.debug_typenames)
	}

	.debug_varnames  0 : {
		* (.debug_varnames)
	}

	/*
	 * Sections to put reflection type information from the TYPEOF() macro, for later extraction.
	 * NOTE: The .typeof_helper1 section must be located below 0x80000000, since addresses of
	 * variables in the section are taken by the C code, and the ip5k can not load addresses above
	 * 2Gb with the 'moveai' instruction.
	 */
	.typeof_helper1 0x70000000 : {
		KEEP (* (.typeof_helper1))
		* (.typeof_helper1)
	}

	.typeof_helper2 0xe0000000 : {
		KEEP (* (.typeof_helper2))
		* (.typeof_helper2)
	}

	/*
	 * A section to put information about translatable strings for later extraction.
	 */
	.i18n_strings 0xe2000000 : {
		KEEP (* (.i18n_strings))
		* (.i18n_strings)
	}

	.downloader 0xd0000000 : AT (0xd0000000) {
		KEEP (* (.downloader*))
		* (.downloader*)
	}
}

