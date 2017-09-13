
# Section: Global Settings

# Enable Global Debug
DEBUG = 1

# Enable all Assertions

# Project Name
PROJECT_NAME = ultra

# Section: Internal Settings

# Build Directory Path
BUILD_DIR = $(PROJECT_DIR)/build

# SDK directory
SDK_DIR = /media/disk/trunk/ubicom-distro/ultra

# Compiler Flags
GLOBAL_CFLAGS = -Os -g -fgnu89-inline -fleading-underscore

# Architecture Directory
ARCH_DIR = ip3k

# Architecture
ARCH = IP3K

# Package: Application
APPLICATION = 1

# Firmware Identity String
APP_IDENTITY_STRING = Unknown - BOARD

# Reserved sectors in flash for the bootloader
APP_BOOTLOADER_RESERVED_SPACE_IN_SECTORS = 1

# Reserved kilobytes for SNV in bootloader sectors
APP_SNV_RESERVED_KB = 1

# Reserved sectors in flash for the kernel
APP_KERNEL_RESERVED_SPACE_IN_SECTORS = 0

# Section: uClinux

# Kernel command line parameter extension for root file systems other than initramfs
APP_BOOTARGS_EXTRA = root=/dev/mtdblock2 rootfstype=squashfs,jffs2 init=/init

# Kernel command line
APP_BOOTARGS = mtdparts=ubicom32_boot_flash:256k(bootloader)ro,13824k(upgrade),1536k(jffs2),256k(fw_env),256k(language_pack),256k(artblock)

# uClinux memory start address offset
APP_UCLINUX_MEM_START_ADDR = 0x00100000

# Board Name
AP_BOARD_NAME = IP7160_DIR657_I_Board

# Board Name
IP7160_DIR657_I_Board = 1

# Build Bootexec
BOOTEXEC_ULTRA = 1

# Enable U-Boot
APP_UBOOT_ENABLE = 1

# U-Boot Path
APP_UBOOT_DIR = /media/disk/trunk/ubicom-distro/u-boot

# U-Boot RAM image size
APP_UBOOT_MEM_SIZE = 0x200000

# Environment variables space size in sectors
APP_UBOOT_ENV_SIZE_IN_SECTORS = 1

# Build Main Exec

# Package: ipBootDecompressor - Bootstrap decompressor.
IPBOOTDECOMPRESSOR = 1

# Package directory name
IPBOOTDECOMPRESSOR_PKG_DIR = ipBootDecompressor

# Package sub-directories
PKG_SUBDIRS += $(IPBOOTDECOMPRESSOR_PKG_DIR)

# Package: ipDebug - Runtime Debug Support
IPDEBUG = 1

# Package directory name
IPDEBUG_PKG_DIR = ipDebug

# Package sub-directories
PKG_SUBDIRS += $(IPDEBUG_PKG_DIR)

# Package: ipDevTree - Device Tree
IPDEVTREE = 1

# Package directory name
IPDEVTREE_PKG_DIR = ipDevTree

# Package sub-directories
PKG_SUBDIRS += $(IPDEVTREE_PKG_DIR)

# Package: ipDSR - Device Service Routine Support
IPDSR = 1

# Package directory name
IPDSR_PKG_DIR = ipDSR

# Package sub-directories
PKG_SUBDIRS += $(IPDSR_PKG_DIR)

# Package: ipEthernetHeader - Ethernet Packet Header Library
IPETHERNETHEADER = 1

# Package directory name
IPETHERNETHEADER_PKG_DIR = ipEthernetHeader

# Package sub-directories
PKG_SUBDIRS += $(IPETHERNETHEADER_PKG_DIR)

# Package: ipEthernetTIO - On-Chip Ethernet Driver VP support
IPETHERNETTIO = 1

# Package directory name
IPETHERNETTIO_PKG_DIR = ipEthernetTIO

# Package sub-directories
PKG_SUBDIRS += $(IPETHERNETTIO_PKG_DIR)

# Multiple Instances
IPETHERNET_MI_ENABLED = 1
IPETHERNET_MI_ENABLED_INSTANCES += eth_lan_

# Section: eth_lan_HRT settings

# Section: eth_lan_Link mode options

# Package: ipGDB - Debugger support
IPGDB = 1

# Package directory name
IPGDB_PKG_DIR = ipGDB

# Package sub-directories
PKG_SUBDIRS += $(IPGDB_PKG_DIR)

# Package: ipHAL - Hardware Abstraction Layer
IPHAL = 1

# Package directory name
IPHAL_PKG_DIR = ipHAL

# Package sub-directories
PKG_SUBDIRS += $(IPHAL_PKG_DIR)

# Architecture Extension
IP7000_REV2 = 1

# Section: Timing

# External Flash
USE_EXTFLASH = 1

# Max page size (k)
EXTFLASH_MAX_PAGE_SIZE_KB = 256

# External DRAM
USE_EXTMEM = 1

# External SPI-ER NAND Flash

# Stack Checking

# Package: ipHeap - Heap Memory Management Features
IPHEAP = 1

# Package directory name
IPHEAP_PKG_DIR = ipHeap

# Package sub-directories
PKG_SUBDIRS += $(IPHEAP_PKG_DIR)

# Runtime Debugging

# Package: ipInterrupt - Hardware Interrupt Infrastructure
IPINTERRUPT = 1

# Package directory name
IPINTERRUPT_PKG_DIR = ipInterrupt

# Package sub-directories
PKG_SUBDIRS += $(IPINTERRUPT_PKG_DIR)

# Package: ipLibC - Standard C library
IPLIBC = 1

# Package directory name
IPLIBC_PKG_DIR = ipLibC

# Package sub-directories
PKG_SUBDIRS += $(IPLIBC_PKG_DIR)

# Package: ipLock - Lock Implementation for multi-threading.
IPLOCK = 1

# Package directory name
IPLOCK_PKG_DIR = ipLock

# Package sub-directories
PKG_SUBDIRS += $(IPLOCK_PKG_DIR)

# Package: ipMACAddr - MAC Address Support Library
IPMACADDR = 1

# Package directory name
IPMACADDR_PKG_DIR = ipMACAddr

# Package sub-directories
PKG_SUBDIRS += $(IPMACADDR_PKG_DIR)

# Package: ipOneshot - Oneshot Timers
IPONESHOT = 1

# Package directory name
IPONESHOT_PKG_DIR = ipOneshot

# Package sub-directories
PKG_SUBDIRS += $(IPONESHOT_PKG_DIR)

# Package: ipPCITIO - PCI TIO driver package
IPPCITIO = 1

# Package directory name
IPPCITIO_PKG_DIR = ipPCITIO

# Package sub-directories
PKG_SUBDIRS += $(IPPCITIO_PKG_DIR)

# Section: PCI Devices

# Section: HRT settings

# Package: ipResetOutput - Reset Output Pin Driver
IPRESETOUTPUT = 1

# Package directory name
IPRESETOUTPUT_PKG_DIR = ipResetOutput

# Package sub-directories
PKG_SUBDIRS += $(IPRESETOUTPUT_PKG_DIR)

# Package: ipSDTIO - SD/SDIO host driver
IPSDTIO = 1

# Package directory name
IPSDTIO_PKG_DIR = ipSDTIO

# Package sub-directories
PKG_SUBDIRS += $(IPSDTIO_PKG_DIR)

# Multiple Instances
IPSDTIO_MI_ENABLED = 1
IPSDTIO_MI_ENABLED_INSTANCES += portf_

# Section: portf_HRT settings

# Package: ipThread - Hardware Multi-Threading Infrastructure
IPTHREAD = 1

# Package directory name
IPTHREAD_PKG_DIR = ipThread

# Package sub-directories
PKG_SUBDIRS += $(IPTHREAD_PKG_DIR)

# Package: ipTimer - System Tick Timer
IPTIMER = 1

# Package directory name
IPTIMER_PKG_DIR = ipTimer

# Package sub-directories
PKG_SUBDIRS += $(IPTIMER_PKG_DIR)

# Package: ipUSBTIO - TIO for USB
IPUSBTIO = 1

# Package directory name
IPUSBTIO_PKG_DIR = ipUSBTIO

# Package sub-directories
PKG_SUBDIRS += $(IPUSBTIO_PKG_DIR)
