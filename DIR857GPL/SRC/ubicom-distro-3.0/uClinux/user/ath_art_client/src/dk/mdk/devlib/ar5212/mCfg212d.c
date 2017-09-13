/*
 *  Copyright © 2003 Atheros Communications, Inc.,  All Rights Reserved.
 *
 */

#ident  "ACI $Id: //depot/sw/src/dk/mdk/devlib/ar5212/mCfg212d.c#19 $, $Header: //depot/sw/src/dk/mdk/devlib/ar5212/mCfg212d.c#19 $"

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
#ifndef VXWORKS
#include <malloc.h>
#endif
#include "wlantype.h"

#include "mCfg212d.h"

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

#include "ar5211reg.h"
#include <string.h>

/**************************************************************************
 * setChannelAr5212 - Perform the algorithm to change the channel
 *					  for AR5002 adapters
 *
 */
A_BOOL setChannelAr5212
(
 A_UINT32 devNum,
 A_UINT32 freq		// New channel
)
{
	A_UINT32 channelSel = 0;
	A_UINT32 bModeSynth = 0;
	A_UINT32 aModeRefSel = 0;
	A_UINT32 regVal = 0;
    LIB_DEV_INFO	*pLibDev = gLibInfo.pLibDevArray[devNum];
 
	if(freq < 4800) {
		if(((freq - 2192) % 5) == 0) {
			channelSel = ((freq-672)*2 - 3040)/10;
			bModeSynth = 0;
		}
		else if (((freq - 2224) % 5) == 0) {
			channelSel = ((freq-704)*2 - 3040)/10;
			bModeSynth = 1;
		}
		else {
			mError(devNum, EINVAL, "%d is an illegal derby driven channel\n", freq);
			return(0);
		}
	
		channelSel = (channelSel << 2) & 0xff;
		channelSel = reverseBits(channelSel, 8);
		if(freq == 2484) {
			REGW(devNum, 0xa204, REGR(devNum, 0xa204) | 0x10);
		}
	}
	else {
		switch(pLibDev->libCfgParams.refClock) {
		case REF_CLK_DYNAMIC:
			if (pLibDev->channelMasks & QUARTER_CHANNEL_MASK) {
				channelSel = reverseBits((A_UINT32)((freq-4800)/2.5 + 1), 8);
				aModeRefSel = reverseBits(0, 2);				
			}
			else if (( (freq % 20) == 0) && (freq >= 5120)){
				channelSel = reverseBits(((freq-4800)/20 << 2), 8);
				aModeRefSel = reverseBits(3, 2);
			} else if ( (freq % 10) == 0) {
				channelSel = reverseBits(((freq-4800)/10 << 1), 8);
				aModeRefSel = reverseBits(2, 2);
			} else if ((freq % 5) == 0) {
				channelSel = reverseBits((freq-4800)/5, 8);
				aModeRefSel = reverseBits(1, 2);
			}
			break;

		case REF_CLK_2_5:
			channelSel = (A_UINT32)((freq-4800)/2.5);
			if (pLibDev->channelMasks & QUARTER_CHANNEL_MASK) {
				channelSel += 1;
			}
			channelSel = reverseBits(channelSel, 8);
			aModeRefSel = reverseBits(0, 2);
			break;

		case REF_CLK_5:
			channelSel = reverseBits((freq-4800)/5, 8);
			aModeRefSel = reverseBits(1, 2);
			break;

		case REF_CLK_10:
			channelSel = reverseBits(((freq-4800)/10 << 1), 8);
			aModeRefSel = reverseBits(2, 2);
			break;

		case REF_CLK_20:
			channelSel = reverseBits(((freq-4800)/20 << 2), 8);
			aModeRefSel = reverseBits(3, 2);
			break;


		default:
			if (pLibDev->channelMasks & QUARTER_CHANNEL_MASK) {
				channelSel = reverseBits((A_UINT32)((freq-4800)/2.5 + 1), 8);
				aModeRefSel = reverseBits(0, 2);				
			}
			else if (( (freq % 20) == 0) && (freq >= 5120)){
				channelSel = reverseBits(((freq-4800)/20 << 2), 8);
				aModeRefSel = reverseBits(3, 2);
			} else if ( (freq % 10) == 0) {
				channelSel = reverseBits(((freq-4800)/10 << 1), 8);
				aModeRefSel = reverseBits(2, 2);
			} else if ((freq % 5) == 0) {
				channelSel = reverseBits((freq-4800)/5, 8);
				aModeRefSel = reverseBits(1, 2);
			}
			break;
		}
	}

	regVal = (channelSel << 4) | (aModeRefSel << 2) | (bModeSynth << 1)| (1 << 12) | 0x1;
	REGW(devNum, PHY_BASE+(0x27<<2), (regVal & 0xff));
    regVal = (regVal >> 8) & 0x7f;
	REGW(devNum, PHY_BASE+(0x36<<2), regVal);		
	mSleep(1);
/*
printf("SNOOP::Turbo mode set %x\n", REGR(devNum, PHY_BASE+0x4));
printf("SNOOP::setChannel to freq=%d:regvalues:reg27=%x:", freq, REGR(devNum, PHY_BASE+(0x27<<2)));
printf("SNOOP::pll reg987c=%x\n", REGR(devNum, 0x987c));
*/

	return(1);
}

/**************************************************************************
* initPowerAr5212 - Set the power for the AR5002 chipsets
*
*/
void initPowerAr5212
(
	A_UINT32 devNum,
	A_UINT32 freq,
	A_UINT32  override,
	A_UCHAR  *pwrSettings
)
{
    LIB_DEV_INFO	*pLibDev = gLibInfo.pLibDevArray[devNum];
	A_INT16		ratesArray[NUM_RATES];
//	FULL_PCDAC_STRUCT  pcdacStruct;
//	A_UINT16			powerValues[PWR_TABLE_SIZE];
//	PCDACS_EEPROM	eepromPcdacs;	
	MODE_HEADER_INFO	*pModeInfo = NULL;
	A_UINT32 mode = pLibDev->mode;

	memset(ratesArray, 0, NUM_RATES * sizeof(A_INT16));
	//only override the power if the eeprom has been read
	if((!pLibDev->eePromLoad) || (!pLibDev->eepData.infoValid)) {
		return;
	}

	if((override) || (pwrSettings != NULL)) {
		mError(devNum, EINVAL, "Override of power not supported.  Disable eeprom load and change config file instead\n");
		return;
	}

	if(((pLibDev->eepData.version >> 12) & 0xF) >= 4) {
		programHeaderInfo(devNum, pLibDev->p16kEepHeader, (A_UINT16)freq, pLibDev->mode);

		switch(mode) {
		case MODE_11A:
			pModeInfo = &(pLibDev->p16kEepHeader->info11a);
			break;

		case MODE_11G:
		case MODE_11O:
			pModeInfo = &(pLibDev->p16kEepHeader->info11g);
			mode = MODE_11G;
			break;

		case MODE_11B:
			pModeInfo = &(pLibDev->p16kEepHeader->info11b);
			break;
		
		}


		//get and set the max power values
		getMaxRDPowerlistForFreq(devNum, (A_UINT16)freq, ratesArray);
        
		forcePowerTxMax(devNum, ratesArray);
		

		if(pLibDev->p16kEepHeader->minorVersion >= 3){
			applyFalseDetectBackoff(devNum, freq, pModeInfo->falseDetectBackoff);		
		}
		return;
	}
	else {
		mError(devNum, EINVAL, "initPowerAr5211: Illegal eeprom version (%d)\n", pLibDev->eepData.version);
		return;
	}
	
	return;
}


