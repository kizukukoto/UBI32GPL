/*
 *  Copyright (c) 2003 Atheros Communications, Inc., All Rights Reserved
 */

/* ar5513reg.h Register definitions for Atheros AR5513 chipset */

#ident  "ACI $Header: //depot/sw/src/dk/mdk/devlib/ar5513/ar5513reg.h#1 $"

// All DSL register accesses should use the appropriate offset added :
//   o  use FLCN_PCI_DSL_XXYYZZ_BASE for addressing over PCI
//   o  use FLCN_MIPS_DSL_XXYYZZ_BASE for addressing over MIPS
//   o  SPECIAL EXCEPTION : PCI interface dsl registers at offset 0x4xyz - they
//                          must use FLCN_PCI_DSL_PCI_ZERO_BASE

// Base offset for PCI addressing
#define FLCN_PCI_DSL_PCI_BASE      0x10000
#define FLCN_PCI_DSL_PCI_ZERO_BASE 0x00000
#define FLCN_PCI_DSL_MPEG_BASE     0x11000
#define FLCN_PCI_DSL_SDRAM_BASE    0x12000
#define FLCN_PCI_DSL_LCLBUS_BASE   0x13000
#define FLCN_PCI_DSL_CONFIG_BASE   0x14000
#define FLCN_PCI_DSL_UART_BASE     0x15000
#define FLCN_PCI_DSL_I2C_BASE      0x16000
#define FLCN_PCI_DSL_SPI_BASE      0x17000
#define FLCN_PCI_DSL_IR_BASE       0x18000

// Base offset for MIPS addressing
#define FLCN_MIPS_DSL_PCI_BASE      0x10100000
#define FLCN_MIPS_DSL_MPEG_BASE     0x10200000
#define FLCN_MIPS_DSL_SDRAM_BASE    0x10300000
#define FLCN_MIPS_DSL_LCLBUS_BASE   0x10400000
#define FLCN_MIPS_DSL_CONFIG_BASE   0x11000000
#define FLCN_MIPS_DSL_UART_BASE     0x11100000
#define FLCN_MIPS_DSL_I2C_BASE      0x11200000
#define FLCN_MIPS_DSL_SPI_BASE      0x11300000
#define FLCN_MIPS_DSL_IR_BASE       0x11400000


// DSL : DMA & PCI Registers in PCI space (usable during sleep)
// WARNING : Remember to use FLCN_PCI_DSL_PCI_ZERO_BASE with this register over PCI
#define FLCN_DSL_PCI_RC                0x4000  // Warm reset control register
#define FLCN_DSL_PCI_RC_OUT_OF_RESET   0x00000000 // MAC reset 
#define FLCN_DSL_PCI_RC_MAC            0x00000001 // MAC reset 
#define FLCN_DSL_PCI_RC_BB             0x00000002 // Baseband reset
#define FLCN_DSL_PCI_RC_RESV0          0x00000004 // Reserved
#define FLCN_DSL_PCI_RC_RESV1          0x00000008 // Reserved
#define FLCN_DSL_PCI_RC_PCI            0x00000010 // PCI-core reset

// DSL : PCI Sleep Control Registers
// WARNING : Remember to use FLCN_PCI_DSL_PCI_ZERO_BASE with this register over PCI
#define FLCN_DSL_PCI_SCR                  0x4004  // Sleep control register
#define FLCN_DSL_PCI_SCR_FORCE_WAKE   0x00000000 // force wake
#define FLCN_DSL_PCI_SCR_FORCE_SLEEP  0x00010000 // force sleep

// WMAC : DMA Control and Interrupt Registers
#define FLCN_MAC_CR                0x0008  // MAC Control Register - only write values of 1 have effect
#define FLCN_MAC_CR_RXE            0x00000004 // Receive enable
#define FLCN_MAC_CR_RXD            0x00000020 // Receive disable
#define FLCN_MAC_CR_SWI            0x00000040 // One-shot software interrupt

#define FLCN_MAC_RXDP              0x000C  // MAC receive queue descriptor pointer


// DSL : RESET/CONFIG REGISTERS
// Use FLCN_PCI_DSL_CONFIG_BASE with this register over PCI
#define FLCN_DSL_CONFIG_COLD_RST             0x0000   // Cold reset control register
#define FLCN_DSL_CONFIG_COLD_RST_AHB         0x00000001   // AHB Cold reset
#define FLCN_DSL_CONFIG_COLD_RST_APB         0x00000002   // AHB Cold reset
#define FLCN_DSL_CONFIG_COLD_RST_CPU_COLD    0x00000004   // CPU Cold reset
#define FLCN_DSL_CONFIG_COLD_RST_CPU_WARM    0x00000008   // CPU Warm reset

#define FLCN_DSL_CONFIG_WARM_RST         0x0004   // Warm reset control register
#define FLCN_DSL_CONFIG_WARM_RST_MAC     0x00000001   // Wmac Warm reset
#define FLCN_DSL_CONFIG_WARM_RST_BB      0x00000002   // BB Warm reset
#define FLCN_DSL_CONFIG_WARM_RST_MPEG    0x00000004   // MPEG Warm reset
#define FLCN_DSL_CONFIG_WARM_RST_PCI     0x00000008   // PCI Warm reset
#define FLCN_DSL_CONFIG_WARM_RST_SDRAM   0x00000010   // SDRAM controller Warm reset
#define FLCN_DSL_CONFIG_WARM_RST_LBUS    0x00000020   // Local Bus Warm reset
#define FLCN_DSL_CONFIG_WARM_RST_I2C     0x00000040   // I2C Warm reset
#define FLCN_DSL_CONFIG_WARM_RST_SPI     0x00000080   // SPI Warm reset
#define FLCN_DSL_CONFIG_WARM_RST_UART    0x00000100   // UART Warm reset
#define FLCN_DSL_CONFIG_WARM_RST_IR      0x00000200   // IR Warm reset
#define FLCN_DSL_CONFIG_WARM_RST_OUT_OF_RESET     0x00000000   // Warm reset out of reset

#define FLCN_DSL_CONFIG_ARB_CTL              0x0008   // AHB Arbitration control config register
#define FLCN_DSL_CONFIG_ARB_CTL_CPU_ARB  0x00000000   // CPU AHB Master req processed
#define FLCN_DSL_CONFIG_ARB_CTL_WMAC_ARB 0x00000001   // WMAC AHB Master req processed
#define FLCN_DSL_CONFIG_ARB_CTL_MPG_ARB  0x00000002   // MPG AHB Master req processed
#define FLCN_DSL_CONFIG_ARB_CTL_LBUS_ARB 0x00000003   // Local Bus AHB Master req processed
#define FLCN_DSL_CONFIG_ARB_CTL_PCI_ARB  0x00000004   // PCI AHB Master req processed
#define FLCN_DSL_CONFIG_ARB_CTL_ALL_ARB  0x0000001F   // PCI AHB Master req processed

#define FLCN_DSL_CONFIG_ENDIAN_CTL            0x000c   // Endianness control config register
#define FLCN_DSL_CONFIG_ENDIAN_CTL_ALL_LITTLE 0x00000000   // All interfaces are Little Endian
#define FLCN_DSL_CONFIG_ENDIAN_CTL_EC_BIG     0x00000001   // EC-to_AHB bridge interface is Big Endian
#define FLCN_DSL_CONFIG_ENDIAN_CTL_WMAC_BIG   0x00000002   // WMAC interface is Big Endian
#define FLCN_DSL_CONFIG_ENDIAN_CTL_MPG_BIG    0x00000004   // MPG interface is Big Endian
#define FLCN_DSL_CONFIG_ENDIAN_CTL_PCI_BIG    0x00000008   // PCI interface is Big Endian
#define FLCN_DSL_CONFIG_ENDIAN_CTL_SDRAM_BIG  0x00000010   // SDRAM interface is Big Endian
#define FLCN_DSL_CONFIG_ENDIAN_CTL_LBUS_BIG   0x00000020   // LBUS interface is Big Endian
#define FLCN_DSL_CONFIG_ENDIAN_CTL_I2C_BIG    0x00000040   // I2C interface is Big Endian
#define FLCN_DSL_CONFIG_ENDIAN_CTL_SPI_BIG    0x00000080   // SPI interface is Big Endian
#define FLCN_DSL_CONFIG_ENDIAN_CTL_UART_BIG   0x00000100   // UART interface is Big Endian
#define FLCN_DSL_CONFIG_ENDIAN_CTL_CPU_BIG    0x00000200   // CPU interface is Big Endian

#define FLCN_DSL_CONFIG_IF_CTL                  0x0018       // Interface control config register
#define FLCN_DSL_CONFIG_IF_CTL_DISABLE_ALL  0x00000000   // Disable PCI, MPG, LBUS
#define FLCN_DSL_CONFIG_IF_CTL_EN_PCI_ONLY  0x00000001   // Enable PCI interface. Disable MPG, LBUS
#define FLCN_DSL_CONFIG_IF_CTL_EN_MPG_LBUS  0x00000002   // Disable PCI interface. Enable MPG, LBUS
#define FLCN_DSL_CONFIG_IF_CTL_EN_ALL       0x00000003   // Enable PCI, MPG, LBUS - Emulation ONLY


// DSL : SDRAM CONTROLLER Registers in PCI space 
#define FLCN_DSL_SDRAM_REFRESH                0x0010     // SDRAM Refresh Interval register
#define FLCN_DSL_SDRAM_REFRESH_MDK_SETTING    0x00000096 // functional setting for mdk. see DW_memctl databook pg. 104.


// PHY registers
#define PHY_BASE_CHAIN0            0x9800  // PHY registers base address for chain0
#define PHY_BASE_CHAIN1            0xa800  // PHY registers base address for chain0
#define PHY_BASE_CHAIN_BOTH        0xb800  // PHY registers base address for chain0

// MAC PCU Registers
#define F2_STA_ID0          0x8000  // MAC station ID0 register - low 32 bits
#define F2_STA_ID1          0x8004  // MAC station ID1 register - upper 16 bits
#define F2_STA_ID1_SADH_MASK   0x0000FFFF // Mask for upper 16 bits of MAC addr
#define F2_STA_ID1_STA_AP      0x00010000 // Device is AP
#define F2_STA_ID1_AD_HOC      0x00020000 // Device is ad-hoc
#define F2_STA_ID1_PWR_SAV     0x00040000 // Power save reporting in self-generated frames
#define F2_STA_ID1_KSRCHDIS    0x00080000 // Key search disable
#define F2_STA_ID1_PCF		   0x00100000 // Observe PCF
#define F2_STA_ID1_USE_DEFANT  0x00200000 // Use default antenna
#define F2_STA_ID1_DEFANT_UPDATE 0x00400000 // Update default antenna w/ TX antenna
#define F2_STA_ID1_RTS_USE_DEF   0x00800000 // Use default antenna to send RTS
#define F2_STA_ID1_ACKCTS_6MB  0x01000000 // Use 6Mb/s rate for ACK & CTS


#define F2_DEF_ANT			0x8058 //default antenna register

#define F2_RXDP             0x000C  // MAC receive queue descriptor pointer

#define F2_QCU_0		  0x0001

#define F2_IMR_S0             0x00a4 // MAC Secondary interrupt mask register 0
//#define F2_IMR_S0_QCU_TXOK_M    0x0000FFFF // Mask for TXOK (QCU 0-15)
#define F2_IMR_S0_QCU_TXDESC_M  0xFFFF0000 // Mask for TXDESC (QCU 0-15)
#define F2_IMR_S0_QCU_TXDESC_S  16		   // Shift for TXDESC (QCU 0-15)

#define F2_IMR               0x00a0  // MAC Primary interrupt mask register
#define F2_IMR_AR5513        0x00a0  // MAC Primary interrupt mask register
#define F2_IMR_TXDESC        0x00000080 // Transmit interrupt request

// Interrupt status registers (read-and-clear access, secondary shadow copies)
#define F2_ISR_RAC           0x00c0 // MAC Primary interrupt status register,
#define F2_ISR               0x0080 // MAC Primary interrupt status register,
#define F2_ISR_AR5513        0x0080 // MAC Primary interrupt status register,

#define F2_IER               0x0024  // MAC Interrupt enable register
#define F2_IER_AR5513        0x0024  // MAC Interrupt enable register
#define F2_IER_ENABLE        0x00000001 // Global interrupt enable
#define F2_IER_DISABLE       0x00000000 // Global interrupt disable

#define F2_Q0_TXDP           0x0800 // MAC Transmit Queue descriptor pointer

#define F2_RX_FILTER         0x803C  // MAC receive filter register
#define F2_RX_FILTER_ALL     0x00000000 // Disallow all frames
#define F2_RX_UCAST          0x00000001 // Allow unicast frames
#define F2_RX_MCAST          0x00000002 // Allow multicast frames
#define F2_RX_BCAST          0x00000004 // Allow broadcast frames
#define F2_RX_CONTROL        0x00000008 // Allow control frames
#define F2_RX_BEACON         0x00000010 // Allow beacon frames
#define F2_RX_PROM           0x00000020 // Promiscuous mode, all packets

#define F2_DIAG_SW           0x8048  // MAC PCU control register
#define F2_DIAG_RX_DIS       0x00000020 // disable receive
#define F2_DIAG_CHAN_INFO    0x00000100 // dump channel info
#define F2_DUAL_CHAIN_CHAN_INFO    0x01000000 // dump channel info

#define F2_CR                0x0008  // MAC Control Register - only write values of 1 have effect
#define F2_CR_RXE            0x00000004 // Receive enable
#define F2_CR_RXD            0x00000020 // Receive disable
#define F2_CR_SWI            0x00000040 // One-shot software interrupt

#define F2_Q_TXE             0x0840 // MAC Transmit Queue enable
#define F2_Q_TXE_M			  0x0000FFFF // Mask for TXE (QCU 0-15)

#define F2_QDCKLGATE         0x005c // MAC QCU/DCU clock gating control register
#define F2_QDCKLGATE_QCU_M    0x0000FFFF // Mask for QCU clock disable 
#define F2_QDCKLGATE_DCU_M    0x07FF0000 // Mask for DCU clock disable 

#define F2_DIAG_ENCRYPT_DIS  0x00000008 // disable encryption

#define F2_D0_LCL_IFS     0x1040 // MAC DCU-specific IFS settings
#define F2_D_LCL_IFS_AIFS_M		   0x0FF00000 // Mask for AIFS
#define F2_D_LCL_IFS_AIFS_S		   20         // Shift for AIFS

#define F2_D_LCL_IFS_CWMIN_M	   0x000003FF // Mask for CW_MIN
#define F2_D_LCL_IFS_CWMAX_M	   0x000FFC00 // Mask for CW_MAX

#define F2_D0_RETRY_LIMIT	  0x1080 // MAC Retry limits
#define F2_Q0_MISC         0x09c0 // MAC Miscellaneous QCU settings

#define F2_D_GBL_IFS_SIFS	0x1030 // MAC DCU-global IFS settings: SIFS duration
#define F2_D_GBL_IFS_EIFS	0x10b0 // MAC DCU-global IFS settings: EIFS duration

#define F2_D_FPCTL			0x1230		//Frame prefetch

#define F2_TIME_OUT         0x8014  // MAC ACK & CTS time-out

#define F2_Q_TXD             0x0880 // MAC Transmit Queue disable

#define F2_KEY_CACHE_START          0x8800  // keycache start address

#define F2_IMR_RXDESC        0x00000002 // Receive interrupt request

#define F2_RPGTO            0x0050  // MAC receive frame gap timeout
#define F2_RPGTO_MASK        0x000003FF // Mask for receive frame gap timeout

#define F2_RPCNT            0x0054 // MAC receive frame count limit
#define F2_RPCNT_MASK        0x0000001F // Mask for receive frame count limit

#define PHY_FRAME_CONTROL1   0x9944  // rest of old PHY frame control register

#define FALCON_COARSE_DELAY  4   // in units of 0.4 ns (needs to be empirically determined)
#define FALCON_FINE_DELAY    0   // in units of 0.1 ns (needs to be empirically determined)

/*
 * Access macros for mips soc registers.
 */
#define sysRegRead(phys)        \
        (*(volatile unsigned int *)PHYS_TO_K1(phys))
#define sysRegWrite(phys,val)   \
        ((*(volatile unsigned int *)PHYS_TO_K1(phys)) = (val))
