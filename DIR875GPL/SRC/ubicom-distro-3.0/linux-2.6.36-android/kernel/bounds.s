	.file	"bounds.c"
; GNU C (GCC) version 4.4.1 20110528 (platform) (ubicom32-elf)
;	compiled by GNU C version 4.1.2 20071124 (Red Hat 4.1.2-42), GMP version 4.3.1, MPFR version 2.3.1.
; GGC heuristics: --param ggc-min-expand=100 --param ggc-min-heapsize=131072
; options passed:  -nostdinc
; -I/home/twu/work/ubicom-distro-3.0/linux-2.6.36-android/arch/ubicom32/include
; -Iinclude -imultilib march-ubicom32v5 -iprefix
; /home/twu/work/ubicom-distro-3.0/toolchain/bin/../lib/gcc/ubicom32-elf/4.4.1/
; -D__KERNEL__ -DIP8000 -DUBICOM32_ARCH_VERSION=5 -D__linux__ -Dlinux
; -DUTS_SYSNAME="uClinux" -DKBUILD_STR(s)=#s
; -DKBUILD_BASENAME=KBUILD_STR(bounds) -DKBUILD_MODNAME=KBUILD_STR(bounds)
; -isystem
; /home/twu/work/ubicom-distro-3.0/toolchain/bin/../lib/gcc/ubicom32-elf/4.4.1/include
; -include include/generated/autoconf.h -MD kernel/.bounds.s.d
; kernel/bounds.c -march=ubicom32v5 -mfastcall -auxbase-strip
; kernel/bounds.s -g -Os -Wall -Wundef -Wstrict-prototypes -Wno-trigraphs
; -Werror-implicit-function-declaration -Wno-format-security
; -Wframe-larger-than=1024 -Wdeclaration-after-statement -Wno-pointer-sign
; -fno-strict-aliasing -fno-common -fno-delete-null-pointer-checks
; -ffunction-sections -fno-stack-protector -fomit-frame-pointer
; -fno-strict-overflow -fconserve-stack -fverbose-asm
; options enabled:  -falign-loops -fargument-alias -fauto-inc-dec
; -fbranch-count-reg -fcaller-saves -fcprop-registers -fcrossjumping
; -fcse-follow-jumps -fdefer-pop -fearly-inlining
; -feliminate-unused-debug-types -fexpensive-optimizations
; -fforward-propagate -ffunction-cse -ffunction-sections -fgcse -fgcse-lm
; -fguess-branch-probability -fident -fif-conversion -fif-conversion2
; -findirect-inlining -finline -finline-functions
; -finline-functions-called-once -finline-small-functions -fipa-cp
; -fipa-pure-const -fipa-reference -fira-share-save-slots
; -fira-share-spill-slots -fkeep-static-consts -fleading-underscore
; -fmath-errno -fmerge-constants -fmerge-debug-strings
; -fmove-loop-invariants -fomit-frame-pointer -foptimize-register-move
; -foptimize-sibling-calls -fpeephole -fpeephole2 -freg-struct-return
; -fregmove -freorder-blocks -freorder-functions -frerun-cse-after-loop
; -fsched-interblock -fsched-spec -fsched-stalled-insns-dep
; -fschedule-insns -fschedule-insns2 -fsigned-zeros -fsplit-ivs-in-unroller
; -fsplit-wide-types -fthread-jumps -ftoplevel-reorder -ftrapping-math
; -ftree-builtin-call-dce -ftree-ccp -ftree-ch -ftree-copy-prop
; -ftree-copyrename -ftree-dce -ftree-dominator-opts -ftree-dse -ftree-fre
; -ftree-loop-im -ftree-loop-ivcanon -ftree-loop-optimize
; -ftree-parallelize-loops= -ftree-pre -ftree-reassoc -ftree-scev-cprop
; -ftree-sink -ftree-sra -ftree-switch-conversion -ftree-ter
; -ftree-vect-loop-version -ftree-vrp -funit-at-a-time -fvar-tracking
; -fverbose-asm -fzero-initialized-in-bss -mfastcall -mhard-float

	.section	.debug_abbrev,"",@progbits
.Ldebug_abbrev0:
	.section	.debug_info,"",@progbits
.Ldebug_info0:
	.section	.debug_line,"",@progbits
.Ldebug_line0:
	.section	.text
.Ltext0:
; Compiler executable checksum: abc902d71d7a2d27e32c851be20a9b14

	.section	.text.foo,"ax",@progbits
	.align 2
	.global	foo
	.type	foo, @function
foo:
.LFB0:
.LSM0:
/* frame/pretend: 0/0 save_regs: 0 out_args: 0  leaf */
.LSM1:
#APP
; 16 "kernel/bounds.c" 1
	
->NR_PAGEFLAGS #23 __NR_PAGEFLAGS	;
; 0 "" 2
.LSM2:
; 17 "kernel/bounds.c" 1
	
->MAX_NR_ZONES #2 __MAX_NR_ZONES	;
; 0 "" 2
.LSM3:
#NO_APP
	calli	a5, 0(a5)	;
.LFE0:
	.size	foo, .-foo
	.section	.debug_frame,"",@progbits
.Lframe0:
	.4byte	.LECIE0-.LSCIE0
.LSCIE0:
	.4byte	0xffffffff
	.byte	0x1
	.string	""
	.uleb128 0x1
	.sleb128 -4
	.byte	0x15
	.byte	0xc
	.uleb128 0x17
	.uleb128 0x0
	.align 2
.LECIE0:
.LSFDE0:
	.4byte	.LEFDE0-.LASFDE0
.LASFDE0:
	.4byte	.Lframe0
	.4byte	.LFB0
	.4byte	.LFE0-.LFB0
	.align 2
.LEFDE0:
	.section	.text
.Letext0:
	.section	.debug_info
	.4byte	0x339
	.2byte	0x2
	.4byte	.Ldebug_abbrev0
	.byte	0x4
	.uleb128 0x1
	.string	"GNU C 4.4.1 20110528 (platform)"
	.byte	0x1
	.string	"kernel/bounds.c"
	.string	"/home/twu/work/ubicom-distro-3.0/linux-2.6.36-android"
	.4byte	0x0
	.4byte	0x0
	.4byte	.Ldebug_ranges0+0x0
	.4byte	.Ldebug_line0
	.uleb128 0x2
	.byte	0x1
	.byte	0x6
	.string	"signed char"
	.uleb128 0x2
	.byte	0x1
	.byte	0x8
	.string	"unsigned char"
	.uleb128 0x2
	.byte	0x2
	.byte	0x5
	.string	"short int"
	.uleb128 0x2
	.byte	0x2
	.byte	0x7
	.string	"short unsigned int"
	.uleb128 0x2
	.byte	0x4
	.byte	0x5
	.string	"int"
	.uleb128 0x2
	.byte	0x4
	.byte	0x7
	.string	"unsigned int"
	.uleb128 0x2
	.byte	0x8
	.byte	0x5
	.string	"long long int"
	.uleb128 0x2
	.byte	0x8
	.byte	0x7
	.string	"long long unsigned int"
	.uleb128 0x2
	.byte	0x4
	.byte	0x7
	.string	"long unsigned int"
	.uleb128 0x3
	.byte	0x4
	.byte	0x7
	.uleb128 0x2
	.byte	0x1
	.byte	0x8
	.string	"char"
	.uleb128 0x2
	.byte	0x4
	.byte	0x5
	.string	"long int"
	.uleb128 0x2
	.byte	0x1
	.byte	0x2
	.string	"_Bool"
	.uleb128 0x4
	.string	"pageflags"
	.byte	0x4
	.byte	0x2
	.byte	0x4b
	.4byte	0x2e8
	.uleb128 0x5
	.string	"PG_locked"
	.sleb128 0
	.uleb128 0x5
	.string	"PG_error"
	.sleb128 1
	.uleb128 0x5
	.string	"PG_referenced"
	.sleb128 2
	.uleb128 0x5
	.string	"PG_uptodate"
	.sleb128 3
	.uleb128 0x5
	.string	"PG_dirty"
	.sleb128 4
	.uleb128 0x5
	.string	"PG_lru"
	.sleb128 5
	.uleb128 0x5
	.string	"PG_active"
	.sleb128 6
	.uleb128 0x5
	.string	"PG_slab"
	.sleb128 7
	.uleb128 0x5
	.string	"PG_owner_priv_1"
	.sleb128 8
	.uleb128 0x5
	.string	"PG_arch_1"
	.sleb128 9
	.uleb128 0x5
	.string	"PG_reserved"
	.sleb128 10
	.uleb128 0x5
	.string	"PG_private"
	.sleb128 11
	.uleb128 0x5
	.string	"PG_private_2"
	.sleb128 12
	.uleb128 0x5
	.string	"PG_writeback"
	.sleb128 13
	.uleb128 0x5
	.string	"PG_head"
	.sleb128 14
	.uleb128 0x5
	.string	"PG_tail"
	.sleb128 15
	.uleb128 0x5
	.string	"PG_swapcache"
	.sleb128 16
	.uleb128 0x5
	.string	"PG_mappedtodisk"
	.sleb128 17
	.uleb128 0x5
	.string	"PG_reclaim"
	.sleb128 18
	.uleb128 0x5
	.string	"PG_buddy"
	.sleb128 19
	.uleb128 0x5
	.string	"PG_swapbacked"
	.sleb128 20
	.uleb128 0x5
	.string	"PG_unevictable"
	.sleb128 21
	.uleb128 0x5
	.string	"PG_mlocked"
	.sleb128 22
	.uleb128 0x5
	.string	"__NR_PAGEFLAGS"
	.sleb128 23
	.uleb128 0x5
	.string	"PG_checked"
	.sleb128 8
	.uleb128 0x5
	.string	"PG_fscache"
	.sleb128 12
	.uleb128 0x5
	.string	"PG_pinned"
	.sleb128 8
	.uleb128 0x5
	.string	"PG_savepinned"
	.sleb128 4
	.uleb128 0x5
	.string	"PG_slob_free"
	.sleb128 11
	.uleb128 0x5
	.string	"PG_slub_frozen"
	.sleb128 6
	.byte	0x0
	.uleb128 0x4
	.string	"zone_type"
	.byte	0x4
	.byte	0x3
	.byte	0xc5
	.4byte	0x329
	.uleb128 0x5
	.string	"ZONE_NORMAL"
	.sleb128 0
	.uleb128 0x5
	.string	"ZONE_MOVABLE"
	.sleb128 1
	.uleb128 0x5
	.string	"__MAX_NR_ZONES"
	.sleb128 2
	.byte	0x0
	.uleb128 0x6
	.byte	0x1
	.string	"foo"
	.byte	0x1
	.byte	0xd
	.byte	0x1
	.4byte	.LFB0
	.4byte	.LFE0
	.byte	0x1
	.byte	0x67
	.byte	0x0
	.section	.debug_abbrev
	.uleb128 0x1
	.uleb128 0x11
	.byte	0x1
	.uleb128 0x25
	.uleb128 0x8
	.uleb128 0x13
	.uleb128 0xb
	.uleb128 0x3
	.uleb128 0x8
	.uleb128 0x1b
	.uleb128 0x8
	.uleb128 0x11
	.uleb128 0x1
	.uleb128 0x52
	.uleb128 0x1
	.uleb128 0x55
	.uleb128 0x6
	.uleb128 0x10
	.uleb128 0x6
	.byte	0x0
	.byte	0x0
	.uleb128 0x2
	.uleb128 0x24
	.byte	0x0
	.uleb128 0xb
	.uleb128 0xb
	.uleb128 0x3e
	.uleb128 0xb
	.uleb128 0x3
	.uleb128 0x8
	.byte	0x0
	.byte	0x0
	.uleb128 0x3
	.uleb128 0x24
	.byte	0x0
	.uleb128 0xb
	.uleb128 0xb
	.uleb128 0x3e
	.uleb128 0xb
	.byte	0x0
	.byte	0x0
	.uleb128 0x4
	.uleb128 0x4
	.byte	0x1
	.uleb128 0x3
	.uleb128 0x8
	.uleb128 0xb
	.uleb128 0xb
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x1
	.uleb128 0x13
	.byte	0x0
	.byte	0x0
	.uleb128 0x5
	.uleb128 0x28
	.byte	0x0
	.uleb128 0x3
	.uleb128 0x8
	.uleb128 0x1c
	.uleb128 0xd
	.byte	0x0
	.byte	0x0
	.uleb128 0x6
	.uleb128 0x2e
	.byte	0x0
	.uleb128 0x3f
	.uleb128 0xc
	.uleb128 0x3
	.uleb128 0x8
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x27
	.uleb128 0xc
	.uleb128 0x11
	.uleb128 0x1
	.uleb128 0x12
	.uleb128 0x1
	.uleb128 0x40
	.uleb128 0xa
	.byte	0x0
	.byte	0x0
	.byte	0x0
	.section	.debug_pubnames,"",@progbits
	.4byte	0x16
	.2byte	0x2
	.4byte	.Ldebug_info0
	.4byte	0x33d
	.4byte	0x329
	.string	"foo"
	.4byte	0x0
	.section	.debug_aranges,"",@progbits
	.4byte	0x1c
	.2byte	0x2
	.4byte	.Ldebug_info0
	.byte	0x4
	.byte	0x0
	.2byte	0x0
	.2byte	0x0
	.4byte	.LFB0
	.4byte	.LFE0-.LFB0
	.4byte	0x0
	.4byte	0x0
	.section	.debug_ranges,"",@progbits
.Ldebug_ranges0:
	.4byte	.Ltext0
	.4byte	.Letext0
	.4byte	.LFB0
	.4byte	.LFE0
	.4byte	0x0
	.4byte	0x0
	.section	.debug_line
	.4byte	.LELT0-.LSLT0
.LSLT0:
	.2byte	0x2
	.4byte	.LELTP0-.LASLTP0
.LASLTP0:
	.byte	0x1
	.byte	0x1
	.byte	0xf6
	.byte	0xf5
	.byte	0xa
	.byte	0x0
	.byte	0x1
	.byte	0x1
	.byte	0x1
	.byte	0x1
	.byte	0x0
	.byte	0x0
	.byte	0x0
	.byte	0x1
	.ascii	"kernel"
	.byte	0
	.ascii	"include/linux"
	.byte	0
	.byte	0x0
	.string	"bounds.c"
	.uleb128 0x1
	.uleb128 0x0
	.uleb128 0x0
	.string	"page-flags.h"
	.uleb128 0x2
	.uleb128 0x0
	.uleb128 0x0
	.string	"mmzone.h"
	.uleb128 0x2
	.uleb128 0x0
	.uleb128 0x0
	.byte	0x0
.LELTP0:
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.4byte	.Letext0
	.byte	0x0
	.uleb128 0x1
	.byte	0x1
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.4byte	.LSM0
	.byte	0x21
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.4byte	.LSM1
	.byte	0x16
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.4byte	.LSM2
	.byte	0x15
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.4byte	.LSM3
	.byte	0x16
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.4byte	.LFE0
	.byte	0x0
	.uleb128 0x1
	.byte	0x1
.LELT0:
	.ident	"GCC: (GNU) 4.4.1 20110528 (platform)"
