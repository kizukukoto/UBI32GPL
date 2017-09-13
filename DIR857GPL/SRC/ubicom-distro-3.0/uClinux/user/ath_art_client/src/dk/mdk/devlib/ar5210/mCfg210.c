/*
 *  Copyright © 2002 Atheros Communications, Inc.,  All Rights Reserved.
 */

#ident  "ACI $Id: //depot/sw/src/dk/mdk/devlib/ar5210/mCfg210.c#13 $, $Header: //depot/sw/src/dk/mdk/devlib/ar5210/mCfg210.c#13 $"

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
#include <string.h>
#include "wlantype.h"
#include "mCfg210.h"
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

//local functions
static void    setupPowerSettings(A_UINT32 devNum, A_UINT32 channelIEEE, A_UCHAR  cp[17]);
static A_UCHAR getGainF(A_UINT32 devNum, TPC_MAP *pRD, A_UCHAR pcdac, A_UCHAR *dBm);
static A_UCHAR getPCDAC(A_UINT32 devNum, TPC_MAP *pRD, A_UCHAR dBm);

void hwResetAr5210
(
 A_UINT32 devNum,
 A_UINT32 resetMask
)
{
	A_UINT32 reg;

	/* Bring out of sleep mode */
	REGW(devNum, F2_SCR, F2_SCR_SLE_FWAKE);
	mSleep(10);

	reg = 0;
	if (resetMask & MAC_RESET) {
		reg = reg | F2_RC_PCU | F2_RC_DMA | F2_RC_F2 ; 
	}
	if (resetMask & BB_RESET) {
		reg = reg | F2_RC_D2; 
	}
	if (resetMask & BB_RESET) {
		reg = reg | F2_RC_PCI; 
	}

	REGW(devNum, F2_RC, reg);
	mSleep(10);

	if(REGR(devNum, F2_RC) == 0) {
		mError(devNum, EIO, "hwResetAr5211: Device did not enter Reset \n");
		return;
	}

	/* Bring out of sleep mode again */
   	REGW(devNum, F2_SCR, F2_SCR_SLE_FWAKE);
	mSleep(10);

	/* Clear the reset */
	REGW(devNum, F2_RC, 0);
	mSleep(10);

	return;
}

// add the pll programming for crete 
void pllProgramAr5210
(
 	A_UINT32 devNum,
 	A_UINT32 turbo
)
{
	devNum = 0;	// access to remove compile warnings
	turbo = 0;	// access to remove compile warnings
	return;
}


/**************************************************************************
 * setAR5000Channel - Perform the algorithm to change the channel 
 *				      for AR5000 adapters
 *
 */
A_BOOL setChannelAr5210
(
 A_UINT32 devNum,
 A_UINT32 freq		// New channel
)
{
	A_UINT32 regVal;
	A_UINT32 channelIEEE = (freq - 5000) / 5;

    regVal = reverseBits((channelIEEE - 24)/2, 5);
	regVal = (regVal << 1) | (1 << 6) | 0x1;
	REGW(devNum, PHY_BASE+(0x27<<2), regVal);
	REGW(devNum, PHY_BASE+(0x30<<2), 0);
	mSleep(1);
	return(1);
}


/**************************************************************************
* eepromReadAr5210 - Read from given EEPROM offset and return the 16 bit value
*
* RETURNS: 16 bit value from given offset (in a 32-bit value)
*/
A_UINT32 eepromReadAr5210
(
 A_UINT32 devNum, 
 A_UINT32 eepromOffset
)
{
	int			to = 50000;
	A_UINT32    status, pcicfg;

	/* Make sure EEPROM has software access */
	if (REGR(devNum, F2_CFG) & F2_CFG_EEBS)	{
		mError(devNum, EIO, "eepromRead: eeprom is busy.\n");
		return 0xdead;
	}
	pcicfg = REGR(devNum, F2_PCICFG);
    if(!(pcicfg & F2_PCICFG_EEAE)) {
	    REGW(devNum, F2_PCICFG, pcicfg | F2_PCICFG_EEAE);
    }

	/* prime read pump */
	REGR(devNum, F2_EEPROM_BASE + (4 * eepromOffset));

	while (to > 0) {
		status = REGR(devNum, F2_EEPROM_STATUS);
		if (status & F2_EEPROM_STAT_RDDONE)	{
			if (status & F2_EEPROM_STAT_RDERR) {
				mError(devNum, EIO, "eepromRead: eeprom read violation on offset %d!\n", eepromOffset);
				return 0xdead;
			}
			status = REGR(devNum, F2_EEPROM_RDATA);
			return (status & 0xffff);
		}
		to--;
	}
	mError(devNum, EIO, "eepromRead_AR5210: eeprom read timeout on offset %d!\n", eepromOffset);
	return 0xdead;
}

/**************************************************************************
* eepromWriteAr5210 - Write to given EEPROM offset
*
*/
void eepromWriteAr5210
(
 A_UINT32 devNum, 
 A_UINT32 eepromOffset, 
 A_UINT32 eepromValue
)
{
	A_UINT32 status, pcicfg;
	int to = 50000;

	/* Make sure EEPROM has software access */
	if (REGR(devNum, F2_CFG) & F2_CFG_EEBS)	{
		mError(devNum, EIO, "eepromWrite: eeprom is busy.\n");
		return;
	}
	pcicfg = REGR(devNum, F2_PCICFG);
    if(!(pcicfg & F2_PCICFG_EEAE)) {
    	REGW(devNum, F2_PCICFG, pcicfg | F2_PCICFG_EEAE);
    }

	/* send write data */
	REGW(devNum, F2_EEPROM_BASE + (4 * eepromOffset), eepromValue);

	while (to > 0) {
		status = REGR(devNum, F2_EEPROM_STATUS);
		if (status & F2_EEPROM_STAT_WRDONE)	{
			if (status & F2_EEPROM_STAT_WRERR) {
				mError(devNum, EIO, "eepromWrite: eeprom write violation on offset %d!\n", eepromOffset);
				return;
			}
			return;
		}
		to--;
	}
	mError(devNum, EIO, "eepromWrite: eeprom write timeout on offset %d!\n", eepromOffset);
	return;
}

/**************************************************************************
* initPowerAr5210 - Set the power for the AR5000 chipsets
*
*/
void initPowerAr5210
(
	A_UINT32 devNum,
	A_UINT32 freq,
	A_UINT32  override,
	A_UCHAR  *pwrSettings
)
{
    LIB_DEV_INFO *pLibDev = gLibInfo.pLibDevArray[devNum];
	A_UINT16		i, num_pwr_regs;
	A_UCHAR			cp[17];

	A_UINT32 pwr_regs[17] = {
		0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0xf0000000,
		0xcc000000, 0x00000000, 0x00000000,
		0x00000000, 0x0a000000, 0x000000e2,
		0x0a000020, 0x01000002, 0x01000018,
		0x40000000, 0x00000418
	};

	if(override) {
		if(!pwrSettings) {
			mError(devNum, EINVAL, "initializeTransmitPower: TX Power overridden with empty array\n");
			return;
		}
		memcpy(cp, pwrSettings, sizeof(cp));
	}
    else {
	    // Check the EEPROM tx power calibration settings
		//only do this if have eeprom data
	    if (pLibDev->eePromLoad) {
		    setupPowerSettings(devNum, (freq-5000)/5, cp);
		}
		else {
			return;
		}
    }

    if( (cp[15] < 1) || (cp[15] > 5) ) {
        mError(devNum, EINVAL, "initializeTransmitPower: OB should be between 1 and 5 inclusively, not %d\n", cp[15]);
        return;
    }
	if( (cp[16] < 2) || (cp[16] > 5) ) {
		mError(devNum, EINVAL, "initializeTransmitPower: DB should be between 2 and 5 inclusively, not %d\n", cp[16]);
		return;
	}

 	// reverse bits of the channel power array
	for (i = 0; i < 7; i++)	{
		cp[i] = (A_UINT8) reverseBits(cp[i], 5);
	}
	for (i = 7; i < 15; i++) {
		cp[i] = (A_UINT8) reverseBits(cp[i], 6);
	}

	// merge channel power values into the register - quite gross
	pwr_regs[0] |= ((cp[1] << 5) & 0xE0) | (cp[0] & 0x1F);
	pwr_regs[1] |= ((cp[3] << 7) & 0x80) | ((cp[2] << 2) & 0x7C) | ((cp[1] >> 3) & 0x03);
	pwr_regs[2] |= ((cp[4] << 4) & 0xF0) | ((cp[3] >> 1) & 0x0F);
	pwr_regs[3] |= ((cp[6] << 6) & 0xC0) | ((cp[5] << 1) & 0x3E) | ((cp[4] >> 4) & 0x01);
	pwr_regs[4] |= ((cp[7] << 3) & 0xF8) | ((cp[6] >> 2) & 0x07);	 
	pwr_regs[5] |= ((cp[9] << 7) & 0x80) | ((cp[8] << 1) & 0x7E) | ((cp[7] >> 5) & 0x01);
	pwr_regs[6] |= ((cp[10] << 5) & 0xE0) | ((cp[9] >> 1) & 0x1F); 
	pwr_regs[7] |= ((cp[11] << 3) & 0xF8) | ((cp[10] >> 3) & 0x07);
	pwr_regs[8] |= ((cp[12] << 1) & 0x7E) | ((cp[11] >> 5) & 0x01);
	pwr_regs[9] |= ((cp[13] << 5) & 0xE0);
	pwr_regs[10] |= ((cp[14] << 3) & 0xF8) | ((cp[13] >> 3) & 0x07);
	pwr_regs[11] |= ((cp[14] >> 5) & 0x01);

	// Set OB
	pwr_regs[8] |=  (reverseBits(cp[15], 3) << 7) & 0x80;
	pwr_regs[9] |= 	(reverseBits(cp[15], 3) >> 1) & 0x03;

	// Set DB
	pwr_regs[9] |=  (reverseBits(cp[16], 3) << 2) & 0x1C;

	// write the registers
	num_pwr_regs = sizeof(pwr_regs) / sizeof(A_UINT32);
	for (i = 0; i < num_pwr_regs; i++) {
		if (i != num_pwr_regs - 1) {
			REGW(devNum, 0x0000989c, pwr_regs[i]);
		}
		else {
			// last write is to a 0x98d4.
			REGW(devNum, 0x000098d4, pwr_regs[i]);
		}
	} // end writing regs
}

void setSinglePowerAr5210
(
 A_UINT32 devNum, 
 A_UCHAR pcdac
)
{
	A_UINT16 i;
	A_CHAR		fieldName[50];

	//change the fields in the register array for PCDAC
	for(i = 0; i < 8; i++) {
		sprintf(fieldName, "%s%d", FEZ_PCDAC_REG_STRING, i);
		changeField(devNum, fieldName, pcdac);
	}

	//set the gain deltas to be 0
	for(i = 1; i < 8; i++) {
		sprintf(fieldName, "%s%d", GAIN_DELTA_REG_STRING, i);
		changeField(devNum, fieldName, 0);
	}
	return;
}

/**************************************************************************
* setupPowerSettings - Get TXPower values and set them in the radio
*
*/
void setupPowerSettings
(
 A_UINT32 devNum,
 A_UINT32 channelIEEE,
 A_UCHAR  cp[17]
)
{
    LIB_DEV_INFO *pLibDev = gLibInfo.pLibDevArray[devNum];
    A_UCHAR gainFRD, gainF36, gainF48, gainF54;
    A_UCHAR dBmRD, dBm36, dBm48, dBm54, dontcare;
    A_UINT32 rd, group;
   	A_UINT16 *edp;
    TPC_MAP  *pRD;
    A_BOOL   useDefault = FALSE;

	const A_UINT8 ar5kDefault[17] = {
//        gain delta                       pc dac
//     54  48   36  24  18  12   9   54  48  36  24  18  12   9   6  ob  db
		9,  9,   0,  0,  0,  0,  0,   2,  2,  6,  6,  6,  6,  6,  6,  2,  2 
	};

	if ((channelIEEE < 34) || (channelIEEE > 64) ) {
        memcpy(cp, ar5kDefault, 17);
        return;
    }

    switch( (pLibDev->eepData.version >> 12) & 0xF ) {
    case 0:
        // Pull in EEPROM information using rel 1.0 settings
	    // Select between the 8 entries for the calibrated channel set
		group = (( ((channelIEEE - 24)/2) - 5)/2);
		edp = &(gLibInfo.pLibDevArray[devNum]->txPowerData.eepromPwrTable[group * 4]);
		cp[0] = (A_UCHAR)((edp[0] >> 8) & 0x1f);                           // gd 54
		cp[1] = (A_UCHAR)((edp[0] >> 3) & 0x1f);                           // gd 48
		cp[2] = (A_UCHAR)(((edp[0] & 0x7) << 2) | ((edp[1] >> 14) & 0x3)); // gd 36
		cp[3] = (A_UCHAR)((edp[1] >> 9) & 0x1f);                           // gd 24
		cp[4] = (A_UCHAR)((edp[1] >> 4) & 0x1f);                           // gd 18
		cp[5] = cp[4];                                                     // gd 12
		cp[6] = 0;                                                         // gd 9

		cp[7] = (A_UCHAR)(((edp[1] & 0xf) << 2) | ((edp[2] >> 14) & 0x3)); // pc 54
		cp[8] = (A_UCHAR)((edp[2] >> 8) & 0x3f);                           // pc 48
		cp[9] = (A_UCHAR)((edp[2] >> 2) & 0x3f);                           // pc 36
		cp[10] = (A_UCHAR)(((edp[2] & 0x3) << 4) | ((edp[3] >> 12) & 0xf)); // pc 24
		cp[11] = (A_UCHAR)(((edp[3]) >> 6) & 0x3f);                        // pc 18
		cp[12] = cp[11];                                                   // pc 12
		cp[13] = (A_UCHAR)((edp[3]) & 0x3f);                               // pc 9
		cp[14] = cp[13];                                                   // pc 6
		cp[15] = 2;  // Set ob
		cp[16] = 2;  // Set db
        break;

    case 1:
        // Match regulatory domain
        for(rd = 0; rd < REGULATORY_DOMAINS; rd++) {
            if( (pLibDev->eepData.currentRD) == pLibDev->eepData.regDomain[rd] ) {
                break;
            }
        }
        if(rd == REGULATORY_DOMAINS) {
            mError(devNum, EINVAL, "setupPowerSettings: No calibrated regulatory domain matches the set value\n");
            useDefault = TRUE;
        }
        group = ((channelIEEE/2) - 17);
        // Pull 5.29 into the 5.27 group
        if(group > 11) group--;
        // Integer divide will set group from 0 to 4
        group = group / 3;
        pRD = &(pLibDev->eepData.tpc[group]);
        // Set PC DAC Values
        
        cp[14] = pRD->regdmn[rd];
        cp[9] = A_MIN(pRD->regdmn[rd], pRD->rate36);
        cp[8] = A_MIN(pRD->regdmn[rd], pRD->rate48);
        cp[7] = A_MIN(pRD->regdmn[rd], pRD->rate54);

        // Find Corresponding gainF values for RD, 36, 48, 54
        gainFRD = getGainF(devNum, pRD, pRD->regdmn[rd], &dBmRD);
        gainF36 = getGainF(devNum, pRD, cp[9], &dBm36);
        gainF48 = getGainF(devNum, pRD, cp[8], &dBm48);
        gainF54 = getGainF(devNum, pRD, cp[7], &dBm54);

        // Power Scale if requested
        if(pLibDev->txPowerData.tpScale != TP_SCALE_MAX) {
            switch(pLibDev->txPowerData.tpScale) {
            case TP_SCALE_50:
                dBmRD -= 3;
                break;
            case TP_SCALE_25:
                dBmRD -= 6;
                break;
            case TP_SCALE_12:
                dBmRD -= 9;
                break;
            case TP_SCALE_MIN:
                dBmRD = 3;
                break;
            default:
                mError(devNum, EINVAL, "Illegal Transmit Power Scale Value: %d\n", 
                    pLibDev->txPowerData.tpScale);
            }
			cp[14] = getPCDAC(devNum, pRD, dBmRD);
            gainFRD = getGainF(devNum, pRD, cp[14], &dontcare);
            dBm36 = A_MIN(dBm36, dBmRD);
			cp[9] = getPCDAC(devNum, pRD, dBm36);
            gainF36 = getGainF(devNum, pRD, cp[9], &dontcare);
            dBm48 = A_MIN(dBm48, dBmRD);
			cp[8] = getPCDAC(devNum, pRD, dBm48);
            gainF48 = getGainF(devNum, pRD, cp[8], &dontcare);
            dBm54 = A_MIN(dBm54, dBmRD);
			cp[7] = getPCDAC(devNum, pRD, dBm54);
            gainF54 = getGainF(devNum, pRD, cp[7], &dontcare);
        }
		cp[13] = cp[12] = cp[11] = cp[10] = cp[14];

        // Set GainF Values
        cp[0] = (A_UCHAR)(gainFRD - gainF54);
        cp[1] = (A_UCHAR)(gainFRD - gainF48);
        cp[2] = (A_UCHAR)(gainFRD - gainF36);
        cp[3] = cp[4] = cp[5] = cp[6] = 0;  // 9, 12, 18, 24 have no gain_delta from 6

        // Set OB/DB Values
        cp[15] = (A_UCHAR)((pLibDev->eepData.biasCurrents >> 4) & 0x7);
        cp[16] = (A_UCHAR)(pLibDev->eepData.biasCurrents & 0x7);
        break;

    default:
        useDefault = TRUE;
        break;
    }
    if(useDefault == TRUE) {
        memcpy(cp, ar5kDefault, 17);
    }
}

/**************************************************************************
* getGainF - Finds or interpolates the gainF value from the table ptr.
*
* Returns: the gainF
*/
A_UCHAR getGainF
(
 A_UINT32 devNum,
 TPC_MAP *pRD, 
 A_UCHAR pcdac, 
 A_UCHAR *dBm
)
{
    A_CHAR low, high, i;
    double interp;
    low = high = -1;

    for(i = 0; i < TP_SCALING_ENTRIES; i++) {
        if(pRD->pcdac[i] == 63) {
            continue;
        }
        if(pcdac == pRD->pcdac[i]) {
			*dBm = I2DBM(i);
            return pRD->gainF[i];  // Exact Match
        }
        if(pcdac > pRD->pcdac[i]) {
            low = i;
        }
        if(pcdac < pRD->pcdac[i]) {
            high = i;
            if(low == -1) {
                *dBm = I2DBM(i);
                // pcdac is lower than lowest setting
#ifdef _DEBUG
                mError(devNum, EINVAL, "getGainF: pcdac is lower than minimum in table: %d\n", pcdac);
#endif
                return pRD->gainF[i];
            }
            break;
        }
    }
    if( (i >= TP_SCALING_ENTRIES) && (low == -1) ) {
        // No settings were found
        mError(devNum, EINVAL, "getGainF: no valid entries in the pcdac table: %d\n", pcdac);
        return 63;
    }
    if(i >= TP_SCALING_ENTRIES) {
        // pcdac setting was above the max setting in the table
        *dBm = I2DBM(low);
#ifdef _DEBUG
        mError(devNum, EINVAL, "getGainF: pcdac is higher than maximum in table: %d\n", pcdac);
#endif
        return pRD->gainF[low];
    }
    assert((low != -1) && (high != -1));
    *dBm = (A_UCHAR)((low + high) + 3);  // Only exact if table has no missing entries

    // Perform interpolation between low and high values to find gainF
    // linearly scale the pcdac between low and high
    interp = (double)(pcdac - pRD->pcdac[low])/(double)(pRD->pcdac[high] - pRD->pcdac[low]);
    // Multiply the scale ratio by the gainF difference (plus a rnd up factor)
    interp = (interp * (double)(pRD->gainF[high] - pRD->gainF[low])) + .9999;
    // Add ratioed gain_f to low gain_f value
    return (A_UCHAR)((A_UCHAR)(interp) + pRD->gainF[low]);
}

/**************************************************************************
* getPCDAC - Finds or interpolates the pcdac value from the table
*
* Returns: the pcdac
*/
A_UCHAR getPCDAC
(
 A_UINT32 devNum,
 TPC_MAP *pRD, 
 A_UCHAR dBm
)
{
    A_INT32 i;
    A_BOOL useNextEntry = FALSE;
    A_UCHAR interp;

    for(i = (TP_SCALING_ENTRIES - 1); i >= 0; i--) {
        // Check for exact entry
        if(dBm == I2DBM(i)) {
            if(pRD->pcdac[i] != 63) 
                return(pRD->pcdac[i]);
            else
                useNextEntry = TRUE;
        }
        // Check for between entry
        else if(dBm+1 == I2DBM(i) && (i > 0)) {
            if((pRD->pcdac[i] != 63) && (pRD->pcdac[i-1] !=63)) {
                interp = (A_UCHAR)((.35 * (pRD->pcdac[i] - pRD->pcdac[i-1])) + .9999);
                interp = (A_UCHAR)(interp + pRD->pcdac[i-1]);
                return interp;
            }
            else {
                useNextEntry = TRUE;
            }
        }
        // Check for grabbing the next lowest
        else if(useNextEntry == TRUE) {
            if(pRD->pcdac[i] != 63) 
                return(pRD->pcdac[i]);
        }
    }
    // Return lowest Entry if we haven't returned
    for(i = 0; i < TP_SCALING_ENTRIES; i++) {
        if(pRD->pcdac[i] != 63) 
            return(pRD->pcdac[i]);
    }
    // No value to return from table
    mError(devNum, EINVAL, "The transmit power table is blank\n");
    return(1);
}

