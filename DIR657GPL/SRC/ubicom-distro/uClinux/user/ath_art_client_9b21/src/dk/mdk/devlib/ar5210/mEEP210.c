/*
 *  Copyright © 2002 Atheros Communications, Inc.,  All Rights Reserved.
 */

#ident  "ACI $Id: //depot/sw/src/dk/mdk/devlib/ar5210/mEEP210.c#7 $, $Header: //depot/sw/src/dk/mdk/devlib/ar5210/mEEP210.c#7 $"

#ifdef VXWORKS
#include "vxworks.h"
#endif

#ifdef __ATH_DJGPPDOS__
#define __int64	long long
typedef unsigned long DWORD;
#define Sleep	delay
#endif	// #ifdef __ATH_DJGPPDOS__

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "wlantype.h"
#include "mEEP210.h"
#ifdef Linux
#include "../athreg.h"
#include "../manlib.h"
#include "../mEeprom.h"
#include "../mConfig.h"
#else
#include "..\athreg.h"
#include "..\manlib.h"
#include "..\mEeprom.h"
#include "..\mConfig.h"
#endif

#include "ar5210reg.h"



A_BOOL read5210eepData
(
 A_UINT32	devNum
)
{
    LIB_DEV_INFO *pLibDev = gLibInfo.pLibDevArray[devNum];
    A_UINT16 i, loc, checksum = 0;
    A_UINT16 eepVals[ATHEROS_EEPROM_ENTRIES];
    const A_INT32 magic_key = 0x5;
    TPC_MAP *pTpChannel;

	if(pLibDev->eepData.version == 0) {
		// Support of .9 & 1.0 calibrated cards
		// Read the power table out of EEPROM
		for (i=0; i < EEPROM_ENTRIES_FOR_TXPOWER; i++) {
			pLibDev->txPowerData.eepromPwrTable[i] = (A_UINT16) eepromRead(devNum, EEPROM_TXPOWER_OFFSET + i);
			if (i % 4 == 0) { // the first of every 4 reads has the "magic" value 5
				if(((pLibDev->txPowerData.eepromPwrTable[i] >> 13) & 0x7) != magic_key) {
#ifdef _DEBUG
					mError(devNum, EIO, "DEBUG - setupEEPromMap: EEPROM is not calibrated *or* invalid key\n");
#endif
					pLibDev->eepData.eepromChecked = TRUE;
					pLibDev->eepData.infoValid = FALSE;
					return FALSE;
				}
			}
		}
		pLibDev->eepData.infoValid = TRUE;
		pLibDev->eepData.eepromChecked = TRUE;
		return TRUE;
	}

	// Read the Atheros defined EEPROM entries
	for (i=0; i < ATHEROS_EEPROM_ENTRIES; i++) {
		eepVals[i] = (A_UINT16) eepromRead(devNum, ATHEROS_EEPROM_OFFSET + i);
		checksum ^= eepVals[i];
	}
	if(checksum != 0xFFFF) {
		mError(devNum, EIO, "setupEEPromMap: Invalid checksum found: 0x%04X\n", checksum);
		pLibDev->eepData.version = 0; // Disable Rev > 0 features
		pLibDev->eepData.eepromChecked = TRUE;
		pLibDev->eepData.infoValid = FALSE;
		return FALSE;
	}
	// If the checksum is considered valid - check the version and begin updating values
	pLibDev->eepData.currentRD = (A_UCHAR) (eepromRead(devNum, REGULATORY_DOMAIN_OFFSET) & 0xff);
	if( ((pLibDev->eepData.version >> 12) & 0xf) != 1 ) {
		mError(devNum, EIO, "setupEEPromMap: Invalid version found: 0x%04X\n", pLibDev->eepData.version);
		pLibDev->eepData.eepromChecked = TRUE;
		pLibDev->eepData.infoValid = FALSE;
		return FALSE;
	}
	pLibDev->eepData.antenna = eepVals[2];
	pLibDev->eepData.biasCurrents = eepVals[3];
	pLibDev->eepData.thresh62 = (A_UINT8) (eepVals[4] & 0xff);
	pLibDev->eepData.xlnaOn = (A_UINT8)((eepVals[4] >> 8) & 0xff);
	pLibDev->eepData.xpaOn = (A_UINT8)(eepVals[5] & 0xff);
	pLibDev->eepData.xpaOff = (A_UINT8)((eepVals[5] >> 8) & 0xff);
	pLibDev->eepData.regDomain[0] = (A_UINT8)((eepVals[6] >> 8) & 0xff);
	pLibDev->eepData.regDomain[1] = (A_UINT8)(eepVals[6] & 0xff);
	pLibDev->eepData.regDomain[2] = (A_UINT8)((eepVals[7] >> 8) & 0xff);
	pLibDev->eepData.regDomain[3] = (A_UINT8)(eepVals[7] & 0xff);
	pLibDev->eepData.rfKill = (A_UINT8)(eepVals[8] & 0x1);
	pLibDev->eepData.devType = (A_UINT8)((eepVals[8] >> 1) & 0x7);
	for(i = 0, loc = TP_SETTINGS_OFFSET; i < CHANNELS_SUPPORTED; i++, loc+=TP_SETTINGS_SIZE) {
		pTpChannel = &(pLibDev->eepData.tpc[i]);

		// Copy pcdac and gain_f values into EEPROM
		pTpChannel->pcdac[0] = (A_UINT8)((eepVals[loc] >> 10) & 0x3F); 
		pTpChannel->gainF[0] = (A_UINT8)((eepVals[loc] >> 4) & 0x3F);
		pTpChannel->pcdac[1] = (A_UINT8)( ((eepVals[loc] << 2) & 0x3C) | 
			((eepVals[loc+1] >> 14) & 0x03) );
		pTpChannel->gainF[1] = (A_UINT8)((eepVals[loc+1] >> 8) & 0x3F);
		pTpChannel->pcdac[2] = (A_UINT8)((eepVals[loc+1] >> 2) & 0x3F); 
		pTpChannel->gainF[2] = (A_UINT8)( ((eepVals[loc+1] << 4) & 0x30) |
			((eepVals[loc+2] >> 12) & 0x0F) );
		pTpChannel->pcdac[3] = (A_UINT8)((eepVals[loc+2] >> 6) & 0x3F); 
		pTpChannel->gainF[3] = (A_UINT8)(eepVals[loc+2] & 0x3F);
		pTpChannel->pcdac[4] = (A_UINT8)((eepVals[loc+3] >> 10) & 0x3F); 
		pTpChannel->gainF[4] = (A_UINT8)((eepVals[loc+3] >> 4) & 0x3F);
		pTpChannel->pcdac[5] = (A_UINT8)( ((eepVals[loc+3] << 2) & 0x3C) | 
			((eepVals[loc+4] >> 14) & 0x03) );
		pTpChannel->gainF[5] = (A_UINT8)((eepVals[loc+4] >> 8) & 0x3F);
		pTpChannel->pcdac[6] = (A_UINT8)((eepVals[loc+4] >> 2) & 0x3F); 
		pTpChannel->gainF[6] = (A_UINT8)( ((eepVals[loc+4] << 4) & 0x30) |
			((eepVals[loc+5] >> 12) & 0x0F) );
		pTpChannel->pcdac[7] = (A_UINT8)((eepVals[loc+5] >> 6) & 0x3F); 
		pTpChannel->gainF[7] = (A_UINT8)(eepVals[loc+5] & 0x3F);
		pTpChannel->pcdac[8] = (A_UINT8)((eepVals[loc+6] >> 10) & 0x3F); 
		pTpChannel->gainF[8] = (A_UINT8)((eepVals[loc+6] >> 4) & 0x3F);
		pTpChannel->pcdac[9] = (A_UINT8)( ((eepVals[loc+6] << 2) & 0x3C) | 
			((eepVals[loc+7] >> 14) & 0x03) );
		pTpChannel->gainF[9] = (A_UINT8)((eepVals[loc+7] >> 8) & 0x3F);
		pTpChannel->pcdac[10] = (A_UINT8)((eepVals[loc+7] >> 2) & 0x3F); 
		pTpChannel->gainF[10] = (A_UINT8)( ((eepVals[loc+7] << 4) & 0x30) |
			((eepVals[loc+8] >> 12) & 0x0F) );

		// Copy Regulatory Domain and Rate Information into EEPROM
		pTpChannel->rate36 = (A_UINT8)((eepVals[loc+8] >> 6) & 0x3F); 
		pTpChannel->rate48 = (A_UINT8)(eepVals[loc+8] & 0x3F);
		pTpChannel->rate54 = (A_UINT8)((eepVals[loc+9] >> 10) & 0x3F); 
		pTpChannel->regdmn[0] = (A_UINT8)((eepVals[loc+9] >> 4) & 0x3F);
		pTpChannel->regdmn[1] = (A_UINT8)( ((eepVals[loc+9] << 2) & 0x3C) | 
			((eepVals[loc+10] >> 14) & 0x03) );
		pTpChannel->regdmn[2] = (A_UINT8)((eepVals[loc+10] >> 8) & 0x3F);
		pTpChannel->regdmn[3] = (A_UINT8)((eepVals[loc+10] >> 2) & 0x3F); 
	}
	
	return TRUE;	
}


/**************************************************************************
* checkProm - Check the values read from the EEPROM for any violations 
*   - and optionally print the table and any violations.
*
* Returns: number of failures, zero for no failures
*/
//TO DO:  Printing should not be done in the library.  This needs to follow
//the ar5211 code and be pulled up from the library.
A_UINT32 check5210Prom
(
 A_UINT32 devNum,
 A_UINT32 enablePrint
)
{
    A_UINT32 c, i;
    LIB_DEV_INFO *pLibDev = gLibInfo.pLibDevArray[devNum];
    A_UINT32 eepFailures = 0;
    A_INT32 lowEntry, lastEntry; 
    TPC_MAP *pTpChannel;
    A_UINT32 ob, db;


    if(enablePrint) {
        printf("===================================EEPROM-Map==================================\n");
        printf(" Version - Major: %d Minor %d\n", (pLibDev->eepData.version >> 12) & 0xF,
            pLibDev->eepData.version & 0xFFF);
        printf(" RF Kill Implemented: %d  Device Type: %d\n", 
            pLibDev->eepData.rfKill, pLibDev->eepData.devType);
        printf(" Antenna - Switch Settling: %3d txrx atten: %2d Bridge Switch: %d D2 Control: %d\n",
    	    (pLibDev->eepData.antenna >> 8) &0x7F, (pLibDev->eepData.antenna >> 2) & 0x3F,
    	    (pLibDev->eepData.antenna >> 1) &0x1, pLibDev->eepData.antenna & 0x1);
        printf(" Transmit Power Bias Current Scaling - Output Bias (ob): %d Driver Bias (db): %d\n",
            (pLibDev->eepData.biasCurrents >> 4) & 0x7, pLibDev->eepData.biasCurrents & 0x7); 
        printf(" Threshold 62: %3d  tx_end to external LNA on: %3d\n",
            pLibDev->eepData.thresh62 & 0xFF, (pLibDev->eepData.xlnaOn >> 8) & 0xFF);
        printf(" tx_end to external PA off: %3d tx_frame to external PA on: %3d\n",  
            (pLibDev->eepData.xpaOff >> 8) & 0xFF, pLibDev->eepData.xpaOn & 0xFF);

        printf(" Regulatory Domains Calibrated into EEPROM\n"); 
        for(i = 0; i < REGULATORY_DOMAINS; i++) {
    	    printf(" RD %d: 0x%02X ", i+1, pLibDev->eepData.regDomain[i]);
        }
        printf("\n Currently Set Regulatory Domain: 0x%02X\n", pLibDev->eepData.currentRD);
        printf("=============================Transmit Power Table==============================\n");
	    printf(" Channel | 5.17--5.19 || 5.20--5.22 || 5.23--5.25 || 5.26--5.29 || 5.30--5.32 |\n");
        printf("=========|============||============||============||============||============|\n");
	    for(i = 0; i < TP_SCALING_ENTRIES; i++) {
		    printf(" %2d dBm  ", i * 2 + 3);
		    for(c = 0; c < CHANNELS_SUPPORTED; c++) {
			    printf("|pc %2d  gf %2d|", pLibDev->eepData.tpc[c].pcdac[i], pLibDev->eepData.tpc[c].gainF[i]);
		    }
		    printf("\n");
	    }
	    printf(" rate36  ");
	    for(c = 0; c < CHANNELS_SUPPORTED; c++) {
		    printf("|  pcdac %2d  |", pLibDev->eepData.tpc[c].rate36);
	    }
	    printf("\n");
	    printf(" rate48  ");
	    for(c = 0; c < CHANNELS_SUPPORTED; c++) {
		    printf("|  pcdac %2d  |", pLibDev->eepData.tpc[c].rate48);
	    }
	    printf("\n");
	    printf(" rate54  ");
	    for(c = 0; c < CHANNELS_SUPPORTED; c++) {
		    printf("|  pcdac %2d  |", pLibDev->eepData.tpc[c].rate54);
	    }
	    printf("\n");
	    for(i = 0; i < REGULATORY_DOMAINS; i++) {
		    printf(" RD %d    ", i+1);
		    for(c = 0; c < CHANNELS_SUPPORTED; c++) {
			    printf("|  pcdac %2d  |", pLibDev->eepData.tpc[c].regdmn[i]);
		    }
		    printf("\n");
	    }
        printf("=========|============||============||============||============||============|\n");
    }

    // Perform callibration checks
    for(i = 0; i < REGULATORY_DOMAINS; i++) {
   	    if(pLibDev->eepData.regDomain[i] == pLibDev->eepData.currentRD) break;
    }
    if(i == REGULATORY_DOMAINS) {
        if(enablePrint) printf("checkProm: Currently Set Regulator Domain: %d is not a callibrated RD\n",
            pLibDev->eepData.currentRD);
        eepFailures++;
    }

    for(c = 0; c < CHANNELS_SUPPORTED; c++) {
        lowEntry = -1;
        // Checks for valid entries
        pTpChannel = &(pLibDev->eepData.tpc[c]);
        // Find lowEntry
	    for(i = 0; i < TP_SCALING_ENTRIES; i++) {
            if(pTpChannel->pcdac[i] != 63) {
                lowEntry = i;
                break;
            }
        }
        if((lowEntry >= 2) && (lowEntry < TP_SCALING_ENTRIES)) {
            if(enablePrint) printf("checkProm: Channel Group (0-4): %d has its lowest entry above 6 dBm\n", c);
            eepFailures++;
        }
        else if(i == TP_SCALING_ENTRIES) {
            if(enablePrint) printf("checkProm: Channel Group (0-4): %d has no valid entries - stopping checks\n", c);
            return 1;
        }
        // Checks for orderliness of entries - increasing pcdac and gainf
        lastEntry = lowEntry;
	    for(i = lowEntry + 1; i < TP_SCALING_ENTRIES; i++) {
            if(pTpChannel->pcdac[i] == 63) {
                continue;
            }
            if(pTpChannel->pcdac[i] < pTpChannel->pcdac[lastEntry]) {
                if(enablePrint) printf("checkProm: Channel Group (0-4): %d has decreasing pcdac value from % d dBm to %d dBm\n",
                    c, I2DBM(lastEntry), I2DBM(i));
                eepFailures++;
            }
            if(pTpChannel->gainF[i] < pTpChannel->gainF[lastEntry]) {
                if(enablePrint) printf("checkProm: Channel Group (0-4): %d has decreasing gainF value from % d dBm to %d dBm\n",
                    c, I2DBM(lastEntry), I2DBM(i));
                eepFailures++;
            }
            lastEntry = i;
        }
    }

    ob = (pLibDev->eepData.biasCurrents >> 4) & 0x7;
    db = pLibDev->eepData.biasCurrents & 0x7;
    if( (ob < 1) || (ob > 5) ) {
        if(enablePrint) printf("initializeTransmitPower: OB should be between 1 and 5 inclusively, not %d\n", ob);
        eepFailures++;
    }
	if( (db < 2) || (db > 5) ) {
		if(enablePrint) printf("initializeTransmitPower: DB should be between 2 and 5 inclusively, not %d\n", db);
		eepFailures++;
	}

    return eepFailures;
}
