/* The ABIs for Blackfin.  */
enum ubicom32_abi
{
  UBICOM32_ABI_FLAT,
  UBICOM32_ABI_FDPIC,
};

/* Target-dependent structure in gdbarch.  */
struct gdbarch_tdep
{
  /* Which ABI is in use?  */
  const char *name;
  enum ubicom32_abi ubicom32_abi;
};

/* Stack alignment and mask for parameters and return values.  */
enum {
  UBICOM32_ALIGN      = 4,
  UBICOM32_ALIGN_MASK = (UBICOM32_ALIGN - 1)
};

/* Size of general data and address registers.  */
enum {
  UBICOM32_REG_SIZE   = 4
};

/* Size of an instruction.  */
enum {
  UBICOM32_INSN_SIZE  = 4
};

#define GDB_DMEM(addr)	(addr)
#define GDB_IMEM(addr)	(addr)
#define REAL_MEM(addr)	(addr)

/* Special register numbers.  */
enum ubicom32_gdb_regnum {
  UBICOM32_D0_REGNUM = 0,			/* general data */
  UBICOM32_D1_REGNUM = 1,			/* general data */
  UBICOM32_D2_REGNUM = 2,			/* general data */
  UBICOM32_D3_REGNUM = 3,			/* general data */
  UBICOM32_D4_REGNUM = 4,			/* general data */
  UBICOM32_D5_REGNUM = 5,			/* general data */
  UBICOM32_D6_REGNUM = 6,			/* general data */
  UBICOM32_D7_REGNUM = 7,			/* general data */
  UBICOM32_D8_REGNUM = 8,			/* general data */
  UBICOM32_D9_REGNUM = 9,			/* general data */
  UBICOM32_D10_REGNUM = 10,			/* general data */
  UBICOM32_D11_REGNUM = 11,			/* general data */
  UBICOM32_D12_REGNUM = 12,			/* general data */
  UBICOM32_D13_REGNUM = 13,			/* general data */
  UBICOM32_D14_REGNUM = 14,			/* general data */
  UBICOM32_D15_REGNUM = 15,			/* general data */
  UBICOM32_A0_REGNUM = 16,			/* general address */
  UBICOM32_A1_REGNUM = 17,			/* general address */
  UBICOM32_A2_REGNUM = 18,			/* general address */
  UBICOM32_A3_REGNUM = 19,			/* general address */
  UBICOM32_A4_REGNUM = 20,			/* general address */
  UBICOM32_A5_REGNUM = 21,			/* general address / link register */
  UBICOM32_A6_REGNUM = 22,			/* general address / frame pointer  */
  UBICOM32_A7_REGNUM = 23,			/* gerenal address / stack pointer */
  UBICOM32_SP_REGNUM = UBICOM32_A7_REGNUM,	/* stack pointer */
  UBICOM32_LR_REGNUM  = UBICOM32_A5_REGNUM,	/* link register (return address) */
  UBICOM32_PC_REGNUM = 34,			/* program counter */
  UBICOM32_RET_REGNUM = UBICOM32_D0_REGNUM,	/* return value */
  UBICOM32_RET2_REGNUM = UBICOM32_D1_REGNUM,	/* 2nd word of 8-byte return value */
  UBICOM32_SRET_REGNUM = UBICOM32_D0_REGNUM,	/* returned struct address */
  UBICOM32_ROSR_REGNUM = 30,			/* read-only status */
  UBICOM32_MT_ACTIVE_REGNUM	= 45,		/* threads active status */
  UBICOM32_MT_EN_REGNUM = 48,			/* threads enabled */
  UBICOM32_MT_PRI_REGNUM = 49,			/* thread priorities */
  UBICOM32_MT_SCHED_REGNUM = 50,		/* thread scheduling policies */
  UBICOM32_GDB_THREAD_REGNUM = 66,		/* change threads from GDB */
  UBICOM32_FP_REGNUM = 67,			/* frame pointer */

};

/* Special register memory locations.  */
enum {
  UBICOM32_D0_REGADDR = 0x0,
  UBICOM32_D1_REGADDR = 0x4,
  UBICOM32_D7_REGADDR = 0x1c,
  UBICOM32_D8_REGADDR = 0x20,
  UBICOM32_D9_REGADDR = 0x24,
  UBICOM32_D13_REGADDR = 0x34,
  UBICOM32_D14_REGADDR = 0x38,
  UBICOM32_A6_REGADDR = 0x98,
  UBICOM32_SP_REGADDR = 0x9c
};

/* ubicom32v3/v4 related */
/* Special register numbers.  */
enum ubicom32v3_gdb_regnum {
  UBICOM32V3_D0_REGNUM = 0,			/* general data */
  UBICOM32V3_D1_REGNUM = 1,			/* general data */
  UBICOM32V3_D2_REGNUM = 2,			/* general data */
  UBICOM32V3_D3_REGNUM = 3,			/* general data */
  UBICOM32V3_D4_REGNUM = 4,			/* general data */
  UBICOM32V3_D5_REGNUM = 5,			/* general data */
  UBICOM32V3_D6_REGNUM = 6,			/* general data */
  UBICOM32V3_D7_REGNUM = 7,			/* general data */
  UBICOM32V3_D8_REGNUM = 8,			/* general data */
  UBICOM32V3_D9_REGNUM = 9,			/* general data */
  UBICOM32V3_D10_REGNUM = 10,			/* general data */
  UBICOM32V3_D11_REGNUM = 11,			/* general data */
  UBICOM32V3_D12_REGNUM = 12,			/* general data */
  UBICOM32V3_D13_REGNUM = 13,			/* general data */
  UBICOM32V3_D14_REGNUM = 14,			/* general data */
  UBICOM32V3_D15_REGNUM = 15,			/* general data */
  UBICOM32V3_A0_REGNUM = 16,			/* general address */
  UBICOM32V3_A1_REGNUM = 17,			/* general address */
  UBICOM32V3_A2_REGNUM = 18,			/* general address */
  UBICOM32V3_A3_REGNUM = 19,			/* general address */
  UBICOM32V3_A4_REGNUM = 20,			/* general address */
  UBICOM32V3_A5_REGNUM = 21,			/* general address / link register */
  UBICOM32V3_A6_REGNUM = 22,			/* general address / frame pointer  */
  UBICOM32V3_A7_REGNUM = 23,			/* gerenal address / stack pointer */
  UBICOM32V3_SP_REGNUM = UBICOM32V3_A7_REGNUM,	/* stack pointer */
  UBICOM32V3_LR_REGNUM  = UBICOM32V3_A5_REGNUM,	/* link register (return address) */
  UBICOM32V3_PC_REGNUM = 34,			/* program counter */
  UBICOM32V3_RET_REGNUM = UBICOM32V3_D0_REGNUM,	/* return value */
  UBICOM32V3_RET2_REGNUM = UBICOM32V3_D1_REGNUM,/* 2nd word of 8-byte return value */
  UBICOM32V3_SRET_REGNUM = UBICOM32V3_D0_REGNUM,/* returned struct address */
  UBICOM32V3_ROSR_REGNUM = 30,			/* read-only status */
  UBICOM32V3_MT_ACTIVE_REGNUM	= 47,		/* threads active status */
  UBICOM32V3_MT_EN_REGNUM = 52,			/* threads enabled */
  UBICOM32V3_MT_PRI_REGNUM = 53,		/* thread priorities */
  UBICOM32V3_MT_SCHED_REGNUM = 54,		/* thread scheduling policies */
};

/* Special register memory locations.  */
enum {
  UBICOM32V3_D0_REGADDR = 0x0,
  UBICOM32V3_D1_REGADDR = 0x4,
  UBICOM32V3_D7_REGADDR = 0x1c,
  UBICOM32V3_D8_REGADDR = 0x20,
  UBICOM32V3_D14_REGADDR = 0x38,
  UBICOM32V3_A6_REGADDR = 0x98,
  UBICOM32V3_SP_REGADDR = 0x9c
};

/* Special registers used by uclinux */
enum ubicom32uclinux_gdb_regnum {
  UBICOM32_TEXT_ADDR_REGNUM = 37,		/* not a real regnum */
  UBICOM32_TEXT_ADDR_END_REGNUM = 38,
  UBICOM32_DATA_ADDR_REGNUM = 39,
  UBICOM32_FDPIC_LOADMAP_EXEC_REGNUM = 40,
  UBICOM32_FDPIC_LOADMAP_INTERP_REGNUM = 41,
};
/* Register flag bits.  */

enum {
  REG_HIDESOME = 0x01,		/* don't display in "info registers" */
  REG_RDONLY = 0x02,		/* register is read-only */
  REG_WRONLY = 0x04		/* register is write-only */
};


/* Fetch the interpreter and executable loadmap addresses (for shared
   library support) for the FDPIC ABI.  Return 0 if successful, -1 if
   not.  (E.g, -1 will be returned if the ABI isn't the FDPIC ABI.)  */
extern int ubicom32_fdpic_loadmap_addresses (struct gdbarch *gdbarch,
					 CORE_ADDR *interp_addr,
					 CORE_ADDR *exec_addr);

/* Given a function entry point, find and return the GOT address for the
   containing load module.  */
CORE_ADDR ubicom32_fdpic_find_global_pointer (CORE_ADDR addr);

/* Given a function entry point, find and return the canonical descriptor
   for that function, if one exists.  If no canonical descriptor could
   be found, return 0.  */
CORE_ADDR ubicom32_fdpic_find_canonical_descriptor (CORE_ADDR entry_point);

/* Given an objfile, return the address of its link map.  This value is
   needed for TLS support.  */
CORE_ADDR ubicom32_fetch_objfile_link_map (struct objfile *objfile);
