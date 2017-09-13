#ifndef __ARREGH__
#define __ARREGH__


/*
 *  Copyright (c) 2001 Atheros Communications, Inc., All Rights Reserved
 */

/* ar5211reg.h Register definitions for Atheros AR5211 chipset */

#ident  "ACI $Header: //depot/sw/src/dk/mdk/devlib/ar5211/ar5211reg.h#10 $"

// DMA Control and Interrupt Registers
#define F2_CR               0x0008  // MAC Control Register - only write values of 1 have effect
#define F2_CR_RXE            0x00000004 // Receive enable
#define F2_CR_RXD            0x00000020 // Receive disable
#define F2_CR_SWI            0x00000040 // One-shot software interrupt

#define F2_RXDP             0x000C  // MAC receive queue descriptor pointer

#define F2_CFG              0x0014  // MAC configuration and status register
#define F2_CFG_SWTD          0x00000001 // byteswap tx descriptor words
#define F2_CFG_SWTB          0x00000002 // byteswap tx data buffer words
#define F2_CFG_SWRD          0x00000004 // byteswap rx descriptor words
#define F2_CFG_SWRB          0x00000008 // byteswap rx data buffer words
#define F2_CFG_SWRG          0x00000010 // byteswap register access data words
#define F2_CFG_AP_ADHOC_INDICATION				0x00000020 // AP/adhoc indication (0-AP, 1-Adhoc)
#define F2_CFG_PHOK          0x00000100 // PHY OK status
#define F2_CFG_EEBS          0x00000200 // EEPROM busy
#define F2_CFG_PCI_MASTER_REQ_Q_THRESH_M       0x00060000 // Mask of PCI core master request queue full threshold
#define F2_CFG_PCI_MASTER_REQ_Q_THRESH_S       17         // Shift for PCI core master request queue full threshold


#define F2_IER              0x0024  // MAC Interrupt enable register
#define F2_IER_ENABLE        0x00000001 // Global interrupt enable
#define F2_IER_DISABLE       0x00000000 // Global interrupt disable


#define F2_RTSD0            0x0028  // MAC RTS Duration Parameters 0
#define F2_RTSD0_RTS_DURATION_6_M	0x000000FF	
#define F2_RTSD0_RTS_DURATION_6_S	0
#define F2_RTSD0_RTS_DURATION_9_M	0x0000FF00	
#define F2_RTSD0_RTS_DURATION_9_S	8
#define F2_RTSD0_RTS_DURATION_12_M	0x00FF0000	
#define F2_RTSD0_RTS_DURATION_12_S	16	
#define F2_RTSD0_RTS_DURATION_18_M	0xFF000000
#define F2_RTSD0_RTS_DURATION_18_S	24

#define F2_RTSD1            0x002c  // MAC RTS Duration Parameters 1
#define F2_RTSD0_RTS_DURATION_24_M	0x000000FF	
#define F2_RTSD0_RTS_DURATION_24_S	0
#define F2_RTSD0_RTS_DURATION_36_M	0x0000FF00	
#define F2_RTSD0_RTS_DURATION_36_S	8
#define F2_RTSD0_RTS_DURATION_48_M	0x00FF0000
#define F2_RTSD0_RTS_DURATION_48_S	16
#define F2_RTSD0_RTS_DURATION_54_M	0xFF000000
#define F2_RTSD0_RTS_DURATION_54_S	24


#define F2_TXCFG            0x0030  // MAC tx DMA size config register
#define F2_TXCFG_CONT_EN     0x00000008 // Enable continuous transmit mode

#define F2_RXCFG            0x0034  // MAC rx DMA size config register

#define F2_RXCFG_DEF_RX_ANTENNA		0x00000008 // Default Receive Antenna
#define F2_RXCFG_ZLFDMA      0x00000010 // Enable DMA of zero-length frame
#define F2_DMASIZE_4B        0x00000000 // DMA size 4 bytes 
#define F2_DMASIZE_8B        0x00000001 // DMA size 8 bytes 
#define F2_DMASIZE_16B       0x00000002 // DMA size 16 bytes 
#define F2_DMASIZE_32B       0x00000003 // DMA size 32 bytes 
#define F2_DMASIZE_64B       0x00000004 // DMA size 64 bytes 
#define F2_DMASIZE_128B      0x00000005 // DMA size 128 bytes 
#define F2_DMASIZE_256B      0x00000006 // DMA size 256 bytes 
#define F2_DMASIZE_512B      0x00000007 // DMA size 512 bytes 

#define F2_MIBC             0x0040  // MAC MIB control register
#define F2_MIBC_COW          0x00000001 // counter overflow warning
#define F2_MIBC_FMC          0x00000002 // freeze MIB counters
#define F2_MIBC_CMC          0x00000004 // clear MIB counters
#define F2_MIBC_MCS          0x00000008 // MIB counter strobe, increment all

#define F2_TOPS             0x0044  // MAC timeout prescale count
#define F2_TOPS_MASK         0x0000FFFF // Mask for timeout prescale

#define F2_RXNPTO           0x0048  // MAC no frame received timeout
#define F2_RXNPTO_MASK       0x000003FF // Mask for no frame received timeout

#define F2_TXNPTO           0x004C  // MAC no frame trasmitted timeout
#define F2_TXNPTO_MASK       0x000003FF // Mask for no frame transmitted timeout
#define F2_TXNPTO_QCU_MASK   0x03FFFC00 // Mask indicating the set of QCUs
									// for which frame completions will cause
									// a reset of the no frame transmitted timeout 

#define F2_RPGTO            0x0050  // MAC receive frame gap timeout
#define F2_RPGTO_MASK        0x000003FF // Mask for receive frame gap timeout

#define F2_RPCNT            0x0054 // MAC receive frame count limit
#define F2_RPCNT_MASK        0x0000001F // Mask for receive frame count limit
  
#define F2_MACMISC           0x0058 // MAC miscellaneous control/status register
#define F2_MACMISC_DMA_OBS_M    0x000001E0 // Mask for DMA observation bus mux select
#define F2_MACMISC_DMA_OBS_S    5          // Shift for DMA observation bus mux select
#define F2_MACMISC_MISC_OBS_M   0x00000E00 // Mask for MISC observation bus mux select
#define F2_MACMISC_MISC_OBS_S   9          // Shift for MISC observation bus mux select
#define F2_MACMISC_MAC_OBS_BUS_LSB_M   0x00007000 // Mask for MAC observation bus mux select (lsb)
#define F2_MACMISC_MAC_OBS_BUS_LSB_S   12         // Shift for MAC observation bus mux select (lsb)
#define F2_MACMISC_MAC_OBS_BUS_MSB_M   0x00038000 // Mask for MAC observation bus mux select (msb)
#define F2_MACMISC_MAC_OBS_BUS_MSB_S   15         // Shift for MAC observation bus mux select (msb)

#define F2_QDCKLGATE         0x005c // MAC QCU/DCU clock gating control register
#define F2_QDCKLGATE_QCU_M    0x0000FFFF // Mask for QCU clock disable 
#define F2_QDCKLGATE_DCU_M    0x07FF0000 // Mask for DCU clock disable 

// Interrupt Status Registers

#define F2_ISR             0x0080 // MAC Primary interrupt status register
#define F2_ISR_RXOK          0x00000001 // At least one frame received sans errors
#define F2_ISR_RXDESC        0x00000002 // Receive interrupt request
#define F2_ISR_RXERR         0x00000004 // Receive error interrupt
#define F2_ISR_RXNOPKT       0x00000008 // No frame received within timeout clock
#define F2_ISR_RXEOL         0x00000010 // Received descriptor empty interrupt
#define F2_ISR_RXORN         0x00000020 // Receive FIFO overrun interrupt
#define F2_ISR_TXOK          0x00000040 // Transmit okay interrupt
#define F2_ISR_TXDESC        0x00000080 // Transmit interrupt request
#define F2_ISR_TXERR         0x00000100 // Transmit error interrupt
#define F2_ISR_TXNOPKT       0x00000200 // No frame transmitted interrupt
#define F2_ISR_TXEOL         0x00000400 // Transmit descriptor empty interrupt
#define F2_ISR_TXURN         0x00000800 // Transmit FIFO underrun interrupt
#define F2_ISR_MIB           0x00001000 // MIB interrupt - see MIBC
#define F2_ISR_SWI           0x00002000 // Software interrupt
#define F2_ISR_RXPHY         0x00004000 // PHY receive error interrupt
#define F2_ISR_RXKCM         0x00008000 // Key-cache miss interrupt
#define F2_ISR_SWBA          0x00010000 // Software beacon alert interrupt
#define F2_ISR_BRSSI         0x00020000 // Beacon threshold interrupt
#define F2_ISR_BMISS         0x00040000 // Beacon missed interrupt
#define F2_ISR_HIUERR        0x00080000 // An unexpected bus error has occurred
#define F2_ISR_BNR		   0x00100000 // Beacon not ready interrupt
#define F2_ISR_TIM		   0x00800000 // TIM interrupt
#define F2_ISR_GPIO          0x01000000 // GPIO Interrupt
#define F2_ISR_QCBROVF       0x02000000 // QCU CBR overflow interrupt
#define F2_ISR_QCBRURN       0x04000000 // QCU CBR underrun interrupt
#define F2_ISR_QTRIG         0x08000000 // QCU scheduling trigger interrupt
#define F2_ISR_RESV0         0xF0000000 // Reserved

#define F2_ISR_S0             0x0084 // MAC Secondary interrupt status register 0
#define F2_ISR_S0_QCU_TXOK_M    0x0000FFFF // Mask for TXOK (QCU 0-15)
#define F2_ISR_S0_QCU_TXDESC_M  0xFFFF0000 // Mask for TXDESC (QCU 0-15)

#define F2_ISR_S1             0x0088 // MAC Secondary interrupt status register 1
#define F2_ISR_S1_QCU_TXERR_M  0x0000FFFF // Mask for TXERR (QCU 0-15)
#define F2_ISR_S1_QCU_TXEOL_M  0xFFFF0000 // Mask for TXEOL (QCU 0-15)
 
#define F2_ISR_S2             0x008c // MAC Secondary interrupt status register 2
#define F2_ISR_S2_QCU_TXURN_M  0x0000FFFF // Mask for TXURN (QCU 0-15)
#define F2_ISR_S2_MCABT        0x00010000 // Master cycle abort interrupt
#define F2_ISR_S2_SSERR        0x00020000 // SERR interrupt
#define F2_ISR_S2_DPERR        0x00040000 // PCI bus parity error
#define F2_ISR_S2_RESV0        0xFFF80000 // Reserved

#define F2_ISR_S3             0x0090 // MAC Secondary interrupt status register 3
#define F2_ISR_S3_QCU_QCBROVF_M  0x0000FFFF // Mask for QCBROVF (QCU 0-15)
#define F2_ISR_S3_QCU_QCBRURN_M  0xFFFF0000 // Mask for QCBRURN (QCU 0-15)

#define F2_ISR_S4             0x0094 // MAC Secondary interrupt status register 4
#define F2_ISR_S4_QCU_QTRIG_M  0x0000FFFF // Mask for QTRIG (QCU 0-15)
#define F2_ISR_S4_RESV0        0xFFFF0000 // Reserved

// Interrupt Mask Registers

#define F2_IMR             0x00a0  // MAC Primary interrupt mask register
#define F2_IMR_RXOK          0x00000001 // At least one frame received sans errors
#define F2_IMR_RXDESC        0x00000002 // Receive interrupt request
#define F2_IMR_RXERR         0x00000004 // Receive error interrupt
#define F2_IMR_RXNOPKT       0x00000008 // No frame received within timeout clock
#define F2_IMR_RXEOL         0x00000010 // Received descriptor empty interrupt
#define F2_IMR_RXORN         0x00000020 // Receive FIFO overrun interrupt
#define F2_IMR_TXOK          0x00000040 // Transmit okay interrupt
#define F2_IMR_TXDESC        0x00000080 // Transmit interrupt request
#define F2_IMR_TXERR         0x00000100 // Transmit error interrupt
#define F2_IMR_TXNOPKT       0x00000200 // No frame transmitted interrupt
#define F2_IMR_TXEOL         0x00000400 // Transmit descriptor empty interrupt
#define F2_IMR_TXURN         0x00000800 // Transmit FIFO underrun interrupt
#define F2_IMR_MIB           0x00001000 // MIB interrupt - see MIBC
#define F2_IMR_SWI           0x00002000 // Software interrupt
#define F2_IMR_RXPHY         0x00004000 // PHY receive error interrupt
#define F2_IMR_RXKCM         0x00008000 // Key-cache miss interrupt
#define F2_IMR_SWBA          0x00010000 // Software beacon alert interrupt
#define F2_IMR_BRSSI         0x00020000 // Beacon threshold interrupt
#define F2_IMR_BMISS         0x00040000 // Beacon missed interrupt
#define F2_IMR_HIUERR        0x00080000 // An unexpected bus error has occurred
#define F2_IMR_BNR           0x00100000 // BNR interrupt
#define F2_IMR_TIM		   0x00800000 // TIM interrupt
#define F2_IMR_GPIO          0x01000000 // GPIO Interrupt
#define F2_IMR_QCBROVF       0x02000000 // QCU CBR overflow interrupt
#define F2_IMR_QCBRURN       0x04000000 // QCU CBR underrun interrupt
#define F2_IMR_QTRIG         0x08000000 // QCU scheduling trigger interrupt
#define F2_IMR_RESV0         0xF0000000 // Reserved


#define F2_IMR_S0             0x00a4 // MAC Secondary interrupt mask register 0
#define F2_IMR_S0_QCU_TXOK_M    0x0000FFFF // Mask for TXOK (QCU 0-15)
#define F2_IMR_S0_QCU_TXDESC_M  0xFFFF0000 // Mask for TXDESC (QCU 0-15)
#define F2_IMR_S0_QCU_TXDESC_S  16		   // Shift for TXDESC (QCU 0-15)

#define F2_IMR_S1             0x00a8 // MAC Secondary interrupt mask register 1
#define F2_IMR_S1_QCU_TXERR_M  0x0000FFFF // Mask for TXERR (QCU 0-15)
#define F2_IMR_S1_QCU_TXEOL_M  0xFFFF0000 // Mask for TXEOL (QCU 0-15)
#define F2_IMR_S1_QCU_TXEOL_S  16		  // Shift for TXEOL (QCU 0-15)
 
#define F2_IMR_S2             0x00ac // MAC Secondary interrupt mask register 2
#define F2_IMR_S2_QCU_TXURN_M  0x0000FFFF // Mask for TXURN (QCU 0-15)
#define F2_IMR_S2_MCABT        0x00010000 // Master cycle abort interrupt
#define F2_IMR_S2_SSERR        0x00020000 // SERR interrupt
#define F2_IMR_S2_DPERR        0x00040000 // PCI bus parity error
#define F2_IMR_S2_RESV0        0xFFF80000 // Reserved

#define F2_IMR_S3             0x00b0 // MAC Secondary interrupt mask register 3
#define F2_IMR_S3_QCU_QCBROVF_M  0x0000FFFF // Mask for QCBROVF (QCU 0-15)
#define F2_IMR_S3_QCU_QCBRURN_M  0xFFFF0000 // Mask for QCBRURN (QCU 0-15)
#define F2_IMR_S3_QCU_QCBRURN_S  16		    // Shift for QCBRURN (QCU 0-15)

#define F2_IMR_S4             0x00b4 // MAC Secondary interrupt mask register 4
#define F2_IMR_S4_QCU_QTRIG_M  0x0000FFFF // Mask for QTRIG (QCU 0-15)
#define F2_IMR_S4_RESV0        0xFFFF0000 // Reserved


// Interrupt status registers (read-and-clear access, secondary shadow copies)

#define F2_ISR_RAC          0x00c0 // MAC Primary interrupt status register,
									// read-and-clear access
#define F2_ISR_S0_S           0x00c4 // MAC Secondary interrupt status register 0,
									// shadow copy
#define F2_ISR_S1_S           0x00c8 // MAC Secondary interrupt status register 1,
									// shadow copy
#define F2_ISR_S2_S           0x00cc // MAC Secondary interrupt status register 2,
									// shadow copy
#define F2_ISR_S3_S           0x00d0 // MAC Secondary interrupt status register 3,
									// shadow copy
#define F2_ISR_S4_S           0x00d4 // MAC Secondary interrupt status register 4,
									// shadow copy

// QCU registers

#define F2_NUM_QCU		  16		// 0-15
#define F2_QCU_0		  0x0001
#define F2_QCU_1		  0x0002
#define F2_QCU_2		  0x0004
#define F2_QCU_3		  0x0008
#define F2_QCU_4		  0x0010
#define F2_QCU_5		  0x0020
#define F2_QCU_6		  0x0040
#define F2_QCU_7		  0x0080
#define F2_QCU_8		  0x0100
#define F2_QCU_9		  0x0200
#define F2_QCU_10		  0x0400
#define F2_QCU_11		  0x0800
#define F2_QCU_12		  0x1000
#define F2_QCU_13		  0x2000
#define F2_QCU_14		  0x4000
#define F2_QCU_15		  0x8000


#define F2_Q0_TXDP           0x0800 // MAC Transmit Queue descriptor pointer
#define F2_Q1_TXDP           0x0804 // MAC Transmit Queue descriptor pointer
#define F2_Q2_TXDP           0x0808 // MAC Transmit Queue descriptor pointer
#define F2_Q3_TXDP           0x080c // MAC Transmit Queue descriptor pointer
#define F2_Q4_TXDP           0x0810 // MAC Transmit Queue descriptor pointer
#define F2_Q5_TXDP           0x0814 // MAC Transmit Queue descriptor pointer
#define F2_Q6_TXDP           0x0818 // MAC Transmit Queue descriptor pointer
#define F2_Q7_TXDP           0x081c // MAC Transmit Queue descriptor pointer
#define F2_Q8_TXDP           0x0820 // MAC Transmit Queue descriptor pointer
#define F2_Q9_TXDP           0x0824 // MAC Transmit Queue descriptor pointer
#define F2_Q10_TXDP          0x0828 // MAC Transmit Queue descriptor pointer
#define F2_Q11_TXDP          0x082c // MAC Transmit Queue descriptor pointer
#define F2_Q12_TXDP          0x0830 // MAC Transmit Queue descriptor pointer
#define F2_Q13_TXDP          0x0834 // MAC Transmit Queue descriptor pointer
#define F2_Q14_TXDP          0x0838 // MAC Transmit Queue descriptor pointer
#define F2_Q15_TXDP          0x083c // MAC Transmit Queue descriptor pointer

#define F2_Q_TXE             0x0840 // MAC Transmit Queue enable
#define F2_Q_TXE_M			  0x0000FFFF // Mask for TXE (QCU 0-15)

#define F2_Q_TXD             0x0880 // MAC Transmit Queue disable
#define F2_Q_TXD_M			  0x0000FFFF // Mask for TXD (QCU 0-15)

#define F2_Q0_CBRCFG         0x08c0 // MAC CBR configuration
#define F2_Q1_CBRCFG         0x08c4 // MAC CBR configuration
#define F2_Q2_CBRCFG         0x08c8 // MAC CBR configuration
#define F2_Q3_CBRCFG         0x08cc // MAC CBR configuration
#define F2_Q4_CBRCFG         0x08d0 // MAC CBR configuration
#define F2_Q5_CBRCFG         0x08d4 // MAC CBR configuration
#define F2_Q6_CBRCFG         0x08d8 // MAC CBR configuration
#define F2_Q7_CBRCFG         0x08dc // MAC CBR configuration
#define F2_Q8_CBRCFG         0x08e0 // MAC CBR configuration
#define F2_Q9_CBRCFG         0x08e4 // MAC CBR configuration
#define F2_Q10_CBRCFG        0x08e8 // MAC CBR configuration
#define F2_Q11_CBRCFG        0x08ec // MAC CBR configuration
#define F2_Q12_CBRCFG        0x08f0 // MAC CBR configuration
#define F2_Q13_CBRCFG        0x08f4 // MAC CBR configuration
#define F2_Q14_CBRCFG        0x08f8 // MAC CBR configuration
#define F2_Q15_CBRCFG        0x08fc // MAC CBR configuration
#define F2_Q_CBRCFG_CBR_INTERVAL_M		0x00FFFFFF // Mask for CBR interval (us)
#define F2_Q_CBRCFG_CBR_OVF_THRESH_M	0xFF000000 // Mask for CBR overflow threshold

#define F2_Q0_RDYTIMECFG         0x0900 // MAC ReadyTime configuration
#define F2_Q1_RDYTIMECFG         0x0904 // MAC ReadyTime configuration
#define F2_Q2_RDYTIMECFG         0x0908 // MAC ReadyTime configuration
#define F2_Q3_RDYTIMECFG         0x090c // MAC ReadyTime configuration
#define F2_Q4_RDYTIMECFG         0x0910 // MAC ReadyTime configuration
#define F2_Q5_RDYTIMECFG         0x0914 // MAC ReadyTime configuration
#define F2_Q6_RDYTIMECFG         0x0918 // MAC ReadyTime configuration
#define F2_Q7_RDYTIMECFG         0x091c // MAC ReadyTime configuration
#define F2_Q8_RDYTIMECFG         0x0920 // MAC ReadyTime configuration
#define F2_Q9_RDYTIMECFG         0x0924 // MAC ReadyTime configuration
#define F2_Q10_RDYTIMECFG        0x0928 // MAC ReadyTime configuration
#define F2_Q11_RDYTIMECFG        0x092c // MAC ReadyTime configuration
#define F2_Q12_RDYTIMECFG        0x0930 // MAC ReadyTime configuration
#define F2_Q13_RDYTIMECFG        0x0934 // MAC ReadyTime configuration
#define F2_Q14_RDYTIMECFG        0x0938 // MAC ReadyTime configuration
#define F2_Q15_RDYTIMECFG        0x093c // MAC ReadyTime configuration
#define F2_Q_RDYTIMECFG_DURATION_M		0x00FFFFFF // Mask for CBR interval (us)
#define F2_Q_RDYTIMECFG_EN				0x01000000 // ReadyTime enable
#define F2_Q_RDYTIMECFG_RESV0			0xFE000000 // Reserved

#define F2_Q_ONESHOTARM_SC       0x0940 // MAC OneShotArm set control
#define F2_Q_ONESHOTARM_SC_M	  0x0000FFFF // Mask for F2_Q_ONESHOTARM_SC (QCU 0-15)
#define F2_Q_ONESHOTARM_SC_RESV0  0xFFFF0000 // Reserved

#define F2_Q_ONESHOTARM_CC       0x0980 // MAC OneShotArm clear control
#define F2_Q_ONESHOTARM_CC_M	  0x0000FFFF // Mask for F2_Q_ONESHOTARM_CC (QCU 0-15)
#define F2_Q_ONESHOTARM_CC_RESV0  0xFFFF0000 // Reserved

#define F2_Q0_MISC         0x09c0 // MAC Miscellaneous QCU settings
#define F2_Q1_MISC         0x09c4 // MAC Miscellaneous QCU settings
#define F2_Q2_MISC         0x09c8 // MAC Miscellaneous QCU settings
#define F2_Q3_MISC         0x09cc // MAC Miscellaneous QCU settings
#define F2_Q4_MISC         0x09d0 // MAC Miscellaneous QCU settings
#define F2_Q5_MISC         0x09d4 // MAC Miscellaneous QCU settings
#define F2_Q6_MISC         0x09d8 // MAC Miscellaneous QCU settings
#define F2_Q7_MISC         0x09dc // MAC Miscellaneous QCU settings
#define F2_Q8_MISC         0x09e0 // MAC Miscellaneous QCU settings
#define F2_Q9_MISC         0x09e4 // MAC Miscellaneous QCU settings
#define F2_Q10_MISC        0x09e8 // MAC Miscellaneous QCU settings
#define F2_Q11_MISC        0x09ec // MAC Miscellaneous QCU settings
#define F2_Q12_MISC        0x09f0 // MAC Miscellaneous QCU settings
#define F2_Q13_MISC        0x09f4 // MAC Miscellaneous QCU settings
#define F2_Q14_MISC        0x09f8 // MAC Miscellaneous QCU settings
#define F2_Q15_MISC        0x09fc // MAC Miscellaneous QCU settings
#define F2_Q_MISC_FSP_M					0x0000000F // Mask for Frame Scheduling Policy
#define F2_Q_MISC_FSP_ASAP				0			// ASAP
#define F2_Q_MISC_FSP_CBR				1			// CBR
#define F2_Q_MISC_FSP_DBA_GATED			2			// DMA Beacon Alert gated
#define F2_Q_MISC_FSP_TIM_GATED			3			// TIM gated
#define F2_Q_MISC_FSP_BEACON_SENT_GATED	4			// Beacon-sent-gated
#define F2_Q_MISC_ONE_SHOT_EN			0x00000010 // OneShot enable
#define F2_Q_MISC_CBR_INCR_DIS1			0x00000020 // Disable CBR expired counter incr (empty q)
#define F2_Q_MISC_CBR_INCR_DIS0			0x00000040 // Disable CBR expired counter incr (empty beacon q)
#define F2_Q_MISC_BEACON_USE			0x00000080 // Beacon use indication
#define F2_Q_MISC_CBR_EXP_CNTR_LIMIT	0x00000100 // CBR expired counter limit enable
#define F2_Q_MISC_RDYTIME_EXP_POLICY    0x00000200 // Enable TXE cleared on ReadyTime expired or VEOL 
#define F2_Q_MISC_RESET_CBR_EXP_CTR     0x00000400 // Reset CBR expired counter
#define F2_Q_MISC_RESV0					0xFFFFF000 // Reserved


#define F2_Q0_STS         0x0a00 // MAC Miscellaneous QCU status
#define F2_Q1_STS         0x0a04 // MAC Miscellaneous QCU status
#define F2_Q2_STS         0x0a08 // MAC Miscellaneous QCU status
#define F2_Q3_STS         0x0a0c // MAC Miscellaneous QCU status
#define F2_Q4_STS         0x0a10 // MAC Miscellaneous QCU status
#define F2_Q5_STS         0x0a14 // MAC Miscellaneous QCU status
#define F2_Q6_STS         0x0a18 // MAC Miscellaneous QCU status
#define F2_Q7_STS         0x0a1c // MAC Miscellaneous QCU status
#define F2_Q8_STS         0x0a20 // MAC Miscellaneous QCU status
#define F2_Q9_STS         0x0a24 // MAC Miscellaneous QCU status
#define F2_Q10_STS        0x0a28 // MAC Miscellaneous QCU status
#define F2_Q11_STS        0x0a2c // MAC Miscellaneous QCU status
#define F2_Q12_STS        0x0a30 // MAC Miscellaneous QCU status
#define F2_Q13_STS        0x0a34 // MAC Miscellaneous QCU status
#define F2_Q14_STS        0x0a38 // MAC Miscellaneous QCU status
#define F2_Q15_STS        0x0a3c // MAC Miscellaneous QCU status
#define F2_Q_STS_PEND_FR_CNT_M		0x00000003 // Mask for Pending Frame Count
#define F2_Q_STS_RESV0				0x000000FC // Reserved
#define F2_Q_STS_CBR_EXP_CNT_M		0x0000FF00 // Mask for CBR expired counter
#define F2_Q_STS_RESV1				0xFFFF0000 // Reserved


#define F2_Q_RDYTIMESHDN   0x0a40 // MAC ReadyTimeShutdown status
#define F2_Q_RDYTIMESHDN_M  0x0000FFFF // Mask for ReadyTimeShutdown status (QCU 0-15)


// DCU registers

#define F2_NUM_DCU        11		// 0-10
#define F2_DCU_0		  0x0001
#define F2_DCU_1		  0x0002
#define F2_DCU_2		  0x0004
#define F2_DCU_3		  0x0008
#define F2_DCU_4		  0x0010
#define F2_DCU_5		  0x0020
#define F2_DCU_6		  0x0040
#define F2_DCU_7		  0x0080
#define F2_DCU_8		  0x0100
#define F2_DCU_9		  0x0200
#define F2_DCU_10		  0x0400
#define F2_DCU_11		  0x0800

#define F2_D0_QCUMASK     0x1000 // MAC QCU Mask
#define F2_D1_QCUMASK     0x1004 // MAC QCU Mask
#define F2_D2_QCUMASK     0x1008 // MAC QCU Mask
#define F2_D3_QCUMASK     0x100c // MAC QCU Mask
#define F2_D4_QCUMASK     0x1010 // MAC QCU Mask
#define F2_D5_QCUMASK     0x1014 // MAC QCU Mask
#define F2_D6_QCUMASK     0x1018 // MAC QCU Mask
#define F2_D7_QCUMASK     0x101c // MAC QCU Mask
#define F2_D8_QCUMASK     0x1020 // MAC QCU Mask
#define F2_D9_QCUMASK     0x1024 // MAC QCU Mask
#define F2_D10_QCUMASK    0x1028 // MAC QCU Mask
#define F2_D_QCUMASK_M	   0x0000FFFF // Mask for QCU Mask (QCU 0-15)
#define F2_D_QCUMASK_RESV0 0xFFFF0000 // Reserved


#define F2_D0_LCL_IFS     0x1040 // MAC DCU-specific IFS settings
#define F2_D1_LCL_IFS     0x1044 // MAC DCU-specific IFS settings
#define F2_D2_LCL_IFS     0x1048 // MAC DCU-specific IFS settings
#define F2_D3_LCL_IFS     0x104c // MAC DCU-specific IFS settings
#define F2_D4_LCL_IFS     0x1050 // MAC DCU-specific IFS settings
#define F2_D5_LCL_IFS     0x1054 // MAC DCU-specific IFS settings
#define F2_D6_LCL_IFS     0x1058 // MAC DCU-specific IFS settings
#define F2_D7_LCL_IFS     0x105c // MAC DCU-specific IFS settings
#define F2_D8_LCL_IFS     0x1060 // MAC DCU-specific IFS settings
#define F2_D9_LCL_IFS     0x1064 // MAC DCU-specific IFS settings
#define F2_D10_LCL_IFS    0x1068 // MAC DCU-specific IFS settings
#define F2_D_LCL_IFS_CWMIN_M	   0x000003FF // Mask for CW_MIN
#define F2_D_LCL_IFS_CWMAX_M	   0x000FFC00 // Mask for CW_MAX
#define F2_D_LCL_IFS_CWMAX_S	   10		  // Shift for CW_MAX
#define F2_D_LCL_IFS_AIFS_M		   0x0FF00000 // Mask for AIFS
#define F2_D_LCL_IFS_AIFS_S		   20         // Shift for AIFS
#define F2_D_LCL_IFS_RESV0		   0xF0000000 // Reserved

#define F2_D0_RETRY_LIMIT	  0x1080 // MAC Retry limits
#define F2_D1_RETRY_LIMIT     0x1084 // MAC Retry limits
#define F2_D2_RETRY_LIMIT     0x1088 // MAC Retry limits
#define F2_D3_RETRY_LIMIT     0x108c // MAC Retry limits
#define F2_D4_RETRY_LIMIT     0x1090 // MAC Retry limits
#define F2_D5_RETRY_LIMIT     0x1094 // MAC Retry limits
#define F2_D6_RETRY_LIMIT     0x1098 // MAC Retry limits
#define F2_D7_RETRY_LIMIT     0x109c // MAC Retry limits
#define F2_D8_RETRY_LIMIT     0x10a0 // MAC Retry limits
#define F2_D9_RETRY_LIMIT     0x10a4 // MAC Retry limits
#define F2_D10_RETRY_LIMIT    0x10a8 // MAC Retry limits
#define F2_D_RETRY_LIMIT_FR_SH_M	   0x0000000F // Mask for frame short retry limit
#define F2_D_RETRY_LIMIT_FR_LG_M	   0x000000F0 // Mask for frame long retry limit
#define F2_D_RETRY_LIMIT_STA_SH_M	   0x00003F00 // Mask for station short retry limit
#define F2_D_RETRY_LIMIT_STA_LG_M	   0x000FC000 // Mask for station short retry limit
#define F2_D_RETRY_LIMIT_RESV0		   0xFFF00000 // Reserved

#define F2_D0_CHNTIME	  0x10c0 // MAC ChannelTime settings
#define F2_D1_CHNTIME     0x10c4 // MAC ChannelTime settings
#define F2_D2_CHNTIME     0x10c8 // MAC ChannelTime settings
#define F2_D3_CHNTIME     0x10cc // MAC ChannelTime settings
#define F2_D4_CHNTIME     0x10d0 // MAC ChannelTime settings
#define F2_D5_CHNTIME     0x10d4 // MAC ChannelTime settings
#define F2_D6_CHNTIME     0x10d8 // MAC ChannelTime settings
#define F2_D7_CHNTIME     0x10dc // MAC ChannelTime settings
#define F2_D8_CHNTIME     0x10e0 // MAC ChannelTime settings
#define F2_D9_CHNTIME     0x10e4 // MAC ChannelTime settings
#define F2_D10_CHNTIME    0x10e8 // MAC ChannelTime settings
#define F2_D_CHNTIME_DUR_M	   0x000FFFFF // Mask for ChannelTime duration (us)
#define F2_D_CHNTIME_EN		   0x00100000 // ChannelTime enable
#define F2_D_CHNTIME_RESV0	   0xFFE00000 // Reserved

#define F2_D0_MISC		  0x1100 // MAC Miscellaneous DCU-specific settings
#define F2_D1_MISC		  0x1104 // MAC Miscellaneous DCU-specific settings
#define F2_D2_MISC        0x1108 // MAC Miscellaneous DCU-specific settings
#define F2_D3_MISC        0x110c // MAC Miscellaneous DCU-specific settings
#define F2_D4_MISC        0x1110 // MAC Miscellaneous DCU-specific settings
#define F2_D5_MISC        0x1114 // MAC Miscellaneous DCU-specific settings
#define F2_D6_MISC        0x1118 // MAC Miscellaneous DCU-specific settings
#define F2_D7_MISC        0x111c // MAC Miscellaneous DCU-specific settings
#define F2_D8_MISC        0x1120 // MAC Miscellaneous DCU-specific settings
#define F2_D9_MISC        0x1124 // MAC Miscellaneous DCU-specific settings
#define F2_D10_MISC       0x1128 // MAC Miscellaneous DCU-specific settings
#define F2_D_MISC_BKOFF_THRESH_M		0x000007FF // Mask for Backoff threshold setting
#define F2_D_MISC_HCF_POLL_EN			0x00000800 // HFC poll enable
#define F2_D_MISC_BKOFF_PERSISTENCE		0x00001000 // Backoff persistence factor setting
#define F2_D_MISC_FR_PREFETCH_EN		0x00002000 // Frame prefetch enable
#define F2_D_MISC_VIR_COL_HANDLING_M	0x0000C000 // Mask for Virtual collision handling policy
#define F2_D_MISC_VIR_COL_HANDLING_NORMAL	0	   // Normal
#define F2_D_MISC_VIR_COL_HANDLING_MODIFIED	1	   // Modified
#define F2_D_MISC_VIR_COL_HANDLING_IGNORE	2	   // Ignore
#define F2_D_MISC_BEACON_USE			0x00010000 // Beacon use indication
#define F2_D_MISC_ARB_LOCKOUT_CNTRL_M	0x00060000 // Mask for DCU arbiter lockout control
#define F2_D_MISC_ARB_LOCKOUT_CNTRL_S	17		   // Shift for DCU arbiter lockout control
#define F2_D_MISC_ARB_LOCKOUT_CNTRL_NONE		0	   // No lockout
#define F2_D_MISC_ARB_LOCKOUT_CNTRL_INTRA_FR	1	   // Intra-frame
#define F2_D_MISC_ARB_LOCKOUT_CNTRL_GLOBAL		2	   // Global
#define F2_D_MISC_ARB_LOCKOUT_IGNORE	0x00080000 // DCU arbiter lockout ignore control
#define F2_D_MISC_SEQ_NUM_INCR_DIS		0x00100000 // Sequence number increment disable
#define F2_D_MISC_POST_FR_BKOFF_DIS		0x00200000 // Post-frame backoff disable
#define F2_D_MISC_VIRT_COLL_POLICY		0x00400000 // Virtual coll. handling policy
#define F2_D_MISC_BLOWN_IFS_POLICY      0x00800000 // Blown IFS handling policy
#define F2_D_MISC_SEQ_NUM_CONTROL		0x01000000 // Sequence Number local or global
#define F2_D_MISC_RESV0					0xFE000000 // Reserved


#define F2_D0_SEQNUM		0x1140 // MAC Frame sequence number control/status
#define F2_D1_SEQNUM		0x1144 // MAC Frame sequence number control/status
#define F2_D2_SEQNUM        0x1148 // MAC Frame sequence number control/status
#define F2_D3_SEQNUM        0x114c // MAC Frame sequence number control/status
#define F2_D4_SEQNUM        0x1150 // MAC Frame sequence number control/status
#define F2_D5_SEQNUM        0x1154 // MAC Frame sequence number control/status
#define F2_D6_SEQNUM        0x1158 // MAC Frame sequence number control/status
#define F2_D7_SEQNUM        0x115c // MAC Frame sequence number control/status
#define F2_D8_SEQNUM        0x1160 // MAC Frame sequence number control/status
#define F2_D9_SEQNUM        0x1164 // MAC Frame sequence number control/status
#define F2_D10_SEQNUM       0x1168 // MAC Frame sequence number control/status
#define F2_D_SEQNUM_M 0x00000FFF // Mask for value of sequence number
#define F2_D_SEQNUM_RESV0	 0xFFFFF000 // Reserved


#define F2_D_GBL_IFS_SIFS	0x1030 // MAC DCU-global IFS settings: SIFS duration
#define F2_D_GBL_IFS_SIFS_M		0x0000FFFF // Mask for SIFS duration (core clocks)
#define F2_D_GBL_IFS_SIFS_RESV0	0xFFFFFFFF // Reserved

#define F2_D_GBL_IFS_SLOT	0x1070 // MAC DCU-global IFS settings: slot duration
#define F2_D_GBL_IFS_SLOT_M		0x0000FFFF // Mask for Slot duration (core clocks)
#define F2_D_GBL_IFS_SLOT_RESV0	0xFFFF0000 // Reserved

#define F2_D_GBL_IFS_EIFS	0x10b0 // MAC DCU-global IFS settings: EIFS duration
#define F2_D_GBL_IFS_EIFS_M		0x0000FFFF // Mask for Slot duration (core clocks)
#define F2_D_GBL_IFS_EIFS_RESV0	0xFFFF0000 // Reserved

#define F2_D_GBL_IFS_MISC	0x10f0 // MAC DCU-global IFS settings: Miscellaneous
#define F2_D_GBL_IFS_MISC_LFSR_SLICE_SEL_M	   0x00000007 // Mask forLFSR slice select
#define F2_D_GBL_IFS_MISC_TURBO_MODE		   0x00000008 // Turbo mode indication
#define F2_D_GBL_IFS_MISC_SIFS_DURATION_USEC_M 0x000003F0 // Mask for SIFS duration (us)
#define F2_D_GBL_IFS_MISC_USEC_DURATION_M      0x000FFC00 // Mask for microsecond duration
#define F2_D_GBL_IFS_MISC_DCU_ARBITER_DLY_M    0x00300000 // Mask for DCU arbiter delay
#define F2_D_GBL_IFS_MISC_RESV0				   0xFFC00000 // Reserved

#define F2_D_FPCTL			0x1230		//Frame prefetch

#define F2_D_TXBLK_BASE           0x00001038
#define CALC_MMR(dcu, idx)        ( (4 * dcu) + \
                                    (idx < 32 ? 0 : (idx < 64 ? 1 : (idx < 96 ? 2 : 3))) )
#define TXBLK_FROM_MMR(mmr)       (F2_D_TXBLK_BASE + ((mmr & 0x1f) << 6) + ((mmr & 0x20) >> 3))
#define CALC_TXBLK_ADDR(dcu, idx) (TXBLK_FROM_MMR(CALC_MMR(dcu, idx)))
#define CALC_TXBLK_VALUE(idx)     (1 << (idx & 0x1f))

// DMA & PCI Registers in PCI space (usable during sleep)
#define F2_RC               0x4000  // Warm reset control register
#define F2_RC_MAC            0x00000001 // MAC reset 
#define F2_RC_BB             0x00000002 // Baseband reset
#define F2_RC_RESV0          0x00000004 // Reserved
#define F2_RC_RESV1          0x00000008 // Reserved
#define F2_RC_PCI            0x00000010 // PCI-core reset

#define F2_SCR              0x4004  // Sleep control register
#define F2_SCR_SLDUR_MASK    0x0000ffff // sleep duration mask, units of 128us
#define F2_SCR_SLE_MASK      0x00030000 // sleep enable mask
#define F2_SCR_SLE_FWAKE     0x00000000 // force wake
#define F2_SCR_SLE_FSLEEP    0x00010000 // force sleep
#define F2_SCR_SLE_NORMAL    0x00020000 // sleep logic normal operation
#define F2_SCR_SLE_UNITS     0x00000008 // SCR units/TU

#define F2_INTPEND          0x4008  // Interrupt Pending register
#define F2_INTPEND_TRUE      0x00000001 // interrupt pending

#define F2_SFR              0x400C  // Sleep force register
#define F2_SFR_SLEEP         0x00000001 // force sleep
#define F2_SFR_WAKE         0x00000002 // force wake

#define F2_PCICFG           0x4010  // PCI configuration register
#define F2_PCICFG_SLEEP_CLK_SEL		0x00000002 // select between 40MHz normal or 32KHz sleep clock
#define F2_PCICFG_CLKRUNEN		0x00000004 // enable PCI CLKRUN function
#define F2_PCICFG_EEPROM_SIZE_M 0x00000018 // Mask for EEPROM size
#define F2_PCICFG_EEPROM_SIZE_4K	0	   // EEPROM size 4 Kbit
#define F2_PCICFG_EEPROM_SIZE_8K	1	   // EEPROM size 4 Kbit
#define F2_PCICFG_EEPROM_SIZE_16K	2	   // EEPROM size 4 Kbit
#define F2_PCICFG_EEPROM_SIZE_FAILED 3	   // Failure
#define F2_PCICFG_ASSOC_STATUS_M	0x00000060 // Mask for Association Status
#define F2_PCICFG_ASSOC_STATUS_NONE			0		
#define F2_PCICFG_ASSOC_STATUS_PENDING		1		
#define F2_PCICFG_ASSOC_STATUS_ASSOCIATED	2		
#define F2_PCICFG_PCI_BUS_SEL_M		0x00000380 // Mask for PCI observation bus mux select
#define F2_PCICFG_DIS_CBE_FIX		0x00000400 // Disable fix for bad PCI CBE# generation
#define F2_PCICFG_SL_INTEN			0x00000800 // enable interrupt line assertion when asleep
#define F2_PCICFG_RESV0				0x00001000 // Reserved
#define F2_PCICFG_SL_INPEN			0x00002000 // Force asleep when an interrupt is pending
#define F2_PCICFG_RESV1				0x0000C000 // Reserved
#define F2_PCICFG_SPWR_DN			0x00010000 // mask for sleep/awake indication
#define F2_PCICFG_LED_MODE_M		0x000E0000 // Mask for LED mode select
#define F2_PCICFG_LED_BLINK_THRESHOLD_M		0x00700000 // Mask for LED blink threshold select
#define F2_PCICFG_LED_SLOW_BLINK_MODE		0x00800000 // LED slowest blink rate mode
#define F2_PCICFG_SLEEP_CLK_RATE_INDICATION	0x03000000 // LED slowest blink rate mode
#define F2_PCICFG_RESV2				0xFF000000 // Reserved




#define F2_NUM_GPIO         6		// Six numbered 0 to 5.

#define F2_GPIOCR           0x4014  // GPIO control register
#define F2_GPIOCR_CR_SHIFT   2          // Each CR is 2 bits
#define F2_GPIOCR_0_CR_N     0x00000000 // Input only mode for GPIODO[0]
#define F2_GPIOCR_0_CR_0     0x00000001 // Output only if GPIODO[0] = 0
#define F2_GPIOCR_0_CR_1     0x00000002 // Output only if GPIODO[0] = 1
#define F2_GPIOCR_0_CR_A     0x00000003 // Always output
#define F2_GPIOCR_1_CR_N     0x00000000 // Input only mode for GPIODO[1]
#define F2_GPIOCR_1_CR_0     0x00000004 // Output only if GPIODO[1] = 0
#define F2_GPIOCR_1_CR_1     0x00000008 // Output only if GPIODO[1] = 1
#define F2_GPIOCR_1_CR_A     0x0000000C // Always output
#define F2_GPIOCR_2_CR_N     0x00000000 // Input only mode for GPIODO[2]
#define F2_GPIOCR_2_CR_0     0x00000010 // Output only if GPIODO[2] = 0
#define F2_GPIOCR_2_CR_1     0x00000020 // Output only if GPIODO[2] = 1
#define F2_GPIOCR_2_CR_A     0x00000030 // Always output
#define F2_GPIOCR_3_CR_N     0x00000000 // Input only mode for GPIODO[3]
#define F2_GPIOCR_3_CR_0     0x00000040 // Output only if GPIODO[3] = 0
#define F2_GPIOCR_3_CR_1     0x00000080 // Output only if GPIODO[3] = 1
#define F2_GPIOCR_3_CR_A     0x000000C0 // Always output
#define F2_GPIOCR_4_CR_N     0x00000000 // Input only mode for GPIODO[4]
#define F2_GPIOCR_4_CR_0     0x00000100 // Output only if GPIODO[4] = 0
#define F2_GPIOCR_4_CR_1     0x00000200 // Output only if GPIODO[4] = 1
#define F2_GPIOCR_4_CR_A     0x00000300 // Always output
#define F2_GPIOCR_5_CR_N     0x00000000 // Input only mode for GPIODO[5]
#define F2_GPIOCR_5_CR_0     0x00000400 // Output only if GPIODO[5] = 0
#define F2_GPIOCR_5_CR_1     0x00000800 // Output only if GPIODO[5] = 1
#define F2_GPIOCR_5_CR_A     0x00000C00 // Always output
#define F2_GPIOCR_INT_SEL0   0x00000000 // Select Interrupt Pin GPIO_0
#define F2_GPIOCR_INT_SEL1   0x00001000 // Select Interrupt Pin GPIO_1 
#define F2_GPIOCR_INT_SEL2   0x00002000 // Select Interrupt Pin GPIO_2 
#define F2_GPIOCR_INT_SEL3   0x00003000 // Select Interrupt Pin GPIO_3 
#define F2_GPIOCR_INT_SEL4   0x00004000 // Select Interrupt Pin GPIO_4 
#define F2_GPIOCR_INT_SEL5   0x00005000 // Select Interrupt Pin GPIO_5
#define F2_GPIOCR_INT_EN     0x00008000 // Enable GPIO Interrupt
#define F2_GPIOCR_INT_SELL   0x00000000 // Generate Interrupt if selected pin is low
#define F2_GPIOCR_INT_SELH   0x00010000 // Generate Interrupt if selected pin is high

#define F2_GPIODO           0x4018  // GPIO data output access register
#define F2_GPIODI           0x401C  // GPIO data input access register
#define F2_GPIOD_MASK        0x0000002F // Mask for reading or writing GPIO data regs

#define F2_SREV             0x4020  // Silicon Revision register
#define F2_SREV_ID_M         0x000000FF // Mask to read SREV info
#define F2_SREV_REVISION_M	 0x0000000F	// Mask for Chip revision level
#define F2_SREV_FPGA         1
#define F2_SREV_D2PLUS       2
#define F2_SREV_D2PLUS_MS    3		// metal spin
#define F2_SREV_CRETE        4
#define F2_SREV_CRETE_MS     5		// FCS metal spin
#define F2_SREV_CRETE_MS23   7          // 2.3 metal spin (6 skipped)
#define F2_SREV_CRETE_23     8          // 2.3 full tape out
#define F2_SREV_VERSION_M	 0x000000F0	// Mask for Chip version indication
#define F2_SREV_VERSION_CRETE	0
#define F2_SREV_VERSION_MAUI_1	1
#define F2_SREV_VERSION_MAUI_2	2


// EEPROM Registers in the MAC
#define F2_EEPROM_ADDR	    0x6000  // EEPROM address register (10 bit)
#define F2_EEPROM_DATA	    0x6004  // EEPROM data register (16 bit)

#define F2_EEPROM_CMD	    0x6008  // EEPROM command register
#define F2_EEPROM_CMD_READ	 0x00000001
#define F2_EEPROM_CMD_WRITE	 0x00000002
#define F2_EEPROM_CMD_RESET	 0x00000004

#define F2_EEPROM_STS	    0x600c  // EEPROM status register
#define F2_EEPROM_STS_READ_ERROR	 0x00000001
#define F2_EEPROM_STS_READ_COMPLETE	 0x00000002
#define F2_EEPROM_STS_WRITE_ERROR	 0x00000004
#define F2_EEPROM_STS_WRITE_COMPLETE 0x00000008

#define F2_EEPROM_CFG	    0x6010  // EEPROM configuration register
#define F2_EEPROM_CFG_SIZE_M	 0x00000003 // Mask for EEPROM size determination override
#define F2_EEPROM_CFG_SIZE_AUTO		0
#define F2_EEPROM_CFG_SIZE_4KBIT	1
#define F2_EEPROM_CFG_SIZE_8KBIT	2
#define F2_EEPROM_CFG_SIZE_16KBIT	3
#define F2_EEPROM_CFG_DIS_WAIT_WRITE_COMPL 0x00000004 // Disable wait for write completion
#define F2_EEPROM_CFG_CLOCK_M	 0x00000018 // Mask for EEPROM clock rate control
#define F2_EEPROM_CFG_CLOCK_S	 3			// Shift for EEPROM clock rate control
#define F2_EEPROM_CFG_CLOCK_156KHZ	0
#define F2_EEPROM_CFG_CLOCK_312KHZ	1
#define F2_EEPROM_CFG_CLOCK_625KHZ	2
#define F2_EEPROM_CFG_RESV0		 0x000000E0 // Reserved
#define F2_EEPROM_CFG_PROT_KEY_M 0x00FFFF00 // Mask for EEPROM protection key
#define F2_EEPROM_CFG_PROT_KEY_S 8			// Shift for EEPROM protection key
#define F2_EEPROM_CFG_EN_L		 0x01000000 // EPRM_EN_L setting

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

#define F2_BSS_ID0          0x8008  // MAC BSSID low 32 bits
#define F2_BSS_ID1          0x800C  // MAC BSSID upper 16 bits / AID
#define F2_BSS_ID1_U16_M     0x0000FFFF // Mask for upper 16 bits of BSSID
#define F2_BSS_ID1_AID_M     0xFFFF0000 // Mask for association ID 
#define F2_BSS_ID1_AID_S     16         // Shift for association ID 

#define F2_SLOT_TIME        0x8010  // MAC Time-out after a collision
#define F2_SLOT_TIME_MASK    0x000007FF // Slot time mask

#define F2_TIME_OUT         0x8014  // MAC ACK & CTS time-out
#define F2_TIME_OUT_ACK_M    0x00001FFF // Mask for ACK time-out
#define F2_TIME_OUT_CTS_M    0x1FFF0000 // Mask for CTS time-out
#define F2_TIME_OUT_CTS_S    16         // Shift for CTS time-out

#define F2_RSSI_THR         0x8018  // MAC Beacon RSSI warning and missed beacon threshold
#define F2_RSSI_THR_MASK     0x000000FF // Mask for Beacon RSSI warning threshold
#define F2_RSSI_THR_BM_THR_M 0x0000FF00 // Mask for Missed beacon threshold
#define F2_RSSI_THR_BM_THR_S 8          // Shift for Missed beacon threshold

#define F2_USEC             0x801c  // MAC transmit latency register
#define F2_USEC_M            0x0000007F // Mask for clock cycles in 1 usec
#define F2_USEC_32_M         0x00003F80 // Mask for number of 32MHz clock cycles in 1 usec
#define F2_USEC_32_S         7          // Shift for number of 32MHz clock cycles in 1 usec
#define F2_USEC_TX_LAT_M     0x000FC000 // Mask for tx latency to start of SIGNAL (usec)
#define F2_USEC_TX_LAT_S     14         // Shift for tx latency to start of SIGNAL (usec)
#define F2_USEC_RX_LAT_M     0x03F00000 // Mask for rx latency to start of SIGNAL (usec)
#define F2_USEC_RX_LAT_S     20         // Shift for rx latency to start of SIGNAL (usec)

#define F2_BEACON           0x8020  // MAC beacon control value/mode bits
#define F2_BEACON_PERIOD_MASK 0x0000FFFF  // Beacon period mask in TU/msec
#define F2_BEACON_TIM_MASK    0x007F0000  // Mask for byte offset of TIM start
#define F2_BEACON_TIM_S       16          // Shift for byte offset of TIM start
#define F2_BEACON_EN          0x00800000  // beacon enable
#define F2_BEACON_RESET_TSF   0x01000000  // Clears TSF to 0

#define F2_CFP_PERIOD       0x8024  // MAC CFP Interval (TU/msec)
#define F2_TIMER0           0x8028  // MAC Next beacon time (TU/msec)
#define F2_TIMER1           0x802c  // MAC DMA beacon alert time (1/8 TU)
#define F2_TIMER2           0x8030  // MAC Software beacon alert (1/8 TU)
#define F2_TIMER3           0x8034  // MAC ATIM window time

#define F2_CFP_DUR          0x8038  // MAC maximum CFP duration in TU

#define F2_RX_FILTER        0x803C  // MAC receive filter register
#define F2_RX_FILTER_ALL     0x00000000 // Disallow all frames
#define F2_RX_UCAST          0x00000001 // Allow unicast frames
#define F2_RX_MCAST          0x00000002 // Allow multicast frames
#define F2_RX_BCAST          0x00000004 // Allow broadcast frames
#define F2_RX_CONTROL        0x00000008 // Allow control frames
#define F2_RX_BEACON         0x00000010 // Allow beacon frames
#define F2_RX_PROM           0x00000020 // Promiscuous mode, all packets

#define F2_MCAST_FIL0       0x8040  // MAC multicast filter lower 32 bits
#define F2_MCAST_FIL1       0x8044  // MAC multicast filter upper 32 bits

#define F2_DIAG_SW          0x8048  // MAC PCU control register
#define F2_DIAG_CACHE_ACK    0x00000001 // disable ACK when no valid key found
#define F2_DIAG_ACK_DIS      0x00000002 // disable ACK generation
#define F2_DIAG_CTS_DIS      0x00000004 // disable CTS generation
#define F2_DIAG_ENCRYPT_DIS  0x00000008 // disable encryption
#define F2_DIAG_DECRYPT_DIS  0x00000010 // disable decryption
#define F2_DIAG_RX_DIS       0x00000020 // disable receive
#define F2_DIAG_LOOP_EN      0x00000040 // enable tx-rx loopback
#define F2_DIAG_CORR_FCS     0x00000080 // corrupt FCS
#define F2_DIAG_CHAN_INFO    0x00000100 // dump channel info
#define F2_DIAG_EN_SCRAMSD   0x00000200 // enable fixed scrambler seed
#define F2_DIAG_SCRAM_SEED_M 0x0001FC00 // Mask for fixed scrambler seed
#define F2_DIAG_SCRAM_SEED_S 10         // Shift for fixed scrambler seed
#define F2_DIAG_FRAME_NV0    0x00020000 // accept frames of non-zero protocol version
#define F2_DIAG_OBS_PT_SEL_M 0x000C0000 // Mask for observation point select
#define F2_DIAG_OBS_PT_SEL_S 18         // Shift for observation point select

#define F2_TSF_L32          0x804c  // MAC local clock lower 32 bits
#define F2_TSF_U32          0x8050  // MAC local clock upper 32 bits

#define F2_DEF_ANT			0x8058 //default antenna register
 
#define F2_LAST_TSTP        0x8080  // MAC Time stamp of the last beacon received
#define F2_NAV              0x8084  // MAC current NAV value
#define F2_RTS_OK           0x8088  // MAC RTS exchange success counter
#define F2_RTS_FAIL         0x808c  // MAC RTS exchange failure counter
#define F2_ACK_FAIL         0x8090  // MAC ACK failure counter
#define F2_FCS_FAIL         0x8094  // FCS check failure counter
#define F2_BEACON_CNT       0x8098  // Valid beacon counter


//
// Key table is 128 entries of 8 double-words each
//
#ifndef SWIG // SWIG cannot parse structures
#define D2_KEY_TABLE		0x8800
#define D2_KEY_NUM_ENTRIES	128
typedef struct D2KeyTableEntry
{
	A_UINT32	KeyVal0;
	A_UINT32	KeyVal1;
	A_UINT32	KeyVal2;
	A_UINT32	KeyVal3;
	A_UINT32	KeyVal4;
	A_UINT32	KeyType;
	A_UINT32	MacAddrLo;
	A_UINT32	MacAddrHi;
} D2_KEY_TABLE_ENTRY;

#define D2_KEY_TYPE_M			0x00000007
#define D2_KEY_TYPE_WEP_40		0
#define D2_KEY_TYPE_WEP_104		1
#define D2_KEY_TYPE_WEP_128		3
#define D2_KEY_TYPE_WEP2		4	
#define D2_KEY_TYPE_AES			5
#define D2_KEY_TYPE_LAST_TX_ANT 0x00000008	//last tx antenna

#ifdef ARCH_BIG_ENDIAN
typedef struct 
{
	A_UINT32	Reserved4:16,
				KeyValid:1,
				MacAddrHi:15;
} MAC_ADDR_HI_KEY_VALID;
#else
typedef struct 
{
	A_UINT32	MacAddrHi:15,
				KeyValid:1,
				Reserved4:16;
} MAC_ADDR_HI_KEY_VALID;
#endif /* ARCH_BIG_ENDIAN */
#endif // #ifndef SWIG

// PHY registers
#define PHY_BASE            0x9800  // PHY registers base address

#define PHY_FRAME_CONTROL   0x9804  // PHY frame control register (this is really called turbo now
#define PHY_FC_TURBO_MODE    0x00000001 // Set turbo mode bits
#define PHY_FC_TURBO_SHORT   0x00000002 // Set short symbols to turbo mode setting

#define PHY_FRAME_CONTROL1   0x9944  // rest of old PHY frame control register
#define PHY_FC_TIMING_ERR    0x01000000 // Detect PHY timing error
#define PHY_FC_PARITY_ERR    0x02000000 // Detect PHY signal parity error
#define PHY_FC_ILLRATE_ERR   0x04000000 // Detect PHY illegal rate error
#define PHY_FC_ILLLEN_ERR    0x08000000 // Detect PHY illegal length error
#define PHY_FC_SERVICE_ERR   0x20000000 // Detect PHY non-zero service bytes error
#define PHY_FC_TX_UNDER_ERR  0x40000000 // Detect PHY transmit underrun error

#define PHY_CHIP_ID         0x9818  // PHY chip revision ID

#define PHY_ACTIVE          0x981C  // PHY activation register
#define PHY_ACTIVE_EN        0x00000001 // Activate PHY chips
#define PHY_ACTIVE_DIS       0x00000000 // Deactivate PHY chips

#define PHY_AGC_CONTROL     0x9860  // PHY chip calibration and noise floor setting
#define PHY_AGC_CONTROL_CAL  0x00000001 // Perform PHY chip internal calibration
#define PHY_AGC_CONTROL_NF   0x00000002 // Perform PHY chip noise-floor calculation

#define PHY_RX_DELAY		0x9914	// PHY analog_power_on_time, in 100ns increments
#define PHY_RX_DELAY_M		0x00003FFF // Mask for delay from active assertion (wake up)
									   // to enable_receiver

#define PHY_TIMING_CTRL4	0x9920	// PHY 
#define PHY_TIMING_CTRL4_IQCORR_Q_Q_COFF_M		0x0000001F // Mask for kcos_theta-1 for q correction
#define PHY_TIMING_CTRL4_IQCORR_Q_I_COFF_M		0x000007E0 // Mask for sin_theta for i correction
#define PHY_TIMING_CTRL4_IQCORR_Q_I_COFF_S		5		   // Shift for sin_theta for i correction
#define PHY_TIMING_CTRL4_IQCORR_ENABLE			0x00000800 // enable IQ correction 
#define PHY_TIMING_CTRL4_IQCAL_LOG_COUNT_MAX_M	0x0000F000 // Mask for max number of samples (logarithmic)
#define PHY_TIMING_CTRL4_IQCAL_LOG_COUNT_MAX_S	12		   // Shift for max number of samples 
#define PHY_TIMING_CTRL4_DO_IQCAL				0x00010000 // perform IQ calibration  

#define PHY_IQCAL_RES_PWR_MEAS_I	0x9c10	//PHY IQ calibration results - power measurement for I  
#define PHY_IQCAL_RES_PWR_MEAS_Q	0x9c14	//PHY IQ calibration results - power measurement for Q  
#define PHY_IQCAL_RES_IQ_CORR_MEAS	0x9c18	//PHY IQ calibration results - IQ correlation measurement

// Device ID defines for AR5211 devices
#define DEV_AR5211_PCI      0x0111
#define DEV_AR5211_PC       0x0011
#define DEV_AR5211_AP       0x0211
#define DEV_LEGACY          0x1107

#endif

