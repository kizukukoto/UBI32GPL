/*
 * arch/ubicom32/include/asm/page.h
 *   Memory page related operations and definitions.
 *
 * (C) Copyright 2009, Ubicom, Inc.
 *
 * This file is part of the Ubicom32 Linux Kernel Port.
 *
 * The Ubicom32 Linux Kernel Port is free software: you can redistribute
 * it and/or modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation, either version 2 of the
 * License, or (at your option) any later version.
 *
 * The Ubicom32 Linux Kernel Port is distributed in the hope that it
 * will be useful, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
 * the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with the Ubicom32 Linux Kernel Port.  If not,
 * see <http://www.gnu.org/licenses/>.
 *
 * Ubicom32 implementation derived from (with many thanks):
 *   arch/m68knommu
 *   arch/blackfin
 *   arch/parisc
 */
#ifndef _ASM_UBICOM32_PAGE_H
#define _ASM_UBICOM32_PAGE_H

/*
 * PAGE_SHIFT determines the page size
 */
#if defined(IP5000) || defined(IP7000)
#define PAGE_SHIFT	12
#endif

#if defined(IP8000)
#define PAGE_SHIFT	14
#endif

#define PAGE_SIZE	(1 << PAGE_SHIFT)
#define PAGE_MASK	(~(PAGE_SIZE - 1))

#include <asm/setup.h>

/*
 * We can only include io.h (which has the phys_to_virt() macros
 * for non-assembly language code but we need the page.h functionality
 * in both.
 */
#ifndef __ASSEMBLY__
#include <asm/io.h>

#define get_user_page(vaddr)		__get_free_page(GFP_KERNEL)
#define free_user_page(page, addr)	free_page(addr)
#define clear_page(page)		memset((page), 0, PAGE_SIZE)
#define copy_page(to,from)		memcpy((to), (from), PAGE_SIZE)
#define clear_user_page(page, vaddr, pg)	clear_page(page)
#define copy_user_page(to, from, vaddr, pg)	copy_page(to, from)

#define __alloc_zeroed_user_highpage(movableflags, vma, vaddr) \
	alloc_page_vma(GFP_HIGHUSER | __GFP_ZERO | movableflags, vma, vaddr)
#define __HAVE_ARCH_ALLOC_ZEROED_USER_HIGHPAGE

/*
 * These are used to make use of C type-checking..
 */
typedef struct page *pgtable_t;
typedef struct {unsigned long pte;} pte_t;
typedef struct {unsigned long pgd;} pgd_t;
typedef struct {unsigned long pgprot;} pgprot_t;

#define pte_val(x)	((x).pte)
#define pgd_val(x)	((x).pgd)
#define pgprot_val(x)	((x).pgprot)
#define __pte(x)	((pte_t){(x)})
#define __pgd(x)	((pgd_t){(x)})
#define __pgprot(x)	((pgprot_t){(x)})

extern unsigned long memory_start;
extern unsigned long memory_end;

#endif /* !__ASSEMBLY__ */

#include <asm/page_offset.h>

#define PAGE_OFFSET		(PAGE_OFFSET_RAW)
#define ARCH_PFN_OFFSET		(PAGE_OFFSET >> PAGE_SHIFT)


#ifndef __ASSEMBLY__

#define __pa(vaddr)		virt_to_phys((void *)(vaddr))
#define __va(paddr)		phys_to_virt((unsigned long)(paddr))

#define pfn_to_page(pfn)	(mem_map + ((pfn) - ARCH_PFN_OFFSET))
#define page_to_pfn(page)	((unsigned long)((page) - mem_map) + ARCH_PFN_OFFSET)
#define pfn_valid(pfn)	        ((unsigned long)(pfn) - ARCH_PFN_OFFSET < max_mapnr)

#define virt_to_pfn(kaddr)	(__pa(kaddr) >> PAGE_SHIFT)
#define pfn_to_virt(pfn)	(__va((pfn) << PAGE_SHIFT))

#define virt_to_page(addr)	pfn_to_page(virt_to_pfn(addr))
#define page_to_virt(page)	pfn_to_virt(page_to_pfn(page))

#define	virt_addr_valid(kaddr)	(((void *)(kaddr) >= (void *)PAGE_OFFSET) && \
				((void *)(kaddr) < (void *)memory_end))

#define VM_DATA_DEFAULT_FLAGS	(VM_READ | VM_WRITE | VM_EXEC | \
				 VM_MAYREAD | VM_MAYWRITE | VM_MAYEXEC)

#endif /* __ASSEMBLY__ */

#ifdef __KERNEL__
#include <asm-generic/page.h>
#endif

#endif /* _ASM_UBICOM32_PAGE_H */
