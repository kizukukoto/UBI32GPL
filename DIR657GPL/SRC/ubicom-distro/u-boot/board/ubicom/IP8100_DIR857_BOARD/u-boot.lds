OUTPUT_ARCH(ubicom32)
ENTRY(_start)
MEMORY {
 ram : ORIGIN = (0xc0000000 + 0x4000000 - (2097152)), LENGTH = (2097152)
}
SECTIONS
{
 __ocm_begin = 0xbffc0000;
 __ocm_limit = __ocm_begin + 0x00040000;
 . = (0xc0000000 + 0x4000000 - (2097152));
 . = ALIGN(4);
 _stext = .;
 .text :
 {
   cpu/ip8k/start.o (.text)
   *(.text)
  *(.text .text.*)
 } > ram
 . = ALIGN(4);
 .rodata : { *(.rodata) } > ram
 . = ALIGN(4);
 .data : { *(.data) } > ram
 . = ALIGN(4);
 .got : { *(.got) } > ram
 __u_boot_cmd_start = .;
 .u_boot_cmd : { *(.u_boot_cmd) } > ram
 __u_boot_cmd_end = .;
 . = ALIGN(4);
 __bss_start = .;
 .bss (NOLOAD) : { *(.bss) } > ram
 _end = .;
}
