/*
 * ip5000.h
 *   Specific details for the UBICOM32 processor.
 *
 * Copyright ?2005-2010 Ubicom Inc. <www.ubicom.com>.  All rights reserved.
 *
 * This file contains confidential information of Ubicom, Inc. and your use of
 * this file is subject to the Ubicom Software License Agreement distributed with
 * this file. If you are uncertain whether you are an authorized user or to report
 * any unauthorized use, please contact Ubicom, Inc. at +1-408-789-2200.
 * Unauthorized reproduction or distribution of this file is subject to civil and
 * criminal penalties.
 *
 * $RCSfile: ip5000.h,v $
 * $Date: 2009/02/24 05:09:50 $
 * $Revision: 1.11 $
 */

/*
 * Memory Size -- use minimum value of Ubicom32
 * OCM
 *	IP5000: 192K@0x03000000 aliased to 0xbffc0000
 *	IP7000: 240K@0x03000000 aliased to 0xbffc0000
 *	IP8000: 256K@0xbffc0000
 * DDR
 *	IP5000: @0x40000000 aliased to 0xc0000000
 *	IP7000: @0x40000000 aliased to 0xc0000000
 *	IP8000: @0xc0000000
 * FLASH
 *	IP5000: @0x60000000
 *	IP7000: @0x60000000
 *	IP8000: @0xb0000000
 */
#define OCM_SECTOR_SIZE	0x00008000		/* 32K */
#define OCMSIZE 	0x00030000		/* 192K on-chip RAM for both program and data */
#define OCMSTART	0xbffc0000
#define OCMEND		(OCMSTART + OCMSIZE)

/*
 * Inline assembly define
 */
#define S(arg) #arg
#define D(arg) S(arg)

/*
 * Assembler include file
 */
#include "ip5000.inc"

/*
 * Interrupts
 */
#define INT_CHIP(reg, bit)	(((reg) << 5) | (bit))
#define INT_REG(interrupt)	(((interrupt) >> 5) * 4)
#define INT_SET(interrupt)	0x0114 + INT_REG(interrupt)
#define INT_CLR(interrupt)	0x0124 + INT_REG(interrupt)
#define INT_STAT(interrupt)	0x0104 + INT_REG(interrupt)
#define INT_MASK(interrupt)	0x00C0 + INT_REG(interrupt)
#define INT_BIT(interrupt)	((interrupt) & 0x1F)
#define INT_BIT_MASK(interrupt) (1 << INT_BIT(interrupt))

/*
 * GLOBAL_CTRL
 */
#define GLOBAL_CTRL_TRAP_RST_EN		(1 << 9)
#define GLOBAL_CTRL_AERROR_RST_EN	(1 << 8)
#define GLOBAL_CTRL_MT_MIN_DELAY(x)	(((x) & 0xf) << 3)
#define GLOBAL_CTRL_HRT_BANK_SELECT	(1 << 2)
#define GLOBAL_CTRL_INT_EN		(1 << 0)

/*
 * On Chip Peripherals Base.
 * IP5000/IP7000 @0x01000000 - aliased to 0xb9000000
 * IP8000 @0xb9000000
 */
#define OCP_BASE	0xb9000000
#define OCP_GENERAL	0x000
#define OCP_TIMERS	0x100
/*#define OCP_TRNG	0x200	   True Random Number Generator Control Reigsters */
#define OCP_DEBUG	0x300
#define OCP_SECURITY	0x400
#define OCP_ICCR	0x500	/* I-Cache Control Registers */
#define OCP_DCCR	0x600	/* D-Cache Control Registers */
#define OCP_OCMC	0x700	/* On Chip Memory Control Registers */
#define OCP_STATISTICS	0x800	/* Statistics Counters */
#define OCP_MTEST	0x900	/* Memory Test Registers */
#define OCP_DEBUG_INST	0x000	/* Up to 16M */

/*
 * General Configuration Registers (PLL)
 */
#define GENERAL_CFG_BASE (OCP_BASE + OCP_GENERAL)
#define GEN_CLK_CORE_CFG	0x00
#define GEN_CLK_IO_CFG		0x04
#define GEN_CLK_DDR_CFG		0x08
#define GEN_CLK_SLIP_CLR	0x10
#define GEN_CLK_SLIP_STATUS	0x14
#define GEN_SW_RESET		0x80
#define GEN_RESET_REASON	0x84
#define GEN_BOND_CFG		0x88

#define GEN_CLK_PLL_SECURITY_BIT_NO	31
#define GEN_CLK_PLL_SECURITY		(1 << GEN_CLK_PLL_SECURITY_BIT_NO)
#define GEN_CLK_PLL_ENSAT		(1 << 30)
#define GEN_CLK_PLL_FASTEN		(1 << 29)
#define GEN_CLK_PLL_NR(v)		(((v) - 1) << 23)
#define GEN_GET_CLK_PLL_NR(v)		((((v) >> 23) & 0x003f) + 1)


#define OSC1_FREQ			12000000
#define GEN_CLK_MPT_FREQ		OSC1_FREQ
#define GEN_CLK_PLL_NF(v)		(((v) - 1) << 11)
#define GEN_GET_CLK_PLL_NF(v)		((((v) >> 11) & 0x0fff) + 1)
#define GEN_CLK_PLL_OD(v)		(((v) - 1) << 8)
#define GEN_GET_CLK_PLL_OD(v)		((((v) >> 8) & 0x7) + 1)
#define GEN_CLK_PLL_SLOWDOWN		0
#define GEN_PLL_OD_VALUE		1
#define DDR_PLL_OD_VALUE		(2 * GEN_PLL_OD_VALUE)	/* a hidden div-by-2 */

#define OSC1_FREQ_JUPITER		48000000
#define GEN_CLK_MPT_FREQ_JUPITER	(OSC1_FREQ_JUPITER / 16)
#define GEN_CLK_PLL_NF_JUPITER(v)	((2 * (v) - 1) << 10)
#define GEN_GET_CLK_PLL_NF_JUPITER(v)	(((((v) >> 10) & 0x1fff) + 1) / 2)
#define GEN_CLK_PLL_OD_JUPITER(v)	((16 / (v)) & 0x7)
#define GEN_GET_CLK_PLL_OD_JUPITER(v)	(16 / ((((v) - 1) & 0x7) + 1))
#define GEN_CLK_PLL_AUX_REF_JUPITER()	(1 << 8)
#define GEN_CLK_PLL_SLOWDOWN_JUPITER	2
#define GEN_PLL_OD_VALUE_JUPITER	2	/* a hidden div-by-2 */
#define DDR_PLL_OD_VALUE_JUPITER	(1 * GEN_PLL_OD_VALUE)

#define GEN_CLK_PLL_RESET	(1 << 7)
#define GEN_CLK_PLL_BYPASS	(1 << 6)
#define GEN_CLK_PLL_POWERDOWN	(1 << 5)
#define GEN_CLK_PLL_SELECT	(1 << 4)

#define PLL_IS_EXACT(N, D)	((((N) / (D)) * (D)) == (N))

#if   (PLL_IS_EXACT((SYSTEM_FREQ / 1000) * GEN_PLL_OD_VALUE * 1, (OSC1_FREQ / 1000)))
# define CORE_PLL_NR_VALUE 1
#elif (PLL_IS_EXACT((SYSTEM_FREQ / 1000) * GEN_PLL_OD_VALUE * 2, (OSC1_FREQ / 1000)))
# define CORE_PLL_NR_VALUE 2
#elif (PLL_IS_EXACT((SYSTEM_FREQ / 1000) * GEN_PLL_OD_VALUE * 3, (OSC1_FREQ / 1000)))
# define CORE_PLL_NR_VALUE 3
#elif (PLL_IS_EXACT((SYSTEM_FREQ / 1000) * GEN_PLL_OD_VALUE * 4, (OSC1_FREQ / 1000)))
# define CORE_PLL_NR_VALUE 4
#elif (PLL_IS_EXACT((SYSTEM_FREQ / 1000) * GEN_PLL_OD_VALUE * 6, (OSC1_FREQ / 1000)))
# define CORE_PLL_NR_VALUE 6
#elif (PLL_IS_EXACT((SYSTEM_FREQ / 1000) * GEN_PLL_OD_VALUE * 8, (OSC1_FREQ / 1000)))
# define CORE_PLL_NR_VALUE 8
#elif (PLL_IS_EXACT((SYSTEM_FREQ / 1000) * GEN_PLL_OD_VALUE * 12, (OSC1_FREQ / 1000)))
# define CORE_PLL_NR_VALUE 12
#else
# error "Invalid SYSTEM_FREQ for given OSC1_FREQ"
#endif
#define CORE_PLL_NF_VALUE ((SYSTEM_FREQ / 1000) * GEN_PLL_OD_VALUE * CORE_PLL_NR_VALUE / (OSC1_FREQ / 1000))
#define CORE_PLL_NF_VALUE_JUPITER ((SYSTEM_FREQ / 1000) * GEN_PLL_OD_VALUE_JUPITER * CORE_PLL_NR_VALUE / (OSC1_FREQ_JUPITER / 1000))

#define GEN_CORE_PLL_CFG_VALUE (GEN_CLK_PLL_ENSAT | GEN_CLK_PLL_NR(CORE_PLL_NR_VALUE) | GEN_CLK_PLL_NF(CORE_PLL_NF_VALUE) | GEN_CLK_PLL_OD(GEN_PLL_OD_VALUE))
#define GEN_CORE_PLL_CFG_VALUE_JUPITER (GEN_CLK_PLL_ENSAT | GEN_CLK_PLL_NR(CORE_PLL_NR_VALUE) | GEN_CLK_PLL_NF_JUPITER(CORE_PLL_NF_VALUE_JUPITER) | GEN_CLK_PLL_OD_JUPITER(GEN_PLL_OD_VALUE_JUPITER))

/*
 * Timer block
 */
#define TIMER_BASE		(OCP_BASE + OCP_TIMERS)
#define TIMER_MPTVAL		0x00
#define TIMER_RTCOM		0x04
#define TIMER_TKEY		0x08
#define TIMER_WDCOM		0x0c
#define TIMER_WDCFG		0x10
#define TIMER_SYSVAL		0x14
#define TIMER_SYSCOM(tmr)	(0x18 + (tmr) * 4)
#define TIMER_TRN_CFG		0x100
#define TIMER_TRN		0x104

#define TIMER_COUNT 10
#define REALTIME_TIMER_INT		INT_CHIP(0, 30)
#define TIMER_INT(tmr)			INT_CHIP(1, (tmr))
#define TIMER_TKEYVAL			0xa1b2c3d4
#define TIMER_WATCHDOG_DISABLE		0x4d3c2b1a
#define TIMER_TRN_CFG_ENABLE_OSC	0x00000007

/*
 * OCP-Debug Module (Mailbox)
 */
#define ISD_MAILBOX_BASE	(OCP_BASE + OCP_DEBUG)
#define ISD_MAILBOX_IN		0x00
#define ISD_MAILBOX_OUT		0x04
#define ISD_MAILBOX_STATUS	0x08

#define ISD_MAILBOX_INT		INT_CHIP(1, 30)

#define ISD_MAILBOX_STATUS_IN_FULL	(1 << 31)
#define ISD_MAILBOX_STATUS_IN_EMPTY	(1 << 30)
#define ISD_MAILBOX_STATUS_OUT_FULL	(1 << 29)
#define ISD_MAILBOX_STATUS_OUT_EMPTY	(1 << 28)

/*
 * OCP-ICCR
 */
#define ICCR_BASE (OCP_BASE + OCP_ICCR)
#define ICACHE_TOTAL_SIZE 65536			/* in bytes - max value */

/*
 * OCP-DCCR
 */
#define DCCR_BASE (OCP_BASE + OCP_DCCR)
#define DCACHE_TOTAL_SIZE 65536			/* in bytes - max value */

#define CACHE_LINE_SHIFT	5
#define CACHE_LINE_SIZE		(1 << CACHE_LINE_SHIFT)	/* in bytes */

#define CCR_ADDR	0x00
#define CCR_RDD		0x04
#define CCR_WRD		0x08
#define CCR_STAT	0x0c
#define CCR_CTRL	0x10

#define CCR_STAT_MCBE	0
#define CCR_STAT_WIDEL	1				/* D-cache only */

#define CCR_CTRL_DONE 			0
#define CCR_CTRL_RESET 			2
#define CCR_CTRL_VALID 			3
#define CCR_CTRL_RD_DATA		(1 << 4)
#define CCR_CTRL_RD_TAG			(2 << 4)
#define CCR_CTRL_WR_DATA		(3 << 4)
#define CCR_CTRL_WR_TAG			(4 << 4)
#define CCR_CTRL_INV_INDEX		(5 << 4)
#define CCR_CTRL_INV_ADDR		(6 << 4)
#define CCR_CTRL_FLUSH_INDEX		(7 << 4)	/* D-cache only */
#define CCR_CTRL_FLUSH_INV_INDEX	(8 << 4)	/* D-cache only */
#define CCR_CTRL_FLUSH_ADDR		(9 << 4)	/* D-cache only */
#define CCR_CTRL_FLUSH_INV_ADDR		(10 << 4)	/* D-cache only */

/*
 * OCP-OCMC
 */
#define OCMC_BASE		(OCP_BASE + OCP_OCMC)
#define OCMC_BANK_MASK		0x00

#define OCMC_BANK_PROG(n)	((1<<(n))-1)

/*
 * Port registers
 * IP5000/IP7000 @0x02000000 - aliased to 0xba000000
 * IP8000 @0xba000000
 */
#define IO_BASE	0xba000000
#define RA	(IO_BASE + 0x00000000)

#define IO_FUNC_FUNCTION_CLK(func)	((1 << ((func) - 1)) << 24)	/* Function 0 doesn't need clock */
#define IO_FUNC_FUNCTION_RESET(func)	((1 << ((func) - 1)) << 4)	/* Function 0 doesn't need reset */
#define IO_FUNC_RX_FIFO			(1 << 3)
#define IO_FUNC_SELECT(func)		((func) << 0)

/*
 * Flash (Port A)
 */
#define IO_XFL_BASE	RA

#define IO_XFL_INT_START	(1 << 16)
#define IO_XFL_INT_ERR		(1 << 8)
#define IO_XFL_INT_DONE		(1 << 0)

#define IO_XFL_CTL0_MASK			(0xffe07fff)
#define IO_XFL_CTL0_RD_CMD(cmd)			(((cmd) & 0xff) << 24)
#define IO_XFL_CTL0_RD_DUMMY(n)			(((n) & 0x7) << 21)
#define IO_XFL_CTL0_CLK_WIDTH(core_cycles)	((((core_cycles) + 1) & 0x7e) << 8)	/* must be even number */
#define IO_XFL_CTL0_CE_WAIT(spi_cycles)		(((spi_cycles) & 0x3f) << 2)
#define IO_XFL_CTL0_MCB_LOCK			(1 << 1)
#define IO_XFL_CTL0_ENABLE			(1 << 0)
#define IO_XFL_CTL0_FAST_VALUE(div, wait)	(IO_XFL_CTL0_RD_CMD(0xb) | IO_XFL_CTL0_RD_DUMMY(1) | IO_XFL_CTL0_CLK_WIDTH(div) | IO_XFL_CTL0_CE_WAIT(wait) | IO_XFL_CTL0_ENABLE)
#define IO_XFL_CTL0_VALUE(div, wait)		(IO_XFL_CTL0_RD_CMD(3) | IO_XFL_CTL0_CLK_WIDTH(div) | IO_XFL_CTL0_CE_WAIT(wait) | IO_XFL_CTL0_ENABLE)

#define IO_XFL_CTL1_MASK			(0xc0003fff)
#define IO_XFL_CTL1_FC_INST(inst)		(((inst) & 0x3) << 30)
#define IO_XFL_CTL1_FC_DATA(n)			(((n) & 0x3ff) << 4)
#define IO_XFL_CTL1_FC_DUMMY(n)			(((n) & 0x7) << 1)
#define IO_XFL_CTL1_FC_ADDR			(1 << 0)

#define IO_XFL_CTL2_FC_CMD(cmd)			(((cmd) & 0xff) << 24)
#define IO_XFL_CTL2_FC_ADDR(addr)		((addr) & 0x00ffffff)	/* Only up to 24 bits */

#define IO_XFL_STATUS0_MCB_ACTIVE		(1 << 0)
#define IO_XFL_STATUS0_IOPCS_ACTIVE		(1 << 1)

/*
 * FIFO
 */
#define IO_PORTX_INT_FIFO_TX_RESET	(1 << 31)
#define IO_PORTX_INT_FIFO_RX_RESET	(1 << 30)
#define IO_PORTX_INT_FIFO_TX_UF		(1 << 15)
#define IO_PORTX_INT_FIFO_TX_WM		(1 << 14)
#define IO_PORTX_INT_FIFO_RX_OF		(1 << 13)
#define IO_PORTX_INT_FIFO_RX_WM		(1 << 12)
