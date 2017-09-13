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
#ifdef __cplusplus
extern "C" {
#endif

typedef struct bpReason {
  unsigned int reason;
  unsigned int status;		/* Used only for DCAPT error */
  unsigned int addr;		/* Contains thread pc for DCAPT, memory address for Parity error */
} bpReason_t;

typedef void (*EraseProgressCallback)(int total_length, int current_length, int block_size, void *param);
extern int initBackend(unsigned int);
extern int initBackendToBootKernel(unsigned int cpuType);
extern int readMailbox(int *data);
extern int debugClose(void);
extern int singleStep(unsigned int threadNo);
extern int stopProcessor(void);
extern int restartProcessor(void);
extern int writeRegisters( int threadNo, int regIndex, 
					 int *inBuf, unsigned int length);
extern int readRegisters( int threadNo, int regIndex, int *dest, 
			  unsigned int length);
extern int createTpacketMP(unsigned char *buffer, unsigned int threadNo);
extern int createGpacket(unsigned char *buffer, unsigned int cpuType);
extern int waitForBP(bpReason_t *reason);
extern int writePgmMemory(unsigned int destAddr, unsigned int len, int *data);
extern int readPgmMemory(unsigned int destAddr, unsigned int len, int *data);
extern int readPgmMemoryRaw(unsigned int destAddr, unsigned int len, int *data);
extern int crcPgmMemory(unsigned int destAddr, unsigned int len, int *data);
extern int writeDataMemory(unsigned int destAddr, unsigned int len, int *data);
extern int writeDataMemory(unsigned int destAddr, unsigned int len, int *data);
extern int readDataMemory(unsigned int destAddr, unsigned int len, int *data);
extern int readDataMemoryRaw(unsigned int destAddr, unsigned int len, int *data);
extern void closeDown(void);
extern void cleanupBackend(unsigned int cpuType);
extern int getDebuggerThreadNo(void);
extern int getFlashDetails(void);
extern int setDebuggerDontDebugMask(unsigned int mask);
extern void printTPacketMP(unsigned char *buffer, unsigned int threadNo);
extern int getDebuggerStaticBpAddrs(int *data, unsigned int numEntries);
extern int getDebuggerThreadPcs(int *data, unsigned int numEntries);
extern enum threadState threadStatus(unsigned int threadNo);
extern int ubicom32isp_connect(char *remote);
extern int ubicom32isp_close(void);
extern int ubicom32isp_detach(void);
extern int ubicom32isp_attach(void);
extern int ubicom32isp_attach(void);
extern int ubicom32isp_reset(void);
extern int compareVersion(unsigned char maj, unsigned char min, unsigned char st, 
			  unsigned char inc, unsigned int unk);
extern int debugOpen(void);
extern int jumpToPc(unsigned int addr);
extern int setupForDownload(void);
extern void retrieve_pending_packet(void);

extern int ubicom32_write_bytes (unsigned int address, char *buffer, unsigned int length);
extern int common_read_bytes_raw (unsigned int address, char *buffer, unsigned int length);
extern int common_verify_bytes (unsigned int address, char *buffer, unsigned int length);
typedef struct ubicom32MainPerThreadRegs {
  int dr[16];		// Data registers
  int ar[8];		// Address registers
  int mac_hi;
  int mac_lo;
  int mac_rc16;
  int source3;
  int inst_cnt;
  int csr;
  int rosr;
  int iread_data;
  int int_mask0;
  int int_mask1;
  int threadPc  ;
}ubicom32MainPerThreadRegs_t;

typedef struct ubicom32MainGlobalRegs {
  int chip_id;			// Read Only
  int int_stat0;		// Read Only
  int int_stat1;		// Read Only
  int int_set0;			// Write Only
  int int_set1;			// Write Only
  int int_clr0;			// Write Only
  int int_clr1;			// Write Only
  int global_ctrl;		// RW
  int mt_active_set;		// Write Only
  int mt_active_clr;		// Write Only
  int mt_active;		// Read Only
  int mt_dbg_active_set;	// Write Only
  int mt_dbg_active;		// Read Only
  int mt_en;			// RW
  int mt_hpri;			// RW
  int mt_hrt;			// RW
  int mt_break_clr;		// Write Only
  int mt_break;			// Read Only
  int mt_single_step;		// RW
  int mt_min_delay_en;		// RW
  int mt_debug_active_clr;	// Write Only
  int perr_addr;		// RW
  int dcapt_tnum;		// Read Only
  int dcapt_pc;			// Read Only
  int dcapt;			// RW
  int scratchpad0;		// RW
  int scratchpad1;		// RW
  int scratchpad2;		// RW
  int scratchpad3;		// RW
} ubicom32MainGlobalRegs_t;

typedef struct mercuryMainRegs {
  ubicom32MainPerThreadRegs_t tRegs[8];
  ubicom32MainGlobalRegs_t globals;
  unsigned int globalsRead;
  unsigned int perThreadRead[8];
} mercuryMainRegs_t;

extern mercuryMainRegs_t mainRegisters;

typedef struct ubicom32v3PerThreadRegs {
  int dr[16];		// Data registers
  int ar[8];		// Address registers
  int mac_hi;
  int mac_lo;
  int mac_rc16;
  int source3;
  int inst_cnt;
  int csr;
  int rosr;
  int iread_data;
  int int_mask0;
  int int_mask1;
  int threadPc;
  int trap_cause;
  int acc1_hi;
  int acc1_lo;
  int previous_pc;
}ubicom32v3PerThreadRegs_t;

#define NUM_PER_THREAD_REGS_UBI32V3 sizeof(ubicom32v3PerThreadRegs_t)/sizeof(int)

typedef struct ubicom32v3GlobalRegs {
  int chip_id;			// 39 Read Only
  int int_stat0;		// 40 Read Only
  int int_stat1;		// 41 Read Only
  int int_set0;			// 42 Write Only
  int int_set1;			// 43 Write Only
  int int_clr0;			// 44 Write Only
  int int_clr1;			// 45 Write Only
  int global_ctrl;		// 46 RW
  int mt_active;		// 47 Read Only
  int mt_active_set;		// 48 Write Only
  int mt_active_clr;		// 49 Write Only
  int mt_dbg_active;		// 50 Read Only
  int mt_dbg_active_set;	// 51 Write Only
  int mt_en;			// 52 RW
  int mt_hpri;			// 53 RW
  int mt_hrt;			// 54 RW
  int mt_break;			// 55 Read Only
  int mt_break_clr;		// 56 Write Only
  int mt_single_step;		// 57 RW
  int mt_min_delay_en;		// 58 RW
  int mt_break_set;		// 59 Write only
  int dcapt;			// 60 RW
  int mt_debug_active_clr;	// 61 Write Only
  int scratchpad0;		// 62 RW
  int scratchpad1;		// 63 RW
  int scratchpad2;		// 64 RW
  int scratchpad3;		// 65 RW
  int chip_cfg;			// 66 RW
  int mt_i_blocked;		// 67 RO
  int mt_d_blocked;		// 68 RO
  int mt_i_blocked_set;		// 69 WO
  int mt_d_blocked_set;		// 70 WO
  int mt_blocked_clr;		// 71 WO
  int mt_trap_en;		// 72 RW
  int mt_trap;			// 73 RO
  int mt_trap_set;		// 74 WO
  int mt_trap_clr;		// 75 WO
  int i_range0_hi;		// 76 RW
  int i_range1_hi;		// 77 RW
  int i_range2_hi;		// RW
  int i_range0_lo;		// RW
  int i_range1_lo;		// RW
  int i_range2_lo;		// RW
  int i_range0_en;		// RW
  int i_range1_en;		// RW
  int i_range2_en;		// RW
  int d_range0_hi;		// RW
  int d_range1_hi;		// RW
  int d_range2_hi;		// RW
  int d_range3_hi;		// RW
  int d_range0_lo;		// RW
  int d_range1_lo;		// RW
  int d_range2_lo;		// RW
  int d_range3_lo;		// RW
  int d_range0_en;		// RW
  int d_range1_en;		// RW
  int d_range2_en;		// RW
  int d_range3_en;		// 96 RW
} ubicom32v3GlobalRegs_t;

#define NUM_GLOBAL_REGS_UBI32V3 sizeof(ubicom32v3GlobalRegs_t)/sizeof(int)

typedef struct ubicom32v3Regs {
  ubicom32v3PerThreadRegs_t tRegs[12];
  ubicom32v3GlobalRegs_t globals;
  unsigned int globalsRead;
  unsigned int perThreadRead[12];
} ubicom32v3Regs_t;

extern ubicom32v3Regs_t ubicom32v3Registers;

typedef struct ubicom32v4GlobalRegs {
  int chip_id;			// 39 Read Only
  int int_stat0;		// 40 Read Only
  int int_stat1;		// 41 Read Only
  int int_set0;			// 42 Write Only
  int int_set1;			// 43 Write Only
  int int_clr0;			// 44 Write Only
  int int_clr1;			// 45 Write Only
  int global_ctrl;		// 46 RW
  int mt_active;		// 47 Read Only
  int mt_active_set;		// 48 Write Only
  int mt_active_clr;		// 49 Write Only
  int mt_dbg_active;		// 50 Read Only
  int mt_dbg_active_set;	// 51 Write Only
  int mt_en;			// 52 RW
  int mt_hpri;			// 53 RW
  int mt_hrt;			// 54 RW
  int mt_break;			// 55 Read Only
  int mt_break_clr;		// 56 Write Only
  int mt_single_step;		// 57 RW
  int mt_min_delay_en;		// 58 RW
  int mt_break_set;		// 59 Write only
  int dcapt;			// 60 RW
  int mt_debug_active_clr;	// 61 Write Only
  int scratchpad0;		// 62 RW
  int scratchpad1;		// 63 RW
  int scratchpad2;		// 64 RW
  int scratchpad3;		// 65 RW
  int chip_cfg;			// 66 RW
  int mt_i_blocked;		// 67 RO
  int mt_d_blocked;		// 68 RO
  int mt_i_blocked_set;		// 69 WO
  int mt_d_blocked_set;		// 70 WO
  int mt_blocked_clr;		// 71 WO
  int mt_trap_en;		// 72 RW
  int mt_trap;			// 73 RO
  int mt_trap_set;		// 74 WO
  int mt_trap_clr;		// 75 WO
  int i_range0_hi;		// 76 RW
  int i_range1_hi;		// 77 RW
  int i_range2_hi;		// RW
  int i_range3_hi;		// RW
  int i_range0_lo;		// 80 RW
  int i_range1_lo;		// RW
  int i_range2_lo;		// RW
  int i_range3_lo;		// RW
  int i_range0_en;		// 84 RW
  int i_range1_en;		// RW
  int i_range2_en;		// RW
  int i_range3_en;		// RW
  int d_range0_hi;		// 88 RW
  int d_range1_hi;		// RW
  int d_range2_hi;		// RW
  int d_range3_hi;		// RW
  int d_range4_hi;		// RW
  int d_range0_lo;		// 93 RW
  int d_range1_lo;		// RW
  int d_range2_lo;		// RW
  int d_range3_lo;		// RW
  int d_range4_lo;		// RW
  int d_range0_en;		// 98 RW
  int d_range1_en;		// RW
  int d_range2_en;		// RW
  int d_range3_en;		// RW
  int d_range4_en;		// 102 RW
} ubicom32v4GlobalRegs_t;

#define NUM_GLOBAL_REGS_UBI32V4 sizeof(ubicom32v4GlobalRegs_t)/sizeof(int)

typedef struct ubicom32v4Regs {
  ubicom32v3PerThreadRegs_t tRegs[12];
  ubicom32v4GlobalRegs_t globals;
  unsigned int globalsRead;
  unsigned int perThreadRead[12];
} ubicom32v4Regs_t;

extern ubicom32v4Regs_t ubicom32v4Registers;



extern int gdbMainGetallRegs(unsigned int threadNo, mercuryMainRegs_t *mainRegs);
extern int gdbMainGetTPacketRegs(int threadNo, mercuryMainRegs_t *mainRegs);
extern int eraseVerifyFlash(unsigned int startAddr, unsigned int verifyLength, EraseProgressCallback proc, void *param);

extern int hw_monitor(unsigned int addr, unsigned int needEvent,
		      unsigned int triggerEvent, unsigned int eventPattern, 
		      unsigned int eventMask,
		      unsigned int runCounter, unsigned int *resBuf, char *args);

extern int protectBootSector(void);
extern int unprotectBootSector(void);
extern unsigned int flashMID, flashType, flashSectorSize, flashSize;
#ifdef __cplusplus
}
#endif
