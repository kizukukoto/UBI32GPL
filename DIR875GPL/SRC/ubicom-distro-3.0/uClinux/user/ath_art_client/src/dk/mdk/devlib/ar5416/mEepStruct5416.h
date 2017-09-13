/*
 * Copyright (c) 2002-2006 Atheros Communications, Inc.
 * All rights reserved.
 *
 */

#ident  "ACI $Id: //depot/sw/src/dk/mdk/devlib/ar5416/mEepStruct5416.h#41 $, $Header: //depot/sw/src/dk/mdk/devlib/ar5416/mEepStruct5416.h#41 $"

#ifndef _AR5416_EEPROM_STRUCT_H_
#define _AR5416_EEPROM_STRUCT_H_

#if defined(WIN32) || defined(WIN64)
#pragma pack (push, 1)
#endif
    
#define AR5416_EEP_VER               0xE
#define AR5416_EEP_VER_MINOR_MASK    0xFFF
#define AR5416_EEP_NO_BACK_VER       0x1
#define AR5416_EEP_MINOR_VER_2       0x2  // Adds modal params txFrameToPaOn, txFrametoDataStart, ht40PowerInc
#define AR5416_EEP_MINOR_VER_3       0x3  // Adds modal params bswAtten, bswMargin, swSettle and base OpFlags for HT20/40 Disable
#define AR5416_EEP_MINOR_VER_4       0x4  // Nothing added.  Needed for tracking higher target powers
#define AR5416_EEP_MINOR_VER_5       0x5  // Added regulatory flags for Japan HT40 operations
#define AR5416_EEP_MINOR_VER_6       0x6  // Added ASPM, WA, WOW flags
#define AR5416_EEP_MINOR_VER_7       0x7  // Added xpa bias vs frequency table
#define AR5416_EEP_MINOR_VER_8       0x8  // Added WOW bit in eeprom strcuture in eepMisc field
#define AR5416_EEP_MINOR_VER         0x12  //add new fields needed for merlin
#define AR5416_EEP_MINOR_VER_b		 AR5416_EEP_MINOR_VER

// 16-bit offset location start of calibration struct
#define AR5416_EEP_START_LOC         256
#define AR5416_NUM_5G_CAL_PIERS      8
#define AR5416_NUM_2G_CAL_PIERS      4
#define AR5416_NUM_5G_20_TARGET_POWERS  8
#define AR5416_NUM_5G_40_TARGET_POWERS  8
#define AR5416_NUM_2G_CCK_TARGET_POWERS 3
#define AR5416_NUM_2G_20_TARGET_POWERS  4
#define AR5416_NUM_2G_40_TARGET_POWERS  4
#define AR5416_NUM_CTLS              24
#define AR5416_NUM_BAND_EDGES        8
#define AR5416_NUM_PD_GAINS          4
#define AR5416_PD_GAINS_IN_MASK      4
#define AR5416_PD_GAIN_ICEPTS        5
#define AR5416_EEPROM_MODAL_SPURS    5
#define AR5416_MAX_RATE_POWER        63
#define AR5416_NUM_PDADC_VALUES      128
#define AR5416_NUM_RATES             16
#define AR5416_BCHAN_UNUSED          0xFF
#define AR5416_MAX_PWR_RANGE_IN_HALF_DB 64
#define AR5416_OPFLAGS_11A           0x01
#define AR5416_OPFLAGS_11G           0x02
#define AR5416_OPFLAGS_5G_HT40       0x04
#define AR5416_OPFLAGS_2G_HT40       0x08
#define AR5416_OPFLAGS_5G_HT20       0x10
#define AR5416_OPFLAGS_2G_HT20       0x20
#define AR5416_EEPMISC_BIG_ENDIAN    0x01
#define AR5416_EEPMISC_WOW           0x02

#define FREQ2FBIN(x,y) ((y) ? ((x) - 2300) : (((x) - 4800) / 5))
#define AR5416_MAX_CHAINS            3
#define AR5416_ANT_16S               25
#define AR5416_FUTURE_MODAL_SZ       6

#define AR5416_NUM_ANT_CHAIN_FIELDS     7
#define AR5416_NUM_ANT_COMMON_FIELDS    4
#define AR5416_SIZE_ANT_CHAIN_FIELD     3
#define AR5416_SIZE_ANT_COMMON_FIELD    4
#define AR5416_ANT_CHAIN_MASK           0x7
#define AR5416_ANT_COMMON_MASK          0xf
#define AR5416_CHAIN_0_IDX              0
#define AR5416_CHAIN_1_IDX              1
#define AR5416_CHAIN_2_IDX              2

#define AR928X_NUM_ANT_CHAIN_FIELDS     6
#define AR928X_SIZE_ANT_CHAIN_FIELD     2
#define AR928X_ANT_CHAIN_MASK           0x3

//delta from which to start power to pdadc table
#define AR5416_PWR_TABLE_OFFSET  -5


typedef enum ConformanceTestLimits {
    FCC        = 0x10,
    MKK        = 0x40,
    ETSI       = 0x30,
    SD_NO_CTL  = 0xE0,
    NO_CTL     = 0xFF,
    CTL_MODE_M = 0xF,
    CTL_11A    = 0,
    CTL_11B    = 1,
    CTL_11G    = 2,
    CTL_TURBO  = 3,
    CTL_108G   = 4,
    CTL_2GHT20 = 5,
    CTL_5GHT20 = 6,
    CTL_2GHT40 = 7,
    CTL_5GHT40 = 8,
} ATH_CTLS;

/* Capabilities Enum */
typedef enum {
    EEPCAP_COMPRESS_DIS  = 0x0001,
    EEPCAP_AES_DIS       = 0x0002,
    EEPCAP_FASTFRAME_DIS = 0x0004,
    EEPCAP_BURST_DIS     = 0x0008,
    EEPCAP_MAXQCU_M      = 0x01F0,
    EEPCAP_MAXQCU_S      = 4,
    EEPCAP_HEAVY_CLIP_EN = 0x0200,
    EEPCAP_KC_ENTRIES_M  = 0xF000,
    EEPCAP_KC_ENTRIES_S  = 12,
} EEPROM_CAPABILITIES;

typedef enum Ar5416_Rates {
    rate6mb,  rate9mb,  rate12mb, rate18mb,
    rate24mb, rate36mb, rate48mb, rate54mb,
    rate1l,   rate2l,   rate2s,   rate5_5l,
    rate5_5s, rate11l,  rate11s,  rateXr,
    rateHt20_0, rateHt20_1, rateHt20_2, rateHt20_3,
    rateHt20_4, rateHt20_5, rateHt20_6, rateHt20_7,
    rateHt40_0, rateHt40_1, rateHt40_2, rateHt40_3,
    rateHt40_4, rateHt40_5, rateHt40_6, rateHt40_7,
    rateDupCck, rateDupOfdm, rateExtCck, rateExtOfdm,
    Ar5416RateSize
} AR5416_RATES;

typedef struct eepFlags {
    A_UINT8  opFlags;
    A_UINT8  eepMisc;
} __ATTRIB_PACK EEP_FLAGS;

#define AR5416_CHECKSUM_LOCATION (AR5416_EEP_START_LOC + 1)
typedef struct BaseEepHeader {
    A_UINT16  length;
    A_UINT16  checksum;
    A_UINT16  version;
    EEP_FLAGS opCapFlags;
    A_UINT16  regDmn[2];
    A_UINT8   macAddr[6];
    A_UINT8   rxMask;
    A_UINT8   txMask;
    A_UINT16  rfSilent;
    A_UINT16  blueToothOptions;
    A_UINT16  deviceCap;
    A_UINT32  binBuildNumber;
    A_UINT8   deviceType; // takes lower byte in eeprom location
    A_UINT8   pwdclkind;  // #bits used: 1, bit0: field "an_top2_pwdclkind", takes upper byte in eeprom location
    A_UINT8   fastClk5g;  // #bits used: 1, bit0: fast_clock_5g_channel bit, takes lower byte in eeprom location
    A_UINT8   divChain;   // #bits used: all, indicates which chain is used (0,1,2, etc) (MB93 specific), takes upper byte in eeprom location
    A_UINT8   rxGainType; // #bits used: all, indicates Rx gain table support. 0: 23dB backoff, 1: 13dB backoff, 2: original
    A_UINT8   dacHiPwrMode; // #bits used: 1, bit0: 1: use the DAC high power mode, 0: don't use the DAC high power mode (1 for MB91 only)
    A_UINT8   openLoopPwrCntl; //#bits used: 1, bit0: 1: use open loop power control, 0: use closed loop power control
    A_UINT8   dacLpMode;  // #bits used: 2: bit0, bit1.  
    A_UINT8   futureBase[26];
} __ATTRIB_PACK BASE_EEP_HEADER; // 64 B

typedef struct spurChanStruct {
    A_UINT16 spurChan;
    A_UINT8  spurRangeLow;
    A_UINT8  spurRangeHigh; 
} __ATTRIB_PACK SPUR_CHAN;

typedef struct ModalEepHeader {
    A_UINT32  antCtrlChain[AR5416_MAX_CHAINS];       // 12
    A_UINT32  antCtrlCommon;                         // 16 (32 MB93)
    A_INT8    antennaGainCh[AR5416_MAX_CHAINS];      // 3
    A_UINT8   switchSettling;                        // 1
    A_UINT8   txRxAttenCh[AR5416_MAX_CHAINS];        // 3  //xatten1_hyst_margin for merlin (0x9848/0xa848 13:7)
    A_UINT8   rxTxMarginCh[AR5416_MAX_CHAINS];       // 3  //xatten2_hyst_margin for merlin (0x9848/0xa848 20:14)
    A_INT8    adcDesiredSize;                        // 1
    A_INT8    pgaDesiredSize;                        // 1
    A_UINT8   xlnaGainCh[AR5416_MAX_CHAINS];         // 3
    A_UINT8   txEndToXpaOff;                         // 1
    A_UINT8   txEndToRxOn;                           // 1
    A_UINT8   txFrameToXpaOn;                        // 1
    A_UINT8   thresh62;                              // 1
    A_INT8    noiseFloorThreshCh[AR5416_MAX_CHAINS]; // 3
    A_UINT8   xpdGain;                               // 1
    A_UINT8   xpd;                                   // 1
    A_INT8    iqCalICh[AR5416_MAX_CHAINS];           // 1
    A_INT8    iqCalQCh[AR5416_MAX_CHAINS];           // 1
    A_UINT8   pdGainOverlap;                         // 1
    A_UINT8   ob;                                    // 1 //for merlin this is chain0 
    A_UINT8   db;                                    // 1 //for merlin this is chain0
    A_UINT8   xpaBiasLvl;                            // 1
    A_UINT8   available1;                            // unused
    A_UINT8   available2;                            // unused
    A_UINT8   txFrameToDataStart;                    // 1
    A_UINT8   txFrameToPaOn;                         // 1
    A_UINT8   ht40PowerIncForPdadc;                  // 1
    A_UINT8   bswAtten[AR5416_MAX_CHAINS];           // 3  //xatten1_db for merlin (0xa20c/b20c 5:0)
    A_UINT8   bswMargin[AR5416_MAX_CHAINS];          // 3  //xatten1_margin for merlin (0xa20c/b20c 16:12
    A_UINT8   swSettleHt40;                          // 1
    A_UINT8   xatten2Db[AR5416_MAX_CHAINS];          // 3 //new for merlin (0xa20c/b20c 11:6)
    A_UINT8   xatten2Margin[AR5416_MAX_CHAINS];      // 3 //new for merlin (0xa20c/b20c 21:17)
    A_UINT8   ob_ch1;                                // 1 //ob and db become chain specific in merlin
    A_UINT8   db_ch1;                                // 1 //merlin is 2 chain, only adding 1 extra chain for now
    A_UINT8   lna_cntl;                              // 8 // bit0: xlnabufmode 
                                                          // bit1, bit2: xlnaisel
                                                          // bit3: xlnabufin
                                                          // bit4: femBandSelectUsed
                                                          // bit5: localbias
                                                          // bit6: force_xpaon
                                                          // bit7: useAnt1
    A_UINT8   miscBits;                              // 2 // bit0, bit1: bb_tx_dac_scale_cck
    A_UINT16  xpaBiasLvlFreq[3];                     // reserved for sowl
    A_INT8    futureModal[6];                        // used to store noise floor cal results in the following order:
    SPUR_CHAN spurChans[AR5416_EEPROM_MODAL_SPURS];  // 20 B
} __ATTRIB_PACK MODAL_EEP_HEADER;                    // == 100 B

typedef struct calDataPerFreq {
    A_UINT8 pwrPdg[AR5416_NUM_PD_GAINS][AR5416_PD_GAIN_ICEPTS];
    A_UINT8 vpdPdg[AR5416_NUM_PD_GAINS][AR5416_PD_GAIN_ICEPTS];
} __ATTRIB_PACK CAL_DATA_PER_FREQ;

typedef struct CalTargetPowerLegacy {
    A_UINT8  bChannel;
    A_UINT8  tPow2x[4];
} __ATTRIB_PACK CAL_TARGET_POWER_LEG;

typedef struct CalTargetPowerHt {
    A_UINT8  bChannel;
    A_UINT8  tPow2x[8];
} __ATTRIB_PACK CAL_TARGET_POWER_HT;

#if defined(ARCH_BIG_ENDIAN) || defined(BIG_ENDIAN)
typedef struct CalCtlEdges {
    A_UINT8  bChannel;
    A_UINT8  flag   :2,
             tPower :6;
} __ATTRIB_PACK CAL_CTL_EDGES;
#else
typedef struct CalCtlEdges {
    A_UINT8  bChannel;
    A_UINT8  tPower :6,
             flag   :2;
} __ATTRIB_PACK CAL_CTL_EDGES;
#endif

typedef struct CalCtlData {
    CAL_CTL_EDGES  ctlEdges[AR5416_MAX_CHAINS][AR5416_NUM_BAND_EDGES];
} __ATTRIB_PACK CAL_CTL_DATA;

typedef struct ar5416Eeprom {
    BASE_EEP_HEADER    baseEepHeader;         // 64 B
    A_UINT8   custData[64];                   // 64 B
    MODAL_EEP_HEADER   modalHeader[2];        // 200 B           
    A_UINT8            calFreqPier5G[AR5416_NUM_5G_CAL_PIERS];        
    A_UINT8            calFreqPier2G[AR5416_NUM_2G_CAL_PIERS];
    CAL_DATA_PER_FREQ  calPierData5G[AR5416_MAX_CHAINS][AR5416_NUM_5G_CAL_PIERS];        
    CAL_DATA_PER_FREQ  calPierData2G[AR5416_MAX_CHAINS][AR5416_NUM_2G_CAL_PIERS];        
    CAL_TARGET_POWER_LEG calTargetPower5G[AR5416_NUM_5G_20_TARGET_POWERS];
    CAL_TARGET_POWER_HT  calTargetPower5GHT20[AR5416_NUM_5G_20_TARGET_POWERS];
    CAL_TARGET_POWER_HT  calTargetPower5GHT40[AR5416_NUM_5G_40_TARGET_POWERS];
    CAL_TARGET_POWER_LEG calTargetPowerCck[AR5416_NUM_2G_CCK_TARGET_POWERS];
    CAL_TARGET_POWER_LEG calTargetPower2G[AR5416_NUM_2G_20_TARGET_POWERS];
    CAL_TARGET_POWER_HT  calTargetPower2GHT20[AR5416_NUM_2G_20_TARGET_POWERS];
    CAL_TARGET_POWER_HT  calTargetPower2GHT40[AR5416_NUM_2G_40_TARGET_POWERS];
    A_UINT8            ctlIndex[AR5416_NUM_CTLS];
    CAL_CTL_DATA       ctlData[AR5416_NUM_CTLS];
    A_UINT8            padding;
} __ATTRIB_PACK AR5416_EEPROM;

typedef struct eepromChannelFreqMhz {
    A_UINT16 synthCenter;
    A_UINT16 ctlCenter;
    A_UINT16 extCenter;
    A_UINT16 ht40Center;
    A_UINT16 txMask;
    A_UINT16 activeTxChains;
    A_BOOL   is2GHz;
    A_BOOL   isHt40;
} __ATTRIB_PACK EEPROM_CHANNEL;

#if defined(WIN32) || defined(WIN64)
#pragma pack (pop)
#endif

#endif //_AR5416_EEPROM_STRUCT_H_
