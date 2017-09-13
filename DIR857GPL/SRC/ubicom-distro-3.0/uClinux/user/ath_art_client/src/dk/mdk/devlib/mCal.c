/* mCal.c - contains functions related to radio calibration */

/* Copyright (c) 2001 Atheros Communications, Inc., All Rights Reserved */

/* 
Revsision history
--------------------
1.0       Created.
*/

#ifdef __ATH_DJGPPDOS__
#define __int64	long long
typedef unsigned long DWORD;
#endif	// #ifdef __ATH_DJGPPDOS__

#include <errno.h>
#include "wlantype.h"
#include "mConfig.h"
#include "manlib.h"

#if defined(SPIRIT_AP) || defined(FREEDOM_AP)
#include "misclib.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

//++JC++
static AR2413_TXGAIN_TBL griffin_tx_gain_tbl[] =
{
#include  "AR2413_tx_gain.tbl"
} ;

static AR2413_TXGAIN_TBL spider_tx_gain_tbl[] =
{
#include  "spider2_0.tbl"
} ;

static AR2413_TXGAIN_TBL eagle_tx_gain_tbl_2[] =
{
#include  "ar5413_tx_gain_2.tbl"
} ;
static AR2413_TXGAIN_TBL eagle_tx_gain_tbl_5[] =
{
#include  "ar5413_tx_gain_5.tbl"
} ;
#define  AR2413_TX_GAIN_TBL_SIZE  26

static AR2413_TXGAIN_TBL owl_tx_gain_tbl_5[] =
{
#include  "ar5416_tx_gain_5.tbl"
} ;
#define  AR5416_TX_GAIN_TBL_5_SIZE  20

static AR2413_TXGAIN_TBL owl_tx_gain_tbl_2[] =
{
#include  "ar5416_tx_gain_2.tbl"
} ;
#define  AR5416_TX_GAIN_TBL_2_SIZE  19

static AR2413_TXGAIN_TBL nala_tx_gain_tbl[] = 
{
#include "nala.tbl"
};

#define  AR2413_TX_GAIN_TBL_SIZE  26
#define  MERLIN_TX_GAIN_TBL_SIZE  22
#define  TXGAIN_TABLE_STRING      "bb_tx_gain_table_"
#define  TXGAIN_TABLE_FILENAME    "merlin_tx_gain_2.tbl"

#define  TXGAIN_TABLE_TX1DBLOQGAIN_LSB           0
#define  TXGAIN_TABLE_TX1DBLOQGAIN_MASK          0x7
#define  TXGAIN_TABLE_TXV2IGAIN_LSB              3
#define  TXGAIN_TABLE_TXV2IGAIN_MASK             0x3
#define  TXGAIN_TABLE_PABUF5GN_LSB               5
#define  TXGAIN_TABLE_PABUF5GN_MASK              0x1
#define  TXGAIN_TABLE_PADRVGN_LSB                6
#define  TXGAIN_TABLE_PADRVGN_MASK               0x7
#define  TXGAIN_TABLE_PAOUT2GN_LSB               9
#define  TXGAIN_TABLE_PAOUT2GN_MASK              0x7
#define  TXGAIN_TABLE_GAININHALFDB_LSB           12
#define  TXGAIN_TABLE_GAININHALFDB_MASK          0x7F

//#define  CREATE_TXGAIN_TABLE_FILE   1

#define MAX_NUM_MODES 2
static MERLIN_TXGAIN_TBL merlin_tx_gain_table[MAX_NUM_MODES][MERLIN_TX_GAIN_TBL_SIZE];
//FJC - should really alloc this on the fly based on the numbers of modes asked for,
//however keep it static for now then don't need to worry about when to free.
//only 2 modes are anticipated right now.

static void ForceSinglePCDACTableMerlin(A_UINT32 devNum, A_UINT16 pcdac, A_UINT16 offset);

MANLIB_API A_BOOL generateTxGainTblFromCfg(A_UINT32 devNum, A_UINT32 modeMask)
{
    A_UINT32 i, j, val;
    A_CHAR fieldName[50];
    LIB_DEV_INFO *pLibDev = gLibInfo.pLibDevArray[devNum];
    A_UCHAR currentMode;
    A_UCHAR modes[MAX_NUM_MODES];
    A_UINT32 numModes = 0;
#if defined(CREATE_TXGAIN_TABLE_FILE)
    FILE *fd;
    char  *modeStr[] = {"11a", "11g", "11b"/*, "OFDM@2.4"*/}; 


    if ( NULL == (fd = fopen(TXGAIN_TABLE_FILENAME, "w")) ) {
        mError(devNum, EIO, "....Failed to open %s\n", TXGAIN_TABLE_FILENAME);
        return(FALSE);
    } 
#endif
    for (i = 0; modeMask; i++) {
        if (modeMask & 0x1) {
            modes[i] = (A_UCHAR)i;
            numModes++;
        }
        modeMask = modeMask >> 1;
        if (numModes == MAX_NUM_MODES) {
            break;
        }
    }

    for (j = 0; j < numModes; j++) {
        currentMode = modes[j];
        if (currentMode > MAX_NUM_MODES) {
            mError(devNum, EIO, "Illegal mode passed to generateTxGainTblFromCfg (%d), max mode is %d\n",
                currentMode, MAX_NUM_MODES);
            return(FALSE);
        }
#if defined(CREATE_TXGAIN_TABLE_FILE)
            fprintf(fd, "\n\nTx gain table for mode %s\n", modeStr[currentMode]);
#endif
        for (i=0; i<MERLIN_TX_GAIN_TBL_SIZE; i++) {
            sprintf(fieldName, "%s%d", TXGAIN_TABLE_STRING, i);      
            val = getFieldForMode(devNum, fieldName, currentMode, pLibDev->turbo);
            merlin_tx_gain_table[currentMode][i].txldBloqgain = (A_UCHAR)((val >> TXGAIN_TABLE_TX1DBLOQGAIN_LSB) & TXGAIN_TABLE_TX1DBLOQGAIN_MASK); 
            merlin_tx_gain_table[currentMode][i].txV2Igain = (A_UCHAR)((val >> TXGAIN_TABLE_TXV2IGAIN_LSB) & TXGAIN_TABLE_TXV2IGAIN_MASK); 
            merlin_tx_gain_table[currentMode][i].pabuf5gn = (A_UCHAR)((val >> TXGAIN_TABLE_PABUF5GN_LSB) & TXGAIN_TABLE_PABUF5GN_MASK); 
            merlin_tx_gain_table[currentMode][i].padrvgn = (A_UCHAR)((val >> TXGAIN_TABLE_PADRVGN_LSB) & TXGAIN_TABLE_PADRVGN_MASK); 
            merlin_tx_gain_table[currentMode][i].paout2gn = (A_UCHAR)((val >> TXGAIN_TABLE_PAOUT2GN_LSB) & TXGAIN_TABLE_PAOUT2GN_MASK); 
            merlin_tx_gain_table[currentMode][i].desired_gain = (A_UCHAR)((val >> TXGAIN_TABLE_GAININHALFDB_LSB) & TXGAIN_TABLE_GAININHALFDB_MASK); 

#if defined(CREATE_TXGAIN_TABLE_FILE)
            fprintf(fd, "%d   %d   %d   %d   %d   %d\n", merlin_tx_gain_table[currentMode][i].desired_gain, 
                merlin_tx_gain_table[currentMode][i].paout2gn, 
                merlin_tx_gain_table[currentMode][i].padrvgn, 
                merlin_tx_gain_table[currentMode][i].pabuf5gn, 
                merlin_tx_gain_table[currentMode][i].txV2Igain, 
                merlin_tx_gain_table[currentMode][i].txldBloqgain); 
#endif
        }
    }
    
#if defined(CREATE_TXGAIN_TABLE_FILE)
    fclose(fd);
#endif
    pLibDev->generatedMerlinGainTable = TRUE;
    return(TRUE);
}

static void ForceSinglePCDACTableMerlin(A_UINT32 devNum, A_UINT16 pcdac, A_UINT16 offset)
{
	A_UINT16 i;
	A_UINT32 dac_gain = 0;
	MERLIN_TXGAIN_TBL *pGainTbl = NULL;
   	LIB_DEV_INFO *pLibDev = gLibInfo.pLibDevArray[devNum];

    offset += 0;
#ifdef FJC_TEST    
    printf(".....ForceSinglePCDACTableMerlin pcdac:%d offset_1:%d\n ", pcdac, offset); 
#endif

    if(!pLibDev->generatedMerlinGainTable) {
        generateTxGainTblFromCfg(devNum, 0x3);
    }

    if(pLibDev->mode == MODE_11A) {
        if (offset > 20) {
	        offset -= 20;
	    }
    }		

    if(pLibDev->mode > MAX_NUM_MODES) {
        mError(devNum, EIO, "Illegal mode passed to ForceSinglePCDACTableMerlin (%d), max mode is %d\n",
            pLibDev->mode, MAX_NUM_MODES);
        return;
    }

    pGainTbl = merlin_tx_gain_table[pLibDev->mode];
	if(NULL == pGainTbl) {
        mError(devNum, EIO, "Error: unable to initialize gainTable in ForceSinglePCDACTableGriffin\n");
        return;
	}

	i = 0;
	if(offset != GAIN_OVERRIDE) {
	    pcdac = (A_UINT16)(pcdac + offset);		// Offset pcdac to get in a reasonable range
	}
	while ((pcdac > pGainTbl[i].desired_gain) && (i < (MERLIN_TX_GAIN_TBL_SIZE -1)) ) {i++;}  // Find entry closest
	
    if (pGainTbl[i].desired_gain > pcdac) {
        dac_gain = pGainTbl[i].desired_gain - pcdac;
	}
	writeField(devNum, "bb_force_dac_gain", 1);
	writeField(devNum, "bb_forced_dac_gain", dac_gain);
	writeField(devNum, "bb_force_tx_gain", 1);
	writeField(devNum, "bb_forced_paout2gn", pGainTbl[i].paout2gn);
	writeField(devNum, "bb_forced_padrvgn", pGainTbl[i].padrvgn);
	writeField(devNum, "bb_forced_txV2Igain", pGainTbl[i].txV2Igain);
	writeField(devNum, "bb_forced_txldBloqgain", pGainTbl[i].txldBloqgain);
    if (pLibDev->mode == MODE_11A) {
	    writeField(devNum, "bb_forced_pabuf5gn", pGainTbl[i].pabuf5gn);
    }
#ifdef FJC_TEST
    printf("...i:%d pcdac:%d offset:%d desiredGain:%d dac_gain:%d paout2gn:0x%x padrvgn:0x%x pabuf5gn:0x%x txV2Igain:0x%x txldBloqgain:0x%x \n",i, pcdac,
        offset, pGainTbl[i].desired_gain, dac_gain, pGainTbl[i].paout2gn, pGainTbl[i].padrvgn, pGainTbl[i].pabuf5gn, pGainTbl[i].txV2Igain, pGainTbl[i].txldBloqgain);
    //printf("0x%x\n", REGR(devNum, 0xa274)); 
//    getchar(); 
#endif
	return;
}



MANLIB_API void ForceSinglePCDACTableGriffin(A_UINT32 devNum, A_UINT16 pcdac, A_UINT16 offset)
{
	A_UINT16 i;
	A_UINT32 dac_gain = 0;
	AR2413_TXGAIN_TBL *pGainTbl = NULL;
	A_UINT32 gainTableSize = AR2413_TX_GAIN_TBL_SIZE;
	
   	LIB_DEV_INFO *pLibDev = gLibInfo.pLibDevArray[devNum];

    if(isMerlinPowerControl(pLibDev->swDevID)) {
        ForceSinglePCDACTableMerlin(devNum, pcdac, 0);
        return;
    }

    if(isSpider(pLibDev->swDevID)) {
		pGainTbl = spider_tx_gain_tbl;
		gainTableSize = 15;
		offset -= 20;
printf("SNOOP: using spider gain table\n");
	}
	else if(isGriffin(pLibDev->swDevID)) {
		if (isNala(pLibDev->swDevID))
		{
			pGainTbl = nala_tx_gain_tbl;
			gainTableSize = sizeof(nala_tx_gain_tbl) / sizeof(AR2413_TXGAIN_TBL);
		}
		else
		{
			pGainTbl = griffin_tx_gain_tbl;
			gainTableSize = sizeof(griffin_tx_gain_tbl)/sizeof(AR2413_TXGAIN_TBL);
		}
	}
	else if(isEagle(pLibDev->swDevID)) {
		if(pLibDev->mode == MODE_11A) {
			pGainTbl = eagle_tx_gain_tbl_5;
			offset += 10;
		} else {
			pGainTbl = eagle_tx_gain_tbl_2;
		}
	}
	else if(isOwl(pLibDev->swDevID)) {
		if(pLibDev->mode == MODE_11A) {
			pGainTbl = owl_tx_gain_tbl_5;
            gainTableSize = AR5416_TX_GAIN_TBL_5_SIZE;
//			offset += 10;
		} else {
			pGainTbl = owl_tx_gain_tbl_2;
            gainTableSize = AR5416_TX_GAIN_TBL_2_SIZE;
            if(offset != GAIN_OVERRIDE) {
			    offset += 10;
            }
		}
	}
	if(isDragon(devNum)) {
		offset += 20;
	}

	if(pGainTbl == NULL) {
		mError(devNum, EIO, "Error: unable to initialize gainTable in ForceSinglePCDACTableGriffin\n");
	}
	i = 0;
	if(offset != GAIN_OVERRIDE) {
		if (pLibDev->mode == 1) {
			offset = (A_UINT16)(offset + 10);	// Up the offset for 11b mode
		}
		pcdac = (A_UINT16)(pcdac + offset);		// Offset pcdac to get in a reasonable range
	}
	
	if(pcdac > pGainTbl[gainTableSize - 1].desired_gain) {
		i = (A_UINT16)(gainTableSize - 1);
	} else {
	  while ((pcdac > pGainTbl[i].desired_gain) &&
			(i < gainTableSize) ) {i++;}  // Find entry closest
	}

	if (pGainTbl[i].desired_gain > pcdac) {
		dac_gain = pGainTbl[i].desired_gain - pcdac;
	}	
	writeField(devNum, "bb_force_dac_gain", 1);
	writeField(devNum, "bb_forced_dac_gain", dac_gain);
	writeField(devNum, "bb_force_tx_gain", 1);
	writeField(devNum, "bb_forced_txgainbb1", pGainTbl[i].bb1_gain);
	writeField(devNum, "bb_forced_txgainbb2", pGainTbl[i].bb2_gain);
	writeField(devNum, "bb_forced_txgainif", pGainTbl[i].if_gain);
	writeField(devNum, "bb_forced_txgainrf", pGainTbl[i].rf_gain);

//   printf("\nSNOOP: offset = %d, i=%d, dac_gain = %d, bb1 = %d, bb2 = %d, gainif = %d, gainrf = %d\n",offset,i,
//   dac_gain, pGainTbl[i].bb1_gain, pGainTbl[i].bb2_gain, pGainTbl[i].if_gain,
//   pGainTbl[i].rf_gain);

#ifdef DEBUG_
    printf("SNOOP: dac_gain = %d, bb1 = %d, bb2 = %d, gainif = %d, gainrf = %d\n",
	   dac_gain, pGainTbl[i].bb1_gain, pGainTbl[i].bb2_gain, pGainTbl[i].if_gain,
	   pGainTbl[i].rf_gain);
#endif
	return;
}
//++JC++

MANLIB_API void ForceSinglePCDACTable(A_UINT32 devNum, A_UINT16 pcdac)
{
	A_UINT16 temp16, i;
	A_UINT32 temp32;
	A_UINT32 regoffset ;
	A_UINT32 pcdac_shift = 8;
	A_UINT32 predist_scale = 0x00FF;
	A_BOOL      falconTrue;

	falconTrue = isFalcon(devNum);

	if (falconTrue) {
		pcdac_shift = 0;
		predist_scale = 0; // no predist in falcon
	}

//++JC++
	LIB_DEV_INFO *pLibDev = gLibInfo.pLibDevArray[devNum];
	if(isGriffin(pLibDev->swDevID) || isEagle(pLibDev->swDevID) || isOwl(pLibDev->swDevID))  {    // For Griffin
		ForceSinglePCDACTableGriffin(devNum, pcdac, 30);  // By default, offset of 30
		return;
	}
//++JC++
	temp16 = (A_UINT16) (0x0000 | (pcdac << pcdac_shift) | predist_scale);
	temp32 = (temp16 << 16) | temp16 ;

	regoffset = 0x9800 + (608 << 2) ;
	for (i=0; i<32; i++)
	{
		REGW(devNum, regoffset, temp32);
		if (falconTrue) {
			REGW(devNum, regoffset + 0x1000, temp32);
		}
		regoffset += 4;
	}

	if (!falconTrue) {
		writeField(devNum, "rf_xpdbias", 1);
	}

	return;
}

/* Return the mac address of the mac specified 
   wmac = 0 (for ethernet) wmac = 1 (for wireless)
   instance number (used only for ethernet) to get the mac address, if more than one 
   ethernet mac is present.
   macAddr - buffer to return the mac address
 */
MANLIB_API void getMacAddr(A_UINT32 devNum, A_UINT16 wmac, A_UINT16 instNo, A_UINT8 *macAddr)
{
	A_UINT32 readVal;

	//verify some of the arguments
	if (checkDevNum(devNum) == FALSE) {
		mError(devNum, EINVAL, "Device Number %d:getMacAddr \n", devNum);
		return;
	}

	if (wmac > 1) {
		mError(devNum, EINVAL, "Device Number %d:getMacAddr: Invalid wmac argument \n", devNum);
		return;
	}

	if (macAddr == NULL) {
		mError(devNum, EINVAL, "Device Number %d:getMacAddr: Invalid buffer address for returning mac address \n", devNum);
		return;
	}

#ifndef MDK_AP
	// Client card can have only wmac 
/*
#ifndef PREDATOR_BUILD
	if (wmac == 0) {
		mError(devNum, EINVAL, "Device Number %d:getMacAddr: Client card can read only WMAC address \n", devNum);
		return;
	}
#endif
*/
			// Read the mac address from the eeprom
	readVal = eepromRead(devNum,0x1d);
	macAddr[0] = (A_UINT8)(readVal & 0xff);
	macAddr[1] = (A_UINT8)((readVal >> 8) & 0xff);
	readVal = eepromRead(devNum,0x1e);
	macAddr[2] = (A_UINT8)(readVal & 0xff);
	macAddr[3] = (A_UINT8)((readVal >> 8) & 0xff);
	readVal = eepromRead(devNum,0x1f);
	macAddr[4] = (A_UINT8)(readVal & 0xff);
	macAddr[5] = (A_UINT8)((readVal >> 8) & 0xff);

	instNo = 0; // referencing to remove warning
#endif

#ifdef SPIRIT_AP
	if (spiritGetMacAddr(devNum,wmac,instNo,macAddr) < 0) {
		mError(devNum, EIO, "Get mac address failed \n");
		return;
	}	
#endif // SPIRIT

#ifdef AP22_AP
	mError(devNum, EIO,"Get Mac Address not implemented for AP22 \n");
#endif // AP22

#ifdef FREEDOM_AP
	if (freedomGetMacAddr(devNum,wmac,instNo,macAddr) < 0) {
		mError(devNum, EIO, "Get mac address failed \n");
		return;
	}	
#endif // FREEDOM

#ifdef SENAO_AP
	mError(devNum, EIO,"Get Mac Address not implemented for senao \n");
#endif // SENAO


	return;
}

#ifdef __cplusplus
}
#endif /* __cplusplus */
