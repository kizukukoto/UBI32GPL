/*
 * arch/ubicom32/include/asm/preempt.h
 *   Ubicom32 preempt enable/disable macros.
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
#ifndef _ASM_UBICOM32_PREEMPT_H
#define _ASM_UBICOM32_PREEMPT_H

#include <asm/ubicom32.h>
#include <asm/thread_info.h>

/*
 * Ubicom32 needs a private preempt enable/disable macros primarily to
 * solve a circular include dependency:
 * 	irqflags.h - ldsr.h - preempt.h - irqflags.h
 */
#if defined(CONFIG_PREEMPT)
#define ubicom32_preempt_disable()				\
do {								\
  struct thread_info *ti = current_thread_info(); \
  asm volatile (                                  \
  "  add.4  %0, %0, %1    \n\t"               \
     :                                            \
     : "U4" (ti->preempt_count), "d" (1)          \
     : "memory", "cc"                             \
  );                                              \
} while (0)


#define ubicom32_preempt_enable_no_resched()			\
do {						        	\
  struct thread_info *ti = current_thread_info(); \
  asm volatile (                                  \
  "  add.4  %0, %0, %1    \n\t"               \
     :                                            \
     : "U4" (ti->preempt_count), "d" (-1)         \
     : "memory", "cc"                             \
  );                                              \
} while (0)

#else
#define ubicom32_preempt_disable()		do { } while (0)
#define ubicom32_preempt_enable_no_resched()	do { } while (0)
#endif

#endif // _ASM_UBICOM32_PREEMPT_H
