/* Remote target communications for serial-line targets in custom GDB protocol

   Copyright 1988, 1989, 1990, 1991, 1992, 1993, 1994, 1995, 1996,
   1997, 1998, 1999, 2000, 2001, 2002 Free Software Foundation, Inc.

   This file is part of GDB.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.  */

/* See the GDB User Guide for details of the GDB remote protocol. */

#include "defs.h"
#include "gdb_string.h"
#include <ctype.h>
#include <fcntl.h>
#include "inferior.h"
#include "bfd.h"
#include "symfile.h"
#include "target.h"
#include "gdbcmd.h"
#include "objfiles.h"
#include "gdb-stabs.h"
#include "gdbthread.h"
#include "remote.h"
#include "regcache.h"
#include "value.h"
#include "gdb_assert.h"
#include "readline/readline.h"

#include <ctype.h>
#include <sys/time.h>
#ifdef USG
#include <sys/types.h>
#endif

#include "event-loop.h"
#include "event-top.h"
#include "inf-loop.h"

#include <signal.h>
#include "serial.h"

#include "gdbcore.h" /* for exec_bfd */
#include "remote-ubicom32-isd.h"
#include "remote-ubicom32-dbginterface.h"
#include "remote-ubicom32v3-dbginterface.h"
#include "ubicom32_dongle.h"
#include "elf-bfd.h"
#include "ubicom32-tdep.h"

#define DO_CORE_DUMPS 1

volatile int ubicom32v3_cntrl_c_abort;
int ubicom32v3_current_thread= -1;
int ubicom32v3currentThread=-1;
unsigned int dcache_status = 0;
static void ubicom32v3_read_dcache_status(void);

enum ubicom32v3_state remote_ubicom32v3_state = UBICOM32V3_DETACHED;

struct step_interpret {
  unsigned int current_pc;
  unsigned int target1_pc;
  unsigned int target2_pc;
  unsigned int address_register;
  unsigned int stepping_thread;
};

struct step_interpret stepinterpret;
extern unsigned int single_step_buffer_address;
static int ubicom32v3_write_bytes (CORE_ADDR address, char *buffer, int length);

/* GDB structure.  */
struct target_ops ubicom32v3_ops;
struct target_ops ubicom32v3_core_ops;

#define UBICOM32V3_FLASH_ADDR 0x60000000
#define UBICOM32V3_FLASH_END	0x6ffffffc
#define UBICOM32V3_LOAD_BLOCK_SIZE 2048

#define UBICOM32V3_DDR_ADDR 0x40000000
#define UBICOM32V3_DDR_END  0x47fffffc

#define UBICOM32V3_OCM_ADDR 0x3ffc0000
#define UBICOM32V3_OCM_END  0x3ffffffc

#define UBICOM32V3_FLASH_SECTION 1
#define UBICOM32V3_DDR_SECTION 2
#define UBICOM32V3_OCM_SECTION 3

#define REGISTER_RAW_SIZE(x)  (4)

void ubicom32v3_supply_registers(struct ubicom32_cpu *cpu, int threadNo, void *regcache);
void ubicom32v4_supply_registers(struct ubicom32_cpu *cpu, int threadNo, void *regcache);
void ubicom32v3_tpacket_supply_registers(struct ubicom32_cpu *cpu, int threadNo);
void ubicom32v4_tpacket_supply_registers(struct ubicom32_cpu *cpu, int threadNo);

static int ignore_trap = 0;

/* ubicom32v3_loadable_section.  */
static int
ubicom32v3_loadable_section(bfd *loadfile_bfd, asection *s)
{
  int addr, length;

  if (!(s->flags & SEC_LOAD))
    {
      if (strncmp(s->name, ".image", strlen(".image")))
	return 0;
    }

  length = bfd_get_section_size (s);
  if (length == 0)
    return 0;

  if ((s->lma >= UBICOM32V3_FLASH_ADDR) && (s->lma + length <= UBICOM32V3_FLASH_END))
    return UBICOM32V3_FLASH_SECTION;
  if ((s->lma >= UBICOM32V3_DDR_ADDR) && (s->lma + length <= UBICOM32V3_DDR_END))
    return UBICOM32V3_DDR_SECTION;
  if ((s->lma >= UBICOM32V3_OCM_ADDR) && (s->lma + length <= UBICOM32V3_OCM_END))
    return UBICOM32V3_OCM_SECTION;
  return 0;
}

/* ubicom32v3_lookup_symbol.  */
static unsigned int
ubicom32v3_lookup_symbol (char *name)
{
  struct symbol *sym;
  struct minimal_symbol *msym;

  sym = lookup_symbol (name, NULL, VAR_DOMAIN, NULL, NULL);
  if (sym)
    return SYMBOL_VALUE (sym);

  msym = lookup_minimal_symbol (name, NULL, NULL);
  if (msym)
    return SYMBOL_VALUE (msym);

  return 0;
}

static void
swizzle_for_write_register (int regnum, const void *val)
{
  int i, j;
  char * value = (char *)malloc(REGISTER_RAW_SIZE (regnum));
  char *ptr = (char *)val;

  for(i= (REGISTER_RAW_SIZE (regnum)) -1, j=0;i >= 0; i--)
    {
      value[j++] = ptr[i];
    }

  /* copy the data back into the buffer */
  for(i= (REGISTER_RAW_SIZE (regnum)) -1, j=0;i >= 0; i--)
    {
      ptr[i] = value[i];
    }

  free(value);
}


static void
swizzled_supply_register (struct regcache *regcache, int regnum, const void *val)
{
  int i, j;
  char * value = (char *)malloc(REGISTER_RAW_SIZE (regnum));
  char *ptr = (char *)val;

  for(i= (REGISTER_RAW_SIZE (regnum)) -1, j=0;i >= 0; i--)
    {
      value[j++] = ptr[i];
    }

  regcache_raw_supply(regcache, regnum, (const void *)value);
  free(value);
}

static void
ubicom32v3_set_thread (int th)
{
  if(th > 0)
    {
      if(remote_ubicom32v3_state == UBICOM32V3_CORE_STOPPED)
	{
	  /*
	   * Looks like the core is being analyzed. Fake it out.
	   */
	  ubicom32v3_current_thread = th;
	  ubicom32v3currentThread = th -1;
	  return;
	}

      /*
       * Normal live operations.
       */
      if(th != ubicom32v3_current_thread)
	{
	  if(ubicom32v3_current_thread != -1)
	    {
	      /* restore the current debugger thread back to original state */
	      (void) ubicom32v3restoreDebuggerThread(1);
	    }
	  ubicom32v3setupDebuggerThread(th -1);
	}
    }
}

static int
ubicom32v3_thread_alive (ptid_t ptid)
{
  return (1);
}

#define MAGIC_NULL_PID 42000

static char *ubi32cpus[] = {"ubicom32v2", "ubicom32v3", "ubicom32v4"};

/* ubicom32v3_open
   Usage: target remote <ip_addr>:<ip_port>
   Connects to the dongle and then connects to target through the dongle.  */
void
ubicom32v3_open (char *args, int from_tty)
{
  int ret, i;
  struct gdbarch_info info;
  extern void gdbarch_info_init (struct gdbarch_info *info);
  char *env_dongle;
  extern int chipID;
  char *file;
  int elf_flags = -1;

  if (exec_bfd)
    {
      elf_flags = elf_elfheader (exec_bfd)->e_flags & 0xffff;
    }

  if (remote_debug)
    {
      if (args)
	printf_filtered ("ubicom32v3_open (%s)\n", args);
      else
	printf_filtered ("ubicom32v3_open ()\n");
    }

  target_preopen (from_tty);

  // printf("Current arch is %s\n", TARGET_ARCHITECTURE->printable_name);

  /* Default device.  */
  if (!args)
    {
      env_dongle = getenv("UBICOM_DONGLE");
      if (env_dongle)
	args = env_dongle;
      else
	args = "localhost:5010";
    }

  /* Connect to dongle.  */
  ret = ubicom32v3isp_connect (args);

  if(ret == -1)
    error ("Unable to find dongle.");
  else if(ret == -2)
    error ("Dongle has no Ubicom32v3 support. Get a new dongle with Ubicom32v3 support and try again.");


  /*
   * control c turnOffconsole in ubicom32v3_open()
   */
  ret = turnOffconsole();

  /* Attach to target.  */
  ret = ubicom32v3isp_attach ();
  if(ret)
    {
      (void)ubicom32v3isp_close();
      remote_ubicom32v3_state = UBICOM32V3_DETACHED;
      switch(ret)
	{
	case -1:
	  error("Error in reply packets from dongle\n");
	  break;
	case -2:
	  error("Found Ubicom32 processor and not Ubicom32v3 processor\n");
	  break;
	case -3:
	  error("Unable to establish ISD connection\n");
	  break;
	default:
	  error("Unable to connect to target\n");
	  break;
	}
    }

  switch (chipID)
    {
    case 0x20000:
    case 0x20001:
    case 0x20002:
      current_cpu = &ubicom32_cpus[0];
      current_cpu->supply_register_fn = ubicom32v3_supply_registers;
      current_cpu->supply_tpacket_regs_fn = ubicom32v3_tpacket_supply_registers;

      if (elf_flags != -1)
	{
	  if (elf_flags != bfd_mach_ubicom32dsp)
	    {
	      char *file = bfd_get_filename (exec_bfd);
	      char *slash_location = strrchr(file, '/');

	      if (slash_location == NULL)
		slash_location = file;
	      else
		slash_location++;

	      printf_unfiltered ("\nWarning Mismatch:\n\tFile \"%s\" has been built for %s.\n\tTarget cpu is ubicom32v3.\n\n", slash_location, ubi32cpus[elf_flags]);
	    }
	}

      break;
    case 0x30000:
    case 0x30001:
      current_cpu = &ubicom32_cpus[1];
      current_cpu->supply_register_fn = ubicom32v4_supply_registers;
      current_cpu->supply_tpacket_regs_fn = ubicom32v4_tpacket_supply_registers;

      if (elf_flags != -1)
	{
	  if (elf_flags != bfd_mach_ubicom32ver4)
	    {
	      char *file = bfd_get_filename (exec_bfd);
	      char *slash_location = strrchr(file, '/');

	      if (slash_location == NULL)
		slash_location = file;
	      else
		slash_location++;

	      printf_unfiltered ("\nWarning Mismatch:\n\tFile \"%s\" has been built for %s.\n\tTarget cpu is ubicom32v4.\n\n", slash_location, ubi32cpus[elf_flags]);
	    }
	}

      break;
    default:
      error("No support for Processor with chip ID 0x%x in this version of the debugger.", chipID);
    }

  if (current_gdbarch == NULL)
    {
      char *ptr = NULL;
      gdbarch_info_init (&info);
      switch (chipID)
	{
	case 0x20000:
	case 0x20001:
	case 0x20002:
	  ptr = "ubicom32dsp";
	  break;
	case 0x30000:
	case 0x30001:
	  ptr = "ubicom32ver4";
	  break;
	}
      info.bfd_arch_info = bfd_scan_arch (ptr);

      if (info.bfd_arch_info == NULL)
	internal_error (__FILE__, __LINE__,
			"set_architecture: bfd_scan_arch failed");
      (void)gdbarch_update_p (info);
    }
  else
    {
      struct gdbarch_tdep *tdep = gdbarch_tdep(current_gdbarch);
      char *arch_string = (char *)tdep->name;
      switch (chipID)
	{
	case 0x20000:
	case 0x20001:
	case 0x20002:
	  if (strcmp(arch_string, "Ubicom32 Version 3"))
	    {
	      /* Architecture mismatch. We need to go find the "ubicom32dsp" arch. */
	      gdbarch_info_init (&info);
	      info.bfd_arch_info = bfd_scan_arch ("ubicom32dsp");

	      if (info.bfd_arch_info == NULL)
		internal_error (__FILE__, __LINE__,
				"set_architecture: bfd_scan_arch failed");
	      (void)gdbarch_update_p (info);
	    }
	  break;
	case 0x30000:
	case 0x30001:
	  if (strcmp(arch_string, "Ubicom32 Version 4"))
	    {
	      /* Architecture mismatch. We need to go find the "ubicom32ver4" arch. */
	      gdbarch_info_init (&info);
	      info.bfd_arch_info = bfd_scan_arch ("ubicom32ver4");

	      if (info.bfd_arch_info == NULL)
		internal_error (__FILE__, __LINE__,
				"set_architecture: bfd_scan_arch failed");
	      (void)gdbarch_update_p (info);
	    }
	  break;
	}
    }
  push_target (&ubicom32v3_ops);		/* Switch to using remote target now */

  remote_ubicom32v3_state = UBICOM32V3_STOPPED;
  inferior_ptid = pid_to_ptid (ubicom32v3_current_thread);
  start_remote(from_tty);
  //inferior_ptid = pid_to_ptid (1);

  /* add the threads to the system */
  for(i=1; i<= current_cpu->num_threads; i++)
    add_thread(pid_to_ptid(i));

  ubicom32v3_read_dcache_status();
}

#ifdef DO_CORE_DUMPS
struct ubicom32v3_core_memory {
  unsigned int ocm_size;
  unsigned char *ocm_mem;
  unsigned int ddr_size;
  unsigned char *ddr_mem;
  unsigned int flash_size;
  unsigned char *flash_mem;
  unsigned char *hrts;
} ubicom32v3_core;


/* ubicom32v3_close.  */
static void
ubicom32v3_core_close (int quitting)
{
  if (remote_debug)
    printf_filtered ("ubicom32v3_core_close (%d)\n", quitting);

  /* Disconnect.  */
  if(ubicom32v3_core.ocm_mem)
    free(ubicom32v3_core.ocm_mem);
  ubicom32v3_core.ocm_size = 0;
  ubicom32v3_core.ocm_mem = NULL;

  if(ubicom32v3_core.ddr_mem)
    free(ubicom32v3_core.ddr_mem);
  ubicom32v3_core.ddr_size = 0;
  ubicom32v3_core.ddr_mem = NULL;

  if(ubicom32v3_core.flash_mem)
    free(ubicom32v3_core.flash_mem);
  ubicom32v3_core.flash_size = 0;
  ubicom32v3_core.flash_mem = NULL;

  if(ubicom32v3_core.hrts)
    free(ubicom32v3_core.hrts);
  ubicom32v3_core.hrts = NULL;

  remote_ubicom32v3_state = UBICOM32V3_DETACHED;
}
#endif

void
ubicom32v3_supply_registers(struct ubicom32_cpu *cpu, int threadNo, void *rcache)
{
  ubicom32v3Regs_t *mainRegs = (ubicom32v3Regs_t *)cpu->registers;
  int *rptr = &mainRegs->tRegs[threadNo].dr[0];
  int i;
  struct regcache *regcache = (struct regcache *)rcache;
  /* first supply the thread specific registers */
  for(i=0; i< cpu->num_per_thread_regs; i++)
    swizzled_supply_register(regcache, i, rptr++);

  /* Now supply the global registers */
  rptr = (int *)&mainRegs->globals;
  for(;i< (cpu->num_per_thread_regs + cpu->num_global_regs); i++)
    swizzled_supply_register(regcache, i, rptr++);
}

void
ubicom32v4_supply_registers(struct ubicom32_cpu *cpu, int threadNo, void *rcache)
{
  ubicom32v4Regs_t *mainRegs = (ubicom32v4Regs_t *)cpu->registers;
  int *rptr = &mainRegs->tRegs[threadNo].dr[0];
  int i;
  struct regcache *regcache = (struct regcache *)rcache;

  /* first supply the thread specific registers */
  for(i=0; i< cpu->num_per_thread_regs; i++)
    swizzled_supply_register(regcache, i, rptr++);

  /* Now supply the global registers */
  rptr = (int *)&mainRegs->globals;
  for(;i< (cpu->num_per_thread_regs + cpu->num_global_regs); i++)
    swizzled_supply_register(regcache, i, rptr++);
}

/* ubicom32v3_fetch_register.  */
static void
ubicom32v3_fetch_register (struct regcache *regcache, int regno)
{
  int i, ret;
  int thread_num = PIDGET (inferior_ptid);

  ubicom32v3_set_thread (thread_num);

  /* grab registers */
  ret = current_cpu->fetch_registers_fn(current_cpu, (thread_num-1));
  if(ret)
    {
      printf_unfiltered("error: Fetch registers for thread %d failed\n", thread_num-1);
      return;
    }

  /* we have the registers. Now supply them */
  current_cpu->supply_register_fn(current_cpu, thread_num-1, regcache);
}

struct ubicom32v3_core_header {
  unsigned int magic;
  unsigned int regOffset;
  unsigned int hrtOffset;
  unsigned int ocmOffset;
  unsigned int ddrOffset;
  unsigned int flashOffset;
};

struct ubicom32v3_cache_dump_header {
  unsigned int dcache_tag_entries;
  unsigned int dcache_data_bytes;
  unsigned int icache_tag_entries;
  unsigned int icache_data_bytes;
};

#ifdef DO_CORE_DUMPS
/* ubicom32v3_core_open
   Usage: ubicom32-elf-gdb elf-filename core-fliename. */
void
ubicom32v3_core_open (char *args, int from_tty)
{
  int ret, i;
  FILE *fp;
  struct ubicom32v3_core_header coreHeader;
  char *file;
  bfd *loadfile_bfd;
  int registerSize;
  asection *s;
  CORE_ADDR dyn_begin;
  unsigned int dyn_length;
  char *ptr = NULL;
  extern int chipID;

  struct gdbarch_info info;
  extern void gdbarch_info_init (struct gdbarch_info *info);

  ubicom32v3_current_thread = 1;
  if (remote_debug)
    {
      if (args)
	printf_filtered ("ubicom32v3_core_open (%s)\n", args);
      else
	printf_filtered ("ubicom32v3_core_open ()\n");
    }

  target_preopen (from_tty);

  remote_ubicom32v3_state = UBICOM32V3_DETACHED;

  /* Open and read in the core file. */
  fp = fopen(args, "rb");

  if(fp == NULL)
    {
      printf_unfiltered("Unable to open core file %s\n", args);
      return;
    }

  /* go retreive the core header from the file */
  (void) fread(&coreHeader, 1, sizeof(coreHeader), fp);

  if(coreHeader.magic != 0x123455aa && coreHeader.magic != 0x123455ab)
    {
      printf_unfiltered("%s is not a Ubicom core file. Incorrect magic number.\n", args);
      fclose(fp);
      return;
    }

  /* run through the cpu list and figure out what type of cpu this core dump belongs to. */
  current_cpu = ubicom32_cpus;
  while (current_cpu->registers != NULL && coreHeader.magic != current_cpu->core_magic)
    {
      current_cpu++;
    }

  if (current_cpu->registers == NULL)
    {
      printf_unfiltered("%s is a core file for an unknown cpu.\n", args);
      fclose(fp);
      return;
    }

  printf_unfiltered("Reading machine state from %s\n", args);

  /* read in the debuggerThreadNo */
  (void) fread(&ubicom32v3currentThread, 1, 4, fp);

  ubicom32v3_current_thread = ubicom32v3currentThread+1;

  /* go retrieve the flash contents from the bfd file */
  file = get_exec_file(1);

  loadfile_bfd = bfd_openr (file, gnutarget);

  if (loadfile_bfd == NULL)
    error ("Error: Unable to open file %s\n", file);

  if (!bfd_check_format (loadfile_bfd, bfd_object))
    {
      bfd_close (loadfile_bfd);
      error ("Error: File is not an object file\n");
    }

  /* read in size of the register area */
  (void) fread(&registerSize, 1, 4, fp);

  if (registerSize != current_cpu->reg_area_size && coreHeader.magic == 0x123455aa)
    {
      /* We are reading an older core. We will read it in piece by piece. */
      fread(&ubicom32v3Registers.tRegs[0], 10, sizeof(ubicom32v3PerThreadRegs_t), fp);

      /* Now read in the globals */
      fread(&ubicom32v3Registers.globals, 1, (sizeof(ubicom32v3GlobalRegs_t) + 11*4), fp);
    }
  else
    {
      /* read in register data from the core */
      (void) fread(current_cpu->registers, 1, current_cpu->reg_area_size, fp);
    }

  /* malloc space for hrts and read in the data */
  ubicom32v3_core.hrts = malloc(512);
  if(ubicom32v3_core.hrts == NULL)
    {
      printf_unfiltered("Could not allocate space to represent hrts\n");
      ubicom32v3_core_close(1);
      return;
    }

  /* read in hrt data */
  (void) fread(ubicom32v3_core.hrts, 1, 512, fp);

  /*read ocm memory size */
  (void) fread(&ubicom32v3_core.ocm_size, 1, 4, fp);

  /* malloc space for it */
  ubicom32v3_core.ocm_mem = malloc(ubicom32v3_core.ocm_size);
  if(ubicom32v3_core.ocm_mem == NULL)
    {
      printf_unfiltered("Could not allocate space to represent ocm memory\n");
      ubicom32v3_core_close(1);
      return;
    }

  /*read ocm memory */
  (void) fread(ubicom32v3_core.ocm_mem, 1, ubicom32v3_core.ocm_size, fp);

  /*read ddr memory size */
  (void) fread(&ubicom32v3_core.ddr_size, 1, 4, fp);

  /* malloc space for it */
  ubicom32v3_core.ddr_mem = malloc(ubicom32v3_core.ddr_size);
  if(ubicom32v3_core.ddr_mem == NULL)
    {
      printf_unfiltered("Could not allocate space to represent ddr memory\n");
      ubicom32v3_core_close(1);
      return;
    }

  /*read ddr memory */
  (void) fread(ubicom32v3_core.ddr_mem, 1, ubicom32v3_core.ddr_size, fp);

  /*read flash memory size */
  (void) fread(&ubicom32v3_core.flash_size, 1, 4, fp);

  /* malloc space for it */
  ubicom32v3_core.flash_mem = malloc(ubicom32v3_core.flash_size);
  if(ubicom32v3_core.flash_mem == NULL)
    {
      printf_unfiltered("Could not allocate space to represent flash memory\n");
      ubicom32v3_core_close(1);
      return;
    }

  /*read flash memory */
  (void) fread(ubicom32v3_core.flash_mem, 1, ubicom32v3_core.flash_size, fp);

  fclose(fp);

  /* Set the Proper GDB arch */
  if (current_cpu->core_magic == 0x123455aa)
    {
      chipID = ubicom32v3Registers.globals.chip_id;
    }
  else if (current_cpu->core_magic == 0x123455ab)
    {
      chipID = ubicom32v4Registers.globals.chip_id;
    }
  gdbarch_info_init (&info);
  switch (chipID)
    {
    case 0x20000:
    case 0x20001:
    case 0x20002:
      ptr = "ubicom32dsp";
      current_cpu->supply_register_fn = ubicom32v3_supply_registers;
      current_cpu->supply_tpacket_regs_fn = ubicom32v3_tpacket_supply_registers;
      break;
    case 0x30000:
    case 0x30001:
      ptr = "ubicom32ver4";
      current_cpu->supply_register_fn = ubicom32v4_supply_registers;
      current_cpu->supply_tpacket_regs_fn = ubicom32v4_tpacket_supply_registers;
      break;
    }
  info.bfd_arch_info = bfd_scan_arch (ptr);

  if (info.bfd_arch_info == NULL)
    internal_error (__FILE__, __LINE__,
		    "set_architecture: bfd_scan_arch failed");
  (void)gdbarch_update_p (info);
  push_target (&ubicom32v3_core_ops);		/* Switch to using remote target now */

  remote_ubicom32v3_state = UBICOM32V3_CORE_STOPPED;
  inferior_ptid = pid_to_ptid (ubicom32v3_current_thread);
  start_remote(from_tty);

  /* add the threads to the system */
  for(i=1; i<= current_cpu->num_threads; i++)
    add_thread(pid_to_ptid(i));

  ubicom32v3_set_thread(ubicom32v3currentThread +1);
}
#endif

/* ubicom32v3_attach */
static void
ubicom32v3_attach (char *args, int from_tty)
{
  struct gdbarch_info info;
  extern void gdbarch_info_init (struct gdbarch_info *info);
  int ret = ubicom32v3isp_attach ();

  if(ret)
    {
      switch(ret)
	{
	case -1:
	  error("Error in reply packets from dongle\n");
	  break;
	case -2:
	  error("Found Ubicom32 processor and not Ubicom32v3 processor\n");
	  break;
	case -3:
	  error("Unable to establish ISD connection\n");
	  break;
	default:
	  error("Unable to connect to target\n");
	  break;
	}
      (void)ubicom32v3isp_close();
      remote_ubicom32v3_state = UBICOM32V3_DETACHED;
    }

  remote_ubicom32v3_state = UBICOM32V3_STOPPED;

  if (current_gdbarch == NULL)
    {
      info.bfd_arch_info = bfd_scan_arch ("ubicom32dsp");

      if (info.bfd_arch_info == NULL)
	internal_error (__FILE__, __LINE__,
			"set_architecture: bfd_scan_arch failed");
      (void)gdbarch_update_p (info);
    }
  else
    {
      struct gdbarch_tdep *tdep = gdbarch_tdep(current_gdbarch);
      char *arch_string = (char *)tdep->name;
      extern int chipID;
      switch (chipID)
	{
	case 0x20000:
	case 0x20001:
	case 0x20002:
	  if (strcmp(arch_string, "Ubicom32 Version 3"))
	    {
	      /* Architecture mismatch. We need to go find the "ubicom32dsp" arch. */
	      gdbarch_info_init (&info);
	      info.bfd_arch_info = bfd_scan_arch ("ubicom32dsp");

	      if (info.bfd_arch_info == NULL)
		internal_error (__FILE__, __LINE__,
				"set_architecture: bfd_scan_arch failed");
	      (void)gdbarch_update_p (info);
	    }
	  break;
	}
    }

  ubicom32v3_read_dcache_status();
}

#ifdef DO_CORE_DUMPS
/* ubicom32v3_core_detach.
   Detach from core.  */
static void
ubicom32v3_core_detach (char *args, int from_tty)
{
  (void) ubicom32v3_core_close(1);
  pop_target();
}
#endif

/* ubicom32v3_detach.
   Detach from the remote board.  */
static void
ubicom32v3_detach (char *args, int from_tty)
{
  int ret;
  // toggleMtEn();

  (void) ubicom32v3isp_detach();
  (void) ubicom32v3isp_close();
  remote_ubicom32v3_state = UBICOM32V3_DETACHED;
  pop_target();

}

/* ubicom32v3_close.  */
static void
ubicom32v3_close (int quitting)
{
  if(remote_ubicom32v3_state == UBICOM32V3_DETACHED)
    return;

  if (remote_debug)
    printf_filtered ("ubicom32v3_close (%d)\n", quitting);

  /* Disconnect.  */
  (void)ubicom32v3isp_close();
  remote_ubicom32v3_state = UBICOM32V3_DETACHED;
}

/* ubicom32v3_cntrl_c.  */
static void
ubicom32v3_cntrl_c (int signo)
{
  ubicom32v3_cntrl_c_abort = 1;
}

/* ubicom32v3_files_info.  */
static void
ubicom32v3_files_info (struct target_ops *target)
{
  if (exec_bfd)
    {
      char *file = bfd_get_filename (exec_bfd);
      printf_unfiltered ("Debugging %s\n", file);
    }
  else
    printf_unfiltered ("No file loaded\n");
}

/* ubicom32v3_stop.
   Notify the target of an asynchronous request to stop.  */
static void
ubicom32v3_stop (void)
{
  if (remote_debug)
    printf_filtered ("ubicom32v3_stop\n");

  ubicom32v3_cntrl_c_abort = 1;
}

static char *
ubicom32v3_thread_pid_to_str (ptid_t ptid)
{
  static char buf[30];

  sprintf (buf, "Thread %d", (PIDGET (ptid))-1);
  return buf;
}

static void
ubicom32v3_store_register(struct regcache *regcache, int regnum)
{
  char *regs = malloc(REGISTER_RAW_SIZE(regnum));
  int thread_num = PIDGET (inferior_ptid);
  int ret;

  ubicom32v3_set_thread(thread_num);

  if(regnum >= 0)
    {
      /* pull the data from regcache */
      regcache_raw_collect (regcache, regnum, regs);

      swizzle_for_write_register (regnum, regs);
      ret = current_cpu->write_register_fn((thread_num-1), regnum, (int *) regs, 1);
      if(ret)
	{
	  printf_unfiltered("error: Write to register %d in thread %d failed\n",
			    regnum, thread_num-1);
	}
    }
  else
    {
      printf_unfiltered("error: Write of all registers not supported\n");
    }
}

/* Prepare to store registers.  Since we may send them all
   we have to read out the ones we don't want to change
   first.  */

static void
ubicom32v3_prepare_to_store (struct regcache *regcache)
{
  int i;
  gdb_byte buf[MAX_REGISTER_SIZE];

  /* Make sure all the necessary registers are cached.  */
  for (i = 0; i < gdbarch_num_regs(current_gdbarch); i++)
    regcache_raw_read (regcache, i, buf);
}

#ifdef DO_CORE_DUMPS
#if 0
/* ubicom32v3_core_wait */
static ptid_t
ubicom32v3_core_wait (ptid_t ptid, struct target_waitstatus *status)
{
  return pid_to_ptid (ubicom32v3_current_thread);
}
#endif
#endif

void
ubicom32v3_tpacket_supply_registers(struct ubicom32_cpu *cpu, int threadNo)
{
  ubicom32v3Regs_t *mainRegs = (ubicom32v3Regs_t *)cpu->registers;
  struct regcache *current_regcache = get_current_regcache();
  swizzled_supply_register(current_regcache, 0, &mainRegs->tRegs[ubicom32v3currentThread].dr[0]);
  swizzled_supply_register(current_regcache, 21, &mainRegs->tRegs[ubicom32v3currentThread].ar[5]);
  swizzled_supply_register(current_regcache, 22, &mainRegs->tRegs[ubicom32v3currentThread].ar[6]);
  swizzled_supply_register(current_regcache, 23, &mainRegs->tRegs[ubicom32v3currentThread].ar[7]);
  swizzled_supply_register(current_regcache, 30, &mainRegs->tRegs[ubicom32v3currentThread].rosr);
  swizzled_supply_register(current_regcache, 34, &mainRegs->tRegs[ubicom32v3currentThread].threadPc);
  swizzled_supply_register(current_regcache, 47, &mainRegs->globals.mt_active);
  swizzled_supply_register(current_regcache, 50, &mainRegs->globals.mt_dbg_active);
  swizzled_supply_register(current_regcache, 52, &mainRegs->globals.mt_en);
}

void
ubicom32v4_tpacket_supply_registers(struct ubicom32_cpu *cpu, int threadNo)
{
  ubicom32v4Regs_t *mainRegs = (ubicom32v4Regs_t *)cpu->registers;
  struct regcache *current_regcache = get_current_regcache();
  swizzled_supply_register(current_regcache, 0, &mainRegs->tRegs[ubicom32v3currentThread].dr[0]);
  swizzled_supply_register(current_regcache, 21, &mainRegs->tRegs[ubicom32v3currentThread].ar[5]);
  swizzled_supply_register(current_regcache, 22, &mainRegs->tRegs[ubicom32v3currentThread].ar[6]);
  swizzled_supply_register(current_regcache, 23, &mainRegs->tRegs[ubicom32v3currentThread].ar[7]);
  swizzled_supply_register(current_regcache, 30, &mainRegs->tRegs[ubicom32v3currentThread].rosr);
  swizzled_supply_register(current_regcache, 34, &mainRegs->tRegs[ubicom32v3currentThread].threadPc);
  swizzled_supply_register(current_regcache, 47, &mainRegs->globals.mt_active);
  swizzled_supply_register(current_regcache, 50, &mainRegs->globals.mt_dbg_active);
  swizzled_supply_register(current_regcache, 52, &mainRegs->globals.mt_en);
}

/* ubicom32v3_wait */
static ptid_t
ubicom32v3_wait (ptid_t ptid, struct target_waitstatus *status)
{
  bpReason_t reason;
  int mask;

  static RETSIGTYPE (*prev_sigint) ();
  int ret;

  if (remote_debug)
    printf_filtered ("ubicom32v3_wait\n");

  status->kind = TARGET_WAITKIND_EXITED;
  status->value.integer = 0;

  if(remote_ubicom32v3_state == UBICOM32V3_STEPPING_PROBLEMS)
    {
      /* single stepping was attempted on a suspended thread */
      status->kind = TARGET_WAITKIND_STOPPED;
      status->value.sig = TARGET_SIGNAL_STOP;
      remote_ubicom32v3_state = UBICOM32V3_STOPPED;
      ubicom32v3_read_dcache_status();
      return pid_to_ptid (ubicom32v3_current_thread);
    }
  else if((remote_ubicom32v3_state == UBICOM32V3_STOPPED) || (remote_ubicom32v3_state == UBICOM32V3_CORE_STOPPED))
    {
      /* Things are already stopped */
      status->kind = TARGET_WAITKIND_STOPPED;
      status->value.sig = TARGET_SIGNAL_TRAP;
      if (remote_ubicom32v3_state != UBICOM32V3_CORE_STOPPED)
	ubicom32v3_read_dcache_status();
      return pid_to_ptid (ubicom32v3_current_thread);
    }

  ubicom32v3_cntrl_c_abort = 0;
  prev_sigint = signal (SIGINT, ubicom32v3_cntrl_c);
  while (1)
    {
      if (ubicom32v3_cntrl_c_abort)
	{
	  /*
	   * control c turnOffconsole in ubicom32v3_wait()
	   */
	  int ret2 = turnOffconsole();

	  ret = ubicom32v3stopProcessor();
	  if(ret)
	    {
	      printf_unfiltered("error: Could not stop the processor\n");
	      signal (SIGINT, prev_sigint);
	      return null_ptid;
	    }

	  reason.reason = BREAKPOINT_HIT;
	  break;
	}
      else
	{
	  if (ignore_trap)
	    {
	      /*
	       * waitForBPnoTrap turnOffconsole in ubicom32v3_wait()
	       */
	      int ret2 = turnOffconsole();

	      /* call waitForBPNoTrap and see if we get anything */
	      ret = ubicom32v3waitForBPNoTrap(&reason);
	    }
	  else
	    {
	      /*
	       * waitForBP turnOffconsole in ubicom32v3_wait()
	       */
	      int ret2 = turnOffconsole();

	      /* call waitForBP and see if we get anything */
	      ret = ubicom32v3waitForBP(&reason);
	    }

	  if(ret< 0)
	    {
	      printf_unfiltered ("error: unable to read status\n");
	      signal (SIGINT, prev_sigint);
	      return null_ptid;
	    }
	  if(ret == 0)
	    {

	      /*
	       * not any BP hit turnOnconsole in ubicom32v3_wait()
	       */
	      int ret2 = turnOnconsole();


	      /* wait for 10 ms before trying again */
	      usleep(10000);
	      continue;
	    }

	  break;
	}
    }

  /* go retrieve the registers for a TPacket */
  ret = ubicom32v3createTpacketMP(ubicom32v3_current_thread-1);
  if(ret)
    {
      printf_unfiltered("error: Could not retrieve registers after stopping the processor\n");
      signal (SIGINT, prev_sigint);
      return null_ptid;
    }

  /* Supply the tpacket registers. */
  current_cpu->supply_tpacket_regs_fn(current_cpu, ubicom32v3_current_thread-1);

  /* XXXXXXXXXxx Rewrite */
  switch(reason.reason)
    {
    case TRAP:
      {
	printf_unfiltered("TRAP in thread %d status 0x%x\n", ubicom32v3currentThread, reason.status);

	if (reason.status & (1 << 0)) {
	  printf_unfiltered("TRAP cause: Instruction address decode error.\n");
	}
	else if (reason.status & (1 << 1)) {
	  printf_unfiltered("TRAP cause: Instruction synchronous error.\n");
	}
	else if (reason.status & (1 << 2)) {
	  printf_unfiltered("TRAP cause: Illegal instruction.\n");
	}
	else if (reason.status & (1 << 3)) {
	  printf_unfiltered("TRAP cause: Source 1 address decode error.\n");
	}
	else if (reason.status & (1 << 4)) {
	  printf_unfiltered("TRAP cause: Destination address decode error.\n");
	}
	else if (reason.status & (1 << 5)) {
	  printf_unfiltered("TRAP cause: Source 1 operand alignment error.\n");
	}
	else if (reason.status & (1 << 6)) {
	  printf_unfiltered("TRAP cause: Destination oprand alignment error.\n");
	}
	else if (reason.status & (1 << 7)) {
	  printf_unfiltered("TRAP cause: Source 1 synchronous error.\n");
	}
	else if (reason.status & (1 << 8)) {
	  printf_unfiltered("TRAP cause: Destination synchrouous error.\n");
	}
	else if (reason.status & (1 << 9)) {
	  printf_unfiltered("TRAP cause: Data Capture.\n");
	}
	else if (reason.status & (1 << 10)) {
	  printf_unfiltered("TRAP cause: Instruction fetch memory protection error.\n");
	}
	else if (reason.status & (1 << 11)) {
	  printf_unfiltered("TRAP cause: Source 1 memory protection error.\n");
	}
	else if (reason.status & (1 << 12)) {
	  printf_unfiltered("TRAP cause: Destination memory protection error.\n");
	}
	status->kind = TARGET_WAITKIND_STOPPED;
	status->value.sig = TARGET_SIGNAL_STOP;
	remote_ubicom32v3_state = UBICOM32V3_TRAP_ERROR;

	break;
      }
    case HALT:
      {
	if (reason.status & (1 << 2)) {
	  printf_unfiltered("HALT cause: Watchdog timer reset.\n");
	}
	if (reason.status & (1 << 4)) {
	  printf_unfiltered("HALT cause: Software reset.\n");
	}
	if (reason.status & (1 << 5)) {
	  printf_unfiltered("HALT cause: Instruction port async error.\n");
	}
	if (reason.status & (1 << 6)) {
	  printf_unfiltered("HALT cause: Data port asnyc error.\n");
	}
	if (reason.status & (1 << 7)) {
	  printf_unfiltered("HALT cause: Instruction address decode error.\n");
	}
	if (reason.status & (1 << 8)) {
	  printf_unfiltered("HALT cause: Instruction synchronous error.\n");
	}
	if (reason.status & (1 << 9)) {
	  printf_unfiltered("HALT cause: Illegal instruction.\n");
	}
	if (reason.status & (1 << 10)) {
	  printf_unfiltered("HALT cause: Soruce 1 address decode error.\n");
	}
	if (reason.status & (1 << 11)) {
	  printf_unfiltered("HALT cause: Destination address decode error.\n");
	}
	if (reason.status & (1 << 12)) {
	  printf_unfiltered("HALT cause: Source 1 operand alignment error.\n");
	}
	if (reason.status & (1 << 13)) {
	  printf_unfiltered("HALT cause: Destination oprand alignment error.\n");
	}
	if (reason.status & (1 << 14)) {
	  printf_unfiltered("HALT cause: Source 1 synchronous error.\n");
	}
	if (reason.status & (1 << 15)) {
	  printf_unfiltered("HALT cause: Destination synchrouous error.\n");
	}
	if (reason.status & (1 << 16)) {
	  printf_unfiltered("HALT cause: Write address watchpoint.\n");
	}
	if (reason.status & (1 << 17)) {
	  printf_unfiltered("HALT cause: Source 1 memory protection error.\n");
	}
	if (reason.status & (1 << 18)) {
	  printf_unfiltered("TRAP cause: Destination memory protection error.\n");
	}
	status->kind = TARGET_WAITKIND_STOPPED;
	status->value.sig = TARGET_SIGNAL_STOP;
	remote_ubicom32v3_state = UBICOM32V3_TRAP_ERROR;

	break;
      }

    default:
      status->kind = TARGET_WAITKIND_STOPPED;
      status->value.sig = TARGET_SIGNAL_TRAP;
      remote_ubicom32v3_state = UBICOM32V3_STOPPED;
    }
  signal (SIGINT, prev_sigint);

  ubicom32v3_read_dcache_status();

  return pid_to_ptid (ubicom32v3_current_thread);
}

/* Resume execution of the target process.  STEP says whether to single-step
   or to run free; SIGGNAL is the signal value (e.g. SIGINT) to be given
   to the target, or zero for no signal.  */
static void
ubicom32v3_resume (ptid_t ptid, int step, enum target_signal siggnal)
{
  int thread_num = ptid_get_pid (inferior_ptid);
  int mask, i;
  CORE_ADDR pc;
  extern unsigned int  mt_active, mt_en;

  ubicom32v3_set_thread(thread_num);
  thread_num--;
  mask = 1<< ubicom32v3currentThread;

  if (remote_debug)
    printf_filtered ("ubicom32v3_resume\n");

  pc = read_pc();

  dcache_status = 0;

 //inferior_ptid = ptid;

  if (step)
    {
      int ret;
      thread_num = PIDGET (inferior_ptid);

      ubicom32v3_set_thread(thread_num);

      /* we are single stepping the main processor */
      if(!(mt_en & mask))
	{
	  // Tryingn to single step a Disabled thread
	  printf_unfiltered("error: Single Stepping a Disabled thread.\n");
	  remote_ubicom32v3_state = UBICOM32V3_STEPPING_PROBLEMS;
	}
      else if (!(mt_active & mask))
	{
	  printf_unfiltered("error: Single Stepping a Suspended thread.\n");
	  remote_ubicom32v3_state = UBICOM32V3_STEPPING_PROBLEMS;
	}
      if(remote_ubicom32v3_state == UBICOM32V3_STEPPING_PROBLEMS)
	return;

      ret = ubicom32v3singleStep(ubicom32v3currentThread);

      if(remote_ubicom32v3_state == UBICOM32V3_STOPPED)
	remote_ubicom32v3_state = UBICOM32V3_STEPPING;
    }
  else
    {
      /* let everything run */
      int ret;
      ret= ubicom32v3restartProcessor(1);

      /*
       * after resume command to run or continue call turnOnconsole in ubicom32v3_resume()
       */
      ret = turnOnconsole();

      remote_ubicom32v3_state = UBICOM32V3_RUNNING;
    }
}

static int
ubicom32v3_write_bytes (CORE_ADDR address, char *buffer, int length)
{
  // split the transfer into lead, aligned middle and end
  unsigned truncBytes = (address & 0x3);
  unsigned truncAddress = address & ~0x3;
  unsigned int leadBytes, leadAddr, midBytes, midAddr, endBytes, endAddr, leadIndex;
  unsigned toWriteWords;
  unsigned char  *sendBuffer;
  unsigned char *freeBuffer;
  unsigned char *cptr;
  int i, j, ret;

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
  ret = ubicom32v3writeMemory(leadAddr, toWriteWords, (int *)sendBuffer);
  //ret = ubicom32v3cacheflushinvalidate(leadAddr, toWriteWords*4);

  free (freeBuffer);
  return (leadBytes + midBytes + endBytes);
}

static int
ubicom32v3_read_bytes (CORE_ADDR address, char *buffer, int length)
{
  // split the transfer into lead, aligned middle and end
  unsigned truncBytes = (address & 0x3);
  unsigned truncAddress = address & ~0x3;
  unsigned int leadBytes, leadAddr, midBytes, midAddr, endBytes, endAddr, leadIndex;
  unsigned toReadWords;
  unsigned char  *recvBuffer;
  unsigned char *freeBuffer;
  unsigned char *cptr;
  int i, j, ret;

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

  ret = ubicom32v3readMemory(leadAddr, toReadWords, (int *)recvBuffer);

  // Now Swizzle the data out
  if(leadBytes)
    {
      for(i= 0;i< leadBytes; i++)
	{
	  *buffer++ = recvBuffer[leadIndex--];
	}
      recvBuffer += 4;
    }

  if(midBytes)
    {
      for(i=0; i< midBytes; i+= 4)
	{
	  for(j=0; j< 4; j++)
	    {
	      *buffer++ = recvBuffer[3-j];
	    }
	  recvBuffer += 4;
	}
    }

  if(endBytes)
    {
      for(i=0; i< endBytes; i++)
	{
	  *buffer ++ = recvBuffer[3-i];
	}
    }
  recvBuffer += 4;
  free (freeBuffer);

  return (leadBytes + midBytes + endBytes);
}

#define OCP_BASE	0x01000000
#define OCP_DCCSTR	0x60C	/* D-Cache Control Statu Register */

static void
ubicom32v3_read_dcache_status(void)
{
  CORE_ADDR address = (OCP_BASE + OCP_DCCSTR);
  char *dptr , temp[4];
  (void) ubicom32v3_read_bytes(address, (char *)temp, sizeof(dcache_status));

  /* need to swizzle to look correct on a little endian machine. */
  dptr = (char *)&dcache_status;
  *dptr++ = temp[3];
  *dptr++ = temp[2];
  *dptr++ = temp[1];
  *dptr++ = temp[0];
}


#if 0
static char *copyvbuf = NULL;
static int
ubicom32v3_slow_verify_bytes (CORE_ADDR address, char *buffer, int length)
{
  // split the transfer into lead, aligned middle and end
  unsigned truncBytes = (address & 0x3);
  unsigned truncAddress = address & ~0x3;
  unsigned int leadBytes, leadAddr, midBytes, midAddr, endBytes, endAddr, leadIndex;
  unsigned toVerifyWords;
  unsigned char cptr[4];
  int i, j, ret;

  if(copyvbuf == NULL)
    copyvbuf = malloc(UBICOM32V3_LOAD_BLOCK_SIZE);

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

  toVerifyWords = midBytes/4;

  /* Allocate the send buffer */
  if(leadBytes)
    {
      // Misaligned start. Deal with it by first backing up the address to nearest
      // Go read 4 bytes from the backed up address
      switch (leadAddr >> 28)
	{
	  case 0x2:
	    // a pram section read
	    ret = readPgmMemory(leadAddr, 1, (int *)cptr);
	    break;
	  default:
	    printf_unfiltered("unknown address: 0x%08x\n", leadAddr);
	}

      for(i=0; i<leadBytes; i++)
	{
	  if (cptr[leadIndex--] != (unsigned char) *buffer++)
	    return -1;
	}
    }

  // Deal with the midsection if any
  if(midBytes)
    {
      int ret, i;
      char *vbuf = copyvbuf;
      //ret = ubicom32v3_fast_read_bytes(midAddr,  (char *)vbuf, toVerifyWords*4 );
      ret = ubicom32v3UltraFastReadMemoryRaw(midAddr, toVerifyWords, (int *)vbuf);
      if (memcmp (vbuf, buffer, toVerifyWords * 4) != 0)
	{
	  for(i=0; i< midBytes; i+=4, vbuf+=4, buffer+=4)
	    {
	      if(vbuf[0] != buffer[0] ||
		 vbuf[1] != buffer[1] ||
		 vbuf[2] != buffer[2] ||
		 vbuf[3] != buffer[3])
		{
		  printf_unfiltered("Mismatch at 0x%x Expected 0x%02x %02x %02x %02x got 0x%02x %02x %02x %02x\n",
				    (midAddr+i),(unsigned char ) buffer[0], (unsigned char ) buffer[1], (unsigned char ) buffer[2], (unsigned char ) buffer[3],
				    (unsigned char ) vbuf[3], (unsigned char ) vbuf[2], (unsigned char ) vbuf[1], (unsigned char) vbuf[0]);
		  return -1;
		}
	    }
	}
      else
	{
	  buffer += toVerifyWords*4;
	}
    }

  if(endBytes)
    {
      // trailing cruft to deal with
      // Go read 4 bytes from the backed up end address
      ret = readPgmMemory(endAddr, 1, (int *)cptr);
      for(i=0; i< endBytes; i++)
	{
	  if (cptr[3-i] != (unsigned char) *buffer++)
	    return -1;
	}
    }

  return (leadBytes + midBytes + endBytes);
}
#endif

static addrBounds_t ubicom32v3bounds[]={
   {0x800, 0xa00},		/* HRT tables */
   {0x01000000, 0x01001000},	/* On chip peripherals */
   {0x02000000, 0x02010000},	/* IO */
   {0x3ffc0000, 0x48000000},	/* On chip Memory + DDR*/
   {0x60000000, 0x61000000},	/* Flash */
};

static unsigned int ubicom32v3numBoundsEntries = sizeof(ubicom32v3bounds)/sizeof(addrBounds_t);

/* ARGSUSED */
static int
ubicom32v3_xfer_memory (CORE_ADDR mem_addr, gdb_byte *buffer, int mem_len,
		    int should_write, struct mem_attrib *attrib,
		    struct target_ops *target)
{
  CORE_ADDR targ_addr;
  int targ_len;
  int res;

  targ_addr = mem_addr;
  targ_len = mem_len;

  if (targ_len <= 0)
    return 0;

  // Check if we got a valid address
  if(targ_addr >= ubicom32v3bounds[ubicom32v3numBoundsEntries-1].upper)
    {
      // address is too high
      return 0;
    }

  for(res =0; res< ubicom32v3numBoundsEntries; res++)
    {
      if(should_write && res == (ubicom32v3numBoundsEntries-1))
	{
	  /* flash write attempt and that is a no no */
	  printf("Flash write is not allowed.\n");
	  return 0;
	}

      // test the very lowest address
      if(targ_addr < ubicom32v3bounds[res].lower)
	{
	  // invalid address. The start is in the hole
	  return (targ_addr - ubicom32v3bounds[res].lower);
	}

      // test if this is a valid address
      if(targ_addr < ubicom32v3bounds[res].upper)
	{
	  // Valid range. Do an upper bound check and adjust length if needed
	  if(targ_len+targ_addr > ubicom32v3bounds[res].upper)
	    targ_len = ubicom32v3bounds[res].upper - targ_addr;

	  break;
	}
    }

  // I dcache_status indicates error then see if the address is a DDR/FLASH address
  if(dcache_status & 0x1)
    {
      // dcache error. Disallow DDR/FLASH operations.
      if (targ_addr >= 0x40000000 && targ_addr < 0x61000000)
	{
	  // Flash DDR range. Do no allow the transaction.
	  printf_unfiltered("D-Cache in error state. DDR and Flash access is not allowed.\n");
	  return -1;
	}
    }

  if (should_write)
    {
      res = ubicom32v3_write_bytes (targ_addr, buffer, targ_len);
    }
  else
    res = ubicom32v3_read_bytes (targ_addr, buffer, targ_len);

  return res;
}

#ifdef DO_CORE_DUMPS
/* ARGSUSED */
static int
ubicom32v3_core_xfer_memory (CORE_ADDR mem_addr, gdb_byte *buffer, int mem_len,
		    int should_write, struct mem_attrib *attrib,
		    struct target_ops *target)
{
  CORE_ADDR targ_addr;
  int targ_len;
  int res;
  char *src = NULL;
  CORE_ADDR index;
  int switch_res;;

  targ_addr = mem_addr;
  targ_len = mem_len;

  if (targ_len <= 0)
    return 0;

  if(should_write)
    return 0;

  // Check if we got a valid address
  if(targ_addr >= ubicom32v3bounds[ubicom32v3numBoundsEntries-1].upper)
    {
      // address is too high
      return 0;
    }

  for(res =0; res< ubicom32v3numBoundsEntries; res++)
    {
      // test the very lowest address
      if(targ_addr < ubicom32v3bounds[res].lower)
	{
	  // invalid address. The start is in the hole
	  return (targ_addr - ubicom32v3bounds[res].lower);
	}

      // test if this is a valid address
      if(targ_addr < ubicom32v3bounds[res].upper)
	{
	  // Valid range. Do an upper bound check and adjust length if needed
	  if(targ_len+targ_addr > ubicom32v3bounds[res].upper)
	    targ_len = ubicom32v3bounds[res].upper - targ_addr;

	  break;
	}
    }

  src = NULL;
  index = targ_addr - ubicom32v3bounds[res].lower;
  switch_res = res;
  res = targ_len;

  switch(switch_res)
    {
    case 0:
      {
	/* hrts */
	src = ubicom32v3_core.hrts;
	break;
      }
    case 1:
      {
	/* read to the IO block. We can't do much about it */
	printf_unfiltered("Core has no information on the On Chip Prepheral registers.\n");
	memset(buffer, 0, targ_len);
	src = NULL;
	break;
      }
    case 2:
      {
	/* read to the IO block. We can't do much about it */
	printf_unfiltered("Core has no information on IO block registers.\n");
	memset(buffer, 0, targ_len);
	src = NULL;
	break;
      }
    case 3:
      {
	/* OCM + DDR Memory */
	if(targ_addr < 0x40000000 )
	  {
	    src = ubicom32v3_core.ocm_mem;
	  }
	else
	  {
	    src = ubicom32v3_core.ddr_mem;
	    index = targ_addr - 0x40000000;
	  }
	break;
      }
    case 4:
      {
	/* flash memory */
	src = ubicom32v3_core.flash_mem;
	break;
      }
    default:
      {
	printf_unfiltered("Incorrect address block 0x%lx\n", targ_addr);
	res = 0;
      }

    }
  if(src)
    {
      char *ptr = &src[index];
      memcpy(buffer, ptr, targ_len);
    }
  return res;
}
#endif

/* ubicom32v3_hw_mon */
static void
ubicom32v3_special_mon (char *args, int from_tty)
{
  unsigned int object_address, event_address, event_pattern, event_mask;
  int scanf_ret, ret;
  unsigned int resBuf[32];

  static RETSIGTYPE (*prev_sigint) ();

  /* scan the args for object, event, event_pat and event_mask */
  scanf_ret = sscanf(args, "0x%x 0x%x 0x%x 0x%x", &object_address, &event_address, &event_pattern, &event_mask);

  printf_unfiltered("object_addr =0x%x event_addr =0x%x event_pattern = 0x%x event_pattern = 0x%x\n",
		    object_address, event_address, event_pattern, event_mask);

  if(scanf_ret != 4)
    {
      printf("Usage: specialmon 0xobject_address 0xevent_address 0xevent_pattern 0xevent_mask\n");
      return;
    }

  /* set up the hardware monitor and get it cranking */
  ret = ubicom32v3hw_monitor_setup(object_address, 1, event_address, event_pattern, event_mask, 32);

  if(ret == -4)
    {
      error("No existing connection.\n Issue command: ubicom32v3hwmon hostname:PORT#\n");
    }

  /* time to monitor the status */
  ubicom32v3_cntrl_c_abort = 0;
  prev_sigint = signal (SIGINT, ubicom32v3_cntrl_c);
  while (1)
    {
      unsigned int status;
      if (ubicom32v3_cntrl_c_abort)
	{
	  printf_unfiltered("Aborting monitoring\n");
	  signal (SIGINT, prev_sigint);
	  return;
	}

      ret = ubicom32v3isp_hw_monitor_status(&status, resBuf, 32);
      if(ret) {
	error("Monitor status command bombed\n");
      }

      if(status == 1)
	break;
    }

  /* I you got here monitor has triggered */
  for(ret =0; ret< 32; ret++)
    {
      printf("0x%08x\n", resBuf[ret]);
    }
}

static void
ubicom32v3_cache_flush (char *args, int from_tty)
{
  int scanf_ret;
  unsigned int address;

  scanf_ret = 0;

  if(args)
    {
      scanf_ret = sscanf(args, "0x%x", &address);
    }

  if(scanf_ret == 0)
    {
      printf("Usage cacheflush 0xaddress\n");
      return;
    }

  scanf_ret = ubicom32v3cacheinvalidate(address, 4);
}

static void
ubicom32v3_bist_test (char *args, int from_tty)
{
  int ret;
  if (!args)
    args = "localhost:5010";

  /* Connect to dongle.  */
  ret = ubicom32v3isp_connect (args);

  if(ret == -1)
    error ("Unable to find dongle.");
  else if(ret == -2)
    error ("Dongle has no Ubicom32v3 support. Get a new dongle with Ubicom32v3 support and try again.");


  /* Attach to target.  */
  ret = ubicom32v3isp_attach ();

  ret = ubicom32v3isp_bist();
}

static void
ubicom32v3_previous_pc (char *args, int from_tty)
{
  int thread_num = ubicom32v3currentThread;
  extern int chipID;
  switch (chipID)
    {
    case 0x20000:
    case 0x20001:
    case 0x20002:
      ubicom32v3Registers.tRegs[thread_num].threadPc = ubicom32v3Registers.tRegs[thread_num].previous_pc;
      break;
    case 0x30000:
    case 0x30001:
      ubicom32v4Registers.tRegs[thread_num].threadPc = ubicom32v4Registers.tRegs[thread_num].previous_pc;
      break;
    }

  remote_ubicom32v3_state = UBICOM32V3_CORE_STOPPED;
  inferior_ptid = pid_to_ptid (ubicom32v3_current_thread);
  start_remote(from_tty);

  /*
  ubicom32v3_fetch_register(get_current_regcache(), 0);
  select_frame(get_current_frame());
  */
}

static void
ubicom32v3_ddr_test (char *args, int from_tty)
{
  /* malloc a Meg of data and fill it with random numbers. */
  int *sptr = (int *)malloc(0x100000);

  int i, ret;
  struct timeval begin, end;
  double time;
  char *cptr1, *cptr2;

  /* malloc a Meg of data for verification.. */
    int *dptr = (int *)malloc(0x100000);

  srandom (getpid());

  for(i=0; i< 0x100000/4; i++)
    {
      sptr[i] = random();
    }


  /* set up the fast downloader */
  ret = ubicom32v3isp_download_fast_transfer_code(UBICOM32V3_OCM_ADDR);
  if (ret)
    {
      printf_unfiltered("Failed downloading fast transfer code.\n");
      return;
    }

  /* load the begin time structure */
  (void) gettimeofday(&begin, NULL);

  /* write 10 Megs */
  for(i=0; i< 10; i++)
    {
      int ret = ubicom32v3UltrafastWriteMemory(0x40000000, 0x100000/4, (int *)sptr);
      if (ret) {
	printf_unfiltered("Fast Write bombed\n");
      }
    }
  /* load the end time structure */
  (void) gettimeofday(&end, NULL);

  /* calculate elapsed time */
  time = ((double)end.tv_sec + (double)end.tv_usec/1000000.0) - ((double)begin.tv_sec + (double)begin.tv_usec/1000000.0);

  printf_unfiltered ("Elapsed time in sec for 10M DDr write = %.2lf\n", time);

  for(i = 0; i< 0x100000/4; i++)
    {
      unsigned int send = sptr[i];
      cptr1 = (char *)&sptr[i];
      cptr2 = (char *) &send;

      /* swizzle the data */
      cptr1[0] = cptr2[3];
      cptr1[1] = cptr2[2];
      cptr1[2] = cptr2[1];
      cptr1[3] = cptr2[0];
    }

  ret = ubicom32v3UltraFastReadMemoryRaw(0x40000000, 0x100000/4, (int *)dptr);
  if (memcmp (sptr, dptr, 0x100000) != 0)
    {
      printf_unfiltered("Data mismatch\n");
    }
  else
    {
      printf_unfiltered("Success\n");
    }
}

static void
ubicom32v3_pll_test (char *args, int from_tty)
{
  int ret;
  if (!args)
    args = "localhost:5010";

  /* Connect to dongle.  */
  ret = ubicom32v3isp_connect (args);

  if(ret == -1)
    error ("Unable to find dongle.");
  else if(ret == -2)
    error ("Dongle has no Ubicom32v3 support. Get a new dongle with Ubicom32v3 support and try again.");


  /* Attach to target.  */
  ret = ubicom32v3isp_attach ();
  ret = ubicom32v3isp_reset();
  ret = ubicom32v3isp_pll_init();
}

/* ubicom32v3_show_load_progress_default.  */
static void
ubicom32v3_show_load_progress_default (const char *section,
			    unsigned int section_sent,
			    unsigned int section_size,
			    unsigned int total_sent,
			    unsigned int total_size)
{
  static int progress;
  int current;

  if (total_sent == 0)
    progress = 0;

  current = (int)((float)total_sent / (float)total_size * (float)80);
  while (progress < current)
    {
      putchar_unfiltered ('.');
      gdb_flush (gdb_stdout);
      progress ++;
    }

  if (total_sent == total_size)
    {
      putchar_unfiltered ('\n');
      gdb_flush (gdb_stdout);
    }
}

/*
 * ubicom32v3_cache_dump()
 *   This routine will dump the cache and store it in a file called Ubicom.cache.
 *    XXXX We will need a way to to be able to find out which version of the chip we
 *    are dumping the cache for. I suppose CHIP_ID could be used to figure that out.
 *    For the time being we will dump 256 Tag lines and 8192 bytes for dcache.
 *    We will dump 512 Tag lines and 16384 bytes of icache.
 */

void
ubicom32v3_cache_dump()
{
  extern int chipID;
  struct ubicom32v3_cache_dump_header cache_header;
  FILE *fp;
  char filename[] = "Ubicom.cache";
  unsigned char *buffer;
  int ret;
  extern int ubicom32v3readcachetags(unsigned int startAddr, unsigned int numcachelines, unsigned int cache, unsigned char *buffer);
  extern int ubicom32v3readcachedata(unsigned int startAddr, unsigned int numdata, unsigned int cache, unsigned char *buffer);

  switch (chipID)
    {
    case 0x20000:
    case 0x20001:
    case 0x20002:
      {
	/* Set up the header of a Ubicom32v3 with 8k Dcache and 16k Icache. */
	cache_header.dcache_tag_entries = 256;
	cache_header.dcache_data_bytes  = 256*32;
	cache_header.icache_tag_entries = 512;
	cache_header.icache_data_bytes  = 512*32;

	/* Allocate the buffer through which we will stage the data. */
	buffer = malloc(512*32);
	if (buffer == NULL)
	  {
	    printf_unfiltered("Unable to allocate buffer for staging data. Cache not dumped.\n");
	    return;
	  }
      }
      break;
    case 0x30000:
    case 0x30001:
      {
	/* Set up the header of a IP&k with 16k Dcache and 16k Icache. */
	cache_header.dcache_tag_entries = 512;
	cache_header.dcache_data_bytes  = 512*32;
	cache_header.icache_tag_entries = 512;
	cache_header.icache_data_bytes  = 512*32;

	/* Allocate the buffer through which we will stage the data. */
	buffer = malloc(512*32);
	if (buffer == NULL)
	  {
	    printf_unfiltered("Unable to allocate buffer for staging data. Cache not dumped.\n");
	    return;
	  }
      }
      break;
    default:
      {
	printf_unfiltered ("Unknown Processor. No cache dumped.");
	return;
      }
    }

  /* Open a Ubicom.cache to save the cache data. */
  fp = fopen(filename, "wb+");

  if (fp == NULL)
    {
      printf_unfiltered("Unable to open %s. Cache not dumped.\n", filename);
      free(buffer);
      return;
    }

    /* Write the header out to the file. */
    fwrite(&cache_header, 1, sizeof(cache_header), fp);

    /* Dump dcache tags. */
    ret = ubicom32v3readcachetags(0, cache_header.dcache_tag_entries, 0, buffer);
    if (ret)
      {
	/* Extraction bombed. */
	printf_unfiltered("Dcache TAG dump bombed.\n");
	fclose(fp);
	free(buffer);
	return;
      }

    /* write out the TAG data to file. */
    fwrite(buffer, 1, cache_header.dcache_tag_entries*4, fp);

    /* Dump dcache data. */
    ret = ubicom32v3readcachedata(0, cache_header.dcache_data_bytes, 0, buffer);
    if (ret)
      {
	/* Extraction bombed. */
	printf_unfiltered("Dcache DATA dump bombed.\n");
	free(buffer);
	fclose(fp);
	return;
      }

    /* write out the data to file. */
    fwrite(buffer, 1, cache_header.dcache_data_bytes, fp);

    /* Dump icache tags. */
    ret = ubicom32v3readcachetags(0, cache_header.icache_tag_entries, 1, buffer);
    if (ret)
      {
	/* Extraction bombed. */
	printf_unfiltered("Icache TAG dump bombed.\n");
	free(buffer);
	fclose(fp);
	return;
      }

    /* write out the TAG data to file. */
    fwrite(buffer, 1, cache_header.icache_tag_entries*4, fp);

    /* Dump icache data. */
    ret = ubicom32v3readcachedata(0, cache_header.icache_data_bytes, 1, buffer);
    if (ret)
      {
	/* Extraction bombed. */
	printf_unfiltered("Icache DATA dump bombed.\n");
	free(buffer);
	fclose(fp);
	return;
      }

    /* write out the data to file. */
    fwrite(buffer, 1, cache_header.icache_data_bytes, fp);

    /* Close the file. */
    fclose(fp);
    free(buffer);
}

/* ubicom32v3_core_dump */
void
ubicom32v3_core_dump (char *args, int from_tty)
{
  char default_name[]="Ubicom.core";
  int default_sdram_size = 2*1024*1024;
  int sdsize, ret;
  struct ubicom32v3_core_header coreHeader;
  char *filename;
  CORE_ADDR begin, end;
  unsigned int mem_size;
  FILE *fp;
  int i;
  char *buffer;
  unsigned int progress, total_size;
  int save_ubicom32v3_current_thread= ubicom32v3_current_thread;
  int save_ubicom32v3currentThread= ubicom32v3currentThread;
  CORE_ADDR begin_sdram, end_sdram;
  const size_t buffer_size = 1024 * 1024;

  /*
   * Turn off the 2 wire interface if possible.
   */
  ret = turnOff2wire();

  /*
   * Dump the cache before you do anything.
   */
  printf_unfiltered("Dumping Caches\n");
  ubicom32v3_cache_dump();

  filename = default_name;

  fp = fopen(filename, "wb+");

  if(fp == NULL)
    {
      printf_filtered("ubicom32v3_core_dump: Cannot open %s\n", filename);
      return;
    }

  /* We managed to open the file */
  /* Update the register info for all the threads */
  for(i=0; i< current_cpu->num_threads; i++)
    {
      ubicom32v3_set_thread(i+1);
      (void) current_cpu->fetch_registers_fn(current_cpu, i);
    }

  ubicom32v3_set_thread(save_ubicom32v3_current_thread);

  /* malloc a 1 Meg buffer to move the data through */
  buffer = malloc(buffer_size);

  if(buffer == NULL)
    {
      printf_unfiltered("ubicom32v3_core_dump: Failed to malloc transfer buffer\n");
      return;
    }

  /* load Ubicom Core Magic into the header */
  coreHeader.magic = current_cpu->core_magic;

  /* Set file position to end of header */
  (void) fseek(fp, sizeof(coreHeader), SEEK_SET);

  /* write out debugger thread number */
  (void) fwrite(&ubicom32v3currentThread, 1, 4, fp);

  /* load the register offset into the coreHeader */
  coreHeader.regOffset = ftell(fp);

  /* write out register area size */
  mem_size = current_cpu->reg_area_size;
  (void) fwrite(&mem_size, 1, 4, fp);

  /* write out the register data */
  (void) fwrite(current_cpu->registers, 1, current_cpu->reg_area_size, fp);

  /* load hrt offset into coreHeader */
  coreHeader.hrtOffset = ftell(fp);

  /* set up the fast downloader */
  ret = ubicom32v3isp_download_fast_transfer_code(UBICOM32V3_OCM_ADDR);

  /* read and write the hrt block */
  (void) ubicom32v3_read_bytes(0x800, buffer, 512);
  (void) fwrite(buffer, 1, 512, fp);

  /* load ocm offset into coreHeader */
  coreHeader.ocmOffset = ftell(fp);

  begin = UBICOM32V3_OCM_ADDR;
  end = UBICOM32V3_DDR_ADDR;

  /* write out the data memory size */
  mem_size = end - begin;
  (void) fwrite(&mem_size, 1, 4, fp);

  printf_unfiltered("Dumping %dk OCM 0x%x-0x%x\n", mem_size / 1024, (int) begin, (int) end);
  /* now read and write out the ocm memory */
  while(mem_size)
    {
      int length = buffer_size;

      if(length > mem_size)
	length = mem_size;

      /* read in the data */
      ret = ubicom32v3UltraFastReadMemoryRaw(begin, length / 4, (int *)buffer);

      /* write out the data. */
      (void) fwrite(buffer, 1, length, fp);

      begin += length;
      mem_size -= length;
    }

  /* load ddr offset into coreHeader */
  coreHeader.ddrOffset = ftell(fp);

  /* First see if there are explicit coredump limits */
  begin_sdram = ubicom32v3_lookup_symbol("__sdram_coredump_begin");
  end_sdram = ubicom32v3_lookup_symbol("__sdram_coredump_limit");
  if (!begin_sdram || !end_sdram) {

	  begin_sdram = ubicom32v3_lookup_symbol("__sdram_begin");
	  end_sdram = ubicom32v3_lookup_symbol("__sdram_limit");
	  if(begin_sdram == 0)
		  printf("begin_sdram not found\n");

	  if(end_sdram == 0)
		  printf("end_sdram not found\n");
  }

  total_size = mem_size = (end_sdram - begin_sdram);
  begin = begin_sdram;
  /* write out the ddr memory size */
  (void) fwrite(&mem_size, 1, 4, fp);


  printf("Dumping %dk DDR 0x%x-0x%x. This takes time.\n", mem_size / 1024, (int)begin_sdram, (int)end_sdram);
  /* now read and write out the DDR memory */
  progress = 0;
  ubicom32v3_show_load_progress_default(NULL, 0, 0, progress, total_size);

  while(mem_size)
    {
      int length = buffer_size;

      if(length > mem_size)
	length = mem_size;

      /* read in the data */
      ret = ubicom32v3UltraFastReadMemoryRaw(begin, length/4, (int *)buffer);

      /* write out the data. */
      (void) fwrite(buffer, 1, length, fp);

      begin += length;
      mem_size -= length;
      progress += length;
      ubicom32v3_show_load_progress_default(NULL, 0, 0, progress, total_size);
    }

  /* read and write out the dynamic_non_volatile_section  */
  begin = UBICOM32V3_FLASH_ADDR;
  end = ubicom32v3_lookup_symbol("__filemedia_end_addr");
  total_size = mem_size = end - begin;

  printf_unfiltered("Dumping %dk FLASH 0x%x-0x%x\n", mem_size / 1024, (int)begin,(int)end);
  progress = 0;
  ubicom32v3_show_load_progress_default(NULL, 0, 0, progress, total_size);
  /* load flash offset into coreHeader */
  coreHeader.flashOffset = ftell(fp);

  /* write out the data memory size */
  (void) fwrite(&mem_size, 1, 4, fp);

  /* now read and write out the data memory */
  while(mem_size)
    {
      int length = buffer_size;

      if(length > mem_size)
	length = mem_size;

      /* read in the data */
      ret = ubicom32v3UltraFastReadMemoryRaw(begin, length/4, (int *)buffer);

      /* write out the data. */
      (void) fwrite(buffer, 1, length, fp);

      begin += length;
      mem_size -= length;
      progress += length;
      ubicom32v3_show_load_progress_default(NULL, 0, 0, progress, total_size);
    }

  /* rewind the file and write out the header into the file */
  rewind(fp);

  (void) fwrite(&coreHeader, 1, sizeof(coreHeader), fp);

  /* close the file */
  fclose(fp);

  /* Turn on turbo 2 wire if possible. */
  ret = turnOn2wire();

  return;
}

/* ubicom32v3_reset.  */
void
ubicom32v3_reset (char *args, int from_tty)
{
  int ret;
  bpReason_t reason;
  int i;

  dcache_status = 0;

  if (remote_debug)
    printf_filtered ("ubicom32v3_reset\n");

  reinit_frame_cache();
  registers_changed();
  ret = ubicom32v3isp_reset();

  /* restart the Cpu to get it to the ok_to_set_breakpoints label */
  ret= ubicom32v3restartProcessor(1);
  remote_ubicom32v3_state = UBICOM32V3_RUNNING;

  for (i=0; i< 500; i++)
    {
      ret = ubicom32v3waitForBP(&reason);

      if(ret< 0)
	{
	  printf_unfiltered ("error: unable to read status\n");
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

  if((i == 500) || (ret < 0))
    {
      /* The processor is still running, we need to stop it. */
      ret = ubicom32v3stopProcessor();
    }

  ubicom32v3_set_thread(1);
  inferior_ptid = pid_to_ptid (1);
  ubicom32v3_fetch_register(get_current_regcache(), 0);
  select_frame(get_current_frame());
  normal_stop();
  ubicom32v3_read_dcache_status();
}

#ifdef DO_CORE_DUMPS
/* ubicom32v3_core_kill.  */
static void
ubicom32v3_core_kill (void)
{
  if (remote_debug)
      printf_filtered ("ubicom32v3_core_kill\n");

  inferior_ptid = null_ptid;

  /* Detach from target.  */
  ubicom32v3_core_detach (NULL, 0);
}
#endif

/* ubicom32v3_kill.  */
static void
ubicom32v3_kill (void)
{
  if (remote_debug)
      printf_filtered ("ubicom32v3_kill\n");

  inferior_ptid = null_ptid;

  /* Detach from target.  */
  ubicom32v3_detach (NULL, 0);
}

/* ubicom32v3_add_module_file.  */
static void
ubicom32v3_erase_flash (char *args, int from_tty)
{
  bfd *loadfile_bfd;
  asection *s, *downloadersection;
  char *arg;
  char **argv;
  struct cleanup *my_cleanups = make_cleanup (null_cleanup, NULL);
  char *file, *data;
  CORE_ADDR start_addr = 0;
  CORE_ADDR end_addr = 0;
  CORE_ADDR length = 0;
  int total_length = 0;
  int progress = 0;
  int has_downloader = 0;
  unsigned int reply;
  CORE_ADDR ocm_begin;
  int offs, argcnt, ret;

  if (args == NULL)
    error (_("Usage: erase-flash start_addr length\n"));

  dont_repeat ();

  argv = buildargv (args);
  make_cleanup_freeargv (argv);

  if (argv == NULL)
    nomem (0);

  /*
   * Find how many arguments were passed to us.
   */
  for (arg = argv[0], argcnt = 0; arg != NULL; arg = argv[++argcnt])
    {
    }

  if (argcnt != 2)
    error ("Usage: erase-flash start_address length\n");

  for (arg = argv[0], argcnt = 0; arg != NULL; arg = argv[++argcnt])
    {
      /* Process the argument. */
      if (argcnt == 0)
	{
	  /* The first argument is the start address. */
	  start_addr = parse_and_eval_address (arg);
	}
      else if (argcnt == 1)
	{
	  /* The second argument is length. */
	  length = parse_and_eval_address (arg);
	  end_addr = start_addr + length;
	}
    }

  /*
   * range check the start and end addresses.
   */
  if ((start_addr < UBICOM32V3_FLASH_ADDR) || (start_addr > UBICOM32V3_FLASH_END))
    error("Start address 0x%lx is out of range\n", start_addr);

  if ((end_addr < UBICOM32V3_FLASH_ADDR) || (end_addr > UBICOM32V3_FLASH_END))
    error("Start address 0x%lx is out of range\n", end_addr);

  file = get_exec_file(1);

  if (remote_debug)
    printf_filtered ("ubicom32v3_load: %s (tty=%d)\n", file, from_tty);

  /* Open the file.  */
  loadfile_bfd = bfd_openr (file, gnutarget);
  if (loadfile_bfd == NULL)
    error ("Error: Unable to open file %s\n", file);

  if (!bfd_check_format (loadfile_bfd, bfd_object))
    {
      bfd_close (loadfile_bfd);
      error ("Error: File is not an object file\n");
    }

  /*
   * Scan through the sections looking for the .downloader section. We better find it
   * because without it we are not going to be able to download any code to the board.
   */
  for (s = loadfile_bfd->sections; s; s = s->next)
    {
      if(strcmp(s->name, ".downloader"))
	{
	  continue;
	}

      has_downloader = 1;
      break;
    }

  if(has_downloader == 0)
    {
      printf_unfiltered("%s has no \".downloader\" section. Unable to erase flash, no flash drivers.\n", file);
      return;
    }

  downloadersection = s;
  length = bfd_get_section_size (s);

  /* Get the downloader section data. */
  data = malloc(length+64);
  if (!data)
    {
      printf_unfiltered ("error: unable to allocate memory for .downloader section data.\n");
      return;
    }

  offs = 0;

  /* read in the .downloader section data from the the file. */
  bfd_get_section_contents (loadfile_bfd, s, data, offs, length);

  /* xxxxxxxxxxxxxxxxxxxxxxxxxx call the backed init */
  ocm_begin = ubicom32v3_lookup_symbol("__ocm_begin");
  if (ocm_begin == 0)
    {
      printf_unfiltered("No __ocm_begin symbol. Unable to initialize Boot Kernel\n");
      free(data);
      ubicom32v3_reset(0,0);
      return;
    }

  /*
   * Turn off the 2 wire interface if possible.
   */
  ret = turnOff2wire();

  ret = ubicom32v3isp_initBackendToBootKernel(length, (unsigned int*)data, ocm_begin);
  if(ret)
    {
      /* Turn on turbo 2 wire if possible. */
      ret = turnOn2wire();
      printf_unfiltered("Unable to initialize Boot Kernel\n");
      free(data);
      ubicom32v3_reset(0,0);
      return;
    }

  free(data);

  end_addr -= start_addr;
  printf("Begin Erase.\n 0x%lx length 0x%lx\n", start_addr, end_addr);

  ret = eraseVerifyFlash(start_addr, end_addr, ubicom32_erase_progress_callback, NULL);

  /* Turn on turbo 2 wire if possible. */
  ret = turnOn2wire();
  ubicom32v3_reset(0,0);

  return;
}

/* ubicom32v3_add_module_file.  */
static void
ubicom32v3_add_module_file (char *args, int from_tty)
{
  bfd *loadfile_bfd;
  asection *s;
  char *arg;
  char **argv;
  struct cleanup *my_cleanups = make_cleanup (null_cleanup, NULL);
  char *filename = NULL;
  CORE_ADDR text_addr = 0;
  CORE_ADDR ocm_text_addr = 0;
  CORE_ADDR dyn_text_addr;
  CORE_ADDR dyn_ocm_text_addr;
  CORE_ADDR data_addr;
  CORE_ADDR dyn_data_addr;
  int num_text, num_ocm_text, num_data, section_index;
  int do_ocm_text = 0;
  int argcnt = 0;
  struct section_addr_info *section_addrs;
  //int flags = OBJF_USERLOADED;
  int flags = OBJF_READNOW;
  struct objfile *ofiles = object_files;
  struct partial_symtab *psymtabs;

  if (args == NULL)
    error (_("add-module-file takes a file name and an address"));

  dont_repeat ();

  argv = buildargv (args);
  make_cleanup_freeargv (argv);

  if (argv == NULL)
    nomem (0);

  for (arg = argv[0], argcnt = 0; arg != NULL; arg = argv[++argcnt])
    {
      /* Process the argument. */
      if (argcnt == 0)
	{
	  /* The first argument is the file name. */
	  filename = tilde_expand (arg);
	  make_cleanup (xfree, filename);
	}
      else
	if (argcnt == 1)
	  {
	    /* The second argument is always the address of the text section. */
	       text_addr = parse_and_eval_address (arg);
	  }
	else if (argcnt == 2)
	  {

	    if (strncmp(arg, "(nil)", strlen("(nil)")) == 0)
	      continue;

	    /* The third argument is always the address of the ocm_text_section */
	    ocm_text_addr = parse_and_eval_address (arg);
	    do_ocm_text = 1;
	  }
	else
	  {
	    error("Too many parameters\n.Usage: add-module-name text_address <ocm_text_address>\n");
	    return;
	  }
    }

  if(do_ocm_text)
    printf_filtered("add-module-file \"%s\" text_address= 0x%lx ocm_text_address= 0x%lx\n", filename, (long)text_addr, (long)ocm_text_addr);
  else
    printf_filtered("add-module-file %s text_address= 0x%lx\n", filename, (long)text_addr);

  /* Open the file. */
  //loadfile_bfd = bfd_openr (filename, gnutarget);
  loadfile_bfd = symfile_bfd_open (filename);
  if (loadfile_bfd == NULL)
    error ("Error: Unable to open file %s\n", filename);

  /* Loop through all the sections and find the OCM sections and calculate how many text and ocmtext sections we have. */
  dyn_text_addr = text_addr;
  dyn_ocm_text_addr = ocm_text_addr;

  num_text = num_ocm_text = num_data = 0;

  for (s = loadfile_bfd->sections; s; s = s->next)
    {
      if (strncmp(s->name, ".text", strlen(".text")) == 0)
	{
	  /* .text section. */
	  num_text ++;
	  dyn_text_addr += bfd_get_section_size (s);
	}

      else if (strncmp(s->name, ".ocm_text", strlen(".ocm_text")) == 0)
	{
	  if (do_ocm_text == 0)
	    {
	      /* The file has ocm_text but no address has been provided. */
	      error("File has ocm_text sections but no ocm base address has been provided.");
	    }

	  num_ocm_text ++;
	  dyn_ocm_text_addr += bfd_get_section_size (s);
	}
      else if (strncmp(s->name, ".data", strlen(".data")) == 0)
	{
	  /* .data section. */
	  num_data ++;
	}
    }

  dyn_data_addr = dyn_text_addr;
  dyn_text_addr = text_addr;
  dyn_ocm_text_addr = ocm_text_addr;

  /* Alocation the structure to hold the addresses. */
  section_addrs = alloc_section_addr_info (num_text + num_ocm_text + num_data);
  section_index = 0;

  for (s = loadfile_bfd->sections; s; s = s->next)
    {
      if (strncmp(s->name, ".text", strlen(".text")) == 0)
	{
	  /* .text section. */
	  section_addrs->other[section_index].name = strdup(s->name);
	  make_cleanup(xfree, section_addrs->other[section_index].name);
	  section_addrs->other[section_index].addr = dyn_text_addr;
	  dyn_text_addr += bfd_get_section_size (s);
	  section_index ++;
	}
      else if (strncmp(s->name, ".ocm_text", strlen(".ocm_text")) == 0)
	{
	  section_addrs->other[section_index].name = strdup(s->name);
	  make_cleanup(xfree, section_addrs->other[section_index].name);
	  section_addrs->other[section_index].addr = dyn_ocm_text_addr;
	  dyn_ocm_text_addr += bfd_get_section_size (s);
	  section_index ++;
	}
      else if (strncmp(s->name, ".data", strlen(".data")) == 0)
	{
	  /* .data section. */
	  section_addrs->other[section_index].name = strdup(s->name);
	  make_cleanup(xfree, section_addrs->other[section_index].name);
	  section_addrs->other[section_index].addr = dyn_data_addr;
	  dyn_data_addr += bfd_get_section_size (s);
	  section_index ++;
	}
    }

  if (section_index != (num_text + num_ocm_text + num_data))
    {
      error("#of output sections does not match input.");
    }

  bfd_close (loadfile_bfd);
  symbol_file_add (filename, from_tty, section_addrs, 0, flags);
  reinit_frame_cache ();
  do_cleanups (my_cleanups);

  /* find the second last file on the object_file list. The newly added file is the last file on the list. */
  ofiles = object_files;
  while(ofiles->next->next)
    {
      ofiles = ofiles->next;
    }

  if (ofiles == NULL)
    error("Cannot find %s of the object_file list\n", bfd_get_filename(loadfile_bfd));

  /* Find the 2nd last files psymtab. */
  psymtabs = ofiles->psymtabs;

  /* find the last entry on this psymtab linked list. */
  while(psymtabs->next)
    psymtabs = psymtabs->next;

  /* We have found the last entry. Chain the new file psymtab linked list to object_files psymtab linked list. */
  psymtabs->next = ofiles->next->psymtabs;
}

/* ubicom32v3_ocm_load_run.  */
static void
ubicom32v3_ocm_load_run (char *arg, int from_tty)
{
  bfd *loadfile_bfd;
  asection *s;
  int total_length = 0;
  int progress = 0;
  int addr, length, offs, size, endaddr;
  unsigned char *data, *verifybuf, *protect;
  unsigned int last_address = 0;
  unsigned int first_address = 0xffffffff;
  int ret;

  char *file;

  size = UBICOM32V3_LOAD_BLOCK_SIZE;
  data = malloc (size *2 );
  if (!data)
    {
      printf_unfiltered ("error: unable to allocate memory for write\n");
      return;
    }
  verifybuf = &data[size];

  file = get_exec_file(1);

  if (remote_debug)
    printf_filtered ("ubicom32v3_load: %s (tty=%d)\n", file, from_tty);

  /* Open the file.  */
  loadfile_bfd = bfd_openr (file, gnutarget);
  if (loadfile_bfd == NULL)
    error ("Error: Unable to open file %s\n", file);

  if (!bfd_check_format (loadfile_bfd, bfd_object))
    {
      bfd_close (loadfile_bfd);
      error ("Error: File is not an object file\n");
    }

  endaddr = -1;
  /* Loop through all the sections and find the OCM sections and get an estimate of how much we are loading. */
  for (s = loadfile_bfd->sections; s; s = s->next)
    {
      if (ubicom32v3_loadable_section (loadfile_bfd, s) != UBICOM32V3_OCM_SECTION)
	continue;

      length = bfd_get_section_size (s);
      addr = s->lma;
      if ((addr + length) > endaddr)
	endaddr = addr + length;
    }

  /* round up endaddr to word boundary. */
  endaddr += 7;
  endaddr &= ~0x3;

  /* Reset the system. */
  ret = ubicom32v3isp_reset();

  /* set up the fast downloader */
  ret = ubicom32v3isp_download_fast_transfer_code(endaddr);

  for (s = loadfile_bfd->sections; s; s = s->next)
    {
      if (ubicom32v3_loadable_section (loadfile_bfd, s) != UBICOM32V3_OCM_SECTION)
	continue;

      length = bfd_get_section_size (s);

      if (remote_debug)
	printf_filtered("%s: 0 / %d\n", s->name, length);

      size = UBICOM32V3_LOAD_BLOCK_SIZE;

      offs = 0;
      while(length)
	{
	  unsigned int retlen;
	  int readtransfer;
	  if(size > length)
	    size = length;

	  /* read in the data size bytes at a time from the section */
	  bfd_get_section_contents (loadfile_bfd, s, data, offs, size);

	  addr = s->lma + offs;

	  retlen = ubicom32v3_write_bytes(addr, (char *)data, size);

	  /* read the data and verify */
	  readtransfer = size;
	  if (size %4)
	    {
	      /* round up size to a 4 byte boundary */
	      size += 3;
	      size &= ~3;
	    }

	  ret = ubicom32v3UltraFastReadMemoryRaw(addr, size/4, (int *)verifybuf);
	  if (memcmp (data, verifybuf, readtransfer) != 0)
	    {
	      int i;
	      char *badblock = verifybuf;
	      char *cmpblock = data;
	      int currentaddress = addr;
	      for(i = 0; i< readtransfer; i++, currentaddress++) {
		if(badblock[i] != cmpblock[i]){
		  printf("addr = 0x%08x expect 0x%02hhx got 0x%02hhx\n", currentaddress, (unsigned)cmpblock[i], (unsigned)badblock[i]);
		  break;
		}
	      }
	    }

	  offs += readtransfer;
	  length -= readtransfer;

	  if (remote_debug)
	    printf_filtered("%s: 0 / %d\n", s->name, length);
	}
    }

  free(data);
  if(s != NULL)
    {
      printf_filtered("Ocm download and run failed.\n");
      return;
    }

  /* Code is downloaded and verified. */
  /* guarantee that thread0 will be alive when we give it control. */
  ret = ubicom32v3isp_make_thread0_alive();

  /* now transfer control to the .downloader section */
  addr = 0x3ffc0000;

  ret = current_cpu->write_register_fn(ubicom32v3currentThread, 34, (void *)&addr, 1);
  ret= ubicom32v3restartProcessor(1);

  if(ret)
    {
      printf_filtered("Ocm Run failed.\n");
    }
}

/* ubicom32v3_verify.  */
static void
ubicom32v3_verify (char *arg, int from_tty)
{
  bfd *loadfile_bfd;
  asection *s;
  int total_length = 0;
  int progress = 0;
  int addr, length, offs, size;
  unsigned char *data, *verifybuf, *protect;
  unsigned int last_address = 0;
  unsigned int first_address = 0xffffffff;
  int ret;

  char *file;

  size = UBICOM32V3_LOAD_BLOCK_SIZE;
  data = malloc (size *2 );
  if (!data)
    {
      printf_unfiltered ("error: unable to allocate memory for write\n");
      return;
    }
  verifybuf = &data[size];

  file = get_exec_file(1);

  if (remote_debug)
    printf_filtered ("ubicom32v3_load: %s (tty=%d)\n", file, from_tty);

  /* Open the file.  */
  loadfile_bfd = bfd_openr (file, gnutarget);
  if (loadfile_bfd == NULL)
    error ("Error: Unable to open file %s\n", file);

  if (!bfd_check_format (loadfile_bfd, bfd_object))
    {
      bfd_close (loadfile_bfd);
      error ("Error: File is not an object file\n");
    }

  /* set up the fast downloader */
  ret = ubicom32v3isp_download_fast_transfer_code(UBICOM32V3_OCM_ADDR);

  for (s = loadfile_bfd->sections; s; s = s->next)
    {
      if (ubicom32v3_loadable_section (loadfile_bfd, s) != UBICOM32V3_FLASH_SECTION)
	continue;

      length = bfd_get_section_size (s);

      if (remote_debug)
	printf_filtered("%s: 0 / %d\n", s->name, length);

      size = UBICOM32V3_LOAD_BLOCK_SIZE;

      offs = 0;
      while(length)
	{
	  int readtransfer;
	  if(size > length)
	    size = length;

	  /* read in the data size bytes at a time from the section */
	  bfd_get_section_contents (loadfile_bfd, s, data, offs, size);

	  addr = s->lma + offs;

	  /* read the data and verify */
	  //readtransfer= ubicom32v3_read_bytes(addr, verifybuf, size);
	  readtransfer = size;
	  if (size %4)
	    {
	      /* round up size to a 4 byte boundary */
	      size += 3;
	      size &= ~3;
	    }

	  ret = ubicom32v3UltraFastReadMemoryRaw(addr, size/4, (int *)verifybuf);
	  if (memcmp (data, verifybuf, readtransfer) != 0)
	    {
	      int i;
	      char *badblock = verifybuf;
	      char *cmpblock = data;
	      int currentaddress = addr;
	      for(i = 0; i< readtransfer; i++, currentaddress++) {
		if(badblock[i] != cmpblock[i]){
		  printf("addr = 0x%08x expect 0x%02hhx got 0x%02hhx\n", currentaddress, (unsigned)cmpblock[i], (unsigned)badblock[i]);
		  break;
		}
	      }
	    }

	  offs += readtransfer;
	  length -= readtransfer;

	  if (remote_debug)
	    printf_filtered("%s: 0 / %d\n", s->name, length);
	}
    }

  free(data);

}

/* ubicom32v3_load.  */
static void
ubicom32v3_load (char *file, int from_tty)
{
  bfd *loadfile_bfd;
  asection *s, *downloadersection;
  int total_length = 0;
  int progress = 0;
  int addr, length, offs, size;
  unsigned char *data, *protect, *foundK;
  unsigned int last_address = 0;
  unsigned int first_address = 0xffffffff;
  int ret;
  int has_downloader = 0;
  unsigned int reply;
  CORE_ADDR ocm_begin;

  if (remote_debug)
    printf_filtered ("ubicom32v3_load: %s (tty=%d)\n", file, from_tty);

  /* Open the file.  */
  loadfile_bfd = bfd_openr (file, gnutarget);
  if (loadfile_bfd == NULL)
    error ("Error: Unable to open file %s\n", file);

  if (!bfd_check_format (loadfile_bfd, bfd_object))
    {
      bfd_close (loadfile_bfd);
      error ("Error: File is not an object file\n");
    }

  /*
   * Scan through the sections looking for the .downloader section. We better find it
   * because without it we are not going to be able to download any code to the board.
   */
  for (s = loadfile_bfd->sections; s; s = s->next)
    {
      unsigned char buf[4];
      CORE_ADDR length;
      int sec;
      unsigned int section_last_address;
      size_t len;

      if(strcmp(s->name, ".downloader"))
	{
	  continue;
	}

      has_downloader = 1;
      break;
    }

  if(has_downloader == 0)
    {
      printf_unfiltered("%s has no \".downloader\" section. Unable to download code.\n", file);
      return;
    }

  downloadersection = s;
  length = bfd_get_section_size (s);

  /* Get the downloader section data. */
  data = malloc(length+64);
  if (!data)
    {
      printf_unfiltered ("error: unable to allocate memory for .downloader section data.\n");
      return;
    }

  offs = 0;

  /* read in the .downloader section data from the the file. */
  bfd_get_section_contents (loadfile_bfd, s, data, offs, length);

  /* xxxxxxxxxxxxxxxxxxxxxxxxxx call the backed init */
  ocm_begin = ubicom32v3_lookup_symbol("__ocm_begin");
  if (ocm_begin == 0)
    {
      printf_unfiltered("No __ocm_begin symbol. Unable to initialize Boot Kernel\n");
      free(data);
      ubicom32v3_reset(0,0);
      return;
    }

  /*
   * Turn off the 2 wire interface if possible.
   */
  ret = turnOff2wire();

  ret = ubicom32v3isp_initBackendToBootKernel(length, (unsigned int*)data, ocm_begin);
  if(ret)
    {
      printf_unfiltered("Unable to initialize Boot Kernel\n");
      free(data);
      ubicom32v3_reset(0,0);
      return;
    }

  free(data);
  size = UBICOM32V3_LOAD_BLOCK_SIZE;
  data = malloc(size*2);
  if (!data)
    {
      printf_unfiltered ("error: unable to allocate memory for write\n");
      return;
    }

  /* xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx */

  for (s = loadfile_bfd->sections; s; s = s->next)
    {
      unsigned char buf[4];
      CORE_ADDR length;
      int sec;
      unsigned int section_last_address;

      sec = ubicom32v3_loadable_section (loadfile_bfd, s);
      if (sec != UBICOM32V3_FLASH_SECTION)
	continue;

      length = bfd_get_section_size (s);
      total_length += length;
      section_last_address = length + s->lma;
      section_last_address +=3;

      /* round up the last address */
      section_last_address &= ~0x3;

      if(section_last_address > last_address)
	last_address = section_last_address;

      if(s->lma < first_address)
	first_address = s->lma;
    }

  last_address -= first_address;

  /*
   * Turn off the 2 wire interface if possible.
   */
  ret = turnOff2wire();

  /* tell spt that we are going to start code download. spt will issue the chip erase.
     This will block till the chip erase returns. */
  data[0] = 0;
  sprintf(data,"Begin Erase.\n 0x%x length 0x%x", first_address, last_address);
  printf("%s\n", data);

  ret = eraseVerifyFlash(first_address, last_address, ubicom32_erase_progress_callback, NULL);
  if(ret)
    return;

  printf("Begin download\n");
  /* Write flash.  */
  for (s = loadfile_bfd->sections; s; s = s->next)
    {
      if (ubicom32v3_loadable_section (loadfile_bfd, s) != UBICOM32V3_FLASH_SECTION)
	continue;

      length = bfd_get_section_size (s);

      if (remote_debug)
	printf_filtered("%s: 0 / %d\n", s->name, length);
      ubicom32v3_show_load_progress_default (NULL, 0, length, progress, total_length);

      size = UBICOM32V3_LOAD_BLOCK_SIZE;

      offs = 0;
      while(length)
	{
	  if(size > length)
	    size = length;

	  addr = s->lma + offs;

	  /*
	   * If the lead address is not aligned to a flash write line boundary then
	   * Send enough so that all subsequent transfers will be aligned to
	   * flash write line boundary.
	   */
	  if(addr & 0xff)
	    {
	      /* the start is misaligned */
	      int endaddr = addr + size;

	      if((endaddr & 0xff) > addr)
		{
		  endaddr &= 0xff;
		  size = endaddr - addr;
		}
	    }

	  /* read in the data size bytes at a time from the section */
	  bfd_get_section_contents (loadfile_bfd, s, data, offs, size);

	  ubicom32v3_write_flash_bytes(addr, data, size);

	  offs += size;
	  length -= size;
	  progress += size;

	  if (remote_debug)
	    printf_filtered("%s: 0 / %d\n", s->name, length);
	  ubicom32v3_show_load_progress_default (NULL, 0, length, progress, total_length);

	}
    }

  /* Verify flash.  */
  for (s = loadfile_bfd->sections; s; s = s->next)
    {
      if (ubicom32v3_loadable_section (loadfile_bfd, s) != UBICOM32V3_FLASH_SECTION)
	continue;

      length = bfd_get_section_size (s);

      if (remote_debug)
	printf_filtered("%s: 0 / %d\n", s->name, length);

      size = UBICOM32V3_LOAD_BLOCK_SIZE;

      offs = 0;
      while(length)
	{
	  if(size > length)
	    size = length;

	  /* read in the data size bytes at a time from the section */
	  bfd_get_section_contents (loadfile_bfd, s, data, offs, size);

	  addr = s->lma + offs;

	  if (common_verify_bytes(addr, data, size) < 0)
	    printf_unfiltered("verify error in section %s (0x%08x - 0x%08x)\n", s->name, addr, addr + size);


	  offs += size;
	  length -= size;
	  progress += size;

	  if (remote_debug)
	    printf_filtered("%s: 0 / %d\n", s->name, length);
	}
    }

  ret = ubicom32v3stopProcessor();

  /* Turn on turbo 2 wire if possible. */
  ret = turnOn2wire();
  free(data);

  /* reset and restart */
  ubicom32v3isp_attach();
  ubicom32v3_reset(0, 0);
}

static void
ubicom32v3_verify_reg_read (char *file, int from_tty)
{
  extern void verify_reg_read(void);

  verify_reg_read();
}

#if 0
static void
ubicom32v3_mail_status (char *file, int from_tty)
{
  int status;
  extern int readStatus(int *);
  int ret = readStatus(&status);
  printf("mail status = 0x%8x\n", status);
}

static void
ubicom32v3_read_mailbox (char *file, int from_tty)
{
  int mboxData=0xaa55aa55;
  extern int readMailbox(int *);
  int ret = readMailbox(&mboxData);
  printf("Ret = %d Mailbox sent = 0x%8x\n", ret,  mboxData);
}
#endif

static int
ubicom32v3_insert_breakpoint (struct bp_target_info *bp_tgt)
{
  unsigned int *bp = (unsigned int *)gdbarch_breakpoint_from_pc(current_gdbarch, &bp_tgt->placed_address, &bp_tgt->placed_size);
  CORE_ADDR addr = bp_tgt->placed_address;
  unsigned int bpToSend;
  char *cptr = (char *)&bpToSend;
  int ret;

  memcpy((void *)&bpToSend, (void *)bp, bp_tgt->placed_size);
  cptr[3] =  (0xff);

  /* make sure the address is in the PRAM space. XXXXXX May change in the future */
  if(addr >= ubicom32v3bounds[ubicom32v3numBoundsEntries-2].lower &&
     ubicom32v3bounds[ubicom32v3numBoundsEntries-2].upper > addr)
    {
      /* valid breakpointable address */

      /* read out the old contents */
      ret = ubicom32v3_read_bytes (addr, bp_tgt->shadow_contents, bp_tgt->placed_size);

      if(ret!= bp_tgt->placed_size)
	return -1;
      /* write in the break point */
      ret = ubicom32v3_write_bytes (addr, (char *) &bpToSend, bp_tgt->placed_size);

      if(ret == bp_tgt->placed_size)
	{
	  /* flush dcache and invalidate the icache */
	  ret = ubicom32v3cacheflushinvalidate(addr, bp_tgt->placed_size);
	  return 0;
	}
      else
	return -1;
    }

  return EINVAL;
}

static int
ubicom32v3_remove_breakpoint (struct bp_target_info *bp_tgt)
{
  unsigned int *bp = (unsigned int *)gdbarch_breakpoint_from_pc(current_gdbarch, &bp_tgt->placed_address, &bp_tgt->placed_size);
  CORE_ADDR addr = bp_tgt->placed_address;
  int ret, i;
  char *prev_contents = (char *)malloc(bp_tgt->placed_size);
  char *cptr = (char *) bp;
  int *iptr = (int *)prev_contents;

  cptr[3] = 0xff;

  /* make sure the address is a breakpointable address */
  if(addr >= ubicom32v3bounds[ubicom32v3numBoundsEntries-2].lower &&
     ubicom32v3bounds[ubicom32v3numBoundsEntries-2].upper > addr)
    {

      /* read out the old contents */
      ret = ubicom32v3_read_bytes (addr, prev_contents, bp_tgt->placed_size);

#if 0
      /* blow off the lower 11 bits */
      *iptr &= ~(0x7ff);
#endif

      /* check if we do have a bp instruction at this address */
      for(ret =0; ret< bp_tgt->placed_size-1; ret++)
	if(cptr[ret] != prev_contents[ret])
	  return 0;

      /* write in the old contents */
      ret = ubicom32v3_write_bytes (addr, (char *) bp_tgt->shadow_contents, bp_tgt->placed_size);

      if(ret == bp_tgt->placed_size)
	{
	  /* flush dcache and invalidate the icache */
	  ret = ubicom32v3cacheflushinvalidate(addr, bp_tgt->placed_size);
	  return 0;
	}
      else
	return -1;
    }
  else
    return EINVAL;
}

/*
 * Collect a descriptive string about the given thread.
 * The target may say anything it wants to about the thread
 * (typically info about its blocked / runnable state, name, etc.).
 * This string will appear in the info threads display.
 *
 * Optional: targets are not required to implement this function.
 */

static char *
ubicom32v3_threads_extra_info (struct thread_info *tp)
{
  unsigned int thread_num;
  extern char * ubicom32v3_remote_threads_extra_info (struct thread_info *tp);
  extern void gdbarch_info_init (struct gdbarch_info *info);
  thread_num = PIDGET(tp->ptid);

  ubicom32v3_set_thread(thread_num);

  return (ubicom32v3_remote_threads_extra_info(tp));
}

/* init_ubicom32v3_ops.  */
static void
init_ubicom32v3_ops (void)
{
  ubicom32v3_ops.to_shortname = "ubicom32v3";
  ubicom32v3_ops.to_longname = "Remote ubicom32v3 debug Via Ubicom Ethernet Dongle";
  ubicom32v3_ops.to_doc = "Remote ubicom32 debug Via Ubicom Ethernet Dongle.\n\
Connect to target ubicom32 board as follows:\n\
target ubicom32 dongle-ip-address:5010\n\
";
  ubicom32v3_ops.to_open = ubicom32v3_open;
  ubicom32v3_ops.to_close = ubicom32v3_close;
  ubicom32v3_ops.to_attach = ubicom32v3_attach;
  ubicom32v3_ops.to_post_attach = NULL;
  ubicom32v3_ops.to_detach = ubicom32v3_detach;
  ubicom32v3_ops.to_resume = ubicom32v3_resume;
  ubicom32v3_ops.to_wait = ubicom32v3_wait;
  //ubicom32v3_ops.to_post_wait = NULL;
  ubicom32v3_ops.to_fetch_registers = ubicom32v3_fetch_register;
  ubicom32v3_ops.to_store_registers = ubicom32v3_store_register;
  ubicom32v3_ops.to_prepare_to_store = ubicom32v3_prepare_to_store;
  ubicom32v3_ops.deprecated_xfer_memory = ubicom32v3_xfer_memory;
  ubicom32v3_ops.to_files_info = ubicom32v3_files_info;
  ubicom32v3_ops.to_kill = ubicom32v3_kill;
  ubicom32v3_ops.to_load = ubicom32v3_load;
  ubicom32v3_ops.to_insert_breakpoint = ubicom32v3_insert_breakpoint;
  ubicom32v3_ops.to_remove_breakpoint = ubicom32v3_remove_breakpoint;

  ubicom32v3_ops.to_terminal_init = NULL;
  ubicom32v3_ops.to_terminal_inferior = NULL;
  ubicom32v3_ops.to_terminal_ours_for_output = NULL;
  ubicom32v3_ops.to_terminal_ours = NULL;
  ubicom32v3_ops.to_terminal_info = NULL;
  ubicom32v3_ops.to_lookup_symbol = NULL;
  ubicom32v3_ops.to_create_inferior = NULL;
  ubicom32v3_ops.to_post_startup_inferior = NULL;
  ubicom32v3_ops.to_acknowledge_created_inferior = NULL;
  ubicom32v3_ops.to_insert_fork_catchpoint = NULL;
  ubicom32v3_ops.to_remove_fork_catchpoint = NULL;
  ubicom32v3_ops.to_insert_vfork_catchpoint = NULL;
  ubicom32v3_ops.to_remove_vfork_catchpoint = NULL;
  ubicom32v3_ops.to_insert_exec_catchpoint = NULL;
  ubicom32v3_ops.to_remove_exec_catchpoint = NULL;
  ubicom32v3_ops.to_reported_exec_events_per_exec_call = NULL;
  ubicom32v3_ops.to_has_exited = NULL;
  ubicom32v3_ops.to_mourn_inferior = NULL;
  ubicom32v3_ops.to_can_run = 0;
  ubicom32v3_ops.to_notice_signals = 0;
  ubicom32v3_ops.to_thread_alive = ubicom32v3_thread_alive;
  ubicom32v3_ops.to_pid_to_str = ubicom32v3_thread_pid_to_str;
  ubicom32v3_ops.to_stop = ubicom32v3_stop;
  ubicom32v3_ops.to_pid_to_exec_file = NULL;
  ubicom32v3_ops.to_stratum = process_stratum;
  ubicom32v3_ops.to_has_all_memory = 1;
  ubicom32v3_ops.to_has_memory = 1;
  ubicom32v3_ops.to_has_stack = 1;
  ubicom32v3_ops.to_has_registers = 1;
  ubicom32v3_ops.to_has_execution = 1;
  ubicom32v3_ops.to_sections = NULL;
  ubicom32v3_ops.to_sections_end = NULL;
  ubicom32v3_ops.to_magic = OPS_MAGIC;
  ubicom32v3_ops.to_extra_thread_info = ubicom32v3_threads_extra_info;
}

#ifdef DO_CORE_DUMPS
/* init_ubicom32v3_ops.  */
static void
init_ubicom32v3_core_ops (void)
{
  extern char * ubicom32v3_remote_threads_extra_info (struct thread_info *tp);
  ubicom32v3_core_ops.to_shortname = "ubicom32v3core";
  ubicom32v3_core_ops.to_longname = "Remote ubicom32v3 debug Via Ubicom core File.";
  ubicom32v3_core_ops.to_doc = "Remote ubicom32v3 debug Via Ubicom core File.\n\
Invoke as ubicom32-elf-gdb elffile corefile.\n\
";
  ubicom32v3_core_ops.to_open = ubicom32v3_core_open;
  ubicom32v3_core_ops.to_close = ubicom32v3_core_close;
  ubicom32v3_core_ops.to_attach = NULL;
  ubicom32v3_core_ops.to_post_attach = NULL;
  ubicom32v3_core_ops.to_detach = ubicom32v3_core_detach;
  ubicom32v3_core_ops.to_resume = NULL;
  ubicom32v3_core_ops.to_wait = ubicom32v3_wait;
  ubicom32v3_core_ops.to_fetch_registers = ubicom32v3_fetch_register;
  ubicom32v3_core_ops.to_store_registers = ubicom32v3_store_register;;
  ubicom32v3_core_ops.to_prepare_to_store = ubicom32v3_prepare_to_store;
  ubicom32v3_core_ops.deprecated_xfer_memory = ubicom32v3_core_xfer_memory;
  ubicom32v3_core_ops.to_files_info = ubicom32v3_files_info;
  ubicom32v3_core_ops.to_kill = ubicom32v3_core_kill;
  ubicom32v3_core_ops.to_load = NULL;
  ubicom32v3_core_ops.to_insert_breakpoint = NULL;
  ubicom32v3_core_ops.to_remove_breakpoint = NULL;

  ubicom32v3_core_ops.to_terminal_init = NULL;
  ubicom32v3_core_ops.to_terminal_inferior = NULL;
  ubicom32v3_core_ops.to_terminal_ours_for_output = NULL;
  ubicom32v3_core_ops.to_terminal_ours = NULL;
  ubicom32v3_core_ops.to_terminal_info = NULL;
  ubicom32v3_core_ops.to_lookup_symbol = NULL;
  ubicom32v3_core_ops.to_create_inferior = NULL;
  ubicom32v3_core_ops.to_post_startup_inferior = NULL;
  ubicom32v3_core_ops.to_acknowledge_created_inferior = NULL;
  ubicom32v3_core_ops.to_insert_fork_catchpoint = NULL;
  ubicom32v3_core_ops.to_remove_fork_catchpoint = NULL;
  ubicom32v3_core_ops.to_insert_vfork_catchpoint = NULL;
  ubicom32v3_core_ops.to_remove_vfork_catchpoint = NULL;
  ubicom32v3_core_ops.to_insert_exec_catchpoint = NULL;
  ubicom32v3_core_ops.to_remove_exec_catchpoint = NULL;
  ubicom32v3_core_ops.to_reported_exec_events_per_exec_call = NULL;
  ubicom32v3_core_ops.to_has_exited = NULL;
  ubicom32v3_core_ops.to_mourn_inferior = NULL;
  ubicom32v3_core_ops.to_can_run = 0;
  ubicom32v3_core_ops.to_notice_signals = 0;
  ubicom32v3_core_ops.to_thread_alive = ubicom32v3_thread_alive;
  ubicom32v3_core_ops.to_pid_to_str = ubicom32v3_thread_pid_to_str;
  ubicom32v3_core_ops.to_stop = NULL;
  ubicom32v3_core_ops.to_pid_to_exec_file = NULL;
  ubicom32v3_core_ops.to_stratum = process_stratum;
  ubicom32v3_core_ops.to_has_all_memory = 1;
  ubicom32v3_core_ops.to_has_memory = 1;
  ubicom32v3_core_ops.to_has_stack = 1;
  ubicom32v3_core_ops.to_has_registers = 1;
  ubicom32v3_core_ops.to_has_execution = 1;
  ubicom32v3_core_ops.to_sections = NULL;
  ubicom32v3_core_ops.to_sections_end = NULL;
  ubicom32v3_core_ops.to_magic = OPS_MAGIC;
  ubicom32v3_core_ops.to_extra_thread_info = ubicom32v3_threads_extra_info;
}
#endif


extern int ubicom32v3pollThreads(unsigned int threadNo, char *args);

unsigned int poll_thread;
static void
myubicom32v3_set_thread (char *args, int from_tty)
{
  sscanf(args, "%d", &poll_thread);
}

static void
ubicom32v3_poll_thread (char *args, int from_tty)
{
  (void) ubicom32v3pollThreads(poll_thread, args);
}

static void
ubicom32v3_trap_ignore(char *args, int from_tty)
{
  ignore_trap = 1;
}

void
_initialize_ubicom32v3 (void)
{
  extern void common_reset(char *args, int from_tty);

  init_ubicom32v3_ops ();

#ifdef DO_CORE_DUMPS
  init_ubicom32v3_core_ops ();
#endif
  add_target (&ubicom32v3_ops);
  add_target (&ubicom32v3_core_ops);
  //add_com ("myreset", class_obscure, ubicom32v3_reset, "reset target.");
  /* Backward compatability.  */
  add_com ("ubicom32v3reset", class_obscure, common_reset,
	   "Backward compatability - use 'reset'.");
  add_com ("tpoll", class_obscure, ubicom32v3_poll_thread,
	   "Thread polling command.");
  add_com ("trapign", class_obscure, ubicom32v3_trap_ignore,
	   "Set debugger to ignore trap events.");
  add_com ("tset", class_obscure, myubicom32v3_set_thread,
	   "Sets thread for polling command.");
  add_com ("specialmon", class_obscure, ubicom32v3_special_mon,
	   "Hardware sampling command.");
  add_com ("bisttest", class_obscure, ubicom32v3_bist_test,
	   "Run bist sequence from gdb.");
  add_com ("plltest", class_obscure, ubicom32v3_pll_test,
	   "Run bist sequence from gdb.");
  add_com ("ddrtest", class_obscure, ubicom32v3_ddr_test,
	   "Run bist sequence from gdb.");
  add_com ("loadtest", class_obscure, ubicom32v3_load,
	   "Run bist sequence from gdb.");
  add_com ("prev_pc", class_obscure, ubicom32v3_previous_pc,
	   "Load PC with value from previous_pc register.");
  add_com ("cacheflush", class_obscure, ubicom32v3_cache_flush,
	   "Flush D-cache and I-cache from for particular address gdb.");
  add_com ("verifyregread", class_obscure, ubicom32v3_verify_reg_read,
	   "Flush D-cache and I-cache from for particular address gdb.");
  add_com ("verify5k", class_obscure, ubicom32v3_verify,
	   "Used to verify contents of flash.");
  add_com ("ocmrun", class_obscure, ubicom32v3_ocm_load_run,
	   "Used to load code into OCM and run it.");
  add_com ("add-module-file", class_obscure, ubicom32v3_add_module_file,
	   "Used to load symbols from a loadable module.");
  add_com ("erase-flash", class_obscure, ubicom32v3_erase_flash,
	   "Used to erase flash. Usage erase-flash start_addr length");
  add_com ("ubicom32v3coredump", class_obscure, ubicom32v3_core_dump,
	   "Used to create a core dump of the machine state.\n\
Usage:\n\
coredump\n\
\tThis will dump core to a file called Ubicom.core \n\twith 2MB of sdram dump.\n\n\
coredump n\n\
\tThis will dump core to a file called Ubicom.core\n\twith nMB of sdram dump.\n\tn is a number between 1 - 16 inclusive.\n\n\
coredump corefilename\n\
\tThis will dump core to a file called \"corefilename\"\n\twith 2MB of sdram dump.\n\n\
coredump corefilename n\n\
\tThis will dump core to a file called \"corefilename\"\n\twith nMB of sdram dump.\n\
\tn is a number between 1 - 16 inclusive.\n");
#if 0
  add_com ("mstat", class_obscure, ubicom32v3_mail_status, "Printout mail box status.");
  add_com ("rdmail", class_obscure, ubicom32v3_read_mailbox, "Read data from mail box.");

  add_com ("mclear", class_obscure, ubicom32v3_mail_clear, "Load the \".protect\". section into Flash Boot sector.");
#endif
}
