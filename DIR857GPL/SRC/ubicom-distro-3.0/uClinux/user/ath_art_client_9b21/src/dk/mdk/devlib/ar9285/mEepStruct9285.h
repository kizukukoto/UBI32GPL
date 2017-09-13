/*
 * Copyright (c) 2002-2006 Atheros Communications, Inc.
 * All rights reserved.
 *
 */

#ident  "ACI $Id: //depot/sw/src/dk/mdk/devlib/ar9285/mEepStruct9285.h#5 $, $Header: //depot/sw/src/dk/mdk/devlib/ar9285/mEepStruct9285.h#5 $"

#ifndef _AR9285_EEPROM_STRUCT_H_
#define _AR9285_EEPROM_STRUCT_H_

#if defined(WIN32) || defined(WIN64)
#pragma pack (push, 1)
#endif
    
#define AR9285_EEP_VER               0xE
#define AR9285_EEP_VER_MINOR_MASK    0xFFF
#define AR9285_EEP_NO_BACK_VER       0x1
#define AR9285_EEP_MINOR_VER_2       0x2  // Adds modal params txFrameToPaOn, txFrametoDataStart, ht40PowerInc
#define AR9285_EEP_MINOR_VER_3       0x3  // Adds modal params bswAtten, bswMargin, swSettle and base OpFlags for HT20/40 Disable
#define AR9285_EEP_MINOR_VER_4       0x4  // Nothing added.  Needed for tracking higher target powers
                                          // Added regulatory flags for Japan HT40 operations
#define AR9285_EEP_MINOR_VER_5		 0x5
#define AR9285_EEP_MINOR_VER_d		 0xd  // for kite
#define AR9285_EEP_MINOR_VER_e		 0xe  // add TX_deiversity
#define AR9285_EEP_MINOR_VER         0xe  
#define AR9285_EEP_MINOR_VER_b		 AR9285_EEP_MINOR_VER

// 16-bit offset location start of calibration struct
#define AR9285_EEP_START_LOC         64
#define AR9285_NUM_5G_CAL_PIERS      8
#define AR9285_NUM_2G_CAL_PIERS      3
#define AR9285_NUM_5G_20_TARGET_POWERS  8
#define AR9285_NUM_5G_40_TARGET_POWERS  8
#define AR9285_NUM_2G_CCK_TARGET_POWERS 3
#define AR9285_NUM_2G_20_TARGET_POWERS  3
#define AR9285_NUM_2G_40_TARGET_POWERS  3
#define AR9285_NUM_CTLS              12
#define AR9285_NUM_BAND_EDGES        4
#define AR9285_NUM_PD_GAINS          2
#define AR9285_PD_GAINS_IN_MASK      4
#define AR9285_PD_GAIN_ICEPTS        5
#define AR9285_EEPROM_MODAL_SPURS    5
#define AR9285_MAX_RATE_POWER        63
#define AR9285_NUM_PDADC_VALUES      128
#define AR9285_NUM_RATES             16
#define AR9285_BCHAN_UNUSED          0xFF
#define AR9285_MAX_PWR_RANGE_IN_HALF_DB 64
#define AR9285_OPFLAGS_11A           0x01
#define AR9285_OPFLAGS_11G           0x02
#define AR9285_OPFLAGS_5G_HT40       0x04
#define AR9285_OPFLAGS_2G_HT40       0x08
#define AR9285_OPFLAGS_5G_HT20       0x10
#define AR9285_OPFLAGS_2G_HT20       0x20
#define AR9285_EEPMISC_BIG_ENDIAN    0x01
#define FREQ2FBIN(x,y) ((y) ? ((x) - 2300) : (((x) - 4800) / 5))
#define AR9285_MAX_CHAINS            1
#define AR9285_ANT_16S               32
#define AR9285_custdatasize          20

#define AR9285_NUM_ANT_CHAIN_FIELDS     7
#define AR9285_NUM_ANT_COMMON_FIELDS    8
#define AR9285_SIZE_ANT_CHAIN_FIELD     3
#define AR9285_SIZE_ANT_COMMON_FIELD    4
#define AR9285_ANT_CHAIN_MASK           0x7
#define AR9285_ANT_COMMON_MASK          0xf
#define AR9285_CHAIN_0_IDX              0
#define AR9285_CHAIN_1_IDX              1
#define AR9285_CHAIN_2_IDX              2

#define AR9285_NUM_ANT_CHAIN_FIELDS     7

//delta from which to start power to pdadc table
#define AR9285_PWR_TABLE_OFFSET  -5

#define AR9285_CHECKSUM_LOCATION (AR9285_EEP_START_LOC + 1)

typedef struct Ar9285_BaseEepHeader {
    A_UINT16  length;     /* 0x40 */
    A_UINT16  checksum;   /* 0x41 */
    A_UINT16  version;    /* 0x42 */
    EEP_FLAGS opCapFlags; /* 0x43 */
    A_UINT16  regDmn[2];  /* 0x44, 0x45 */
    A_UINT8   macAddr[6]; /* 0x46, 0x47, 0x48 */
    A_UINT8   rxMask;     /* 0x49 lower byte */
    A_UINT8   txMask;     /* 0x49 upper byte */
    A_UINT16  rfSilent;   /* 0x4A */
    A_UINT16  blueToothOptions; /* 0x4B */
    A_UINT16  deviceCap;        /* 0x4C */
    A_UINT32  binBuildNumber;   /* 0x4D */
    A_UINT8   deviceType;       /* 0x4E */
    A_UINT8   txGainType;       /* 0x4F */

} __ATTRIB_PACK Ar9285_BASE_EEP_HEADER; // 32 B

typedef struct Ar9285_ModalEepHeader {
    A_UINT32  antCtrlChain[AR9285_MAX_CHAINS];       // 4
    A_UINT32  antCtrlCommon;                         // 4
    A_INT8    antennaGainCh[AR9285_MAX_CHAINS];      // 1
    A_UINT8   switchSettling;                        // 1
    A_UINT8   txRxAttenCh[AR9285_MAX_CHAINS];        // 1  //xatten1_hyst_margin for merlin (0x9848/0xa848 13:7)
    A_UINT8   rxTxMarginCh[AR9285_MAX_CHAINS];       // 1  //xatten2_hyst_margin for merlin (0x9848/0xa848 20:14)
    A_INT8    adcDesiredSize;                        // 1
    A_INT8    pgaDesiredSize;                        // 1
    A_UINT8   xlnaGainCh[AR9285_MAX_CHAINS];         // 1
    A_UINT8   txEndToXpaOff;                         // 1
    A_UINT8   txEndToRxOn;                           // 1
    A_UINT8   txFrameToXpaOn;                        // 1
    A_UINT8   thresh62;                              // 1
    A_INT8    noiseFloorThreshCh[AR9285_MAX_CHAINS]; // 1
    A_UINT8   xpdGain;                               // 1
    A_UINT8   xpd;                                   // 1
    A_INT8    iqCalICh[AR9285_MAX_CHAINS];           // 1
    A_INT8    iqCalQCh[AR9285_MAX_CHAINS];           // 1
    A_UINT8   pdGainOverlap;                         // 1
    A_UINT8   ob;                                    // 1 //for merlin this is chain0 
    A_UINT8   db;                                    // 1 //for merlin this is chain0
    A_UINT8   xpaBiasLvl;                            // 1
//    A_UINT8   pwrDecreaseFor2Chain;                  // 1
//    A_UINT8   pwrDecreaseFor3Chain;                  // 1
    A_UINT8   txFrameToDataStart;                    // 1
    A_UINT8   txFrameToPaOn;                         // 1   --> 30 B
    A_UINT8   ht40PowerIncForPdadc;                  // 1
    A_UINT8   bswAtten[AR9285_MAX_CHAINS];           // 1  //xatten1_db for merlin (0xa20c/b20c 5:0)
    A_UINT8   bswMargin[AR9285_MAX_CHAINS];          // 1  //xatten1_margin for merlin (0xa20c/b20c 16:12
    A_UINT8   swSettleHt40;                          // 1
    A_UINT8   xatten2Db[AR9285_MAX_CHAINS];          // 1  //new for merlin (0xa20c/b20c 11:6)
    A_UINT8   xatten2Margin[AR9285_MAX_CHAINS];      // 1  //new for merlin (0xa20c/b20c 21:17)
	A_UINT8   db2;									 // 1
	A_UINT8   version;								 // 1
//    A_UINT8   ob_ch1;                                // 1  //ob and db become chain specific in merlin
//    A_UINT8   db_ch1;                                // 1  //merlin is 2 chain, only adding 1 extra chain for now
	A_UINT8   ob_23;
	A_UINT8   ob_4;
	A_UINT8   db_23;
	A_UINT8   db_4;
	A_UINT8   db2_23;
	A_UINT8   db2_4;
	A_UINT8   tx_diversity;
    A_UINT8   futureModal[3];                       //3
    SPUR_CHAN spurChans[AR9285_EEPROM_MODAL_SPURS];  // 20 B       
} __ATTRIB_PACK Ar9285_MODAL_EEP_HEADER;                    // == 68 B

typedef struct Ar9285_calDataPerFreq {
    A_UINT8 pwrPdg[AR9285_NUM_PD_GAINS][AR9285_PD_GAIN_ICEPTS];
    A_UINT8 vpdPdg[AR9285_NUM_PD_GAINS][AR9285_PD_GAIN_ICEPTS];
} __ATTRIB_PACK Ar9285_CAL_DATA_PER_FREQ;


typedef struct Ar9285_CalCtlData {
    CAL_CTL_EDGES  ctlEdges[AR9285_MAX_CHAINS][AR9285_NUM_BAND_EDGES];
} __ATTRIB_PACK Ar9285_CAL_CTL_DATA;

typedef struct ar9285Eeprom {
    Ar9285_BASE_EEP_HEADER    baseEepHeader;         // 32 B
    A_UINT8   custData[20];                   // 20 B
    Ar9285_MODAL_EEP_HEADER   modalHeader[1];        // 68 B           
    A_UINT8            calFreqPier2G[AR9285_NUM_2G_CAL_PIERS];  // 3 B
    Ar9285_CAL_DATA_PER_FREQ  calPierData2G[AR9285_MAX_CHAINS][AR9285_NUM_2G_CAL_PIERS]; // 60 B       
    CAL_TARGET_POWER_LEG calTargetPowerCck[AR9285_NUM_2G_CCK_TARGET_POWERS]; // 15 B
    CAL_TARGET_POWER_LEG calTargetPower2G[AR9285_NUM_2G_20_TARGET_POWERS];   // 15 B
    CAL_TARGET_POWER_HT  calTargetPower2GHT20[AR9285_NUM_2G_20_TARGET_POWERS]; // 27 B
    CAL_TARGET_POWER_HT  calTargetPower2GHT40[AR9285_NUM_2G_40_TARGET_POWERS]; // 27 B
    A_UINT8            ctlIndex[AR9285_NUM_CTLS]; // 12 B
    Ar9285_CAL_CTL_DATA       ctlData[AR9285_NUM_CTLS]; // 96 B
    A_UINT8            padding;
} __ATTRIB_PACK ar9285_EEPROM;

#if defined(WIN32) || defined(WIN64)
#pragma pack (pop)
#endif

#endif //_AR9285_EEPROM_STRUCT_H_
