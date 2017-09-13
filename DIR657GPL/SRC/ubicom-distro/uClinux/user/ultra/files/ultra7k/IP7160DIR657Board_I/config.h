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
#define APP_BOOTLOADER_RESERVED_SPACE_IN_SECTORS 1

/* Reserved kilobytes for SNV in bootloader sectors */
#define APP_SNV_RESERVED_KB 1

/* Reserved sectors in flash for the kernel */
#define APP_KERNEL_RESERVED_SPACE_IN_SECTORS 0

/* Kernel command line parameter extension for root file systems other than initramfs */
#define APP_BOOTARGS_EXTRA "root=/dev/mtdblock2 rootfstype=squashfs,jffs2 init=/init"

/* Kernel command line */
#define APP_BOOTARGS "mtdparts=ubicom32_boot_flash:256k(bootloader)ro,13824k(upgrade),1536k(jffs2),256k(fw_env),256k(language_pack),256k(artblock)"

/* uClinux memory start address offset */
#define APP_UCLINUX_MEM_START_ADDR 0x00100000

/* Board Name */
#define AP_BOARD_NAME IP7160_DIR657_I_Board

/* Board Name */
#define IP7160_DIR657_I_Board 1

/* Build Bootexec */
#define BOOTEXEC_ULTRA 1

/* Enable U-Boot */
#define APP_UBOOT_ENABLE 1

/* U-Boot RAM image size */
#define APP_UBOOT_MEM_SIZE 0x200000

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
#define DEBUG_TURBO2WIRE_CLK_PORT RA

/* Clock Pin */
#define DEBUG_TURBO2WIRE_CLK_PIN 7

/* Data Port */
#define DEBUG_TURBO2WIRE_TXD_PORT RA

/* Data Pin */
#define DEBUG_TURBO2WIRE_TXD_PIN 6

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
#define DSR_THREAD 7

/* DSR Stack Size */
#define DSR_STACK_SIZE 1024

/**************************************************************/
/* Package: ipEthernetHeader - Ethernet Packet Header Library */
/**************************************************************/
#define IPETHERNETHEADER 1

/***************************************************************/
/* Package: ipEthernetTIO - On-Chip Ethernet Driver VP support */
/***************************************************************/
#define IPETHERNETTIO 1

/* RX Netpages */
#define IPETHERNET_RX_NETPAGES 64

/* TX Buffered Frames */
#define IPETHERNET_TX_FRAMES 4

/* MII Management I/F */
#define IPETHERNET_MII_MGMT_ENABLE 1

/* MDIO port */
#define IPETHERNET_MII_MDIO_PORT RE

/* MDIO pin */
#define IPETHERNET_MII_MDIO_PIN 1

/* MDC port */
#define IPETHERNET_MII_MDC_PORT RE

/* MDC pin */
#define IPETHERNET_MII_MDC_PIN 2

/* Clock cycle time */
#define IPETHERNET_MII_DUTY_CYCLE_ns 100

/* Multiple Instances */
#define IPETHERNET_MI_ENABLED 1

/* eth_lan_Enable ethernet instance */
#define eth_lan_IPETHERNET_INSTANCE_ENABLED 1

/* eth_lan_Use port RI */
#define eth_lan_USE_MII_RI_FOR_ETHERNET 1

/* eth_lan_Select GMII */
#define eth_lan_GMII_RI_USE_GMII 1

/* eth_lan_HRT thread name */
#define eth_lan_IPETHERNET_MII_HRT_THREAD_NAME IPETHERNET_THREAD_NUM

/* eth_lan_Instance Thread Number */
#define eth_lan_IPETHERNET_THREAD_NUM 11

/* eth_lan_Forced full duplex */
#define eth_lan__ETH_FORCED_FULL_DUPLEX 1

/* Flow control */
#define IPETHERNET_FLOW_CONTROL 1

/* Flow control threshold */
#define IPETHERNET_FLOW_CONTROL_PAUSE_THRESH (IPETHERNET_RX_NETPAGES/2)

/*************************************/
/* Package: ipGDB - Debugger support */
/*************************************/
#define IPGDB 1

/* GDB Runtime Support */
#define GDB_RUNTIME_ENABLED 1

/* GDB Thread Number */
#define GDB_THREAD 8

/***********************************************/
/* Package: ipHAL - Hardware Abstraction Layer */
/***********************************************/
#define IPHAL 1

/* Architecture Extension */
#define IP7000_REV2 1

/* System clock frequency */
#define SYSTEM_FREQ 550000000

/* IO PLL frequency */
#define IO_PLL_FREQ 500000000

/* DDR DLL frequency */
#define DDR_PLL_FREQ 226000000

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

/* DDR Family */
#define DDRDRAM 1

/* Part Number */
#define MICRON_MT47H64M16 1

/* DDR data bus width (IP8K+ only) */
#define DDRDRAM_DATA_WIDTH 16

/* Limited DRAM Size */
#define USE_EXTMEM_LIMITED_SIZE 1

/* Full Coredump */
#define FULL_COREDUMP 1

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

/***************************************/
/* Package: ipOneshot - Oneshot Timers */
/***************************************/
#define IPONESHOT 1

/**********************************************/
/* Package: ipPCITIO - PCI TIO driver package */
/**********************************************/
#define IPPCITIO 1

/* Thread Name */
#define IPPCI_HRT_THREAD_NAME IPPCI_THREAD_NUM

/* Thread Number */
#define IPPCI_THREAD_NUM 10

/****************************************************/
/* Package: ipResetOutput - Reset Output Pin Driver */
/****************************************************/
#define IPRESETOUTPUT 1

/* Reset Output - Active Low */
#define RESET_OUTPUT_ACTIVE_LOW 1

/* Reset Port */
#define RESET_OUTPUT_ACTIVE_LOW_PORT RE

/* Reset Pin */
#define RESET_OUTPUT_ACTIVE_LOW_PIN 7

/* Reset Pulse Duration(ms) */
#define RESET_OUTPUT_LOW_TIME 20

/* Reset Hold Time(ms) */
#define RESET_OUTPUT_RELEASE_HIGH_TIME 20

/******************************************/
/* Package: ipSDTIO - SD/SDIO host driver */
/******************************************/
#define IPSDTIO 1

/* Multiple Instances */
#define IPSDTIO_MI_ENABLED 1

/* portf_Enable SDTIO instance */
#define portf_IPSDTIO_INSTANCE_ENABLED 1

/* portf_SD Port */
#define portf_SDTIO_PORT RF

/* portf_Use 250Mhz Clock */
#define portf_SDTIO_CLK_250 1

/* portf_Maximum SD CLK Frequency (HZ) */
#define portf_SDTIO_MAX_CLK 25000000

/* portf_Thread Name */
#define portf_IPSDTIO_HRT_THREAD_NAME IPSDTIO_HRT_THREAD_NUM

/* portf_Thread number */
#define portf_IPSDTIO_HRT_THREAD_NUM 9

/* Runtime Debugging */
#define IPSDTIO_DEBUG 1

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

/***********************************/
/* Package: ipUSBTIO - TIO for USB */
/***********************************/
#define IPUSBTIO 1

/* HRT table definition */

#define HRT_TABLE_DATA \
	IPPCI_THREAD_NUM,\
	ARCH_THREAD_NULL,\
	portf_IPSDTIO_HRT_THREAD_NUM,\
	ARCH_THREAD_NULL,\
	IPPCI_THREAD_NUM,\
	eth_lan_IPETHERNET_THREAD_NUM,\
	portf_IPSDTIO_HRT_THREAD_NUM,\
	ARCH_THREAD_NULL,\
	IPPCI_THREAD_NUM,\
	ARCH_THREAD_NULL,\
	portf_IPSDTIO_HRT_THREAD_NUM,\
	eth_lan_IPETHERNET_THREAD_NUM | ARCH_THREAD_TABLE_END

/* HRT spacing */
#define IPPCI_THREAD_NUM_SPACING 4
#define portf_IPSDTIO_HRT_THREAD_NUM_SPACING 4
#define eth_lan_IPETHERNET_THREAD_NUM_SPACING 6

/* Thread allocation bitmaps */
#define THREAD_HRT_RESERVED 0xe00
#define THREAD_NRT_RESERVED 0x180
#define THREAD_UNASSIGNED   0x07f
