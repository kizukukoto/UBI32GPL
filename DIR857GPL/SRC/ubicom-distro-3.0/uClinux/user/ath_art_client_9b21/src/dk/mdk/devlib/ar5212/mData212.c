/*
 *  Copyright © 2002 Atheros Communications, Inc.,  All Rights Reserved.
 *
 */

#ident  "ACI $Id: //depot/sw/src/dk/mdk/devlib/ar5212/mData212.c#7 $, $Header: //depot/sw/src/dk/mdk/devlib/ar5212/mData212.c#7 $"

#ifdef VXWORKS
#include "vxworks.h"
#endif

#ifdef __ATH_DJGPPDOS__
#define __int64	long long
typedef unsigned long DWORD;
#define Sleep	delay
#endif	// #ifdef __ATH_DJGPPDOS__

#include <errno.h>
#include "mData212.h"
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
#include "stdio.h"

extern void rxBeginConfigAr5211
(
 A_UINT32 devNum
);
void macAPIInitAr5212
(
 A_UINT32 devNum
) 
{
	LIB_DEV_INFO *pLibDev = gLibInfo.pLibDevArray[devNum];
	pLibDev->txDescStatus1 = FIRST_VENICE_STATUS_WORD;
	pLibDev->txDescStatus2 = SECOND_VENICE_STATUS_WORD;
	pLibDev->decryptErrMsk = VENICE_DESC_DECRYPT_ERROR;
	pLibDev->bitsToRxSigStrength = VENICE_BITS_TO_RX_SIG_STRENGTH;
	pLibDev->rxDataRateMsk = VENICE_DATA_RATE_MASK;
	
	return;
}

void setDescriptorAr5212
(
 A_UINT32 devNum,
 MDK_ATHEROS_DESC*  localDescPtr,
 A_UINT32 pktSize,
 A_UINT32 antMode,
 A_UINT32 descNum,
 A_UINT32 rateIndex,
 A_UINT32 broadcast
)
{
	LIB_DEV_INFO *pLibDev = gLibInfo.pLibDevArray[devNum];
	A_UINT16	 queueIndex = pLibDev->selQueueIndex;
	A_UINT32 frameLen = 0;
	MDK_VENICE_DESC  *localVeniceDescPtr = (MDK_VENICE_DESC *)localDescPtr;
	A_UINT32	numAttempts;
	A_BOOL      xrRate = 0;

	//check for this being an xr rate
	if(rateIndex >= LOWEST_XR_RATE_INDEX && rateIndex <= HIGHEST_XR_RATE_INDEX) {
			xrRate = 1;
	}
	//create and write the 2 control words
    frameLen = (pktSize + FCS_FIELD) + ((pLibDev->wepEnable && (descNum < pLibDev->tx[queueIndex].numDesc)) ? WEP_ICV_FIELD : 0);
	localVeniceDescPtr->hwControl[0] = frameLen | 0x01000000 |  //clear dest mask
				 (antMode << BITS_TO_TX_ANT_MODE); //note leaving wep off for now
	

    if (descNum == pLibDev->tx[queueIndex].numDesc - 1) {
        localVeniceDescPtr->hwControl[0] |= DESC_TX_INTER_REQ;
    }
    
	if(xrRate) {
		localVeniceDescPtr->hwControl[0] |= DESC_ENABLE_CTS;
	}

	localVeniceDescPtr->hwControl[1] = pktSize | (broadcast ? (1 << BITS_TO_VENICE_NOACK) : 0) |
									(DESC_COMPRESSION_DISABLE << BITS_TO_COMPRESSION); //Disable compression for now


	//setup the remainder 2 control words for venice
	//for now only setup one group of retries
	//add 1 to retries to account for 1st attempt (but if retries is 15, don't add at moment)
	numAttempts = pLibDev->tx[queueIndex].retryValue;
	if(numAttempts < 0xf) {
		numAttempts++;
	}
	localVeniceDescPtr->hwControl[2] = numAttempts << BITS_TO_DATA_TRIES0;
	localVeniceDescPtr->hwControl[3] = rateValues[rateIndex] << BITS_TO_TX_DATA_RATE0;

	if(xrRate) {
		localVeniceDescPtr->hwControl[3] |= (rateValues[0] << BITS_TO_RTS_CTS_RATE);
	}

}

void setDescriptorEndPacketAr5212
(
 A_UINT32 devNum,
 MDK_ATHEROS_DESC*  localDescPtr,
 A_UINT32 pktSize,
 A_UINT32 antMode,
 A_UINT32 descNum,
 A_UINT32 rateIndex,
 A_UINT32 broadcast
)
{
	LIB_DEV_INFO *pLibDev = gLibInfo.pLibDevArray[devNum];
	MDK_VENICE_DESC  *localVeniceDescPtr = (MDK_VENICE_DESC *)localDescPtr;
	A_UINT32	numAttempts;
	A_UINT16	 queueIndex = pLibDev->selQueueIndex;

	setDescriptorAr5212(devNum, localDescPtr, pktSize, antMode, descNum, rateIndex, broadcast);

	numAttempts = pLibDev->tx[queueIndex].retryValue;
	if(numAttempts < 0xf) {
		numAttempts++;
	}
	//set a second set of attempts at rate 6.
	localVeniceDescPtr->hwControl[2] = localVeniceDescPtr->hwControl[2] | (numAttempts << BITS_TO_DATA_TRIES1);
	localVeniceDescPtr->hwControl[3] = localVeniceDescPtr->hwControl[3] | (rateValues[0] << BITS_TO_TX_DATA_RATE1);
}

void setStatsPktDescAr5212
(
 A_UINT32 devNum,
 MDK_ATHEROS_DESC*  localDescPtr,
 A_UINT32 pktSize,
 A_UINT32 rateValue
)
{
	LIB_DEV_INFO *pLibDev = gLibInfo.pLibDevArray[devNum];
	MDK_VENICE_DESC  *localVeniceDescPtr = (MDK_VENICE_DESC *)localDescPtr;
	A_UINT32 tempRateValue = rateValue;

	if(pLibDev->mode == MODE_11G) {
		tempRateValue = rateValues[8];  //try an attempt at 1Mbps 
	}
	else {
		tempRateValue = rateValues[0];
	}

	localVeniceDescPtr->hwControl[0] = pktSize + 4;

	localVeniceDescPtr->hwControl[1] = pktSize;

	localVeniceDescPtr->hwControl[2] = (0xf << BITS_TO_DATA_TRIES0) | (0xf << BITS_TO_DATA_TRIES1);
	localVeniceDescPtr->hwControl[3] = (rateValue << BITS_TO_TX_DATA_RATE0) | (tempRateValue << BITS_TO_TX_DATA_RATE1);
	devNum = 0;		//this is not used quieting warnings
	return;
}

void setContDescriptorAr5212
(
 A_UINT32 devNum,
 MDK_ATHEROS_DESC*  localDescPtr,
 A_UINT32 pktSize,
 A_UINT32 antMode,
 A_UCHAR  dataRate
)
{
	MDK_VENICE_DESC  *localVeniceDescPtr = (MDK_VENICE_DESC *)localDescPtr;

	localVeniceDescPtr->hwControl[0] = (antMode << BITS_TO_TX_ANT_MODE) |
				 (pktSize + FCS_FIELD);

	//clear no ack bit for better performance
	localVeniceDescPtr->hwControl[1] = pktSize | 
									(DESC_COMPRESSION_DISABLE << BITS_TO_COMPRESSION); //Disable compression for now
	
	//set max attempts for 1 exchange for better performance
	localVeniceDescPtr->hwControl[2] = (A_UINT32)((0xf << BITS_TO_DATA_TRIES0) 
		| (0xf << BITS_TO_DATA_TRIES1)
		| (0xf << BITS_TO_DATA_TRIES2)
		| (0xf << BITS_TO_DATA_TRIES3));
	localVeniceDescPtr->hwControl[3] = (rateValues[dataRate] << BITS_TO_TX_DATA_RATE0) 
		| (rateValues[dataRate] << BITS_TO_TX_DATA_RATE1)
		| (rateValues[dataRate] << BITS_TO_TX_DATA_RATE2)
		| (rateValues[dataRate] << BITS_TO_TX_DATA_RATE3);

	devNum = 0;	//this is not used quieting warnings
}

A_UINT32 txGetDescRateAr5212
(
 A_UINT32 devNum,
 A_UINT32 descAddr,
 A_BOOL   *ht40,
 A_BOOL   *shortGi
)
{
	LIB_DEV_INFO *pLibDev = gLibInfo.pLibDevArray[devNum];
	A_UINT32 descRate;

	pLibDev->devMap.OSmemRead(devNum, descAddr + FOURTH_CONTROL_WORD, 
						(A_UCHAR *)&(descRate), sizeof(descRate));

	descRate = (descRate >> BITS_TO_TX_DATA_RATE0) & VENICE_DATA_RATE_MASK;
    *ht40 = 0;
    *shortGi = 0;
	return(descRate);
}

void rxBeginConfigAr5212
(
 A_UINT32 devNum
)
{
	LIB_DEV_INFO *pLibDev = gLibInfo.pLibDevArray[devNum];

	//mostly the same, so call into Ar5211
	rxBeginConfigAr5211(devNum);

	//enable wait for Poll if XR enabled
	if(pLibDev->libCfgParams.enableXR) {
		REGW(devNum, XRMODE_REG, REGR(devNum, XRMODE_REG) | (0x1 << BITS_TO_XR_WAIT_FOR_POLL));
	}

}
