/** \file
 * COPYRIGHT (C) Atheros Communications Inc., 2009, All Rights Reserved
 *
 * - Filename    : mEep9287.C
 * - Description : eeprom routines for kiwi chipset
 * - $Id: //depot/sw/src/dk/mdk/devlib/ar9287/mEep9287.c#20 $
 * - $DateTime: 2009/09/08 15:27:00 $
 * 
 * - History :
 * - Date      Author     Modification                          
 ******************************************************************************/

#ifdef __ATH_DJGPPDOS__
    #define __int64 long long
    typedef unsigned long DWORD;
    #define Sleep   delay
#endif  // #ifdef __ATH_DJGPPDOS__


/****** Includes <> "" ********************************************************/
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef VXWORKS
  #include <malloc.h>
#endif
#include <assert.h>
#include "wlantype.h"
#include "mCfg5416.h"
#include "mEep5416.h"
#include "mEep9287.h"
#include "ar5416reg.h"
#include "mConfig.h"
#include "athreg.h"
#include "mIds.h"
#include "mEepStruct5416.h"
#include "mEepStruct9287.h"

#ifndef __ATH_DJGPPDOS__
    #include "endian_func.h"
#endif

/****** Definitions ***********************************************************/

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

/****** Macros ****************************************************************/

/****** Static declarations ***************************************************/
static A_UINT16 EEPROM_DEBUG_LEVEL = 0;
static A_UINT32 newEepromArea[sizeof(AR9287_EEPROM)];
static AR9287_EEPROM dummyEepromWriteArea;

/****** Externals/Globals  ****************************************************/
#ifndef __ATH_DJGPPDOS__
extern  void flash_read(A_UINT32 , A_UINT32, A_UINT8 *,A_UINT32 , A_UINT32 );
#endif

/****** Local function prototypes *********************************************/
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
ar9287SetPowerPerRateTable(AR9287_EEPROM *pEepData, EEPROM_CHANNEL *pEChan, A_INT16 *ratesArray,
                           A_UINT16 cfgCtl, A_UINT16 AntennaReduction, A_UINT16 powerLimit);

static A_UINT16
ar9287GetMaxEdgePower(A_UINT16 freq, CAL_CTL_EDGES *pRdEdgesPower, A_BOOL is2GHz);


static void
ar9287GetTargetPowers(EEPROM_CHANNEL *pEChan, CAL_TARGET_POWER_HT *powInfo,
                      A_UINT16 numChannels, CAL_TARGET_POWER_HT *pNewPower, A_UINT16 numRates, A_BOOL isHt40Target);

static void
ar9287GetTargetPowersLeg(EEPROM_CHANNEL *pEChan, CAL_TARGET_POWER_LEG *powInfo,
                      A_UINT16 numChannels, CAL_TARGET_POWER_LEG *pNewPower, A_UINT16 numRates, A_BOOL isExtTarget);

static void
ar9287SetPowerCalTable(A_UINT32 devNum, AR9287_EEPROM *pEepData, EEPROM_CHANNEL *pEChan, A_INT16 *pTxPowerIndexOffset);

void
ar9287GetPowerPerRateTable(A_UINT32 devNum, A_UINT16 freq, A_INT16 *pRatesPower);


#ifdef OWL_AP
static void ar9287EepromDump(A_UINT32 devNum, AR9287_EEPROM *ar9287Eep);
static A_UINT16 fbin2freq_gen8(A_UINT8 fbin, A_BOOL is2GHz);
#endif

#if (0) 
static void
ar9287GetGainBoundariesAndPdadcs(A_UINT32 devNum, EEPROM_CHANNEL *pEChan, Ar9287_CAL_DATA_PER_FREQ *pRawDataSet,
                                 A_UINT8 * bChans,  A_UINT16 availPiers,
                                 A_UINT16 tPdGainOverlap, A_INT16 *pMinCalPower, A_UINT16 * pPdGainBoundaries,
                                 A_UINT8 * pPDADCValues, A_UINT16 numXpdGains);
#endif

#if (0) 
static A_BOOL
ar9287FillVpdTable(A_UINT8 pMin, A_UINT8 pMax, A_UINT8 *pwrList,
                   A_UINT8 *vpdList, A_UINT16 numIntercepts, A_UINT8 *pRetVpdList);
#endif

#if (0) 
static A_BOOL
ar9287VerifyPdadcTable(A_UINT32 devNum, Ar9287_CAL_DATA_PER_FREQ *pRawDataSet, A_UINT16 numXpdGains);
#endif

static A_UINT16 readEepromOldStyle (
 A_UINT32 devNum
);

static A_UINT16 normalizeEeprom( void );


static void
ar9287GetTxGainIndex(
    EEPROM_CHANNEL* pEChan, 
    CAL_DATA_PER_FREQ_OP_LOOP* pRawDatasetOpLoop,
    A_UINT8* pCalChans,
    A_UINT16 availPiers,
    A_INT8*  pPwr);


static void
ar9287OpenLoopPowerControlGetPdadcs(A_UINT32 devNum,
									A_INT32  txPower,
                                    A_UINT16 chain);

/******************************************************************************/
/* LOCAL FUNCTION DEFINITIONS                                                 */
/******************************************************************************/

/******************************************************************************/
/* PUBLIC FUNCTION DEFINITIONS                                                */
/******************************************************************************/
/** 
 * - Function Name: ar9287EepRegister
 * - Description  :
 * - Arguments
 *     - 
 * - Returns      :                                                                
 *******************************************************************************/
void ar9287EepRegister( void ) {
    
}
/* ar9287EepRegister */

/**
 * - Function Name: ar9287EepromAttach
 * - Description  : 
 * - Arguments
 *     -      
 * - Returns      : 
 ******************************************************************************/
A_BOOL ar9287EepromAttach(A_UINT32 devNum)
{
    LIB_DEV_INFO *pLibDev = gLibInfo.pLibDevArray[devNum];
    A_UINT16 sum = 0;

#ifdef OWL_PB42
    A_UINT16 len = 0;
    A_UINT16 *pHalf;
    A_UINT32 i;
#endif

    pLibDev->ar9287Eep = &dummyEepromWriteArea;

#ifdef OWL_PB42
   if(isHowlAP(devNum) || isFlashCalData())
    {
     readEepromByteBasedOldStyle(devNum);
     len = pLibDev->ar9287Eep->baseEepHeader.length;
     pHalf = (A_UINT16 *)pLibDev->ar9287Eep;
     for (i = 0; i < (A_UINT32)(len / 2); i++) {
        sum ^= *pHalf++;
     }
    }
   else
    {
#endif

#if defined(OWL_AP) && (!defined(OWL_OB42))
    readEepromByteBasedOldStyle(devNum);

    len = pLibDev->ar9287Eep->baseEepHeader.length;

    pHalf = (A_UINT16 *)pLibDev->ar9287Eep;
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
        mError(devNum, EIO, "ar9287EepromAttach: bad EEPROM checksum 0x%x\n", sum);
        return FALSE;
    }

    // ar9287ShowEepSize(); /* debug */

    if ((pLibDev->ar9287Eep->baseEepHeader.version & AR9287_EEP_VER_MINOR_MASK) < AR9287_EEP_NO_BACK_VER) {
        mError(devNum, EIO, "ar9287EepromAttach: Incompatible EEPROM Version 0x%04X - recalibrate\n", pLibDev->ar9287Eep->baseEepHeader.version);
        return FALSE;
    }

    /* Setup some values checked by common code */
    pLibDev->eepData.version = pLibDev->ar9287Eep->baseEepHeader.version;
    pLibDev->eepData.protect = 0;
    pLibDev->eepData.turboDisabled = TRUE;
    pLibDev->eepData.currentRD = (A_UCHAR)pLibDev->ar9287Eep->baseEepHeader.regDmn[0];

    return TRUE;
}
/* ar9287EepromAttach */

/** 
 * - Function Name: ar9287DisplayEepSize
 * - Description  :
 * - Arguments
 *     - 
 * - Returns      :                                                                
 *******************************************************************************/
A_UINT32 ar9287DisplayEepSize( void ) {
    printf("Size of BASE_EEP_HEADER is 0x%x\n", sizeof(BASE_EEP_HEADER));
    printf("Size of MODAL_EEP_HEADER is 0x%x\n", sizeof(MODAL_EEP_HEADER));
    printf("Size of CAL_DATA_PER_FREQ is 0x%x\n", sizeof(CAL_DATA_PER_FREQ));
    printf("Size of CAL_TARGET_POWER_LEG is 0x%x\n", sizeof(CAL_TARGET_POWER_LEG));
    printf("Size of CAL_TARGET_POWER_HT is 0x%x\n", sizeof(CAL_TARGET_POWER_HT));
    printf("Size of AR9287_EEPROM is 0x%x\n", sizeof(AR9287_EEPROM));
    return (0);
}
/* ar9287DisplayEepSize */

/**
 * - Function Name: normalizeEeprom
 * - Description  : assume that the EEPROM is in little-endian format and 
 *                  normalize to our host architecture
 * - Arguments
 *    -      
 * - Returns      : 
 ******************************************************************************/
static A_UINT16 normalizeEeprom( void )
{
    AR9287_EEPROM *eeprom = &dummyEepromWriteArea;
    Ar9287_BASE_EEP_HEADER* base_hdr = &eeprom->baseEepHeader;
    Ar9287_MODAL_EEP_HEADER* modal_hdr;
    A_UINT16* eep_data_raw;
    A_UINT16  csum = 0;
    A_UINT32  num_words, offset;
    A_UINT32  i, j;

    /* convert the whole thing back to its native format. */
    num_words = sizeof(AR9287_EEPROM)/sizeof(A_UINT16);
    eep_data_raw = (A_UINT16 *)&dummyEepromWriteArea;
    for (offset = 0; offset < num_words; offset++, eep_data_raw++) {
        *eep_data_raw  = le2cpu16(*eep_data_raw);
    }

    /* compute crc while in eeprom native format */
    eep_data_raw = (A_UINT16 *)&dummyEepromWriteArea;
    num_words = le2cpu16(base_hdr->length)/2;

    for (i = 0; i < num_words; i++) {
    	// printf("i=%d  0x%04x\n", i, *eep_data_raw);
        csum ^= *eep_data_raw++;
    }

/***************************************/
/*     now do structure specific swaps */
/***************************************/

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

    /* convert Modal Eep header */ /* BAA */
#if (1)    
    modal_hdr = &eeprom->modalHeader[0];
    modal_hdr->antCtrlCommon = le2cpu32(modal_hdr->antCtrlCommon);

    for( j=0; j<AR9287_MAX_CHAINS; j++ ) {
        modal_hdr->antCtrlChain[j] =
        le2cpu32(modal_hdr->antCtrlChain[j]);
    }

    for( j=0; j<AR9287_EEPROM_MODAL_SPURS; j++ ) {
        modal_hdr->spurChans[j].spurChan =
        le2cpu16(modal_hdr->spurChans[j].spurChan);
    }
#else    
    for( i=0; i<2; i++ ) {
        modal_hdr = &eeprom->modalHeader[i];

        modal_hdr->antCtrlCommon = le2cpu32(modal_hdr->antCtrlCommon);

        for( j=0; j<AR9287_MAX_CHAINS; j++ ) {
            modal_hdr->antCtrlChain[j] =
            le2cpu32(modal_hdr->antCtrlChain[j]);
        }

        for( j=0; j<AR9287_EEPROM_MODAL_SPURS; j++ ) {
            modal_hdr->spurChans[j].spurChan =
            le2cpu16(modal_hdr->spurChans[j].spurChan);
        }
    }
#endif    

    return(csum);
}
/* normalizeEeprom */

/**
 * - Function Name: readEepromOldStyle
 * - Description  : read the eeprom contents
 * - Arguments
 *     -      
 * - Returns      : 
 ******************************************************************************/
A_UINT16 readEepromOldStyle (A_UINT32 devNum)
{
    A_UINT32  csum;
    A_UINT32  jj;
    A_UINT16* tempPtr = (A_UINT16 *)&dummyEepromWriteArea;

    eepromReadBlock(devNum, AR9287_EEP_START_LOC, sizeof(AR9287_EEPROM)/2, newEepromArea);
//    printf("TEST:: AR9287_EEPROM loc %x size %d\n", AR9287_EEP_START_LOC, sizeof(AR9287_EEPROM));
    for(jj=0; jj < sizeof(AR9287_EEPROM)/2; jj ++) {
        *(tempPtr+jj) = (A_UINT16)*(newEepromArea + jj);
        //printf("location[%x] = %x\n", jj, *(tempPtr+jj));
    }

    csum = normalizeEeprom();
    return (A_UINT16) csum;
}
/* readEepromOldStyle */

#if defined(OWL_AP)
void readEepromByteBasedOldStyle (
 A_UINT32 devNum
)
{
    A_UINT32 jj;
    LIB_DEV_INFO  *pLibDev = gLibInfo.pLibDevArray[devNum];
    A_UINT8 *tempPtr = (A_UINT8 *)&dummyEepromWriteArea;
#ifndef VXWORKS
       flash_read(devNum, FLC_RADIOCFG,tempPtr,sizeof(AR9287_EEPROM),(AR9287_EEP_START_LOC)<<1);
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

    for(jj=0; jj < sizeof(AR9287_EEPROM); jj ++) {
        *(tempPtr + jj) = sysFlashConfigRead(devNum,FLC_RADIOCFG,
                                             (AR9287_EEP_START_LOC << 1) + jj);
//              printf("location[%x] = %x\n", jj, *(tempPtr+jj));
    }
#ifndef ARCH_BIG_ENDIAN
	#ifdef RSP_BLOCKSWAP
	// RSP-ENDIAN - only for testing, disabled by default
	    for(jj=0; jj < sizeof(AR9287_EEPROM); jj+=2) {
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

/**
 * - Function Name: ar9287EepromSetBoardValues
 * - Description  : load eeprom header info into the device
 * - Arguments
 *     -      
 * - Returns      : 
 ******************************************************************************/
void
ar9287EepromSetBoardValues(A_UINT32 devNum, A_UINT32 freq)
{
    LIB_DEV_INFO  *pLibDev = gLibInfo.pLibDevArray[devNum];
    int is2GHz = ((pLibDev->mode == MODE_11G) || (pLibDev->mode == MODE_11B));
    Ar9287_MODAL_EEP_HEADER* pModal;
    Ar9287_BASE_EEP_HEADER*  pBase;
    A_UINT16 antWrites[AR9287_ANT_16S];
    A_CHAR *bank6names[] = {"rf_ob", "rf_b_ob", "rf_db", "rf_b_db"};
    A_UINT32 numBk6Params = sizeof(bank6names)/sizeof(bank6names[0]), b;
    PARSE_FIELD_INFO bank6Fields[sizeof(bank6names)/sizeof(bank6names[0])];
    CAL_DATA_PER_FREQ_U* calDataPerFreqOpLoop_ch0;
    CAL_DATA_PER_FREQ_U* calDataPerFreqOpLoop_ch1;
    int i, j, offset_num;
    A_UINT16 eepRev;

    /* get the eeprom rev */
    eepRev = (A_UINT16)(pLibDev->ar9287Eep->baseEepHeader.version & AR9287_EEP_VER_MINOR_MASK);

    freq = 0; // quiet compiler warning
    assert(((pLibDev->eepData.version >> 12) & 0xF) == AR9287_EEP_VER);
    pModal = &(pLibDev->ar9287Eep->modalHeader[0]);
    pBase  = &(pLibDev->ar9287Eep->baseEepHeader);
    calDataPerFreqOpLoop_ch0 = (CAL_DATA_PER_FREQ_U*)pLibDev->ar9287Eep->calPierData2G[0];
    calDataPerFreqOpLoop_ch1 = (CAL_DATA_PER_FREQ_U*)pLibDev->ar9287Eep->calPierData2G[1];

    /* Set the antenna register(s) correctly for the chip revision */
	antWrites[0] = (A_UINT16)((pModal->antCtrlCommon >> 28) & 0xF); /* ??? kiwi check this */
	antWrites[1] = (A_UINT16)((pModal->antCtrlCommon >> 24) & 0xF);
	antWrites[2] = (A_UINT16)((pModal->antCtrlCommon >> 20) & 0xF);
	antWrites[3] = (A_UINT16)((pModal->antCtrlCommon >> 16) & 0xF);
	antWrites[4] = (A_UINT16)((pModal->antCtrlCommon >> 12) & 0xF);
	antWrites[5] = (A_UINT16)((pModal->antCtrlCommon >> 8) & 0xF);
	antWrites[6] = (A_UINT16)((pModal->antCtrlCommon >> 4)  & 0xF);
	antWrites[7] = (A_UINT16)(pModal->antCtrlCommon & 0xF);

	offset_num = 8;

    for (i = 0, j = offset_num; i < AR9287_MAX_CHAINS; i++) {
		antWrites[j++] = (A_UINT16)((pModal->antCtrlChain[i] >> 28) & 0xf);
        antWrites[j++] = (A_UINT16)((pModal->antCtrlChain[i] >> 10) & 0x3);
        antWrites[j++] = (A_UINT16)((pModal->antCtrlChain[i] >> 8) & 0x3);
        antWrites[j++] = 0; //dummy placeholder this is missing for merlin
        antWrites[j++] = (A_UINT16)((pModal->antCtrlChain[i] >> 6) & 0x3);
        antWrites[j++] = (A_UINT16)((pModal->antCtrlChain[i] >> 4) & 0x3);
        antWrites[j++] = (A_UINT16)((pModal->antCtrlChain[i] >> 2) & 0x3);
        antWrites[j++] = (A_UINT16)(pModal->antCtrlChain[i] & 0x3);
    }
    assert (j <= AR9287_ANT_16S);

    forceAntennaTbl5416(devNum, antWrites); /* ??? kiwi check this */

    if( pLibDev->libCfgParams.ht40_enable ) {
        writeField(devNum, "bb_switch_settling", pModal->swSettleHt40);
    }
    else {
        writeField(devNum, "bb_switch_settling", pModal->switchSettling);
    }
    writeField(devNum, "bb_adc_desired_size", pModal->adcDesiredSize);
    writeField(devNum, "bb_tx_end_to_xpaa_off", pModal->txEndToXpaOff);
    writeField(devNum, "bb_tx_end_to_xpab_off", pModal->txEndToXpaOff);
    writeField(devNum, "bb_tx_frame_to_xpaa_on", pModal->txFrameToXpaOn);
    writeField(devNum, "bb_tx_frame_to_xpab_on", pModal->txFrameToXpaOn);
    writeField(devNum, "bb_tx_end_to_a2_rx_on", pModal->txEndToRxOn);
    writeField(devNum, "bb_thresh62", pModal->thresh62);

    REGW(devNum, PHY_CCA, (REGR(devNum, PHY_CCA_CH0) & ~PHY_CCA_THRESH62_M) | ((pModal->thresh62  << PHY_CCA_THRESH62_S) & PHY_CCA_THRESH62_M));

    writeField(devNum, "bb_thresh62_ext", pModal->thresh62);

    writeField(devNum, "bb_ch0_xatten1_hyst_margin", pModal->txRxAttenCh[0]);
    writeField(devNum, "bb_ch0_xatten2_hyst_margin", pModal->rxTxMarginCh[0]);
    writeField(devNum, "bb_ch1_xatten1_hyst_margin", pModal->txRxAttenCh[1]);
    writeField(devNum, "bb_ch1_xatten2_hyst_margin", pModal->rxTxMarginCh[1]);

    writeField(devNum, "bb_ch0_iqcorr_q_i_coff", pModal->iqCalICh[0]);
    writeField(devNum, "bb_ch0_iqcorr_q_q_coff", pModal->iqCalQCh[0]);
    writeField(devNum, "bb_ch0_iqcorr_enable", 1);
    writeField(devNum, "bb_ch1_iqcorr_q_i_coff", pModal->iqCalICh[1]);
    writeField(devNum, "bb_ch1_iqcorr_q_q_coff", pModal->iqCalQCh[1]);

    /* Write and update OB/DB */
    for (b = 0; b < numBk6Params; b++) {
        memcpy(bank6Fields[b].fieldName, bank6names[b], strlen(bank6names[b]) + 1);
        bank6Fields[b].valueString[1] = '\0';
    }

    /* apply ob/db, same for both chains */
    writeField(devNum, "an_ch0_rf2g3_db1",    pModal->db1);
    writeField(devNum, "an_ch0_rf2g3_db2",    pModal->db2);
    writeField(devNum, "an_ch0_rf2g3_ob_cck", pModal->ob_cck);
    writeField(devNum, "an_ch0_rf2g3_ob_psk", pModal->ob_psk);
    writeField(devNum, "an_ch0_rf2g3_ob_qam", pModal->ob_qam);
    writeField(devNum, "an_ch0_rf2g3_ob_pal_off", pModal->ob_pal_off);
    writeField(devNum, "an_ch1_rf2g3_db1",    pModal->db1);
    writeField(devNum, "an_ch1_rf2g3_db2",    pModal->db2);
    writeField(devNum, "an_ch1_rf2g3_ob_cck", pModal->ob_cck);
    writeField(devNum, "an_ch1_rf2g3_ob_psk", pModal->ob_psk);
    writeField(devNum, "an_ch1_rf2g3_ob_qam", pModal->ob_qam);
    writeField(devNum, "an_ch1_rf2g3_ob_pal_off", pModal->ob_pal_off);

    writeField(devNum, "bb_ch0_xatten1_margin", pModal->bswMargin[0]);
    writeField(devNum, "bb_ch0_xatten1_db", pModal->bswAtten[0]);
    writeField(devNum, "bb_ch1_xatten1_margin", pModal->bswMargin[1]);
    writeField(devNum, "bb_ch1_xatten1_db", pModal->bswAtten[1]);

    writeField(devNum, "bb_tx_frame_to_tx_d_start", pModal->txFrameToDataStart);
    writeField(devNum, "bb_tx_frame_to_pa_on", pModal->txFrameToPaOn);

    writeField(devNum, "an_top2_xpabias_lvl", pModal->xpaBiasLvl);

    /* get the spur channels */
    memcpy(pLibDev->libCfgParams.spurChans, 
           pModal->spurChans, 
           AR9287_EEPROM_MODAL_SPURS*sizeof(A_UINT32));

    /* load the spur channels, if any */
    for( i=0; i<5; i++ ) {
        /* check for list terminator */
        if( 0x8000 == pLibDev->libCfgParams.spurChans[i] ) {
            pLibDev->libCfgParams.spurChans[i] = 0;
            break;
        }
        /* decode the frequencies */
        pLibDev->libCfgParams.spurChans[i]=
        ( ((pLibDev->libCfgParams.spurChans[i]) + ((is2GHz) ? 23000 : 49000)) * 100 )/1000;
    }


    /* get the pdadc measured during calibration and stored in the eeprom */
    pLibDev->libCfgParams.calPdadc = calDataPerFreqOpLoop_ch0[0].calDataOpen.vpdPdg[0][0];
    pLibDev->libCfgParams.calPdadc_ch1 = calDataPerFreqOpLoop_ch1[0].calDataOpen.vpdPdg[0][0];
    //printf("calPdadc = %d, calPdadc_ch1 = %d\n", calDataPerFreqOpLoop_ch0->calDataOpen.vpdPdg[0][0], calDataPerFreqOpLoop_ch1[0].calDataOpen.vpdPdg[0][0]);

    /* load the open loop power control flag */
    pLibDev->libCfgParams.openLoopPwrCntl = pBase->openLoopPwrCntl & 0x01;

    /* load the pwr table offset */
    pLibDev->libCfgParams.pwrTableOffset = (A_INT32)pBase->pwrTableOffset;

    /* load the temperature sensor slope */
    if( eepRev >= AR9287_EEP_MINOR_VER_2 ) {
        pLibDev->libCfgParams.tempSensSlope = (A_INT32)pBase->tempSensSlope;
    }

    /* load the temperature sensor slope PAL ON */
    if( eepRev >= AR9287_EEP_MINOR_VER_3 ) {
        pLibDev->libCfgParams.tempSensSlopePalOn = (A_INT32)pBase->tempSensSlopePalOn;
    }

    /* load the PAL power threshold and PAL enable conditions */
    if( isKiwi1_1(pLibDev->macRev) && (eepRev >= AR9287_EEP_MINOR_VER_4) ) {
        writeField(devNum, "bb_pal_power_threshold", (pBase->palPwrThresh & 0x3f));
        writeField(devNum, "bb_enable_pal_2ch_HT40_QAM", pBase->palEnableModes & 0x01);
        writeField(devNum, "bb_enable_pal_2ch_HT20_QAM", pBase->palEnableModes & 0x02);
        writeField(devNum, "bb_enable_pal_1ch_HT40_QAM", pBase->palEnableModes & 0x04);
    }

#ifdef __ATH_DJGPPDOS__
    pLibDev->libCfgParams.rx_chain_mask = pBase->rxMask;
    pLibDev->libCfgParams.tx_chain_mask = pBase->txMask;
#endif

    pLibDev->eepromHeaderApplied[pLibDev->mode] = TRUE;
    return;
}
/* ar9287EepromSetBoardValues */

/**
 * - Function Name: ar9287EepromSetTransmitPower
 * - Description  : Set the transmit power in the baseband for the given
 *                  operating channel, mode and rate
 * - Arguments
 *     - devNum: device number
 *     - freq: operating channel (in MHz)
 *     - pRates: rate for which the power should be set. If null, set output
 *               power to target power.
 * - Returns      : none
 ******************************************************************************/
void
ar9287EepromSetTransmitPower(A_UINT32 devNum, A_UINT32 freq, A_UINT16 *pRates)
{
    LIB_DEV_INFO  *pLibDev = gLibInfo.pLibDevArray[devNum];
    A_BOOL is2GHz = ((pLibDev->mode == MODE_11G) || (pLibDev->mode == MODE_11B));
    Ar9287_MODAL_EEP_HEADER *pModal;
    EEPROM_CHANNEL pEChan;
    A_INT16 ratesArray[Ar5416RateSize];
    A_UINT16 powerLimit = AR9287_MAX_RATE_POWER;
    A_INT16  txPowerIndexOffset = 0;
    A_UINT16 cfgCtl = (A_UINT16)(pLibDev->ar9287Eep->baseEepHeader.regDmn[0] & 0xFF);
    A_UINT16 twiceAntennaReduction = (A_UINT16)((cfgCtl == 0x10) ? 6 : 0);
    A_UINT8  ht40PowerIncForPdadc = 2;
    int i;
    pModal = &(pLibDev->ar9287Eep->modalHeader[0]);

    pEChan.is2GHz = is2GHz;
    pEChan.synthCenter = (A_UINT16)freq;
    pEChan.ht40Center = (A_UINT16)freq;
    pEChan.isHt40 = (A_BOOL)pLibDev->libCfgParams.ht40_enable;
    pEChan.txMask = pLibDev->ar9287Eep->baseEepHeader.txMask;
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

    if ((pLibDev->ar9287Eep->baseEepHeader.version & AR9287_EEP_VER_MINOR_MASK) >= AR9287_EEP_MINOR_VER_2) {
        ht40PowerIncForPdadc = pModal->ht40PowerIncForPdadc;
    }

    /* Setup MIMO eepromChannel info */
    if (pRates) {
        memcpy(ratesArray, pRates, sizeof(A_UINT16) * Ar5416RateSize);
    }

    if(pLibDev->eePromLoad) {
        assert(((pLibDev->eepData.version >> 12) & 0xF) == AR9287_EEP_VER);
        cfgCtl = (A_UINT16)(pLibDev->ar9287Eep->baseEepHeader.regDmn[0] & 0xF0);

        /* set output power to target power from the calTgtPwr file */
        if (!pRates) {
            ar9287SetPowerPerRateTable(pLibDev->ar9287Eep, &pEChan, &ratesArray[0], cfgCtl, twiceAntennaReduction, powerLimit);
        }
        ar9287SetPowerCalTable(devNum, pLibDev->ar9287Eep, &pEChan, &txPowerIndexOffset);
    }

    /*
     * txPowerIndexOffset is set by the SetPowerTable() call -
     *  adjust the rate table (0 offset if rates EEPROM not loaded)
     */
    for (i = 0; i < N(ratesArray); i++) {
        ratesArray[i] = (A_INT16)(txPowerIndexOffset + ratesArray[i]);
        if (ratesArray[i] > AR9287_MAX_RATE_POWER)
            ratesArray[i] = AR9287_MAX_RATE_POWER;
    }
    pLibDev->pwrIndexOffset = txPowerIndexOffset;

    /* This factor applies to both closed and open loop power cntl */
    if(isMerlinPowerControl(pLibDev->swDevID)) {
        for (i = 0; i < Ar5416RateSize; i++) {
            ratesArray[i] = (A_UINT16)(ratesArray[i] - pLibDev->libCfgParams.pwrTableOffset * 2);
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
    if( pLibDev->libCfgParams.openLoopPwrCntl) {
		REGW(devNum, 0xa3CC,
	        POW_SM(ratesArray[rateHt40_3], 24)
		  | POW_SM(ratesArray[rateHt40_2], 16)
		  | POW_SM(ratesArray[rateHt40_1],  8)
		  |	POW_SM(ratesArray[rateHt40_0],  0)
		);
		REGW(devNum, 0xa3D0,
	        POW_SM(ratesArray[rateHt40_7], 24)
		  | POW_SM(ratesArray[rateHt40_6], 16)
		  | POW_SM(ratesArray[rateHt40_5],  8)
	      | POW_SM(ratesArray[rateHt40_4],  0)
		);
	} else { 
		// correct PAR difference between HT40 and HT20/LEGACY
		// only in close Loop Power control
		REGW(devNum, 0xa3CC,
	        POW_SM(ratesArray[rateHt40_3] + ht40PowerIncForPdadc, 24)
		  | POW_SM(ratesArray[rateHt40_2] + ht40PowerIncForPdadc, 16)
		  | POW_SM(ratesArray[rateHt40_1] + ht40PowerIncForPdadc,  8)
		  |	POW_SM(ratesArray[rateHt40_0] + ht40PowerIncForPdadc,  0)
		);
		REGW(devNum, 0xa3D0,
	        POW_SM(ratesArray[rateHt40_7] + ht40PowerIncForPdadc, 24)
		  | POW_SM(ratesArray[rateHt40_6] + ht40PowerIncForPdadc, 16)
		  | POW_SM(ratesArray[rateHt40_5] + ht40PowerIncForPdadc,  8)
	      | POW_SM(ratesArray[rateHt40_4] + ht40PowerIncForPdadc,  0)
		);
	}
	/* Write the Dup/Ext 40 power per rate set */
	REGW(devNum, 0xa3D4,
        POW_SM(ratesArray[rateExtOfdm], 24)
          | POW_SM(ratesArray[rateExtCck],  16)
          | POW_SM(ratesArray[rateDupOfdm],  8)
          | POW_SM(ratesArray[rateDupCck],   0)
    );

    return;
}
/* ar9287EepromSetTransmitPower */

/**
 * - Function Name: ar9287SetPowerPerRateTable
 * - Description  : Sets the transmit power in the baseband for the given 
 *                  operating channel and mode
 * - Arguments
 *     -      
 * - Returns      : 
 ******************************************************************************/
static void
ar9287SetPowerPerRateTable(AR9287_EEPROM*  pEepData,
                           EEPROM_CHANNEL* pEChan, 
                           A_INT16* ratesArray,
                           A_UINT16 cfgCtl,
                           A_UINT16 AntennaReduction,
                           A_UINT16 powerLimit)
{
/* Local defines to distinguish between extension and control CTL's */
#define EXT_ADDITIVE (0x8000)
#define CTL_11A_EXT (CTL_11A | EXT_ADDITIVE)
#define CTL_11G_EXT (CTL_11G | EXT_ADDITIVE)
#define CTL_11B_EXT (CTL_11B | EXT_ADDITIVE)
    A_UINT16 twiceMaxEdgePower = AR9287_MAX_RATE_POWER;
    A_UINT16 twiceMaxRDPower   = AR9287_MAX_RATE_POWER;
    int i;
    A_INT16  twiceAntennaReduction, largestAntenna;
    Ar9287_CAL_CTL_DATA *rep;
    CAL_TARGET_POWER_LEG targetPowerOfdm, targetPowerCck = {0, 0, 0, 0, 0};
    CAL_TARGET_POWER_LEG targetPowerOfdmExt = {0, 0, 0, 0, 0}, targetPowerCckExt = {0, 0, 0, 0, 0};
    CAL_TARGET_POWER_HT  targetPowerHt20, targetPowerHt40 = {0, 0, 0, 0, 0};
    A_INT16 scaledPower, minCtlPower;
//    A_UINT16 ctlModesFor11a[] = {CTL_11A, CTL_5GHT20, CTL_11A_EXT, CTL_5GHT40};
    A_UINT16 ctlModesFor11g[] = {CTL_11B, CTL_11G, CTL_2GHT20, CTL_11B_EXT, CTL_11G_EXT, CTL_2GHT40};
    A_UINT16 numCtlModes, *pCtlMode, ctlMode;
    A_UINT16 freq;

    /* Decrease the baseline target power with the regulatory maximums */
    twiceMaxRDPower = AR9287_MAX_RATE_POWER;

    /* Compute TxPower reduction due to Antenna Gain */
    largestAntenna = A_MAX(A_MAX(pEepData->modalHeader[pEChan->is2GHz].antennaGainCh[0], pEepData->modalHeader[pEChan->is2GHz].antennaGainCh[1]),
        pEepData->modalHeader[pEChan->is2GHz].antennaGainCh[0]);
    twiceAntennaReduction =  (A_INT16)A_MAX((AntennaReduction * 2) - largestAntenna, 0);

    /* scaledPower is the minimum of the user input power level and the regulatory allowed power level */
    scaledPower = (A_INT16)A_MIN(powerLimit, twiceMaxRDPower + twiceAntennaReduction);

    /* Get target powers from EEPROM - our baseline for TX Power */
    /* Setup for CTL modes */
    numCtlModes = N(ctlModesFor11g) - 3; // No ExtCck, ExtOfd, HT40
    pCtlMode = ctlModesFor11g;

    ar9287GetTargetPowersLeg(pEChan, pEepData->calTargetPowerCck,
                             AR9287_NUM_2G_CCK_TARGET_POWERS, &targetPowerCck, 4, FALSE);
    ar9287GetTargetPowersLeg(pEChan, pEepData->calTargetPower2G,
                             AR9287_NUM_2G_20_TARGET_POWERS, &targetPowerOfdm, 4, FALSE);
    ar9287GetTargetPowers(pEChan, pEepData->calTargetPower2GHT20,
                          AR9287_NUM_2G_20_TARGET_POWERS, &targetPowerHt20, 8, FALSE);
    if( pEChan->isHt40 ) {
        numCtlModes = N(ctlModesFor11g);
        ar9287GetTargetPowers(pEChan, pEepData->calTargetPower2GHT40,
                              AR9287_NUM_2G_40_TARGET_POWERS, &targetPowerHt40, 8, TRUE);
        ar9287GetTargetPowersLeg(pEChan, pEepData->calTargetPowerCck,
                                 AR9287_NUM_2G_CCK_TARGET_POWERS, &targetPowerCckExt, 4, TRUE);
        ar9287GetTargetPowersLeg(pEChan, pEepData->calTargetPower2G,
                                 AR9287_NUM_2G_20_TARGET_POWERS, &targetPowerOfdmExt, 4, TRUE);
    }

	//bug fix for 21429, don't apply the CTLs when in ART mode
	numCtlModes = 0;

    /* For MIMO, need to apply regulatory caps individually across dynamically running modes: CCK, OFDM, HT20, HT40 */
    for (ctlMode = 0; ctlMode < numCtlModes; ctlMode++) {
        A_BOOL isHt40CtlMode = (pCtlMode[ctlMode] == CTL_2GHT40);
        if (isHt40CtlMode) {
            freq = pEChan->ht40Center;
        } else if (pCtlMode[ctlMode] & EXT_ADDITIVE) {
            freq = pEChan->extCenter;
        } else {
            freq = pEChan->ctlCenter;
        }
        for (i = 0; (i < AR9287_NUM_CTLS) && pEepData->ctlIndex[i]; i++) {
            A_UINT16 twiceMinEdgePower;

            if (((cfgCtl | (pCtlMode[ctlMode] & CTL_MODE_M)) == pEepData->ctlIndex[i]) ||
                ((cfgCtl | (pCtlMode[ctlMode] & CTL_MODE_M))== ((pEepData->ctlIndex[i] & CTL_MODE_M) | SD_NO_CTL)))
            {
                rep = &(pEepData->ctlData[i]);
                twiceMinEdgePower = ar9287GetMaxEdgePower(freq, rep->ctlEdges[pEChan->activeTxChains - 1], pEChan->is2GHz);
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
/* ar9287SetPowerPerRateTable */

/**
 * - Function Name: get9287MaxPowerForRate
 * - Description  :
 * - Arguments
 *     -
 * - Returns      :
 ******************************************************************************/
A_INT16 get9287MaxPowerForRate(
 A_UINT32 devNum,
 A_UINT32 freq,
 A_UINT32 rate)
{
    A_INT16     ratesArray[Ar5416RateSize];
    A_UINT16        dataRate = 0;

    //get and set the max power values
    ar9287GetPowerPerRateTable(devNum, (A_UINT16)freq, ratesArray);

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
/* get9287MaxPowerForRate */

/**
 * - Function Name: ar9287GetMaxEdgePower
 * - Description  : Find the maximum conformance test limit for the given 
 *                  channel and CTL info
 * - Arguments
 *     -      
 * - Returns      : 
 ******************************************************************************/
static A_UINT16
ar9287GetMaxEdgePower(A_UINT16 freq, CAL_CTL_EDGES *pRdEdgesPower, A_BOOL is2GHz)
{
    A_UINT16 twiceMaxEdgePower = AR9287_MAX_RATE_POWER;
    int      i;

    /* Get the edge power */
    for (i = 0; (i < AR9287_NUM_BAND_EDGES) && (pRdEdgesPower[i].bChannel != AR9287_BCHAN_UNUSED) ; i++) {
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
/* ar9287GetMaxEdgePower */

/**
 * - Function Name: ar9287GetTargetPowers
 * - Description  : Return the rates of target power for the given target power
 *                  table channel, and number of channels
 * - Arguments
 *     -      
 * - Returns      : 
 ******************************************************************************/
static void
ar9287GetTargetPowers(EEPROM_CHANNEL *pEChan, CAL_TARGET_POWER_HT *powInfo,
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
        for (i = 0; (i < numChannels) && (powInfo[i].bChannel != AR9287_BCHAN_UNUSED); i++) {
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
/* ar9287GetTargetPowers */

/**
 * - Function Name: ar9287GetTargetPowersLeg
 * - Description  : Return the four rates of target power for the given target
 *                  power table channel, and number of channels
 * - Arguments
 *     -      
 * - Returns      : 
 ******************************************************************************/
static void
ar9287GetTargetPowersLeg(EEPROM_CHANNEL*       pEChan, 
                         CAL_TARGET_POWER_LEG* powInfo,
                         A_UINT16              numChannels, 
                         CAL_TARGET_POWER_LEG* pNewPower,
                         A_UINT16              numRates,
                         A_BOOL                isExtTarget)
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
        for (i = 0; (i < numChannels) && (powInfo[i].bChannel != AR9287_BCHAN_UNUSED); i++) {
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
/* ar9287GetTargetPowersLeg */

/**
 * - Function Name: ar9287SetPowerCalTable
 * - Description  : Pull the PDADC piers from cal data and interpolate them
 *                  across the given points as well as from the nearest pier(s) 
 *                  to get a power detector linear voltage to power level table
 * - Arguments
 *     -      
 * - Returns      : 
 ******************************************************************************/
static void
ar9287SetPowerCalTable(A_UINT32 devNum, AR9287_EEPROM *pEepData, EEPROM_CHANNEL *pEChan, A_INT16 *pTxPowerIndexOffset)
{
    LIB_DEV_INFO  *pLibDev = gLibInfo.pLibDevArray[devNum];
    Ar9287_CAL_DATA_PER_FREQ *pRawDataset = NULL;
    CAL_DATA_PER_FREQ_OP_LOOP* pRawDatasetOpLoop = NULL;
    A_UINT8  *pCalBChans = NULL;
    A_UINT16 pdGainOverlap_t2;
    static A_UINT8  pdadcValues[AR9287_NUM_PDADC_VALUES];
    A_UINT16 gainBoundaries[AR9287_PD_GAINS_IN_MASK];
    A_UINT16 numPiers, i, j;
    A_INT16  tMinCalPower = 0;
    A_UINT16 numXpdGain, xpdMask;
    A_UINT16 xpdGainValues[AR9287_NUM_PD_GAINS] = {0, 0, 0, 0};
    A_UINT32 reg32, regOffset, regChainOffset;
    A_INT16 firstPower_t2 = 0;
    A_UINT16 k, pdgainIndex = 0, powerIndex = 0;
    A_INT8 pwr;
    A_UINT16 diff = 0;
    A_UINT16 ii;

#ifdef WIN32
    A_CHAR logfileBuffer[256];
    FILE* filePtr = NULL;
#endif

    xpdMask = pEepData->modalHeader[0].xpdGain;

    if ((pLibDev->ar9287Eep->baseEepHeader.version & AR9287_EEP_VER_MINOR_MASK) >= AR9287_EEP_MINOR_VER_2) {
        pdGainOverlap_t2 = pEepData->modalHeader[0].pdGainOverlap;
    } else {
        pdGainOverlap_t2 = (A_UINT16)(REGR(devNum, TPCRG5_REG) & BB_PD_GAIN_OVERLAP_MASK);
    }

    pCalBChans = pEepData->calFreqPier2G;
    numPiers = AR9287_NUM_2G_CAL_PIERS;

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
    for (i = 1; i <= AR9287_PD_GAINS_IN_MASK; i++) {
        if ((xpdMask >> (AR9287_PD_GAINS_IN_MASK - i)) & 1) {
            if (numXpdGain >= AR9287_NUM_PD_GAINS) {
                assert(0);
                break;
            }
            xpdGainValues[numXpdGain] = (A_UINT16)(AR9287_PD_GAINS_IN_MASK - i);
            pLibDev->eepData.xpdGainValues[numXpdGain] = xpdGainValues[numXpdGain];
            numXpdGain++;
        }
    }
    pLibDev->eepData.numPdGain = numXpdGain;

    /* Write the detector gain biases and their number */
    REGW(devNum, TPCRG1_REG, (REGR(devNum, TPCRG1_REG) & 0xFFC03FFF) |
         (((numXpdGain - 1) & 0x3) << 14) | ((xpdGainValues[0] & 0x3) << 16) |
         ((xpdGainValues[1] & 0x3) << 18) | ((xpdGainValues[2] & 0x3) << 20));

    for (i = 0; i < AR9287_MAX_CHAINS; i++) {
        firstPower_t2 = *pTxPowerIndexOffset;
        pdgainIndex = 0;
        powerIndex = 0;
        regChainOffset = i * 0x1000;
        
        if (pEChan->txMask & (1 << i)) {
            if (pEChan->is2GHz) {
                if( pLibDev->libCfgParams.openLoopPwrCntl ) { /* open loop power control */
                    pRawDatasetOpLoop = (CAL_DATA_PER_FREQ_OP_LOOP*) pEepData->calPierData2G[i];
                }
                else {
                    pRawDataset = (Ar9287_CAL_DATA_PER_FREQ*) pEepData->calPierData2G[i];
                }
            }

if (EEPROM_DEBUG_LEVEL >= 2) printf("\nCalling GetGainBoundaries for chain %d (chan=%d)\n", i, pEChan->synthCenter);

            if( pLibDev->libCfgParams.openLoopPwrCntl ) { /* open loop power control */

                /* get power from eeprom stored during calibration */
                ar9287GetTxGainIndex( 
                    pEChan, 
                    pRawDatasetOpLoop, 
                    pCalBChans, 
                    numPiers,
                    &pwr);
                
                //printf(">>> i(%d), pwr(%d)\n", i, pwr);
                ar9287OpenLoopPowerControlGetPdadcs( 
                    devNum, 
                    (A_INT8)pwr, /* must cast as a signed number */
                    i);
            }
#if (0)             
            else {
                ar9287GetGainBoundariesAndPdadcs(devNum, pEChan, pRawDataset, pCalBChans, numPiers, pdGainOverlap_t2,
                                                 &tMinCalPower, gainBoundaries, pdadcValues, numXpdGain);
            }
#endif            

            /* Prior to writing the boundaries or the pdadc vs. power table into 
             * the chip registers the default starting point on the pdadc vs. 
             * power table needs to be checked and the curve boundaries adjusted 
             * accordingly
             */
            if( isMerlinPowerControl(pLibDev->swDevID) ) {
                A_UINT16 gainBoundLimit;

                if(AR9287_PWR_TABLE_OFFSET != pLibDev->libCfgParams.pwrTableOffset) {
                    /* get the difference in dB */
                    diff = (A_UINT16)(pLibDev->libCfgParams.pwrTableOffset - (A_INT32)AR9287_PWR_TABLE_OFFSET);
                    /* get the number of half dB steps */
                    diff *= 2; 

                    /* change the original gain boundary settings 
                     * by the number of half dB steps
                     */
                    for( ii = 0; ii < numXpdGain; ii++ ) {
                        //printf(">>> gainBoundaries[%d] = %d, ", ii,gainBoundaries[ii]);
                        gainBoundaries[ii] = (A_UINT16)(gainBoundaries[ii] - diff);
                        //printf("gainBoundaries[%d] = %d\n", ii,gainBoundaries[ii]);
                    }
                }

                /* Because of a hardware limitation, ensure the gain boundary 
                 * is not larger than (63 - overlap) 
                 */
                gainBoundLimit = (A_UINT16)((A_UINT16)AR9287_MAX_RATE_POWER - pdGainOverlap_t2);
                //printf(">>> gainBoundLimit: %d\n", gainBoundLimit);
                for( ii = 0; ii < numXpdGain; ii++ ) {
                    gainBoundaries[ii] = (A_UINT16)A_MIN(gainBoundLimit, gainBoundaries[ii]);
                    //printf(">>> gainBoundaries[%d]: %d\n", ii, gainBoundaries[ii]);
                }
            }

			if (i == 0) {
                /* Some items done only once */
                pLibDev->eepData.midPower = gainBoundaries[0];
                /*
                 * Note the pdadc table may not start at 0 dBm power, could be
                 * negative or greater than 0.  Need to offset the power
                 * values by the amount of minPower for griffin
                 */
                if(isMerlin(pLibDev->swDevID) || isKiwi(pLibDev->swDevID)) {
                    *pTxPowerIndexOffset = 0;
                } else {
                    if (tMinCalPower != 0) {
                        *pTxPowerIndexOffset = (A_INT16)(0 - tMinCalPower);
                    } else {
                        *pTxPowerIndexOffset = 0;
                    }
                }
            }

            if ((i == 0) || IS_MAC_5416_2_0_UP(pLibDev->macRev)) {

                /* per chain as of ar5416 v2 */
                if( (pLibDev->libCfgParams.openLoopPwrCntl) && !isKiwi(pLibDev->swDevID)) {
                    /* open loop power control */
                    REGW(devNum, 0xa26c + regChainOffset,
                         (0x6 & 0xf) |
                         (0x38 << 4)  |
                         (0x38 << 10) |
                         (0x38 << 16) |
                         (0x38 << 22));
                }
                else {

                    /* closed loop power control */
                    REGW(devNum, 0xa26c + regChainOffset,
                         (pdGainOverlap_t2 & 0xf) |
                         ((gainBoundaries[0] & 0x3f) << 4)  |
                         ((gainBoundaries[1] & 0x3f) << 10) |
                         ((gainBoundaries[2] & 0x3f) << 16) |
                         ((gainBoundaries[3] & 0x3f) << 22));
                }
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

            /* If this is a board that has a pwrTableOffset that differs from 
             * the default AR9287_PWR_TABLE_OFFSET then the start of the pdadc 
             * vs pwr table needs to be adjusted prior to writing to the chip.
             */
            if( isMerlinPowerControl(pLibDev->swDevID) ) {
                if( (A_INT32)AR9287_PWR_TABLE_OFFSET != pLibDev->libCfgParams.pwrTableOffset ) {

                    /* shift the table to start at the new offset */
                    for( ii = 0; ii<((A_UINT16)AR9287_NUM_PDADC_VALUES-diff); ii++ ) {
                        pdadcValues[ii] = pdadcValues[ii+diff];
                    }

                    /* fill the back of the table */
                    for( ii = (A_UINT16)(AR9287_NUM_PDADC_VALUES-diff); ii < AR9287_NUM_PDADC_VALUES; ii++ ) {
                        pdadcValues[ii] = pdadcValues[AR9287_NUM_PDADC_VALUES-diff];
                    }
                }
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
                                firstPower_t2 = (A_INT16)(gainBoundaries[pdgainIndex - 1] - pdGainOverlap_t2 - k); //compensate for where k is
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
    } /* for (i = 0; i < AR9287_MAX_CHAINS; i++) */

#ifdef WIN32
    //setup to log if needed
    if ( (pLibDev->libCfgParams.enablePdadcLogging) && (filePtr != NULL) ) {
        fclose(filePtr);
    }
#endif //WIN32
    return;
}
/* ar9287SetPowerCalTable */

#if (0) /* unused in kiwi */
/**
 * - Function Name: ar9287GetGainBoundariesAndPdadcs
 * - Description  : Uses the data points read from EEPROM to reconstruct the 
 *                  pdadc power table. Called by ar9287SetPowerCalTable only.
 * - Arguments
 *     -      
 * - Returns      : 
 ******************************************************************************/
static void
ar9287GetGainBoundariesAndPdadcs(A_UINT32        devNum, 
                                 EEPROM_CHANNEL* pEChan,
                                 Ar9287_CAL_DATA_PER_FREQ* pRawDataSet,
                                 A_UINT8*  bChans,
                                 A_UINT16  availPiers,
                                 A_UINT16  tPdGainOverlap,
                                 A_INT16*  pMinCalPower,
                                 A_UINT16* pPdGainBoundaries,
                                 A_UINT8*  pPDADCValues,
                                 A_UINT16  numXpdGains) {
    LIB_DEV_INFO  *pLibDev = gLibInfo.pLibDevArray[devNum];
    int       i, j, k;
    A_INT16   ss;         /* potentially -ve index for taking care of pdGainOverlap */
    A_UINT16  idxL, idxR, numPiers; /* Pier indexes */

    /* filled out Vpd table for all pdGains (chanL) */
    static A_UINT8   vpdTableL[AR9287_NUM_PD_GAINS][AR9287_MAX_PWR_RANGE_IN_HALF_DB];

    /* filled out Vpd table for all pdGains (chanR) */
    static A_UINT8   vpdTableR[AR9287_NUM_PD_GAINS][AR9287_MAX_PWR_RANGE_IN_HALF_DB];

    /* filled out Vpd table for all pdGains (interpolated) */
    static A_UINT8   vpdTableI[AR9287_NUM_PD_GAINS][AR9287_MAX_PWR_RANGE_IN_HALF_DB];

    A_UINT8   *pVpdL, *pVpdR, *pPwrL, *pPwrR;
    A_UINT8   minPwrT4[AR9287_NUM_PD_GAINS];
    A_UINT8   maxPwrT4[AR9287_NUM_PD_GAINS];
    A_INT16   vpdStep;
    A_INT16   tmpVal;
    A_UINT16  sizeCurrVpdTable, maxIndex, tgtIndex;
    A_BOOL    match;
    A_INT16  minDelta = 0;

    /* Trim numPiers for the number of populated channel Piers */
    for( numPiers = 0; numPiers < availPiers; numPiers++ ) {
        if( bChans[numPiers] == AR9287_BCHAN_UNUSED ) {
            break;
        }
    }

    if( pLibDev->libCfgParams.verifyPdadcTable ) {
        for( i = 0; i < numPiers; i++ ) {
            if( !ar9287VerifyPdadcTable(devNum, &pRawDataSet[i], numXpdGains) ) {
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
    /* Find where the current channel is in relation to the pier frequencies 
     * that were used during calibration
     */
    match = getLowerUpperIndex(
                              (A_UINT8)FREQ2FBIN(pEChan->synthCenter, pEChan->is2GHz),
                              bChans, 
                              numPiers, 
                              &idxL, 
                              &idxR);

    /* If channel is outside of the range of pier frequencies used during 
     * calibration or it is exactly one of those channels, then we have a match.
     * In the former case, use the "edge" pier frequency, in the latter case,
     * use the frequency that matches.
     */
    if( match ) {
        /* Directly fill both vpd tables from the matching index */
        for( i = 0; i < numXpdGains; i++ ) {
            minPwrT4[i] = pRawDataSet[idxL].pwrPdg[i][0];
            maxPwrT4[i] = pRawDataSet[idxL].pwrPdg[i][4];
            ar9287FillVpdTable(minPwrT4[i], maxPwrT4[i], pRawDataSet[idxL].pwrPdg[i],
                               pRawDataSet[idxL].vpdPdg[i], AR9287_PD_GAIN_ICEPTS, vpdTableI[i]);
        }
    }
    else {
        for( i = 0; i < numXpdGains; i++ ) {
            pVpdL = pRawDataSet[idxL].vpdPdg[i];
            pPwrL = pRawDataSet[idxL].pwrPdg[i];
            pVpdR = pRawDataSet[idxR].vpdPdg[i];
            pPwrR = pRawDataSet[idxR].pwrPdg[i];

            /* Start Vpd interpolation from the max of the minimum powers */
            minPwrT4[i] = A_MAX(pPwrL[0], pPwrR[0]);

            /* End Vpd interpolation from the min of the max powers */
            maxPwrT4[i] = A_MIN(pPwrL[AR9287_PD_GAIN_ICEPTS - 1], pPwrR[AR9287_PD_GAIN_ICEPTS - 1]);
            assert(maxPwrT4[i] > minPwrT4[i]);

            /* Fill pier Vpds */
            ar9287FillVpdTable(minPwrT4[i], maxPwrT4[i], pPwrL, pVpdL, AR9287_PD_GAIN_ICEPTS, vpdTableL[i]);
            ar9287FillVpdTable(minPwrT4[i], maxPwrT4[i], pPwrR, pVpdR, AR9287_PD_GAIN_ICEPTS, vpdTableR[i]);
            if( EEPROM_DEBUG_LEVEL >= 2 ) printf("\nfinal table for %d :\n", i);
            /* Interpolate the final vpd */
            for( j = 0; j <= (maxPwrT4[i] - minPwrT4[i]) / 2; j++ ) {
                vpdTableI[i][j] = (A_UINT8)(interpolate((A_UINT16)FREQ2FBIN(pEChan->synthCenter, pEChan->is2GHz),
                                                        bChans[idxL], bChans[idxR], vpdTableL[i][j], vpdTableR[i][j]));
                if( EEPROM_DEBUG_LEVEL >= 2 ) printf("[%d,%d]",j,vpdTableI[i][j]);
            }
        }
    }
    *pMinCalPower = (A_INT16)(minPwrT4[0] / 2);

    k = 0; /* index for the final table */
    for( i = 0; i < numXpdGains; i++ ) {
        if( EEPROM_DEBUG_LEVEL >= 2 ) printf("\nstart stitching from %d:\n", i);
        if( i == (numXpdGains - 1) ) {
            pPdGainBoundaries[i] = (A_UINT16)(maxPwrT4[i] / 2);
        }
        else {
            pPdGainBoundaries[i] = (A_UINT16)((maxPwrT4[i] + minPwrT4[i+1]) / 4);
        }

        pPdGainBoundaries[i] = (A_UINT16)A_MIN(AR9287_MAX_RATE_POWER, pPdGainBoundaries[i]);

#if (0)         
        /*
         * WORKAROUND for 5416 1.0 until we get a per chain gain boundary register.
         * this is not the best solution
         */
        if( (i == 0) && !IS_MAC_5416_2_0_UP(pLibDev->macRev) ) {
            //fix the gain delta, but get a delta that can be applied to min to
            //keep the upper power values accurate, don't think max needs to
            //be adjusted because should not be at that area of the table?
            minDelta = (A_INT16)(pPdGainBoundaries[0] - 23);
            pPdGainBoundaries[0] = 23;
        }
        else {
#endif        
            minDelta = 0;
#if (0)         
        }
#endif        

        /* Find starting index for this pdGain */
#if (0)         
        if( i == 0 ) {
            ss = 0; /* for the first pdGain, start from index 0 */
            if( isMerlinPowerControl(pLibDev->swDevID) ) {
                //may need to extrapolate even for first power
                ss = (A_INT16)(0 - (minPwrT4[i] / 2));
            }
        }
        else {
#endif        
            ss = (A_INT16)((pPdGainBoundaries[i-1] - (minPwrT4[i] / 2)) - tPdGainOverlap + 1 + minDelta); // need overlap entries extrapolated below.
#if (0)         
        }
#endif        
        vpdStep = (A_INT16)(vpdTableI[i][1] - vpdTableI[i][0]);
        vpdStep = (A_INT16)((vpdStep < 1) ? 1 : vpdStep);
        /*
         *-ve ss indicates need to extrapolate data below for this pdGain
         */
        while( (ss < 0) && (k < (AR9287_NUM_PDADC_VALUES - 1)) ) {
            tmpVal = (A_INT16)(vpdTableI[i][0] + ss * vpdStep);
            pPDADCValues[k++] = (A_UCHAR)((tmpVal < 0) ? 0 : tmpVal);
            ss++;
            if( EEPROM_DEBUG_LEVEL >= 2 ) printf("[%d,%d]", (ss-1), pPDADCValues[k-1]);
        }

        if( EEPROM_DEBUG_LEVEL >= 2 ) printf("*");
        sizeCurrVpdTable = (A_UCHAR)((maxPwrT4[i] - minPwrT4[i]) / 2 + 1);
        tgtIndex = (A_UCHAR)(pPdGainBoundaries[i] + tPdGainOverlap - (minPwrT4[i] / 2));
        maxIndex = (tgtIndex < sizeCurrVpdTable) ? tgtIndex : sizeCurrVpdTable;

        while( (ss < maxIndex) && (k < (AR9287_NUM_PDADC_VALUES - 1)) ) {
            pPDADCValues[k++] = vpdTableI[i][ss++];
            if( EEPROM_DEBUG_LEVEL >= 2 ) printf("[%d,%d]", (ss-1), pPDADCValues[k-1]);
        }
        if( EEPROM_DEBUG_LEVEL >= 2 ) printf("*");
        vpdStep = (A_INT16)(vpdTableI[i][sizeCurrVpdTable - 1] - vpdTableI[i][sizeCurrVpdTable - 2]);
        vpdStep = (A_INT16)((vpdStep < 1) ? 1 : vpdStep);
        /*
         * for last gain, pdGainBoundary == Pmax_t2, so will
         * have to extrapolate
         */
        if( tgtIndex >= maxIndex ) {  /* need to extrapolate above */
            while( (ss <= tgtIndex) && (k < (AR9287_NUM_PDADC_VALUES - 1)) ) {
                tmpVal = (A_INT16)((vpdTableI[i][sizeCurrVpdTable - 1] +
                                    (ss - maxIndex + 1) * vpdStep));
                pPDADCValues[k++] = (A_UCHAR)((tmpVal > 255) ? 255 : tmpVal);
                ss++;
                if( EEPROM_DEBUG_LEVEL >= 2 ) printf("[%d,%d]", (ss-1), pPDADCValues[k-1]);
            }
        }               /* extrapolated above */
        if( EEPROM_DEBUG_LEVEL >= 2 ) printf("*");
    }                   /* for all pdGainUsed */

    /* Fill out pdGainBoundaries - only up to 2 allowed here, but hardware allows up to 4 */
    while( i < AR9287_PD_GAINS_IN_MASK ) {
        pPdGainBoundaries[i] = pPdGainBoundaries[i-1];
        i++;
    }

    while( k < AR9287_NUM_PDADC_VALUES ) {
        pPDADCValues[k] = pPDADCValues[k-1];
        k++;
    }
    return;
}
/* ar9287GetGainBoundariesAndPdadcs */
#endif

/**
 * - Function Name: ar9287GetTxGainIndex
 * - Description  : When the card is calibrated using the open loop power 
 *                  control scheme, this routine retrieves the Tx gain table
 *                  index for the pcdac that was used to calibrate the board.
 * - Arguments
 *     -      
 * - Returns      : 
 ******************************************************************************/
#define KIWI_TX_GAIN_TABLE_SIZE (22)
static void ar9287GetTxGainIndex(
                                EEPROM_CHANNEL* pEChan,
                                CAL_DATA_PER_FREQ_OP_LOOP* pRawDatasetOpLoop,
                                A_UINT8* pCalChans,
                                A_UINT16 availPiers,
                                A_INT8*  pPwr)
{
    A_UINT16  idxL, idxR, numPiers; /* Pier indexes */
    A_BOOL match;

//    printf(">>> pCalChans: %d, %d, %d, numPiers: %d\n", pCalChans[0], pCalChans[1], pCalChans[2], numPiers); /* BBB */
//    printf(">>> ar9287GetTxGainIndex: chan(%d), is2Ghz(%d) ", pEChan->synthCenter, pEChan->is2GHz);  /* BBB */

    /* Get the number of populated channel Piers */
    for( numPiers = 0; numPiers < availPiers; numPiers++ ) {
        if( pCalChans[numPiers] == AR9287_BCHAN_UNUSED ) {
            break;
        }
    }

    /* Find where the current channel is in relation to the pier frequencies 
     * that were used during calibration
     */
    match = getLowerUpperIndex(
                              (A_UINT8)FREQ2FBIN(pEChan->synthCenter, pEChan->is2GHz),
                              pCalChans, 
                              numPiers, 
                              &idxL, 
                              &idxR);
// printf("!!!!!!!!!!!!!!!!!!!! idxL(%d), idxR(%d)\n", idxL, idxR); /* BBB */

    /* If channel is outside of the range of pier frequencies used during 
     * calibration or it is exactly one of those channels, then we have a match.
     * In the former case, use the "edge" pier frequency, in the latter case,
     * use the frequency that matches.
     * Retrieve the pcdac used to make the power measurements during 
     * calibration. It is the same for all channels so pick the first channel 
     * and get the pcdac.
     */
    if( match ) {
        /* Directly fill both vpd tables from the matching index */
        *pPwr = (A_INT8)pRawDatasetOpLoop[idxL].pwrPdg[0][0];
        //printf(">>>match: pcdac(%d), pwr(%d)\n", pcdac, *pPwr); /* BBB */
    }
    else {
        A_INT8 aL =  (A_INT8)pRawDatasetOpLoop[idxL].pwrPdg[0][0];
        A_INT8 aR =  (A_INT8)pRawDatasetOpLoop[idxR].pwrPdg[0][0];
        *pPwr = (A_INT8)((aL + aR)/2);
        //*pPwr = (A_INT8)((pRawDatasetOpLoop[idxL].pwrPdg[0][0] + pRawDatasetOpLoop[idxR].pwrPdg[0][0])/2);
        //printf("\n>>>idxL (%d), idxR (%d), aL (%d), aR(%d)\n", aL, aR);
        //printf(">>>NO match: pwr(%d)\n", *pPwr); /* BBB */
    }
}
/* ar9287GetTxGainIndex */

/**
 * - Function Name: ar9287OpenLoopPowerControlGetPdadcs
 * - Description  : PDADC vs Tx power table on open-loop power control mode
 * - Arguments
 *     -      
 * - Returns      : 
 ******************************************************************************/
static void
ar9287OpenLoopPowerControlGetPdadcs(A_UINT32 devNum,
									A_INT32  txPower, 
                                    A_UINT16 chain)
{
    A_UINT32 tmpVal;
	A_UINT32 a;

	// To enable open-loop power control
	tmpVal = REGR(devNum, 0xa270); // Chain 0
	tmpVal = tmpVal & 0xFCFFFFFF;
	tmpVal = tmpVal | (0x3 << 24); //bb_error_est_mode
	REGW(devNum, 0xa270, tmpVal);
//    printf("reg 0xa270 is 0x%x\n", REGR(devNum, 0xa270));
	tmpVal = REGR(devNum, 0xb270); // Chain 1
	tmpVal = tmpVal & 0xFCFFFFFF;
	tmpVal = tmpVal | (0x3 << 24); //bb_error_est_mode
	REGW(devNum, 0xb270, tmpVal);
//    printf("reg 0xb270 is 0x%x\n", REGR(devNum, 0xb270));

    /* write the olpc ref power for each chain */
    if( chain == 0 ) { // Chain 0
        tmpVal = REGR(devNum, 0xa398); 
        tmpVal = tmpVal & 0xff00ffff;
//        printf("Chain 0 txPower before %d\n", txPower);
        a = (txPower)&0xff; /* removed the signed part */
        //printf("a 0x%x\n", a);
        tmpVal = tmpVal | (a << 16);
        //printf("tmpVal 0x%x \n", tmpVal);
        REGW(devNum, 0xa398, tmpVal);
    }

    if( chain == 1 ) { // Chain 1
        tmpVal = REGR(devNum, 0xb398); 
        tmpVal = tmpVal & 0xff00ffff;
//        printf("Chain 1 txPower before %d\n", txPower);
        a = (txPower)&0xff; /* removed the signed part */
        //printf("a 0x%x\n", a);
        tmpVal = tmpVal | (a << 16);
        //printf("tmpVal 0x%x \n", tmpVal);
        REGW(devNum, 0xb398, tmpVal);
    }

#if (0)
    {
        A_UINT32 i;
        for( i=a; (a <= i) && (i <= b)/*AR5416_NUM_PDADC_VALUES*/; i++ ) {
            printf("pPDADCValues[%d] = %d\n", i, pPDADCValues[i]);
        }
    }
#endif    
}
/* ar9287OpenLoopPowerControlGetPdadcs */

/**************************************************************
 * getLowerUppderIndex
 * This is a generic function for finding whether a "target" is 
 * in a list (pList), and where it is with respect to the elements
 * of the list.
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
     * If this is the case, then use the edges
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
/* getLowerUpperIndex */

#if (0) 
/**************************************************************
 * ar9287FillVpdTable
 *
 * Fill the Vpdlist for indices Pmax-Pmin
 * Note: pwrMin, pwrMax and Vpdlist are all in dBm * 4
 */
static A_BOOL
ar9287FillVpdTable(A_UINT8 pwrMin, A_UINT8 pwrMax, A_UINT8 *pPwrList,
                   A_UINT8 *pVpdList, A_UINT16 numIntercepts, A_UINT8 *pRetVpdList) {
    A_UINT16  i, k;
    A_UINT8   currPwr = pwrMin;
    A_UINT16  idxL, idxR;
    if( EEPROM_DEBUG_LEVEL >= 2 ) printf("\npwrmax = %d, pwrmin=%d\n", pwrMax, pwrMin);
    assert(pwrMax > pwrMin);

    /* (pwrMax - pwrMin)/2 results in the number of half dB steps between pwrMax and
     * pwrMin since pwrMax and pwrMin are the actual power x4
     */
    for( i = 0; i <= (pwrMax - pwrMin) / 2; i++ ) {
        getLowerUpperIndex(currPwr, pPwrList, numIntercepts,
                           &(idxL), &(idxR));
        if( idxR < 1 )
            idxR = 1;           /* extrapolate below */
        if( idxL == numIntercepts - 1 )
            idxL = (A_UINT16)(numIntercepts - 2);   /* extrapolate above */
        if( pPwrList[idxL] == pPwrList[idxR] )
            k = pVpdList[idxL];
        else
            k = (A_UINT16)( ((currPwr - pPwrList[idxL]) * pVpdList[idxR] + (pPwrList[idxR] - currPwr) * pVpdList[idxL]) /
                            (pPwrList[idxR] - pPwrList[idxL]) );
        assert(k < 256);
        pRetVpdList[i] = (A_UINT8)k;
        currPwr += 2;               /* half dB steps */
        if( EEPROM_DEBUG_LEVEL >= 2 ) printf("[%d,%d]", i,pRetVpdList[i]);
    }
    if( EEPROM_DEBUG_LEVEL >= 2 ) printf("\n");
    return(TRUE);
}
/* ar9287FillVpdTable */
#endif

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
/* interpolate */

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
    if (fbin == AR9287_BCHAN_UNUSED) {
        return fbin;
    }

    return (A_UINT16)((is2GHz) ? (2300 + fbin) : (4800 + 5 * fbin));
}

void getAR9287Struct(
 A_UINT32 devNum,
 void **ppReturnStruct,     //return ptr to struct asked for
 A_UINT32 *pNumBytes        //return the size of the structure
)
{
    LIB_DEV_INFO *pLibDev = gLibInfo.pLibDevArray[devNum];  
    if(pLibDev->ar9287Eep) {
		*ppReturnStruct = pLibDev->ar9287Eep;
		*pNumBytes = sizeof(AR9287_EEPROM);
    } else {
#if defined(OWL_AP) && (!defined(OWL_OB42))
        readEepromByteBasedOldStyle(devNum);    
#else
        readEepromOldStyle(devNum);    
#endif
	    *ppReturnStruct = &dummyEepromWriteArea;
        *pNumBytes = sizeof(AR9287_EEPROM);
    }
    return;
}
/* fbin2freq */


/**
 * - Function Name: ar5416GetPowerPerRateTable
 * - Description  : 
 * - Arguments
 *     -      
 * - Returns      : 
 ******************************************************************************/
void
ar9287GetPowerPerRateTable(A_UINT32 devNum, A_UINT16 freq, A_INT16 *pRatesPower)
{
    LIB_DEV_INFO  *pLibDev = gLibInfo.pLibDevArray[devNum];
    A_BOOL is2GHz = ((pLibDev->mode == MODE_11G) || (pLibDev->mode == MODE_11B));
    EEPROM_CHANNEL eChan;
    A_UINT16 cfgCtl = (A_UINT16)(pLibDev->ar9287Eep->baseEepHeader.regDmn[0] & 0xFF);
    A_UINT16 twiceAntennaReduction = (A_UINT16)((cfgCtl == 0x10) ? 6 : 0);
    A_UINT16 powerLimit = AR9287_MAX_RATE_POWER;

    eChan.is2GHz = is2GHz;
    eChan.synthCenter = (A_UINT16)freq;
    eChan.ht40Center = (A_UINT16)freq;
    eChan.isHt40 = (A_BOOL)pLibDev->libCfgParams.ht40_enable;
    eChan.txMask = (A_UINT16)pLibDev->ar9287Eep->baseEepHeader.txMask;
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
    ar9287SetPowerPerRateTable(pLibDev->ar9287Eep, &eChan, pRatesPower, cfgCtl, twiceAntennaReduction, powerLimit);
}
/* ar9287GetPowerPerRateTable */


#if (0) 
/**************************************************************************
 * ar9287VerifyPdadcTable
 *
 * Verify that the pdadc table is increasing with increased power. The pdadc
 * readings stored in the eeprom should increase monotonically.  This routine 
 * ensures that what was loaded from the eeprom does this.
 * RETURNS: TRUE if table is good, FALSE if its not
 */
static A_BOOL
ar9287VerifyPdadcTable(A_UINT32 devNum, Ar9287_CAL_DATA_PER_FREQ *pRawDataSet, A_UINT16 numXpdGains) {
    LIB_DEV_INFO  *pLibDev = gLibInfo.pLibDevArray[devNum];
    A_UINT16 i, j;

    for( i = 0; i < numXpdGains; i++ ) {
        for( j = 1; j < AR9287_PD_GAIN_ICEPTS; j++ ) {
            if( pRawDataSet->vpdPdg[i][j] < pRawDataSet->vpdPdg[i][j-1] ) {
                //bad pdadc table
                printf("SNOOP: pdadc table is not increasing\n");
                return(FALSE);
            }
        }
        if( pRawDataSet->vpdPdg[i][0] >= pRawDataSet->vpdPdg[i][AR9287_PD_GAIN_ICEPTS-1] ) {
            printf("SNOOP: low power pdadc is greater than high power pdadc\n");
            return(FALSE);
        }

        if( (pRawDataSet->vpdPdg[i][AR9287_PD_GAIN_ICEPTS-1] - pRawDataSet->vpdPdg[i][0]) <
            (A_INT32)pLibDev->libCfgParams.pdadcDelta ) {
            printf("SNOOP: max/min pdadc delta is less than test limit\n");
            return(FALSE);
        }
    }
    return(TRUE);
}
/* ar9287VerifyPdadcTable */
#endif


#undef N
#undef POW_SM







