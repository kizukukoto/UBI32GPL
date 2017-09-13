/* Target-dependent code for GDB, the GNU debugger.

   Copyright (C) 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008,
   2009
   Free Software Foundation, Inc.

   This file is part of GDB.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include <sys/types.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#if defined(_WIN32)
#include <winsock.h>
#define usleep(x) Sleep(x/1000)
#else
#include <netinet/in.h>
#include <unistd.h>
#endif
#include <string.h>
#include "remote-ubicom32-isd.h"
#include "remote-ubicom32-dbginterface.h"
#include "ubicom32_dongle.h"

//#define DEBUG 1

enum isd_command {
  ISD_NOP,
  ISD_OPEN,
  ISD_CLOSE,
  ISD_DEVICE_ID,
  ISD_DEVICE_VER,
  ISD_MAIL_STATUS,
  ISD_DEBUG_RST_REQ,
  ISD_WRITE_MAIL_BOX,
  ISD_READ_MAIL_BOX,
  ISD_CLEAN_MAIL,
  ISD_TEST_MODE,
  ISD_WR_MME,
  ISD_WR_MEC,
  ISD_WR_MEP,
  ISD_WR_MRC,
  ISD_WR_MOA,
  ISD_RD_MB,
  ISD_WR_MBRE = 0x10,
  ISD_WR_MEM,
  ISD_RD_MST,
  ISD_HALT_MP,
  ISD_REL_MP,
  ISD_FORCE_MP,
  ISD_RD_MP_REG,
  ISD_SET_MP_REG_ADDR,
  ISD_WR_MP_REG,
  ISD_RD_IPORT_STAT,
  ISD_WR_IBUF,
  ISD_WR_RST_HALT_MP_EN,
  ISD_RD_RST_HALT_MP,
};

/* Mail Box Queue Status bit defines XXXX Probably subject to change*/
#define M_BOX_IN_Q_EMPTY	0x40000000
#define M_BOX_IN_Q_FULL		0x80000000
#define M_BOX_OUT_Q_EMPTY	0x10000000
#define M_BOX_OUT_Q_FULL	0x20000000

extern int turnOnconsole(void);
extern int turnOffconsole(void);

extern unsigned int  rosr, pc, mt_active, mt_dbg_active, mt_en;
unsigned int mt_sched, mt_pri;

int ubicom32v3GdbIndexToRegAddrMap []={
  0x0,0x4,0x8,0xc,		// Data registers 0 - 15
  0x10,0x14,0x18,0x1c,
  0x20,0x24,0x28,0x2c,
  0x30,0x34,0x38,0x3c,
  0x80,0x84,0x88,0x8c,		// Address registers 0 - 7
  0x90,0x94,0x98,0x9c,
  0xa0,0xa4,0xa8,0xac,		// mac_hi - source3
  0xb0,0xb4,0xb8,0xbc,		// Inst_cnt - Iread_data
  0xc0,0xc4,			// 32 int_mask0 int_mask1
  0xd0,0xd4,0xd8,0xdc,		// 34 Thread PC trap_cause acc1_hi acc1_lo
  0xe0,				// 38 previous PC


				// Global registers
  0x100, 0x104, 0x108,		// 39 Chip_id - int_stat1
  0x114, 0x118,			// 42 int_set0 inst_set1
  0x124, 0x128,			// 44 int_clr0 inst_clr1
  0x134, 0x138, 0x13c,		// 46  Global Control mt_active mt_active_set
  0x140, 0x144, 0x148, 0x14c,	// 49 mt_active clr mt_dbg_active mt_dbg_active_set mt_en
  0x150, 0x154,	0x158, 0x15c,	// 53 mt_hpri, mt_hrt mt_break mt_break_clr
  0x160, 0x164,	0x168,		// 57 mt_single_step, mt_min_delay_en mt_break_set
  0x170, 0x17c,			// 60 dcapt mt_dbg_active_clr
  0x180, 0x184, 0x188, 0x18c,	// 62 scratchpad0 - scratchpad3
  0x1a0, 0x1a4, 0x1a8, 0x1ac,	// 66 chip_cfg mt_i_blocked mt_d_blocked mt_i_blocked_set
  0x1b0, 0x1b4, 0x1b8, 0x1bc,	// 70 mt_d_blocked_set mt_blocked_clr mt_trap_en mt_trap
  0x1c0, 0x1c4,			// 74 mt_trap_set mt_trap_clr
  0x200, 0x204, 0x208,		// 76 i_range0_hi i_range1_hi i_range2_hi
  0x220, 0x224, 0x228,		// 79 i_range0_lo i_range1_lo i_range2_lo
  0x240, 0x244, 0x248,		// 82 i_range0_en i_range1_en i_range2_en
  0x260, 0x264, 0x268, 0x26c,	// 85 d_range0_hi d_range1_hi d_range2_hi d_range3_hi
  0x280, 0x284, 0x288, 0x28c,	// 89 d_range0_lo d_range1_lo d_range2_lo d_range3_lo
  0x2a0, 0x2a4, 0x2a8, 0x2ac,	// 93 d_range0_en d_range1_en d_range2_en d_range3_en
};

int ubicom32v4GdbIndexToRegAddrMap []={
  0x0,0x4,0x8,0xc,		// Data registers 0 - 15
  0x10,0x14,0x18,0x1c,
  0x20,0x24,0x28,0x2c,
  0x30,0x34,0x38,0x3c,
  0x80,0x84,0x88,0x8c,		// Address registers 0 - 7
  0x90,0x94,0x98,0x9c,
  0xa0,0xa4,0xa8,0xac,		// mac_hi - source3
  0xb0,0xb4,0xb8,0xbc,		// Inst_cnt - Iread_data
  0xc0,0xc4,			// 32 int_mask0 int_mask1
  0xd0,0xd4,0xd8,0xdc,		// 34 Thread PC trap_cause acc1_hi acc1_lo
  0xe0,				// 38 previous PC


				// Global registers
  0x100, 0x104, 0x108,		// 39 Chip_id - int_stat1
  0x114, 0x118,			// 42 int_set0 inst_set1
  0x124, 0x128,			// 44 int_clr0 inst_clr1
  0x134, 0x138, 0x13c,		// 46  Global Control mt_active mt_active_set
  0x140, 0x144, 0x148, 0x14c,	// 49 mt_active clr mt_dbg_active mt_dbg_active_set mt_en
  0x150, 0x154,	0x158, 0x15c,	// 53 mt_hpri, mt_hrt mt_break mt_break_clr
  0x160, 0x164,	0x168,		// 57 mt_single_step, mt_min_delay_en mt_break_set
  0x170, 0x17c,			// 60 dcapt mt_dbg_active_clr
  0x180, 0x184, 0x188, 0x18c,	// 62 scratchpad0 - scratchpad3
  0x1a0, 0x1a4, 0x1a8, 0x1ac,	// 66 chip_cfg mt_i_blocked mt_d_blocked mt_i_blocked_set
  0x1b0, 0x1b4, 0x1b8, 0x1bc,	// 70 mt_d_blocked_set mt_blocked_clr mt_trap_en mt_trap
  0x1c0, 0x1c4,			// 74 mt_trap_set mt_trap_clr
  0x200, 0x204, 0x208, 0x20c,	// 76 i_range0_hi i_range1_hi i_range2_hi i_range3_hi
  0x220, 0x224, 0x228, 0x22c,	// 80 i_range0_lo i_range1_lo i_range2_lo i_ramge3_lo
  0x240, 0x244, 0x248, 0x24c,	// 84 i_range0_en i_range1_en i_range2_en i_range3_en
  0x260, 0x264, 0x268, 0x26c, 0x270,	// 88 d_range0_hi d_range1_hi d_range2_hi d_range3_hi d_range4_hi
  0x280, 0x284, 0x288, 0x28c, 0x290,	// 93 d_range0_lo d_range1_lo d_range2_lo d_range3_lo d_range4_lo
  0x2a0, 0x2a4, 0x2a8, 0x2ac, 0x2b0	// 98 d_range0_en d_range1_en d_range2_en d_range3_en d_range4_en
};


// Snaphot of the registers
ubicom32v3Regs_t ubicom32v3Registers;
ubicom32v4Regs_t ubicom32v4Registers;
int ubicom32v3gdbGetallRegs(struct ubicom32_cpu *cpu, int threadNo);
int ubicom32v4gdbGetallRegs(struct ubicom32_cpu *cpu, int threadNo);
int ubicom32v3gdbGetTPacketRegs(struct ubicom32_cpu *cpu, int threadNo);
int ubicom32v4gdbGetTPacketRegs(struct ubicom32_cpu *cpu, int threadNo);
int ubicom32v3writeRegisters( int threadNo, int regIndex, int *inBuf, unsigned int length);
int ubicom32v4writeRegisters( int threadNo, int regIndex, int *inBuf, unsigned int length);

struct ubicom32_cpu ubicom32_cpus[] = {
  {(void *)&ubicom32v3Registers, sizeof(ubicom32v3Registers), 10, NUM_PER_THREAD_REGS_UBI32V3, NUM_GLOBAL_REGS_UBI32V3, 0x123455aa, ubicom32v3GdbIndexToRegAddrMap,
   ubicom32v3gdbGetallRegs, ubicom32v3gdbGetTPacketRegs, NULL, NULL, ubicom32v3writeRegisters, NULL, NULL},
  {(void *)&ubicom32v4Registers, sizeof(ubicom32v4Registers), 12, NUM_PER_THREAD_REGS_UBI32V3, NUM_GLOBAL_REGS_UBI32V4, 0x123455ab, ubicom32v4GdbIndexToRegAddrMap,
   ubicom32v4gdbGetallRegs, ubicom32v4gdbGetTPacketRegs, NULL, NULL, ubicom32v4writeRegisters, NULL, NULL},
  { NULL, 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL}
};

struct ubicom32_cpu *current_cpu = ubicom32_cpus;

int ubicom32isp_detach(void);

extern unsigned int okToDoubleBuffer;
extern unsigned int dongleHasMarsSupport;
extern unsigned int dongleIsUbipod;
extern unsigned int ubipodControl2wire;
extern unsigned int ubipodControlconsole;
extern unsigned int errorExpected;
extern unsigned int doubleBuffer;
extern struct ubicom32command_packet *pending_packet;
extern unsigned int pending_packet_length;
extern unsigned int pending_packet_opcode;

extern int deviceId;
extern int deviceVersion;
extern int chipID;

unsigned int send_console_data;	/* This will be non zero if mailbox data has to be sent after restart */
unsigned int console_data[2];	/* This is the recovered console data. */

#if defined(_WIN32)
int ubicom32v3currentThread;
int ubicom32v3_current_thread;
#else
extern int ubicom32v3currentThread;
extern int ubicom32v3_current_thread;
#endif

struct thread_saved_state {
  unsigned int pc;
  unsigned int d0;
  unsigned int csr;
  unsigned int a0;
  unsigned int a1;
  unsigned int previous_pc;
} current_thread_saved_state;

struct direct_read_registers {
  unsigned int chip_id;
  unsigned int int_stat0;
  unsigned int int_stat1;
  unsigned int mt_active;
  unsigned int mt_dbg_active;
  unsigned int mt_en;
  unsigned int mt_break;
  unsigned int scratchpad0;
  unsigned int scratchpad1;
  unsigned int scratchpad2;
  unsigned int scratchpad3;
  unsigned int mt_i_blocked;
  unsigned int mt_d_blocked;
  unsigned int mt_trap;
} direct_read_regs;

unsigned int save_previous_pc = 0x01606138;	//move.4 scratchpad0,previous_pc

#if 0
unsigned int save_current_thread_state [] = {
  0x01636121,  //move.4 scratchpad3,a1
  0x01626120,  //move.4 scratchpad2,a0
  0x0161612d,  //move.4 scratchpad1,csr
  0x01606100,  //move.4 scratchpad0,d0
  0xe0020006, // moveai a0, #%hi(0x01000300) (Load mailbox base address into a0
};
#endif

unsigned int save_current_thread_state [] = {
  0x01606100,  //move.4 scratchpad0,d0
  0x01606121,  //move.4 scratchpad0,a1
  0x01606120,  //move.4 scratchpad0,a0
  0x0160612d,  //move.4 scratchpad0,csr
  0xe0020006, // moveai a0, #%hi(0x01000300) (Load mailbox base address into a0
};

unsigned int restore_current_thread_state [] = {
  0x01006400,  //move.4 d0,(a0)
  0x012d6400,  //move.4 csr,(a0)
  0x01216400,  //move.4 a1,(a0)
  0x01206400,  //move.4 a0,(a0)
  0xc9600003,  //movei scratchpad0,#3
  0xc9630000,  //movei scratchpad3,#0
};

unsigned int restore_watchdog [] = {
  0xe0220002,  //moveai a1,#%hi(0x01000100) Set a1 to timer base address
  0x04226400,  //move.4 8(a1),(a0)          Load tkey value from Mbox (0xa1b2c3d4)
  0x01006400,  //move.4 d0,(a0)		    Load d0 with timer increment (0x1000)
  0x7c230420,  //add.4 12(a1),(a1),d0	    timer->wdcom = timer->mptval + increment
  0xcc240000,  //movei 8(a1),#0		    timer->tkey = 0
};

unsigned int move_register_to_scratchpad0 = 0x01606100; // move.4 scratchpad0, d0 (lowest 8 bits = reg address / 4
unsigned int move_a2_to_scratchpad0 = 0x01606122;	//move.4 scratchpad0,a2
unsigned int move_scratchpad0_to_a2 = 0x01226160;	//move.4 a2,scratchpad0
unsigned int move_memory_to_scratchpad0 = 0x01606221;	//move.4 scratchpad0,(a1)4++
unsigned int move_from_mailbox_to_register = 0x01006400; //move.4 d0, (a0) bits 16-23 is register address / 4
unsigned int move_from_mailbox_to_memory = 0x02216400; // move.4 (a1)4++, (a0)
unsigned int move_from_memory_to_mailbox = 0x04016221; // move.4 4(a0), (a1)4++
unsigned int move_from_mailbox_to_mailbox = 0x04016400; // move.4 4(a0),(a0)
unsigned int move_dcache_base_to_a1 = 0xe022000c; // moveai a1, #%hi(0x01000600)
unsigned int move_icache_base_to_a1 = 0xe022000a; // moveai a1, #%hi(0x01000500)
unsigned int move_icache_base_to_a2 = 0xe042000a;	// moveai a2,#%hi(0x01000500)
unsigned int load_dcache_control_reg = 0x04246400;	// move.4 16(a1), (a0)
unsigned int load_cache_control_reg = 0x04246400;	// move.4 16(a1), (a0)
unsigned int load_icache_control_reg = 0x04446400;	// move.4 16(a2),(a0)
unsigned int move_from_cache_dreg_to_mailbox = 0x04016421;	// move.4 4(a0),4(a1)
unsigned int load_dcache_addr_and_activate[] = {
	0x04206400, // move.4 (a1), (a0)
	0x24241c24, // bset 16(a1), 16(a1), #0x3
};

unsigned int load_cache_addr_and_activate[] = {
	0x04206400, // move.4 (a1), (a0)
	0x24241c24, // bset 16(a1), 16(a1), #0x3
};

unsigned int load_icache_addr_and_activate[] = {
  0x04406400,	//move.4 (a2),(a0)
  0x24441c44,	//bset 16(a2),16(a2),#0x3
};

unsigned int mailbox_base_to_a0 = 0xe0020006; // moveai a0, #%hi(0x01000300)
unsigned int move_from_mailbox_to_a1 = 0x01216400;	//move.4 a1,(a0)

unsigned single_step_deactivate_threads = 0xc95fffff;	//movei mt_dbg_active_clr,#-1
unsigned single_step_activate_thread = 0xc9520000;	//movei mt_dbg_active_set,#0
unsigned activate_all_threads = 0xc952ffff;	//movei mt_dbg_active_set,#-1
unsigned setup_mt_single_step = 0xc9580000;	//movei mt_single_step,#0

unsigned int clear_break_register = 0xc9570000;	//movei mt_break_clr,#0
unsigned int clear_trap_register = 0xc9710000;	//movei mt_trap_clr,#0

unsigned int bist_sequence_1[] = {
  0xe022000e,	//moveai a1,#%hi(0x01000700)
  0x04216060,	//move.4 4(a1),#96
  0x04216000,	//move.4 4(a1),#0
  0x24213c21,	//bset 4(a1),4(a1),#0x7
  0xd6e00001,	//jmpt.t 60000108 <__pram_bist+0x14>
  0x2c213c21,	//bclr 4(a1),4(a1),#0x7
  0x24211c21,	//bset 4(a1),4(a1),#0x3
  0x24211421,	//bset 4(a1),4(a1),#0x2
  0x24212421,	//bset 4(a1),4(a1),#0x4
  0xd6a00001,	//jmpt.f 6000011c <__pram_bist+0x28>
  0xd6e00001,	//jmpt.t 60000120 <__pram_bist+0x2c>
};

unsigned int bist_sequence_2[] = {
  0x24213c21,	//bset 4(a1),4(a1),#0x7
  0xd6e00001,	//jmpt.t 60000128 <__pram_bist+0x34>
  0x2c213c21,	//bclr 4(a1),4(a1),#0x7
  0x10c01c22,	//btst 8(a1),#0x3
  0xd1fffffc,	//jmpeq.t 60000120 <__pram_bist+0x2c>
};

static int fast_write_code_address;
static int fast_read_code_address;

static unsigned int iport_stat = 0;
static int already_in_ubicom32v3StopProcessor=0;

int
ubicom32v3haltMPandRreadDirectRegisters(struct direct_read_registers *direct_regs)
{
  struct ubicom32command_packet packet;
  struct ubicom32command *command;
  int count, ret;
  unsigned int reply;

  ubicom32command_packet_init(&packet);

  // stop all the threads by sending ISD_HALT_MP
  count = ubicom32command_packet_append(&packet, 0, ISD_HALT_MP, 1);

  /* Ship the packet down for execution */
  ret = ubicom32command_packet_send(&packet);
  if(ret)
    return ret;

  ubicom32command_packet_init(&packet);

  /* recover Chip ID*/
  count = ubicom32command_packet_append(&packet, 0, ISD_RD_MP_REG, 0x100);
  count = ubicom32command_packet_append(&packet, 0, ISD_NOP, 0);

  /* recover int_stat0*/
  count = ubicom32command_packet_append(&packet, 0, ISD_RD_MP_REG, 0x104);
  count = ubicom32command_packet_append(&packet, 0, ISD_NOP, 0);

  /* recover int_stat 1*/
  count = ubicom32command_packet_append(&packet, 0, ISD_RD_MP_REG, 0x108);
  count = ubicom32command_packet_append(&packet, 0, ISD_NOP, 0);

  /* recover mt_active*/
  count = ubicom32command_packet_append(&packet, 0, ISD_RD_MP_REG, 0x138);
  count = ubicom32command_packet_append(&packet, 0, ISD_NOP, 0);

  /* recover mt_dbg_active */
  count = ubicom32command_packet_append(&packet, 0, ISD_RD_MP_REG, 0x144);
  count = ubicom32command_packet_append(&packet, 0, ISD_NOP, 0);

  /* recover mt_en */
  count = ubicom32command_packet_append(&packet, 0, ISD_RD_MP_REG, 0x14c);
  count = ubicom32command_packet_append(&packet, 0, ISD_NOP, 0);

  /* recover mt_break */
  count = ubicom32command_packet_append(&packet, 0, ISD_RD_MP_REG, 0x158);
  count = ubicom32command_packet_append(&packet, 0, ISD_NOP, 0);

  /* recover scratchpad0*/
  count = ubicom32command_packet_append(&packet, 0, ISD_RD_MP_REG, 0x180);
  count = ubicom32command_packet_append(&packet, 0, ISD_NOP, 0);

  /* recover scratchpad1*/
  count = ubicom32command_packet_append(&packet, 0, ISD_RD_MP_REG, 0x184);
  count = ubicom32command_packet_append(&packet, 0, ISD_NOP, 0);

  /* recover scratchpad2 */
  count = ubicom32command_packet_append(&packet, 0, ISD_RD_MP_REG, 0x188);
  count = ubicom32command_packet_append(&packet, 0, ISD_NOP, 0);

  /* recover scratchpad3 */
  count = ubicom32command_packet_append(&packet, 0, ISD_RD_MP_REG, 0x18c);
  count = ubicom32command_packet_append(&packet, 0, ISD_NOP, 0);

  /* recover mt_i_blocked */
  count = ubicom32command_packet_append(&packet, 0, ISD_RD_MP_REG, 0x1a4);
  count = ubicom32command_packet_append(&packet, 0, ISD_NOP, 0);

  /* recover mt_d_blocked */
  count = ubicom32command_packet_append(&packet, 0, ISD_RD_MP_REG, 0x1a8);
  count = ubicom32command_packet_append(&packet, 0, ISD_NOP, 0);

  /* recover mt_trap */
  count = ubicom32command_packet_append(&packet, 0, ISD_RD_MP_REG, 0x1bc);
  count = ubicom32command_packet_append(&packet, 0, ISD_NOP, 0);

  /* ship down the packet */
  ret = ubicom32command_packet_send(&packet);
  if(ret)
    return ret;

  command = &packet.commands[0];

  // extract reply
  reply = (command->operand0<<24) | (command->operand1<<16) | (command->operand2<<8) | command->operand3;
  direct_regs->chip_id = reply;
  command +=2;

  // extract reply
  reply = (command->operand0<<24) | (command->operand1<<16) | (command->operand2<<8) | command->operand3;
  direct_regs->int_stat0 = reply;
  command +=2;

  // extract reply
  reply = (command->operand0<<24) | (command->operand1<<16) | (command->operand2<<8) | command->operand3;
  direct_regs->int_stat1 = reply;
  command +=2;

  // extract reply
  reply = (command->operand0<<24) | (command->operand1<<16) | (command->operand2<<8) | command->operand3;
  direct_regs->mt_active = reply;
  mt_active = reply;
  command +=2;

  // extract reply
  reply = (command->operand0<<24) | (command->operand1<<16) | (command->operand2<<8) | command->operand3;
  direct_regs->mt_dbg_active = reply;
  mt_dbg_active = reply;
  command +=2;

  // extract reply
  reply = (command->operand0<<24) | (command->operand1<<16) | (command->operand2<<8) | command->operand3;
  direct_regs->mt_en = reply;
  mt_en = reply;
  command +=2;

  // extract reply
  reply = (command->operand0<<24) | (command->operand1<<16) | (command->operand2<<8) | command->operand3;
  direct_regs->mt_break = reply;
  command +=2;

  // extract reply
  reply = (command->operand0<<24) | (command->operand1<<16) | (command->operand2<<8) | command->operand3;
  direct_regs->scratchpad0 = reply;
  command +=2;

  // extract reply
  reply = (command->operand0<<24) | (command->operand1<<16) | (command->operand2<<8) | command->operand3;
  direct_regs->scratchpad1 = reply;
  command +=2;

  // extract reply
  reply = (command->operand0<<24) | (command->operand1<<16) | (command->operand2<<8) | command->operand3;
  direct_regs->scratchpad2 = reply;
  command +=2;

  // extract reply
  reply = (command->operand0<<24) | (command->operand1<<16) | (command->operand2<<8) | command->operand3;
  direct_regs->scratchpad3 = reply;
  command +=2;

  // extract reply
  reply = (command->operand0<<24) | (command->operand1<<16) | (command->operand2<<8) | command->operand3;
  direct_regs->mt_i_blocked = reply;
  command +=2;

  // extract reply
  reply = (command->operand0<<24) | (command->operand1<<16) | (command->operand2<<8) | command->operand3;
  direct_regs->mt_d_blocked = reply;
  command +=2;

  // extract reply
  reply = (command->operand0<<24) | (command->operand1<<16) | (command->operand2<<8) | command->operand3;
  direct_regs->mt_trap = reply;
  command +=2;

  return 0;
}

/*
 * ubicom32v3setupDebuggerThread(thredNo)
 * All threads are in a halted state by a HALT_MP command
 * This routine sets up a given thread as the debugger thread by a. Saving the thread's PC by reading it via a ISD_RD_MP_REG command.
 * The thread pc is then set to point to the debug space ie PC is set to 0x1000000 by a ISD_SET_MP_REG_ADDR command.
 * Force the thread to run with a FORCE_MP command
 * Save a0, a1 and csr by executing move.4 scratchpad3, a1; move.4 scratchpad2, a0; move.4 scratchpad2, csr via debug port
 * Save d0 by move.4 scratchpad0, d0 and reading back scratchpad0 to host by a ISD_RD_MP_REG command.
 * Set a0 to mailbox base by executing moveai a0, #%hi(0x01000300) via debug port
 */
int
ubicom32v3setupDebuggerThread(unsigned int threadNo)
{
  struct ubicom32command_packet packet;
  struct ubicom32command *command;
  int count, ret;
  int pcIndex, d0Index, previous_pc_index;
  unsigned int reply;

  ubicom32command_packet_init(&packet);

  // send down ISD_RD_MP_REG PC for this thread
  ubicom32v3currentThread = threadNo;
  ubicom32v3_current_thread = threadNo+1;

  /* recover thread PC */
  count = ubicom32command_packet_append(&packet, 0, ISD_RD_MP_REG, (0xd0 | ubicom32v3currentThread <<10));
  pcIndex = count;
  count = ubicom32command_packet_append(&packet, 0, ISD_NOP, 0);

  /* set thread PC to debug space */
  count = ubicom32command_packet_append(&packet, 0, ISD_SET_MP_REG_ADDR, (0xd0 | ubicom32v3currentThread <<10));
  count = ubicom32command_packet_append(&packet, 0, ISD_WR_MP_REG, 0x1000000);

  /* force the current thread to execute out of debug space */
  count = ubicom32command_packet_append(&packet, 0, ISD_FORCE_MP, (1<< ubicom32v3currentThread));

  count = ubicom32command_packet_append(&packet, 0, ISD_RD_IPORT_STAT, 0);
  count = ubicom32command_packet_append(&packet, 0, ISD_RD_IPORT_STAT, 0);

  /* Save previous PC. */
  count = ubicom32command_packet_append(&packet, IBUF_CHECK, ISD_WR_IBUF, save_previous_pc);

  /* retrieve contents of scratchpad0 using ISD_RD_MP_REG. Scratchpad0 has previous_pc contents */
  count = ubicom32command_packet_append(&packet, 0, ISD_RD_MP_REG, 0x180);
  previous_pc_index = count;
  count = ubicom32command_packet_append(&packet, 0, ISD_NOP, 0);

  /* Save d0 in scratchpad0. */
  count = ubicom32command_packet_append(&packet, IBUF_CHECK, ISD_WR_IBUF, save_current_thread_state[0]);

  /* retrieve contents of scratchpad0 using ISD_RD_MP_REG. Scratchpad0 has d0 contents */
  count = ubicom32command_packet_append(&packet, 0, ISD_RD_MP_REG, 0x180);
  d0Index = count;
  count = ubicom32command_packet_append(&packet, 0, ISD_NOP, 0);

  /* Save a1 in scratchpad0. */
  count = ubicom32command_packet_append(&packet, IBUF_CHECK, ISD_WR_IBUF, save_current_thread_state[1]);

  /* retrieve contents of scratchpad0 using ISD_RD_MP_REG. Scratchpad0 has a1 contents */
  count = ubicom32command_packet_append(&packet, 0, ISD_RD_MP_REG, 0x180);
  count = ubicom32command_packet_append(&packet, 0, ISD_NOP, 0);

  /* Save a0 in scratchpad0. */
  count = ubicom32command_packet_append(&packet, IBUF_CHECK, ISD_WR_IBUF, save_current_thread_state[2]);

  /* retrieve contents of scratchpad0 using ISD_RD_MP_REG. Scratchpad0 has a0 contents */
  count = ubicom32command_packet_append(&packet, 0, ISD_RD_MP_REG, 0x180);
  count = ubicom32command_packet_append(&packet, 0, ISD_NOP, 0);

  /* Save csr in scratchpad0. */
  count = ubicom32command_packet_append(&packet, IBUF_CHECK, ISD_WR_IBUF, save_current_thread_state[3]);

  /* retrieve contents of scratchpad0 using ISD_RD_MP_REG. Scratchpad0 has csr contents */
  count = ubicom32command_packet_append(&packet, 0, ISD_RD_MP_REG, 0x180);
  count = ubicom32command_packet_append(&packet, 0, ISD_NOP, 0);

  /* load mailbox base address into a0 */
  count = ubicom32command_packet_append(&packet, IBUF_CHECK, ISD_WR_IBUF, save_current_thread_state[4]);

  /* issue RD_IPORT_STAT command. */
  count = ubicom32command_packet_append(&packet, 0, ISD_RD_IPORT_STAT, 0);
  count = ubicom32command_packet_append(&packet, 0, ISD_NOP, 0);

  /* ship down the packet */
  ret = ubicom32command_packet_send(&packet);
  if(ret)
    return ret;

  command = &packet.commands[pcIndex];

  // extract reply
  reply = (command->operand0<<24) | (command->operand1<<16) | (command->operand2<<8) | command->operand3;

  if((reply & 0xff000000) == 0x01000000)
    {
      /* This is bad. Issue a reset and try to recover */
      extern int ubicom32v3isp_reset(void);
      extern int ubicom32v3restartProcessor(unsigned int);
      extern int ubicom32v3waitForBP(bpReason_t *);
      extern int ubicom32v3stopProcessor(void);
      ret = ubicom32v3isp_reset();
#if 0
      ret = ubicom32v3restartProcessor(1);

      for (i=0; i< 400; i++)
	{
	  ret = ubicom32v3waitForBP(&reason);

	  if(ret< 0)
	    {
	      //printf_unfiltered ("error: unable to read status\n");
	      break;
	    }

	  if(ret == 0)
	    {
	      /* wait for 10 ms before trying again */
	      usleep(10000);
	      continue;
	}
	  else
	    {
	      break;
	    }
	}

      //printf("Out of the loop i = %d\n", i);
      if((i == 400) || (ret < 0))
	{
	  /* The processor is still running, we need to stop it. */
	  if(already_in_ubicom32v3StopProcessor)
	    {
	      ret = ubicom32v3isp_reset();
	    }
	  else
	    {
	      ret = ubicom32v3stopProcessor();
	    }
	}
#endif
      return ret;
    }
  else
    current_thread_saved_state.pc = reply;

  command = &packet.commands[previous_pc_index];

  // extract previous_pc
  reply = (command->operand0<<24) | (command->operand1<<16) | (command->operand2<<8) | command->operand3;
  current_thread_saved_state.previous_pc = reply;

  command = &packet.commands[d0Index];

  // extract reply d0
  reply = (command->operand0<<24) | (command->operand1<<16) | (command->operand2<<8) | command->operand3;
  current_thread_saved_state.d0 = reply;

  command+= 3;

  // extract reply a1
  reply = (command->operand0<<24) | (command->operand1<<16) | (command->operand2<<8) | command->operand3;
  current_thread_saved_state.a1 = reply;

  command+=3;

  // extract reply a0
  reply = (command->operand0<<24) | (command->operand1<<16) | (command->operand2<<8) | command->operand3;
  current_thread_saved_state.a0 = reply;

  command+=3;

  // extract reply csr
  reply = (command->operand0<<24) | (command->operand1<<16) | (command->operand2<<8) | command->operand3;
  current_thread_saved_state.csr = reply;

  return ret;
}

#if 0
int
ubicom32v3setupDebuggerThread(unsigned int threadNo)
{
  struct ubicom32command_packet packet;
  struct ubicom32command *command;
  int count, ret, i;
  int pcIndex, d0Index, previous_pc_index;
  unsigned int reply;

  ubicom32command_packet_init(&packet);

  // send down ISD_RD_MP_REG PC for this thread
  ubicom32v3currentThread = threadNo;
  ubicom32v3_current_thread = threadNo+1;

  /* recover thread PC */
  count = ubicom32command_packet_append(&packet, 0, ISD_RD_MP_REG, (0xd0 | ubicom32v3currentThread <<10));
  pcIndex = count;
  count = ubicom32command_packet_append(&packet, 0, ISD_NOP, 0);

  /* set thread PC to debug space */
  count = ubicom32command_packet_append(&packet, 0, ISD_SET_MP_REG_ADDR, (0xd0 | ubicom32v3currentThread <<10));
  count = ubicom32command_packet_append(&packet, 0, ISD_WR_MP_REG, 0x1000000);

  /* force the current thread to execute out of debug space */
  count = ubicom32command_packet_append(&packet, 0, ISD_FORCE_MP, (1<< ubicom32v3currentThread));

  count = ubicom32command_packet_append(&packet, 0, ISD_RD_IPORT_STAT, 0);
  count = ubicom32command_packet_append(&packet, 0, ISD_RD_IPORT_STAT, 0);

  /* Save previous PC. */
  count = ubicom32command_packet_append(&packet, IBUF_CHECK, ISD_WR_IBUF, save_previous_pc);

  /* retrieve contents of scratchpad0 using ISD_RD_MP_REG. Scratchpad0 has previous_pc contents */
  count = ubicom32command_packet_append(&packet, 0, ISD_RD_MP_REG, 0x180);
  previous_pc_index = count;
  count = ubicom32command_packet_append(&packet, 0, ISD_NOP, 0);

  /* send down the register save instructions */
  for(i=0; i< 5; i++)
    {
      count = ubicom32command_packet_append(&packet, IBUF_CHECK, ISD_WR_IBUF, save_current_thread_state[i]);
    }

  /* retrieve contents of scratchpad0 using ISD_RD_MP_REG. Scratchpad0 has d0 contents */
  count = ubicom32command_packet_append(&packet, 0, ISD_RD_MP_REG, 0x180);
  d0Index = count;

  count = ubicom32command_packet_append(&packet, 0, ISD_NOP, 0);

  /* retrieve contents of scratchpad0 using ISD_RD_MP_REG. Scratchpad1 has csr contents */
  count = ubicom32command_packet_append(&packet, 0, ISD_RD_MP_REG, 0x184);
  count = ubicom32command_packet_append(&packet, 0, ISD_NOP, 0);

  /* retrieve contents of scratchpad0 using ISD_RD_MP_REG. Scratchpad2 has a0 contents */
  count = ubicom32command_packet_append(&packet, 0, ISD_RD_MP_REG, 0x188);
  count = ubicom32command_packet_append(&packet, 0, ISD_NOP, 0);

  /* retrieve contents of scratchpad0 using ISD_RD_MP_REG. Scratchpad3 has a1 contents */
  count = ubicom32command_packet_append(&packet, 0, ISD_RD_MP_REG, 0x18c);
  count = ubicom32command_packet_append(&packet, 0, ISD_NOP, 0);
  count = ubicom32command_packet_append(&packet, 0, ISD_RD_IPORT_STAT, 0);
  count = ubicom32command_packet_append(&packet, 0, ISD_NOP, 0);

  /* ship down the packet */
  ret = ubicom32command_packet_send(&packet);
  if(ret)
    return ret;

  command = &packet.commands[pcIndex];

  // extract reply
  reply = (command->operand0<<24) | (command->operand1<<16) | (command->operand2<<8) | command->operand3;

  if((reply & 0xff000000) == 0x01000000)
    {
      /* This is bad. Issue a reset and try to recover */
      extern int ubicom32v3isp_reset(void);
      extern int ubicom32v3restartProcessor(unsigned int);
      extern int ubicom32v3waitForBP(bpReason_t *);
      extern int ubicom32v3stopProcessor(void);
      int i;
      bpReason_t reason;
      ret = ubicom32v3isp_reset();
#if 0
      ret = ubicom32v3restartProcessor(1);

      for (i=0; i< 400; i++)
	{
	  ret = ubicom32v3waitForBP(&reason);

	  if(ret< 0)
	    {
	      //printf_unfiltered ("error: unable to read status\n");
	      break;
	    }

	  if(ret == 0)
	    {
	      /* wait for 10 ms before trying again */
	      usleep(10000);
	      continue;
	}
	  else
	    {
	      break;
	    }
	}

      //printf("Out of the loop i = %d\n", i);
      if((i == 400) || (ret < 0))
	{
	  /* The processor is still running, we need to stop it. */
	  if(already_in_ubicom32v3StopProcessor)
	    {
	      ret = ubicom32v3isp_reset();
	    }
	  else
	    {
	      ret = ubicom32v3stopProcessor();
	    }
	}
#endif
      return ret;
    }
  else
    current_thread_saved_state.pc = reply;

  command = &packet.commands[previous_pc_index];

  // extract previous_pc
  reply = (command->operand0<<24) | (command->operand1<<16) | (command->operand2<<8) | command->operand3;
  current_thread_saved_state.previous_pc = reply;

  command = &packet.commands[d0Index];

  // extract reply
  reply = (command->operand0<<24) | (command->operand1<<16) | (command->operand2<<8) | command->operand3;
  current_thread_saved_state.d0 = reply;

  command++;
  command++;

  // extract reply
  reply = (command->operand0<<24) | (command->operand1<<16) | (command->operand2<<8) | command->operand3;
  current_thread_saved_state.csr = reply;

  command++;
  command++;

  // extract reply
  reply = (command->operand0<<24) | (command->operand1<<16) | (command->operand2<<8) | command->operand3;
  current_thread_saved_state.a0 = reply;

  command++;
  command++;

  // extract reply
  reply = (command->operand0<<24) | (command->operand1<<16) | (command->operand2<<8) | command->operand3;
  current_thread_saved_state.a1 = reply;

  return ret;
}
#endif


/*
 * ubicom32v3restoreDebuggerThread()
 * This routine restores the ubicom32v3currentThread back to its original state by restoring watchdog and
 * restoring d0, csr, a0 and a1
 * Restore d0 by writing saved value of d0 to mailbox and execute move.4 d0, (a0)
 * restore csr by writing saved value of d0 to mailbox and execute move.4 csr, (a0)
 * restore a0 by writing saved value of d0 to mailbox and execute move.4 a0, (a0)
 * restore a1 by writing saved value of d0 to mailbox and execute move.4 a1, (a0)
 * execute FORCE_MP 0
 * restore thread PC to saved PC via IPSD_SET_MP_REG_ADDR command
 */
int
ubicom32v3restoreDebuggerThread(unsigned int scratch3value)
{
  struct ubicom32command_packet packet;
  int count, ret;
  unsigned int s3value = (scratch3value & 0xffff) | restore_current_thread_state[5];

  if(ubicom32v3_current_thread == -1)
    return 0;

  ubicom32command_packet_init(&packet);

  /* Restore watchdog .*/
  /* Load a1 with timer base address */
  count = ubicom32command_packet_append(&packet, IBUF_CHECK, ISD_WR_IBUF, restore_watchdog[0]);

  /* Write TKEY_VALUE 0xa1b2c3d4 to mailbox */
  count = ubicom32command_packet_append(&packet, WRITE_CHECK, ISD_WRITE_MAIL_BOX, 0xa1b2c3d4);

  /* Load tkey value from mailbox. */
  count = ubicom32command_packet_append(&packet, IBUF_CHECK, ISD_WR_IBUF, restore_watchdog[1]);

  /* Write timer increment to mailbox 0x800000 */
  count = ubicom32command_packet_append(&packet, WRITE_CHECK, ISD_WRITE_MAIL_BOX, 0x800000);

  /* Move timer increment from mailbox into d0 */
  count = ubicom32command_packet_append(&packet, IBUF_CHECK, ISD_WR_IBUF, restore_watchdog[2]);

  /* timer->wdcom = timer->mptval + increment */
  count = ubicom32command_packet_append(&packet, IBUF_CHECK, ISD_WR_IBUF, restore_watchdog[3]);

  /* timer->tkey = 0 */
  count = ubicom32command_packet_append(&packet, IBUF_CHECK, ISD_WR_IBUF, restore_watchdog[4]);

  /* write the saved  d0 value to the mailbox */
  count = ubicom32command_packet_append(&packet, WRITE_CHECK, ISD_WRITE_MAIL_BOX, current_thread_saved_state.d0);
  count = ubicom32command_packet_append(&packet, IBUF_CHECK, ISD_WR_IBUF, restore_current_thread_state[0]);

  /* write the saved  csr value to the mailbox */
  count = ubicom32command_packet_append(&packet, WRITE_CHECK, ISD_WRITE_MAIL_BOX, current_thread_saved_state.csr);
  count = ubicom32command_packet_append(&packet, IBUF_CHECK, ISD_WR_IBUF, restore_current_thread_state[1]);

  /* write the saved  a1 value to the mailbox */
  count = ubicom32command_packet_append(&packet, WRITE_CHECK, ISD_WRITE_MAIL_BOX, current_thread_saved_state.a1);
  count = ubicom32command_packet_append(&packet, IBUF_CHECK, ISD_WR_IBUF, restore_current_thread_state[2]);

  /* write the saved  a0 value to the mailbox */
  count = ubicom32command_packet_append(&packet, WRITE_CHECK, ISD_WRITE_MAIL_BOX, current_thread_saved_state.a0);
  count = ubicom32command_packet_append(&packet, IBUF_CHECK, ISD_WR_IBUF, restore_current_thread_state[3]);

  /* set scratchpad0 to 1 */
  count = ubicom32command_packet_append(&packet, IBUF_CHECK, ISD_WR_IBUF, restore_current_thread_state[4]);

  /* set scratchpad3 to the provided value */
  count = ubicom32command_packet_append(&packet, IBUF_CHECK, ISD_WR_IBUF, s3value);

  /* force the thread to stop */
  count = ubicom32command_packet_append(&packet, 0, ISD_FORCE_MP, 0);

  /* set the thread PC to the saved thread pc */
  count = ubicom32command_packet_append(&packet, 0, ISD_SET_MP_REG_ADDR, (0xd0 | ubicom32v3currentThread <<10));
  count = ubicom32command_packet_append(&packet, 0, ISD_WR_MP_REG, current_thread_saved_state.pc);

  /* ship down the command */
  ret = ubicom32command_packet_send(&packet);

  ubicom32v3currentThread=ubicom32v3_current_thread=-1;
  return ret;
}

int
ubicom32v3readRegisters( int threadNo,
		   int regIndex,
		   int *dest,
		   unsigned int length)
{
  int ret, count;
  int *regAddrMap = &current_cpu->register_map_table[regIndex];
  struct ubicom32command_packet packet;
  ubicom32command_packet_init(&packet);

  /*
   * Each register read command is made up of 3 Debug Port command a) Execute move.4 scratchpad0, register
   * b) ISD_RD_MP_REG scratchpad0 (this moves the data to host ) c) ISD_NOP to kick the data out. (This because of the RTL bug.)
   */
  while(length)
    {
      unsigned int transferLength = (ubicom32command_packet_get_free(&packet))/3;
      unsigned int i;
      int transferStartIndex;
      struct ubicom32command *command;
      unsigned int move_reg = move_register_to_scratchpad0;
      unsigned int reply;

      if(transferLength > length)
	transferLength = length;

      // Index of the first transfer entry
      transferStartIndex = ubicom32command_packet_get_next(&packet) + 1;

      for(i=0; i< transferLength; i++)
	{
	  /* issue move.4 scratchpad0, register */
	  count = ubicom32command_packet_append(&packet, IBUF_CHECK, ISD_WR_IBUF, (move_reg | ( *regAddrMap++>>2)));

	  /* retrieve scratchpad0 contents */
	  count = ubicom32command_packet_append(&packet, 0, ISD_RD_MP_REG, 0x180);
	  count = ubicom32command_packet_append(&packet, 0, ISD_NOP, 0);
	}

      // Ship the packet for processing
      ret = ubicom32command_packet_send(&packet);
      if(ret)
	return ret;

      // Move the reply data into destination buffer
      command = &packet.commands[transferStartIndex];
      for(i=0; i< transferLength; i++, command+= 3)
	{
	  reply = (command->operand0<<24) | (command->operand1<<16) |
	    (command->operand2<<8) | command->operand3;

	  if(regIndex == 34)
	    *dest++ = current_thread_saved_state.pc;
	  else if(regIndex == 0)
	    *dest++ = current_thread_saved_state.d0;
	  else if(regIndex == 16)
	    *dest++ = current_thread_saved_state.a0;
	  else if(regIndex == 17)
	    *dest++ = current_thread_saved_state.a1;
	  else if(regIndex == 29)
	    *dest++ = current_thread_saved_state.csr;
	  else if(regIndex == 38)
	    *dest++ = current_thread_saved_state.previous_pc;
	  else
	    *dest++ = (int)reply;

	  regIndex++;
	}

      // readjust length etc.
      ubicom32command_packet_init(&packet);
      length = length - transferLength;
    }

  return 0;
}

int
ubicom32v3writeRegisters( int threadNo,
		int regIndex,
		int *inBuf,
		unsigned int length)
{
  int ret, count;
  unsigned int i;
  struct ubicom32command_packet packet;
  int *regAddrMap = &current_cpu->register_map_table[regIndex];
  ubicom32v3Regs_t *mainRegs = (ubicom32v3Regs_t *)current_cpu->registers;
  //int *regAddrMap = &ubicom32v3GdbIndexToRegAddrMap[regIndex];

  if(remote_ubicom32v3_state == UBICOM32V3_CORE_STOPPED)
    {
      int *ptr;
      if(regIndex < 39)
	{
	  // Per thread registers
	  ptr = &mainRegs->tRegs[threadNo].dr[regIndex];
	}
      else
	{
	  // global registers
	  ptr = &mainRegs->globals.chip_id;
	  ptr += (regIndex-39);
	}
      while (length--)
	{
	  *ptr++ = *inBuf++;
	}
      return 0;
    }

  ubicom32command_packet_init(&packet);
  if(regIndex < 39)
    {
      // invalidate the Per Thread entry
      mainRegs->perThreadRead[threadNo]=0;
    }
  else
    {
      // invalidate the global entry
      mainRegs->globalsRead=0;
    }

  /*
   * Each register write command is made up of 2. a) Write mailbox with content of the register
   * b) Execute move.4 register, (a0)
   */

  while(length)
    {
      unsigned int transferLength = ubicom32command_packet_get_free(&packet)/2;
      unsigned int move_reg = move_from_mailbox_to_register;
      unsigned int must_send=0;

      if(transferLength > length)
	transferLength = length;

      for(i=0; i< transferLength; i++)
	{
	  switch(regIndex++)
	    {
	    case 34:
	      {
		/* ThreadPC is getting written */
		current_thread_saved_state.pc = *inBuf++;
		continue;
	      }
	      break;
	    case 0:
	      {
		/* d0 is getting written */
		current_thread_saved_state.d0 = *inBuf++;
		continue;
	      }
	      break;
	    case 16:
	      {
		/* a0 is getting written */
		current_thread_saved_state.a0 = *inBuf++;
		continue;
	      }
	      break;
	    case 17:
	      {
		/* a1 is getting written */
		current_thread_saved_state.a1 = *inBuf++;
		continue;
	      }
	      break;
	    case 29:
	      {
		/* csr is getting written */
		current_thread_saved_state.csr = *inBuf++;
		continue;
	      }
	    case 28:
	      {
		/* previous_pc is getting written */
		current_thread_saved_state.previous_pc = *inBuf++;
		continue;
	      }
	      break;
	    default:
	      {
		must_send = 1;
		count = ubicom32command_packet_append(&packet, WRITE_CHECK, ISD_WRITE_MAIL_BOX,
						  (unsigned int)(*inBuf++));
		count = ubicom32command_packet_append(&packet, IBUF_CHECK, ISD_WR_IBUF, (move_reg | (( *regAddrMap++>>2) << 16)));
	      }
	    }
	}
      // Ship the packet for processing if needed
      if(must_send) {
	ret = ubicom32command_packet_send(&packet);
	if(ret)
	  return ret;
      }

      length = length - transferLength;
      ubicom32command_packet_init(&packet);
    }

  return 0;
}

int
ubicom32v4writeRegisters( int threadNo,
		int regIndex,
		int *inBuf,
		unsigned int length)
{
  int ret, count;
  unsigned int i;
  struct ubicom32command_packet packet;
  int *regAddrMap = &current_cpu->register_map_table[regIndex];
  ubicom32v4Regs_t *mainRegs = (ubicom32v4Regs_t *)current_cpu->registers;
  //int *regAddrMap = &ubicom32v3GdbIndexToRegAddrMap[regIndex];

  if(remote_ubicom32v3_state == UBICOM32V3_CORE_STOPPED)
    {
      int *ptr;
      if(regIndex < 39)
	{
	  // Per thread registers
	  ptr = &mainRegs->tRegs[threadNo].dr[regIndex];
	}
      else
	{
	  // global registers
	  ptr = &mainRegs->globals.chip_id;
	  ptr += (regIndex-39);
	}
      while (length--)
	{
	  *ptr++ = *inBuf++;
	}
      return 0;
    }

  ubicom32command_packet_init(&packet);
  if(regIndex < 39)
    {
      // invalidate the Per Thread entry
      mainRegs->perThreadRead[threadNo]=0;
    }
  else
    {
      // invalidate the global entry
      mainRegs->globalsRead=0;
    }

  /*
   * Each register write command is made up of 2. a) Write mailbox with content of the register
   * b) Execute move.4 register, (a0)
   */

  while(length)
    {
      unsigned int transferLength = ubicom32command_packet_get_free(&packet)/2;
      unsigned int move_reg = move_from_mailbox_to_register;
      unsigned int must_send=0;

      if(transferLength > length)
	transferLength = length;

      for(i=0; i< transferLength; i++)
	{
	  switch(regIndex++)
	    {
	    case 34:
	      {
		/* ThreadPC is getting written */
		current_thread_saved_state.pc = *inBuf++;
		continue;
	      }
	      break;
	    case 0:
	      {
		/* d0 is getting written */
		current_thread_saved_state.d0 = *inBuf++;
		continue;
	      }
	      break;
	    case 16:
	      {
		/* a0 is getting written */
		current_thread_saved_state.a0 = *inBuf++;
		continue;
	      }
	      break;
	    case 17:
	      {
		/* a1 is getting written */
		current_thread_saved_state.a1 = *inBuf++;
		continue;
	      }
	      break;
	    case 29:
	      {
		/* csr is getting written */
		current_thread_saved_state.csr = *inBuf++;
		continue;
	      }
	    case 28:
	      {
		/* previous_pc is getting written */
		current_thread_saved_state.previous_pc = *inBuf++;
		continue;
	      }
	      break;
	    default:
	      {
		must_send = 1;
		count = ubicom32command_packet_append(&packet, WRITE_CHECK, ISD_WRITE_MAIL_BOX,
						  (unsigned int)(*inBuf++));
		count = ubicom32command_packet_append(&packet, IBUF_CHECK, ISD_WR_IBUF, (move_reg | (( *regAddrMap++>>2) << 16)));
	      }
	    }
	}
      // Ship the packet for processing if needed
      if(must_send) {
	ret = ubicom32command_packet_send(&packet);
	if(ret)
	  return ret;
      }

      length = length - transferLength;
      ubicom32command_packet_init(&packet);
    }

  return 0;
}

int ubicom32v3gdbGetallRegs(struct ubicom32_cpu *cpu, int threadNo)
{
  int ret;
  ubicom32v3Regs_t *mainRegs = (ubicom32v3Regs_t *)cpu->registers;
  int *dptr = &mainRegs->tRegs[threadNo].dr[0];

  if(mainRegs->perThreadRead[threadNo] ==0)
    {
      /* get all per thread debugger registers for the given thread */
      mainRegs->perThreadRead[threadNo]=1;

      /* read the data register block for the given thread */
      ret = ubicom32v3readRegisters(threadNo, 0, dptr, 39);
      if(ret)
	return (ret);

    }

  if(mainRegs->globalsRead == 0)
    {
      struct direct_read_registers *direct_regs = &direct_read_regs;

      mainRegs->globalsRead = 1;

      /* Start on the globals. Begin athe chip_id */
      dptr = &mainRegs->globals.chip_id;
      *dptr++ = direct_regs->chip_id;
      *dptr++ = direct_regs->int_stat0;
      *dptr++ = direct_regs->int_stat1;

      /* read global control */
      dptr = &mainRegs->globals.global_ctrl;
      ret = ubicom32v3readRegisters(1, 46, dptr, 2);
      if(ret)
	return ret;

      mt_active = direct_regs->mt_active = mainRegs->globals.mt_active;

      /* read mt_dbg_active */
      mainRegs->globals.mt_dbg_active = direct_regs->mt_dbg_active;

      mainRegs->globals.mt_en = direct_regs->mt_en;

      /* read mt_hpri etc */
      dptr = &mainRegs->globals.mt_hpri;
      ret = ubicom32v3readRegisters(1, 53, dptr, 2);
      if(ret)
	return ret;
      mt_pri = mainRegs->globals.mt_hpri;
      mt_sched = mainRegs->globals.mt_hrt;

      mainRegs->globals.mt_break = direct_regs->mt_break;

      dptr = &mainRegs->globals.mt_single_step;
      ret = ubicom32v3readRegisters(1, 57, dptr, 2);
      if(ret)
	return ret;

      dptr = &mainRegs->globals.dcapt;
      ret = ubicom32v3readRegisters(1, 60, dptr, 1);
      if(ret)
	return ret;

      mainRegs->globals.scratchpad0 = direct_regs->scratchpad0;
      mainRegs->globals.scratchpad1 = direct_regs->scratchpad1;
      mainRegs->globals.scratchpad2 = direct_regs->scratchpad2;
      mainRegs->globals.scratchpad3 = direct_regs->scratchpad3;

      dptr = &mainRegs->globals.chip_cfg;
      ret = ubicom32v3readRegisters(1, 66, dptr, 1);
      if(ret)
	return ret;

      mainRegs->globals.mt_i_blocked = direct_regs->mt_i_blocked;
      mainRegs->globals.mt_d_blocked = direct_regs->mt_d_blocked;

      dptr = &mainRegs->globals.mt_trap_en;
      ret = ubicom32v3readRegisters(1, 72, dptr, 1);
      if(ret)
	return ret;

      mainRegs->globals.mt_en = direct_regs->mt_en;

      dptr = &mainRegs->globals.i_range0_hi;
      ret = ubicom32v3readRegisters(1, 76, dptr, 21);
      if(ret)
	return ret;
    }

  return 0;
}

int
ubicom32v4gdbGetallRegs(struct ubicom32_cpu *cpu, int threadNo)
{
  int ret;
  ubicom32v4Regs_t *mainRegs = (ubicom32v4Regs_t *)cpu->registers;
  int *dptr = &mainRegs->tRegs[threadNo].dr[0];

  if(mainRegs->perThreadRead[threadNo] ==0)
    {
      /* get all per thread debugger registers for the given thread */
      mainRegs->perThreadRead[threadNo]=1;

      /* read the data register block for the given thread */
      ret = ubicom32v3readRegisters(threadNo, 0, dptr, 39);
      if(ret)
	return (ret);

    }

  if(mainRegs->globalsRead == 0)
    {
      struct direct_read_registers *direct_regs = &direct_read_regs;

      mainRegs->globalsRead = 1;

      /* Start on the globals. Begin athe chip_id */
      dptr = &mainRegs->globals.chip_id;
      *dptr++ = direct_regs->chip_id;
      *dptr++ = direct_regs->int_stat0;
      *dptr++ = direct_regs->int_stat1;

      /* read global control */
      dptr = &mainRegs->globals.global_ctrl;
      ret = ubicom32v3readRegisters(1, 46, dptr, 2);
      if(ret)
	return ret;

      mt_active = direct_regs->mt_active = mainRegs->globals.mt_active;

      /* read mt_dbg_active */
      mainRegs->globals.mt_dbg_active = direct_regs->mt_dbg_active;

      mainRegs->globals.mt_en = direct_regs->mt_en;

      /* read mt_hpri etc */
      dptr = &mainRegs->globals.mt_hpri;
      ret = ubicom32v3readRegisters(1, 53, dptr, 2);
      if(ret)
	return ret;
      mt_pri = mainRegs->globals.mt_hpri;
      mt_sched = mainRegs->globals.mt_hrt;

      mainRegs->globals.mt_break = direct_regs->mt_break;

      dptr = &mainRegs->globals.mt_single_step;
      ret = ubicom32v3readRegisters(1, 57, dptr, 2);
      if(ret)
	return ret;

      dptr = &mainRegs->globals.dcapt;
      ret = ubicom32v3readRegisters(1, 60, dptr, 1);
      if(ret)
	return ret;

      mainRegs->globals.scratchpad0 = direct_regs->scratchpad0;
      mainRegs->globals.scratchpad1 = direct_regs->scratchpad1;
      mainRegs->globals.scratchpad2 = direct_regs->scratchpad2;
      mainRegs->globals.scratchpad3 = direct_regs->scratchpad3;

      dptr = &mainRegs->globals.chip_cfg;
      ret = ubicom32v3readRegisters(1, 66, dptr, 1);
      if(ret)
	return ret;

      mainRegs->globals.mt_i_blocked = direct_regs->mt_i_blocked;
      mainRegs->globals.mt_d_blocked = direct_regs->mt_d_blocked;

      dptr = &mainRegs->globals.mt_trap_en;
      ret = ubicom32v3readRegisters(1, 72, dptr, 1);
      if(ret)
	return ret;

      mainRegs->globals.mt_en = direct_regs->mt_en;

      dptr = &mainRegs->globals.i_range0_hi;
      ret = ubicom32v3readRegisters(1, 76, dptr, 27);
      if(ret)
	return ret;
    }

  return 0;
}

int
ubicom32v3gdbGetTPacketRegs(struct ubicom32_cpu *cpu, int threadNo)
{
  //extern unsigned int  mt_active, mt_dbg_active, mt_en, mt_sched, mt_pri;
  int ret;
  ubicom32v3Regs_t *mainRegs = (ubicom32v3Regs_t *)cpu->registers;
  int *dptr;
  /* get d0 register for the given thread */

  /* read the data register block for the given thread */
  mainRegs->tRegs[threadNo].dr[0] = current_thread_saved_state.d0;

  /* read the address a5-a7 register block */
  dptr = &mainRegs->tRegs[threadNo].ar[5];
  ret = ubicom32v3readRegisters(threadNo, 21, dptr, 3);
  if(ret)
    return(ret);

  /* read the thread rosr */
  dptr = &mainRegs->tRegs[threadNo].rosr;
  ret = ubicom32v3readRegisters(threadNo, 30, dptr, 1);
  if(ret)
    return ret;

  /* read the thread pc */
  mainRegs->tRegs[threadNo].threadPc = current_thread_saved_state.pc;

  /* read mt_active */
  dptr = &mainRegs->globals.mt_active;
  ret = ubicom32v3readRegisters(threadNo, 47, dptr, 1);
  if(ret)
    return ret;

  /* read mt_dbg_active */
  dptr = &mainRegs->globals.mt_dbg_active;
  ret = ubicom32v3readRegisters(threadNo, 51, dptr, 1);
  if(ret)
    return ret;

  /* read mt_en etc */
  dptr = &mainRegs->globals.mt_en;
  ret = ubicom32v3readRegisters(threadNo, 52, dptr, 3);
  if(ret)
    return ret;

  // Update some of our internal copies

  mt_active = (int) ubicom32v3Registers.globals.mt_active;
  mt_dbg_active = (int) ubicom32v3Registers.globals.mt_dbg_active;
  mt_en = (int) ubicom32v3Registers.globals.mt_en;
  mt_pri = (int) ubicom32v3Registers.globals.mt_hpri;
  mt_sched = (int) ubicom32v3Registers.globals.mt_hrt;

  return 0;
}

int
ubicom32v4gdbGetTPacketRegs(struct ubicom32_cpu *cpu, int threadNo)
{
  //extern unsigned int  mt_active, mt_dbg_active, mt_en, mt_sched, mt_pri;
  int ret;
  ubicom32v4Regs_t *mainRegs = (ubicom32v4Regs_t *)cpu->registers;
  int *dptr;
  /* get d0 register for the given thread */

  /* read the data register block for the given thread */
  mainRegs->tRegs[threadNo].dr[0] = current_thread_saved_state.d0;

  /* read the address a5-a7 register block */
  dptr = &mainRegs->tRegs[threadNo].ar[5];
  ret = ubicom32v3readRegisters(threadNo, 21, dptr, 3);
  if(ret)
    return(ret);

  /* read the thread rosr */
  dptr = &mainRegs->tRegs[threadNo].rosr;
  ret = ubicom32v3readRegisters(threadNo, 30, dptr, 1);
  if(ret)
    return ret;

  /* read the thread pc */
  mainRegs->tRegs[threadNo].threadPc = current_thread_saved_state.pc;

  /* read mt_active */
  dptr = &mainRegs->globals.mt_active;
  ret = ubicom32v3readRegisters(threadNo, 47, dptr, 1);
  if(ret)
    return ret;

  /* read mt_dbg_active */
  dptr = &mainRegs->globals.mt_dbg_active;
  ret = ubicom32v3readRegisters(threadNo, 51, dptr, 1);
  if(ret)
    return ret;

  /* read mt_en etc */
  dptr = &mainRegs->globals.mt_en;
  ret = ubicom32v3readRegisters(threadNo, 52, dptr, 3);
  if(ret)
    return ret;

  // Update some of our internal copies

  mt_active = (int) mainRegs->globals.mt_active;
  mt_dbg_active = (int) mainRegs->globals.mt_dbg_active;
  mt_en = (int) mainRegs->globals.mt_en;
  mt_pri = (int) mainRegs->globals.mt_hpri;
  mt_sched = (int) mainRegs->globals.mt_hrt;

  return 0;
}

int
ubicom32v3readMemory(unsigned int destAddr, unsigned int length, int *data)
{
  int ret, count;
  struct ubicom32command_packet packet;
  ubicom32command_packet_init(&packet);

  /* setup a1 with the destination address */
  count = ubicom32command_packet_append(&packet, WRITE_CHECK, ISD_WRITE_MAIL_BOX, (unsigned int)(destAddr));
  count = ubicom32command_packet_append(&packet, 0, ISD_RD_IPORT_STAT, 0);
  count = ubicom32command_packet_append(&packet, 0, ISD_RD_IPORT_STAT, 0);
  count = ubicom32command_packet_append(&packet, IBUF_CHECK, ISD_WR_IBUF, move_from_mailbox_to_a1);
  count = ubicom32command_packet_append(&packet, 0, ISD_NOP, move_from_mailbox_to_a1);

  // Ship the packet for processing
  ret = ubicom32command_packet_send(&packet);
  if(ret)
    return ret;


  /*
   * Each memory read command is made up of 2 Debug Port commands a) Execute move.4 scratchpad0, (a1)4++
   * b) ISD_RD_MP_REG scratchpad0 (this moves the data to host) c) ISD_NOP (rtl bug).
   */
  ubicom32command_packet_init(&packet);
  while(length)
    {
      unsigned int transferLength = (ubicom32command_packet_get_free(&packet));
      unsigned int i;
      int transferStartIndex;
      struct ubicom32command *command;
      unsigned int reply;

      //transferLength -= ubicom32command_packet_get_next(&packet);
      transferLength /= 3;
      // Index of the first transfer entry
      transferStartIndex = ubicom32command_packet_get_next(&packet) + 1;

      if(transferLength > length)
	transferLength = length;

      for(i=0; i< transferLength; i++)
	{
	  /* issue move.4 scratchpad0), (a1)4++ */
	  count = ubicom32command_packet_append(&packet, IBUF_CHECK, ISD_WR_IBUF, move_memory_to_scratchpad0);

	  /* retrieve scratchpad0 contents */
	  count = ubicom32command_packet_append(&packet, 0, ISD_RD_MP_REG, 0x180);

	  count = ubicom32command_packet_append(&packet, 0, ISD_NOP, 0);
	}

      // Ship the packet for processing
      ret = ubicom32command_packet_send(&packet);
      if(ret)
	return ret;

      // Move the reply data into destination buffer
      command = &packet.commands[transferStartIndex];
      for(i=0; i< transferLength; i++, command+= 3)
	{
	  reply = (command->operand0<<24) | (command->operand1<<16) |
	    (command->operand2<<8) | command->operand3;

	  *data++ = (int)reply;
	}

      // readjust length etc.
      ubicom32command_packet_init(&packet);
      length = length - transferLength;
      count = 0;
    }
  return 0;
}

int
ubicom32v3writeMemory(unsigned int destAddr, unsigned int length, int *data)
{
  int ret, count;
  unsigned int i;
  struct ubicom32command_packet packet;

  ubicom32command_packet_init(&packet);

  /* Set up a1 with destination address */
  count = ubicom32command_packet_append(&packet, WRITE_CHECK, ISD_WRITE_MAIL_BOX, (unsigned int)(destAddr));
  //count = ubicom32command_packet_append(&packet, 0, ISD_RD_IPORT_STAT, 0);
  count = ubicom32command_packet_append(&packet, IBUF_CHECK, ISD_WR_IBUF, move_from_mailbox_to_a1);

  /*
   * Each memory write command is made up of 2. a) Write mailbox with content of the memory location
   * b) Execute move.4 (a1)4++, (a0)
   */
  while(length)
    {
      unsigned int transferLength = ubicom32command_packet_get_free(&packet)/2;

      if(transferLength > length)
	transferLength = length;

      for(i=0; i< transferLength; i++)
	{
	  count = ubicom32command_packet_append(&packet, WRITE_CHECK, ISD_WRITE_MAIL_BOX, (unsigned int)(*data++));
	  count = ubicom32command_packet_append(&packet, IBUF_CHECK, ISD_WR_IBUF, move_from_mailbox_to_memory);
	}

      // Ship the packet for processing
      ret = ubicom32command_packet_send(&packet);
      if(ret)
	return ret;

      length = length - transferLength;
      ubicom32command_packet_init(&packet);
    }

  return 0;
}

/* request to start the cpu with the PC at the given address for fast transfers */
int
ubicom32v3isp_force_run_cpu(unsigned int address)
{
  int ret, count;
  struct ubicom32command_packet packet;

  ubicom32command_packet_init(&packet);

  /* force the thread to stop */
  count = ubicom32command_packet_append(&packet, 0, ISD_FORCE_MP, 0);

  /* set the thread PC to the given address */
  count = ubicom32command_packet_append(&packet, 0, ISD_SET_MP_REG_ADDR, (0xd0 | ubicom32v3currentThread <<10));
  count = ubicom32command_packet_append(&packet, 0, ISD_WR_MP_REG, address);

  /* force the thread to run */
  count = ubicom32command_packet_append(&packet, 0, ISD_FORCE_MP, 1 << ubicom32v3currentThread);
  count = ubicom32command_packet_append(&packet, 0, ISD_RD_IPORT_STAT, 0);
  count = ubicom32command_packet_append(&packet, 0, ISD_RD_IPORT_STAT, 0);

  /* ship down the command */
  ret = ubicom32command_packet_send(&packet);

  return ret;
}

extern struct ubicom32command_packet *pending_packet;
extern unsigned int pending_packet_length;
extern unsigned int pending_packet_opcode;
extern unsigned int doubleBuffer;

int
ubicom32v3UltraFastReadMemoryRaw(unsigned int destAddr, unsigned int length, int *data)
{
  int ret, count;
  //struct ubicom32command_packet packet;
  struct ubicom32command_packet packets[2];
  struct ubicom32command_packet *packet;
  int pending_transferStartIndex = 0;
  int pending_transferLength = 0;
  unsigned char *cptr;

  packet = packets;
  ubicom32command_packet_init(packet);

  /* Set up a1 with destination address */
  count = ubicom32command_packet_append(packet, WRITE_CHECK, ISD_WRITE_MAIL_BOX, (unsigned int)(destAddr));
  //count = ubicom32command_packet_append(packet, 0, ISD_RD_IPORT_STAT, 0);
  count = ubicom32command_packet_append(packet, IBUF_CHECK, ISD_WR_IBUF, move_from_mailbox_to_a1);

  /* set up d0 with the length */
  count = ubicom32command_packet_append(packet, WRITE_CHECK, ISD_WRITE_MAIL_BOX, (unsigned int)(length));
  count = ubicom32command_packet_append(packet, IBUF_CHECK, ISD_WR_IBUF, move_from_mailbox_to_register);

  // Ship the packet for processing
  ret = ubicom32command_packet_send(packet);
  if(ret)
    return ret;

  /* Get the cpu executing the fast code for write */
  ubicom32v3isp_force_run_cpu(fast_read_code_address);

  /* Go into double buffered mode */
  pending_packet = NULL;
  doubleBuffer = 1;

  while(length)
    {
      unsigned int transferLength = 256;

      if (chipID == 0x20000)
	{
	  /* Mars 1.2: Need to use the work around of shoving NOPS */
	  ubicom32command_packet_init_opcode(packet, ISPISD_OPCODE_UBICOM32COMMAND_READ_MAILBOX_UBICOM32V3);
	}
      else
	{
	  ubicom32command_packet_init_opcode(packet, ISPISD_OPCODE_UBICOM32COMMAND_READ_MAILBOX);
	}

      if(transferLength > length)
	transferLength = length;

      // load length into operand field
      packet->header.operand = transferLength;
      packet->header.length = 16;

      // Ship the packet for processing
      ret = ubicom32command_packet_send(packet);
      if(ret)
	{
	  doubleBuffer = 0;
	  pending_packet = NULL;
	  return ret;
	}

      if(pending_packet)
	{
	  // Move the reply data into destination buffer
	  cptr = (unsigned char *)&pending_packet->commands[pending_transferStartIndex];

	  if (pending_transferLength)
	    {
	      (void) memcpy(data,cptr, pending_transferLength*4);
	      data += pending_transferLength;
	    }
	}

      // Setup the pending packet
      pending_packet = packet;
      pending_packet_length = (ntohl (pending_packet->header.length) + 4*transferLength);
      pending_packet_opcode = ntohl (pending_packet->header.opcode);
      pending_transferLength = transferLength;

      if(packet == packets)
	packet = &packets[1];
      else
	packet = packets;

      // readjust length etc.
      length = length - transferLength;
    }

  if(pending_packet)
    {
      retrieve_pending_packet();

      // Move the reply data into destination buffer
      cptr = (unsigned char *)&pending_packet->commands[pending_transferStartIndex];

      (void) memcpy(data,cptr, pending_transferLength*4);
      data += pending_transferLength;
    }

  pending_packet = NULL;
  doubleBuffer = 0;

  /* Force the thread to debug space */
  ret = ubicom32v3isp_force_run_cpu(0x01000000);

  return 0;
}

int
ubicom32v3UltrafastWriteMemory(unsigned int destAddr, unsigned int length, int *data)
{
  int ret, count;
  unsigned int i;
  struct ubicom32command_packet packets[2], *packet;

  packet = packets;
  pending_packet = NULL;

  ubicom32command_packet_init(packet);

  /* Set up a1 with destination address */
  count = ubicom32command_packet_append(packet, WRITE_CHECK, ISD_WRITE_MAIL_BOX, (unsigned int)(destAddr));
  //count = ubicom32command_packet_append(&packet, 0, ISD_RD_IPORT_STAT, 0);
  count = ubicom32command_packet_append(packet, IBUF_CHECK, ISD_WR_IBUF, move_from_mailbox_to_a1);

  /* set up d0 with the length */
  count = ubicom32command_packet_append(packet, WRITE_CHECK, ISD_WRITE_MAIL_BOX, (unsigned int)(length));
  count = ubicom32command_packet_append(packet, IBUF_CHECK, ISD_WR_IBUF, move_from_mailbox_to_register);

  // Ship the packet for processing
  ret = ubicom32command_packet_send(packet);
  if(ret)
    return ret;

  /* Get the cpu executing the fast code for write */
  ubicom32v3isp_force_run_cpu(fast_write_code_address);

  /* Go into double buffered mode */
  pending_packet = NULL;
  doubleBuffer = 1;

  /*
   * Each memory write command is Write mailbox with content of the memory location
   */
  while(length)
    {
      unsigned int transferLength = 256;
      int *dest;

      // initialize packet for writing mailbox
      ubicom32command_packet_init_opcode(packet, ISPISD_OPCODE_UBICOM32COMMAND_WRITE_MAILBOX);

      if(transferLength > length)
	transferLength = length;

      // load length into operand field
      packet->header.operand = transferLength;
      packet->header.length = 16 + transferLength * 4;

      // adjust length
      length -= transferLength;

      /* copy the data into the command data space */
      dest = (unsigned int *)&packet->commands[0];
      for(i=0; i< transferLength; i++, dest++, data++)
	{
	  unsigned char *s, *d;
	  d = (unsigned char *)dest;
	  s = (unsigned char *)data;

	  d[3] = s[0];
	  d[2] = s[1];
	  d[1] = s[2];
	  d[0] = s[3];
	}
      //memcpy(dest, (void *) data, transferLength * 4);

      // Ship the packet for processing
      ret = ubicom32command_packet_send(packet);
      if(ret)
	{
	  doubleBuffer = 0;
	  pending_packet = NULL;
	  return ret;
	}

      // Setup the pending packet
      pending_packet = packet;
      pending_packet_length = 16;
      pending_packet_opcode = ntohl (pending_packet->header.opcode);

      if(packet == packets)
	packet = &packets[1];
      else
	packet = packets;
    }

  if(pending_packet)
    {
      retrieve_pending_packet();
    }

  pending_packet = NULL;
  doubleBuffer = 0;

  /* Force the thread to debug space */
  ubicom32v3isp_force_run_cpu(0x01000000);
  return 0;
}

/*
 *ubicom32v3chacheflushinvalidate
 * This routine will flush the Dcache by address and invalidate the Icache for the same address.
 * Length is specified in number of bytes.
 */
int
ubicom32v3cacheflushinvalidate(unsigned int startAddr, unsigned int length)
{
  int ret, count;
  unsigned int i;
  struct ubicom32command_packet packet;
  unsigned int endAddr = startAddr+length;
  unsigned int numcachelines = (endAddr & 0x1f) ? 1: 0;

  /* backoff start address and end addresses to the cacheline boundary. */
  startAddr &= ~0x1f;
  endAddr &= ~0x1f;

  /* compute the number of cache entries that need to be dealt with */
  numcachelines += (endAddr - startAddr)/32;

  ubicom32command_packet_init(&packet);

  /* save a2 to scratchpad0 */
  count = ubicom32command_packet_append(&packet, IBUF_CHECK, ISD_WR_IBUF, move_a2_to_scratchpad0);

  /* load a1 with dcache base address */
  count = ubicom32command_packet_append(&packet, IBUF_CHECK, ISD_WR_IBUF, move_dcache_base_to_a1);

  /* load a2 with icache base address */
  count = ubicom32command_packet_append(&packet, IBUF_CHECK, ISD_WR_IBUF, move_icache_base_to_a2);

  /* load 0x90 (Flush dcache by address operation) to dcache control register via mailbox */
  count = ubicom32command_packet_append(&packet, WRITE_CHECK, ISD_WRITE_MAIL_BOX, 0x90);
  count = ubicom32command_packet_append(&packet, IBUF_CHECK, ISD_WR_IBUF, load_dcache_control_reg);

  /* load 0x60 (Invalidate icache by address operation) to icache control register via mailbox */
  count = ubicom32command_packet_append(&packet, WRITE_CHECK, ISD_WRITE_MAIL_BOX, 0x60);
  count = ubicom32command_packet_append(&packet, IBUF_CHECK, ISD_WR_IBUF, load_icache_control_reg);

  /*
   * Flushing dcache takes 3 operations a) Load address into mailbox b) move.4 (a1), (a0) c) bset 16(a1), 16(a1), #0x3
   * Invalidating icache takes 2 operations a) move.4 (a2), (a0) c) bset 16(a2), 16(a2), #0x3
   * That is a total of 5 command slots
   */
  while(numcachelines)
    {
      unsigned int transferLength = ubicom32command_packet_get_free(&packet)/6;

      if(transferLength > numcachelines)
	transferLength = numcachelines;

      for(i=0; i< transferLength; i++)
	{
	  /* write cacheline start address to mailbox */
	  count = ubicom32command_packet_append(&packet, WRITE_CHECK, ISD_WRITE_MAIL_BOX, startAddr);

	  /* load address from mailbox to dcache addr register and activate the flush.*/
	  count = ubicom32command_packet_append(&packet, IBUF_CHECK, ISD_WR_IBUF, load_dcache_addr_and_activate[0]);
	  count = ubicom32command_packet_append(&packet, IBUF_CHECK, ISD_WR_IBUF, load_dcache_addr_and_activate[1]);

	  /* write cacheline start address to mailbox */
	  count = ubicom32command_packet_append(&packet, WRITE_CHECK, ISD_WRITE_MAIL_BOX, startAddr);

	  /* load address from mailbox to icache addr register and activate the invalidation */
	  count = ubicom32command_packet_append(&packet, IBUF_CHECK, ISD_WR_IBUF, load_icache_addr_and_activate[0]);
	  count = ubicom32command_packet_append(&packet, IBUF_CHECK, ISD_WR_IBUF, load_icache_addr_and_activate[1]);

	  startAddr += 32;
	}

      numcachelines -= transferLength;

      // Ship the packet for processing
      ret = ubicom32command_packet_send(&packet);
      if(ret)
	return ret;

      ubicom32command_packet_init(&packet);
    }

  /* restore a2 from scratchpad0 */
  count = ubicom32command_packet_append(&packet, IBUF_CHECK, ISD_WR_IBUF, move_scratchpad0_to_a2);
  return(ubicom32command_packet_send(&packet));
}

/*
 *ubicom32v3chacheinvalidate
 * This routine will invalidate the Dcache by address.
 * Length is specified in number of bytes.
 */
int
ubicom32v3cacheinvalidate(unsigned int startAddr, unsigned int length)
{
  int ret, count;
  unsigned int i;
  struct ubicom32command_packet packet;
  unsigned int endAddr = startAddr+length;
  unsigned int numcachelines = (endAddr & 0x1f) ? 1: 0;

  /* backoff start address and end addresses to the cacheline boundary. */
  startAddr &= ~0x1f;
  endAddr &= ~0x1f;

  /* compute the number of cache entries that need to be dealt with */
  numcachelines += (endAddr - startAddr)/32;

  ubicom32command_packet_init(&packet);

  /* load a1 with dcache base address */
  count = ubicom32command_packet_append(&packet, IBUF_CHECK, ISD_WR_IBUF, move_dcache_base_to_a1);

  /* load 0x60 (Invalidate dcache by address operation) to dcache control register via mailbox */
  count = ubicom32command_packet_append(&packet, WRITE_CHECK, ISD_WRITE_MAIL_BOX, 0xA0);
  count = ubicom32command_packet_append(&packet, IBUF_CHECK, ISD_WR_IBUF, load_dcache_control_reg);

  ret = 0;
  /*
   * Invalidating dcache takes 3 operations a) Load address into mailbox b) move.4 (a1), (a0) c) bset 16(a1), 16(a1), #0x3
   * That is a total of 3 command slots
   */
  while(numcachelines)
    {
      unsigned int transferLength = ubicom32command_packet_get_free(&packet)/3;

      if(transferLength > numcachelines)
	transferLength = numcachelines;

      for(i=0; i< transferLength; i++)
	{
	  /* write cacheline start address to mailbox */
	  count = ubicom32command_packet_append(&packet, WRITE_CHECK, ISD_WRITE_MAIL_BOX, startAddr);

	  /* load address from mailbox to dcache addr register and activate the flush.*/
	  count = ubicom32command_packet_append(&packet, IBUF_CHECK, ISD_WR_IBUF, load_dcache_addr_and_activate[0]);
	  count = ubicom32command_packet_append(&packet, IBUF_CHECK, ISD_WR_IBUF, load_dcache_addr_and_activate[1]);

	  startAddr += 32;
	}

      numcachelines -= transferLength;

      // Ship the packet for processing
      ret = ubicom32command_packet_send(&packet);
      if(ret)
	return ret;

      ubicom32command_packet_init(&packet);
    }

  return(ret);
}

/*
 *ubicom32v3readchachetags
 * This routine will read cache TAG entries
 * numcachelines is the number of Tags to read.
 * startAddr is the address of the first TAG to read (lower 5 bits will be forced to zero).
 * cache  0=dcache 1=icache
 * buffer is the destination data buffer.
 */
int
ubicom32v3readcachetags(unsigned int startAddr, unsigned int numcachelines, unsigned int cache, unsigned char *buffer)
{
  int ret, count;
  unsigned int i;
  struct ubicom32command_packet packet;

  /* backoff start address to the cacheline boundary. */
  startAddr &= ~0x1f;

  ubicom32command_packet_init(&packet);

  /* load a1 with cache base address */
  if (cache)
    {
      /* Load with icache base address */
      count = ubicom32command_packet_append(&packet, IBUF_CHECK, ISD_WR_IBUF, move_icache_base_to_a1);
    }
  else
    {
      /* load with dcache base address */
      count = ubicom32command_packet_append(&packet, IBUF_CHECK, ISD_WR_IBUF, move_dcache_base_to_a1);
    }

  /* load 0x20 (Direct Read TAG) to dcache control register via mailbox */
  count = ubicom32command_packet_append(&packet, WRITE_CHECK, ISD_WRITE_MAIL_BOX, 0x20);
  count = ubicom32command_packet_append(&packet, IBUF_CHECK, ISD_WR_IBUF, load_cache_control_reg);

  ret = 0;
  /*
   * Reading a cache TAG takes 5 operations a) Load address into mailbox b) move.4 (a1), (a0) c) bset 16(a1), 16(a1), #0x3
   * d) move.4 4(a1), 4(a0) (move CRDD to mailbox), e) read mailbox.
   * That is a total of 5 command slots
   */
  while(numcachelines)
    {
      unsigned int transferLength = ubicom32command_packet_get_free(&packet)/5;
      unsigned read_tag_entry = 0;
      struct ubicom32command *command;
      unsigned int reply;

      if(transferLength > numcachelines)
	transferLength = numcachelines;


      for(i=0; i< transferLength; i++)
	{
	  /* write cacheline start address to mailbox */
	  count = ubicom32command_packet_append(&packet, WRITE_CHECK, ISD_WRITE_MAIL_BOX, startAddr);

	  /* load address from mailbox to dcache addr register and activate the flush.*/
	  count = ubicom32command_packet_append(&packet, IBUF_CHECK, ISD_WR_IBUF, load_cache_addr_and_activate[0]);
	  count = ubicom32command_packet_append(&packet, IBUF_CHECK, ISD_WR_IBUF, load_cache_addr_and_activate[1]);

	  /* Load instruction to move data from Dcache Dreg to mailbox */
	  count = ubicom32command_packet_append(&packet, IBUF_CHECK, ISD_WR_IBUF, move_from_cache_dreg_to_mailbox);

	  /* get the data from mailbox. */
	  count = ubicom32command_packet_append(&packet, READ_CHECK, ISD_READ_MAIL_BOX, 0);

	  if (read_tag_entry == 0)
	    read_tag_entry = count;

	  startAddr += 32;
	}

      numcachelines -= transferLength;

      // Ship the packet for processing
      ret = ubicom32command_packet_send(&packet);
      if(ret)
	return ret;

      /* data is back. Pull out the tag entries. */
      command = &packet.commands[read_tag_entry];
      for(i=0; i< transferLength; i++)
	{
	  reply = (command->operand0<<24) | (command->operand1<<16) |
	    (command->operand2<<8) | command->operand3;

	  /* copy the reply to buffer */
	  memcpy(buffer, &reply, 4);

	  /* bump up the buffer. */
	  buffer += 4;

	  /* bump up to the next entry. */
	  command += 5;
	}

      ubicom32command_packet_init(&packet);
    }

  return(ret);
}

/*
 *ubicom32v3readchachedata
 * This routine will read cache Data entries
 * numdata is the number of cache bytes to read.
 * startAddr is the address of the first byte to read (lower 2 bits will be forced to zero).
 * cache = 0 for dcache = 1 for icache
 * buffer is the destination data buffer.
 */
int
ubicom32v3readcachedata(unsigned int startAddr, unsigned int numdata, unsigned int cache, unsigned char *buffer)
{
  int ret, count;
  unsigned int i;
  struct ubicom32command_packet packet;
  unsigned int numtransfers = numdata/4;

  /* backoff start address to word boundary. */
  startAddr &= ~0x3;

  ubicom32command_packet_init(&packet);

  /* load a1 with cache base address */
  if (cache)
    {
      /* Load with icache base address */
      count = ubicom32command_packet_append(&packet, IBUF_CHECK, ISD_WR_IBUF, move_icache_base_to_a1);
    }
  else
    {
      /* load with dcache base address */
      count = ubicom32command_packet_append(&packet, IBUF_CHECK, ISD_WR_IBUF, move_dcache_base_to_a1);
    }

  /* load 0x10 (Direct Read Data) to cache control register via mailbox */
  count = ubicom32command_packet_append(&packet, WRITE_CHECK, ISD_WRITE_MAIL_BOX, 0x10);
  count = ubicom32command_packet_append(&packet, IBUF_CHECK, ISD_WR_IBUF, load_cache_control_reg);

  ret = 0;
  /*
   * Reading a cache Data takes 5 operations a) Load address into mailbox b) move.4 (a1), (a0) c) bset 16(a1), 16(a1), #0x3
   * d) move.4 4(a1), 4(a0) (move CRDD to mailbox), e) read mailbox.
   * That is a total of 5 command slots
   */
  while(numtransfers)
    {
      unsigned int transferLength = ubicom32command_packet_get_free(&packet)/5;
      unsigned read_tag_entry = 0;
      struct ubicom32command *command;
      unsigned int reply;

      if(transferLength > numtransfers)
	transferLength = numtransfers;


      for(i=0; i< transferLength; i++)
	{
	  /* write cacheline start address to mailbox */
	  count = ubicom32command_packet_append(&packet, WRITE_CHECK, ISD_WRITE_MAIL_BOX, startAddr);

	  /* load address from mailbox to dcache addr register and activate the flush.*/
	  count = ubicom32command_packet_append(&packet, IBUF_CHECK, ISD_WR_IBUF, load_cache_addr_and_activate[0]);
	  count = ubicom32command_packet_append(&packet, IBUF_CHECK, ISD_WR_IBUF, load_cache_addr_and_activate[1]);

	  /* Load instruction to move data from Dcache Dreg to mailbox */
	  count = ubicom32command_packet_append(&packet, IBUF_CHECK, ISD_WR_IBUF, move_from_cache_dreg_to_mailbox);

	  /* get the data from mailbox. */
	  count = ubicom32command_packet_append(&packet, READ_CHECK, ISD_READ_MAIL_BOX, 0);

	  if (read_tag_entry == 0)
	    read_tag_entry = count;

	  startAddr += 4;
	}

      numtransfers -= transferLength;

      // Ship the packet for processing
      ret = ubicom32command_packet_send(&packet);
      if(ret)
	return ret;

      /* data is back. Pull out the tag entries. */
      command = &packet.commands[read_tag_entry];
      for(i=0; i< transferLength; i++)
	{
	  reply = (command->operand0<<24) | (command->operand1<<16) |
	    (command->operand2<<8) | command->operand3;

	  /* copy the reply to buffer */
	  memcpy(buffer, &reply, 4);

	  /* bump up the buffer. */
	  buffer += 4;

	  /* bump up to the next entry. */
	  command += 5;
	}

      ubicom32command_packet_init(&packet);
    }

  return(ret);
}

static int single_step_thread=-1;

int
unclog_mailbox(void)
{
  send_console_data = 0;
  if (ubipodControlconsole)
    {
      /*
       * Read the mailbox status and see if we have to clean the mailbox interface.
       */
      int data;
      struct ubicom32command *command;
      int count, ret;
      unsigned int reply;
      struct ubicom32command_packet packet;
      ret = readStatus(&data);
      if(ret)
	return ret;

      while ((data & M_BOX_OUT_Q_EMPTY) == 0) {
	/*
	 * There is console data in outbound queue from target to host. Get the
	 * dongle to pull it out and send it to the console.
	 */
	ubicom32command_packet_init_opcode(&packet, ISPISD_OPCODE_UBICOM32COMMAND_DONGLE_UNCLOG_MAILBOX);
	ret = ubicom32command_packet_send(&packet);
	if (ret)
	  return ret;

	/*
	 * Wait till the Outbound Queue from target to host drains.
	 */
	ret = readStatus(&data);
	if(ret)
	  return ret;
      }

      while ((data & M_BOX_IN_Q_EMPTY) == 0) {
	/*
	 * There is console data in inbound queue from host to target. Get the
	 * dongle to pull it out of the mailbox and ship it back to the host via mailbox..
	 */
	ubicom32command_packet_init(&packet);

	/* Save Inbound Mailbox content to outbound mailbox. */
	count = ubicom32command_packet_append(&packet, IBUF_CHECK, ISD_WR_IBUF, move_from_mailbox_to_mailbox);

	/* pull the data out of mailbox. */
	count = ubicom32command_packet_append(&packet, READ_CHECK, ISD_READ_MAIL_BOX, 0);

	/* Ship the packet down for execution */
	ret = ubicom32command_packet_send(&packet);
	if(ret)
	  return ret;

	command = &packet.commands[1];

	reply = (command->operand0<<24) | (command->operand1<<16) |
	  (command->operand2<<8) | command->operand3;

	console_data[send_console_data++] = reply;

	/*
	 * Recover Mail Status.
	 */
	ret = readStatus(&data);
	if(ret)
	  return ret;
      }
    }

  return 0;
}

int
ubicom32v3singleStep(unsigned int threadNo)
{
  int ret, count;
  struct ubicom32command_packet packet;
  struct ubicom32command *command;
  unsigned int second_thread = (threadNo == 0) ? 1: 0;
  unsigned int previous_pc = current_thread_saved_state.pc;
  unsigned int current_pc = previous_pc;
  unsigned int num_attempts = 0;
  unsigned int mt_dbg_active;

  single_step_thread = threadNo;

  memset(current_cpu->registers, 0, current_cpu->reg_area_size);

  /* All the single step manipulations take place via some other thread.
   * Restore the current thread and setup a second thread do the manipulations.
   */
  ret = ubicom32v3restoreDebuggerThread(1);
  if(ret)
    return ret;

  while (current_pc == previous_pc)
    {
      ret = ubicom32v3setupDebuggerThread(second_thread);
      if(ret)
	return ret;

      ubicom32command_packet_init(&packet);

      /* deactivate all threads by executing movei mt_dbg_active_clr, #-1 */
      count = ubicom32command_packet_append(&packet, IBUF_CHECK, ISD_WR_IBUF, single_step_deactivate_threads);

      /* Activate the thread we want to single step by executing movei mt_dbg_active_set, #(1<<thredNo) */
      count = ubicom32command_packet_append(&packet, IBUF_CHECK, ISD_WR_IBUF, (single_step_activate_thread|(1<<threadNo)));

      /* Set the bit for the single stepping thread in mt_single_step */
      count = ubicom32command_packet_append(&packet, IBUF_CHECK, ISD_WR_IBUF, (setup_mt_single_step|(1<<threadNo)));

      /* Ship the packet down for execution */
      ret = ubicom32command_packet_send(&packet);
      if(ret)
	return ret;

      /* restore the second_thread registers */
      ret = ubicom32v3restoreDebuggerThread(1);

      ubicom32command_packet_init(&packet);

      // start all the threads
      count = ubicom32command_packet_append(&packet, 0, ISD_REL_MP, 0);

      /* Ship the packet down for execution */
      ret = ubicom32command_packet_send(&packet);
      if(ret)
	return ret;

      if(++num_attempts > 5)
	break;

      /* keep looping till mt_dbg_active goes to zero */
      while (1)
	{
	  ubicom32command_packet_init(&packet);

	  /* read mt_dbg_active */
	  count = ubicom32command_packet_append(&packet, 0, ISD_RD_MP_REG, 0x144);
	  command = &packet.commands[count];
	  count = ubicom32command_packet_append(&packet, 0, ISD_NOP, 0);

	  /* read back single step thread's pc */
	  count = ubicom32command_packet_append(&packet, 0, ISD_RD_MP_REG, (0xd0 | single_step_thread <<10));
	  count = ubicom32command_packet_append(&packet, 0, ISD_NOP, 0);

	  ret = ubicom32command_packet_send(&packet);
	  if(ret)
	    return ret;

	  mt_dbg_active = (command->operand0<<24) | (command->operand1<<16) | (command->operand2<<8) | command->operand3;
	  command++;
	  command++;
	  current_pc = (command->operand0<<24) | (command->operand1<<16) | (command->operand2<<8) | command->operand3;

	  if (mt_dbg_active == 0)
	    {
	      /* some debugger event has happened. Send down a HALT_MP to stop everybody */
	      ret = ubicom32v3haltMPandRreadDirectRegisters (&direct_read_regs);
	      if(ret)
		return ret;
	      break;
	    }
	}
    }
  return ret;
}

// Status holds the state of all threads and the processor
int status;

/*
 * ubicom32v3StopProcessos. This will issue HALT_MP to stop all thread. I will set the current thread to tbe thread 0.
 * We will then recover the current threads PC etc
 */
int
ubicom32v3stopProcessor(void)
{
  int ret;
  if(already_in_ubicom32v3StopProcessor)
    {
      ret = debugReset();

      /* sleep for .2 s before you attempt a connection */
      usleep(200000);
      ret = debugOpen();
    }

  already_in_ubicom32v3StopProcessor = 1;

  memset(current_cpu->registers, 0, current_cpu->reg_area_size);

  ret = ubicom32v3haltMPandRreadDirectRegisters (&direct_read_regs);
  if(ret)
    return ret;

  ubicom32v3currentThread = 0;

  /* setup the debugger thread */
  ret = ubicom32v3setupDebuggerThread(ubicom32v3currentThread);
  already_in_ubicom32v3StopProcessor = 0;

  ret = unclog_mailbox();
  if(ret)
    return ret;

  return 0;
}

/*
 * ubicom32v3restartProcessor. This will first restore the state of ubicom32v3currentThread. It will then issue REL_MP to restart all the threads.
 */
int
ubicom32v3restartProcessor(unsigned int stratch3value)
{
  int ret, count;
  struct ubicom32command_packet packet;

  ubicom32command_packet_init(&packet);
  /* force mt_single_step to zero movei mt_single_step, #0 */
  count = ubicom32command_packet_append(&packet, IBUF_CHECK, ISD_WR_IBUF, setup_mt_single_step);

  /* execute movei mt_dbg_active_set, #-1 to activate all threads */
  count = ubicom32command_packet_append(&packet, IBUF_CHECK, ISD_WR_IBUF, activate_all_threads);
  ret = ubicom32command_packet_send(&packet);
  if(ret)
    return ret;

  memset(current_cpu->registers, 0, current_cpu->reg_area_size);

  /* restore the currentThread registers */
  ret = ubicom32v3restoreDebuggerThread(stratch3value);

  /* Restart the 2 wire interface. */
  ret = turnOn2wire();

  ubicom32command_packet_init(&packet);

  // start all the threads
  count = ubicom32command_packet_append(&packet, 0, ISD_REL_MP, 0);

  if (ubipodControlconsole && send_console_data) {
    // append the console data to the mailbox.
    int i;
    for (i=0; i < send_console_data; i++) {
      count = ubicom32command_packet_append(&packet, WRITE_CHECK, ISD_WRITE_MAIL_BOX, console_data[i]);
    }
  }

  /* Ship the packet down for execution */
  ret = ubicom32command_packet_send(&packet);

  //
  return ret;
}


int
ubicom32v3createTpacketMP(unsigned int threadNo)
{
  // Retrieve d0, a5-a7, rosr, pc, mt_active, dbg_active, mt_enable
  // We will need these to construct a T packet
  int ret = current_cpu->fetch_tpacket_regs_fn(current_cpu, threadNo);

  if(ret)
    {
      printf("error: Could not retrieve registers for thread %d\n", threadNo);
      return ret;
    }

  return ret;
}

int
ubicom32v3waitForBP(bpReason_t *reason)
{
  int ret, count;
  struct ubicom32command_packet packet;
  struct ubicom32command *command;
  unsigned int mt_dbg_active, mt_break, mt_trap, dbg_halt_mp;
  static int thread_number = 0;
  int thread_mask = 1 << thread_number;
  int may_be_posix_gdb_event;

  may_be_posix_gdb_event = 0;

  /* read mt_dbg_active, mt_break and mt_trap and then analyze */
  ubicom32command_packet_init(&packet);

  /* read mt_dbg_active */
  count = ubicom32command_packet_append(&packet, 0, ISD_RD_MP_REG, 0x144);
  command = &packet.commands[count];
  count = ubicom32command_packet_append(&packet, 0, ISD_NOP, 0);

  /* read mt_break */
  count = ubicom32command_packet_append(&packet, 0, ISD_RD_MP_REG, 0x158);
  count = ubicom32command_packet_append(&packet, 0, ISD_NOP, 0);

  /* read mt_trap */
  count = ubicom32command_packet_append(&packet, 0, ISD_RD_MP_REG, 0x1bc);
  count = ubicom32command_packet_append(&packet, 0, ISD_NOP, 0);

  /* read dbg_halt_mp */
  count = ubicom32command_packet_append(&packet, 0, ISD_RD_RST_HALT_MP, 0);
  count = ubicom32command_packet_append(&packet, 0, ISD_NOP, 0);

  ret = ubicom32command_packet_send(&packet);
  if(ret)
    {
      /* some thing crapped out. Issue a reset and return control back to gdb */
      extern int ubicom32v3isp_reset(void);
      ret = ubicom32v3isp_reset();
      return 1;
    }

  /* Go recover the registers */
  mt_dbg_active = (command->operand0<<24) | (command->operand1<<16) |
    (command->operand2<<8) | command->operand3;

  command++;
  command++;

  mt_break = (command->operand0<<24) | (command->operand1<<16) |
    (command->operand2<<8) | command->operand3;

  command++;
  command++;
  mt_trap = (command->operand0<<24) | (command->operand1<<16) |
    (command->operand2<<8) | command->operand3;

  command++;
  command++;
  dbg_halt_mp = (command->operand0<<24) | (command->operand1<<16) |
    (command->operand2<<8) | command->operand3;

  if(mt_dbg_active && mt_break == 0 && mt_trap == 0 && ((dbg_halt_mp & 0x2) == 0))
    {
      /* Things are running properly. No debugger event to report. */
      return 0;
    }

  if (mt_trap)
    {
      may_be_posix_gdb_event = 0;
      while (may_be_posix_gdb_event < 100)
	{
	  ubicom32command_packet_init(&packet);

	  /* read mt_trap */
	  count = ubicom32command_packet_append(&packet, 0, ISD_RD_MP_REG, 0x1bc);
	  command = &packet.commands[count];
	  count = ubicom32command_packet_append(&packet, 0, ISD_NOP, 0);

	  ret = ubicom32command_packet_send(&packet);
	  if(ret)
	    {
	      /* some thing crapped out. Issue a reset and return control back to gdb */
	      extern int ubicom32v3isp_reset(void);
	      ret = ubicom32v3isp_reset();
	      return 1;
	    }

	  mt_trap = (command->operand0<<24) | (command->operand1<<16) |
	    (command->operand2<<8) | command->operand3;
	  may_be_posix_gdb_event++;

	  if (mt_trap == 0)
	    {
	      return 0;
	    }
	  else
	    {
	      if (ubipodControlconsole)
		{
		  ret = turnOnconsole();
		}

	      /* Sleep 10 ms and try again. */
	      usleep(10000);

	      if (ubipodControlconsole)
		{
		  ret = turnOffconsole();
		}
	    }
	}
    }

  /* some debugger event has happened. Send down a HALT_MP to stop everybody */
  ret = ubicom32v3haltMPandRreadDirectRegisters (&direct_read_regs);
  if(ret)
    return ret;

  /* analyze the cause */
  if(single_step_thread != -1 && mt_dbg_active == 0 && mt_break == 0 && mt_trap == 0)
    {
      /* Single step thread has finished execution. */
      ret = ubicom32v3setupDebuggerThread(single_step_thread);
      if(ret)
	{
	  single_step_thread = -1;
	  return -1;
	}

      ubicom32command_packet_init(&packet);
      /* force mt_single_step to zero movei mt_single_step, #0 */
      count = ubicom32command_packet_append(&packet, IBUF_CHECK, ISD_WR_IBUF, setup_mt_single_step);
      /* Ship the packet down for execution */
      ret = ubicom32command_packet_send(&packet);
      if(ret)
	return ret;

      reason->reason = BREAKPOINT_HIT;
      single_step_thread = -1;
#ifdef DEBUG
      printf ("SS mt_dbg_active = 0x%x mt_break = 0x%x mt_trap = 0x%x dbg_halt_mp = 0x%x\n",
			 mt_dbg_active, mt_break, mt_trap, dbg_halt_mp);
#endif
      return ubicom32v3_current_thread;
    }

  single_step_thread = -1;

  if(mt_dbg_active == 0 && mt_break == 0 && mt_trap == 0)
    {
      reason->reason = BREAKPOINT_HIT;
      thread_number = 0;
      /* special purpose break point */
      ret = ubicom32v3setupDebuggerThread(thread_number);
      if(ret)
	return -1;

      thread_number ++;
      if(thread_number == current_cpu->num_threads)
	thread_number = 0;

#ifdef DEBUG
      printf ("NR mt_dbg_active = 0x%x mt_break = 0x%x mt_trap = 0x%x dbg_halt_mp = 0x%x\n",
			 mt_dbg_active, mt_break, mt_trap, dbg_halt_mp);
#endif
      return ubicom32v3_current_thread;
    }

  if(dbg_halt_mp & 0x2)
    {
      reason->reason = HALT;
      thread_number = 0;
      /* Internal reset has happened */
      ret = ubicom32v3setupDebuggerThread(thread_number);

      /* We have to retrieve the reset reason  register in the timer block at address 0x01000084. */
      ret = ubicom32v3readMemory(0x01000084, 1, &reason->status);
      if(ret)
	return -1;

      thread_number ++;
      if(thread_number == current_cpu->num_threads)
	thread_number = 0;

#ifdef DEBUG
      printf ("HLT mt_dbg_active = 0x%x mt_break = 0x%x mt_trap = 0x%x dbg_halt_mp = 0x%x\n",
			 mt_dbg_active, mt_break, mt_trap, dbg_halt_mp);
#endif
      return ubicom32v3_current_thread;
    }

#if 0
  if (mt_trap)
    {
      may_be_posix_gdb_event = 0;
      while (may_be_posix_gdb_event < 100)
	{
	  ubicom32command_packet_init(&packet);
	  printf("Re Read mt trap\n");

	  /* read mt_trap */
	  count = ubicom32command_packet_append(&packet, 0, ISD_RD_MP_REG, 0x1bc);
	  command = &packet.commands[count];
	  count = ubicom32command_packet_append(&packet, 0, ISD_NOP, 0);

	  ret = ubicom32command_packet_send(&packet);
	  if(ret)
	    {
	      /* some thing crapped out. Issue a reset and return control back to gdb */
	      extern int ubicom32v3isp_reset(void);
	      ret = ubicom32v3isp_reset();
	      return 1;
	    }

	  mt_trap = (command->operand0<<24) | (command->operand1<<16) |
	    (command->operand2<<8) | command->operand3;
	  may_be_posix_gdb_event++;

	  if (mt_trap == 0)
	    {
	      break;
	    }
	  else
	    {
	      /* Sleep 10 ms and try again. */
	      usleep(10000);
	    }
	}
    }
#endif

#ifdef DEBUG
  printf("Mt trap = 0x%x\n", mt_trap);
#endif

  if(mt_trap)
    {
      int i;
      reason->reason = TRAP;
      for(i=0; i< current_cpu->num_threads; i++)
	{
	  if(mt_trap & thread_mask)
	    {
	      ret = ubicom32v3setupDebuggerThread(thread_number);
	      if(ret)
		return -1;

	      /* We got a hit. Bump up the mask anyway */
	      /* We have to retrieve the trap_cause register for this thread */
	      ret = ubicom32v3readRegisters(ubicom32v3currentThread, 35, &reason->status, 1);

	      /* need to clear out the trap bit for this thread */
	      //ret = write_ibuf((clear_trap_register|thread_mask));
	      ubicom32command_packet_init(&packet);
	      count = ubicom32command_packet_append(&packet, IBUF_CHECK, ISD_WR_IBUF, (clear_trap_register|thread_mask));
	      ubicom32command_packet_send(&packet);

	      /* bump up thread_number*/
	      thread_number ++;
	      if(thread_number == current_cpu->num_threads)
		thread_number = 0;

#ifdef DEBUG
	      printf ("TRAP mt_dbg_active = 0x%x mt_break = 0x%x mt_trap = 0x%x dbg_halt_mp = 0x%x\n",
			 mt_dbg_active, mt_break, mt_trap, dbg_halt_mp);
#endif
	      return ubicom32v3_current_thread;
	    }

	  /* Bump up the thread_number*/
	  thread_number ++;
	  if(thread_number == current_cpu->num_threads)
	    thread_number = 0;
	  thread_mask = 1 << thread_number;
	}
    }

  if(mt_break)
    {
      int i;
      reason->reason = BREAKPOINT_HIT;
      for(i=0; i< current_cpu->num_threads; i++)
	{
	  if(mt_break & thread_mask)
	    {
	      ret = ubicom32v3setupDebuggerThread(thread_number);
	      if(ret)
		return -1;

	      /* need to clear out the bit for this thread from mt_break register*/
	      //ret = write_ibuf((clear_break_register|thread_mask));
	      ubicom32command_packet_init(&packet);
	      count = ubicom32command_packet_append(&packet, IBUF_CHECK, ISD_WR_IBUF, (clear_break_register|thread_mask));
	      ubicom32command_packet_send(&packet);

	      /* We got a hit. Bump up thread_number */
	      thread_number ++;
	      if(thread_number == current_cpu->num_threads)
		thread_number = 0;

#ifdef DEBUG
	      printf ("BREAK: mt_dbg_active = 0x%x mt_break = 0x%x mt_trap = 0x%x dbg_halt_mp = 0x%x\n",
			 mt_dbg_active, mt_break, mt_trap, dbg_halt_mp);
#endif
	      return ubicom32v3_current_thread;
	    }

	  /* Bump up the thread_number*/
	  thread_number ++;
	  if(thread_number == current_cpu->num_threads)
	    thread_number = 0;
	  thread_mask = 1 << thread_number;
	}
    }
  return 0;
}

int
ubicom32v3waitForBPNoTrap(bpReason_t *reason)
{
  int ret, count;
  struct ubicom32command_packet packet;
  struct ubicom32command *command;
  unsigned int mt_dbg_active, mt_break, mt_trap, dbg_halt_mp;
  static int thread_number = 0;
  int thread_mask = 1 << thread_number;
  int may_be_posix_gdb_event;

  may_be_posix_gdb_event = 0;

  /* read mt_dbg_active, mt_break and mt_trap and then analyze */
  ubicom32command_packet_init(&packet);

  /* read mt_dbg_active */
  count = ubicom32command_packet_append(&packet, 0, ISD_RD_MP_REG, 0x144);
  command = &packet.commands[count];
  count = ubicom32command_packet_append(&packet, 0, ISD_NOP, 0);

  /* read mt_break */
  count = ubicom32command_packet_append(&packet, 0, ISD_RD_MP_REG, 0x158);
  count = ubicom32command_packet_append(&packet, 0, ISD_NOP, 0);

  /* read mt_trap */
  count = ubicom32command_packet_append(&packet, 0, ISD_RD_MP_REG, 0x1bc);
  count = ubicom32command_packet_append(&packet, 0, ISD_NOP, 0);

  /* read dbg_halt_mp */
  count = ubicom32command_packet_append(&packet, 0, ISD_RD_RST_HALT_MP, 0);
  count = ubicom32command_packet_append(&packet, 0, ISD_NOP, 0);

  ret = ubicom32command_packet_send(&packet);
  if(ret)
    {
      /* some thing crapped out. Issue a reset and return control back to gdb */
      extern int ubicom32v3isp_reset(void);
      ret = ubicom32v3isp_reset();
      return 1;
    }

  /* Go recover the registers */
  mt_dbg_active = (command->operand0<<24) | (command->operand1<<16) |
    (command->operand2<<8) | command->operand3;

  command++;
  command++;

  mt_break = (command->operand0<<24) | (command->operand1<<16) |
    (command->operand2<<8) | command->operand3;

  command++;
  command++;
  mt_trap = (command->operand0<<24) | (command->operand1<<16) |
    (command->operand2<<8) | command->operand3;

  command++;
  command++;
  dbg_halt_mp = (command->operand0<<24) | (command->operand1<<16) |
    (command->operand2<<8) | command->operand3;

  if(mt_dbg_active && mt_break == 0 && ((dbg_halt_mp & 0x2) == 0))
    {
      /* Things are running properly. No debugger event to report. */
      return 0;
    }

  /* some debugger event has happened. Send down a HALT_MP to stop everybody */
  ret = ubicom32v3haltMPandRreadDirectRegisters (&direct_read_regs);
  if(ret)
    return ret;

  /* analyze the cause */
  if(single_step_thread != -1 && mt_dbg_active == 0 && mt_break == 0)
    {
      /* Single step thread has finished execution. */
      ret = ubicom32v3setupDebuggerThread(single_step_thread);
      if(ret)
	{
	  single_step_thread = -1;
	  return -1;
	}

      ubicom32command_packet_init(&packet);
      /* force mt_single_step to zero movei mt_single_step, #0 */
      count = ubicom32command_packet_append(&packet, IBUF_CHECK, ISD_WR_IBUF, setup_mt_single_step);
      /* Ship the packet down for execution */
      ret = ubicom32command_packet_send(&packet);
      if(ret)
	return ret;

      reason->reason = BREAKPOINT_HIT;
      single_step_thread = -1;
#ifdef DEBUG
      printf ("SS mt_dbg_active = 0x%x mt_break = 0x%x mt_trap = 0x%x dbg_halt_mp = 0x%x\n",
			 mt_dbg_active, mt_break, mt_trap, dbg_halt_mp);
#endif
      return ubicom32v3_current_thread;
    }

  single_step_thread = -1;

  if(mt_dbg_active == 0 && mt_break == 0)
    {
      reason->reason = BREAKPOINT_HIT;
      thread_number = 0;
      /* special purpose break point */
      ret = ubicom32v3setupDebuggerThread(thread_number);
      if(ret)
	return -1;

      thread_number ++;
      if(thread_number == current_cpu->num_threads)
	thread_number = 0;

#ifdef DEBUG
      printf ("NR mt_dbg_active = 0x%x mt_break = 0x%x mt_trap = 0x%x dbg_halt_mp = 0x%x\n",
			 mt_dbg_active, mt_break, mt_trap, dbg_halt_mp);
#endif
      return ubicom32v3_current_thread;
    }

  if(dbg_halt_mp & 0x2)
    {
      reason->reason = HALT;
      thread_number = 0;
      /* Internal reset has happened */
      ret = ubicom32v3setupDebuggerThread(thread_number);

      /* We have to retrieve the reset reason  register in the timer block at address 0x01000084. */
      ret = ubicom32v3readMemory(0x01000084, 1, (int *)&reason->status);
      if(ret)
	return -1;

      thread_number ++;
      if(thread_number == current_cpu->num_threads)
	thread_number = 0;

#ifdef DEBUG
      printf ("HLT mt_dbg_active = 0x%x mt_break = 0x%x mt_trap = 0x%x dbg_halt_mp = 0x%x\n",
			 mt_dbg_active, mt_break, mt_trap, dbg_halt_mp);
#endif
      return ubicom32v3_current_thread;
    }

  if(mt_break)
    {
      int i;
      reason->reason = BREAKPOINT_HIT;
      for(i=0; i< current_cpu->num_threads; i++)
	{
	  if(mt_break & thread_mask)
	    {
	      ret = ubicom32v3setupDebuggerThread(thread_number);
	      if(ret)
		return -1;

	      /* need to clear out the bit for this thread from mt_break register*/
	      //ret = write_ibuf((clear_break_register|thread_mask));
	      ubicom32command_packet_init(&packet);
	      count = ubicom32command_packet_append(&packet, IBUF_CHECK, ISD_WR_IBUF, (clear_break_register|thread_mask));
	      ubicom32command_packet_send(&packet);

	      /* We got a hit. Bump up thread_number */
	      thread_number ++;
	      if(thread_number == current_cpu->num_threads)
		thread_number = 0;

#ifdef DEBUG
	      printf ("BREAK: mt_dbg_active = 0x%x mt_break = 0x%x mt_trap = 0x%x dbg_halt_mp = 0x%x\n",
			 mt_dbg_active, mt_break, mt_trap, dbg_halt_mp);
#endif
	      return ubicom32v3_current_thread;
	    }

	  /* Bump up the thread_number*/
	  thread_number ++;
	  if(thread_number == current_cpu->num_threads)
	    thread_number = 0;
	  thread_mask = 1 << thread_number;
	}
    }
  return 0;
}

/* Detach the debugger from the debug kernel. Restart the processor. Leave the ISP connection open */
int
ubicom32v3isp_detach(void)
{
  int ret, count;
  struct ubicom32command_packet packet;

  ubicom32command_packet_init(&packet);

  // Write 0 to RST_HALT_MP_EN
  count = ubicom32command_packet_append(&packet, 0, ISD_WR_RST_HALT_MP_EN, 0);

  /* Ship the packet down for execution */
  ret = ubicom32command_packet_send(&packet);

  if(ret)
    return ret;

  return(ubicom32v3restartProcessor(0));
}

/* This routine only establishes connection to the dongle */
int
ubicom32v3isp_connect(char *remote)
{
  struct ubicom32command_packet packet;
  int ret;

  if(dongle_interface_status())
    {
      ubicom32v3isp_detach();
      dongle_close();
    }

  if(dongle_open(remote) == -1) {
	return -1;
  }

  pending_packet = NULL;
  doubleBuffer = 0;

  /* See if we can control the console interface. */
  ubicom32command_packet_init_opcode(&packet, ISPISD_OPCODE_UBICOM32COMMAND_DONGLE_STOP_CONSOLE);
  errorExpected = 1;
  ret = ubicom32command_packet_send(&packet);
  errorExpected = 0;

  if(ret == 0)
    {
      ubipodControlconsole = 1;
    }
  else
    {
      ubipodControlconsole = 0;
      dongle_close();

      if(dongle_open(remote) == -1)
	return -1;
    }

  /* Test and see if we are hooked up to a Ubipod dongle. */
  ubicom32command_packet_init_opcode(&packet, ISPISD_OPCODE_UBICOM32COMMAND_DONGLE_CHECK_UBIPOD);
  errorExpected = 1;
  ret = ubicom32command_packet_send(&packet);
  errorExpected = 0;

  if(ret ==0)
    {
      dongleIsUbipod = 1;
      dongleHasMarsSupport = 1;
      okToDoubleBuffer = 1;

      /* See if we can control the 2 wire interface. */
      ubicom32command_packet_init_opcode(&packet, ISPISD_OPCODE_UBICOM32COMMAND_DONGLE_START_2WIRE);
      errorExpected = 1;
      ret = ubicom32command_packet_send(&packet);
      errorExpected = 0;

      if(ret == 0)
	{
	  ubipodControl2wire = 1;
	}
      else
	{
	  ubipodControl2wire = 0;
	  dongle_close();

	  if(dongle_open(remote) == -1)
	    return -1;
	}
      return 0;
    }
  else
    {
      dongleIsUbipod = 0;
      dongle_close();

      if(dongle_open(remote) == -1)
	return -1;
    }

  ubicom32command_packet_init_opcode(&packet, ISPISD_OPCODE_UBICOM32COMMAND_DONGLE_CHECK_MARS);
  errorExpected = 1;
  ret = ubicom32command_packet_send(&packet);
  errorExpected = 0;

  if(ret ==0)
    {
      dongleHasMarsSupport = 1;
    }
  else
    {
      dongleHasMarsSupport=0;
      dongle_close();

      /* inform that the dongle has no Mars support */
      return -2;
    }

  okToDoubleBuffer = 1;
  return 0;
}

int
ubicom32v3isp_close(void)
{

  if (ubipodControlconsole)
    {
      /*
       * call turnOnconsole after disconnect from dongle in ubicom32v3isp_close()
       */
      int ret = turnOnconsole();
    }
  return (dongle_close());
}

int
ubicom32v3isp_force_open(void)
{
  struct ubicom32command_packet packet;
  int ret, count;

  ubicom32command_packet_init(&packet);

  // Send down ISD_OPEN
  count = ubicom32command_packet_append(&packet, OPTION_FORCE|OPTION_IGNORE_RESPONSE, ISD_OPEN, 0);

  count = ubicom32command_packet_append(&packet, OPTION_FORCE, ISD_OPEN, 0);

  count = ubicom32command_packet_append(&packet, OPTION_FORCE, ISD_OPEN, 0);
  ret = ubicom32command_packet_send(&packet);

  return 0;
}

/*
 * This routine will contact the Ubicom32v3 hardware via the debug port.
 * By the time this is called the serial connection is alive.
 */
int
ubicom32v3isp_attach(void)
{
  int ret;
  int num_attempts = 0;
  extern unsigned int debugging_processor;

  if(dongle_interface_status() == 0)
    return -3;

  // Open connection to hardware
  while(num_attempts++ <10)
    {
      ret = debugOpen();
      if(ret)
	{
	  ret = ubicom32v3isp_force_open();
	  ret = debugClose();
	  continue;
	}
      else
	break;
    }

  if(ret)
    return -1;

  /* This would be a good point to see if we are indeed talking to a 5k 3k or nothing at all.*/
  if (debugging_processor == 0)
    {
      return -3;
    }
  else if (debugging_processor != 5133)
    {
      /* Processor is a 3k. */
      return -2;
    }

  iport_stat = 0;

  ret = ubicom32v3stopProcessor();
  return 0;
}

int
ubicom32v3isp_reset(void)
{
  /* reset the board through the mailbox interface */
  int ret;
  extern unsigned int dontDebugMask;

  if(dongle_interface_status() == 0)
    {
      printf("No connection to target. Request denied.\n");
      return -1;
    }

  ret = debugReset();

  /* sleep for .2 s before you attempt a connection */
  usleep(200000);

  return(ubicom32v3isp_attach());
}

#if 0
int
ubicom32v3_hw_monitor(unsigned int addr, unsigned int needEvent,
	   unsigned int triggerEvent, unsigned int eventPattern,
	   unsigned int eventMask,
	   unsigned int runCounter, unsigned int *resBuf, char *args)
{
  int ret, count;
  unsigned int stat, numTimes;

  struct ubicom32command_packet packet;
  struct ubicom32command *command;
  unsigned int reply;
  int commandEchoIndex;

  if(dongle_interface_status() == 0)
    {
      if(args == NULL)
	return -3;
      ubicom32isp_connect(args);

      if(dongle_interface_status() == 0)
	return -4;

      ret = debugOpen();
      if(ret)
	return ret;
    }

  ubicom32command_packet_init(&packet);

  // Initialize by writing 0 to MME register
  count = ubicom32command_packet_append(&packet, 0, ISD_WR_MME, 0);

  // Write the Memory Object Address (MOA) register
  count = ubicom32command_packet_append(&packet, 0, ISD_WR_MOA, addr);

  if(needEvent)
    {
      // Write Event Pattern register
      count = ubicom32command_packet_append(&packet, 0, ISD_WR_MEP, eventPattern);

      // Write the Memory Event Mask register
      count = ubicom32command_packet_append(&packet, 0, ISD_WR_MEM, eventMask);

      // Write the Memory Event Control register with Event Enable (bit 15) on
      triggerEvent |= 0x8000;
    }
  else
    triggerEvent = 0;

  //Writeto Memory event Control Register
  count = ubicom32command_packet_append(&packet, 0, ISD_WR_MEC, triggerEvent);

  // Set the Monitor Run Counter register
  count = ubicom32command_packet_append(&packet, 0, ISD_WR_MRC, runCounter);

  // Start monitoring by writing a 1 to Monitor Mode Enable
  count = ubicom32command_packet_append(&packet, 0, ISD_WR_MME, 1);

  // Ship this packet down to get monitoring going
  // Ship the packet for processing
  ret = ubicom32command_packet_send(&packet);
  if(ret)
    return ret;

  numTimes = 0;
  while(1)
    {
      // Monitor the status
      ubicom32command_packet_init(&packet);

      // Send down ISD_RD_MST
      count = ubicom32command_packet_append(&packet, 0, ISD_RD_MST, 0);

      ret = ubicom32command_packet_send(&packet);
      if(ret)
	return ret;

      command = &packet.commands[0];

      // extract status
      stat = (command->operand0<<24) | (command->operand1<<16) | (command->operand2<<8) | command->operand3;

      if(stat &1)
	break;

      numTimes++;
      if(numTimes > 2)
	break;
    }

  if(numTimes > 2)
    return -6;
#ifdef DEBUG
  printf("status returned = 0x%08x\n", stat);
#endif

  ubicom32command_packet_init(&packet);

  if(runCounter > 32)
    runCounter = 32;


  for(numTimes=0; numTimes< runCounter; numTimes++)
    {
      // send down ISD_WR_MBRE command
      count = ubicom32command_packet_append(&packet, 0, ISD_WR_MBRE, 0);
    }

  ret = ubicom32command_packet_send(&packet);
  if(ret)
    return ret;

  command = &packet.commands[runCounter-1];

  for(numTimes=0; numTimes< runCounter; numTimes++, command--)
    {
      stat = (command->operand0<<24) | (command->operand1<<16) | (command->operand2<<8) | command->operand3;

      *resBuf++ = stat;
    }

  return 0;
}
#endif

unsigned int  bist_reset_code[] = {
	0xe022000e,
	0x04216060,
	0x04216000,
	0xc8000000,
	0x24213c21,
	0xc9000001,
	0x04206100,
};

int
ubicom32v3isp_bist(void)
{
  /* run the bist sequence from the debugger */
  int count, i;
  struct ubicom32command_packet packet;
  struct ubicom32command *command;
  unsigned int reply1, reply2;

  if (chipID >= 0x30000)
    return 0;

  ubicom32command_packet_init(&packet);

#if defined DOING_RTL
  count = ubicom32command_packet_append(&packet, 0, ISD_RD_IPORT_STAT, 0);
  for(i=0; i< sizeof(bist_reset_code)/4; i++)
    {
      count = ubicom32command_packet_append(&packet, IBUF_CHECK, ISD_WR_IBUF, bist_reset_code[i]);
    }
  count = ubicom32command_packet_append(&packet, 0, ISD_RD_IPORT_STAT, 0);
  ubicom32command_packet_send(&packet);
#else
  //count = ubicom32command_packet_append(&packet, 0, ISD_RD_IPORT_STAT, 0);
  for(i=0; i< sizeof(bist_sequence_1)/4; i++)
    {
      count = ubicom32command_packet_append(&packet, IBUF_CHECK, ISD_WR_IBUF, bist_sequence_1[i]);
    }
  count = ubicom32command_packet_append(&packet, 0, ISD_RD_IPORT_STAT, 0);
  ubicom32command_packet_send(&packet);

  command = &packet.commands[count];
  reply1 = (command->operand0<<24) | (command->operand1<<16) |
    (command->operand2<<8) | command->operand3;

  reply1 &= ~3;

  while(1)
    {
      ubicom32command_packet_init(&packet);
      for(i=0; i< sizeof(bist_sequence_2)/4; i++)
	{
	  count = ubicom32command_packet_append(&packet, IBUF_CHECK, ISD_WR_IBUF, bist_sequence_2[i]);
	}
      count = ubicom32command_packet_append(&packet, 0, ISD_RD_IPORT_STAT, 0);

      ubicom32command_packet_send(&packet);

      command = &packet.commands[count];

      reply2 = (command->operand0<<24) | (command->operand1<<16) |
	(command->operand2<<8) | command->operand3;

      reply2 &= ~3;
      //printf("rep1 =0x%x rep2 =0x%x\n", reply1, reply2);
      if(reply2 > reply1)
	break;
    }
#endif
  return 0;
}

unsigned int init_pll_sequence_1[] = {
  0xe0040000,	//moveai a0,#%hi(0x02000000)
  0xc9000b20,	//movei d0,#2848 250Mhz Flash access time 20ns
  0xc9010615,	//movei d1,#2581 250Mhz Flash access time 20ns
  0x17c00101,	//shmrg.2 d0,d1,d0
  0x040c6100,	//move.4 48(a0),d0
  0xe0020000,	//moveai a0,#%hi(0x01000000)
  0xe0220002,	//moveai a1,#%hi(0x01000100)
  0x2c002400,	//bclr (a0),(a0),#0x4
  //  0x2c012401,	//bclr 4(a0),4(a0),#0x4
  //0x2c022402,	//bclr 8(a0),8(a0),#0x4
  //0x2c032403,	//bclr 12(a0),12(a0),#0x4
  0x24003c00,	//bset (a0),(a0),#0x7
  //0x24013c01,	//bset 4(a0),4(a0),#0x7
  //0x24023c02,	//bset 8(a0),8(a0),#0x7
  //0x24033c03,	//bset 12(a0),12(a0),#0x7
  0xcc004283,	//movei (a0),#17027
  //0xcc014880,	//movei 2(a0),#18560 125 Mhz
  0xcc019880,	//movei 2(a0),#-26496 250 Hhz
  //0xcc024202,	//movei 4(a0),#16898
  //0xcc03f880,	//movei 6(a0),#-1920
  //0xcc044000,	//movei 8(a0),#16384
  //0xcc059980,	//movei 10(a0),#-26240
  //0xcc060000,	//movei 12(a0),#0
  //0xcc070080,	//movei 14(a0),#128
  0x2c003c00,	//bclr (a0),(a0),#0x7
  //0x2c013c01,	//bclr 4(a0),4(a0),#0x7
  //0x2c023c02,	//bclr 8(a0),8(a0),#0x7
};

unsigned int init_pll_sequence_2[] = {
  0x24002400,	//bset (a0),(a0),#0x4
  //0x24012401,	//bset 4(a0),4(a0),#0x4
  //0x24022402,	//bset 8(a0),8(a0),#0x4
  //0x2c033c03,	//bclr 12(a0),12(a0),#0x7 ??
};

unsigned int pll_delay[] = {
  0xc90003e8,	//movei d0,#1000
  0x79000420,	//add.4 d0,(a1),d0
  0x90000420,	//sub.4 #0,(a1),d0
  0xd3ffffff,	//jmple.t 6002acf8
};

int
ubicom32v3isp_pll_init(void)
{
  /* run the pll sequence from the debugger */
  int count, i;
  struct ubicom32command_packet packet;
  struct ubicom32command *command;
  unsigned int reply1, reply2;

  ubicom32command_packet_init(&packet);
  //count = ubicom32command_packet_append(&packet, 0, ISD_RD_IPORT_STAT, 0);
  for(i=0; i< sizeof(init_pll_sequence_1)/4; i++)
    {
      count = ubicom32command_packet_append(&packet, IBUF_CHECK, ISD_WR_IBUF, init_pll_sequence_1[i]);
    }
  count = ubicom32command_packet_append(&packet, 0, ISD_RD_IPORT_STAT, 0);
  ubicom32command_packet_send(&packet);

  command = &packet.commands[count];
  reply1 = (command->operand0<<24) | (command->operand1<<16) |
    (command->operand2<<8) | command->operand3;

  reply1 &= ~3;

  /* delay for the pll */
  while(1)
    {
      ubicom32command_packet_init(&packet);
      for(i=0; i< sizeof(pll_delay)/4; i++)
	{
	  count = ubicom32command_packet_append(&packet, IBUF_CHECK, ISD_WR_IBUF, pll_delay[i]);
	}
      count = ubicom32command_packet_append(&packet, 0, ISD_RD_IPORT_STAT, 0);

      ubicom32command_packet_send(&packet);

      command = &packet.commands[count];

      reply2 = (command->operand0<<24) | (command->operand1<<16) |
	(command->operand2<<8) | command->operand3;

      reply2 &= ~3;
      printf("rep1 =0x%x rep2 =0x%x\n", reply1, reply2);
      if(reply2 > reply1)
	break;
    }

  /* send down pll_init_sequence_2 */
  ubicom32command_packet_init(&packet);
  for(i=0; i< sizeof(init_pll_sequence_2)/4; i++)
    {
      count = ubicom32command_packet_append(&packet, 0x20, ISD_WR_IBUF, init_pll_sequence_2[i]);
    }
  count = ubicom32command_packet_append(&packet, 0, ISD_RD_IPORT_STAT, 0);
  ubicom32command_packet_send(&packet);

  command = &packet.commands[count];
  reply1 = (command->operand0<<24) | (command->operand1<<16) |
    (command->operand2<<8) | command->operand3;

  reply1 &= ~3;

  /* delay for the pll */
  while(1)
    {
      ubicom32command_packet_init(&packet);
      for(i=0; i< sizeof(pll_delay)/4; i++)
	{
	  count = ubicom32command_packet_append(&packet, 0x20, ISD_WR_IBUF, pll_delay[i]);
	}
      count = ubicom32command_packet_append(&packet, 0, ISD_RD_IPORT_STAT, 0);

      ubicom32command_packet_send(&packet);

      command = &packet.commands[count];

      reply2 = (command->operand0<<24) | (command->operand1<<16) |
	(command->operand2<<8) | command->operand3;

      reply2 &= ~3;
      printf("rep1 =0x%x rep2 =0x%x\n", reply1, reply2);
      if(reply2 > reply1)
	break;
    }
  return 0;
}

unsigned int fast_transfer_code[] = {
  /* read into processor */
  0x10c0f402,	//btst 8(a0),#0x1e
  0xd5ffffff,	//jmpne.t 3ffc0000 <__data_begin>
  0x02216400,	//move.4 (a1)4++,(a0)
  0x790000ff,	//add.4 d0,#-1,d0
  0xd5fffffc,	//jmpne.t 3ffc0000 <__data_begin>
  0xd6e00000,	//jmpt.t 3ffc0014 <__data_begin+0x14>

  /* read out of processor */
  0x10c0ec02,	//btst 8(a0),#0x1d
  0xd5ffffff,	//jmpne.t 3ffc0018 <mem_read_out>
  0x04016221,	//move.4 4(a0),(a1)4++
  0x790000ff,	//add.4 d0,#-1,d0
  0xd5fffffc,	//jmpne.t 3ffc0018 <mem_read_out>
  0xd6e00000,	//jmpt.t 3ffc002c <mem_read_out+0x14>

#if defined DOING_RTL
  0xc8000000,	//nop to satisfy RTL
  0xc8000000,	//nop to satisfy RTL
  0xc8000000,	//nop to satisfy RTL
  0xc8000000,	//nop to satisfy RTL
  0xc8000000,	//nop to satisfy RTL
  0xc8000000,	//nop to satisfy RTL
  0xc8000000,	//nop to satisfy RTL
  0xc8000000,	//nop to satisfy RTL
  0xc8000000,	//nop to satisfy RTL
  0xc8000000,	//nop to satisfy RTL
#endif
};

int
ubicom32v3isp_download_fast_transfer_code(unsigned int address)
{
  char junk[sizeof(fast_transfer_code)];

  /* push the fast transfer code to the given address */
  int ret = ubicom32v3writeMemory(address, sizeof(fast_transfer_code)/4, fast_transfer_code);

  /* set up the location of the fast transfer code */
  fast_write_code_address = address;
  fast_read_code_address = address + 6*4;

  ret = ubicom32v3readMemory(address, sizeof(fast_transfer_code)/4, (int *)junk);
  return ret;
}

int
ubicom32v3isp_wakeup_downloader(void)
{
  int ret, count;
  struct ubicom32command *command;
  unsigned int reply;
  struct ubicom32command_packet packet;

  ubicom32command_packet_init(&packet);

  // stop all the threads
  count = ubicom32command_packet_append(&packet, WRITE_CHECK, ISD_WRITE_MAIL_BOX, READ_STATUS);

  /* Ship the packet down for execution */
  ret = ubicom32command_packet_send(&packet);
  if(ret)
    return ret;

  // We need to wait for the reply
  ubicom32command_packet_init(&packet);

  /* wait for READ_STATUS_DONE */
  count = ubicom32command_packet_append(&packet, READ_CHECK, ISD_READ_MAIL_BOX, 0);

  /* Read back the status word */
  count = ubicom32command_packet_append(&packet, READ_CHECK, ISD_READ_MAIL_BOX, 0);


  /* Ship the packet down for execution */
  ret = ubicom32command_packet_send(&packet);
  if(ret)
    return ret;

  command = &packet.commands[0];

  reply = (command->operand0<<24) | (command->operand1<<16) |
    (command->operand2<<8) | command->operand3;

  if(reply != READ_STATUS_DONE)
    {
      printf("Reply 0x%x does not match READ_STATUS_DONE\n", reply);
      return -1;
    }

  return 0;
}

unsigned int make_thread0_alive[] = {
  0xc9530001,	//movei mt_en,#1
  0xc94f0001,	//movei mt_active_set,#1
};

int
ubicom32v3isp_make_thread0_alive(void)
{
  int ret, count;
  struct ubicom32command_packet packet;

  ubicom32command_packet_init(&packet);
  count = ubicom32command_packet_append(&packet, IBUF_CHECK, ISD_WR_IBUF, make_thread0_alive[0]);
  count = ubicom32command_packet_append(&packet, IBUF_CHECK, ISD_WR_IBUF, make_thread0_alive[1]);

  /* Ship the packet down for execution */
  ret = ubicom32command_packet_send(&packet);
  return ret;
}

int
ubicom32v3hw_monitor_setup(unsigned int addr, unsigned int needEvent,
	   unsigned int triggerEvent, unsigned int eventPattern,
	   unsigned int eventMask,
	   unsigned int runCounter)
{
  int ret, count;

  struct ubicom32command_packet packet;

  if(dongle_interface_status() == 0)
    {
	return -4;
    }

  ubicom32command_packet_init(&packet);

  // Initialize by writing 0 to MME register
  count = ubicom32command_packet_append(&packet, 0, ISD_WR_MME, 0);

  // Write the Memory Object Address (MOA) register
  count = ubicom32command_packet_append(&packet, 0, ISD_WR_MOA, addr);

  if(needEvent)
    {
      // Write Event Pattern register
      count = ubicom32command_packet_append(&packet, 0, ISD_WR_MEP, eventPattern);

      // Write the Memory Event Mask register
      count = ubicom32command_packet_append(&packet, 0, ISD_WR_MEM, eventMask);

      // Write the Memory Event Control register with Event Enable (bit 15) on
      triggerEvent |= 0x8000;
    }
  else
    triggerEvent = 0;

  //Writeto Memory event Control Register
  count = ubicom32command_packet_append(&packet, 0, ISD_WR_MEC, triggerEvent);

  // Set the Monitor Run Counter register
  count = ubicom32command_packet_append(&packet, 0, ISD_WR_MRC, runCounter);

  // Start monitoring by writing a 1 to Monitor Mode Enable
  count = ubicom32command_packet_append(&packet, 0, ISD_WR_MME, 1);

  // Ship this packet down to get monitoring going
  // Ship the packet for processing
  ret = ubicom32command_packet_send(&packet);
  if(ret)
    return ret;

  return ret;
}

int
ubicom32v3isp_hw_monitor_status(unsigned int *status,
			  unsigned int *resBuf,
			  unsigned int runCounter)
{
  struct ubicom32command_packet packet;
  int ret, count;
  unsigned int stat, numTimes;
  struct ubicom32command *command;

  *status = -1;
  numTimes = 0;
  while(1)
    {
      // Monitor the status
      ubicom32command_packet_init(&packet);

      // Send down ISD_RD_MST
      count = ubicom32command_packet_append(&packet, 0, ISD_RD_MST, 0);

      ret = ubicom32command_packet_send(&packet);
      if(ret)
	return ret;

      command = &packet.commands[0];

      // extract status
      stat = (command->operand0<<24) | (command->operand1<<16) | (command->operand2<<8) | command->operand3;

      if(stat &1)
	{
	  *status = 1;
	  break;
	}

      numTimes++;
      if(numTimes > 10)
	return ret;
    }

#ifdef DEBUG
  printf("status returned = 0x%08x\n", stat);
#endif

  ubicom32command_packet_init(&packet);

  if(runCounter > 32)
    runCounter = 32;


  for(numTimes=0; numTimes< runCounter; numTimes++)
    {
      // send down ISD_WR_MBRE command
      count = ubicom32command_packet_append(&packet, 0, ISD_WR_MBRE, 0);
    }

  ret = ubicom32command_packet_send(&packet);
  if(ret)
    return ret;

  command = &packet.commands[runCounter-1];

  for(numTimes=0; numTimes< runCounter; numTimes++, command--)
    {
      stat = (command->operand0<<24) | (command->operand1<<16) | (command->operand2<<8) | command->operand3;

      *resBuf++ = stat;
    }

  return 0;
}

void verify_reg_read(void)
{
  struct ubicom32command_packet packet;
  struct ubicom32command *command;
  unsigned int reply;
  int ret, count;

  ubicom32command_packet_init(&packet);

  count = ubicom32command_packet_append(&packet, 0, ISD_RD_MP_REG, 0x100);
  count = ubicom32command_packet_append(&packet, 0, ISD_NOP, 0);

  count = ubicom32command_packet_append(&packet, 0, ISD_RD_MP_REG, 0x104);
  count = ubicom32command_packet_append(&packet, 0, ISD_NOP, 0);

  count = ubicom32command_packet_append(&packet, 0, ISD_RD_MP_REG, 0x108);
  count = ubicom32command_packet_append(&packet, 0, ISD_NOP, 0);

  count = ubicom32command_packet_append(&packet, 0, ISD_RD_MP_REG, 0x138);
  count = ubicom32command_packet_append(&packet, 0, ISD_NOP, 0);

  count = ubicom32command_packet_append(&packet, 0, ISD_RD_MP_REG, 0x1a4);
  count = ubicom32command_packet_append(&packet, 0, ISD_NOP, 0);

  count = ubicom32command_packet_append(&packet, 0, ISD_RD_MP_REG, 0x1a8);
  count = ubicom32command_packet_append(&packet, 0, ISD_NOP, 0);

  ret = ubicom32command_packet_send(&packet);

  if (ret)
    return;

  command = &packet.commands[0];

  reply = (command->operand0<<24) | (command->operand1<<16) | (command->operand2<<8) | command->operand3;
  command++;
  command++;
  printf("Chip ID 0x%x\n", reply);

  reply = (command->operand0<<24) | (command->operand1<<16) | (command->operand2<<8) | command->operand3;
  command++;
  command++;
  printf("Int Stat0 0x%x\n", reply);

  reply = (command->operand0<<24) | (command->operand1<<16) | (command->operand2<<8) | command->operand3;
  command++;
  command++;
  printf("Int Stat1 0x%x\n", reply);

  reply = (command->operand0<<24) | (command->operand1<<16) | (command->operand2<<8) | command->operand3;
  command++;
  command++;
  printf("Mt_active 0x%x\n", reply);

  reply = (command->operand0<<24) | (command->operand1<<16) | (command->operand2<<8) | command->operand3;
  command++;
  command++;
  printf("MT_I_BLOCKED 0x%x\n", reply);

  reply = (command->operand0<<24) | (command->operand1<<16) | (command->operand2<<8) | command->operand3;
  command++;
  command++;
  printf("MT_D_BLOCKED 0x%x\n", reply);

}

static int
ubicom32v3_fast_write_bytes (unsigned int address, char *buffer, unsigned int length)
{
  // split the transfer into lead, aligned middle and end
  unsigned truncBytes = (address & 0x3);
  unsigned truncAddress = address & ~0x3;
  unsigned int leadBytes, leadAddr, midBytes, midAddr, endBytes, endAddr, leadIndex;
  unsigned toWriteWords;
  unsigned char  *sendBuffer;
  unsigned char *freeBuffer;
  unsigned char *cptr;
  int j, ret;
  unsigned int i;

  leadBytes =0;
  leadIndex = 0;
  if(truncBytes)
    {
      leadBytes = 4-truncBytes;
      if(leadBytes > length)
	leadBytes = length;
      leadIndex = 3-truncBytes;
      length -= leadBytes;
      address = truncAddress + 4;
    }

  leadAddr = truncAddress;

  endAddr = address + length;
  endBytes = endAddr & 0x3;
  endAddr &= ~0x3;

  midBytes = length - endBytes;
  midAddr = address;

  toWriteWords = midBytes/4;

  if(leadBytes)
    toWriteWords++;

  if(endBytes)
    toWriteWords++;

  /* Allocate the send buffer */
  sendBuffer = (unsigned char *)malloc(toWriteWords*4);
  freeBuffer = sendBuffer;
  cptr = sendBuffer;

  if(leadBytes)
    {
      // Misaligned start. Deal with it by first backing up the address to nearest
      // Go read 4 bytes from the backed up address
      ret = ubicom32v3readMemory(leadAddr, 1, (int *)cptr);

      for(i=0; i<leadBytes; i++)
	{
	  cptr[leadIndex--] = (unsigned char) *buffer++;
	}

      // Bump up cptr by 4
      cptr += 4;
    }

  // Deal with the midsection if any
  if(midBytes)
    {
      // Move the data into the transfer buffer
      for(i=0; i< midBytes; i+= 4)
	{
	  for(j=0; j< 4; j++)
	    {
	      cptr[3-j] = (*buffer++ &0xff);
	    }
	  cptr+= 4;
	}
    }

  if(endBytes)
    {
      // trailing cruft to deal with
      // Go read 4 bytes from the backed up end address
      ret = ubicom32v3readMemory(endAddr, 1, (int *)cptr);

      for(i=0; i< endBytes; i++)
	{
	  cptr[3-i] = (unsigned char) *buffer++;
	}
    }

  // Send the data to target
  ret = ubicom32v3UltrafastWriteMemory(leadAddr, toWriteWords, (int *)sendBuffer);

  free (freeBuffer);
  return (leadBytes + midBytes + endBytes);
}

static int
ubicom32v3_fast_read_bytes (unsigned int address, char *buffer, unsigned int length)
{
  // split the transfer into lead, aligned middle and end
  unsigned truncBytes = (address & 0x3);
  unsigned truncAddress = address & ~0x3;
  unsigned int leadBytes, leadAddr, midBytes, midAddr, endBytes, endAddr, leadIndex;
  unsigned toReadWords;
  unsigned char  *recvBuffer;
  unsigned char *freeBuffer;
  unsigned char *cptr;
  int ret;
  unsigned int i;

  leadIndex = 0;
  if(truncBytes)
    {
      leadBytes = 4-truncBytes;
      if(leadBytes > length)
	leadBytes = length;
      leadIndex = truncBytes;
      length -= leadBytes;
      address = truncAddress + 4;
    }
  else
    leadBytes =0;

  leadAddr = truncAddress;

  endAddr = address + length;
  endBytes = endAddr & 0x3;
  endAddr &= ~0x3;

  midBytes = length - endBytes;
  midAddr = address;

  toReadWords = midBytes/4;

  if(leadBytes)
    toReadWords++;

  if(endBytes)
    toReadWords++;

  /* Allocate the recv buffer */
  recvBuffer = (unsigned char *)malloc(toReadWords*4);
  freeBuffer = recvBuffer;
  cptr = recvBuffer;

  ret = ubicom32v3UltraFastReadMemoryRaw(leadAddr, toReadWords, (int *)recvBuffer);

  // Now Swizzle the data out
  if(leadBytes)
    {
      for(i= 0;i< leadBytes; i++)
	{
	  *buffer++ = recvBuffer[leadIndex++];
	}
      recvBuffer += 4;
    }

  if(midBytes)
    {
      memcpy (buffer, recvBuffer, midBytes);
      recvBuffer += midBytes;
      buffer += midBytes;
    }

  if(endBytes)
    {
      for(i=0; i< endBytes; i++)
	{
	  *buffer ++ = recvBuffer[i];
	}
    }
  recvBuffer += 4;
  free (freeBuffer);

  return (leadBytes + midBytes + endBytes);
}

int ubicom32v3isp_initBackendToBootKernel( unsigned int downloader_len, int *downloader_data, unsigned int vma)
{
  int ret;
  char *verifybuf;
  unsigned int retlen;

  ret = ubicom32v3isp_reset();
  if (ret)
    {
      dongle_error ("Reset error");
      return ret;
    }

  /* Call ubicom32v3isp_bist to run bist. */
  ret = ubicom32v3isp_bist();
  if (ret)
    {
      dongle_error ("BIST error");
      return ret;
    }

  /* set up the fast downloader */
  ret = ubicom32v3isp_download_fast_transfer_code((vma + downloader_len+ 7) & ~3);
  if (ret)
    {
      dongle_error ("Download fast transfer code error");
      return ret;
    }

  /* transfer the .downloader section to the hardware. */
  retlen = ubicom32v3_fast_write_bytes(vma, (char *)downloader_data, downloader_len);
  if (retlen != downloader_len)
    {
      dongle_error ("Transfer .downloader failed");
      return -1;
    }

  /* allocate the verify buffer. */
  verifybuf = malloc(downloader_len);
  if (verifybuf == NULL)
    {
      dongle_error ("Downloader malloc failed");
      return -1;
    }

  /* read the data back into the verify buffer and do a memcmp */
  retlen = ubicom32v3_fast_read_bytes(vma, verifybuf, downloader_len);
  if (retlen != downloader_len)
    {
      dongle_error ("Re-read failed");
      return -1;
    }

  /* Do the buffer comparison. */
  if (memcmp (downloader_data, verifybuf, downloader_len) != 0)
    {
      dongle_error("Could not download .downloader section.");
      return -1;
    }

  /* Downloader is verified. */
  free(verifybuf);

  /* guarantee that thread0 will be alive when we give it control. */
  ret = ubicom32v3isp_make_thread0_alive();

  /* now transfer control to the .downloader section */
  vma += 12*4;

  ret = ubicom32v3writeRegisters(ubicom32v3currentThread, 34, (void *)&vma, 1);
  ret= ubicom32v3restartProcessor(1);

  if(ret)
    {
      /* Jump to PC failed */
      char buffer[180];
      sprintf(buffer, "Failed to revector the debugger thread to 0x%x.\n", vma);
      dongle_error(buffer);
      return -1;
    }

  /* sleep for about .2 seconds */
  usleep(200000);

  /* Before we can go ahead wakeup the downloader */
  ret = ubicom32v3isp_wakeup_downloader();
  if(ret)
    {
      dongle_error("Downloader is dead..");
      return -1;
    }

  return 0;
}

int
ubicom32v3_write_flash_bytes (unsigned int address, char *buffer, unsigned int length)
{
  // split the transfer into lead, aligned middle and end
  unsigned truncBytes = (address & 0x3);
  unsigned truncAddress = address & ~0x3;
  unsigned int leadBytes, leadAddr, midBytes, midAddr, endBytes, endAddr, leadIndex;
  unsigned toWriteWords;
  unsigned char  *sendBuffer;
  unsigned char *freeBuffer;
  unsigned char *cptr;
  unsigned int i;
  int  j, ret;

  leadBytes =0;
  leadIndex = 0;
  if(truncBytes)
    {
      leadBytes = 4-truncBytes;
      if(leadBytes > length)
	leadBytes = length;
      leadIndex = 3-truncBytes;
      length -= leadBytes;
      address = truncAddress + 4;
    }

  leadAddr = truncAddress;

  endAddr = address + length;
  endBytes = endAddr & 0x3;
  endAddr &= ~0x3;

  midBytes = length - endBytes;
  midAddr = address;

  toWriteWords = midBytes/4;

  if(leadBytes)
    toWriteWords++;

  if(endBytes)
    toWriteWords++;

  /* Allocate the send buffer */
  sendBuffer = (unsigned char *)malloc(toWriteWords*4);
  freeBuffer = sendBuffer;
  cptr = sendBuffer;

  if(leadBytes)
    {
      // Misaligned start. Deal with it by first backing up the address to nearest
      // Go read 4 bytes from the backed up address
      ret = readPgmMemory(leadAddr, 1, (int *)cptr);

      for(i=0; i<leadBytes; i++)
	{
	  cptr[leadIndex--] = (unsigned char) *buffer++;
	}

      // Bump up cptr by 4
      cptr += 4;
    }

  // Deal with the midsection if any
  if(midBytes)
    {
      // Move the data into the transfer buffer
      for(i=0; i< midBytes; i+= 4)
	{
	  for(j=0; j< 4; j++)
	    {
	      cptr[3-j] = (*buffer++ &0xff);
	    }
	  cptr+= 4;
	}
    }

  if(endBytes)
    {
      // trailing cruft to deal with
      // Go read 4 bytes from the backed up end address
      ret = readPgmMemory(endAddr, 1, (int *)cptr);

      for(i=0; i< endBytes; i++)
	{
	  cptr[3-i] = (unsigned char) *buffer++;
	}
    }

  // Send the data to target
  ret = writePgmMemory(leadAddr, toWriteWords, (int *)sendBuffer);

  free (freeBuffer);
  return (leadBytes + midBytes + endBytes);
}

int
ubicom32v3pollThreads(unsigned int threadNo, char *args)
{
  struct ubicom32command_packet packet;
  struct ubicom32command *command;
  int count, ret;
  int pcIndex;
  unsigned int reply;

  if(dongle_interface_status() == 0)
    {
      if (args == NULL)
	return -1;

      if(ubicom32v3isp_connect(args) == -1)
	return -1;

      debugOpen();
    }

  ubicom32command_packet_init(&packet);

  /* recover thread PC */
  count = ubicom32command_packet_append(&packet, 0, ISD_RD_MP_REG, (0xd0 | threadNo <<10));
  pcIndex = count;
  count = ubicom32command_packet_append(&packet, 0, ISD_RD_MP_REG, (0xd0 | threadNo <<10));
  count = ubicom32command_packet_append(&packet, 0, ISD_RD_MP_REG, (0xd0 | threadNo <<10));
  count = ubicom32command_packet_append(&packet, 0, ISD_RD_MP_REG, (0xd0 | threadNo <<10));
  count = ubicom32command_packet_append(&packet, 0, ISD_RD_MP_REG, (0xd0 | threadNo <<10));
  count = ubicom32command_packet_append(&packet, 0, ISD_RD_MP_REG, (0xd0 | threadNo <<10));
  count = ubicom32command_packet_append(&packet, 0, ISD_RD_MP_REG, (0xd0 | threadNo <<10));
  count = ubicom32command_packet_append(&packet, 0, ISD_RD_MP_REG, (0xd0 | threadNo <<10));
  count = ubicom32command_packet_append(&packet, 0, ISD_RD_MP_REG, (0xd0 | threadNo <<10));
  count = ubicom32command_packet_append(&packet, 0, ISD_RD_MP_REG, (0xd0 | threadNo <<10));
  count = ubicom32command_packet_append(&packet, 0, ISD_RD_MP_REG, (0xd0 | threadNo <<10));
  count = ubicom32command_packet_append(&packet, 0, ISD_RD_MP_REG, (0xd0 | threadNo <<10));
  count = ubicom32command_packet_append(&packet, 0, ISD_RD_MP_REG, (0xd0 | threadNo <<10));
  count = ubicom32command_packet_append(&packet, 0, ISD_RD_MP_REG, (0xd0 | threadNo <<10));
  count = ubicom32command_packet_append(&packet, 0, ISD_RD_MP_REG, (0xd0 | threadNo <<10));
  count = ubicom32command_packet_append(&packet, 0, ISD_RD_MP_REG, (0xd0 | threadNo <<10));
  count = ubicom32command_packet_append(&packet, 0, ISD_RD_MP_REG, (0xd0 | threadNo <<10));
  count = ubicom32command_packet_append(&packet, 0, ISD_RD_MP_REG, (0xd0 | threadNo <<10));
  count = ubicom32command_packet_append(&packet, 0, ISD_RD_MP_REG, (0xd0 | threadNo <<10));
  count = ubicom32command_packet_append(&packet, 0, ISD_RD_MP_REG, (0xd0 | threadNo <<10));
  count = ubicom32command_packet_append(&packet, 0, ISD_RD_MP_REG, (0xd0 | threadNo <<10));
  count = ubicom32command_packet_append(&packet, 0, ISD_RD_MP_REG, (0xd0 | threadNo <<10));
  count = ubicom32command_packet_append(&packet, 0, ISD_RD_MP_REG, (0xd0 | threadNo <<10));
  count = ubicom32command_packet_append(&packet, 0, ISD_RD_MP_REG, (0xd0 | threadNo <<10));
  count = ubicom32command_packet_append(&packet, 0, ISD_RD_MP_REG, (0xd0 | threadNo <<10));
  count = ubicom32command_packet_append(&packet, 0, ISD_RD_MP_REG, (0xd0 | threadNo <<10));
  count = ubicom32command_packet_append(&packet, 0, ISD_RD_MP_REG, (0xd0 | threadNo <<10));
  count = ubicom32command_packet_append(&packet, 0, ISD_NOP, 0);

  /* ship down the packet */
  ret = ubicom32command_packet_send(&packet);
  if(ret)
    return ret;

  while(pcIndex < count)
    {
      command = &packet.commands[pcIndex];

      // extract reply
      reply = (command->operand0<<24) | (command->operand1<<16) | (command->operand2<<8) | command->operand3;

      printf("Thread %d at 0x%x\n", threadNo, reply);

      pcIndex++;
    }

  return ret;
}
