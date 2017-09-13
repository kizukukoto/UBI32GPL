/* Dynamic debugging is globally disabled. No dbg_lvl struct was generated */

/* Enable Global Debug */
#define DEBUG 1

/* Project Name */
#define PROJECT_NAME ultra

/* Original File Name */
#define PKG_FILE_NAME "TemplateUBICOM32.lpj"

/* Architecture */
#define ARCH IP3K

/************************/
/* Package: Application */
/************************/
#define APPLICATION 1

/* Firmware Identity String */
#define APP_IDENTITY_STRING "Unknown - BOARD"

/* Reserved sectors in flash for the bootloader */
#define APP_BOOTLOADER_RESERVED_SPACE_IN_SECTORS 2

/* Reserved kilobytes for SNV in bootloader sectors */
#define APP_SNV_RESERVED_KB 1

/* Reserved sectors in flash for the kernel */
#define APP_KERNEL_RESERVED_SPACE_IN_SECTORS 0

/* Kernel command line parameter extension for root file systems other than initramfs */
#define APP_BOOTARGS_EXTRA "root=/dev/mtdblock2 rootfstype=squashfs,jffs2 init=/init"

/* Kernel command line */
#define APP_BOOTARGS "mtdparts=ubicom32fc.0:512k(bootloader)ro,13568k(rootfs),1536k(rootfs_data),256k(fw_env),256k(language_pack),256k(artblock)"

/* uClinux memory start address offset */
#define APP_UCLINUX_MEM_START_ADDR 0x00100000

/* Board Name */
#define AP_BOARD_NAME IP8100_DIR857_BOARD

/* Board Name */
#define IP8100_DIR857_BOARD 1

/* Build Bootexec */
#define BOOTEXEC_ULTRA 1

/* Enable U-Boot */
#define APP_UBOOT_ENABLE 1

/* U-Boot RAM image size */
#define APP_UBOOT_MEM_SIZE 0x100000

/* Environment variables space size in sectors */
#define APP_UBOOT_ENV_SIZE_IN_SECTORS 1

/*********************************************************/
/* Package: ipBootDecompressor - Bootstrap decompressor. */
/*********************************************************/
#define IPBOOTDECOMPRESSOR 1

/********************************************/
/* Package: ipDebug - Runtime Debug Support */
/********************************************/
#define IPDEBUG 1

/* Serial Interface */
#define DEBUG_SERIAL_ENABLED 1

/* Turbo 2 wire (IO based) */
#define USE_TURBO2WIRE_FOR_DEBUG 1

/* Clock Port */
#define DEBUG_TURBO2WIRE_CLK_PORT PG4

/* Clock Pin */
#define DEBUG_TURBO2WIRE_CLK_PIN 18

/* Data Port */
#define DEBUG_TURBO2WIRE_TXD_PORT PG4

/* Data Pin */
#define DEBUG_TURBO2WIRE_TXD_PIN 19

/* Maximum bit rate */
#define DEBUG_TURBO2WIRE_BAUD_RATE 5000000

/************************************/
/* Package: ipDevTree - Device Tree */
/************************************/
#define IPDEVTREE 1

/***************************************************/
/* Package: ipDSR - Device Service Routine Support */
/***************************************************/
#define IPDSR 1

/* DSR thread number */
#define DSR_THREAD 10

/* DSR Stack Size */
#define DSR_STACK_SIZE 1024

/***************************************************************/
/* Package: ipEthernetDMA - On-Chip Ethernet Driver VP support */
/***************************************************************/
#define IPETHERNETDMA 1

/* MII Management I/F */
#define IPETHERNET_MII_MGMT_ENABLE 1

/* Multiple Instances */
#define IPETHERNET_MI_ENABLED 1

/* eth_lan_Enable ethernet instance */
#define eth_lan_IPETHERNET_INSTANCE_ENABLED 1

/* eth_lan_Use port RH */
#define eth_lan_USE_MII_RH_FOR_ETHERNET 1

/* eth_lan_Select GMII */
#define eth_lan_GMII_RH_USE_GMII 1

/* eth_lan_Forced full duplex */
#define eth_lan__ETH_FORCED_FULL_DUPLEX 1

/* eth_wan_Enable ethernet instance */
#define eth_wan_IPETHERNET_INSTANCE_ENABLED 1

/* eth_wan_Use port RI */
#define eth_wan_USE_MII_RI_FOR_ETHERNET 1

/* eth_wan_Select GMII */
#define eth_wan_GMII_RI_USE_GMII 1

/* eth_wan_Select I/O pin map */
#define eth_wan_GMII_RI_PORT_SELETION PG3

/* eth_wan_Auto-detect */
#define eth_wan__ETH_AUTO_LINK_MODE 1

/* eth_wan_Get link state from PHY */
#define eth_wan__ETH_GET_LINK_FROM_PHY 1

/* eth_wan_PHY Address */
#define eth_wan__ETH_PHY_ADDRESS 4

/* Flow control */
#define IPETHERNET_FLOW_CONTROL 1

/**************************************************************/
/* Package: ipEthernetHeader - Ethernet Packet Header Library */
/**************************************************************/
#define IPETHERNETHEADER 1

/************************************************************************/
/* Package: ipEthSwitchDev - Driver for external Ethernet Switch Device */
/************************************************************************/
#define IPETHSWITCHDEV 1

/* Atheros AR8327 - Gigabit */
#define ETHSWITCH_ATHEROS_AR8327 1

/*************************************/
/* Package: ipGDB - Debugger support */
/*************************************/
#define IPGDB 1

/* GDB Runtime Support */
#define GDB_RUNTIME_ENABLED 1

/* GDB Thread Number */
#define GDB_THREAD 11

/***********************************************/
/* Package: ipHAL - Hardware Abstraction Layer */
/***********************************************/
#define IPHAL 1

/* Architecture Extension */
#define IP8000_PROD 1

/* System clock frequency */
#define SYSTEM_FREQ 600000000

/* IO PLL frequency */
#define IO_PLL_FREQ 500000000

/* DDR DLL frequency */
#define DDR_PLL_FREQ 533000000

/* External Flash */
#define USE_EXTFLASH 1

/* Min total size (k) */
#define EXTFLASH_MIN_TOTAL_SIZE_KB 16384

/* Max page size (k) */
#define EXTFLASH_MAX_PAGE_SIZE_KB 256

/* Max access time (ns) */
#define EXTFLASH_MAX_ACCESS_TIME 20

/* External DRAM */
#define USE_EXTMEM 1

/* DDR3 Family */
#define DDR3DRAM 1

/* DDR3 max. size (MB) */
#define DDR3DRAM_MAX_SIZE 256

/* DDR3 data bus width */
#define DDR3DRAM_DATA_WIDTH 32

/* Limited DRAM Size */
#define USE_EXTMEM_LIMITED_SIZE 1

/* Full Coredump */
#define FULL_COREDUMP 1

/* External SPI-ER NAND Flash */
#define SPI_ER_NAND 1

/* Enable Gate Port */
#define SPI_ER_NAND_PORT PG5

/* Enable Gate Pin */
#define SPI_ER_NAND_PIN 2

/*****************************************************/
/* Package: ipHeap - Heap Memory Management Features */
/*****************************************************/
#define IPHEAP 1

/* OCM Heap Size */
#define HEAP_OCM_HEAP_KB 4

/************************************************************/
/* Package: ipInterrupt - Hardware Interrupt Infrastructure */
/************************************************************/
#define IPINTERRUPT 1

/****************************************/
/* Package: ipLibC - Standard C library */
/****************************************/
#define IPLIBC 1

/**************************************************************/
/* Package: ipLock - Lock Implementation for multi-threading. */
/**************************************************************/
#define IPLOCK 1

/****************************************************/
/* Package: ipMACAddr - MAC Address Support Library */
/****************************************************/
#define IPMACADDR 1

/************************************/
/* Package: ipMII - MII Phy Control */
/************************************/
#define IPMII 1

/* MDIO port */
#define MII_MDIO_PORT PG3

/* MDIO pin */
#define MII_MDIO_PIN 7

/* MDC port */
#define MII_MDC_PORT PG3

/* MDC pin */
#define MII_MDC_PIN 4

/* Clock cycle time */
#define MII_DUTY_CYCLE 100

/***************************************/
/* Package: ipOneshot - Oneshot Timers */
/***************************************/
#define IPONESHOT 1

/************************************************/
/* Package: ipPCIE - PCI Express driver package */
/************************************************/
#define IPPCIE 1

/* Port J PCIe Enable */
#define IPPCIE_USE_RJ 1

/****************************************************/
/* Package: ipResetOutput - Reset Output Pin Driver */
/****************************************************/
#define IPRESETOUTPUT 1

/* Reset Output - Active Low */
#define RESET_OUTPUT_ACTIVE_LOW 1

/* Reset Port */
#define RESET_OUTPUT_ACTIVE_LOW_PORT PG4

/* Reset Pin */
#define RESET_OUTPUT_ACTIVE_LOW_PIN 14

/* Reset Pulse Duration(ms) */
#define RESET_OUTPUT_LOW_TIME 1

/* Reset Hold Time(ms) */
#define RESET_OUTPUT_RELEASE_HIGH_TIME 10

/***************************************************************/
/* Package: ipThread - Hardware Multi-Threading Infrastructure */
/***************************************************************/
#define IPTHREAD 1

/****************************************/
/* Package: ipTimer - System Tick Timer */
/****************************************/
#define IPTIMER 1

/* Tick rate */
#define TICK_RATE 1000

/* HRT table definition */

#define HRT_TABLE_DATA \
	 ARCH_THREAD_NULL | ARCH_THREAD_TABLE_END

/* Thread allocation bitmaps */
#define THREAD_HRT_RESERVED 0x000
#define THREAD_NRT_RESERVED 0xc00
#define THREAD_UNASSIGNED   0x3ff
