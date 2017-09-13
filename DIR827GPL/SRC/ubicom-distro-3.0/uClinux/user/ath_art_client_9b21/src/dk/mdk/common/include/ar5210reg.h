#ifndef __AR5210REGH__
#define __AR5210REGH__

/*
 *  Copyright (c) 2001 Atheros Communications, Inc., All Rights Reserved
 */

/* ar5210reg.h Register definitions for Atheros AR5210 chipset */

#ident  "ACI $Header: //depot/sw/src/dk/mdk/common/include/ar5210reg.h#4 $"

// DMA Control and Interrupt Registers
#define F2_TXDP0            0x0000  // MAC transmit descriptor queue 0 pointer
#define F2_TXDP1            0x0004  // MAC transmit descriptor queue 1 pointer

#define F2_CR               0x0008  // MAC Control Register - only write values of 1 have effect
#define F2_CR_TXE0           0x00000001 // Transmit queue 0 enable
#define F2_CR_TXE1           0x00000002 // Transmit queue 1 enable
#define F2_CR_RXE            0x00000004 // Receive enable
#define F2_CR_TXD0           0x00000008 // Transmit queue 0 disable
#define F2_CR_TXD1           0x00000010 // Transmit queue 1 disable
#define F2_CR_RXD            0x00000020 // Receive disable
#define F2_CR_SWI            0x00000040 // One-shot software interrupt

#define F2_RXDP             0x000C  // MAC receive queue descriptor pointer

#define F2_CFG              0x0014  // MAC configuration and status register
#define F2_CFG_SWTD          0x00000001 // byteswap tx descriptor words
#define F2_CFG_SWTB          0x00000002 // byteswap tx data buffer words
#define F2_CFG_SWRD          0x00000004 // byteswap rx descriptor words
#define F2_CFG_SWRB          0x00000008 // byteswap rx data buffer words
#define F2_CFG_SWRG          0x00000010 // byteswap register access data words
#define F2_CFG_PHOK          0x00000100 // PHY OK status
#define F2_CFG_EEBS          0x00000200 // EEPROM busy
#define F2_CFG_TXCNT_M       0x00007800 // Mask of Number of tx descriptors in DMA transmit queues
#define F2_CFG_TXCNT_S       11         // Shift for Number of tx descriptors in DMA transmit queues
#define F2_CFG_TXFSTAT       0x00008000 // DMA transmit status
#define F2_CFG_TXFSTRT       0x00010000 // DMA transmit re-enable after filtered stop

#define F2_ISR              0x001c  // MAC interrupt service register
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
#define F2_ISR_F2ERR         0x00080000 // An unexpected MAC error has occurred
#define F2_ISR_MCABT         0x00100000 // Master cycle abort interrupt
#define F2_ISR_SSERR         0x00200000 // SERR interrupt
#define F2_ISR_DPERR         0x00400000 // PCI bus parity error
#define F2_ISR_GPIO          0x01000000 // GPIO Interrupt
#define F2_ISR_RESV0         0xFE000000 // Reserved

#define F2_IMR              0x0020  // MAC interrupt mask register
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
#define F2_IMR_F2ERR         0x00080000 // An unexpected MAC error has occurred
#define F2_IMR_MCABT         0x00100000 // Master cycle abort interrupt
#define F2_IMR_SSERR         0x00200000 // SERR interrupt
#define F2_IMR_DPERR         0x00400000 // PCI bus parity error
#define F2_IMR_GPIO          0x01000000 // GPIO Interrupt
#define F2_IMR_RESV0         0xFE000000 // Reserved

#define F2_IER              0x0024  // MAC Interrupt enable register
#define F2_IER_ENABLE        0x00000001 // Global interrupt enable
#define F2_IER_DISABLE       0x00000000 // Global interrupt disable

#define F2_BCR              0x0028  // MAC beacon control register
#define F2_BCR_APMODE        0x00000000 // 0 for access point mode
#define F2_BCR_STAMODE       0x00000001 // 1 for station/adhoc mode 
#define F2_BCR_BDMAE         0x00000002 // beacon DMA enable
#define F2_BCR_TQ1PV         0x00000004 // TXQ1 packet valid (non-beacon)
#define F2_BCR_TQ1V          0x00000008 // TXQ1 valid, contains valid tx chain
#define F2_BCR_BCGET         0x00000010 // Force a get beacon from TXQ1

#define F2_BSR              0x002c  // MAC beacon status register
#define F2_BSR_BDLYSW        0x00000001 // beacon tx delayed because of TXDP1 not enabled
#define F2_BSR_BDLYDMA       0x00000002 // beacon tx delayed because DMA engine lagged
#define F2_BSR_TXQ1F         0x00000004 // DMA is fetching from TXQ1
#define F2_BSR_ATIMDLY       0x00000008 // ATIMs not ready at start of ATIM window
#define F2_BSR_SNPBCMD       0x00000010 // Snapped BCR AP/Adhoc bit
#define F2_BSR_SNPBDMAE      0x00000020 // Snapped BCR beacon DMA enable bit
#define F2_BSR_SNPTQ1FV      0x00000040 // Snapped BCR AltQ non-beacon valid bit
#define F2_BSR_SNPTQ1V       0x00000080 // Snapped BCR AltQ valid bit
#define F2_BSR_SNP_VALID     0x00000100 // Snapped BCR values are valid
#define F2_BSR_SWBACNT_M     0x00FF0000 // Mask of software beacon alert count
#define F2_BSR_SWBACNT_S     16         // Shift for software beacon alert count

#define F2_TXCFG            0x0030  // MAC tx DMA size config register
#define F2_TXCFG_TXFSTP      0x00000008 // Transmit DMA stop on filtered enable
#define F2_TXCFG_TXFULL      0x00000070 // DMA tx descriptor queue full threshold
#define F2_TXCFG_CONT_EN     0x00000080 // Enable continuous transmit mode
#define F2_RXCFG            0x0034  // MAC rx DMA size config register
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

#define F2_RPGTO            0x0050  // MAC receive frame gap timeout
#define F2_RPGTO_MASK        0x000003FF // Mask for receive frame gap timeout

#define F2_RPCNT            0x0054 // MAC receive frame count limit
#define F2_RPCNT_MASK        0x0000001F // Mask for receive frame count limit
  
#define F2_F2MISC           0x0058 // MAC miscellaneous control/status register
#define F2_MISC_LED_DELAY_M  0x001C0000 // Mask for LED decay rate
#define F2_MISC_LED_DELAY_S  18         // Shift for LED decay rate
#define F2_MISC_LED_BLINK_M  0x00E00000 // Mask for LED blink rate
#define F2_MISC_LED_BLINK_S  21         // Shift for LED blink rate

// DMA & PCI Registers in PCI space (usable during sleep)
#define F2_RC               0x4000  // Warm reset control register
#define F2_RC_PCU            0x00000001 // PCU reset 
#define F2_RC_DMA            0x00000002 // DMA & MMR reset
#define F2_RC_F2             0x00000004 // MAC reset
#define F2_RC_D2             0x00000008 // Baseband reset
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

#define F2_PCICFG           0x4010  // PCI configuration register
#define F2_PCICFG_EEAE       0x00000001 // enable software access to EEPROM 
#define F2_PCICFG_CLKRUNEN   0x00000004 // enable PCI CLKRUN function
#define F2_PCICFG_LED_PEND   0x00000020 // LED0&1 provide pending status
#define F2_PCICFG_LED_ACT    0x00000040 // LED0&1 provide activity status
#define F2_PCICFG_SL_INTEN   0x00000800 // enable interrupt line assertion when asleep
#define F2_PCICFG_BCTL		 0x00001000 // LED blink rate ctrl (0 = bytes/s, 1 = frames/s)
#define F2_PCICFG_SPWR_DN    0x00010000 // mask for sleep/awake indication

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
#define F2_SREV_FPGA         1
#define F2_SREV_D2PLUS       2
#define F2_SREV_D2PLUS_MS    3		// metal spin
#define F2_SREV_CRETE        4
#define F2_SREV_CRETE_MS     5		// FCS metal spin
#define F2_SREV_CRETE_MS23   7          // 2.3 metal spin (6 skipped)
#define F2_SREV_CRETE_23     8          // 2.3 full tape out

// EEPROM Registers in the MAC
#define F2_EEPROM_BASE	    0x6000  // Base register for EEPROM config
#define F2_EEPROM_RDATA	    0x6800  // EEPROM read data values
#define F2_EEPROM_STATUS    0x6c00  // Status register for EEPROM action status
#define F2_EEPROM_STAT_RDERR  0x0001 // Indicates a read error
#define F2_EEPROM_STAT_RDDONE 0x0002 // Indicates a read has completed
#define F2_EEPROM_STAT_WRERR  0x0004 // Indicates a write error
#define F2_EEPROM_STAT_WRDONE 0x0008 // Indicates a write has completed

// MAC PCU Registers
#define F2_STA_ID0          0x8000  // MAC station ID0 register - low 32 bits
#define F2_STA_ID1          0x8004  // MAC station ID1 register - upper 16 bits
#define F2_STA_ID1_SADH_MASK   0x0000FFFF // Mask for upper 16 bits of MAC addr
#define F2_STA_ID1_STA_AP      0x00010000 // Device is AP
#define F2_STA_ID1_AD_HOC      0x00020000 // Device is ad-hoc
#define F2_STA_ID1_PWR_SAV     0x00040000 // Power save reporting in self-generated frames
#define F2_STA_ID1_KSRCHDIS    0x00080000 // Key search disable
#define F2_STA_ID1_PSPOLLDIS   0x00100000 // Automatic PS poll disable
#define F2_STA_ID1_PCF         0x00200000 // Observe PCF
#define F2_STA_ID1_DESC_ANT    0x00400000 // Descriptor selects antenna
#define F2_STA_ID1_DEFAULT_ANT 0x00800000 // Toggles the antenna setting
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
#define F2_RSSI_THR_BM_THR_M 0x00000700 // Mask for Missed beacon threshold
#define F2_RSSI_THR_BM_THR_S 8          // Shift for Missed beacon threshold

#define F2_RETRY_LMT        0x801c  // MAC Retry limits
#define F2_RETRY_LMT_SH_M    0x0000000F // Mask for short frame retry limit
#define F2_RETRY_LMT_LG_M    0x000000F0 // Mask for long frame retry limit
#define F2_RETRY_LMT_LG_S    4          // Shift for long frame retry limit
#define F2_RETRY_LMT_SSH_M   0x00003F00 // Mask for short station retry limit
#define F2_RETRY_LMT_SSH_S   8          // Shift for short station retry limit
#define F2_RETRY_LMT_SLG_M   0x000FC000 // Mask for long station retry limit
#define F2_RETRY_LMT_SLG_S   14         // Shift for long station retry limit
#define F2_RETRY_LMT_CW_M    0x3FF00000 // Mask for minimum contention window
#define F2_RETRY_LMT_CW_S    20         // Shift for minimum contention window

#define F2_USEC             0x8020  // MAC transmit latency register
#define F2_USEC_M            0x0000007F // Mask for clock cycles in 1 usec
#define F2_USEC_32_M         0x00003F80 // Mask for number of 32MHz clock cycles in 1 usec
#define F2_USEC_32_S         7          // Shift for number of 32MHz clock cycles in 1 usec
#define F2_USEC_TX_LAT_M     0x000FC000 // Mask for tx latency to start of SIGNAL (usec)
#define F2_USEC_TX_LAT_S     14         // Shift for tx latency to start of SIGNAL (usec)
#define F2_USEC_RX_LAT_M     0x03F00000 // Mask for rx latency to start of SIGNAL (usec)
#define F2_USEC_RX_LAT_S     20         // Shift for rx latency to start of SIGNAL (usec)

#define F2_BEACON           0x8024  // MAC beacon control value/mode bits
#define F2_BEACON_PERIOD_MASK 0x0000FFFF  // Beacon period mask in TU/msec
#define F2_BEACON_TIM_MASK    0x007F0000  // Mask for byte offset of TIM start
#define F2_BEACON_TIM_S       16          // Shift for byte offset of TIM start
#define F2_BEACON_EN          0x00800000  // beacon enable
#define F2_BEACON_RESET_TSF   0x01000000  // Clears TSF to 0

#define F2_CFP_PERIOD       0x8028  // MAC CFP Interval (TU/msec)
#define F2_TIMER0           0x802c  // MAC Next beacon time (TU/msec)
#define F2_TIMER1           0x8030  // MAC DMA beacon alert time (1/8 TU)
#define F2_TIMER2           0x8034  // MAC Software beacon alert (1/8 TU)
#define F2_TIMER3           0x8038  // MAC ATIM window time

#define F2_IFS0             0x8040  // MAC IFS 0 timers
#define F2_IFS0_SIFS_M       0x000007FF  // SIFS in core clock cycles
#define F2_IFS0_DIFS_M       0x007FF800  // Mask for DIFS in core clock cycles
#define F2_IFS0_DIFS_S       11          // Shift for DIFS in core clock cycles

#define F2_IFS1             0x8044  // MAC IFS 1 timers
#define F2_IFS1_PIFS_M       0x00000FFF  // PIFS in core clock cycles
#define F2_IFS1_EIFS_M       0x03FFF000  // Mask for EIFS in core clock cycles
#define F2_IFS1_EIFS_S       12          // Shift for EIFS in core clock cycles
#define F2_IFS1_CS_EN        0x04000000  // Carrier sense enable

#define F2_CFP_DUR          0x8048  // MAC maximum CFP duration in TU

#define F2_RX_FILTER        0x804C  // MAC receive filter register
#define F2_RX_FILTER_ALL     0x00000000 // Disallow all frames
#define F2_RX_UCAST          0x00000001 // Allow unicast frames
#define F2_RX_MCAST          0x00000002 // Allow multicast frames
#define F2_RX_BCAST          0x00000004 // Allow broadcast frames
#define F2_RX_CONTROL        0x00000008 // Allow control frames
#define F2_RX_BEACON         0x00000010 // Allow beacon frames
#define F2_RX_PROM           0x00000020 // Promiscuous mode, all packets

#define F2_MCAST_FIL0       0x8050  // MAC multicast filter lower 32 bits
#define F2_MCAST_FIL1       0x8054  // MAC multicast filter upper 32 bits
#define F2_TX_MASK0         0x8058  // MAC tx block mask lower 32 bits
#define F2_TX_MASK1         0x805c  // MAC tx block mask upper 32 bits
#define F2_CLR_TMASK        0x8060  // MAC tx block filter mask clear
#define F2_TRIG_LEV         0x8064  // MAC transmit FIFO threshold-fill before tx

#define F2_DIAG_SW          0x8068  // MAC PCU control register
#define F2_DIAG_CACHE_ACK    0x00000001 // disable ACK when no valid key found
#define F2_DIAG_ACK_DIS      0x00000002 // disable ACK generation
#define F2_DIAG_CTS_DIS      0x00000004 // disable CTS generation
#define F2_DIAG_ENCRYPT_DIS  0x00000008 // disable encryption
#define F2_DIAG_DECRYPT_DIS  0x00000010 // disable decryption
#define F2_DIAG_TX_DIS       0x00000020 // disable transmit
#define F2_DIAG_RX_DIS       0x00000040 // disable receive
#define F2_DIAG_LOOP_EN      0x00000080 // enable tx-rx loopback
#define F2_DIAG_CORR_FCS     0x00000100 // corrupt FCS
#define F2_DIAG_CHAN_INFO    0x00000200 // dump channel info
#define F2_DIAG_EN_SCRAMSD   0x00000400 // enable fixed scrambler seed
#define F2_DIAG_SCRAM_SEED_M 0x0003F800 // Mask for fixed scrambler seed
#define F2_DIAG_SCRAM_SEED_S 11         // Shift for fixed scrambler seed
#define F2_DIAG_DIS_SEQ_INC  0x00040000 // freeze sequence number increment
#define F2_DIAG_FRAME_NV0    0x00080000 // accept frames of non-zero protocol version

#define F2_TSF_L32          0x806c  // MAC local clock lower 32 bits
#define F2_TSF_U32          0x8070  // MAC local clock upper 32 bits
#define F2_LAST_TSTP        0x8080  // MAC Time stamp of the last beacon received
#define F2_RETRY_CNT        0x8084  // MAC current short and long retry counts
#define F2_RETRY_CNT_SSH_M   0x0000003F // Mask for Current station short retry count
#define F2_RETRY_CNT_SLG_M   0x00000FC0 // Mask for Current station long retry count
#define F2_RETRY_CNT_SLG_S   6          // Shift for Current station long retry count

#define F2_BACKOFF          0x8088  // MAC contention window size and back-off count
#define F2_BACKOFF_CW_M      0x000003FF // Current contention window size
#define F2_BACKOFF_CNT_M     0x03FF0000 // Mask for Back-off count
#define F2_BACKOFF_CNT_S     16         // Shift for Back-off count

#define F2_NAV              0x808c  // MAC current NAV value
#define F2_RTS_OK           0x8090  // MAC RTS exchange success counter
#define F2_RTS_FAIL         0x8094  // MAC RTS exchange failure counter
#define F2_ACK_FAIL         0x8098  // MAC ACK failure counter
#define F2_FCS_FAIL         0x809C  // FCS check failure counter
#define F2_BEACON_CNT       0x80A0  // Valid beacon counter

//
// Key table is 64 entries of 8 double-words each
//
#ifndef SWIG // SWIG cannot parse structures
#define D2_KEY_TABLE		0x9000
#define D2_KEY_NUM_ENTRIES	64
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

#define PHY_FRAME_CONTROL   0x9804  // PHY frame control register
#define PHY_FC_TURBO_MODE    0x00000001 // Set turbo mode bits
#define PHY_FC_TURBO_SHORT   0x00000002 // Set short symbols to turbo mode setting
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

// Device ID defines for AR5210 devices
#define DEV_E2_PCI          0x0101 
#define DEV_E5_PCI          0x0102
#define DEV_E7_PCI          0x0103
#define DEV_E7_PCI_PA       0x0104
#define DEV_E7_PC_PA        0x0004
#define DEV_E9_PCI_PA       0x0105
#define DEV_E9_PC_PA        0x0005
#define DEV_E9_PC_ANT       0x0006
#define DEV_AR5210_PCI      0x0107
#define DEV_AR5210_PC       0x0007
#define DEV_AR5210_AP       0x0207
#define DEV_AR5001          0x0010
#define DEV_AR5001_QMAC     0x0011
#define DEV_AR5001_QMAC_FPGA 0xf011
#define DEV_LEGACY          0x1107


#endif


