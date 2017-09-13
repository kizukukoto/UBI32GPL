/*
 * Copyright (c) 2002-2006 Atheros Communications, Inc.
 * All rights reserved.
 *
 */

#ident  "ACI $Id: //depot/sw/src/dk/mdk/devlib/ar9285/mEep9285.c#9 $, $Header: //depot/sw/src/dk/mdk/devlib/ar9285/mEep9285.c#9 $"

#ifdef VXWORKS
#include "vxworks.h"
#endif

#ifdef __ATH_DJGPPDOS__
#define __int64 long long
typedef unsigned long DWORD;
#define Sleep   delay
#endif  // #ifdef __ATH_DJGPPDOS__


#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef VXWORKS
#include <malloc.h>
#include <assert.h>
#endif
#include "wlantype.h"

#include "mCfg5416.h"
#include "mEep5416.h"
#include "mEep9285.h"

#include "ar5416reg.h"
#include "mConfig.h"
#include "athreg.h"

#include "mEepStruct5416.h"
#include "mEepStruct9285.h"
#ifndef __ATH_DJGPPDOS__
#include "endian_func.h"

#ifndef _IQV
extern  flash_read(A_UINT32 ,A_UINT32 ,A_UINT8 *,A_UINT32 , A_UINT32 );
#else	// _IQV
extern  void flash_read(A_UINT32 ,A_UINT32 ,A_UINT8 *,A_UINT32 , A_UINT32 );
#endif	// _IQV
#endif

//#define EEPROM_DUMP 1
static A_UINT16 EEPROM_DEBUG_LEVEL = 0;

static ar9285_EEPROM dummyEepromWriteArea;

static A_UINT16
fbin2freq(A_UINT8 fbin, A_BOOL is2GHz);

static A_INT16
interpolate(A_UINT16 target, A_UINT16 srcLeft, A_UINT16 srcRight,
            A_INT16 targetLeft, A_INT16 targetRight);

static A_BOOL
getLowerUpperIndex(A_UINT8 target, A_UINT8 *pList, A_UINT16 listSize,
                   A_UINT16 *indexL, A_UINT16 *indexR);

/* Configuring the Transmit Power Per Rate Baseband Table */
static void
ar9285SetPowerPerRateTable(ar9285_EEPROM *pEepData, EEPROM_CHANNEL *pEChan, A_INT16 *ratesArray,
                           A_UINT16 cfgCtl, A_UINT16 AntennaReduction, A_UINT16 powerLimit);

static A_UINT16
ar9285GetMaxEdgePower(A_UINT16 freq, CAL_CTL_EDGES *pRdEdgesPower, A_BOOL is2GHz);


static void
ar9285GetTargetPowers(EEPROM_CHANNEL *pEChan, CAL_TARGET_POWER_HT *powInfo,
                      A_UINT16 numChannels, CAL_TARGET_POWER_HT *pNewPower, A_UINT16 numRates, A_BOOL isHt40Target);

static void
ar9285GetTargetPowersLeg(EEPROM_CHANNEL *pEChan, CAL_TARGET_POWER_LEG *powInfo,
                      A_UINT16 numChannels, CAL_TARGET_POWER_LEG *pNewPower, A_UINT16 numRates, A_BOOL isExtTarget);

static void
ar9285SetPowerCalTable(A_UINT32 devNum, ar9285_EEPROM *pEepData, EEPROM_CHANNEL *pEChan, A_INT16 *pTxPowerIndexOffset);

void
ar9285GetPowerPerRateTable(A_UINT32 devNum, A_UINT16 freq, A_INT16 *pRatesPower);


#ifdef OWL_AP
static void ar9285EepromDump(A_UINT32 devNum, ar9285_EEPROM *ar9285Eep);
static A_UINT16 fbin2freq_gen8(A_UINT8 fbin, A_BOOL is2GHz);
#endif

static void
ar9285GetGainBoundariesAndPdadcs(A_UINT32 devNum, EEPROM_CHANNEL *pEChan, Ar9285_CAL_DATA_PER_FREQ *pRawDataSet,
                                 A_UINT8 * bChans,  A_UINT16 availPiers,
                                 A_UINT16 tPdGainOverlap, A_INT16 *pMinCalPower, A_UINT16 * pPdGainBoundaries,
                                 A_UINT8 * pPDADCValues, A_UINT16 numXpdGains);

static A_BOOL
ar9285FillVpdTable(A_UINT8 pMin, A_UINT8 pMax, A_UINT8 *pwrList,
                   A_UINT8 *vpdList, A_UINT16 numIntercepts, A_UINT8 *pRetVpdList);

static A_BOOL
ar9285VerifyPdadcTable(A_UINT32 devNum, Ar9285_CAL_DATA_PER_FREQ *pRawDataSet, A_UINT16 numXpdGains);

static A_UINT16 readEepromOldStyle (
 A_UINT32 devNum
);

static A_UINT16 normalizeEeprom( void );

#define POW_SM(_r, _s)     (((_r) & 0x3f) << (_s))
#define N(a)            (sizeof (a) / sizeof (a[0]))

#ifdef OWL_AP
A_UINT8 sysFlashConfigRead(A_UINT32,int fcl , int offset);
void sysFlashConfigWrite(A_UINT32,int fcl, int offset, A_UINT8 *data, int len);
#define FLC_RADIOCFG    2
static void readEepromByteBasedOldStyle (
 A_UINT32 devNum
);
#endif

/**************************************************************
 * ar9285EepromAttach
 *
 * Attach either the provided data stream or EEPROM to the EEPROM data structure
 */
A_BOOL
ar9285EepromAttach(A_UINT32 devNum)
{
    LIB_DEV_INFO *pLibDev = gLibInfo.pLibDevArray[devNum];
    A_UINT16 sum = 0;

#ifdef OWL_PB42
    A_UINT16 len = 0;
    A_UINT16 *pHalf;
    A_UINT32 i;
#endif
//A_UINT8 * temp= (A_UINT8 *)&dummyEepromWriteArea;
    pLibDev->ar9285Eep = &dummyEepromWriteArea;
#ifdef OWL_PB42
   if(isHowlAP(devNum) || isFlashCalData())
    {
     readEepromByteBasedOldStyle(devNum);
     len = pLibDev->ar9285Eep->baseEepHeader.length;
     pHalf = (A_UINT16 *)pLibDev->ar9285Eep;
     for (i = 0; i < (A_UINT32)(len / 2); i++) {
        sum ^= *pHalf++;
     }
    }
   else
    {
#endif

#if defined(OWL_AP) && (!defined(OWL_OB42))
    readEepromByteBasedOldStyle(devNum);

    len = pLibDev->ar9285Eep->baseEepHeader.length;

    pHalf = (A_UINT16 *)pLibDev->ar9285Eep;
    for (i = 0; i < (A_UINT32)(len / 2); i++) {
        sum ^= *pHalf++;
    }
#else
    sum = readEepromOldStyle(devNum);
#endif

#ifdef OWL_PB42
  }//endif isHowlAP
#endif
    /* Check CRC - Attach should fail on a bad checksum */
    if (sum != 0xffff) {
     //   printf("SNOOP: bad checksum!\n");
        mError(devNum, EIO, "ar9285EepromAttach: bad EEPROM checksum 0x%x\n", sum);
        return FALSE;
    }

#if 0
printf("Size of Ar9285_BASE_EEP_HEADER is 0x%x\n", sizeof(Ar9285_BASE_EEP_HEADER));
printf("Size of Ar9285_MODAL_EEP_HEADER is 0x%x\n", sizeof(Ar9285_MODAL_EEP_HEADER));
printf("Size of Ar9285_CAL_DATA_PER_FREQ is 0x%x\n", sizeof(Ar9285_CAL_DATA_PER_FREQ));
printf("Size of CAL_TARGET_POWER_LEG is 0x%x\n", sizeof(CAL_TARGET_POWER_LEG));
printf("Size of CAL_TARGET_POWER_HT is 0x%x\n", sizeof(CAL_TARGET_POWER_HT));
printf("Size of ar9285_EEPROM is 0x%x\n", sizeof(ar9285_EEPROM));
#endif

#ifdef OWL_AP
    //ar9285EepromDump(devNum, pLibDev->ar9285Eep);
#endif


    if ((pLibDev->ar9285Eep->baseEepHeader.version & AR9285_EEP_VER_MINOR_MASK) < AR9285_EEP_NO_BACK_VER) {
        mError(devNum, EIO, "ar9285EepromAttach: Incompatible EEPROM Version 0x%04X - recalibrate\n", pLibDev->ar9285Eep->baseEepHeader.version);
        return FALSE;
    }

    /* Setup some values checked by common code */
    pLibDev->eepData.version = pLibDev->ar9285Eep->baseEepHeader.version;
    pLibDev->eepData.protect = 0;
    pLibDev->eepData.turboDisabled = TRUE;
    pLibDev->eepData.currentRD = (A_UCHAR)pLibDev->ar9285Eep->baseEepHeader.regDmn[0];
    /* Do we need to fill in the tpcMap? */

    return TRUE;
}


/* assume that the EEPROM is in little-endian format and normalize to
 * our host architectire */
static A_UINT16 normalizeEeprom( void )
{
    ar9285_EEPROM *eeprom = &dummyEepromWriteArea;
    Ar9285_BASE_EEP_HEADER  *base_hdr = &eeprom->baseEepHeader;
    Ar9285_MODAL_EEP_HEADER *modal_hdr;
    A_UINT16 *eep_data_raw;
    A_UINT32 num_words, offset;
    A_UINT32 i, j;
    A_UINT16 csum = 0;

    /* convert the whole thing back to its native format. */
    num_words = sizeof(ar9285_EEPROM)/sizeof(A_UINT16);
    eep_data_raw = (A_UINT16 *)&dummyEepromWriteArea;
    for (offset = 0; offset < num_words; offset++, eep_data_raw++) {
        *eep_data_raw  = le2cpu16(*eep_data_raw);
    }

    /* compute crc while in eeprom native format */
    eep_data_raw = (A_UINT16 *)&dummyEepromWriteArea;
//    printf("\n LENGTH: 0x%04x\n", base_hdr->length);
    num_words = le2cpu16(base_hdr->length)/2;

    for (i = 0; i < num_words; i++) {
    	// printf("i=%d  0x%04x\n", i, *eep_data_raw);
        csum ^= *eep_data_raw++;
    }

    /* now do structure specific swaps */

    /* convert Base Eep header */
    base_hdr->length= le2cpu16(base_hdr->length);
    base_hdr->checksum = le2cpu16(base_hdr->checksum);
    base_hdr->version = le2cpu16(base_hdr->version);
    base_hdr->regDmn[0] = le2cpu16(base_hdr->regDmn[0]);
    base_hdr->regDmn[1] = le2cpu16(base_hdr->regDmn[1]);
    base_hdr->rfSilent = le2cpu16(base_hdr->rfSilent);
    base_hdr->blueToothOptions = le2cpu16(base_hdr->blueToothOptions);
    base_hdr->deviceCap = le2cpu16(base_hdr->deviceCap);
    base_hdr->binBuildNumber = le2cpu32(base_hdr->binBuildNumber);

    /* convert Modal Eep header */
    modal_hdr = &eeprom->modalHeader[0];

    modal_hdr->antCtrlCommon = le2cpu32(modal_hdr->antCtrlCommon);

    for (j=0; j<AR9285_MAX_CHAINS; j++) {
        modal_hdr->antCtrlChain[j] =
        le2cpu32(modal_hdr->antCtrlChain[j]);
    }

    for (j=0; j<AR9285_EEPROM_MODAL_SPURS; j++) {
        modal_hdr->spurChans[j].spurChan =
        le2cpu16(modal_hdr->spurChans[j].spurChan);
    }

    return csum;
}

static A_UINT32 newEepromArea[sizeof(ar9285_EEPROM)];
A_UINT16 readEepromOldStyle (A_UINT32 devNum)
{
    A_UINT32 jj;
    A_UINT16 *tempPtr = (A_UINT16 *)&dummyEepromWriteArea;
    A_UINT8  *tempTest8;
    A_UINT32 csum;
    eepromReadBlock(devNum, AR9285_EEP_START_LOC, sizeof(ar9285_EEPROM)/2, newEepromArea);

//    printf("TEST:: ar9285_EEPROM loc %x size %d\n", AR9285_EEP_START_LOC, sizeof(ar9285_EEPROM));
    for(jj=0; jj < sizeof(ar9285_EEPROM)/2; jj ++) {
        *(tempPtr+jj) = (A_UINT16)*(newEepromArea + jj);
        //printf("location[%x] = %x\n", jj, *(tempPtr+jj));
    }


    tempTest8 = (A_UINT8 *)tempPtr;
/*    for(jj=0; jj < 200; jj ++) {
       if((jj % 4) == 0) {
           printf("%x, %x, %x, %x \n ", jj, *(tempTest8 + jj),
                  *((A_UINT16 *)(tempTest8) + jj),
                  *((A_UINT32 *)(tempTest8) + jj));
       }
       else if((jj % 2) == 0) {
           printf("%x, %x, %x \n ", jj, *(tempTest8 + jj),
                  *((A_UINT16 *)(tempTest8) + jj));
       }
       else
           printf("%x, %x\n ", jj, *(tempTest8 + jj));
    }*/

    csum = normalizeEeprom();

    return ((A_UINT16)csum);
}
#if defined(OWL_AP)
void readEepromByteBasedOldStyle (
 A_UINT32 devNum
)
{
    A_UINT32 jj;
    LIB_DEV_INFO  *pLibDev = gLibInfo.pLibDevArray[devNum];
    A_UINT8 *tempPtr = (A_UINT8 *)&dummyEepromWriteArea;
#ifndef VXWORKS
	flash_read(devNum,FLC_RADIOCFG,tempPtr,sizeof(ar9285_EEPROM),(AR9285_EEP_START_LOC)<<1);
#else
        if(isKiteSW(pLibDev->swDevID)){
             for(jj=0; jj < sizeof(ar9285_EEPROM); jj ++) {
                 *(tempPtr + jj) = sysFlashConfigRead(devNum, FLC_RADIOCFG,(AR9285_EEP_START_LOC << 1) + jj); 
             }
        }
#endif
}
#endif

#if defined(OWL_AP) && (!defined(OWL_OB42))
void readEepromByteBasedOldStyle (
 A_UINT32 devNum
)
{
    A_UINT32 jj;
    A_UINT8 *tempPtr = (A_UINT8 *)&dummyEepromWriteArea;

    for(jj=0; jj < sizeof(ar9285_EEPROM); jj ++) {
        *(tempPtr + jj) = sysFlashConfigRead(devNum,FLC_RADIOCFG,
                                             (AR9285_EEP_START_LOC << 1) + jj);
//              printf("location[%x] = %x\n", jj, *(tempPtr+jj));
    }
#ifndef ARCH_BIG_ENDIAN
	#ifdef RSP_BLOCKSWAP
	// RSP-ENDIAN - only for testing, disabled by default
	    for(jj=0; jj < sizeof(ar9285_EEPROM); jj+=2) {
		swp=*(tempPtr + jj);
		*(tempPtr + jj+1)=*(tempPtr + jj);
		*(tempPtr + jj+1)=swp;
	    }
	#endif
#endif
#if 0
    for(jj=0; jj < 100; jj ++) {
       if((jj % 4) == 0) {
           printf("%x, %x, %x, %x \n ", jj, *(tempPtr + jj),
                  *((A_UINT16 *)(tempPtr) + jj),
                  *((A_UINT32 *)(tempPtr) + jj));
       }
       else if((jj % 2) == 0) {
           printf("%x, %x, %x \n ", jj, *(tempPtr + jj),
                  *((A_UINT16 *)(tempPtr) + jj));
       }
       else
           printf("%x, %x\n ", jj, *(tempPtr + jj));
    }
#endif
}
#endif

#if 0 // SW routines
/**************************************************************
 * ar9285SetSpurMitigation
 *
 * Apply Spur Immunity to Boards that require it.
 * Applies only to OFDM RX operation.
 */
void
ar9285EepromSetSpurMitigation(struct ath_hal *ah, HAL_CHANNEL *chan)
{
#define CHAN_TO_SPUR(is2GHz, chan)   ( ((chan) - ((is2GHz) ? 2300 : 4900)) * 10 )
#define SPUR_TO_KHZ(is2GHz, spur)    ( ((spur) + ((is2GHz) ? 23000 : 49000)) * 100 )
#define SPUR_VAL_MASK         0x3FFF
#define SPUR_CHAN_WIDTH       87
#define BIN_WIDTH_BASE_100HZ  3125
#define MAX_BINS_ALLOWED      28
#define NO_SPUR               0x8000
    A_UINT32      pilotMask[2] = {0, 0}, binMagMask[4] = {0, 0, 0 , 0};
    A_UINT16      i;
    A_UINT16      finalSpur, curChanAsSpur, binWidth = 0, spurDetectWidth, spurChan;
    A_INT32       spurDeltaPhase = 0, spurFreqSd = 0, spurOffset, binOffsetNumT16, curBinOffset;
    A_INT16       numBinOffsets;
    A_UINT16      magMapFor4[4] = {1, 2, 2, 1};
    A_UINT16      magMapFor3[3] = {1, 2, 1};
    A_UINT16      *pMagMap;
    A_BOOL        is2GHz = IS_CHAN_2GHZ(chan);

    /* No spur handling */
    A_ASSERT(!IS_CHAN_B(chan));

    curChanAsSpur = CHAN_TO_SPUR(is2GHz, chan->channel);

    /*
     * Check if spur immunity should be performed for this channel
     * Should only be performed once per channel and then saved
     */
    finalSpur = NO_SPUR;
    spurDetectWidth = SPUR_CHAN_WIDTH;

    /* Decide if any spur affects the current channel */
    for (i = 0; i < EEPROM_MODAL_SPURS; i++) {
		spurChan = ah->ah_pEeprom->spurChans[i][0];
        if (spurChan == NO_SPUR) {
            break;
        }
        if ((curChanAsSpur - spurDetectWidth <= (spurChan & SPUR_VAL_MASK)) &&
            (curChanAsSpur + spurDetectWidth >= (spurChan & SPUR_VAL_MASK)))
        {
            finalSpur = spurChan & SPUR_VAL_MASK;
            break;
        }
    }

    /* Write spur immunity data */
    if (finalSpur == NO_SPUR) {
        /* Disable Spur Immunity Regs if they appear set */
        if (OS_REG_READ(AR_PHY_TIMING_CTRL4) & AR_PHY_TIMING_CTRL4_ENABLE_SPUR_FILTER) {
            /* XXX - Use the new Dragon spur-mitigation off reg bit */
            /* Clear Spur Delta Phase, Spur Freq, and enable bits */
            OS_REG_RMW_FIELD(AR_PHY_MASK_CTL, AR_PHY_MASK_CTL_RATE, 0);
            OS_REG_CLR_BIT(AR_PHY_TIMING_CTRL4, AR_PHY_TIMING_CTRL4_ENABLE_SPUR_FILTER |
                                                AR_PHY_TIMING_CTRL4_ENABLE_CHAN_MASK   |
                                                AR_PHY_TIMING_CTRL4_ENABLE_PILOT_MASK);
            OS_REG_WRITE(AR_PHY_TIMING11, 0);

            /* Clear pilot masks */
            OS_REG_WRITE(AR_PHY_TIMING7, 0);
            OS_REG_RMW_FIELD(AR_PHY_TIMING8, AR_PHY_TIMING8_PILOT_MASK_2, 0);
            OS_REG_WRITE(AR_PHY_TIMING9, 0);
            OS_REG_RMW_FIELD(AR_PHY_TIMING10, AR_PHY_TIMING10_PILOT_MASK_2, 0);

            /* Clear magnitude masks */
            OS_REG_WRITE(AR_PHY_BIN_MASK_1, 0);
            OS_REG_WRITE(AR_PHY_BIN_MASK_2, 0);
            OS_REG_WRITE(AR_PHY_BIN_MASK_3, 0);
            OS_REG_RMW_FIELD(AR_PHY_MASK_CTL, AR_PHY_MASK_CTL_MASK_4, 0);
            OS_REG_WRITE(AR_PHY_BIN_MASK2_1, 0);
            OS_REG_WRITE(AR_PHY_BIN_MASK2_2, 0);
            OS_REG_WRITE(AR_PHY_BIN_MASK2_3, 0);
            OS_REG_RMW_FIELD(AR_PHY_BIN_MASK2_4, AR_PHY_BIN_MASK2_4_MASK_4, 0);
        }
    } else {
        spurOffset = finalSpur - curChanAsSpur;
        /*
         * Spur calculations:
         * spurDeltaPhase is (spurOffsetIn100KHz / chipFrequencyIn100KHz) << 21
         * spurFreqSd is (spurOffsetIn100KHz / sampleFrequencyIn100KHz) << 11
         */
        if (IS_CHAN_A(chan)) {
            /* Chip Frequency & sampleFrequency are 40 MHz */
            spurDeltaPhase = (spurOffset << 17) / 25;
            spurFreqSd = spurDeltaPhase >> 10;
            binWidth = BIN_WIDTH_BASE_100HZ;
        } else {
            /* Chip Frequency is 44MHz, sampleFrequency is 40 MHz */
            spurFreqSd = (spurOffset << 8) / 55;
            spurDeltaPhase = (spurOffset << 17) / 25;
            binWidth = BIN_WIDTH_BASE_100HZ;
        }

        /* Compute Pilot Mask */
        binOffsetNumT16 = ((spurOffset * 1000) << 4) / binWidth;
        /* The spur is on a bin if it's remainder at times 16 is 0 */
        if (binOffsetNumT16 & 0xF) {
            numBinOffsets = 4;
            pMagMap = magMapFor4;
        } else {
            numBinOffsets = 3;
            pMagMap = magMapFor3;
        }
        for (i = 0; i < numBinOffsets; i++) {
            A_ASSERT(A_ABS((binOffsetNumT16 >> 4) <= MAX_BINS_ALLOWED));

            /* Get Pilot Mask values */
            curBinOffset = (binOffsetNumT16 >> 4) + i + 25;
            if ((curBinOffset >= 0) && (curBinOffset <= 32)) {
                if (curBinOffset <= 25) {
                    pilotMask[0] |= 1 << curBinOffset;
                } else if (curBinOffset >= 27){
                    pilotMask[0] |= 1 << (curBinOffset - 1);
                }
            }
            else if ((curBinOffset >= 33) && (curBinOffset <= 52)) {
                pilotMask[1] |= 1 << (curBinOffset - 33);
            }

            /* Get viterbi values */
            if ((curBinOffset >= -1) && (curBinOffset <= 14)) {
                binMagMask[0] |= pMagMap[i] << (curBinOffset + 1) * 2;
            } else if ((curBinOffset >= 15) && (curBinOffset <= 30)) {
                binMagMask[1] |= pMagMap[i] << (curBinOffset - 15) * 2;
            } else if ((curBinOffset >= 31) && (curBinOffset <= 46)) {
                binMagMask[2] |= pMagMap[i] << (curBinOffset -31) * 2;
            } else if((curBinOffset >= 47) && (curBinOffset <= 53)) {
                binMagMask[3] |= pMagMap[i] << (curBinOffset -47) * 2;
            }
        }

#if 0
        HALDEBUG(HAL_DEBUG_EEPROM, "||=== Spur Mitigation Debug =====\n");
        HALDEBUG(HAL_DEBUG_EEPROM, "|| For channel %d MHz a spur was found at freq %d KHz\n", chan->channel, SPUR_TO_KHZ(is2GHz, finalSpur));
        HALDEBUG(HAL_DEBUG_EEPROM, "|| Offset found to be %d (100 KHz)\n", spurOffset);
        HALDEBUG(HAL_DEBUG_EEPROM, "|| spurDeltaPhase %d, spurFreqSd %d\n", spurDeltaPhase, spurFreqSd);
        HALDEBUG(HAL_DEBUG_EEPROM, "|| Pilot Mask 0 0X%08X 1 0X%08X\n", pilotMask[0], pilotMask[1]);
        HALDEBUG(HAL_DEBUG_EEPROM, "|| Viterbi Mask 0 0X%08X 1 0X%08X 2 0X%08X 3 0X%08X\n", binMagMask[0], binMagMask[1], binMagMask[2], binMagMask[3]);
        HALDEBUG(HAL_DEBUG_EEPROM, "||\n");
        HALDEBUG(HAL_DEBUG_EEPROM, "||===");

        for (i = 0; i <= 25; i++) {
            if ((pilotMask[0] >> i) & 1) {
                HALDEBUG(HAL_DEBUG_EEPROM, " X ");
            } else {
                HALDEBUG(HAL_DEBUG_EEPROM, "   ");
            }
        }
        HALDEBUG(HAL_DEBUG_EEPROM, "===||\n");

        HALDEBUG(HAL_DEBUG_EEPROM, "||");
        for (i = 0; i <= 31; i+=2) {
            HALDEBUG(HAL_DEBUG_EEPROM, " %d ", (binMagMask[0] >> i) & 0x3);
        }
        for (i = 0; i <= 23; i+=2) {
            HALDEBUG(HAL_DEBUG_EEPROM, " %d ", (binMagMask[1] >> i) & 0x3);
        }
        HALDEBUG(HAL_DEBUG_EEPROM, "||\n");

        HALDEBUG(HAL_DEBUG_EEPROM, "||-27-26-25-24-23-22-21-20-19-18-17-16-15-14-13-12-11-10-9 -8 -7 -6 -5 -4 -3 -2 -1  0 ||\n");
        HALDEBUG(HAL_DEBUG_EEPROM, "||------------------------------------------------------------------------------------||\n");

        HALDEBUG(HAL_DEBUG_EEPROM, "||===");
        for (i = 26; i <= 31; i++) {
            if ((pilotMask[0] >> i) & 1) {
                HALDEBUG(HAL_DEBUG_EEPROM, " X ");
            } else {
                HALDEBUG(HAL_DEBUG_EEPROM, "   ");
            }
        }
        for (i = 0; i <= 19; i++) {
            if ((pilotMask[1] >> i) & 1) {
                HALDEBUG(HAL_DEBUG_EEPROM, " X ");
            } else {
                HALDEBUG(HAL_DEBUG_EEPROM, "   ");
            }
        }
        HALDEBUG(HAL_DEBUG_EEPROM, "===||\n");

        HALDEBUG(HAL_DEBUG_EEPROM, "||");
        for (i = 22; i <= 31; i+=2) {
            HALDEBUG(HAL_DEBUG_EEPROM, " %d ", (binMagMask[1] >> i) & 0x3);
        }
        for (i = 0; i <= 31; i+=2) {
            HALDEBUG(HAL_DEBUG_EEPROM, " %d ", (binMagMask[2] >> i) & 0x3);
        }
        for (i = 0; i <= 13; i+=2) {
            HALDEBUG(HAL_DEBUG_EEPROM, " %d ", (binMagMask[3] >> i) & 0x3);
        }
        HALDEBUG(HAL_DEBUG_EEPROM, "||\n");

        HALDEBUG(HAL_DEBUG_EEPROM, "|| 0  1  2  3  4  5  6  7  8  9  10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27||\n");
        HALDEBUG(HAL_DEBUG_EEPROM, "||------------------------------------------------------------------------------------||\n");
#endif

        /* Write Spur Delta Phase, Spur Freq, and enable bits */
        OS_REG_RMW_FIELD(AR_PHY_MASK_CTL, AR_PHY_MASK_CTL_RATE, 0xFF);
        OS_REG_SET_BIT(AR_PHY_TIMING_CTRL4, AR_PHY_TIMING_CTRL4_ENABLE_SPUR_FILTER |
                                            AR_PHY_TIMING_CTRL4_ENABLE_CHAN_MASK   |
                                            AR_PHY_TIMING_CTRL4_ENABLE_PILOT_MASK);
        OS_REG_WRITE(AR_PHY_TIMING11, AR_PHY_TIMING11_USE_SPUR_IN_AGC
                     | SM(spurFreqSd, AR_PHY_TIMING11_SPUR_FREQ_SD)
                     | SM(spurDeltaPhase, AR_PHY_TIMING11_SPUR_DELTA_PHASE));

        /* Write pilot masks */
        OS_REG_WRITE(AR_PHY_TIMING7, pilotMask[0]);
        OS_REG_RMW_FIELD(AR_PHY_TIMING8, AR_PHY_TIMING8_PILOT_MASK_2, pilotMask[1]);
        OS_REG_WRITE(AR_PHY_TIMING9, pilotMask[0]);
        OS_REG_RMW_FIELD(AR_PHY_TIMING10, AR_PHY_TIMING10_PILOT_MASK_2, pilotMask[1]);

        /* Write magnitude masks */
        OS_REG_WRITE(AR_PHY_BIN_MASK_1, binMagMask[0]);
        OS_REG_WRITE(AR_PHY_BIN_MASK_2, binMagMask[1]);
        OS_REG_WRITE(AR_PHY_BIN_MASK_3, binMagMask[2]);
        OS_REG_RMW_FIELD(AR_PHY_MASK_CTL, AR_PHY_MASK_CTL_MASK_4, binMagMask[3]);
        OS_REG_WRITE(AR_PHY_BIN_MASK2_1, binMagMask[0]);
        OS_REG_WRITE(AR_PHY_BIN_MASK2_2, binMagMask[1]);
        OS_REG_WRITE(AR_PHY_BIN_MASK2_3, binMagMask[2]);
        OS_REG_RMW_FIELD(AR_PHY_BIN_MASK2_4, AR_PHY_BIN_MASK2_4_MASK_4, binMagMask[3]);
    }
}
#endif // if 0 SW API's

#ifdef EEPROM_DUMP
void
ar9285PrintPowerPerRate(A_INT16 *pRatesPower)
{
    const char *rateString[] = {" 6mb OFDM", " 9mb OFDM", "12mb OFDM", "18mb OFDM",
                                   "24mb OFDM", "36mb OFDM", "48mb OFDM", "54mb OFDM",
                                   "1L   CCK ", "2L   CCK ", "2S   CCK ", "5.5L CCK ",
                                   "5.5S CCK ", "11L  CCK ", "11S  CCK ", "XR       ",
                                   "HT20mcs 0", "HT20mcs 1", "HT20mcs 2", "HT20mcs 3",
                                   "HT20mcs 4", "HT20mcs 5", "HT20mcs 6", "HT20mcs 7",
                                   "HT40mcs 0", "HT40mcs 1", "HT40mcs 2", "HT40mcs 3",
                                   "HT40mcs 4", "HT40mcs 5", "HT40mcs 6", "HT40mcs 7",
                                   "Dup CCK  ", "Dup OFDM ", "Ext CCK  ", "Ext OFDM ",
    };
    int i;

    for (i = 0; i < Ar5416RateSize; i+=4) {
        printf(" %s %3d.%1d dBm | %s %3d.%1d dBm | %s %3d.%1d dBm | %s %3d.%1d dBm\n",
            rateString[i], pRatesPower[i] / 2, (pRatesPower[i] % 2) * 5,
            rateString[i + 1], pRatesPower[i + 1] / 2, (pRatesPower[i + 1] % 2) * 5,
            rateString[i + 2], pRatesPower[i + 2] / 2, (pRatesPower[i + 2] % 2) * 5,
            rateString[i + 3], pRatesPower[i + 3] / 2, (pRatesPower[i + 3] % 2) * 5);
    }
}
#endif

/**************************************************************
 * ar9285EepromSetBoardValues
 *
 * Read EEPROM header info and program the device for correct operation
 * given the channel value.
 */
void
ar9285EepromSetBoardValues(A_UINT32 devNum, A_UINT32 freq)
{
    LIB_DEV_INFO  *pLibDev = gLibInfo.pLibDevArray[devNum];
    int is2GHz = ((pLibDev->mode == MODE_11G) || (pLibDev->mode == MODE_11B));
    Ar9285_MODAL_EEP_HEADER* pModal;
    Ar9285_BASE_EEP_HEADER*  pBase;
    A_UINT16 antWrites[AR9285_ANT_16S];
    A_CHAR *bank6names[] = {"rf_ob", "rf_b_ob", "rf_db", "rf_b_db"};
    A_UINT32 numBk6Params = sizeof(bank6names)/sizeof(bank6names[0]), b;
    PARSE_FIELD_INFO bank6Fields[sizeof(bank6names)/sizeof(bank6names[0])];

    int i, j, offset_num;

    freq = 0; // quiet compiler warning
    assert(((pLibDev->eepData.version >> 12) & 0xF) == AR9285_EEP_VER);
    pModal = &(pLibDev->ar9285Eep->modalHeader[0]);
    pBase  = &(pLibDev->ar9285Eep->baseEepHeader);

    /* Set the antenna register(s) correctly for the chip revision */
	antWrites[0] = (A_UINT16)((pModal->antCtrlCommon >> 28) & 0xF);
	antWrites[1] = (A_UINT16)((pModal->antCtrlCommon >> 24) & 0xF);
	antWrites[2] = (A_UINT16)((pModal->antCtrlCommon >> 20) & 0xF);
	antWrites[3] = (A_UINT16)((pModal->antCtrlCommon >> 16) & 0xF);
	antWrites[4] = (A_UINT16)((pModal->antCtrlCommon >> 12) & 0xF);
	antWrites[5] = (A_UINT16)((pModal->antCtrlCommon >> 8) & 0xF);
	antWrites[6] = (A_UINT16)((pModal->antCtrlCommon >> 4)  & 0xF);
	antWrites[7] = (A_UINT16)(pModal->antCtrlCommon & 0xF);

	offset_num = 8;

    for (i = 0, j = offset_num; i < AR9285_MAX_CHAINS; i++) {
		antWrites[j++] = (A_UINT16)((pModal->antCtrlChain[i] >> 28) & 0xf);
        antWrites[j++] = (A_UINT16)((pModal->antCtrlChain[i] >> 10) & 0x3);
        antWrites[j++] = (A_UINT16)((pModal->antCtrlChain[i] >> 8) & 0x3);
        antWrites[j++] = 0; //dummy placeholder this is missing for merlin
        antWrites[j++] = (A_UINT16)((pModal->antCtrlChain[i] >> 6) & 0x3);
        antWrites[j++] = (A_UINT16)((pModal->antCtrlChain[i] >> 4) & 0x3);
        antWrites[j++] = (A_UINT16)((pModal->antCtrlChain[i] >> 2) & 0x3);
        antWrites[j++] = (A_UINT16)(pModal->antCtrlChain[i] & 0x3);
    }
        assert (j <= AR9285_ANT_16S);


    forceAntennaTbl5416(devNum, antWrites);

    writeField(devNum, "bb_switch_settling", pModal->switchSettling);

    writeField(devNum, "bb_adc_desired_size", pModal->adcDesiredSize);
    if(!isMerlin(pLibDev->swDevID)) {
        writeField(devNum, "bb_pga_desired_size", pModal->pgaDesiredSize);
    }

    writeField(devNum, "bb_tx_end_to_xpaa_off", pModal->txEndToXpaOff);
    writeField(devNum, "bb_tx_end_to_xpab_off", pModal->txEndToXpaOff);
    writeField(devNum, "bb_tx_frame_to_xpaa_on", pModal->txFrameToXpaOn);
    writeField(devNum, "bb_tx_frame_to_xpab_on", pModal->txFrameToXpaOn);

    writeField(devNum, "bb_tx_end_to_a2_rx_on", pModal->txEndToRxOn);

    writeField(devNum, "bb_thresh62", pModal->thresh62);
    REGW(devNum, PHY_CCA, (REGR(devNum, PHY_CCA_CH0) & ~PHY_CCA_THRESH62_M) | ((pModal->thresh62  << PHY_CCA_THRESH62_S) & PHY_CCA_THRESH62_M));

    writeField(devNum, "bb_thresh62_ext", pModal->thresh62);

    if(!isMerlin(pLibDev->swDevID)) {
        writeField(devNum, "bb_ch0_txrxatten", pModal->txRxAttenCh[0]);
        writeField(devNum, "bb_ch0_rxtx_margin_2ghz", pModal->rxTxMarginCh[0]);
    } else {
        writeField(devNum, "bb_ch0_xatten1_hyst_margin", pModal->txRxAttenCh[0]);
        writeField(devNum, "bb_ch0_xatten2_hyst_margin", pModal->rxTxMarginCh[0]);
        /* Fix bug EV55076 : wrong noise floor when mc_tx_def_ant_sel=1 and bb_enable_ant_div_lnadiv=0 */
        writeField(devNum, "bb_ch1_xatten1_hyst_margin", pModal->txRxAttenCh[0]);
        writeField(devNum, "bb_ch1_xatten2_hyst_margin", pModal->rxTxMarginCh[0]);
    }


    writeField(devNum, "bb_ch0_iqcorr_q_i_coff", pModal->iqCalICh[0]);
    writeField(devNum, "bb_ch0_iqcorr_q_q_coff", pModal->iqCalQCh[0]);
    writeField(devNum, "bb_ch0_iqcorr_enable", 1);

    /* Write and update OB/DB */
    for (b = 0; b < numBk6Params; b++) {
        memcpy(bank6Fields[b].fieldName, bank6names[b], strlen(bank6names[b]) + 1);
        bank6Fields[b].valueString[1] = '\0';
    }

    /* Minor Version Specific application */
    if ((pLibDev->ar9285Eep->baseEepHeader.version & AR9285_EEP_VER_MINOR_MASK) >= AR9285_EEP_MINOR_VER_2) {
		if(pModal->version >= 2)
		{
			writeField(devNum, "an_rf2g3_ob_0",  pModal->ob & 7);
			writeField(devNum, "an_rf2g3_ob_1",  (pModal->ob >> 4) & 7);
			writeField(devNum, "an_rf2g3_ob_2",  (pModal->ob_23) & 7);
			writeField(devNum, "an_rf2g3_ob_3",  (pModal->ob_23 >> 4) & 7);
			writeField(devNum, "an_rf2g3_ob_4",  (pModal->ob_4) & 7);
			writeField(devNum, "an_rf2g3_db1_0", pModal->db & 7);
			writeField(devNum, "an_rf2g3_db1_1", (pModal->db >> 4) & 7);
			writeField(devNum, "an_rf2g3_db1_2", (pModal->db_23) & 7);
			writeField(devNum, "an_rf2g4_db1_3", (pModal->db_23 >> 4) & 7);
			writeField(devNum, "an_rf2g4_db1_4", (pModal->db_4) & 7);
			writeField(devNum, "an_rf2g4_db2_0", pModal->db2 & 7);
			writeField(devNum, "an_rf2g4_db2_1", (pModal->db2 >> 4) & 7);
			writeField(devNum, "an_rf2g4_db2_2", (pModal->db2_23) & 7);
			writeField(devNum, "an_rf2g4_db2_3", (pModal->db2_23 >> 4) & 7);
			writeField(devNum, "an_rf2g4_db2_4", (pModal->db2_4) & 7);

		}
		else if(pModal->version == 1)
		{
			writeField(devNum, "an_rf2g3_ob_0",  pModal->ob & 7);
			writeField(devNum, "an_rf2g3_ob_1",  (pModal->ob >> 4) & 7);
			writeField(devNum, "an_rf2g3_ob_2",  (pModal->ob >> 4) & 7);
			writeField(devNum, "an_rf2g3_ob_3",  (pModal->ob >> 4) & 7);
			writeField(devNum, "an_rf2g3_ob_4",  (pModal->ob >> 4) & 7);
			writeField(devNum, "an_rf2g3_db1_0", pModal->db & 7);
			writeField(devNum, "an_rf2g3_db1_1", (pModal->db >> 4) & 7);
			writeField(devNum, "an_rf2g3_db1_2", (pModal->db >> 4) & 7);
			writeField(devNum, "an_rf2g4_db1_3", (pModal->db >> 4) & 7);
			writeField(devNum, "an_rf2g4_db1_4", (pModal->db >> 4) & 7);
			writeField(devNum, "an_rf2g4_db2_0", pModal->db2 & 7);
			writeField(devNum, "an_rf2g4_db2_1", (pModal->db2 >> 4) & 7);
			writeField(devNum, "an_rf2g4_db2_2", (pModal->db2 >> 4) & 7);
			writeField(devNum, "an_rf2g4_db2_3", (pModal->db2 >> 4) & 7);
			writeField(devNum, "an_rf2g4_db2_4", (pModal->db2 >> 4) & 7);
		}
		else
		{
			writeField(devNum, "an_rf2g3_ob_0",  pModal->ob);
			writeField(devNum, "an_rf2g3_ob_1",  pModal->ob);
			writeField(devNum, "an_rf2g3_ob_2",  pModal->ob);
			writeField(devNum, "an_rf2g3_ob_3",  pModal->ob);
			writeField(devNum, "an_rf2g3_ob_4",  pModal->ob);
			writeField(devNum, "an_rf2g3_db1_0", pModal->db);
			writeField(devNum, "an_rf2g3_db1_1", pModal->db);
			writeField(devNum, "an_rf2g3_db1_2", pModal->db);
			writeField(devNum, "an_rf2g4_db1_3", pModal->db);
			writeField(devNum, "an_rf2g4_db1_4", pModal->db);
			writeField(devNum, "an_rf2g4_db2_0", pModal->db);
			writeField(devNum, "an_rf2g4_db2_1", pModal->db);
			writeField(devNum, "an_rf2g4_db2_2", pModal->db);
			writeField(devNum, "an_rf2g4_db2_3", pModal->db);
			writeField(devNum, "an_rf2g4_db2_4", pModal->db);
		}
    }
	if(pModal->version >= 3)
	{
		writeField(devNum, "bb_enable_ant_div_lnadiv", (pModal->ob_4>>4) & 1);
		writeField(devNum, "bb_ant_div_alt_gaintb", (pModal->ob_4>>5) & 1);
		writeField(devNum, "bb_ant_div_main_gaintb", (pModal->ob_4>>6) & 1);
		writeField(devNum, "bb_enable_ant_fast_div", (pModal->ob_4>>7) & 1);

		writeField(devNum, "bb_ant_div_alt_lnaconf", (pModal->db_4>>4) & 3);
		writeField(devNum, "bb_ant_div_main_lnaconf", (pModal->db_4>>6) & 3);
	}

        writeField(devNum, "bb_tx_frame_to_tx_d_start", pModal->txFrameToDataStart);
        writeField(devNum, "bb_tx_frame_to_pa_on", pModal->txFrameToPaOn);


    if ((pLibDev->ar9285Eep->baseEepHeader.version & AR9285_EEP_VER_MINOR_MASK) >= AR9285_EEP_MINOR_VER_3) {
        if (pLibDev->libCfgParams.ht40_enable) {
            writeField(devNum, "bb_switch_settling", pModal->swSettleHt40);
        }
            writeField(devNum, "bb_ch0_xatten1_margin", pModal->bswMargin[0]);
            writeField(devNum, "bb_ch0_xatten1_db", pModal->bswAtten[0]);
            writeField(devNum, "bb_ch0_xatten2_margin", pModal->xatten2Margin[0]);
            writeField(devNum, "bb_ch0_xatten2_db", pModal->xatten2Db[0]);
            /* Fix bug EV55076 : wrong noise floor when mc_tx_def_ant_sel=1 and bb_enable_ant_div_lnadiv=0 */
            writeField(devNum, "bb_ch1_xatten1_margin", pModal->bswMargin[0]);
            writeField(devNum, "bb_ch1_xatten1_db", pModal->bswAtten[0]);
            writeField(devNum, "bb_ch1_xatten2_margin", pModal->xatten2Margin[0]);
            writeField(devNum, "bb_ch1_xatten2_db", pModal->xatten2Db[0]);
    }
		writeField(devNum, "an_top3_xpabias_lvl", pModal->xpaBiasLvl);	

        /* get the spur channels */
        memcpy(pLibDev->libCfgParams.spurChans, 
               pModal->spurChans, 
               AR9285_EEPROM_MODAL_SPURS*sizeof(A_UINT32));

        /* load the spur channels, if any */
        for( i=0; i<5; i++ ) {
            /* check for list terminator */
            if(0x8000 == pLibDev->libCfgParams.spurChans[i]) {
                pLibDev->libCfgParams.spurChans[i] = 0;
                break;
            }
            /* decode the frequencies */
            pLibDev->libCfgParams.spurChans[i]=
            ( ((pLibDev->libCfgParams.spurChans[i]) + ((is2GHz) ? 23000 : 49000)) * 100 )/1000;
        }

		/* load the Tx gain table type */
        pLibDev->libCfgParams.txGainType = pBase->txGainType & 0x03;

    pLibDev->eepromHeaderApplied[pLibDev->mode] = TRUE;
    return;
}

/**
 * - Function Name: ar9285EepromSetTransmitPower
 *                  operating channel, mode and rate
 * - Arguments
 *     - devNum: device number
 *     - freq: operating channel (in MHz)
 *     - pRates: rate for which the power should be set. If null, set output
 *               power to target power.
 * - Returns      : none
 ******************************************************************************/
void
ar9285EepromSetTransmitPower(A_UINT32 devNum, A_UINT32 freq, A_UINT16 *pRates)
{
    LIB_DEV_INFO  *pLibDev = gLibInfo.pLibDevArray[devNum];
    A_BOOL is2GHz = ((pLibDev->mode == MODE_11G) || (pLibDev->mode == MODE_11B));
    Ar9285_MODAL_EEP_HEADER *pModal;
    EEPROM_CHANNEL pEChan;
    A_INT16 ratesArray[Ar5416RateSize];
    A_UINT16 powerLimit = AR9285_MAX_RATE_POWER;
    A_INT16  txPowerIndexOffset = 0;
    A_UINT16 cfgCtl = (A_UINT16)(pLibDev->ar9285Eep->baseEepHeader.regDmn[0] & 0xFF);
    A_UINT16 twiceAntennaReduction = (A_UINT16)((cfgCtl == 0x10) ? 6 : 0);
    A_UINT8  ht40PowerIncForPdadc = 2;
    int i;
	pModal = &(pLibDev->ar9285Eep->modalHeader[0]);
    pEChan.is2GHz = is2GHz;
    pEChan.synthCenter = (A_UINT16)freq;
    pEChan.ht40Center = (A_UINT16)freq;
    pEChan.isHt40 = (A_BOOL)pLibDev->libCfgParams.ht40_enable;
    pEChan.txMask = pLibDev->ar9285Eep->baseEepHeader.txMask;
    pEChan.activeTxChains = (A_UINT16)( ((pLibDev->libCfgParams.tx_chain_mask >> 2) & 1) +
        ((pLibDev->libCfgParams.tx_chain_mask >> 1) & 1) + (pLibDev->libCfgParams.tx_chain_mask & 1) );
    if (pEChan.isHt40) {
        int ctlDir;
        ctlDir = ((pLibDev->libCfgParams.extended_channel_op) ? (-1) : (1));
        pEChan.ctlCenter = (A_UINT16)(freq + (ctlDir * 10));
        pEChan.extCenter = (A_UINT16)(freq - (ctlDir * ((pLibDev->libCfgParams.extended_channel_sep == 20) ? (10) : (15))));
    } else {
        pEChan.extCenter = (A_UINT16)freq;
        pEChan.ctlCenter = (A_UINT16)freq;
    }

    memset(ratesArray, 0, sizeof(ratesArray));

    if ((pLibDev->ar9285Eep->baseEepHeader.version & AR9285_EEP_VER_MINOR_MASK) >= AR9285_EEP_MINOR_VER_2) {
        ht40PowerIncForPdadc = pModal->ht40PowerIncForPdadc;
    }

    /* Setup MIMO eepromChannel info */
    if (pRates) {
        memcpy(ratesArray, pRates, sizeof(A_UINT16) * Ar5416RateSize);
    }

    if(pLibDev->eePromLoad) {
        assert(((pLibDev->eepData.version >> 12) & 0xF) == AR9285_EEP_VER);
        cfgCtl = (A_UINT16)(pLibDev->ar9285Eep->baseEepHeader.regDmn[0] & 0xF0);
        /* set output power to target power from the calTgtPwr file */
        if (!pRates) {
            ar9285SetPowerPerRateTable(pLibDev->ar9285Eep, &pEChan, &ratesArray[0], cfgCtl, twiceAntennaReduction, powerLimit);
        }
        ar9285SetPowerCalTable(devNum, pLibDev->ar9285Eep, &pEChan, &txPowerIndexOffset);
    }
    /*
     * txPowerIndexOffset is set by the SetPowerTable() call -
     *  adjust the rate table (0 offset if rates EEPROM not loaded)
     */
    for (i = 0; i < N(ratesArray); i++) {
        ratesArray[i] = (A_INT16)(txPowerIndexOffset + ratesArray[i]);
        if (ratesArray[i] > AR9285_MAX_RATE_POWER)
            ratesArray[i] = AR9285_MAX_RATE_POWER;
    }
    pLibDev->pwrIndexOffset = txPowerIndexOffset;

    if(isMerlinPowerControl(pLibDev->swDevID)) {
        for (i = 0; i < Ar5416RateSize; i++) {
            ratesArray[i] -= AR9285_PWR_TABLE_OFFSET*2;
        }
    }
    /* Write the OFDM power per rate set */
    REGW(devNum, 0x9934,
        POW_SM(ratesArray[rate18mb], 24)
          | POW_SM(ratesArray[rate12mb], 16)
          | POW_SM(ratesArray[rate9mb],  8)
          | POW_SM(ratesArray[rate6mb],  0)
    );
    REGW(devNum, 0x9938,
        POW_SM(ratesArray[rate54mb], 24)
          | POW_SM(ratesArray[rate48mb], 16)
          | POW_SM(ratesArray[rate36mb],  8)
          | POW_SM(ratesArray[rate24mb],  0)
    );

    /* Write the CCK power per rate set */
    REGW(devNum, 0xa234,
        POW_SM(ratesArray[rate2s], 24)
          | POW_SM(ratesArray[rate2l],  16)
          | POW_SM(ratesArray[rateXr],  8) /* XR target power */
          | POW_SM(ratesArray[rate1l],   0)
    );
    REGW(devNum, 0xa238,
        POW_SM(ratesArray[rate11s], 24)
          | POW_SM(ratesArray[rate11l], 16)
          | POW_SM(ratesArray[rate5_5s],  8)
          | POW_SM(ratesArray[rate5_5l],  0)
    );

	if(!isKite11gSW(pLibDev->swDevID))
	{
		/* Write the HT20 power per rate set */
		REGW(devNum, 0xa38C,
			POW_SM(ratesArray[rateHt20_3], 24)
			  | POW_SM(ratesArray[rateHt20_2],  16)
			  | POW_SM(ratesArray[rateHt20_1],  8)
			  | POW_SM(ratesArray[rateHt20_0],   0)
		);
		REGW(devNum, 0xa390,
			POW_SM(ratesArray[rateHt20_7], 24)
			  | POW_SM(ratesArray[rateHt20_6],  16)
			  | POW_SM(ratesArray[rateHt20_5],  8)
			  | POW_SM(ratesArray[rateHt20_4],   0)
		);

		/* Write the HT40 power per rate set */
		// correct PAR difference between HT40 and HT20/LEGACY
		REGW(devNum, 0xa3CC,
			POW_SM(ratesArray[rateHt40_3] + ht40PowerIncForPdadc, 24)
			  | POW_SM(ratesArray[rateHt40_2] + ht40PowerIncForPdadc,  16)
			  | POW_SM(ratesArray[rateHt40_1] + ht40PowerIncForPdadc,  8)
			  | POW_SM(ratesArray[rateHt40_0] + ht40PowerIncForPdadc,   0)
		);
		REGW(devNum, 0xa3D0,
			POW_SM(ratesArray[rateHt40_7] + ht40PowerIncForPdadc, 24)
			  | POW_SM(ratesArray[rateHt40_6] + ht40PowerIncForPdadc,  16)
			  | POW_SM(ratesArray[rateHt40_5] + ht40PowerIncForPdadc,  8)
			  | POW_SM(ratesArray[rateHt40_4] + ht40PowerIncForPdadc,   0)
		);
		/* Write the Dup/Ext 40 power per rate set */
		REGW(devNum, 0xa3D4,
			POW_SM(ratesArray[rateExtOfdm], 24)
			  | POW_SM(ratesArray[rateExtCck],  16)
			  | POW_SM(ratesArray[rateDupOfdm],  8)
			  | POW_SM(ratesArray[rateDupCck],   0)
		);
	}

    return;
}
/* ar9285EepromSetTransmitPower */

/**************************************************************
 * ar9285SetPowerPerRateTable
 *
 * Sets the transmit power in the baseband for the given
 * operating channel and mode.
 */
static void
ar9285SetPowerPerRateTable(ar9285_EEPROM *pEepData,
                           EEPROM_CHANNEL *pEChan,
                           A_INT16 *ratesArray,
                           A_UINT16 cfgCtl,
                           A_UINT16 AntennaReduction,
                           A_UINT16 powerLimit)
{
/* Local defines to distinguish between extension and control CTL's */
#define EXT_ADDITIVE (0x8000)
#define CTL_11A_EXT (CTL_11A | EXT_ADDITIVE)
#define CTL_11G_EXT (CTL_11G | EXT_ADDITIVE)
#define CTL_11B_EXT (CTL_11B | EXT_ADDITIVE)
    A_UINT16 twiceMaxEdgePower = AR9285_MAX_RATE_POWER;
    A_UINT16 twiceMaxRDPower = AR9285_MAX_RATE_POWER;
    int i;
    A_INT16  twiceAntennaReduction, largestAntenna;
    Ar9285_CAL_CTL_DATA *rep;
    CAL_TARGET_POWER_LEG targetPowerOfdm = {0, 0, 0, 0, 0}, targetPowerCck = {0, 0, 0, 0, 0};
    CAL_TARGET_POWER_LEG targetPowerOfdmExt = {0, 0, 0, 0, 0}, targetPowerCckExt = {0, 0, 0, 0, 0};
    CAL_TARGET_POWER_HT  targetPowerHt20, targetPowerHt40 = {0, 0, 0, 0, 0};
    A_INT16 scaledPower, minCtlPower;
//    A_UINT16 ctlModesFor11a[] = {CTL_11A, CTL_5GHT20, CTL_11A_EXT, CTL_5GHT40};
    A_UINT16 ctlModesFor11g[] = {CTL_11B, CTL_11G, CTL_2GHT20, CTL_11B_EXT, CTL_11G_EXT, CTL_2GHT40};
    A_UINT16 numCtlModes = 0, *pCtlMode = 0, ctlMode = 0;
    A_UINT16 freq;

    /* Decrease the baseline target power with the regulatory maximums */
    twiceMaxRDPower = AR9285_MAX_RATE_POWER;

    /* Compute TxPower reduction due to Antenna Gain */
    largestAntenna = A_MAX(A_MAX(pEepData->modalHeader[0].antennaGainCh[0], pEepData->modalHeader[0].antennaGainCh[0]),
        pEepData->modalHeader[pEChan->is2GHz].antennaGainCh[0]);
    twiceAntennaReduction =  (A_INT16)A_MAX((AntennaReduction * 2) - largestAntenna, 0);

    /* scaledPower is the minimum of the user input power level and the regulatory allowed power level */
    scaledPower = (A_INT16)A_MIN(powerLimit, twiceMaxRDPower + twiceAntennaReduction);

    /* Get target powers from EEPROM - our baseline for TX Power */
    if (pEChan->is2GHz) {
        /* Setup for CTL modes */
        numCtlModes = N(ctlModesFor11g) - 3; // No ExtCck, ExtOfd, HT40
        pCtlMode = ctlModesFor11g;

        ar9285GetTargetPowersLeg(pEChan, pEepData->calTargetPowerCck,
                              AR9285_NUM_2G_CCK_TARGET_POWERS, &targetPowerCck, 4, FALSE);
        ar9285GetTargetPowersLeg(pEChan, pEepData->calTargetPower2G,
                              AR9285_NUM_2G_20_TARGET_POWERS, &targetPowerOfdm, 4, FALSE);
		if((pEepData->baseEepHeader.opCapFlags.opFlags & AR9285_OPFLAGS_2G_HT20) == 0)
		{
			ar9285GetTargetPowers(pEChan, pEepData->calTargetPower2GHT20,
								  AR9285_NUM_2G_20_TARGET_POWERS, &targetPowerHt20, 8, FALSE);
			if (pEChan->isHt40) {
				numCtlModes = N(ctlModesFor11g);
				ar9285GetTargetPowers(pEChan, pEepData->calTargetPower2GHT40,
									  AR9285_NUM_2G_40_TARGET_POWERS, &targetPowerHt40, 8, TRUE);
				ar9285GetTargetPowersLeg(pEChan, pEepData->calTargetPowerCck,
									  AR9285_NUM_2G_CCK_TARGET_POWERS, &targetPowerCckExt, 4, TRUE);
				ar9285GetTargetPowersLeg(pEChan, pEepData->calTargetPower2G,
									  AR9285_NUM_2G_20_TARGET_POWERS, &targetPowerOfdmExt, 4, TRUE);
			}
		}
    } 

	//bug fix for 21429, don't apply the CTLs when in ART mode
	numCtlModes = 0;

    /* For MIMO, need to apply regulatory caps individually across dynamically running modes: CCK, OFDM, HT20, HT40 */
    for (ctlMode = 0; ctlMode < numCtlModes; ctlMode++) {
        A_BOOL isHt40CtlMode = (pCtlMode[ctlMode] == CTL_5GHT40) || (pCtlMode[ctlMode] == CTL_2GHT40);
        if (isHt40CtlMode) {
            freq = pEChan->ht40Center;
        } else if (pCtlMode[ctlMode] & EXT_ADDITIVE) {
            freq = pEChan->extCenter;
        } else {
            freq = pEChan->ctlCenter;
        }
        for (i = 0; (i < AR9285_NUM_CTLS) && pEepData->ctlIndex[i]; i++) {
            A_UINT16 twiceMinEdgePower;

            if (((cfgCtl | (pCtlMode[ctlMode] & CTL_MODE_M)) == pEepData->ctlIndex[i]) ||
                ((cfgCtl | (pCtlMode[ctlMode] & CTL_MODE_M))== ((pEepData->ctlIndex[i] & CTL_MODE_M) | SD_NO_CTL)))
            {
                rep = &(pEepData->ctlData[i]);
                twiceMinEdgePower = ar9285GetMaxEdgePower(freq, rep->ctlEdges[pEChan->activeTxChains - 1], pEChan->is2GHz);
                if (cfgCtl == SD_NO_CTL) {
                    /* Find the minimum of all CTL edge powers that apply to this channel */
                    twiceMaxEdgePower = A_MIN(twiceMaxEdgePower, twiceMinEdgePower);
                } else {
                    twiceMaxEdgePower = twiceMinEdgePower;
                    break;
                }
            }
        }
        minCtlPower = (A_UCHAR)A_MIN(twiceMaxEdgePower, scaledPower);
        /* ==Apply ctl mode to correct target power set== */
        switch(pCtlMode[ctlMode]) {
        case CTL_11B:
            for (i = 0; i < N(targetPowerCck.tPow2x); i++) {
                targetPowerCck.tPow2x[i] = (A_UCHAR)A_MIN(targetPowerCck.tPow2x[i], minCtlPower);
            }
            break;
        case CTL_11A:
        case CTL_11G:
            for (i = 0; i < N(targetPowerOfdm.tPow2x); i++) {
                targetPowerOfdm.tPow2x[i] = (A_UCHAR)A_MIN(targetPowerOfdm.tPow2x[i], minCtlPower);
            }
            break;
        case CTL_5GHT20:
        case CTL_2GHT20:
            for (i = 0; i < N(targetPowerHt20.tPow2x); i++) {
                targetPowerHt20.tPow2x[i] = (A_UCHAR)A_MIN(targetPowerHt20.tPow2x[i], minCtlPower);
            }
            break;
        case CTL_11B_EXT:
            targetPowerCckExt.tPow2x[0] = (A_UCHAR)A_MIN(targetPowerCckExt.tPow2x[0], minCtlPower);
            break;
        case CTL_11A_EXT:
        case CTL_11G_EXT:
            targetPowerOfdmExt.tPow2x[0] = (A_UCHAR)A_MIN(targetPowerOfdmExt.tPow2x[0], minCtlPower);
            break;
        case CTL_5GHT40:
        case CTL_2GHT40:
            for (i = 0; i < N(targetPowerHt40.tPow2x); i++) {
                targetPowerHt40.tPow2x[i] = (A_UCHAR)A_MIN(targetPowerHt40.tPow2x[i], minCtlPower);
            }
            break;
        default:
            assert(0);
            break;
        }

    } /* end ctl mode checking */

    /* Set rates Array from collected data */
    ratesArray[rate6mb] = ratesArray[rate9mb] = ratesArray[rate12mb] = ratesArray[rate18mb] = ratesArray[rate24mb] = targetPowerOfdm.tPow2x[0];
    ratesArray[rate36mb] = targetPowerOfdm.tPow2x[1];
    ratesArray[rate48mb] = targetPowerOfdm.tPow2x[2];
    ratesArray[rate54mb] = targetPowerOfdm.tPow2x[3];
    ratesArray[rateXr] = targetPowerOfdm.tPow2x[0];

    for (i = 0; i < N(targetPowerHt20.tPow2x); i++) {
        ratesArray[rateHt20_0 + i] = targetPowerHt20.tPow2x[i];
    }

    if (pEChan->is2GHz) {
        ratesArray[rate1l]  = targetPowerCck.tPow2x[0];
        ratesArray[rate2s] = ratesArray[rate2l]  = targetPowerCck.tPow2x[1];
        ratesArray[rate5_5s] = ratesArray[rate5_5l] = targetPowerCck.tPow2x[2];;
        ratesArray[rate11s] = ratesArray[rate11l] = targetPowerCck.tPow2x[3];;
    }
    if (pEChan->isHt40) {
        for (i = 0; i < N(targetPowerHt40.tPow2x); i++) {
            ratesArray[rateHt40_0 + i] = targetPowerHt40.tPow2x[i];
        }
        ratesArray[rateDupOfdm] = targetPowerHt40.tPow2x[0];
        ratesArray[rateDupCck]  = targetPowerHt40.tPow2x[0];
        ratesArray[rateExtOfdm] = targetPowerOfdmExt.tPow2x[0];
        if (pEChan->is2GHz) {
            ratesArray[rateExtCck]  = targetPowerCckExt.tPow2x[0];
        }
    }
    return;
#undef EXT_ADDITIVE
#undef CTL_11A_EXT
#undef CTL_11G_EXT
#undef CTL_11B_EXT
}

/**
 * - Function Name: get9285MaxPowerForRate
 * - Description  :
 * - Arguments
 *     -
 * - Returns      :
 ******************************************************************************/
A_INT16 get9285MaxPowerForRate(
 A_UINT32 devNum,
 A_UINT32 freq,
 A_UINT32 rate)
{
    LIB_DEV_INFO  *pLibDev = gLibInfo.pLibDevArray[devNum];
    A_INT16     ratesArray[Ar5416RateSize];
    A_UINT16        dataRate = 0, i;

    //get and set the max power values
    ar9285GetPowerPerRateTable(devNum, (A_UINT16)freq, ratesArray);
    if(isMerlinPowerControl(pLibDev->swDevID)) {
        for (i = 0; i < Ar5416RateSize; i++) {
//            ratesArray[i] = ratesArray[i] += AR9285_PWR_TABLE_OFFSET*2;
        }
    }

    //TODO - do the mapping of input rate to power/rate
    switch(rate) {
    //legacy ofdm
    case 6: dataRate = rate6mb; break;
    case 9: dataRate = rate9mb; break;
    case 12: dataRate = rate12mb; break;
    case 18: dataRate = rate18mb; break;
    case 24: dataRate = rate24mb; break;
    case 36: dataRate = rate36mb; break;
    case 48: dataRate = rate48mb; break;
    case 54: dataRate = rate54mb; break;

    //CCK rates
    case 0xb1: dataRate = rate1l; break;
    case 0xb2: dataRate = rate2l; break;
    case 0xd2: dataRate = rate2s; break;
    case 0xb5: dataRate = rate5_5l; break;
    case 0xd5: dataRate = rate5_5s; break;
    case 0xbb: dataRate = rate11l; break;
    case 0xdb: dataRate = rate11s; break;

    //XR
    case 0xea:
    case 0xeb:
    case 0xe1:
    case 0xe2:
    case 0xe3:
        dataRate = rateXr;
        break;

    //HT20 rates
    case 0x40:
    case 0x48:
        dataRate = rateHt20_0;
        break;
    case 0x41:
    case 0x49:
        dataRate = rateHt20_1;
        break;
    case 0x42:
    case 0x4a:
        dataRate = rateHt20_2;
        break;
    case 0x43:
    case 0x4b:
        dataRate = rateHt20_3;
        break;
    case 0x44:
    case 0x4c:
        dataRate = rateHt20_4;
        break;
    case 0x45:
    case 0x4d:
        dataRate = rateHt20_5;
        break;
    case 0x46:
    case 0x4e:
        dataRate = rateHt20_6;
        break;
    case 0x47:
    case 0x4f:
        dataRate = rateHt20_7;
        break;

    //HT40 rates
    case 0x80:
    case 0x88:
        dataRate = rateHt40_0;
        break;
    case 0x81:
    case 0x89:
        dataRate = rateHt40_1;
        break;
    case 0x82:
    case 0x8a:
        dataRate = rateHt40_2;
        break;
    case 0x83:
    case 0x8b:
        dataRate = rateHt40_3;
        break;
    case 0x84:
    case 0x8c:
        dataRate = rateHt40_4;
        break;
    case 0x85:
    case 0x8d:
        dataRate = rateHt40_5;
        break;
    case 0x86:
    case 0x8e:
        dataRate = rateHt40_6;
        break;
    case 0x87:
    case 0x8f:
        dataRate = rateHt40_7;
        break;
    default:
        mError(devNum, EINVAL, "Device Number %d:DATA rate does not match a known rate: %d\n",
            devNum, rate);
        return (0xfff);
        break;
    }
    return(ratesArray[dataRate]);
}

/**************************************************************
 * ar9285GetMaxEdgePower
 *
 * Find the maximum conformance test limit for the given channel and CTL info
 */
static A_UINT16
ar9285GetMaxEdgePower(A_UINT16 freq, CAL_CTL_EDGES *pRdEdgesPower, A_BOOL is2GHz)
{
    A_UINT16 twiceMaxEdgePower = AR9285_MAX_RATE_POWER;
    int      i;

    /* Get the edge power */
    for (i = 0; (i < AR9285_NUM_BAND_EDGES) && (pRdEdgesPower[i].bChannel != AR9285_BCHAN_UNUSED) ; i++) {
        /*
         * If there's an exact channel match or an inband flag set
         * on the lower channel use the given rdEdgePower
         */
        if (freq == fbin2freq(pRdEdgesPower[i].bChannel, is2GHz)) {
            twiceMaxEdgePower = pRdEdgesPower[i].tPower;
            break;
        } else if ((i > 0) && (freq < fbin2freq(pRdEdgesPower[i].bChannel, is2GHz))) {
            if (fbin2freq(pRdEdgesPower[i - 1].bChannel, is2GHz) < freq && pRdEdgesPower[i - 1].flag) {
                twiceMaxEdgePower = pRdEdgesPower[i - 1].tPower;
            }
            /* Leave loop - no more affecting edges possible in this monotonic increasing list */
            break;
        }
    }
    assert(twiceMaxEdgePower > 0);
    return twiceMaxEdgePower;
}

/**************************************************************
 * ar9285GetTargetPowers
 *
 * Return the rates of target power for the given target power table
 * channel, and number of channels
 */
static void
ar9285GetTargetPowers(EEPROM_CHANNEL *pEChan, CAL_TARGET_POWER_HT *powInfo,
                      A_UINT16 numChannels, CAL_TARGET_POWER_HT *pNewPower,
                      A_UINT16 numRates, A_BOOL isHt40Target)
{
    A_UINT16 clo, chi;
    int i;
    int matchIndex = -1, lowIndex = -1;
    A_UINT16 freq;
    freq = isHt40Target ? pEChan->ht40Center : pEChan->ctlCenter;

    /* Copy the target powers into the temp channel list */
    if (freq <= fbin2freq(powInfo[0].bChannel, pEChan->is2GHz)) {
        matchIndex = 0;
    } else {
        for (i = 0; (i < numChannels) && (powInfo[i].bChannel != AR9285_BCHAN_UNUSED); i++) {
            if (freq == fbin2freq(powInfo[i].bChannel, pEChan->is2GHz)) {
                matchIndex = i;
                break;
            } else if ((freq < fbin2freq(powInfo[i].bChannel, pEChan->is2GHz)) &&
                       (freq > fbin2freq(powInfo[i - 1].bChannel, pEChan->is2GHz)))
            {
                lowIndex = i - 1;
                break;
            }
        }
        if ((matchIndex == -1) && (lowIndex == -1)) {
            assert(freq > fbin2freq(powInfo[i - 1].bChannel, pEChan->is2GHz));
            matchIndex = i - 1;
        }
    }

    if (matchIndex != -1) {
        *pNewPower = powInfo[matchIndex];
    } else {
        assert(lowIndex != -1);
        /*
         * Get the lower and upper channels, target powers,
         * and interpolate between them.
         */
        clo = fbin2freq(powInfo[lowIndex].bChannel, pEChan->is2GHz);
        chi = fbin2freq(powInfo[lowIndex + 1].bChannel, pEChan->is2GHz);

        for (i = 0; i < numRates; i++) {
            pNewPower->tPow2x[i] = (A_UCHAR)interpolate(freq, clo, chi,
                                   powInfo[lowIndex].tPow2x[i], powInfo[lowIndex + 1].tPow2x[i]);
        }
    }
}

/**************************************************************
 * ar9285GetTargetPowersLeg
 *
 * Return the four rates of target power for the given target power table
 * channel, and number of channels
 */
static void
ar9285GetTargetPowersLeg(EEPROM_CHANNEL *pEChan, CAL_TARGET_POWER_LEG *powInfo,
                      A_UINT16 numChannels, CAL_TARGET_POWER_LEG *pNewPower, A_UINT16 numRates, A_BOOL isExtTarget)
{
    A_UINT16 clo, chi;
    int i;
    int matchIndex = -1, lowIndex = -1;
    A_UINT16 freq;
    freq = isExtTarget ? pEChan->extCenter : pEChan->ctlCenter;

    /* Copy the target powers into the temp channel list */
    if (freq <= fbin2freq(powInfo[0].bChannel, pEChan->is2GHz)) {
        matchIndex = 0;
    } else {
        for (i = 0; (i < numChannels) && (powInfo[i].bChannel != AR9285_BCHAN_UNUSED); i++) {
            if (freq == fbin2freq(powInfo[i].bChannel, pEChan->is2GHz)) {
                matchIndex = i;
                break;
            } else if ((freq < fbin2freq(powInfo[i].bChannel, pEChan->is2GHz)) &&
                       (freq > fbin2freq(powInfo[i - 1].bChannel, pEChan->is2GHz)))
            {
                lowIndex = i - 1;
                break;
            }
        }
        if ((matchIndex == -1) && (lowIndex == -1)) {
            assert(freq > fbin2freq(powInfo[i - 1].bChannel, pEChan->is2GHz));
            matchIndex = i - 1;
        }
    }

    if (matchIndex != -1) {
        *pNewPower = powInfo[matchIndex];
    } else {
        assert(lowIndex != -1);
        /*
         * Get the lower and upper channels, target powers,
         * and interpolate between them.
         */
        clo = fbin2freq(powInfo[lowIndex].bChannel, pEChan->is2GHz);
        chi = fbin2freq(powInfo[lowIndex + 1].bChannel, pEChan->is2GHz);

        for (i = 0; i < numRates; i++) {
            pNewPower->tPow2x[i] = (A_UCHAR)interpolate(freq, clo, chi,
                                   powInfo[lowIndex].tPow2x[i], powInfo[lowIndex + 1].tPow2x[i]);
        }
    }
}

/**************************************************************
 * ar9285SetPowerCalTable
 *
 * Pull the PDADC piers from cal data and interpolate them across the given
 * points as well as from the nearest pier(s) to get a power detector
 * linear voltage to power level table.
 */
static void
ar9285SetPowerCalTable(A_UINT32 devNum, ar9285_EEPROM *pEepData, EEPROM_CHANNEL *pEChan, A_INT16 *pTxPowerIndexOffset)
{
    LIB_DEV_INFO  *pLibDev = gLibInfo.pLibDevArray[devNum];
    Ar9285_CAL_DATA_PER_FREQ *pRawDataset = 0;
    A_UINT8  *pCalBChans = NULL;
    A_UINT16 pdGainOverlap_t2;
    static A_UINT8  pdadcValues[AR9285_NUM_PDADC_VALUES];
    A_UINT16 gainBoundaries[4];
    A_UINT16 numPiers = 0, i, j;
    A_INT16  tMinCalPower;
    A_UINT16 numXpdGain, xpdMask;
    A_UINT16 xpdGainValues[AR9285_NUM_PD_GAINS];
    A_UINT32 reg32, regOffset, regChainOffset;
    A_INT16 firstPower_t2 = 0;
    A_UINT16 k, pdgainIndex = 0, powerIndex = 0;
#ifdef WIN32
    A_CHAR logfileBuffer[256];
    FILE* filePtr = NULL;
#endif

	memset(xpdGainValues, 0, sizeof(xpdGainValues)/sizeof(A_UINT16));

    xpdMask = pEepData->modalHeader[0].xpdGain;
    if ((pLibDev->ar9285Eep->baseEepHeader.version & AR9285_EEP_VER_MINOR_MASK) >= AR9285_EEP_MINOR_VER_2) {
        pdGainOverlap_t2 = pEepData->modalHeader[0].pdGainOverlap;
    } else {
        pdGainOverlap_t2 = (A_UINT16)(REGR(devNum, TPCRG5_REG) & BB_PD_GAIN_OVERLAP_MASK);
    }

    if (pEChan->is2GHz) {
        pCalBChans = pEepData->calFreqPier2G;
        numPiers = AR9285_NUM_2G_CAL_PIERS;
    }
#ifdef WIN32
    //setup to log if needed
    if (pLibDev->libCfgParams.enablePdadcLogging) {
        sprintf(logfileBuffer, "pwr_pdadc_%d.log", pEChan->synthCenter);
        filePtr = fopen(logfileBuffer, "w");
        if(!filePtr) {
            printf("Unable to open file for power to pdadc logging, disabling logging\n");
            pLibDev->libCfgParams.enablePdadcLogging = 0;
        }
    }
#endif

    numXpdGain = 0;
    /* Calculate the value of xpdgains from the xpdGain Mask */
    for (i = 1; i <= AR9285_PD_GAINS_IN_MASK; i++) {
        if ((xpdMask >> (AR9285_PD_GAINS_IN_MASK - i)) & 1) {
            if (numXpdGain >= AR9285_NUM_PD_GAINS) {
                assert(0);
                break;
            }
            xpdGainValues[numXpdGain] = (A_UINT16)(AR9285_PD_GAINS_IN_MASK - i);
            pLibDev->eepData.xpdGainValues[numXpdGain] = xpdGainValues[numXpdGain];
            numXpdGain++;
        }
    }
    pLibDev->eepData.numPdGain = numXpdGain;

    /* Write the detector gain biases and their number */
		REGW(devNum, TPCRG1_REG, (REGR(devNum, TPCRG1_REG) & 0xFFC03FFF) |
			(((numXpdGain - 1) & 0x3) << 14) | ((xpdGainValues[0] & 0x3) << 16) |
			((xpdGainValues[1] & 0x3) << 18));

    for (i = 0; i < AR9285_MAX_CHAINS; i++) {
        firstPower_t2 = *pTxPowerIndexOffset;
        pdgainIndex = 0;
        powerIndex = 0;

        regChainOffset = i * 0x1000;

        if (pEChan->txMask & (1 << i)) {
            if (pEChan->is2GHz) {
                pRawDataset = pEepData->calPierData2G[i];
            }
if (EEPROM_DEBUG_LEVEL >= 2) printf("\nCalling GetGainBoundaries for chain %d (chan=%d)\n", i, pEChan->synthCenter);

            ar9285GetGainBoundariesAndPdadcs(devNum, pEChan, pRawDataset, pCalBChans, numPiers, pdGainOverlap_t2,
                                         &tMinCalPower, gainBoundaries, pdadcValues, numXpdGain);
            if (i == 0) {
                /* Some items done only once */
                pLibDev->eepData.midPower = gainBoundaries[0];
                /*
                 * Note the pdadc table may not start at 0 dBm power, could be
                 * negative or greater than 0.  Need to offset the power
                 * values by the amount of minPower for griffin
                 */
//                if (tMinCalPower != 0) {
//                    *pTxPowerIndexOffset = (A_INT16)(0 - tMinCalPower);
//                } else {
                    *pTxPowerIndexOffset = 0;
//                }
            }
			if(gainBoundaries[1] > 0x3a)
			{
				gainBoundaries[1] = 0x3a;
				gainBoundaries[2] = 0x3a;
				gainBoundaries[3] = 0x3a;
			}
            if ((i == 0) || IS_MAC_5416_2_0_UP(pLibDev->macRev)) {
                /* per chain as of ar5416 v2 */
                REGW(devNum, 0xa26c + regChainOffset,
                     (pdGainOverlap_t2 & 0xf) |
                     ((gainBoundaries[0] & 0x3f) << 4)  |
                     ((gainBoundaries[1] & 0x3f) << 10) |
                     ((gainBoundaries[2] & 0x3f) << 16) |
                     ((gainBoundaries[3] & 0x3f) << 22));
#ifdef EEPROM_DUMP
                printf("PDADC chain %d: NumPd Gains %d, xpd_Gain1 %d, xpd_Gain2 %d, Overlap %2d.%d, dB Bound1 %2d.%d, Bound2 %2d.%d\n",
                    i, numXpdGain, xpdGainValues[0], xpdGainValues[1],
                    pdGainOverlap_t2/2, (pdGainOverlap_t2 % 2) * 5,
                    gainBoundaries[0]/2, (gainBoundaries[0] % 2) * 5,
                    gainBoundaries[1]/2, (gainBoundaries[1] % 2) * 5);
#endif
#ifdef WIN32
                if (pLibDev->libCfgParams.enablePdadcLogging) {
                    fprintf(filePtr, "\nchannel = %d, ch = %d\n", pEChan->synthCenter, i);
                    fprintf(filePtr, "pdgain = %d, gainBoundary = %3.1f\n", xpdGainValues[0], (float)gainBoundaries[0]/2);
                    fprintf(filePtr, "\npower, pdadc\n");
                }
#endif //WIN32
            }
            /* Write the power values into the baseband power table - physically layed out */
            regOffset = 0x9800 + (672 << 2) + regChainOffset;
            for (j = 0; j < 32; j++) {
                reg32 = ((pdadcValues[4*j + 0] & 0xFF) << 0)  |
                    ((pdadcValues[4*j + 1] & 0xFF) << 8)  |
                    ((pdadcValues[4*j + 2] & 0xFF) << 16) |
                    ((pdadcValues[4*j + 3] & 0xFF) << 24) ;
                REGW(devNum, regOffset, reg32);
#ifdef EEPROM_DUMP
                printf("PDADC: Chain %d | PDADC %3d Value %3d | PDADC %3d Value %3d | PDADC %3d Value %3d | PDADC %3d Value %3d |\n",
                    i,
                    4*j, pdadcValues[4*j],
                    4*j+1, pdadcValues[4*j + 1],
                    4*j+2, pdadcValues[4*j + 2],
                    4*j+3, pdadcValues[4*j + 3]);
#endif
#ifdef WIN32
                if (pLibDev->libCfgParams.enablePdadcLogging) {
                    if(pdadcValues[4*j] != pdadcValues[4*j + 1]) {
                        for(k = 0; k < 3; k++) {
                            fprintf(filePtr, " %3.1f,%d,\n",
                                float(firstPower_t2 + 4*powerIndex+k)/2, pdadcValues[4*j + k]);
                            if(pdadcValues[4*j + k+1] < pdadcValues[4*j + k]) {
                                //new pdgain
                                pdgainIndex++;
                                powerIndex = 0;
                                firstPower_t2 = (A_INT16)(gainBoundaries[pdgainIndex - 1]- 10 - pdGainOverlap_t2 - k); //compensate for where k is
                                fprintf(filePtr, "\nchannel = %d, ch = %d,\n", pEChan->synthCenter, i);
                                fprintf(filePtr, "pdgain = %d,gainBoundary = %3.1f\n", xpdGainValues[pdgainIndex], (float)gainBoundaries[pdgainIndex]/2);
                                fprintf(filePtr, "\npower, pdadc,\n");
                            }
                        }
                        fprintf(filePtr, " %3.1f,%d,\n",
                                float(firstPower_t2 + 4*powerIndex+k)/2, pdadcValues[4*j + k]);

                        powerIndex++;
                    }
                }
#endif //WIN32
                regOffset += 4;
            }
        }
    }

#ifdef WIN32
    //setup to log if needed
    if ( (pLibDev->libCfgParams.enablePdadcLogging) && (filePtr != NULL) ) {
        fclose(filePtr);
    }
#endif //WIN32
    return;
}

/**************************************************************
 * ar9285GetGainBoundariesAndPdadcs
 *
 * Uses the data points read from EEPROM to reconstruct the pdadc power table
 * Called by ar9285SetPowerCalTable only.
 */
static void
ar9285GetGainBoundariesAndPdadcs(A_UINT32 devNum, EEPROM_CHANNEL *pEChan, Ar9285_CAL_DATA_PER_FREQ *pRawDataSet,
                                 A_UINT8 * bChans,  A_UINT16 availPiers,
                                 A_UINT16 tPdGainOverlap, A_INT16 *pMinCalPower, A_UINT16 * pPdGainBoundaries,
                                 A_UINT8 * pPDADCValues, A_UINT16 numXpdGains)
{
    LIB_DEV_INFO  *pLibDev = gLibInfo.pLibDevArray[devNum];
    int       i, j, k;
    A_INT16   ss;         /* potentially -ve index for taking care of pdGainOverlap */
    A_UINT16  idxL, idxR, numPiers; /* Pier indexes */

    /* filled out Vpd table for all pdGains (chanL) */
    static A_UINT8   vpdTableL[AR9285_NUM_PD_GAINS][AR9285_MAX_PWR_RANGE_IN_HALF_DB];

    /* filled out Vpd table for all pdGains (chanR) */
    static A_UINT8   vpdTableR[AR9285_NUM_PD_GAINS][AR9285_MAX_PWR_RANGE_IN_HALF_DB];

    /* filled out Vpd table for all pdGains (interpolated) */
    static A_UINT8   vpdTableI[AR9285_NUM_PD_GAINS][AR9285_MAX_PWR_RANGE_IN_HALF_DB];

    A_UINT8   *pVpdL, *pVpdR, *pPwrL, *pPwrR;
    A_UINT8   minPwrT4[AR9285_NUM_PD_GAINS];
    A_UINT8   maxPwrT4[AR9285_NUM_PD_GAINS];
    A_INT16   vpdStep;
    A_INT16   tmpVal;
    A_UINT16  sizeCurrVpdTable, maxIndex, tgtIndex;
    A_BOOL    match;
    A_INT16  minDelta = 0;

    /* Trim numPiers for the number of populated channel Piers */
    for (numPiers = 0; numPiers < availPiers; numPiers++) {
        if (bChans[numPiers] == AR9285_BCHAN_UNUSED) {
            break;
        }
    }

    if(pLibDev->libCfgParams.verifyPdadcTable) {
        for (i = 0; i < numPiers; i++) {
            if (!ar9285VerifyPdadcTable(devNum, &pRawDataSet[i], numXpdGains)) {
                printf("\n\nBad pdadc table!!!!!  Try recalibration\n\n");
        	    printf("\n\n");
	            printf("                    ########    ###    #### ##             \n");
	            printf("                    ##         ## ##    ##  ##             \n");
	            printf("                    ##        ##   ##   ##  ##             \n");
	            printf("                    ######   ##     ##  ##  ##             \n");
	            printf("                    ##       #########  ##  ##             \n");
	            printf("                    ##       ##     ##  ##  ##             \n");
	            printf("                    ##       ##     ## #### ########       \n");
                printf("\n\n");
                exit(0);

            }
        }
    }

    /* Find pier indexes around the current channel */
    match = getLowerUpperIndex((A_UINT8)FREQ2FBIN(pEChan->synthCenter, pEChan->is2GHz),
        bChans, numPiers, &idxL,
        &idxR);

    if (match) {
        /* Directly fill both vpd tables from the matching index */
        for (i = 0; i < numXpdGains; i++) {
            minPwrT4[i] = pRawDataSet[idxL].pwrPdg[i][0];
            maxPwrT4[i] = pRawDataSet[idxL].pwrPdg[i][4];
            ar9285FillVpdTable(minPwrT4[i], maxPwrT4[i], pRawDataSet[idxL].pwrPdg[i],
                               pRawDataSet[idxL].vpdPdg[i], AR9285_PD_GAIN_ICEPTS, vpdTableI[i]);
        }
    } else {
        for (i = 0; i < numXpdGains; i++) {
            pVpdL = pRawDataSet[idxL].vpdPdg[i];
            pPwrL = pRawDataSet[idxL].pwrPdg[i];
            pVpdR = pRawDataSet[idxR].vpdPdg[i];
            pPwrR = pRawDataSet[idxR].pwrPdg[i];

            /* Start Vpd interpolation from the max of the minimum powers */
            minPwrT4[i] = A_MAX(pPwrL[0], pPwrR[0]);

            /* End Vpd interpolation from the min of the max powers */
            maxPwrT4[i] = A_MIN(pPwrL[AR9285_PD_GAIN_ICEPTS - 1], pPwrR[AR9285_PD_GAIN_ICEPTS - 1]);
            assert(maxPwrT4[i] > minPwrT4[i]);

            /* Fill pier Vpds */
            ar9285FillVpdTable(minPwrT4[i], maxPwrT4[i], pPwrL, pVpdL, AR9285_PD_GAIN_ICEPTS, vpdTableL[i]);
            ar9285FillVpdTable(minPwrT4[i], maxPwrT4[i], pPwrR, pVpdR, AR9285_PD_GAIN_ICEPTS, vpdTableR[i]);
if (EEPROM_DEBUG_LEVEL >= 2) printf("\nfinal table for %d :\n", i);
            /* Interpolate the final vpd */
            for (j = 0; j <= (maxPwrT4[i] - minPwrT4[i]) / 2; j++) {
                vpdTableI[i][j] = (A_UINT8)(interpolate((A_UINT16)FREQ2FBIN(pEChan->synthCenter, pEChan->is2GHz),
                    bChans[idxL], bChans[idxR], vpdTableL[i][j], vpdTableR[i][j]));
if (EEPROM_DEBUG_LEVEL >= 2) printf("[%d,%d]",j,vpdTableI[i][j]);
            }
        }
    }
    *pMinCalPower = (A_INT16)(minPwrT4[0] / 2);

    k = 0; /* index for the final table */
    for (i = 0; i < numXpdGains; i++) {
if (EEPROM_DEBUG_LEVEL >= 2) printf("\nstart stitching from %d:\n", i);
        if (i == (numXpdGains - 1)) {
            pPdGainBoundaries[i] = (A_UINT16)(maxPwrT4[i] / 2);
        } else {
            pPdGainBoundaries[i] = (A_UINT16)((maxPwrT4[i] + minPwrT4[i+1]) / 4);
        }

        pPdGainBoundaries[i] = (A_UINT16)A_MIN(AR9285_MAX_RATE_POWER, pPdGainBoundaries[i]);

        /*
         * WORKAROUND for 5416 1.0 until we get a per chain gain boundary register.
         * this is not the best solution
         */
        if ((i == 0) && !IS_MAC_5416_2_0_UP(pLibDev->macRev)) {
            //fix the gain delta, but get a delta that can be applied to min to
            //keep the upper power values accurate, don't think max needs to
            //be adjusted because should not be at that area of the table?
            minDelta = (A_INT16)(pPdGainBoundaries[0] - 23);
            pPdGainBoundaries[0] = 23;
        } else {
            minDelta = 0;
        }

        /* Find starting index for this pdGain */
        if (i == 0) {
            ss = 0; /* for the first pdGain, start from index 0 */
            //may need to extrapolate even for first power
            ss = (A_INT16)(0 - (minPwrT4[i] / 2));
        } else {
            ss = (A_INT16)((pPdGainBoundaries[i-1] - (minPwrT4[i] / 2)) - tPdGainOverlap + 1 + minDelta); // need overlap entries extrapolated below.
        }
        vpdStep = (A_INT16)(vpdTableI[i][1] - vpdTableI[i][0]);
        vpdStep = (A_INT16)((vpdStep < 1) ? 1 : vpdStep);
        /*
         *-ve ss indicates need to extrapolate data below for this pdGain
         */
        while ((ss < 0) && (k < (AR9285_NUM_PDADC_VALUES - 1))) {
            tmpVal = (A_INT16)(vpdTableI[i][0] + ss * vpdStep);
            pPDADCValues[k++] = (A_UCHAR)((tmpVal < 0) ? 0 : tmpVal);
            ss++;
if (EEPROM_DEBUG_LEVEL >= 2) printf("[%d,%d]", (ss-1), pPDADCValues[k-1]);
        }
if (EEPROM_DEBUG_LEVEL >= 2) printf("*");
        sizeCurrVpdTable = (A_UCHAR)((maxPwrT4[i] - minPwrT4[i]) / 2 + 1);
        tgtIndex = (A_UCHAR)(pPdGainBoundaries[i] + tPdGainOverlap - (minPwrT4[i] / 2));
        maxIndex = (tgtIndex < sizeCurrVpdTable) ? tgtIndex : sizeCurrVpdTable;

        while ((ss < maxIndex) && (k < (AR9285_NUM_PDADC_VALUES - 1))) {
            pPDADCValues[k++] = vpdTableI[i][ss++];
if (EEPROM_DEBUG_LEVEL >= 2) printf("[%d,%d]", (ss-1), pPDADCValues[k-1]);
        }
if (EEPROM_DEBUG_LEVEL >= 2) printf("*");
        vpdStep = (A_INT16)(vpdTableI[i][sizeCurrVpdTable - 1] - vpdTableI[i][sizeCurrVpdTable - 2]);
        vpdStep = (A_INT16)((vpdStep < 1) ? 1 : vpdStep);
        /*
         * for last gain, pdGainBoundary == Pmax_t2, so will
         * have to extrapolate
         */
        if (tgtIndex >= maxIndex) {  /* need to extrapolate above */
            while ((ss <= tgtIndex) && (k < (AR9285_NUM_PDADC_VALUES - 1))) {
                tmpVal = (A_INT16)((vpdTableI[i][sizeCurrVpdTable - 1] +
                          (ss - maxIndex + 1) * vpdStep));
                pPDADCValues[k++] = (A_UCHAR)((tmpVal > 255) ? 255 : tmpVal);
                ss++;
if (EEPROM_DEBUG_LEVEL >= 2) printf("[%d,%d]", (ss-1), pPDADCValues[k-1]);
            }
        }               /* extrapolated above */
if (EEPROM_DEBUG_LEVEL >= 2) printf("*");
    }                   /* for all pdGainUsed */

    /* Fill out pdGainBoundaries - only up to 2 allowed here, but hardware allows up to 4 */
    while (i < AR9285_PD_GAINS_IN_MASK) {
        pPdGainBoundaries[i] = pPdGainBoundaries[i-1];
        i++;
    }

    while (k < AR9285_NUM_PDADC_VALUES) {
        pPDADCValues[k] = pPDADCValues[k-1];
        k++;
    }
    return;
}

/**************************************************************
 * getLowerUppderIndex
 *
 * Return indices surrounding the value in sorted integer lists.
 * Requirement: the input list must be monotonically increasing
 *     and populated up to the list size
 * Returns: match is set if an index in the array matches exactly
 *     or a the target is before or after the range of the array.
 */
static A_BOOL
getLowerUpperIndex(A_UINT8 target, A_UINT8 *pList, A_UINT16 listSize,
                   A_UINT16 *indexL, A_UINT16 *indexR)
{
    A_UINT16 i;

    /*
     * Check first and last elements for beyond ordered array cases.
     */
    if (target <= pList[0]) {
        *indexL = *indexR = 0;
        return TRUE;
    }
    if (target >= pList[listSize-1]) {
        *indexL = *indexR = (A_UINT16)(listSize - 1);
        return TRUE;
    }

    /* look for value being near or between 2 values in list */
    for (i = 0; i < listSize - 1; i++) {
        /*
         * If value is close to the current value of the list
         * then target is not between values, it is one of the values
         */
        if (pList[i] == target) {
            *indexL = *indexR = i;
            return TRUE;
        }
        /*
         * Look for value being between current value and next value
         * if so return these 2 values
         */
        if (target < pList[i + 1]) {
            *indexL = i;
            *indexR = (A_UINT16)(i + 1);
            return FALSE;
        }
    }
    assert(0);
    return FALSE;
}

/**************************************************************
 * ar9285FillVpdTable
 *
 * Fill the Vpdlist for indices Pmax-Pmin
 * Note: pwrMin, pwrMax and Vpdlist are all in dBm * 4
 */
static A_BOOL
ar9285FillVpdTable(A_UINT8 pwrMin, A_UINT8 pwrMax, A_UINT8 *pPwrList,
                   A_UINT8 *pVpdList, A_UINT16 numIntercepts, A_UINT8 *pRetVpdList)
{
    A_UINT16  i, k;
    A_UINT8   currPwr = pwrMin;
    A_UINT16  idxL, idxR;
if (EEPROM_DEBUG_LEVEL >= 2) printf("\npwrmax = %d, pwrmin=%d\n", pwrMax, pwrMin);
    assert(pwrMax > pwrMin);
    for (i = 0; i <= (pwrMax - pwrMin) / 2; i++) {
        getLowerUpperIndex(currPwr, pPwrList, numIntercepts,
                           &(idxL), &(idxR));
        if (idxR < 1)
            idxR = 1;           /* extrapolate below */
        if (idxL == numIntercepts - 1)
            idxL = (A_UINT16)(numIntercepts - 2);   /* extrapolate above */
        if (pPwrList[idxL] == pPwrList[idxR])
            k = pVpdList[idxL];
        else
            k = (A_UINT16)( ((currPwr - pPwrList[idxL]) * pVpdList[idxR] + (pPwrList[idxR] - currPwr) * pVpdList[idxL]) /
                  (pPwrList[idxR] - pPwrList[idxL]) );
        assert(k < 256);
        pRetVpdList[i] = (A_UINT8)k;
        currPwr += 2;               /* half dB steps */
if (EEPROM_DEBUG_LEVEL >= 2) printf("[%d,%d]", i,pRetVpdList[i]);
    }
if (EEPROM_DEBUG_LEVEL >= 2) printf("\n");
    return TRUE;
}

/**************************************************************************
 * interpolate
 *
 * Returns signed interpolated or the scaled up interpolated value
 */
static A_INT16
interpolate(A_UINT16 target, A_UINT16 srcLeft, A_UINT16 srcRight,
            A_INT16 targetLeft, A_INT16 targetRight)
{
    A_INT16 rv;

    if (srcRight == srcLeft) {
        rv = targetLeft;
    } else {
        rv = (A_INT16)( ((target - srcLeft) * targetRight +
              (srcRight - target) * targetLeft) / (srcRight - srcLeft) );
    }
    return rv;
}

/**************************************************************************
 * fbin2freq
 *
 * Get channel value from binary representation held in eeprom
 * RETURNS: the frequency in MHz
 */
static A_UINT16
fbin2freq(A_UINT8 fbin, A_BOOL is2GHz)
{
    /*
     * Reserved value 0xFF provides an empty definition both as
     * an fbin and as a frequency - do not convert
     */
    if (fbin == AR9285_BCHAN_UNUSED) {
        return fbin;
    }

    return (A_UINT16)((is2GHz) ? (2300 + fbin) : (4800 + 5 * fbin));
}

void getAR9285Struct(
 A_UINT32 devNum,
 void **ppReturnStruct,     //return ptr to struct asked for
 A_UINT32 *pNumBytes        //return the size of the structure
)
{
    LIB_DEV_INFO *pLibDev = gLibInfo.pLibDevArray[devNum];
    if(pLibDev->ar9285Eep) {
		*ppReturnStruct = pLibDev->ar9285Eep;
		*pNumBytes = sizeof(ar9285_EEPROM);
	} else {
// #ifdef OWL_AP
#if defined(OWL_AP) && (!defined(OWL_OB42))
	    readEepromByteBasedOldStyle(devNum);
#else
		readEepromOldStyle(devNum);
#endif
		*ppReturnStruct = &dummyEepromWriteArea;
		*pNumBytes = sizeof(ar9285_EEPROM);
	}
    return;
}

void
ar9285GetPowerPerRateTable(A_UINT32 devNum, A_UINT16 freq, A_INT16 *pRatesPower)
{
    LIB_DEV_INFO  *pLibDev = gLibInfo.pLibDevArray[devNum];
    A_BOOL is2GHz = ((pLibDev->mode == MODE_11G) || (pLibDev->mode == MODE_11B));
    EEPROM_CHANNEL eChan;
    A_UINT16 cfgCtl = (A_UINT16)(pLibDev->ar9285Eep->baseEepHeader.regDmn[0] & 0xFF);
    A_UINT16 twiceAntennaReduction = (A_UINT16)((cfgCtl == 0x10) ? 6 : 0);
    A_UINT16 powerLimit = AR9285_MAX_RATE_POWER;

    eChan.is2GHz = is2GHz;
    eChan.synthCenter = (A_UINT16)freq;
    eChan.ht40Center = (A_UINT16)freq;
    eChan.isHt40 = (A_BOOL)pLibDev->libCfgParams.ht40_enable;
    eChan.txMask = (A_UINT16)pLibDev->ar9285Eep->baseEepHeader.txMask;
    if (eChan.isHt40) {
        int ctlDir;
        ctlDir = ((pLibDev->libCfgParams.extended_channel_op) ? (-1) : (1));
        eChan.ctlCenter = (A_UINT16)(freq + (ctlDir * 10));
        eChan.extCenter = (A_UINT16)(freq - (ctlDir * ((pLibDev->libCfgParams.extended_channel_sep == 20) ? (10) : (15))));
    } else {
        eChan.extCenter = (A_UINT16)freq;
        eChan.ctlCenter = (A_UINT16)freq;
    }

    switch (pLibDev->mode) {
    case MODE_11A:
        cfgCtl |= CTL_11A;
        break;
    case MODE_11G:
    case MODE_11O:
        cfgCtl |= CTL_11G;
        break;
    case MODE_11B:
        cfgCtl |= CTL_11B;
        break;
    default:
        assert(0);
    }

    assert(pRatesPower);
    ar9285SetPowerPerRateTable(pLibDev->ar9285Eep, &eChan, pRatesPower, cfgCtl, twiceAntennaReduction, powerLimit);
}


/**************************************************************************
 * ar9285VerifyPdadcTable
 *
 * Verify that the pdadc table is increasing with increased power.
 * RETURNS: TRUE if table is good, FALSE if its not
 */
static A_BOOL
ar9285VerifyPdadcTable(A_UINT32 devNum, Ar9285_CAL_DATA_PER_FREQ *pRawDataSet, A_UINT16 numXpdGains)
{
    LIB_DEV_INFO  *pLibDev = gLibInfo.pLibDevArray[devNum];
    A_UINT16 i, j;

    for(i = 0; i < numXpdGains; i++) {
        for (j = 1; j < AR9285_PD_GAIN_ICEPTS; j++) {
            if(pRawDataSet->vpdPdg[i][j] < pRawDataSet->vpdPdg[i][j-1]) {
                //bad pdadc table
                printf("SNOOP: pdadc table is not increasing\n");
                return FALSE;
            }
        }
        if (pRawDataSet->vpdPdg[i][0] >= pRawDataSet->vpdPdg[i][AR9285_PD_GAIN_ICEPTS-1]) {
            printf("SNOOP: low power pdadc is greater than high power pdadc\n");
            return FALSE;
        }

        if ((pRawDataSet->vpdPdg[i][AR9285_PD_GAIN_ICEPTS-1] - pRawDataSet->vpdPdg[i][0]) <
            (A_UINT8)pLibDev->libCfgParams.pdadcDelta) {
            printf("SNOOP: max/min pdadc delta is less than test limit\n");
            return FALSE;
        }
    }
    return (TRUE);
}


#ifdef OWL_AP
/**************************************************************************
 * fbin2freq
 *
 * Get channel value from binary representation held in eeprom
 * RETURNS: the frequency in MHz
 */
static A_UINT16
fbin2freq_gen8(A_UINT8 fbin, A_BOOL is2GHz)
{
    /*
     * Reserved value 0xFF provides an empty definition both as
     * an fbin and as a frequency - do not convert
     */
    if (fbin == AR9285_BCHAN_UNUSED) {
        return fbin;
    }

    return (A_UINT16)((is2GHz) ? (2300 + fbin) : (4800 + 5 * fbin));
}

/**************************************************************
 * ar9285EepromDump
 *
 * Produce a formatted dump of the EEPROM structure
 */
void ar9285EepromDump(A_UINT32 devNum, ar9285_EEPROM *ar9285Eep)
{
    A_UINT16          i, j, k, c, r;
    Ar9285_BASE_EEP_HEADER   *pBase = &(ar9285Eep->baseEepHeader);
    Ar9285_MODAL_EEP_HEADER  *pModal;
    CAL_TARGET_POWER_HT  *pPowerInfo = NULL;
    CAL_TARGET_POWER_LEG  *pPowerInfoLeg = NULL;
    A_BOOL  legacyPowers = FALSE;
    Ar9285_CAL_DATA_PER_FREQ *pDataPerChannel;
    A_BOOL            is2Ghz = 0;
    A_UINT8           noMoreSpurs;
    A_UINT8           *pChannel;
    A_UINT16          channelCount, channelRowCnt, vpdCount, rowEndsAtChan;
    A_UINT16          xpdGainMapping[] = {0, 1, 2, 4};
    A_UINT16          xpdGainValues[AR9285_NUM_PD_GAINS], numXpdGain = 0, xpdMask;
    A_UINT16          numPowers = 0, numRates = 0, ratePrint = 0, numChannels, tempFreq;
    A_UINT16          numTxChains = (A_UINT16)( ((pBase->txMask >> 2) & 1) +
        ((pBase->txMask >> 1) & 1) + (pBase->txMask & 1) );

    const static char *sRatePrint[3][8] = {
      {"     6-24     ", "      36      ", "      48      ", "      54      ", "", "", "", ""},
      {"       1      ", "       2      ", "     5.5      ", "      11      ", "", "", "", ""},
      {"   HT MCS0    ", "   HT MCS1    ", "   HT MCS2    ", "   HT MCS3    ",
       "   HT MCS4    ", "   HT MCS5    ", "   HT MCS6    ", "   HT MCS7    "},
    };

    const static char *sTargetPowerMode[7] = {
      "5G OFDM ", "5G HT20 ", "5G HT40 ", "2G CCK  ", "2G OFDM ", "2G HT20 ", "2G HT40 ",
    };

    const static char *sDeviceType[] = {
      "UNKNOWN [0] ",
      "Cardbus     ",
      "PCI         ",
      "MiniPCI     ",
      "Access Point",
      "PCIExpress  ",
      "UNKNOWN [6] ",
      "UNKNOWN [7] ",
    };

    const static char *sCtlType[] = {
        "[ 11A base mode ]",
        "[ 11B base mode ]",
        "[ 11G base mode ]",
        "[ INVALID       ]",
        "[ INVALID       ]",
        "[ 2G HT20 mode  ]",
        "[ 5G HT20 mode  ]",
        "[ 2G HT40 mode  ]",
        "[ 5G HT40 mode  ]",
    };

    assert(ar9285Eep);


    /* Print Header info */
    pBase = &(ar9285Eep->baseEepHeader);
    printf("\n");
    printf(" =======================Header Information======================\n");
    printf(" |  Major Version           %2d  |  Minor Version           %2d  |\n",
             pBase->version >> 12, pBase->version & 0xFFF);
    printf(" |-------------------------------------------------------------|\n");
    printf(" |  Checksum           0x%04X   |  Length             0x%04X   |\n",
             pBase->checksum, pBase->length);
    printf(" |  RegDomain 1        0x%04X   |  RegDomain 2        0x%04X   |\n",
             pBase->regDmn[0], pBase->regDmn[1]);
    printf(" |  MacAddress: 0x%02X:%02X:%02X:%02X:%02X:%02X                            |\n",
             pBase->macAddr[0], pBase->macAddr[1], pBase->macAddr[2],
             pBase->macAddr[3], pBase->macAddr[4], pBase->macAddr[5]);
    printf(" |  TX Mask            0x%04X   |  RX Mask            0x%04X   |\n",
             pBase->txMask, pBase->rxMask);
    printf(" |  OpFlags: 5GHz %d, 2GHz %d, Disable 5HT40 %d, Disable 2HT40 %d  |\n",
             (pBase->opCapFlags.opFlags & AR9285_OPFLAGS_11A) || 0, (pBase->opCapFlags.opFlags & AR9285_OPFLAGS_11G) || 0,
             (pBase->opCapFlags.opFlags & AR9285_OPFLAGS_5G_HT40) || 0, (pBase->opCapFlags.opFlags & AR9285_OPFLAGS_2G_HT40) || 0);
    printf(" |  OpFlags: Disable 5HT20 %d, Disable 2HT20 %d                  |\n",
             (pBase->opCapFlags.opFlags & AR9285_OPFLAGS_5G_HT20) || 0, (pBase->opCapFlags.opFlags & AR9285_OPFLAGS_2G_HT20) || 0);
    printf(" |  Misc: Big Endian %d                                         |\n",
             (pBase->opCapFlags.eepMisc & AR9285_EEPMISC_BIG_ENDIAN) || 0);
    printf(" |  Cal Bin Maj Ver %3d Cal Bin Min Ver %3d Cal Bin Build %3d  |\n",
             (pBase->binBuildNumber >> 24) & 0xFF,
             (pBase->binBuildNumber >> 16) & 0xFF,
             (pBase->binBuildNumber >> 8) & 0xFF);
    if ((pBase->version & AR9285_EEP_VER_MINOR_MASK) >= AR9285_EEP_MINOR_VER_3) {
        printf(" |  Device Type: %s                                  |\n",
                 sDeviceType[(pBase->deviceType & 0x7)]);
    }
    printf(" |  Customer Data in hex                                       |\n");	
	for (i = 0; i < AR9285_custdatasize; i++) {
        if ((i % 16) == 0) {
            printf(" |  = ");
        }
        printf("%02X ", ar9285Eep->custData[i]);
        if ((i % 16) == 15) {
            printf("        =|\n");
        }
    }
	if(AR9285_custdatasize%16 != 0)
	{
		printf("        =|\n");
	}

    /* Print Modal Header info */
    for (i = 0; i < 1; i++) {
        if ((i == 0) && !(pBase->opCapFlags.opFlags & AR9285_OPFLAGS_11G)) {
            continue;
        }
        pModal = &(ar9285Eep->modalHeader[i]);
        printf(" |======================2GHz Modal Header======================|\n");
        printf(" |  Ant Chain 0     0x%08lX  |\n",
                 pModal->antCtrlChain[0]);


		printf(" |  Ant Ctl Common  0x%08lX  |  Antenna Gain Chain 0   %3d  |\n",
                 pModal->antCtrlCommon, pModal->antennaGainCh[0]);


		printf(" |  Switch Settling        %3d  |  TxRxAttenuation Ch 0   %3d  |\n",
                 pModal->switchSettling, pModal->txRxAttenCh[0]);


		printf(" |  RxTxMargin Chain 0     %3d  |  \n",
                 pModal->rxTxMarginCh[0]);

		printf("   |  adc desired size       %3d  |\n",
                 pModal->adcDesiredSize);

		printf(" |  pga desired size       %3d  |  tx end to rx on        %3d  |\n",
                 pModal->pgaDesiredSize, pModal->txEndToRxOn);

		printf(" |  xlna gain Chain 0      %3d  |\n",
                 pModal->xlnaGainCh[0]);

		printf(" |   tx end to xpa off      %3d  |\n",
                  pModal->txEndToXpaOff);

		printf(" |  tx frame to xpa on     %3d  |  thresh62               %3d  |\n",
                 pModal->txFrameToXpaOn, pModal->thresh62);

		printf(" |  noise floor thres 0    %3d  |  noise floor thres 1    %3d  |\n",
                 pModal->noiseFloorThreshCh[0]);

		printf("  |  Xpd Gain Mask 0x%X | Xpd %2d  |\n",
                 pModal->xpdGain, pModal->xpd);

		printf(" |  IQ Cal I Chain 0       %3d  |  IQ Cal Q Chain 0       %3d  |\n",
                 pModal->iqCalICh[0], pModal->iqCalQCh[0]);


		printf(" |  pdGain Overlap     %2d.%d dB  |  Analog Output Bias (ob)%3d  |\n",
                 pModal->pdGainOverlap / 2, (pModal->pdGainOverlap % 2) * 5, pModal->ob);

		printf(" |  Analog Driver Bias (db)%3d  |  xpa bias level         %3d  |\n",
                 pModal->db, pModal->xpaBiasLvl); /* BAALNA */

		//printf(" |  pwr dec 2 chain    %2d.%d dB  |  pwr dec 3 chain    %2d.%d dB  |\n",
                 //pModal->pwrDecreaseFor2Chain / 2, (pModal->pwrDecreaseFor2Chain % 2) * 5,
                 //pModal->pwrDecreaseFor3Chain / 2, (pModal->pwrDecreaseFor3Chain % 2) * 5);

		if ((pBase->version & AR9285_EEP_VER_MINOR_MASK) >= AR9285_EEP_MINOR_VER_2) {
            printf(" |  txFrameToDataStart     %3d  |  txFrameToPaOn          %3d  |\n",
                    pModal->txFrameToDataStart, pModal->txFrameToPaOn);
            printf(" |  ht40PowerIncForPdadc   %3d  |  bswAtten Chain 0       %3d  |\n",
                    pModal->ht40PowerIncForPdadc, pModal->bswAtten[0]);
        }
        if ((pBase->version & AR9285_EEP_VER_MINOR_MASK) >= AR9285_EEP_MINOR_VER_3) {
            printf(" |  bswMargin Chain 0      %3d  |\n",
                    pModal->bswMargin[0]);
            printf(" |  bswMargin Chain 2      %3d  |  switch settling HT40   %3d  |\n",
                    pModal->bswMargin[2], pModal->swSettleHt40);
        }

		printf(" |  xatten2Db Chain 0      %3d  |\n",
				pModal->xatten2Db[0]);

		printf(" |  xatten2Margin Chain 0  %3d  |\n",
			 pModal->xatten2Margin[0]);

//		printf(" |  Anlg Out Bias Ch 1 (ob)%3d  |  Anlg Drv Bias Ch 1 (db)%3d  |\n",
//				 pModal->ob_ch1, pModal->db_ch1);

		//printf(" |  xlna buf mode            %3d  |  xlna i sel             %3d  |\n",
                 //(pModal->lna_cntl & 0x01), ((pModal->lna_cntl >> 1) & 0x03));

		//printf(" |  xlna buf in              %3d  |  femBandSelUsed         %3d  |\n",
                 //((pModal->lna_cntl >> 3) & 0x01), ((pModal->lna_cntl >> 4) & 0x01));

     //   printf(" |  local bias               %3d  |  force_xpaon            %3d  |\n",
       //          ((pModal->lna_cntl >> 5) & 0x01), ((pModal->lna_cntl >> 6) & 0x01));
    }
    printf(" |=============================================================|\n");

#define SPUR_TO_KHZ(is2GHz, spur)    ( ((spur) + ((is2GHz) ? 23000 : 49000)) * 100 )
#define NO_SPUR                      ( 0x8000 )

    /* Print spur data */
    printf("============================Spur Information===========================\n");
    for (i = 0; i < 1; i++) {
        if ((i == 0) && !(pBase->opCapFlags.opFlags & AR9285_OPFLAGS_11G)) {
            continue;
        }
        printf("| 11G Spurs in MHz (Range of 0 defaults to channel width)             |\n");
        pModal = &(ar9285Eep->modalHeader[i]);
        noMoreSpurs = 0;
        for (j = 0; j < AR9285_EEPROM_MODAL_SPURS; j++) {
            if ((pModal->spurChans[j].spurChan == NO_SPUR) || noMoreSpurs) {
                noMoreSpurs = 1;
                printf("|   NO SPUR   ");
            } else {
                printf("|    %4d.%1d   ", SPUR_TO_KHZ(i, pModal->spurChans[j].spurChan)/1000,
                         (SPUR_TO_KHZ(i, pModal->spurChans[j].spurChan)/100) % 10);
            }
        }
        printf("|\n");
        for (j = 0; j < AR9285_EEPROM_MODAL_SPURS; j++) {
            if ((pModal->spurChans[j].spurChan == NO_SPUR) || noMoreSpurs) {
                noMoreSpurs = 1;
                printf("|             ");
            } else {
                printf("|<%2d.%1d-=-%2d.%1d>",
                       pModal->spurChans[j].spurRangeLow / 10, pModal->spurChans[j].spurRangeLow % 10,
                       pModal->spurChans[j].spurRangeHigh / 10, pModal->spurChans[j].spurRangeHigh % 10);
            }
        }
        printf("|\n");
    }
    printf("|=====================================================================|\n");

#undef SPUR_TO_KHZ
#undef NO_SPUR

    /* Print calibration info */
    for (i = 0; i < 1; i++) {
        if ((i == 0) && !(pBase->opCapFlags.opFlags & AR9285_OPFLAGS_11G)) {
            continue;
        }
        for (c = 0; c < AR9285_MAX_CHAINS; c++) {
            if (!((pBase->txMask >> c) & 1)) {
                continue;
            }
            pDataPerChannel = ar9285Eep->calPierData2G[c];
            numChannels = AR9285_NUM_2G_CAL_PIERS;
            pChannel = ar9285Eep->calFreqPier2G;
            xpdMask = ar9285Eep->modalHeader[i].xpdGain;

            numXpdGain = 0;
            /* Calculate the value of xpdgains from the xpdGain Mask */
            for (j = 1; j <= AR9285_PD_GAINS_IN_MASK; j++) {
                if ((xpdMask >> (AR9285_PD_GAINS_IN_MASK - j)) & 1) {
                    if (numXpdGain >= AR9285_NUM_PD_GAINS) {
                        assert(0);
                        break;
                    }
                    xpdGainValues[numXpdGain++] = (A_UINT16)(AR9285_PD_GAINS_IN_MASK - j);
                }
            }

            printf("===============Power Calibration Information Chain %d =======================\n", c);
            for (channelRowCnt = 0; channelRowCnt < numChannels; channelRowCnt += 5) {
                rowEndsAtChan = (A_UINT16)A_MIN(numChannels, (channelRowCnt + 5));
                for (channelCount = channelRowCnt; channelCount < rowEndsAtChan; channelCount++) {
                    printf("|     %04d     ", fbin2freq_gen8(pChannel[channelCount], (A_BOOL)i));
                }
                printf("|\n|==============|==============|==============|==============|==============|\n");
                for (channelCount = channelRowCnt; channelCount < rowEndsAtChan; channelCount++) {
                    printf("|pdadc pwr(dBm)");
                }
                printf("|\n");

                for (k = 0; k < numXpdGain; k++) {
                    printf("|              |              |              |              |              |\n");
                    printf("| PD_Gain %2d   |              |              |              |              |\n",
                             xpdGainMapping[xpdGainValues[k]]);

                    for (vpdCount = 0; vpdCount < AR9285_PD_GAIN_ICEPTS; vpdCount++) {
                        for (channelCount = channelRowCnt; channelCount < rowEndsAtChan; channelCount++) {
                            printf("|  %02d   %2d.%02d  ", pDataPerChannel[channelCount].vpdPdg[k][vpdCount],
                                     pDataPerChannel[channelCount].pwrPdg[k][vpdCount] / 4,
                                     (pDataPerChannel[channelCount].pwrPdg[k][vpdCount] % 4) * 25);
                        }
                        printf("|\n");
                    }
                }
                printf("|              |              |              |              |              |\n");
                printf("|==============|==============|==============|==============|==============|\n");
            }
            printf("|\n");
        }
    }

    /* Print Target Powers */
    for (i = 3; i < 7; i++) {
        if ((i >=0 && i <=2) ) {
            continue;
        }
        if ((i >= 3 && i <=6) && !(pBase->opCapFlags.opFlags & AR9285_OPFLAGS_11G)) {
            continue;
        }
        switch(i) {
        case 0:
        case 1:
        case 2:
            break;
        case 3:
            pPowerInfo = (CAL_TARGET_POWER_HT *)ar9285Eep->calTargetPowerCck;
            pPowerInfoLeg = ar9285Eep->calTargetPowerCck;
            numPowers = AR9285_NUM_2G_CCK_TARGET_POWERS;
            ratePrint = 1;
            numRates = 4;
            is2Ghz = TRUE;
            legacyPowers = TRUE;
            break;
        case 4:
            pPowerInfo = (CAL_TARGET_POWER_HT *)ar9285Eep->calTargetPower2G;
            pPowerInfoLeg = ar9285Eep->calTargetPower2G;
            numPowers = AR9285_NUM_2G_20_TARGET_POWERS;
            ratePrint = 0;
            numRates = 4;
            is2Ghz = TRUE;
            legacyPowers = TRUE;
           break;
        case 5:
            pPowerInfo = ar9285Eep->calTargetPower2GHT20;
            numPowers = AR9285_NUM_2G_20_TARGET_POWERS;
            ratePrint = 2;
            numRates = 8;
            is2Ghz = TRUE;
            legacyPowers = FALSE;
            break;
        case 6:
            pPowerInfo = ar9285Eep->calTargetPower2GHT40;
            numPowers = AR9285_NUM_2G_40_TARGET_POWERS;
            ratePrint = 2;
            numRates = 8;
            is2Ghz = TRUE;
            legacyPowers = FALSE;
            break;
        }
        printf("============================Target Power Info===============================\n");
        for (j = 0; j < numPowers; j+=4) {
            printf("|   %s   ", sTargetPowerMode[i]);
            for (k = j; k < A_MIN(j + 4, numPowers); k++) {
                if(legacyPowers) {
                    printf("|     %04d     ",fbin2freq_gen8(pPowerInfoLeg[k].bChannel, (A_BOOL)i));
                } else {
                    printf("|     %04d     ",fbin2freq_gen8(pPowerInfo[k].bChannel, (A_BOOL)i));
                }
            }
            printf("|\n");
            printf("|==============|==============|==============|==============|==============|\n");
            for (r = 0; r < numRates; r++) {
                printf("|%s", sRatePrint[ratePrint][r]);
                for (k = j; k < A_MIN(j + 4, numPowers); k++) {
                    if(legacyPowers) {
                        printf("|     %2d.%d     ", pPowerInfoLeg[k].tPow2x[r] / 2,
                             (pPowerInfo[k].tPow2x[r] % 2) * 5);
                    } else {
                        printf("|     %2d.%d     ", pPowerInfo[k].tPow2x[r] / 2,
                             (pPowerInfo[k].tPow2x[r] % 2) * 5);
                    }
                }
                printf("|\n");
            }
            printf("|==============|==============|==============|==============|==============|\n");
        }
    }
    printf("\n");

    /* Print Band Edge Powers */
    printf("=======================Test Group Band Edge Power========================\n");
    for (i = 0; (ar9285Eep->ctlIndex[i] != 0) && (i < AR9285_NUM_CTLS); i++) {
        printf("|                                                                       |\n");
        printf("| CTL: 0x%02x %s                                           |\n",
                 ar9285Eep->ctlIndex[i], sCtlType[ar9285Eep->ctlIndex[i] & CTL_MODE_M]);
        printf("|=======|=======|=======|=======|=======|=======|=======|=======|=======|\n");

        for (c = 0; c < numTxChains; c++) {
            printf("======================== Edges for Chain %d ==============================\n", c);
            printf("| edge  ");
            for (j = 0; j < AR9285_NUM_BAND_EDGES; j++) {
                if (ar9285Eep->ctlData[i].ctlEdges[c][j].bChannel == AR9285_BCHAN_UNUSED) {
                    printf("|  --   ");
                } else {
                    tempFreq = fbin2freq_gen8(ar9285Eep->ctlData[i].ctlEdges[c][j].bChannel, is2Ghz);
                    printf("| %04d  ", tempFreq);
                }
            }
            printf("|\n");
            printf("|=======|=======|=======|=======|=======|=======|=======|=======|=======|\n");

            printf("| power ");
            for (j = 0; j < AR9285_NUM_BAND_EDGES; j++) {
                if (ar9285Eep->ctlData[i].ctlEdges[c][j].bChannel == AR9285_BCHAN_UNUSED) {
                    printf("|  --   ");
                } else {
                    printf("| %2d.%d  ", ar9285Eep->ctlData[i].ctlEdges[c][j].tPower / 2,
                        (ar9285Eep->ctlData[i].ctlEdges[c][j].tPower % 2) * 5);
                }
            }
            printf("|\n");
            printf("|=======|=======|=======|=======|=======|=======|=======|=======|=======|\n");

            printf("| flag  ");
            for (j = 0; j < AR9285_NUM_BAND_EDGES; j++) {
                if (ar9285Eep->ctlData[i].ctlEdges[c][j].bChannel == AR9285_BCHAN_UNUSED) {
                    printf("|  --   ");
                } else {
                    printf("|   %1d   ", ar9285Eep->ctlData[i].ctlEdges[c][j].flag);
                }
            }
            printf("|\n");
            printf("=========================================================================\n");
        }
    }

}
#endif

#undef N
#undef POW_SM

