/*
 * Copyright (c) 2002-2005 Atheros Communications, Inc.
 * All rights reserved.
 *
 */

#ident  "ACI $Id: //depot/sw/src/dk/mdk/devlib/ar6000/mEepStruct6000.h#4 $, $Header: //depot/sw/src/dk/mdk/devlib/ar6000/mEepStruct6000.h#4 $"

#ifndef _AR6000_EEPROM_STRUCT_H_
#define _AR6000_EEPROM_STRUCT_H_

#define AR6000_EEP_VER               0xF
#define AR6000_NUM_11A_CAL_PIERS     8
#define AR6000_NUM_11G_CAL_PIERS     4
#define AR6000_NUM_11A_TARGET_POWERS 4
#define AR6000_NUM_11B_TARGET_POWERS 2
#define AR6000_NUM_11G_TARGET_POWERS 3
#define AR6000_NUM_CTLS              8
#define AR6000_NUM_BAND_EDGES        8
#define AR6000_NUM_PD_GAINS          2 // Allow support for 1 or 2
#define AR6000_PD_GAINS_IN_MASK      4
#define AR6000_EEPROM_MODAL_SPURS    5
#define AR6000_MAX_RATE_POWER        63
#define AR6000_NUM_PDADC_VALUES      128
#define AR6000_NUM_RATES             16
#define AR6000_BCHAN_UNUSED          0xFF
#define AR6000_MAX_PWR_RANGE_IN_HALF_DB 64
#define AR6000_OPFLAGS_11A           1
#define AR6000_OPFLAGS_11G           2
#define FREQ2FBIN(x,y) ((y) ? ((x) - 2300) : (((x) - 4800) / 5))

typedef enum ConformanceTestLimits {
    FCC        = 0x10,
    MKK        = 0x40,
    ETSI       = 0x30,
    SD_NO_CTL  = 0xE0,
    NO_CTL     = 0xFF,
    CTL_MODE_M = 7,
    CTL_11A    = 0,
    CTL_11B    = 1,
    CTL_11G    = 2,
    CTL_TURBO  = 3,
    CTL_108G   = 4
} ATH_CTLS;

typedef struct BaseEepHeader {
    A_UINT16  checksum;
    A_UINT16  version;
    A_UINT16  regDmn;
    A_UINT8   macAddr[6];
    A_UINT16  opFlags;
    A_UINT8   custData[30];
} __ATTRIB_PACK BASE_EEP_HEADER;

typedef struct ModalEepHeader {
    A_UINT32  antCtrl[2];            // 64 bits
    A_UINT8   antCtrl0;              // 8 bits
    A_INT8    antennaGain;           // 8 bits
    A_UINT8   switchSettling;        // 7 bits
    A_UINT8   txRxAtten;             // 6 bits
    A_UINT8   rxTxMargin;            // 6 bits
    A_INT8    adcDesiredSize;        // 8 bits
    A_INT8    pgaDesiredSize;        // 8 bits
    A_UINT8   txEndToXlnaOn;         // 8 bits
    A_UINT8   xlnaGain;              // 8 bits
    A_UINT8   txEndToXpaOff;         // 8 bits
    A_UINT8   txFrameToXpaOn;        // 8 bits
    A_UINT8   thresh62;              // 8 bits
    A_INT8    noiseFloorThresh;      // 8 bits
    A_UINT8   xpdGain:4,             // 4 bits
              xpd    :4;             // 4 bits
    A_INT8    iqCalI;                // 6 bits
    A_INT8    iqCalQ;                // 5 bits
} __ATTRIB_PACK MODAL_EEP_HEADER;

typedef struct calDataPerFreq {
    A_UINT8 pwrPdg0[4];
    A_UINT8 vpdPdg0[4];
    A_UINT8 pwrPdg1[5];
    A_UINT8 vpdPdg1[5];
} __ATTRIB_PACK CAL_DATA_PER_FREQ;

typedef struct CalTargetPower {
    A_UINT32 bChannel    : 8,
             tPow6to24   : 6,
             tPow36      : 6,
             tPow48      : 6,
             tPow54      : 6;
} __ATTRIB_PACK CAL_TARGET_POWER;

typedef struct CalCtlEdges {
    A_UINT8  bChannel;
    A_UINT8  tPower :6,
             flag   :2;
} __ATTRIB_PACK CAL_CTL_EDGES;

typedef struct CalCtlData {
    CAL_CTL_EDGES  ctlEdges[AR6000_NUM_BAND_EDGES];
} __ATTRIB_PACK CAL_CTL_DATA;

typedef struct ar6kEeprom {
    BASE_EEP_HEADER    baseEepHeader;                            // 44 bytes
    MODAL_EEP_HEADER   modalHeader[2];                           // 48 bytes
    A_UINT16           spurChans[2][AR6000_EEPROM_MODAL_SPURS];         // 20 bytes
    A_UINT8            calFreqPier11A[AR6000_NUM_11A_CAL_PIERS];        // 8 bytes
    A_UINT8            calFreqPier11G[AR6000_NUM_11G_CAL_PIERS];        // 4 bytes
    CAL_DATA_PER_FREQ  calPierData11A[AR6000_NUM_11A_CAL_PIERS];        // 144 bytes
    CAL_DATA_PER_FREQ  calPierData11G[AR6000_NUM_11G_CAL_PIERS];        // 72 bytes
    CAL_TARGET_POWER   calTargetPower11A[AR6000_NUM_11A_TARGET_POWERS]; // 16 bytes
    CAL_TARGET_POWER   calTargetPower11B[AR6000_NUM_11B_TARGET_POWERS]; // 8 bytes
    CAL_TARGET_POWER   calTargetPower11G[AR6000_NUM_11G_TARGET_POWERS]; // 12 bytes
    A_UINT8            ctlIndex[AR6000_NUM_CTLS];                       // 8 bytes
    CAL_CTL_DATA       ctlData[AR6000_NUM_CTLS];                        // 128 bytes
} __ATTRIB_PACK AR6K_EEPROM;                               // Total 512 bytes

#endif //_AR6000_EEPROM_STRUCT_H_
