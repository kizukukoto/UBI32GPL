/*
 * debug_kernel.ld
 *
 * Copyright © 2002-2006 Ubicom Inc. <www.ubicom.com>.  All rights reserved.
 *
 * This file contains confidential information of Ubicom, Inc. and your use of
 * this file is subject to the Ubicom Software License Agreement distributed with
 * this file. If you are uncertain whether you are an authorized user or to report
 * any unauthorized use, please contact Ubicom, Inc. at +1-650-210-1500.
 * Unauthorized reproduction or distribution of this file is subject to civil and
 * criminal penalties.
 *
 * $RCSfile: loader_kernel_ip5000.ld,v $
 * $Date: 2008/11/30 12:05:57 $
 * $Revision: 1.3 $
 */

MEMORY
{
	progmem (wx) : ORIGIN = 0xbffc0000, LENGTH = 192K
}

SECTIONS
{
	/* begin markers */
	__ocm_begin = 0xbffc0000;
	__data_begin = 0xbffc0000;
	__ocm_text_begin = 0xbffc0000;

	/*
	 * Allocated memory end markers
	 *	(initialized to start of appropiate memory address).
	 */
	__data_end = __data_begin;
	__ocm_text_end = __ocm_text_begin;
	
	/* begin marker */
	__image_begin = __ocm_text_begin;

	/* text mem */
	.text __ocm_text_end :
	{
		KEEP(*(.codeloader))
		* (.codeloader)
		* (.entry)
		* (.entry.*)
		* (.ocm_text)
		* (.ocm_text.*)
		* (.text)
		* (.text.*)
	} >progmem
	__ocm_text_end = ((__ocm_text_end + SIZEOF(.text) + 3) & ~(3));

	/* data mem */
	__data_run_begin = (( __ocm_text_end + 0x10000 -1) & ~( 0x8000 -1) );
	__data_load_begin = __ocm_text_end;
	.data __data_run_begin : AT (__data_load_begin)
	{
		* (.rodata);
		* (.rodata.*);
		* (.data);
		* (.data.*);
	} >progmem
	__data_run_end = ((__data_run_begin + SIZEOF(.data) + 3) & ~(3));
	__ocm_text_load_end = ((__data_load_begin + SIZEOF(.data) + 3) & ~(3));
	__data_end = __data_run_end;
	__ocm_text_end = __ocm_text_load_end;

	/* end marker */
	__image_end = __ocm_text_end;

	/* bss mem.  */
	__bss_begin = __data_end;
	.bss __bss_begin :
	{
		* (.bss)
		* (.bss.*)
	} >progmem
	__bss_end = ((__bss_begin + SIZEOF(.bss) + 3) & ~(3));
	__data_end = __bss_end;

	/* ocm_data.  */
	__ocm_data_begin = __ocm_text_end;
	.ocm_data __ocm_data_begin :
	{
		* (.ocm_data)
		* (.ocm_data.*)
	} >progmem
	__ocm_data_end = ((__ocm_data_begin + SIZEOF(.ocm_data) + 3) & ~(3));
	__ocm_text_end = __ocm_data_end;
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
}
