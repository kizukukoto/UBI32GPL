/*
 *  Copyright © 2002 Atheros Communications, Inc.,  All Rights Reserved.
 *
 */

#ident  "ACI $Id: //depot/sw/src/dk/mdk/devlib/ar5211/mCfg211.c#84 $, $Header: //depot/sw/src/dk/mdk/devlib/ar5211/mCfg211.c#84 $"

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

#include "mCfg211.h"

#ifdef Linux
#include "../athreg.h"
#include "../manlib.h"
#include "../mEeprom.h"
#include "../mConfig.h"
#include "../mDevtbl.h"
#else
#include "..\athreg.h"
#include "..\manlib.h"
#include "..\mEeprom.h"
#include "..\mConfig.h"
#include "..\mDevtbl.h"
#if defined(COBRA_AP) && defined(PCI_INTERFACE)
#include "..\mIds.h"
#endif
#endif

#include "ar5211reg.h"
#if defined(SPIRIT_AP) || defined(FREEDOM_AP)
#ifdef AR5312
#include "ar531xreg.h"
#endif
#ifdef AR5315
#include "ar531xPlusreg.h"
#endif

// delta macro defined in both mEeprom.h and ar531x.h 
// cannot #include ar531x.h 
// declaring only the functions and MACROS needed from ar531x.h
UINT8 sysFlashConfigRead(int fcl , int offset);
void sysFlashConfigWrite(int fcl, int offset, UINT8 *data, int len);
#define FLC_RADIOCFG	2

#endif

#ifdef __ATH_DJGPPDOS__
#include "mlibif_dos.h"
#endif //__ATH_DJGPPDOS__

FREQ_MAP beanieFreqMapping[] = {
	2412,	0x2e,	0,	//5580
	2417,	0x5d,	1,  //5585
	2422,	0x2f,	0,	//5590
	2427,	0x5f,	1,	//5595
	2432,	0x30,	0,	//5600
	2437,	0x61,	1,	//5605
	2442,	0x31,	0,	//5610
	2447,	0x63,	1,	//5615
	2452,	0x32,	0,	//5620
	2457,	0x65,	1,	//5625
	2462,	0x33,	0,	//5630
	2467,	0x67,	1,	//5635
	2472,	0x34,	0,	//5640
	2484,	0x32,	0	//5620
};

FREQ_MAP beanieFreqMapping2928[] = {
	2412,	0x16,	0,	//5340
	2417,	0x2d,	1,  //5345
	2422,	0x17,	0,	//5350
	2427,	0x2f,	1,	//5355
	2432,	0x18,	0,	//5360
	2437,	0x31,	1,	//5365
	2442,	0x19,	0,	//5370
	2447,	0x33,	1,	//5375
	2452,	0x1a,	0,	//5380
	2457,	0x35,	1,	//5385
	2462,	0x1b,	0,	//5390
	2467,	0x37,	1,	//5395
	2472,	0x1c,	0,	//5400
	2484,	0x1a,	0	//5405
};

A_UINT16	sizeBeanieMapping = sizeof(beanieFreqMapping)/sizeof(FREQ_MAP);


/* Reset the mac */
void hwResetAr5211
(
 	A_UINT32 devNum,
 	A_UINT32 resetMask
)
{
	A_UINT32 reg;
	LIB_DEV_INFO	*pLibDev = gLibInfo.pLibDevArray[devNum];
#if defined(SPIRIT_AP) || defined(FREEDOM_AP)
	A_UINT32 reg1;
#endif
	A_UINT32 i;
	A_UINT32 timeout = 1000;
	A_UINT32 value;
	A_UINT32 attempt;
	A_UINT32 numAttempts = 5;
	A_UINT32 swDevID;
#if defined (AR5312) || defined(AR5315)
	A_UINT32 tmpAddr;
#endif

	swDevID = ar5kInitData[pLibDev->ar5kInitIndex].swDeviceID;

#if !defined(SPIRIT_AP) && !defined(FREEDOM_AP)
	/* Bring out of sleep mode */
	REGW(devNum, F2_SCR, F2_SCR_SLE_FWAKE);
	for(i = 0; i < timeout; i++ ) {
		value = REGR(devNum, 0x4010); 
		if (!(value & 0x10000)) {
			break;
		}
	}
	
	if (i == timeout) {
		mError(devNum, EIO, "hwResetAr5211: Device did not exit sleep, prior to reset \n");
		return;
	}

	reg = 0;
	if (resetMask & MAC_RESET) {
		reg = reg | F2_RC_MAC; 
	}
	if (resetMask & BB_RESET) {
		reg = reg | F2_RC_BB; 
    } 
    if (resetMask & BUS_RESET) {
//#ifndef CONDOR_HACK
		if(((pLibDev->macRev < 0x90) || (pLibDev->macRev > 0x9f)) && (pLibDev->macRev != 0)){
			//ie its not condor
//   			printf("SNOOP: doing a PCI reset, macRev = %x: BAD FOR CONDOR!!!!!!!!!!!!!!!1\n", pLibDev->macRev);
			reg = reg | F2_RC_PCI; 
		}
//#endif
	}

//	REGR(devNum, F2_RXDP);  // To clear any pending writes, as per doc/mac_registers.txt
	REGW(devNum, F2_RC, reg);
	mSleep(5);
	//workaround for hainan 1.0
	if(pLibDev->macRev == 0x55) {
		for (i = 0; i < 20; i++) {
			REGR(devNum, 0x4020);
		}
	}
//	mSleep(10);


	if(REGR(devNum, F2_RC) == 0) {
		mError(devNum, EIO, "hwResetAr5211: Device did not enter Reset \n");
		return;
	}


	//revisiting the resetCode after bug 11837
	for (attempt = 0; attempt < numAttempts; attempt++) {
		/* Bring out of sleep mode again */
   		REGW(devNum, F2_SCR, F2_SCR_SLE_FWAKE);
		mSleep(1);

		for(i = 0; i < timeout; i++ ) {
			if (!(REGR(devNum, F2_PCICFG) & 0x10000)) {
				break;
			}
		}
		
		if(i != timeout) {
			break;
		}

    	REGW(devNum, F2_RC, reg);
	    mSleep(1);
		if(REGR(devNum, F2_RC) == 0) {
			mError(devNum, EIO, "hwResetAr5211: Device did not enter Reset on second attempt \n");
			return;
		}
	}

	if (attempt == numAttempts) {
		mError(devNum, EIO, "hwResetAr5211: Device did not exit sleep \n");
		return;
	}

        /* Clear the reset */
		REGW(devNum, F2_RC, 0);
	mSleep(1);
		for(i = 0; i < timeout; i++ ) {
			if (!REGR(devNum, F2_RC)) {
				break;
			}
		}
		
	if (i == timeout) {
		mError(devNum, EIO, "hwResetAr5211: Device did not exit reset \n");
		return;
	}
#else

#if defined(COBRA_AP) && defined(PCI_INTERFACE)
	if((pLibDev->hwDevID & 0xff) != (DEVICE_ID_COBRA & 0xff)) {
	    /* Bring out of sleep mode */
	    REGW(devNum, F2_SCR, F2_SCR_SLE_FWAKE);
	    for(i = 0; i < timeout; i++ ) {
		    value = REGR(devNum, 0x4010); 
		    if (!(value & 0x10000)) {
			    break;
			}
		}
	
	    if (i == timeout) {
		    mError(devNum, EIO, "hwResetAr5211: Device did not exit sleep, prior to reset \n");
		    return;
		}

	    reg = 0;
	    if (resetMask & MAC_RESET) {
		    reg = reg | F2_RC_MAC; 
		}
	    if (resetMask & BB_RESET) {
		    reg = reg | F2_RC_BB; 
		} 
        if (resetMask & BUS_RESET) {
    	    reg = reg | F2_RC_PCI; 
		}

        //	REGR(devNum, F2_RXDP);  // To clear any pending writes, as per doc/mac_registers.txt
	    REGW(devNum, F2_RC, reg);
	    mSleep(5);
	    //workaround for hainan 1.0
	    if(pLibDev->macRev == 0x55) {
		    for (i = 0; i < 20; i++) {
			    REGR(devNum, 0x4020);
			}
		}
//	    mSleep(10);

	    if(REGR(devNum, F2_RC) == 0) {
		    mError(devNum, EIO, "hwResetAr5211: Device did not enter Reset \n");
		    return;
		}

	    //revisiting the resetCode after bug 11837
	    for (attempt = 0; attempt < numAttempts; attempt++) {
		    /* Bring out of sleep mode again */
   		    REGW(devNum, F2_SCR, F2_SCR_SLE_FWAKE);
		    mSleep(1);

		    for(i = 0; i < timeout; i++ ) {
			    if (!(REGR(devNum, F2_PCICFG) & 0x10000)) {
				    break;
				}
			}
		
		    if(i != timeout) {
			    break;
			}

    	    REGW(devNum, F2_RC, reg);
	        mSleep(1);
		    if(REGR(devNum, F2_RC) == 0) {
			    mError(devNum, EIO, "hwResetAr5211: Device did not enter Reset on second attempt \n");
			    return;
			}
		}

	    if (attempt == numAttempts) {
		    mError(devNum, EIO, "hwResetAr5211: Device did not exit sleep \n");
		    return;
		}

        /* Clear the reset */
		REGW(devNum, F2_RC, 0);
	    mSleep(1);
		for(i = 0; i < timeout; i++ ) {
			if (!REGR(devNum, F2_RC)) {
				break;
			}
		}
		
	    if (i == timeout) {
		    mError(devNum, EIO, "hwResetAr5211: Device did not exit reset \n");
		    return;
		}
	    return;
	}
#endif //COBRA_AP

	// wmac is choosen based on the baseaddress
	//FJC bug fix 7/23/03 want both wlan macs to be out of reset for low power	

#ifdef AR5312
	tmpAddr = AR531X_WLAN0;
#endif
#ifdef AR5315
	tmpAddr = AR531XPLUS_WLAN0;
#endif

	if (pLibDev->devMap.DEV_REG_ADDRESS == tmpAddr) {
		taskLock();

#ifdef AR5312
	tmpAddr = AR531X_RESET;
#endif
#ifdef AR5315
	tmpAddr = AR531XPLUS_RESET;
#endif

		reg = sysRegRead(tmpAddr);

		if (resetMask & MAC_RESET) {
#ifdef AR5312
			reg = reg | RESET_WLAN0 | RESET_WARM_WLAN0_MAC ; 
#endif
#ifdef AR5315
			reg = reg | RESET_WARM_WLAN0_MAC ; 
#endif
		}
		if (resetMask & BB_RESET) {
			reg = reg | RESET_WARM_WLAN0_BB;
		}

#ifdef AR5312
		sysRegWrite(AR531X_RESET,reg);
		taskUnlock();
		mSleep(10);
	
		/* clear the reset */
		taskLock();
		reg = sysRegRead(AR531X_RESET);
		reg = reg & ~(RESET_WLAN0 | RESET_WARM_WLAN0_MAC | RESET_WARM_WLAN0_BB);
		sysRegWrite(AR531X_RESET,reg);
#endif
#ifdef AR5315
		sysRegWrite(AR531XPLUS_RESET,reg);
		taskUnlock();
		mSleep(10);
	
		/* clear the reset */
		taskLock();
		reg = sysRegRead(AR531XPLUS_RESET);
		reg = reg & ~(RESET_WARM_WLAN0_MAC | RESET_WARM_WLAN0_BB);
		sysRegWrite(AR531XPLUS_RESET,reg);
#endif
		taskUnlock();
		mSleep(10);

#ifdef FREEDOM_AP
#ifdef AR5312
		/* clear the reset for the other wireless interface. */
		taskLock();
		reg1 = sysRegRead(AR531X_RESET);
		reg1 = reg1 & ~(RESET_WLAN1 | RESET_WARM_WLAN1_MAC | RESET_WARM_WLAN1_BB);
		sysRegWrite(AR531X_RESET,reg1);
		taskUnlock();
		mSleep(10);
#endif
#endif
//		sysRegRead(AR531X_RESET);
	} 

#ifdef FREEDOM_AP
#ifdef AR5312
	if (pLibDev->devMap.DEV_REG_ADDRESS == AR531X_WLAN1) {
		taskLock();
		reg1 = sysRegRead(AR531X_RESET);
		if (resetMask & MAC_RESET) {
			reg1 = reg1 | RESET_WARM_WLAN1_MAC ; 
		}
		if (resetMask & BB_RESET) {
			reg1 = reg1 | RESET_WARM_WLAN1_BB;
		}

		/* Freedom 1.0 bug cold reset of wmac1 and apb controlled by 
		   the same bit. dont do a cold reset */
		if ((sysRegRead(AR531X_REV) & REV_MAJ) != 0x30) {
			reg1 = reg1 | RESET_WLAN1; 
		}

		sysRegWrite(AR531X_RESET,reg1);
		taskUnlock();
		mSleep(10);
//		sysRegRead(AR531X_RESET);
		
		/* clear the reset */
		taskLock();
		reg1 = sysRegRead(AR531X_RESET);
		reg1 = reg1 & ~(RESET_WLAN1 | RESET_WARM_WLAN1_MAC | RESET_WARM_WLAN1_BB);
		sysRegWrite(AR531X_RESET,reg1);
		taskUnlock();
		mSleep(10);
		
		/* clear the reset for the other wireless interface */
		taskLock();
		reg = sysRegRead(AR531X_RESET);
		reg = reg & ~(RESET_WLAN0 | RESET_WARM_WLAN0_MAC | RESET_WARM_WLAN0_BB);
		sysRegWrite(AR531X_RESET,reg);
		taskUnlock();
		mSleep(10);
//		sysRegRead(AR531X_RESET);
	}
#endif
#endif // FREEDOM_AP
#endif
}

void pllProgramAr5211
(
 	A_UINT32 devNum,
 	A_UINT32 turbo
)
{
	LIB_DEV_INFO	*pLibDev = gLibInfo.pLibDevArray[devNum];
	A_UINT32 reset;

	reset = 0;

	REGW(devNum, 0xa200, 0);
// move the first part to venice table latter
#if !defined(FREEDOM_AP) && !defined(SPIRIT_AP)
	if(pLibDev->mode == MODE_11B)	{
		REGW(devNum, 0x987c, 0x19) ;
	} else {
		if(turbo == HALF_SPEED_MODE) {
			REGW(devNum, 0x987c, 0x13);
		} else if(turbo == QUARTER_SPEED_MODE) {
			REGW(devNum, 0x987c, 0x218);
		} else {
			REGW(devNum, 0x987c, 0x18);
		}
	} 
	reset = 1;
#else

// pll programming not needed for spirit
#ifdef FREEDOM_AP

#if defined(COBRA_AP) && defined(PCI_INTERFACE)
	if(!isCobra(pLibDev->swDevID)) {
	    if(pLibDev->mode == MODE_11B)	{
		    REGW(devNum, 0x987c, 0x19) ;
		} else {
		    if(turbo == HALF_SPEED_MODE) {
			    REGW(devNum, 0x987c, 0x13);
			} else if(turbo == QUARTER_SPEED_MODE) {
			    REGW(devNum, 0x987c, 0x218);
			} else {
			    REGW(devNum, 0x987c, 0x18);
			}
		} 
	    reset = 1;
	}
	else {
	if(pLibDev->mode == MODE_11B)   {
		REGW(devNum, 0x987c, 0x4b) ;
		m_changeField(devNum,"bb_tx_frame_to_xpab_on", 0x7);
	} else {
		REGW(devNum, 0x987c, 0x05) ;
	}
	mSleep(2);
	reset = 1;
	}
#else
	
	if(pLibDev->mode == MODE_11B)   {
		REGW(devNum, 0x987c, 0x4b) ;
		m_changeField(devNum,"bb_tx_frame_to_xpab_on", 0x7);
	} else {
		REGW(devNum, 0x987c, 0x05) ;
	}
	mSleep(2);
	reset = 1;

#endif // COBRA_AP
#endif // FREEDOM_AP
#endif
	if (reset) {
		hwResetAr5211(devNum, MAC_RESET | BB_RESET);
	}
}


void setSinglePowerAr5211
(
 A_UINT32 devNum, 
 A_UCHAR pcdac
)
{
	A_UINT16 i;
	A_CHAR		fieldName[50];

	//change the fields in the register array for PCDAC
	for(i = 0; i < 64; i++) {
		sprintf(fieldName, "%s%d", PCDAC_REG_STRING, i);
		changeField(devNum, fieldName, pcdac);
	}

	//change all rate power levels to point to first entry in table
	for (i = 0; i < 8; i++) {
		sprintf(fieldName, "%s%d", TXPOWER_REG_STRING, i);
		changeField(devNum, fieldName, 0);
	}
}

/**************************************************************************
 * setChannelAr5211 - Perform the algorithm to change the channel
 *					  for AR5001 adapters
 *
 */
A_BOOL setChannelAr5211
(
 A_UINT32 devNum,
 A_UINT32 freq		// New channel
)
{
	A_UINT32 regVal;
	A_UCHAR	 refClk;
	A_UINT32 channelIEEE = (freq - 5000) / 5;

    if (freq >= 5725) {
		if (((freq % 5) != 0) || ((freq % 10) == 0)) {
			regVal = reverseBits((channelIEEE - 24)/2, 8);
			refClk = 0;
        }
        else {
            regVal = reverseBits(channelIEEE - 24, 8);
			refClk = 1;
        }
    } 
    else {
		refClk = 0;
        if(freq < 5120) {
			if (((freq % 5) == 0) && ((freq % 10) != 0)) {
				regVal = 0xd5 + (freq-4905)/5;
				refClk = 1;
			}
			else {
				regVal = 0xea + (freq-4900)/10;
			}
			regVal = reverseBits(regVal, 8);
		}
		else {
			regVal = reverseBits((channelIEEE - 24)/2, 8);
		}
    }
	regVal = (regVal << 2) | (refClk << 1) | (1 << 10) | 0x1;
	REGW(devNum, PHY_BASE+(0x27<<2), (regVal & 0xff));
    regVal = (regVal >> 8) & 0xff;
	REGW(devNum, PHY_BASE+(0x34<<2), regVal);		
	mSleep(1);

	//apply the backoff if needed, this may be overwitten in an eeprom load
//	applyFalseDetectBackoff(devNum, freq, pLibDev->suppliedFalseDetBackoff[pLibDev->mode]);
	return(1);
}

/**************************************************************************
 * setChannelAr5211_beanie - Perform the algorithm to change the channel
 *							 for AR5001 adapters containing beanie
 *
 */
A_BOOL setChannelAr5211_beanie
(
 A_UINT32 devNum,
 A_UINT32 freq		// New channel
)
{
	A_UINT32 regVal;
	A_UINT32 regVal1;
	A_UCHAR	 beanieRefClk = 1;
	A_UINT32 beanieChannelSel;
	A_UINT16 i;
	A_UINT32 pciValue;
	LIB_DEV_INFO	*pLibDev = gLibInfo.pLibDevArray[devNum];	
	FREQ_MAP  *pFreqMapping;		
	
	if (freq < 4000) {
//		applyFalseDetectBackoff(devNum, freq, pLibDev->suppliedFalseDetBackoff[pLibDev->mode]);
		//get the sombrero info
		if(pLibDev->libCfgParams.beanie2928Mode) {
			pFreqMapping = beanieFreqMapping2928;
		}
		else {
			pFreqMapping = beanieFreqMapping;
		}
		for (i = 0; i < sizeBeanieMapping; i++) {
			if(freq == pFreqMapping[i].requiredFreq) {
				break;
			}
		}
		
		if (i == sizeBeanieMapping) {
			return(attemptOtherChannel(devNum, freq));
		}

		//get the beanie channel select value
		if (freq == 2484) {
			//handle special case
			if(pLibDev->libCfgParams.beanie2928Mode) {
				beanieChannelSel = reverseBits(0x35, 8);
			}
			else {
				beanieChannelSel = reverseBits(0x44, 8);
			}

			if(pLibDev->swDevID >= 0x0013) {
				if(freq == 2484) {
					REGW(devNum, 0xa204, REGR(devNum, 0xa204) | 0x10);
				}
			}
		}
		else if (pLibDev->libCfgParams.beanie2928Mode) {
			beanieChannelSel = reverseBits(0x37, 8); 
		}
		else {
			beanieChannelSel = reverseBits(0x46, 8);
		}

		//program the registers
		regVal = (reverseBits(pFreqMapping[i].sombreroChannelSel, 8) << 2) | 
					(pFreqMapping[i].sombreroRefClk << 1) | (1 << 10) | 0x1;
		regVal1 = (beanieChannelSel << 5) | (beanieRefClk << 4);

		pciValue = (regVal & 0xff) | ((regVal1 & 0xff) << 8);
		REGW(devNum, PHY_BASE+(0x27<<2), pciValue);
		regVal = (regVal >> 8) & 0xff;
		regVal1 = regVal1  & 0xff00;
		pciValue = regVal | regVal1;
		REGW(devNum, PHY_BASE+(0x34<<2), pciValue);		
	}
	else {
		//set channel for sombrero only
		setChannelAr5211(devNum, freq);
	}
	
	mSleep(1);
	return(1);
}


/**************************************************************************
 * attemptOtherChannel - Attempt to see if trying to set one of the other
 *                       beanie driven channels, attempt to support beanie
 *                       freq of 3168, 2816 and 3136  and som freq in steps 10
 *
 */
A_BOOL attemptOtherChannel
(
 A_UINT32 devNum,
 A_UINT32 freq		
)
{
	A_UINT32	somFreq;
	A_UINT32	beanieFreq[] = {3168, 2816, 3136};
	A_UINT16	numFreq = sizeof(beanieFreq)/sizeof(A_UINT32);
	A_UINT16	i;

	for(i = 0; i < numFreq; i++) {
		somFreq = freq + beanieFreq[i]; 
		if( (somFreq >= 5120) && (somFreq <= 5900) && !(somFreq % 10))
		{
			//set that channel
			return(setAllChannelAr5211(devNum, somFreq, beanieFreq[i]));
		}
	}
	//if got to here then did not find a channel
	mError(devNum, EINVAL, "%d is an illegal beanie driven channel\n", freq);
	return(0);
}

/**************************************************************************
 * setAllChannelAr5211 - Perform the algorithm to change the channel
 *						 for AR5001 adapters containing beanie. Open up
 *						 lots of 2GHz channels
 *
 */
MANLIB_API A_BOOL setAllChannelAr5211
(
 A_UINT32 devNum,
 A_UINT32 somFreq,		// Sombrero channel
 A_UINT32 beanieFreq	//beanie freq
)
{
	A_UINT32 regVal;
	A_UINT32 regVal1;
	A_UCHAR	 somRefClk;
	A_UINT32 somChannelSel;
	A_UCHAR	 beanieRefClk = 1;
	A_UINT32 beanieChannelSel;
	A_UINT32 pciValue;
	A_UINT32 channelIEEE = (somFreq - 5000) / 5;

    //work out the sombrero refClk and channel select
	if (somFreq >= 5725) {
		if (((somFreq % 5) != 0) || ((somFreq % 10) == 0)) {
			somChannelSel = reverseBits((channelIEEE - 24)/2, 8);
			somRefClk = 0;
        }
        else {
            somChannelSel = reverseBits(channelIEEE - 24, 8);
			somRefClk = 1;
        }
    } 
    else {
        if(somFreq < 5120) {
			somChannelSel = 0xec + (somFreq-4920)/10;
			somChannelSel = reverseBits(somChannelSel, 8);
		}
		else {
			somChannelSel = reverseBits((channelIEEE - 24)/2, 8);
		}
		somRefClk = 0;
    }
	
	switch(beanieFreq) {
	case 0:
		return(setChannelAr5211(devNum, somFreq));
		break;

	case 3168:
		beanieChannelSel = reverseBits(0x46, 8);
		break;

	case 3136:
		beanieChannelSel = reverseBits(0x44, 8);
		break;
		
	case 2816:
		beanieChannelSel = reverseBits(0x30, 8);
		break;

	default:
		mError(devNum, EINVAL, "setChannelAr5211_beanieAllChan: illegal beanie freq %d\n", beanieFreq);
		return 0;
	}

	//program the registers
	regVal = (somChannelSel << 2) | (somRefClk << 1) | (1 << 10) | 0x1;
	regVal1 = (beanieChannelSel << 5) | (beanieRefClk << 4);

	pciValue = (regVal & 0xff) | ((regVal1 & 0xff) << 8);
	REGW(devNum, PHY_BASE+(0x27<<2), pciValue);
	regVal = (regVal >> 8) & 0xff;
	regVal1 = regVal1  & 0xff00;
	pciValue = regVal | regVal1;

	REGW(devNum, PHY_BASE+(0x34<<2), pciValue);		
	
	mSleep(1);
	return(1);
}

/**************************************************************************
* eepromReadAr5211 - Read from given EEPROM offset and return the 16 bit value
*
* RETURNS: 16 bit value from given offset (in a 32-bit value)
*/
A_UINT32 eepromReadAr5211
(
 A_UINT32 devNum, 
 A_UINT32 eepromOffset
)
{
	int			to = 50000;
	A_UINT32	status;
	A_UINT32	eepromValue;
#if defined(COBRA_AP) && defined(PCI_INTERFACE)
	LIB_DEV_INFO	*pLibDev = gLibInfo.pLibDevArray[devNum];
#endif 

#if defined(SPIRIT_AP) || defined(FREEDOM_AP)

#if defined(COBRA_AP) && defined(PCI_INTERFACE)
	if((pLibDev->hwDevID & 0xff) == (DEVICE_ID_COBRA & 0xff)) {
// 	16 bit addressing 
	eepromOffset = eepromOffset << 1;
        eepromValue = (sysFlashConfigRead(FLC_RADIOCFG, eepromOffset) << 8) | 
		       sysFlashConfigRead(FLC_RADIOCFG, (eepromOffset + 1));
//	q_uiPrintf("Read from eeprom @ %x : %x \n",(eepromOffset >> 1),eepromValue); 
	return eepromValue;
	}
	else {
	    //write the address
	    REGW(devNum, F2_EEPROM_ADDR, eepromOffset);

	    //issue read command
	    REGW(devNum, F2_EEPROM_CMD, F2_EEPROM_CMD_READ);

	    //wait for read to be done
	    while (to > 0) {
		    status = REGR(devNum, F2_EEPROM_STS);
		    if (status & F2_EEPROM_STS_READ_COMPLETE) {
			    //read the value
			    eepromValue = REGR(devNum, F2_EEPROM_DATA) & 0xffff;
			    return(eepromValue);
			}

		    if(status & F2_EEPROM_STS_READ_ERROR) {
			    mError(devNum, EIO, "eepromRead_AR5211: eeprom read failure on offset %d!\n", eepromOffset);
			    return 0xdead;
			}
		    to--;
		}
	}
#else
	
// 	16 bit addressing 
	eepromOffset = eepromOffset << 1;
        eepromValue = (sysFlashConfigRead(FLC_RADIOCFG, eepromOffset) << 8) | 
		       sysFlashConfigRead(FLC_RADIOCFG, (eepromOffset + 1));
//	q_uiPrintf("Read from eeprom @ %x : %x \n",(eepromOffset >> 1),eepromValue); 
	return eepromValue;
#endif //COBRA_AP

#else
	//write the address
	REGW(devNum, F2_EEPROM_ADDR, eepromOffset);

	//issue read command
	REGW(devNum, F2_EEPROM_CMD, F2_EEPROM_CMD_READ);

	//wait for read to be done
	while (to > 0) {
		status = REGR(devNum, F2_EEPROM_STS);
		if (status & F2_EEPROM_STS_READ_COMPLETE) {
			//read the value
			eepromValue = REGR(devNum, F2_EEPROM_DATA) & 0xffff;
			return(eepromValue);
		}

		if(status & F2_EEPROM_STS_READ_ERROR) {
			mError(devNum, EIO, "eepromRead_AR5211: eeprom read failure on offset %d!\n", eepromOffset);
			return 0xdead;
		}
		to--;
	}
#endif
	mError(devNum, EIO, "eepromRead_AR5211: eeprom read timeout on offset %d!\n", eepromOffset);
	return 0xdead;

}


void eepromWriteAr5211
(
 A_UINT32 devNum, 
 A_UINT32 eepromOffset, 
 A_UINT32 eepromValue
)
{
#define ADDR_GPIO_CR            0x4014
#define FD_GPIO_2               0x30

	int			to = 50000;
	A_UINT32	status;
	LIB_DEV_INFO	*pLibDev = gLibInfo.pLibDevArray[devNum];
	A_UINT32        rb;

#if defined(SPIRIT_AP) || defined(FREEDOM_AP)

#if defined(COBRA_AP) && defined(PCI_INTERFACE)
	if((pLibDev->hwDevID & 0xff)== (DEVICE_ID_COBRA & 0xff)) {
	    A_UINT8 *pVal;
	
	    pVal = (A_UINT8 *)&eepromValue;

        // 16 bit addressing
        //	q_uiPrintf("Write to eeprom @ %x : %x \n",eepromOffset,eepromValue); 
	    eepromOffset = eepromOffset << 1;	
        // write only the lower 2 bytes
	    sysFlashConfigWrite(FLC_RADIOCFG, eepromOffset, (pVal+2) , 2);	
	    return;
	}
	else {
	    //write the address
	    REGW(devNum, F2_EEPROM_ADDR, eepromOffset);

	    //write the value
	    REGW(devNum, F2_EEPROM_DATA, eepromValue);

	    //issue write command
	    REGW(devNum, F2_EEPROM_CMD, F2_EEPROM_CMD_WRITE);

	    //wait for write to be done
	    while (to > 0) {
   		    status = REGR(devNum, F2_EEPROM_STS);
		    if (status & F2_EEPROM_STS_WRITE_COMPLETE) {
			    return;
			}

		    if(status & F2_EEPROM_STS_WRITE_ERROR) {
			    mError(devNum, EIO, "eepromWrite:ar5211 eeprom write failure on offset %d!\n", eepromOffset);
			    return;
			}
		    to--;
		}
	}
#else     
{
	A_UINT8 *pVal;
	
	pVal = (A_UINT8 *)&eepromValue;

// 16 bit addressing
//	q_uiPrintf("Write to eeprom @ %x : %x \n",eepromOffset,eepromValue); 
	eepromOffset = eepromOffset << 1;	
// write only the lower 2 bytes
	sysFlashConfigWrite(FLC_RADIOCFG, eepromOffset, (pVal+2) , 2);	
	return;
}
#endif //COBRA_AP
#else
    if (isNala(pLibDev->swDevID)) {
	  //  Enable EEPROM write
	  rb = REGR(devNum, ADDR_GPIO_CR);
	  REGW(devNum, ADDR_GPIO_CR, (rb | FD_GPIO_2));
	}
	//write the address
	REGW(devNum, F2_EEPROM_ADDR, eepromOffset);

	//write the value
	REGW(devNum, F2_EEPROM_DATA, eepromValue);

	//issue write command
	REGW(devNum, F2_EEPROM_CMD, F2_EEPROM_CMD_WRITE);

	//wait for write to be done
	while (to > 0) {
		status = REGR(devNum, F2_EEPROM_STS);
		if (status & F2_EEPROM_STS_WRITE_COMPLETE) {
			return;
		}

		if(status & F2_EEPROM_STS_WRITE_ERROR) {
			mError(devNum, EIO, "eepromWrite:ar5211 eeprom write failure on offset %d!\n", eepromOffset);
			return;
		}
		to--;
	}
	if (isNala(pLibDev->swDevID))
	{
		//  Disable EEPROM write
		rb = REGR(devNum, ADDR_GPIO_CR);
		REGW(devNum, ADDR_GPIO_CR, (rb & ~FD_GPIO_2));
	}

#endif
#undef ADDR_GPIO_CR
#undef FD_GPIO_2

	mError(devNum, EIO, "eepromWritear5211: eeprom write timeout on offset %d!\n", eepromOffset);
	return;
}

/**************************************************************************
* initPowerAr5211 - Set the power for the AR5001 chipsets
*
*/
void initPowerAr5211
(
	A_UINT32 devNum,
	A_UINT32 freq,
	A_UINT32  override,
	A_UCHAR  *pwrSettings
)
{
    LIB_DEV_INFO	*pLibDev = gLibInfo.pLibDevArray[devNum];
	A_INT16		ratesArray[NUM_RATES];
	MDK_FULL_PCDAC_STRUCT  pcdacStruct;
	A_UINT16			powerValues[PWR_TABLE_SIZE];
	MDK_PCDACS_EEPROM	eepromPcdacs;	
	MODE_HEADER_INFO	*pModeInfo = NULL;

	//only override the power if the eeprom has been read
	if((!pLibDev->eePromLoad) || (!pLibDev->eepData.infoValid)) {
		return;
	}

	if((override) || (pwrSettings != NULL)) {
		mError(devNum, EINVAL, "Override of power not supported.  Disable eeprom load and change config file instead\n");
		return;
	}

	if((((pLibDev->eepData.version >> 12) & 0xF) == 3) || (((pLibDev->eepData.version >> 12) & 0xF) == 4)){
		programHeaderInfo(devNum, pLibDev->p16kEepHeader, (A_UINT16)freq, pLibDev->mode);

		//fill out the full pcdac info for this channel
		pcdacStruct.channelValue = (A_UINT16)freq;
		//setup the pcdac struct to point to the correct info, based on mode
		switch(pLibDev->mode) {
		case MODE_11A:
			eepromPcdacs.numChannels = pLibDev->pCalibrationInfo->numChannels_11a;
			eepromPcdacs.pChannelList = pLibDev->pCalibrationInfo->Channels_11a;
			eepromPcdacs.pDataPerChannel = pLibDev->pCalibrationInfo->DataPerChannel_11a;
			pModeInfo = &(pLibDev->p16kEepHeader->info11a);
			break;

		case MODE_11G:
		case MODE_11O:
			eepromPcdacs.numChannels = pLibDev->pCalibrationInfo->numChannels_2_4;
			eepromPcdacs.pChannelList = pLibDev->pCalibrationInfo->Channels_11g;
			eepromPcdacs.pDataPerChannel = pLibDev->pCalibrationInfo->DataPerChannel_11g;
			pModeInfo = &(pLibDev->p16kEepHeader->info11g);
			break;

		case MODE_11B:
			eepromPcdacs.numChannels = pLibDev->pCalibrationInfo->numChannels_2_4;
			eepromPcdacs.pChannelList = pLibDev->pCalibrationInfo->Channels_11b;
			eepromPcdacs.pDataPerChannel = pLibDev->pCalibrationInfo->DataPerChannel_11b;
			pModeInfo = &(pLibDev->p16kEepHeader->info11b);
			break;
		
		}
		if((pLibDev->p16kEepHeader->majorVersion <= 3) ||
			((pLibDev->p16kEepHeader->majorVersion >= 4) && (pLibDev->p16kEepHeader->eepMap == 0)) ) {
			mapPcdacTable(&eepromPcdacs, &pcdacStruct);
			
			//get the power table from the pcdac table
			getPwrTable(&pcdacStruct, powerValues);
		
			//write power table to registers
			forcePCDACTable(devNum, powerValues);
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
		mError(devNum, EINVAL, "initPowerAr5211: Illegal eeprom version (%d)\n",  pLibDev->eepData.version);
		return;
	}
	
	return;
}

MANLIB_API void forceAntennaTbl5211
(
 A_UINT32 devNum,
 A_UINT16 *pAntennaTbl
)
{
	writeField(devNum, "bb_switch_table_idle", pAntennaTbl[0]);
	writeField(devNum, "bb_switch_table_t1", pAntennaTbl[1]);
	writeField(devNum, "bb_switch_table_r1", pAntennaTbl[2]);
	writeField(devNum, "bb_switch_table_r1x1", pAntennaTbl[3]);
	writeField(devNum, "bb_switch_table_r1x2", pAntennaTbl[4]);
	writeField(devNum, "bb_switch_table_r1x12", pAntennaTbl[5]);
	writeField(devNum, "bb_switch_table_t2", pAntennaTbl[6]);
	writeField(devNum, "bb_switch_table_r2", pAntennaTbl[7]);
	writeField(devNum, "bb_switch_table_r2x1", pAntennaTbl[8]);
	writeField(devNum, "bb_switch_table_r2x2", pAntennaTbl[9]);
	writeField(devNum, "bb_switch_table_r2x12", pAntennaTbl[10]);
	return;
}

MANLIB_API void setAntennaTbl5211
(
 A_UINT32 devNum,
 A_UINT16 *pAntennaTbl
)
{
	changeField(devNum, "bb_switch_table_idle", pAntennaTbl[0]);
	changeField(devNum, "bb_switch_table_t1", pAntennaTbl[1]);
	changeField(devNum, "bb_switch_table_r1", pAntennaTbl[2]);
	changeField(devNum, "bb_switch_table_r1x1", pAntennaTbl[3]);
	changeField(devNum, "bb_switch_table_r1x2", pAntennaTbl[4]);
	changeField(devNum, "bb_switch_table_r1x12", pAntennaTbl[5]);
	changeField(devNum, "bb_switch_table_t2", pAntennaTbl[6]);
	changeField(devNum, "bb_switch_table_r2", pAntennaTbl[7]);
	changeField(devNum, "bb_switch_table_r2x1", pAntennaTbl[8]);
	changeField(devNum, "bb_switch_table_r2x2", pAntennaTbl[9]);
	changeField(devNum, "bb_switch_table_r2x12", pAntennaTbl[10]);
	return;
}

MANLIB_API void readAntennaTbl5211
(
 A_UINT32 devNum,
 A_UINT16 *pAntennaTbl
)
{
	A_UINT32 unsignedValue;
	A_INT32  signedValue;
	A_BOOL	 signedFlag;

	readField(devNum, "bb_switch_table_idle", &unsignedValue, &signedValue, &signedFlag);
	pAntennaTbl[0] = (A_UINT16)unsignedValue;
	readField(devNum, "bb_switch_table_t1", &unsignedValue, &signedValue, &signedFlag);
	pAntennaTbl[1] = (A_UINT16)unsignedValue;
	readField(devNum, "bb_switch_table_r1", &unsignedValue, &signedValue, &signedFlag);
	pAntennaTbl[2] = (A_UINT16)unsignedValue;
	readField(devNum, "bb_switch_table_r1x1", &unsignedValue, &signedValue, &signedFlag);
	pAntennaTbl[3] = (A_UINT16)unsignedValue;
	readField(devNum, "bb_switch_table_r1x2", &unsignedValue, &signedValue, &signedFlag);
	pAntennaTbl[4] = (A_UINT16)unsignedValue;
	readField(devNum, "bb_switch_table_r1x12", &unsignedValue, &signedValue, &signedFlag);
	pAntennaTbl[5] = (A_UINT16)unsignedValue;
	readField(devNum, "bb_switch_table_t2", &unsignedValue, &signedValue, &signedFlag);
	pAntennaTbl[6] = (A_UINT16)unsignedValue;
	readField(devNum, "bb_switch_table_r2", &unsignedValue, &signedValue, &signedFlag);
	pAntennaTbl[7] = (A_UINT16)unsignedValue;
	readField(devNum, "bb_switch_table_r2x1", &unsignedValue, &signedValue, &signedFlag);
	pAntennaTbl[8] = (A_UINT16)unsignedValue;
	readField(devNum, "bb_switch_table_r2x2", &unsignedValue, &signedValue, &signedFlag);
	pAntennaTbl[9] = (A_UINT16)unsignedValue;
	readField(devNum, "bb_switch_table_r2x12", &unsignedValue, &signedValue, &signedFlag);
	pAntennaTbl[10] = (A_UINT16)unsignedValue;

}

