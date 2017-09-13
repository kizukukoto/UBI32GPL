/*
 * arch/ubicom32/mach-common/profile_vma.c
 *   Implementation for Ubicom32 Profiler vma walker
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
 */
#include <linux/mm.h>
#include <asm/profpkt.h>

int profile_get_vma(struct profile_map *profile_pm, int num, int profile_num_maps)
{
#if !defined(CONFIG_MMU)
	struct rb_node *rb __attribute__ ((unused));
	struct vm_area_struct *vma __attribute__ ((unused));
	int type __attribute__ ((unused)) = PROFILE_MAP_TYPE_UNKNOWN_USED;
	int flags __attribute__ ((unused));
	down_read(&nommu_vma_sem);
	for (rb = rb_first(&nommu_vma_tree); rb && num < profile_num_maps; rb = rb_next(rb)) {
		vma = rb_entry(rb, struct vm_area_struct, vm_rb);
		profile_pm[num].start = (vma->vm_start - SDRAMSTART) >> PAGE_SHIFT;
		profile_pm[num].type_size = (vma->vm_end - vma->vm_start + (1 << PAGE_SHIFT) - 1) >> PAGE_SHIFT;
		flags = vma->vm_flags & 0xf;
		if (flags == (VM_READ | VM_EXEC)) {
			type = PROFILE_MAP_TYPE_TEXT;
		} else if (flags == (VM_READ | VM_WRITE | VM_EXEC)) {
			type = PROFILE_MAP_TYPE_STACK;
		} else if (flags == (VM_READ | VM_WRITE)) {
			type = PROFILE_MAP_TYPE_APP_DATA;
		}
		profile_pm[num].type_size |= type << PROFILE_MAP_TYPE_SHIFT;
		num++;
	}
	up_read(&nommu_vma_sem);
	if (rb) {
		return -1;
	}
#endif
	return num;
}
