/** \file
 * COPYRIGHT (C) Atheros Communications Inc., 2009
 *
 * - Filename    : mEepStruct9287.H
 * - Description : eeprom definitions for kiwi chipset 
 * - $Id: //depot/sw/src/dk/mdk/devlib/ar9287/mEepStruct9287.h#9 $
 * - $DateTime: 2009/09/10 13:21:07 $
 * - History :
 * - Date      Author     Modification
 ******************************************************************************/
#ifndef mEepStruct9287_H
#define mEepStruct9287_H

/**** Includes ****************************************************************/

/**** Definitions *************************************************************/
#define AR9287_EEP_VER               0xE
#define AR9287_EEP_VER_MINOR_MASK    0xFFF
#define AR9287_EEP_MINOR_VER_1       0x1  
#define AR9287_EEP_MINOR_VER_2       0x2
#define AR9287_EEP_MINOR_VER_3       0x3
#define AR9287_EEP_MINOR_VER_4       0x4
#define AR9287_EEP_MINOR_VER         AR9287_EEP_MINOR_VER_4
#define AR9287_EEP_MINOR_VER_b		 AR9287_EEP_MINOR_VER
#define AR9287_EEP_NO_BACK_VER       AR9287_EEP_MINOR_VER_1

// 16-bit offset location start of calibration struct
#define AR9287_EEP_START_LOC            128 
#define AR9287_NUM_2G_CAL_PIERS         3
#define AR9287_NUM_2G_CCK_TARGET_POWERS 3
#define AR9287_NUM_2G_20_TARGET_POWERS  3
#define AR9287_NUM_2G_40_TARGET_POWERS  3
#define AR9287_NUM_CTLS              12 /* max number of CTL groups to store in the eeprom */
#define AR9287_NUM_BAND_EDGES        4  /* max number of band edges per CTL group */
#define AR9287_NUM_PD_GAINS             4
#define AR9287_PD_GAINS_IN_MASK         4
#define AR9287_PD_GAIN_ICEPTS           1
#define AR9287_EEPROM_MODAL_SPURS       5
#define AR9287_MAX_RATE_POWER           63
#define AR9287_NUM_PDADC_VALUES         128
#define AR9287_NUM_RATES                16
#define AR9287_BCHAN_UNUSED             0xFF
#define AR9287_MAX_PWR_RANGE_IN_HALF_DB 64
#define AR9287_OPFLAGS_11A              0x01
#define AR9287_OPFLAGS_11G              0x02
#define AR9287_OPFLAGS_2G_HT40          0x08
#define AR9287_OPFLAGS_2G_HT20          0x20
#define AR9287_OPFLAGS_5G_HT40          0x04
#define AR9287_OPFLAGS_5G_HT20          0x10
#define AR9287_EEPMISC_BIG_ENDIAN       0x01
#define AR9287_EEPMISC_WOW              0x02
#define AR9287_MAX_CHAINS               2
#define AR9287_ANT_16S                  32
#define AR9287_custdatasize             20

#define AR9287_NUM_ANT_CHAIN_FIELDS     6
#define AR9287_NUM_ANT_COMMON_FIELDS    4
#define AR9287_SIZE_ANT_CHAIN_FIELD     2
#define AR9287_SIZE_ANT_COMMON_FIELD    4
#define AR9287_ANT_CHAIN_MASK           0x3
#define AR9287_ANT_COMMON_MASK          0xf
#define AR9287_CHAIN_0_IDX              0
#define AR9287_CHAIN_1_IDX              1
#define AR9287_DATA_SZ                  32 /* customer data size */

//delta from which to start power to pdadc table
#define AR9287_PWR_TABLE_OFFSET  -5

#define AR9287_CHECKSUM_LOCATION (AR9287_EEP_START_LOC + 1)

#if defined(WIN32) || defined(WIN64)
#pragma pack (push, 1)
#endif

typedef struct ar9287_BaseEepHeader {
    A_UINT16  length;     /* eeploc: 0x80 */
    A_UINT16  checksum;   /* eeploc: 0x81 */
    A_UINT16  version;    /* eeploc: 0x82 */
    EEP_FLAGS opCapFlags; /* eeploc: 0x83 */
    A_UINT16  regDmn[2];  /* eeploc: 0x84, 0x85 */
    A_UINT8   macAddr[6]; /* eeploc: 0x86, 0x87, 0x88 */
    A_UINT8   rxMask;     /* eeploc: 0x89 lower byte */
    A_UINT8   txMask;     /* eeploc: 0x89 upper byte */
    A_UINT16  rfSilent;   /* eeploc: 0x8A */
    A_UINT16  blueToothOptions;   /* eeploc: 0x8B */
    A_UINT16  deviceCap;          /* eeploc: 0x8C */
    A_UINT32  binBuildNumber;     /* eeploc: 0x8D, 0x8E */
    A_UINT8   deviceType;         /* eeploc: 0x8F lower byte */
    A_UINT8   openLoopPwrCntl;    /* eeploc: 0x8F upper byte */ //#bits used: 1, bit0: 1: use open loop power control, 0: use closed loop power control
    A_INT8    pwrTableOffset;     /* eeploc: 0x90 lower byte */ // offset in dB to be added to beginning of pdadc table in calibration 
    A_INT8    tempSensSlope;      /* eeploc: 0x90 upper byte */ // temperature sensor slope used for temp compensation
    A_INT8    tempSensSlopePalOn; /* eeploc: 0x91 lower byte */ // temperature sensor slope used for temp compensation when PAL is ON
    A_UINT8   palPwrThresh;       /* eeploc: 0x91 upper byte */ // PAL power threshold, # bits used 6
    A_UINT8   palEnableModes;     /* eeploc: 0x92 lower byte */ //bit0: 1: enable pal_2ch_HT40_QAM, 0: disable pal_2ch_HT40_QAM
                                                                  //bit1: 1: enable pal_2ch_HT20_QAM, 0: disable pal_2ch_HT20_QAM
                                                                  //bit2: 1: enable pal_1ch_HT40_QAM, 0: disable pal_1ch_HT40_QAM
    A_UINT8   futureBase[27];     /* eeploc: 0x92 upper byte, 0x93->0x9F */
} __ATTRIB_PACK Ar9287_BASE_EEP_HEADER;  /* 64 bytes */

typedef struct Ar9287_ModalEepHeader {
    A_UINT32  antCtrlChain[AR9287_MAX_CHAINS];       // 8, eeploc: 0xB0, 0xB1
    A_UINT32  antCtrlCommon;                         // 4, eeploc: 0xB2, 0xB3 
    A_INT8    antennaGainCh[AR9287_MAX_CHAINS];      // 2, eeploc: 0xB4
    A_UINT8   switchSettling;                        // 1, eeploc: 0xB5 lower byte
    A_UINT8   txRxAttenCh[AR9287_MAX_CHAINS];        // 2, eeploc: 0xB5 upper byte, 0xB6 lower byte  //xatten1_hyst_margin for merlin (0x9848/0xa848 13:7)
    A_UINT8   rxTxMarginCh[AR9287_MAX_CHAINS];       // 2, eeploc: 0xB6 upper byte, 0xB7 lower byte  //xatten2_hyst_margin for merlin (0x9848/0xa848 20:14)
    A_INT8    adcDesiredSize;                        // 1, eeploc: 0xB7 upper byte
    A_UINT8   txEndToXpaOff;                         // 1, eeploc: 0xB8 lower byte
    A_UINT8   txEndToRxOn;                           // 1, eeploc: 0xB8 upper byte
    A_UINT8   txFrameToXpaOn;                        // 1, eeploc: 0xB9 lower byte
    A_UINT8   thresh62;                              // 1, eeploc: 0xB9 upper byte
    A_INT8    noiseFloorThreshCh[AR9287_MAX_CHAINS]; // 2, eeploc: 0xBA 
    A_UINT8   xpdGain;                               // 1, eeploc: 0xBB lower byte
    A_UINT8   xpd;                                   // 1, eeploc: 0xBB upper byte
    A_INT8    iqCalICh[AR9287_MAX_CHAINS];           // 2, eeploc: 0xBC 
    A_INT8    iqCalQCh[AR9287_MAX_CHAINS];           // 2, eeploc: 0xBD
    A_UINT8   pdGainOverlap;                         // 1, eeploc: 0xBE lower byte
    A_UINT8   xpaBiasLvl;                            // 1, eeploc: 0xBE upper byte
    A_UINT8   txFrameToDataStart;                    // 1, eeploc: 0xBF lower byte
    A_UINT8   txFrameToPaOn;                         // 1, eeploc: 0xBF upper byte
    A_UINT8   ht40PowerIncForPdadc;                  // 1, eeploc: 0xC0 lower byte
    A_UINT8   bswAtten[AR9287_MAX_CHAINS];           // 2, eeploc: 0xC0 upper byte, 0xC1 lower byte //xatten1_db for merlin (0xa20c/b20c 5:0)
    A_UINT8   bswMargin[AR9287_MAX_CHAINS];          // 2, eeploc: 0xC1 upper byte, 0xC2 lower byte //xatten1_margin for merlin (0xa20c/b20c 16:12
    A_UINT8   swSettleHt40;                          // 1, eeploc: 0xC2 upper byte
	A_UINT8   version;								 // 1, eeploc: 0xC3 lower byte 
    A_UINT8   db1;                                   // 1, eeploc: 0xC3 upper byte
    A_UINT8   db2;                                   // 1, eeploc: 0xC4 lower byte
    A_UINT8   ob_cck;                                // 1, eeploc: 0xC4 upper byte
    A_UINT8   ob_psk;                                // 1, eeploc: 0xC5 lower byte
    A_UINT8   ob_qam;                                // 1, eeploc: 0xC5 upper byte
    A_UINT8   ob_pal_off;                            // 1, eeploc: 0xC6 lower byte
    A_UINT8   futureModal[30];                       // 30, eeploc: 0xC6 upper byte, 0xC7 to 0xD5
    SPUR_CHAN spurChans[AR9287_EEPROM_MODAL_SPURS];  // 20B, eeploc: 0xD6 to 0xDF
} __ATTRIB_PACK Ar9287_MODAL_EEP_HEADER;             // == 99B

typedef struct Ar9287_calDataPerFreq {
    A_UINT8 pwrPdg[AR9287_NUM_PD_GAINS][AR9287_PD_GAIN_ICEPTS];
    A_UINT8 vpdPdg[AR9287_NUM_PD_GAINS][AR9287_PD_GAIN_ICEPTS];
} __ATTRIB_PACK Ar9287_CAL_DATA_PER_FREQ;

typedef struct Ar9287_CalCtlData {
    CAL_CTL_EDGES  ctlEdges[AR9287_MAX_CHAINS][AR9287_NUM_BAND_EDGES];
} __ATTRIB_PACK Ar9287_CAL_CTL_DATA;

typedef struct ar9287Eeprom {
    Ar9287_BASE_EEP_HEADER  baseEepHeader;             // 64 B
    A_UINT8                 custData[AR9287_DATA_SZ];  // 32 B
    Ar9287_MODAL_EEP_HEADER modalHeader[1];            // 99 B           
    A_UINT8                 calFreqPier2G[AR9287_NUM_2G_CAL_PIERS];  // 3 B
    CAL_DATA_PER_FREQ_U     calPierData2G[AR9287_MAX_CHAINS][AR9287_NUM_2G_CAL_PIERS]; // 6x40 = 240
    CAL_TARGET_POWER_LEG    calTargetPowerCck[AR9287_NUM_2G_CCK_TARGET_POWERS];   // 3x5 = 15 B
    CAL_TARGET_POWER_LEG    calTargetPower2G[AR9287_NUM_2G_20_TARGET_POWERS];     // 3x5 = 15 B
    CAL_TARGET_POWER_HT     calTargetPower2GHT20[AR9287_NUM_2G_20_TARGET_POWERS]; // 3x9 = 27 B
    CAL_TARGET_POWER_HT     calTargetPower2GHT40[AR9287_NUM_2G_40_TARGET_POWERS]; // 3x9 = 27 B
    A_UINT8                 ctlIndex[AR9287_NUM_CTLS]; // 12 B
    Ar9287_CAL_CTL_DATA     ctlData[AR9287_NUM_CTLS];  // 2x8x12 = 192 B
    A_UINT8                 padding; //1
} __ATTRIB_PACK AR9287_EEPROM; 

#if defined(WIN32) || defined(WIN64)
#pragma pack (pop)
#endif

/**** Macros ******************************************************************/
#define FREQ2FBIN(x,y) ((y) ? ((x) - 2300) : (((x) - 4800) / 5))

/**** Declarations and externals **********************************************/

/**** Public function prototypes **********************************************/

#endif  /* mEepStruct9287_H */
