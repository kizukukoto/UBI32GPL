/* mEeprom.c - contians functions for reading eeprom and getting pcdac power settings for all channels */
/* Copyright (c) 2001 Atheros Communications, Inc., All Rights Reserved */

#ident  "ACI $Id: //depot/sw/src/dk/mdk/devlib/mEeprom.c#187 $, $Header: //depot/sw/src/dk/mdk/devlib/mEeprom.c#187 $"

/*
Revsision history
--------------------
1.0       Created.
*/

#ifdef VXWORKS
#include "vxworks.h"
#endif

#ifdef __ATH_DJGPPDOS__
#include <unistd.h>
#ifndef EILSEQ
    #define EILSEQ EIO
#endif	// EILSEQ

#define __int64	long long
typedef unsigned long DWORD;
#define Sleep	delay
#endif	// #ifdef __ATH_DJGPPDOS__

#include "wlantype.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#ifdef WIN32
#include <memory.h>
#include <conio.h>
#endif
#include "athreg.h"
#include "manlib.h"
#include "mEeprom.h"
#include "mConfig.h"
#include "mDevtbl.h"
#include <errno.h>

#include "mEEP211.h"
#include "mEEPROM_d.h"
#include "mEEPROM_g.h"
#include "ar2413reg.h"
#ifdef LINUX
#include "mEep6000.h"
#endif
#include "mEep5416.h"
#include "mEepStruct5416.h"
#ifndef __ATH_DJGPPDOS__
extern void flash_read(A_UINT32,A_UINT32,A_UINT16 *,A_INT32,A_UINT32);
#endif

#if defined(SPIRIT_AP) || defined(FREEDOM_AP) || defined(OWL_AP)
// delta macro defined in both mEeprom.h and ar531x.h
// cannot #include ar531x.h
// declaring only the functions and MACROS needed from ar531x.h
A_UINT8 sysFlashConfigRead(A_UINT32 devNum, int fcl , int offset);
void sysFlashConfigWrite(A_UINT32 devNum, int fcl, int offset, A_UINT8 *data, int len);
#define FLC_RADIOCFG	2

#endif

A_UINT16	intercepts_pre3p2[] = {0,5,10,20,30,50,70,85,90,95,100};
A_UINT16	intercepts[] = {0,10,20,30,40,50,60,70,80,90,100} ;

#define NUM_INTERCEPTS	sizeof(intercepts)/sizeof(A_UINT16)
A_UINT16	channels_11b[] = {2412, 2447, 2484};
A_UINT16	channels_11g[] = {2312, 2412, 2484};

A_UINT16  curr_pwr_index_offset; // scale-up factor to make it Pmax referenced power table

static 	A_UINT16 cornerFixChannels[] = {5180, 5320, 5640, 5825};
#define NUM_CORNER_FIX_CHANNELS sizeof(cornerFixChannels)/sizeof(A_UINT16)
//static void writeCornerCal(A_UINT32 devNum, A_UINT16 channel,MDK_EEP_HEADER_INFO	*pHeaderInfo);

//this is static (and a structure), so that getCtlPowerInfo() can return this info
//from library
static CTL_POWER_INFO	   savedCtlPowerInfo;
A_UINT32 checkSumLengthLib=0x400; //default to 16k
A_UINT32 eepromSizeLib=0;

#if !defined(FREEDOM_AP)
extern void getAR6000Struct(A_UINT32 devNum, void **ppReturnStruct, A_UINT32 *pNumBytes);

extern void getAR5416Struct(A_UINT32 devNum, void **ppReturnStruct, A_UINT32 *pNumBytes);
#endif

EEP_OFFSET_STRUCT eep3_2 = {
    0xbf,		//HDR_COUNTRY_CODE
    0xc2,		//HDR_MODE_DEVICE_INFO
    0xc4,		//HDR_ANTENNA_GAIN
	0xc9,       //HEADER_MAC_FEATURES
    0xc5,		//HDR_11A_COMMON
    0xd0,		//HDR_11B_COMMON
    0xda,		//HDR_11G_COMMON
    0xe4,		//HDR_CTL
    0xee,		//HDR_11A_SPECIFIC
    0xec,		//HDR_11B_SPECIFIC
    0xed,		//HDR_11G_SPECIFIC
	0x100,		//GROUP1_11A_FREQ_PIERS
    0x105,		//GROUP2_11A_RAW_PWR
    0x137,		//GROUP3_11B_RAW_PWR
    0x146,		//GROUP4_11G_RAW_PWR
    0x155,		//GROUP5_11A_TRGT_PWR
    0x165,		//GROUP6_11B_TRGT_PWR
    0x169,		//GROUP7_11G_TRGT_PWR
    0x16f,		//GROUP8_CTL_INFO
};

EEP_OFFSET_STRUCT eep3_3 = {
    0xbf,		//HDR_COUNTRY_CODE
    0xc2,		//HDR_MODE_DEVICE_INFO
    0xc3,		//HDR_ANTENNA_GAIN
	0xc9,        //HEADER_MAC_FEATURES
    0xd4,		//HDR_11A_COMMON
    0xf2,		//HDR_11B_COMMON
    0x10d,		//HDR_11G_COMMON
    0x128,		//HDR_CTL
    0xe0,		//HDR_11A_SPECIFIC
    0xfc,		//HDR_11B_SPECIFIC
    0x117,		//HDR_11G_SPECIFIC
	0x150,		//GROUP1_11A_FREQ_PIERS
    0x155,		//GROUP2_11A_RAW_PWR
    0x187,		//GROUP3_11B_RAW_PWR
    0x196,		//GROUP4_11G_RAW_PWR
    0x1a5,		//GROUP5_11A_TRGT_PWR
    0x1b5,		//GROUP6_11B_TRGT_PWR
    0x1b9,		//GROUP7_11G_TRGT_PWR
    0x1bf,		//GROUP8_CTL_INFO
};


EEP_OFFSET_STRUCT *pOffsets = NULL;


/**************************************************************************
* mapPcdacTable - Create full pcdac table from limited table
*
* Given the limited EEPROM table, creates a full linear pcdac to power table
* by extrapolating the power for the intermediate pcdac values
*
* RETURNS: Fills in the full pcdac to power table
*/
void
mapPcdacTable
(
 MDK_PCDACS_EEPROM *pSrcStruct,					//limited input table
 MDK_FULL_PCDAC_STRUCT *pPcdacStruct			//pointer to table to complete
)
{
	A_UINT16			*pPcdacValue;
	A_INT16				*pPwrValue;
	A_UINT16			i, j;
	A_UINT16			numPcdacs = 0;

	//create the pcdac values
	for(i = PCDAC_START, j = 0; i <= PCDAC_STOP; i+= PCDAC_STEP, j++) {
		numPcdacs++;
		pPcdacStruct->PcdacValues[j] = i;
	}

	pPcdacStruct->numPcdacValues = numPcdacs;
	pPcdacStruct->pcdacMin = pPcdacStruct->PcdacValues[0];
	pPcdacStruct->pcdacMax = pPcdacStruct->PcdacValues[numPcdacs - 1];

	pPcdacValue = pPcdacStruct->PcdacValues;
	pPwrValue = pPcdacStruct->PwrValues;
	//for each of the pcdac values, calculate the power
	for(j = 0; j < pPcdacStruct->numPcdacValues; j++ ) {
		*pPwrValue = getScaledPower(pPcdacStruct->channelValue, *pPcdacValue, pSrcStruct);
		pPcdacValue++;
		pPwrValue++;
    }
	return;
}

/**************************************************************************
* getScaledPower - Calculate power from pcdac values by extrapolation
*
* Either find the power value for the pcdac in the input table
* or extrapolate between 2 nearest pcdac and channel values to get the power
* Power returned is scaled to avoid use of floats
*
* RETURNS: scaled Power value for a given pcdac
*/
A_INT16
getScaledPower
(
 A_UINT16			channel,			//which channel, power is needed for
 A_UINT16			pcdacValue,			//which pcdac value, power is needed for
 MDK_PCDACS_EEPROM		*pSrcStruct			//input structure to search
)
{
	A_INT16	     powerValue;
	A_UINT16	lFreq, rFreq;			//left and right frequency values
	A_UINT16	llPcdac, ulPcdac;		//lower and upper left pcdac values
	A_UINT16	lrPcdac, urPcdac;		//lower and upper right pcdac values
	A_INT16	    lPwr, uPwr;				//lower and upper temp pwr values
	A_INT16	lScaledPwr, rScaledPwr;	//left and right scaled power

	//if value pcdac is part of the input set then can return
	if(findValueInList(channel, pcdacValue, pSrcStruct, &powerValue)){
		//value was copied from srcStruct
		return(powerValue);
	}

	//find nearest channel and pcdac value to extrapolate between
	getLeftRightChannels(channel, pSrcStruct, &lFreq, &rFreq);
	getLowerUpperPcdacs(pcdacValue, lFreq, pSrcStruct, &llPcdac, &ulPcdac);
	getLowerUpperPcdacs(pcdacValue, rFreq, pSrcStruct, &lrPcdac, &urPcdac);

	//get the power index for the upper and lower pcdac
	findValueInList(lFreq, llPcdac, pSrcStruct, &lPwr);
	findValueInList(lFreq, ulPcdac, pSrcStruct, &uPwr);
	lScaledPwr = getInterpolatedValue( pcdacValue, llPcdac, ulPcdac, lPwr, uPwr, 0);

	findValueInList(rFreq, lrPcdac, pSrcStruct, &lPwr);
	findValueInList(rFreq, urPcdac, pSrcStruct, &uPwr);
	rScaledPwr = getInterpolatedValue( pcdacValue, lrPcdac, urPcdac, lPwr, uPwr, 0);

	//finally get the power for this pcdac
	return(getInterpolatedValue( channel, lFreq, rFreq, lScaledPwr, rScaledPwr, 0));
}


/**************************************************************************
* findValueInList - Search for pcdac value for a given channel
*
* Search input table to see if already have a pcdac/power mapping for
* the given pcdac value at the given channel
*
* RETURNS: TRUE (and fills in power value) if found. FALSE otherwise
*/
A_BOOL
findValueInList
(
 A_UINT16			channel,			//which channel, power is needed for
 A_UINT16			pcdacValue,			//which pcdac value, power is needed for
 MDK_PCDACS_EEPROM		*pSrcStruct,		//input structure to search
 A_INT16			*powerValue			//return power value found in src struct
)
{
	MDK_DATA_PER_CHANNEL	*pChannelData;
	A_UINT16			*pPcdac;
	A_UINT16			i, j;

	pChannelData = pSrcStruct->pDataPerChannel;
	for(i = 0; i < pSrcStruct->numChannels; i++ ) {
		if (pChannelData->channelValue == channel) {
			pPcdac = pChannelData->PcdacValues;
			for(j = 0; j < pChannelData->numPcdacValues; j++ ) {
				if (*pPcdac == pcdacValue) {
					*powerValue = pChannelData->PwrValues[j];
					return(TRUE);
				}
				pPcdac++;
			}
	    }
		pChannelData++;
	}
	return(FALSE);
}

/**************************************************************************
* getInterpolatedValue - Calculate Interpolated value
*
* Use relationship between left and right source points then apply to the target
* points to perform the interpolation, scale the return value as required by the
* input flag
*
* RETURNS: the interpolated value, scaled or otherwise
*/
A_UINT16 getInterpolatedValue
(
 A_UINT16	source,				//Input source value
 A_UINT16	srcLeft,			//Value lower than source
 A_UINT16	srcRight,			//value greater than source
 A_UINT16	targetLeft,			//lower target value
 A_UINT16	targetRight,		//upper target value
 A_BOOL		scaleUp
)
{
  A_UINT16 returnValue;
  A_UINT16 lRatio;
  A_UINT16 scaleValue = SCALE;

  //to get an accurate ratio, always scale, if return scale needed, then don't scale back down
  if((targetLeft * targetRight) == 0) {
	return(0);
  }

  if (scaleUp) {
	scaleValue = 1;
  }

  if (abs(srcRight - srcLeft) > 0) {
    //note the ratio always need to be scaled, since it will be a fraction
    lRatio = (A_UINT16)((source - srcLeft)*SCALE / (srcRight - srcLeft));
    returnValue = (A_UINT16)((lRatio*targetRight + (SCALE-lRatio)*targetLeft)/scaleValue);
  }
  else {
	  returnValue = (A_UINT16)(targetLeft*(scaleUp ? SCALE : 1));
  }
  return (returnValue);
}




/**************************************************************************
* getLowerUpperValues - Get 2 values closest to search value in list
*
* Given a search value and a list of values, find the upper and lower values
* closest to it.If the value is one of the values in the list, or is within
* a small delta of a value in the list then return the same upper
* and lower value.  All arithmatic is scaled to avoid floating point.
*
* RETURNS: the upper and lower values
*/
void getLowerUpperValues
(
 A_UINT16	value,			//value to search for
 A_UINT16	*pList,			//ptr to the list to search
 A_UINT16	listSize,		//number of entries in list
 A_UINT16	*pLowerValue,	//return the lower value
 A_UINT16	*pUpperValue	//return the upper value
)
{
	A_UINT16	i;
	A_UINT16	listEndValue = *(pList + listSize - 1);
	A_UINT32	target = value * SCALE;

	//see if value is lower than the first value in the list
	//if so return first value
	if (target < (A_UINT32)(*pList * SCALE - DELTA)) {
		*pLowerValue = *pList;
		*pUpperValue = *pList;
		return;
	}

	//see if value is greater than last value in list
	//if so return last value
	if (target > (A_UINT32)(listEndValue * SCALE + DELTA)) {
		*pLowerValue = listEndValue;
		*pUpperValue = listEndValue;
		return;
	}

	//look for value being near or between 2 values in list
	for(i = 0; i < listSize; i++) {
		//if value is close to the current value of the list
		//then target is not between values, it is one of the values
		if (abs(pList[i]*SCALE - target) < DELTA) {
			*pLowerValue = pList[i];
			*pUpperValue = pList[i];
			return;
		}

		//look for value being between current value and next value
		//if so return these 2 values
		if (target < (A_UINT32)(pList[i + 1]*SCALE - DELTA)) {
			*pLowerValue = pList[i];
			*pUpperValue = pList[i + 1];
			return;
		}
	}
}


/**************************************************************************
* getLeftRightChannels - Get lower and upper channels from source struct.
*
* As above function, except pulls the channel list from the input struct
*
* RETURNS: the upper and lower channel values
*/
void getLeftRightChannels
(
 A_UINT16			channel,			//channel to search for
 MDK_PCDACS_EEPROM		*pSrcStruct,		//ptr to struct to search
 A_UINT16			*pLowerChannel,		//return lower channel
 A_UINT16			*pUpperChannel		//return upper channel
)
{
	getLowerUpperValues(channel, pSrcStruct->pChannelList, pSrcStruct->numChannels,
						pLowerChannel, pUpperChannel);
	return;
}

/**************************************************************************
* getLeftRightpcdacs - Get lower and upper pcdacs from source struct.
*
* As above function, except pulls the pcdac list from the input struct
*
* RETURNS: the upper and lower pcdac values
*/void getLowerUpperPcdacs
(
 A_UINT16			pcdac,				//pcdac to search for
 A_UINT16			channel,			//current channel
 MDK_PCDACS_EEPROM		*pSrcStruct,		//ptr to struct to search
 A_UINT16			*pLowerPcdac,		//return lower pcdac
 A_UINT16			*pUpperPcdac		//return upper pcdac
)
{
	MDK_DATA_PER_CHANNEL	*pChannelData;
	A_UINT16			i;

	//find the channel information
	pChannelData = pSrcStruct->pDataPerChannel;
	for (i = 0; i < pSrcStruct->numChannels; i++) {
		if(pChannelData->channelValue == channel) {
			break;
		}
		pChannelData++;
	}

	getLowerUpperValues(pcdac, pChannelData->PcdacValues, pChannelData->numPcdacValues,
						pLowerPcdac, pUpperPcdac);
	return;
}


/**************************************************************************
* getPwrTable - Get Linear power table from the linear pcdac table
*
* Given a linear pcdac table, calculate the pcdac values that are needed
* to get a linear power table
*
* RETURNS: Fill in the linear power table
*/
void getPwrTable
(
 MDK_FULL_PCDAC_STRUCT *pPcdacStruct,	//pointer to linear pcdac table
 A_UINT16			*pPwrTable		//ptr to power table to fill
)
{

	A_INT16			 minScaledPwr;
	A_INT16			 maxScaledPwr;
	A_INT16			 pwr;
	A_UINT16		 pcdacMin = 0;
	A_UINT16		 pcdacMax = 63;
	A_UINT16		 i, j;
	A_UINT16		 numPwrEntries = 0;
	A_UINT16		 scaledPcdac;

	minScaledPwr = pPcdacStruct->PwrValues[0];
	maxScaledPwr = pPcdacStruct->PwrValues[pPcdacStruct->numPcdacValues - 1];
	//find minimum and make monotonic
	for(j = 0; j < pPcdacStruct->numPcdacValues; j++) {
		if (minScaledPwr >= pPcdacStruct->PwrValues[j]) {
			minScaledPwr = pPcdacStruct->PwrValues[j];
			pcdacMin = j;
		}
		//make the list monotonically increasing otherwise interpolation algorithm will get fooled
		//gotta start working from the top, hence i = numpcdacs - j.
		i = (A_UINT16)(pPcdacStruct->numPcdacValues - 1 - j);
		if(i == 0) {
			break;
		}

		if (pPcdacStruct->PwrValues[i-1] > pPcdacStruct->PwrValues[i]) {
			//it could be a glitch, so make the power for this pcdac,
			//the same as the power from the next highest pcdac
			pPcdacStruct->PwrValues[i - 1] = pPcdacStruct->PwrValues[i];
		}
	}

	for(j = 0; j < pPcdacStruct->numPcdacValues; j++) {
		if (maxScaledPwr < pPcdacStruct->PwrValues[j]) {
			maxScaledPwr = pPcdacStruct->PwrValues[j];
			pcdacMax = j;
		}
	}

	//may not get control at lower power values,
	//Fill lower power values with min pcdac, until reach min pcdac/power value
	pwr = (A_UINT16)(PWR_STEP * ((minScaledPwr-PWR_MIN + PWR_STEP/2)/PWR_STEP)  + PWR_MIN);
	for (i = 0; i < (2 * (pwr - PWR_MIN)/SCALE + 1); i++) {
		*pPwrTable = pcdacMin;
		pPwrTable++;
		numPwrEntries++;
	}

	//interpolate the correct pcdac value for required power until reach max power
	i = 0;
	while (pwr < pPcdacStruct->PwrValues[pPcdacStruct->numPcdacValues - 1])
	{
		pwr += PWR_STEP;
		while ((pwr < pPcdacStruct->PwrValues[pPcdacStruct->numPcdacValues - 1]) &&  //stop if dbM > max_power_possible
			((pwr - pPcdacStruct->PwrValues[i])*(pwr - pPcdacStruct->PwrValues[i+1]) > 0))
		{
			i++;
		}
		scaledPcdac = (A_UINT16)(getInterpolatedValue(pwr, pPcdacStruct->PwrValues[i],
					  pPcdacStruct->PwrValues[i+1],
					  (A_UINT16)(pPcdacStruct->PcdacValues[i]*2),
					  (A_UINT16)(pPcdacStruct->PcdacValues[i+1]*2),
					  0 ) + 1); //scale by 2 and add 1 to enable round up or down as needed
		*pPwrTable = (A_UINT16)(scaledPcdac/2);
		if(*pPwrTable > pcdacMax) {
			*pPwrTable = pcdacMax;
		}
		pPwrTable++;
		numPwrEntries++;
	}

	//fill remainder of power table with max pcdac value if saturation was reached
	while ( numPwrEntries < PWR_TABLE_SIZE ) {
		*pPwrTable = *(pPwrTable - 1);
		pPwrTable++;
		numPwrEntries++;
	}
	return;
}

/**************************************************************************
* getPcdacInterceptsFromPcdacMinMax - Calculate pcdac values from percentage
*                                     intercepts
*
* Give the min and max pcdac values, use the percentage intercepts to calculate
* what the pcdac values should be
*
* RETURNS: Fill in the the pcdac values
*/
void getPcdacInterceptsFromPcdacMinMax
(
	A_UINT16	pcdacMin,		//minimum pcdac value
	A_UINT16	pcdacMax,		//maximim pcdac value
	A_UINT16	*pPcdacValues	//return the pcdac value
)
{
	A_UINT16	i;

	//loop for the percentages in steps or 5
	for(i = 0; i < NUM_INTERCEPTS; i++ ) {
		*pPcdacValues =  (A_UINT16)((intercepts[i] * pcdacMax + (100-intercepts[i]) * pcdacMin)/100);
		pPcdacValues++;
	}
	return;
}


/**************************************************************************
* getMaxRDPowerlistForFreq - Get the max power values for this channel
*
* Use the test group data read in from the eeprom to get the max power
* values for the channel
*
* RETURNS: Fill in the max power for each rate
*/
void getMaxRDPowerlistForFreq
(
 A_UINT32		devNum,
 A_UINT16		channel,			//input channel value
 A_INT16		*pRatesPower		//pointer to power/rate table to fill

)
{
    LIB_DEV_INFO	*pLibDev = gLibInfo.pLibDevArray[devNum];
	A_UINT16 lowerChannel, lowerIndex=0, lowerPower=0;
	A_UINT16 upperChannel, upperIndex=0, upperPower=0;
	A_INT16 twiceMaxEdgePower=63;
	A_INT16 twicePower = 0;
	A_INT32 ctlPower = NO_CTL_IN_EEPROM;
	A_UINT16 i;
	A_UINT16 tempChannelList[NUM_16K_EDGES];		//temp array for holding edge channels
	A_UINT16 numChannels = 0;
	MDK_TRGT_POWER_INFO *pPowerInfo = NULL;

#if defined(LINUX)
	if (isDragon(devNum)) {
	   ar6000GetPowerPerRateTable(devNum, channel, pRatesPower);
	   return;
	}
#endif

	if(isOwl(pLibDev->swDevID)) {
	   ar5416GetPowerPerRateTable(devNum, channel, pRatesPower);
	   return;
	}

	switch(pLibDev->mode) {
	case MODE_11A:
		numChannels = pLibDev->p16KTrgtPowerInfo->numTargetPwr_11a;
		pPowerInfo = pLibDev->p16KTrgtPowerInfo->trgtPwr_11a;
		pRatesPower[XR_POWER_INDEX] = pLibDev->p16kEepHeader->info11a.xrTargetPower;
		break;

	case MODE_11G:
	case MODE_11O:
		numChannels = pLibDev->p16KTrgtPowerInfo->numTargetPwr_11g;
		pPowerInfo = pLibDev->p16KTrgtPowerInfo->trgtPwr_11g;
		pRatesPower[XR_POWER_INDEX] = pLibDev->p16kEepHeader->info11g.xrTargetPower;
		break;

	case MODE_11B:
		numChannels = pLibDev->p16KTrgtPowerInfo->numTargetPwr_11b;
		pPowerInfo = pLibDev->p16KTrgtPowerInfo->trgtPwr_11b;
		pRatesPower[XR_POWER_INDEX] = 0;
		break;
	}

	//extrapolate the power values for the test Groups
	for (i = 0; i < numChannels; i++) {
		tempChannelList[i] = pPowerInfo[i].testChannel;
	}

	getLowerUpperValues(channel, tempChannelList, numChannels, &lowerChannel, &upperChannel);

	//get the index for the channel
	for (i = 0; i < numChannels; i++) {
		if (lowerChannel == tempChannelList[i]) {
			lowerIndex = i;
		}

		if (upperChannel == tempChannelList[i]) {
			upperIndex = i;
			break;
		}
	}


	for(i = 0; i < 8; i++) {
		if(pLibDev->mode != MODE_11B) {
			//power for rates 6,9,12,18,24 are all the same
			if (i < 5) {
				lowerPower = pPowerInfo[lowerIndex].twicePwr6_24;
				upperPower = pPowerInfo[upperIndex].twicePwr6_24;
			}
			if (i == 5) {
				lowerPower = pPowerInfo[lowerIndex].twicePwr36;
				upperPower = pPowerInfo[upperIndex].twicePwr36;
			}
			if (i == 6) {
				lowerPower = pPowerInfo[lowerIndex].twicePwr48;
				upperPower = pPowerInfo[upperIndex].twicePwr48;
			}
			if (i == 7) {
				lowerPower = pPowerInfo[lowerIndex].twicePwr54;
				upperPower = pPowerInfo[upperIndex].twicePwr54;
			}
		} else {
			switch(i) {
			case 0:
			case 1:
				lowerPower = pPowerInfo[lowerIndex].twicePwr6_24;
				upperPower = pPowerInfo[upperIndex].twicePwr6_24;
				break;

			case 2:
			case 3:
				lowerPower = pPowerInfo[lowerIndex].twicePwr36;
				upperPower = pPowerInfo[upperIndex].twicePwr36;
				break;

			case 4:
			case 5:
				lowerPower = pPowerInfo[lowerIndex].twicePwr48;
				upperPower = pPowerInfo[upperIndex].twicePwr48;
				break;

			case 6:
			case 7:
				lowerPower = pPowerInfo[lowerIndex].twicePwr54;
				upperPower = pPowerInfo[upperIndex].twicePwr54;
				break;
			}
		}

		twicePower = (A_UINT16)(getInterpolatedValue(channel, lowerChannel, upperChannel, lowerPower,
						upperPower, 0));

		if(twicePower > twiceMaxEdgePower) {
			twicePower = twiceMaxEdgePower;
		}

		if(pLibDev->libCfgParams.applyCtlLimit) {
			if(pLibDev->mode != MODE_11B) {
				ctlPower = getCtlPower(devNum, pLibDev->libCfgParams.ctlToApply, channel, pLibDev->mode, pLibDev->turbo);
			}
			//note if not ctl limit, this will fail, due to default value of ctlPower being high
			if(ctlPower < twicePower) {
				pRatesPower[i] = (A_INT16)ctlPower;
//				printf("Applying ctl power (x2) of %d to pRatesPower[%d]\n", ctlPower, i);
			} else {
				pRatesPower[i] = twicePower;
			}
		}
		else {
			pRatesPower[i] = twicePower;
		}
	}

	//fill in the CCK power values (fill for all modes)
	fillCCKMaxPower(devNum, channel, pRatesPower);
	return;
}

void fillCCKMaxPower
(
 A_UINT32		devNum,
 A_UINT16		channel,			//input channel value
 A_INT16		*pRatesPower		//pointer to power/rate table to fill
)
{
    LIB_DEV_INFO	*pLibDev = gLibInfo.pLibDevArray[devNum];
	A_UINT16 lowerChannel, lowerIndex=0;
	A_INT16  lowerPower=0;
	A_UINT16 upperChannel, upperIndex=0;
	A_INT16  upperPower=0;
	A_INT16 twiceMaxEdgePower=63;
	A_INT32 ctlPower = NO_CTL_IN_EEPROM;
	A_INT16 twicePower = 0;
	A_UINT16 i;
	A_UINT16 tempChannelList[NUM_16K_EDGES];		//temp array for holding edge channels
	A_UINT16 numChannels = 0;
	MDK_TRGT_POWER_INFO *pPowerInfo = NULL;

	numChannels = pLibDev->p16KTrgtPowerInfo->numTargetPwr_11b;
	pPowerInfo = pLibDev->p16KTrgtPowerInfo->trgtPwr_11b;

	//extrapolate the power values for the test Groups
	for (i = 0; i < numChannels; i++) {
		tempChannelList[i] = pPowerInfo[i].testChannel;
	}

	getLowerUpperValues(channel, tempChannelList, numChannels, &lowerChannel, &upperChannel);

	//get the index for the channel
	for (i = 0; i < numChannels; i++) {
		if (lowerChannel == tempChannelList[i]) {
			lowerIndex = i;
		}

		if (upperChannel == tempChannelList[i]) {
			upperIndex = i;
			break;
		}
	}


	for(i = 8; i < XR_POWER_INDEX; i++) {
		switch(i) {
		case 8:
			lowerPower = pPowerInfo[lowerIndex].twicePwr6_24;
			upperPower = pPowerInfo[upperIndex].twicePwr6_24;
			break;

		case 9:
		case 10:
			lowerPower = pPowerInfo[lowerIndex].twicePwr36;
			upperPower = pPowerInfo[upperIndex].twicePwr36;
			break;

		case 11:
		case 12:
			lowerPower = pPowerInfo[lowerIndex].twicePwr48;
			upperPower = pPowerInfo[upperIndex].twicePwr48;
			break;

		case 13:
		case 14:
			lowerPower = pPowerInfo[lowerIndex].twicePwr54;
			upperPower = pPowerInfo[upperIndex].twicePwr54;
			break;
		}

		twicePower = (A_UINT16)(getInterpolatedValue(channel, lowerChannel, upperChannel, lowerPower,
						upperPower, 0));


		if(twicePower > twiceMaxEdgePower) {
			twicePower = twiceMaxEdgePower;
		}

		if(pLibDev->libCfgParams.applyCtlLimit) {
			if((pLibDev->mode == MODE_11B) || (pLibDev->mode == MODE_11G) ) {
				ctlPower = getCtlPower(devNum, pLibDev->libCfgParams.ctlToApply, channel, MODE_11B, pLibDev->turbo);
			}

			if(ctlPower < twicePower) {
				pRatesPower[i] = (A_INT16)ctlPower;
//				printf("Applying ctl power (x2) of %d to pRatesPower[%d]\n", ctlPower, i);
			} else {
				pRatesPower[i] = twicePower;
			}
		}
		else {
			pRatesPower[i] = twicePower;
		}
	}

	return;
}


/**************************************************************************
* forcePCDACTable - Write the linear power - pcdac table to device registers
*
*
* RETURNS:
*/
MANLIB_API void forcePCDACTable
(
 A_UINT32		devNum,
 A_UINT16		*pPcdacs
)
{
	A_UINT16	regOffset;
	A_UINT16	i;
	A_UINT32	temp32;
  LIB_DEV_INFO *pLibDev = gLibInfo.pLibDevArray[devNum];
	A_UINT32    pcdac_shift = 8;
	A_UINT32    predist_scale = 0x00FF;
	A_BOOL      falconTrue;

	falconTrue = isFalcon(devNum);

	if (falconTrue) {
		pcdac_shift = 0;
		predist_scale = 0; // no predist in falcon
	}

	regOffset = 0x9800 + (608 << 2) ;
	for(i = 0; i < 32; i++) {
		temp32 = 0xffff & ((pPcdacs[2*i + 1] << pcdac_shift) | predist_scale);
		temp32 = (temp32 << 16) | (0xffff & ((pPcdacs[2*i] << pcdac_shift) | predist_scale));
		if (pLibDev->devMap.remoteLib) {
			pciValues[i].offset = regOffset;
			pciValues[i].baseValue = temp32;
		}
		else {
		    REGW(devNum, regOffset, temp32);
		}
		//printf("Snoop: regOffset = %x, regValue = %x\n", regOffset, temp32);
		regOffset += 4;
	}
	if (pLibDev->devMap.remoteLib) {
			sendPciWrites(devNum, pciValues, 32);
	}

}

/**************************************************************************
* forceChainPCDACTable - Write the linear power - pcdac table to device registers
*
*
* RETURNS:
*/
MANLIB_API void forceChainPCDACTable
(
 A_UINT32		devNum,
 A_UINT16		*pPcdacs,
 A_UINT32       chainNum
)
{
	A_UINT16	regOffset;
	A_UINT16	i;
	A_UINT32	temp32;
	A_UINT32    pcdac_shift = 8;
	A_UINT32    predist_scale = 0x00FF;
	A_BOOL      falconTrue;
	A_UINT32    regDelta = 0x0000;

	falconTrue = isFalcon(devNum);

	if (falconTrue) {
		pcdac_shift = 0;
		predist_scale = 0; // no predist in falcon
		regDelta = chainNum*0x1000;
	}

	regOffset = 0x9800 + (608 << 2) ;
	for(i = 0; i < 32; i++) {
		temp32 = 0xffff & ((pPcdacs[2*i + 1] << pcdac_shift) | predist_scale);
		temp32 = (temp32 << 16) | (0xffff & ((pPcdacs[2*i] << pcdac_shift) | predist_scale));
		REGW(devNum, regOffset + regDelta, temp32); // write cal data for chain 1
		regOffset += 4;
	}
}

/**************************************************************************
* forcePDADCTable - Write the pdadc table and gain boundaries to device
*                   registers 672-703
*
* RETURNS:  Nothing
*/
MANLIB_API void forcePDADCTable
(
 A_UINT32		devNum,
 A_UINT16		*pPdadcs,
 A_UINT16		*pGainBoundaries
)
{
	A_UINT16	regOffset;
	A_UINT16	i;
	A_UINT32	temp32;

	regOffset = 0x9800 + (672 << 2) ;
	for(i = 0; i < 32; i++) {
		temp32 = ((pPdadcs[4*i + 0] & 0xFF) << 0)  |
			     ((pPdadcs[4*i + 1] & 0xFF) << 8)  |
				 ((pPdadcs[4*i + 2] & 0xFF) << 16) |
				 ((pPdadcs[4*i + 3] & 0xFF) << 24) ;
		REGW(devNum, regOffset, temp32);
		//printf("Snoop: regOffset = %x, regValue = %x\n", regOffset, temp32);
		regOffset += 4;
	}

	//cant use field names incase we are loading ear, these don't exist in non griffin
	//config files.
	temp32 = ((pGainBoundaries[0] & 0x3f) << 4)  |
			 ((pGainBoundaries[1] & 0x3f) << 10) |
			 ((pGainBoundaries[2] & 0x3f) << 16) |
			 ((pGainBoundaries[3] & 0x3f) << 22) ;
	REGW(devNum, 0xa26c, (REGR(devNum, 0xa26c) & 0xf)| temp32);

	//writeField(devNum, "bb_pd_gain_boundary_1", pGainBoundaries[0]);
	//writeField(devNum, "bb_pd_gain_boundary_2", pGainBoundaries[1]);
	//writeField(devNum, "bb_pd_gain_boundary_3", pGainBoundaries[2]);
	//writeField(devNum, "bb_pd_gain_boundary_4", pGainBoundaries[3]);
}



/**************************************************************************
* fbin2freq - Get channel value from binary representation held in eeprom
*
*
* RETURNS: the frequency in MHz
*/
A_UINT16 fbin2freq(A_UINT32 devNum, A_UINT16 fbin)
{
	A_UINT16 returnValue;
    LIB_DEV_INFO	*pLibDev = gLibInfo.pLibDevArray[devNum];

	if((pLibDev->p16kEepHeader->majorVersion == 3) && (pLibDev->p16kEepHeader->minorVersion <= 2)) {
		returnValue = (fbin>62) ? (A_UINT16)(5100 + 10*62 + 5*(fbin-62)) : (A_UINT16)(5100 + 10*fbin);
	}
	else {
		returnValue = (A_UINT16)(4800 + 5*fbin);
	}
	return returnValue;
}

A_UINT16 fbin2freq_2p4(A_UINT32 devNum, A_UINT16 fbin)
{
	A_UINT16 returnValue;
    LIB_DEV_INFO	*pLibDev = gLibInfo.pLibDevArray[devNum];

	if((pLibDev->p16kEepHeader->majorVersion == 3) && (pLibDev->p16kEepHeader->minorVersion <= 2)) {
		returnValue = (A_UINT16)(2400 + fbin);
	}
	else {
		returnValue = (A_UINT16)(2300 + fbin);
	}
	return returnValue;
}


/**************************************************************************
* allocateEepStructs - Allocate structs to hold eeprom contents
*
* Fills in pointers of the LIB_DEV_INFO struct
*
* RETURNS: TRUE if successfully allocated, false otherwise
*/
A_BOOL allocateEepStructs
(
 A_UINT32			devNum
)
{
    LIB_DEV_INFO *pLibDev = gLibInfo.pLibDevArray[devNum];
	A_UINT32	 i;

	//check to see if we already allocated thes stucts, if so, don't do again
	//if one ptrs is not null then so will the rest.
	if(pLibDev->pCalibrationInfo) {
		return(TRUE);
	}

	//allocate the struct to hold the pcdac/power info
	pLibDev->pCalibrationInfo = (MDK_PCDACS_ALL_MODES *)malloc(sizeof(MDK_PCDACS_ALL_MODES));
	if(NULL == pLibDev->pCalibrationInfo) {
		mError(devNum, ENOMEM, "Device Number %d:Device Number %d:Unable to allocate eeprom structure for pcdac/power info\n", devNum);
		return FALSE;
	}

	memset(pLibDev->pCalibrationInfo, 0, sizeof(MDK_PCDACS_ALL_MODES));

	pLibDev->pCalibrationInfo->numChannels_11a = NUM_11A_EEPROM_CHANNELS;
	pLibDev->pCalibrationInfo->numChannels_2_4 = NUM_2_4_EEPROM_CHANNELS;

	for(i = 0; i < NUM_11A_EEPROM_CHANNELS; i ++) {
		pLibDev->pCalibrationInfo->DataPerChannel_11a[i].numPcdacValues = NUM_PCDAC_VALUES;
	}

	//allocate the struct to hold the header info
	pLibDev->p16kEepHeader = (MDK_EEP_HEADER_INFO *)malloc(sizeof(MDK_EEP_HEADER_INFO));
	if(NULL ==pLibDev->p16kEepHeader) {
		mError(devNum, ENOMEM, "Device Number %d:Device Number %d:Unable to allocate eeprom structure for header info\n", devNum);
		freeEepStructs(devNum);
		return FALSE;
	}
	memset(pLibDev->p16kEepHeader, 0, sizeof(MDK_EEP_HEADER_INFO));

	//allocate the structure to hold target power info
	pLibDev->p16KTrgtPowerInfo = (MDK_TRGT_POWER_ALL_MODES *)malloc(sizeof(MDK_TRGT_POWER_ALL_MODES));
	if(NULL == pLibDev->p16KTrgtPowerInfo) {
		mError(devNum, ENOMEM, "Device Number %d:Device Number %d:Unable to allocate eeprom structure for target power info\n", devNum);
		freeEepStructs(devNum);
		return FALSE;
	}
	memset(pLibDev->p16KTrgtPowerInfo, 0, sizeof(MDK_TRGT_POWER_ALL_MODES));
	pLibDev->p16kEepHeader->scaledOfdmCckDelta = TENX_OFDM_CCK_DELTA_INIT;
	pLibDev->p16kEepHeader->scaledCh14FilterCckDelta = TENX_CH14_FILTER_CCK_DELTA_INIT;
	pLibDev->p16kEepHeader->ofdmCckGainDeltaX2 = OFDM_CCK_GAIN_DELTA_INIT;

//	pLibDev->libCfgParams.chainSelect = 0;
	pLibDev->startOfRfPciValues  = 0;

	//allocate structure for RD edges
	pLibDev->p16KRdEdgesPower = (MDK_RD_EDGES_POWER *)malloc(sizeof(MDK_RD_EDGES_POWER) * NUM_16K_EDGES * MAX_NUM_CTL);
	if(NULL == pLibDev->p16KRdEdgesPower) {
		mError(devNum, ENOMEM, "Device Number %d:Device Number %d:Unable to allocate eeprom structure for RD edges info\n", devNum);
		freeEepStructs(devNum);
		return FALSE;
	}
	memset(pLibDev->p16KRdEdgesPower, 0, sizeof(MDK_RD_EDGES_POWER) * NUM_16K_EDGES * MAX_NUM_CTL);

	return TRUE;
}


/**************************************************************************
* freeEepStructs - Free EEPROM structs from LIB_DEV_INFO
*
* Generic free function, has to check for allocation before free since
* not all structs may be allocated.
*
* RETURNS:
*/
void freeEepStructs
(
 A_UINT32			devNum
)
{
    LIB_DEV_INFO *pLibDev = gLibInfo.pLibDevArray[devNum];
	A_UINT16 i;

	if(pLibDev->pCalibrationInfo != NULL) {
		free(pLibDev->pCalibrationInfo);
		pLibDev->pCalibrationInfo = NULL;
	}

	if(pLibDev->p16kEepHeader != NULL) {
		free(pLibDev->p16kEepHeader);
		pLibDev->p16kEepHeader = NULL;
	}

	if(pLibDev->p16KTrgtPowerInfo != NULL) {
		free(pLibDev->p16KTrgtPowerInfo);
		pLibDev->p16KTrgtPowerInfo = NULL;
	}

	if (pLibDev->p16KRdEdgesPower != NULL) {
		free(pLibDev->p16KRdEdgesPower);
		pLibDev->p16KRdEdgesPower = NULL;
	}

	if(pLibDev->pGen3CalData != NULL) {
		free(pLibDev->pGen3CalData);
		pLibDev->pGen3CalData = NULL;
	}

	if (isFalcon(devNum)) {
		if(pLibDev->chain[1].pGen3CalData != NULL) {
			free(pLibDev->chain[1].pGen3CalData);
			pLibDev->chain[1].pGen3CalData = NULL;
		}
	}
	if(pLibDev->pGen5CalData != NULL) {
		free(pLibDev->pGen5CalData);
		pLibDev->pGen5CalData = NULL;
	}
	for(i = 0; i < 3; i++) {
		if(pLibDev->pGen3RawData[i] != NULL) {
			free(pLibDev->pGen3RawData[i]);
			pLibDev->pGen3RawData[i] = NULL;
		}
		if(pLibDev->pGen5RawData[i] != NULL) {
			free(pLibDev->pGen5RawData[i]);
			pLibDev->pGen5RawData[i] = NULL;
		}
	}
	return;
}


/**************************************************************************
* readEepromIntoDataset - Read eeprom contents into structs held by LIB_DEV_INFO
*
*
*
* RETURNS: TRUE of OK, FALSE otherwise
*/
A_BOOL readEepromIntoDataset
(
 A_UINT32			devNum
)
{
    LIB_DEV_INFO *pLibDev = gLibInfo.pLibDevArray[devNum];
	A_UINT32			tempValue;
	A_UINT16			i, j, jj;
	A_UINT16			offset = 0;
	MDK_RD_EDGES_POWER		*pRdEdgePwrInfo = pLibDev->p16KRdEdgesPower;
	MDK_TRGT_POWER_INFO	    *pPowerInfo = NULL;
	A_UINT16			numChannels = 0;
	A_UINT16			mode;
	A_UINT16			*pNumTrgtChannels = NULL;
	A_UINT16			sizeCtl;
	EEPROM_DATA_STRUCT_GEN3 *pTempGen3CalData[3];
	EEPROM_DATA_STRUCT_GEN5  *pTempGen5CalData[3];

	// for dual 11a support, start reading eeprom_block1 from location 0x400
	if ((pLibDev->devMap.devIndex == 1) && (pLibDev->libCfgParams.eepStartLocation > 0)) {
		pLibDev->eepromStartLoc = pLibDev->libCfgParams.eepStartLocation;
	}

	//verify the checksum
	if (!eepromVerifyChecksum(devNum)) {
		mError(devNum, EIO, "Device Number %d: eeprom checksum failed\n", devNum);
		return FALSE;
	}

	//get the version information
	tempValue = eepromRead(devNum, (HDR_VERSION + pLibDev->eepromStartLoc));
	pLibDev->p16kEepHeader->majorVersion = (A_UINT16)((tempValue >> 12) & 0x0f);
	pLibDev->p16kEepHeader->minorVersion = (A_UINT16)(tempValue & 0x0fff);


	if((pLibDev->p16kEepHeader->majorVersion == 3) && (pLibDev->p16kEepHeader->minorVersion <= 2)) {
		pOffsets = &eep3_2;
		pLibDev->p16kEepHeader->numCtl = NUM_CTL_EEP3_2;
	}
	else if(((pLibDev->p16kEepHeader->majorVersion == 3) && (pLibDev->p16kEepHeader->minorVersion >= 3)) ||
			(pLibDev->p16kEepHeader->majorVersion >= 4))
	{
		pOffsets = &eep3_3;
		pLibDev->p16kEepHeader->numCtl = NUM_CTL_EEP3_3;
	}
	else {
		mError(devNum, EIO, "Device Number %d:Device Number %d:eeprom version not supported\n", devNum);
		return (FALSE);
	}

	if (!(( pLibDev->p16kEepHeader->majorVersion > 3) ||
		( pLibDev->p16kEepHeader->majorVersion == 3) && (pLibDev->p16kEepHeader->minorVersion >= 2)))
	{
		for (jj=0; jj<NUM_INTERCEPTS; jj++)
		{
			intercepts[jj] = intercepts_pre3p2[jj] ;
		}
	}

	//the channel list for 2.4 is fixed, fill this in here
	for(i = 0; i < NUM_2_4_EEPROM_CHANNELS; i++) {
		pLibDev->pCalibrationInfo->Channels_11b[i] = channels_11b[i];
		if((pLibDev->p16kEepHeader->majorVersion == 3) && (pLibDev->p16kEepHeader->minorVersion <= 2)) {
			pLibDev->pCalibrationInfo->Channels_11g[i] = channels_11b[i];
		}
		else {
			pLibDev->pCalibrationInfo->Channels_11g[i] = channels_11g[i];
		}
		pLibDev->pCalibrationInfo->DataPerChannel_11b[i].numPcdacValues = NUM_PCDAC_VALUES;
		pLibDev->pCalibrationInfo->DataPerChannel_11g[i].numPcdacValues = NUM_PCDAC_VALUES;
	}

	//read the header information here
	readHeaderInfo(devNum, pLibDev->p16kEepHeader);

	if((pLibDev->p16kEepHeader->majorVersion == 3) ||
		((pLibDev->p16kEepHeader->majorVersion >= 4) && (pLibDev->p16kEepHeader->eepMap == 0)))
	{
		// not really need to correct for  pLibDev->eepromStartLoc as dual 11a mode is
		// only supported for eep_map = 1. done anyway.
		if(!readCalData_gen2(devNum, pLibDev->pCalibrationInfo)) {
			return (FALSE);
		}
	}
	else if((pLibDev->p16kEepHeader->majorVersion >= 4) && (pLibDev->p16kEepHeader->eepMap == 1)) {
		if(!initialize_datasets(devNum, pTempGen3CalData, pLibDev->pGen3RawData)) {
			return (FALSE);
		}

		//read eeprom data into contiguous struct.
		// need to pass up to ART for display
		if(pLibDev->pGen3CalData == NULL) {
			pLibDev->pGen3CalData = (EEPROM_FULL_DATA_STRUCT_GEN3 *)malloc(sizeof(EEPROM_FULL_DATA_STRUCT_GEN3));
			if(NULL == pLibDev->pGen3CalData) {
				mError(devNum, ENOMEM, "Device Number %d:Device Number %d:Unable to allocate memory for full gen3 eeprom struct\n", devNum);
				return (FALSE);
			}
		}

		//copy in the data
		memset(pLibDev->pGen3CalData, 0, sizeof(EEPROM_FULL_DATA_STRUCT_GEN3));
		copyGen3EepromStruct(pLibDev->pGen3CalData, pTempGen3CalData);

		if (isFalcon(devNum)) {

			tempValue = pLibDev->eepromStartLoc;
			if (pLibDev->libCfgParams.useEepromNotFlash) {
				pLibDev->eepromStartLoc = 0x400;
			} else {
				pLibDev->eepromStartLoc = 0x400;
			}

			if (pLibDev->chain[1].p16kEepHeader == NULL) {
				pLibDev->chain[1].p16kEepHeader = (MDK_EEP_HEADER_INFO *)malloc(sizeof(MDK_EEP_HEADER_INFO));
				if(NULL == pLibDev->chain[1].p16kEepHeader) {
					mError(devNum, ENOMEM, "Device Number %d:Device Number %d:Unable to allocate memory for chain 1 full gen3 eeprom struct\n", devNum);
					return (FALSE);
				}
			}

			memcpy(pLibDev->chain[1].p16kEepHeader, pLibDev->p16kEepHeader, sizeof(MDK_EEP_HEADER_INFO));

			read_chain1_antenna_control(devNum, pLibDev->chain[1].p16kEepHeader);

			if(!initialize_datasets(devNum, pTempGen3CalData, pLibDev->chain[1].pGen3RawData)) {
				return (FALSE);
			}

			//read eeprom data into contiguous struct
			if(pLibDev->chain[1].pGen3CalData == NULL) {
				pLibDev->chain[1].pGen3CalData = (EEPROM_FULL_DATA_STRUCT_GEN3 *)malloc(sizeof(EEPROM_FULL_DATA_STRUCT_GEN3));
				if(NULL == pLibDev->chain[1].pGen3CalData) {
					mError(devNum, ENOMEM, "Device Number %d:Device Number %d:Unable to allocate memory for chain 1 full gen3 eeprom struct\n", devNum);
					return (FALSE);
				}

			}

			//copy in the data
			memset(pLibDev->chain[1].pGen3CalData, 0, sizeof(EEPROM_FULL_DATA_STRUCT_GEN3));
			copyGen3EepromStruct(pLibDev->chain[1].pGen3CalData, pTempGen3CalData);

			pLibDev->eepromStartLoc = tempValue;
		}

		//#######TO DO would free pTempGen3CalData here
		for(i = 0; i < 3; i++) {
			if(pTempGen3CalData[i]) {
				free(pTempGen3CalData[i]);
			}
		}

	}

	else if((pLibDev->p16kEepHeader->majorVersion >= 5) && (pLibDev->p16kEepHeader->eepMap == 2)) {
		if(!initialize_datasets_gen5(devNum, pTempGen5CalData, pLibDev->pGen5RawData)) {
			return (FALSE);
		}

		//read eeprom data into contiguous struct.
		// need to pass up to ART for display
		if(pLibDev->pGen5CalData == NULL) {
			pLibDev->pGen5CalData = (EEPROM_FULL_DATA_STRUCT_GEN5 *)malloc(sizeof(EEPROM_FULL_DATA_STRUCT_GEN5));
			if(NULL == pLibDev->pGen5CalData) {
				mError(devNum, ENOMEM, "Device Number %d:Device Number %d:Unable to allocate memory for full gen5 eeprom struct\n", devNum);
				return (FALSE);
			}
		}

		//copy in the data
		memset(pLibDev->pGen5CalData, 0, sizeof(EEPROM_FULL_DATA_STRUCT_GEN5));
		copyGen5EepromStruct(pLibDev->pGen5CalData, pTempGen5CalData);

		for(i = 0; i < 3; i++) {
			if(pTempGen5CalData[i]) {
				free(pTempGen5CalData[i]);
			}
		}

	} // griffin eeprom read

	//read the power per rate info for test channels
	for(mode = 0; mode <= MAX_MODE; mode++) {
		switch(mode) {
		case MODE_11A:
			offset = pOffsets->GROUP5_11A_TRGT_PWR;
			if(pLibDev->p16kEepHeader->majorVersion >= 4) {
				offset = pLibDev->p16kEepHeader->trgtPowerStartLocation;
			}
			numChannels = NUM_TEST_FREQUENCIES;
			pPowerInfo = pLibDev->p16KTrgtPowerInfo->trgtPwr_11a;
			pNumTrgtChannels = &(pLibDev->p16KTrgtPowerInfo->numTargetPwr_11a);
			break;

		case MODE_11G:
		case MODE_11O:
			offset = pOffsets->GROUP7_11G_TRGT_PWR;
			if(pLibDev->p16kEepHeader->majorVersion >= 4) {
				offset = (A_UINT16)(pLibDev->p16kEepHeader->trgtPowerStartLocation + 0x14);
			}
			numChannels = 3;
			pPowerInfo = pLibDev->p16KTrgtPowerInfo->trgtPwr_11g;
			pNumTrgtChannels = &(pLibDev->p16KTrgtPowerInfo->numTargetPwr_11g);
			break;

		case MODE_11B:
			offset = pOffsets->GROUP6_11B_TRGT_PWR;
			if(pLibDev->p16kEepHeader->majorVersion >= 4) {
				offset = (A_UINT16)(pLibDev->p16kEepHeader->trgtPowerStartLocation + 0x10);
			}
			numChannels = 2;
			pPowerInfo = pLibDev->p16KTrgtPowerInfo->trgtPwr_11b;
			pNumTrgtChannels = &(pLibDev->p16KTrgtPowerInfo->numTargetPwr_11b);
			break;
		default:
			mError(devNum, EIO, "Device Number %d:Device Number %d:Bad mode in readEepromIntoDataset, internal software error, should not get here\n", devNum);
			return(FALSE);
		} //end switch

		*pNumTrgtChannels = 0;
		for(i = 0; i < numChannels; i++) {
			if(readTrgtPowers(devNum, (A_UINT16)(offset + pLibDev->eepromStartLoc), pPowerInfo, mode)) {
				(*pNumTrgtChannels)++;
			}
			pPowerInfo++;
			offset+=2;
		}

	}

	//read the CTL edge power limits

	i = 0;
	offset = pOffsets->GROUP8_CTL_INFO;
	if(pLibDev->p16kEepHeader->majorVersion >= 4) {
		offset = (A_UINT16)(pLibDev->p16kEepHeader->trgtPowerStartLocation + 0x1a);
	}
	while ((pLibDev->p16kEepHeader->testGroups[i] != 0) && (i < pLibDev->p16kEepHeader->numCtl)) {
		sizeCtl = readCtlInfo(devNum, (A_UINT16)(offset + pLibDev->eepromStartLoc), pRdEdgePwrInfo);

		for(j = 0; j < NUM_16K_EDGES; j++ ) {
			if ((pRdEdgePwrInfo[j].rdEdge==0) && (pRdEdgePwrInfo[j].twice_rdEdgePower==0))
			{
				pRdEdgePwrInfo[j].rdEdge = 0; // if not all rdedges were specified for this CTL
			} else
			{
				if(((pLibDev->p16kEepHeader->testGroups[i] & 0x7) == 0) ||
					((pLibDev->p16kEepHeader->testGroups[i] & 0x7) == 0x3)  ){ //turbo mode

					pRdEdgePwrInfo[j].rdEdge = fbin2freq(devNum, pRdEdgePwrInfo[j].rdEdge);
				}
				else {
					pRdEdgePwrInfo[j].rdEdge = fbin2freq_2p4(devNum, pRdEdgePwrInfo[j].rdEdge);
				}
			}
		}
		i++;
		offset = (A_UINT16)(offset + sizeCtl);
		pRdEdgePwrInfo+=NUM_16K_EDGES;
	}

	return TRUE;
}
// only searches for integer values in integer lists. used for channel and pcdac lists
void iGetLowerUpperValues
(
 A_UINT16	value,			//value to search for
 A_UINT16	*pList,			//ptr to the list to search
 A_UINT16	listSize,		//number of entries in list
 A_UINT16	*pLowerValue,	//return the lower value
 A_UINT16	*pUpperValue	//return the upper value
)
{
	A_UINT16	i;
	A_UINT16	listEndValue = *(pList + listSize - 1);
	A_UINT16	target = value ;

	//see if value is lower than the first value in the list
	//if so return first value
	if (target <= (*pList)) {
		*pLowerValue = *pList;
		*pUpperValue = *pList;
		return;
	}

	//see if value is greater than last value in list
	//if so return last value
	if (target >= listEndValue) {
		*pLowerValue = listEndValue;
		*pUpperValue = listEndValue;
		return;
	}

	//look for value being near or between 2 values in list
	for(i = 0; i < listSize; i++) {
		//if value is close to the current value of the list
		//then target is not between values, it is one of the values
		if (pList[i] == target) {
			*pLowerValue = pList[i];
			*pUpperValue = pList[i];
			return;
		}

		//look for value being between current value and next value
		//if so return these 2 values
		if (target < pList[i + 1]) {
			*pLowerValue = pList[i];
			*pUpperValue = pList[i + 1];
			return;
		}
	}
}



/**************************************************************************
* read_chain1_antenna_control - Read chain1 antenna control info
*
* RETURNS:
*/
void  read_chain1_antenna_control
(
 A_UINT32			devNum,
 MDK_EEP_HEADER_INFO	*pHeaderInfo		//ptr to header struct to fill
)
{
	A_UINT32		tempValue;
	A_UINT16		offset;
	A_UINT16        i;
	MODE_HEADER_INFO	*pModeInfo;
    LIB_DEV_INFO *pLibDev = gLibInfo.pLibDevArray[devNum];

	offset = pOffsets->HDR_11A_COMMON;
	tempValue = eepromRead(devNum, (offset + pLibDev->eepromStartLoc));
	pHeaderInfo->info11a.antennaControl[0] = (A_UINT16)((tempValue << 4) & 0x3f);

	offset++;
	tempValue = eepromRead(devNum, (offset + pLibDev->eepromStartLoc));
	pHeaderInfo->info11a.antennaControl[0] = (A_UINT16)(((tempValue >> 12) & 0x0f) | pHeaderInfo->info11a.antennaControl[0]);
	pHeaderInfo->info11a.antennaControl[1] = (A_UINT16)((tempValue >> 6) & 0x3f);
	pHeaderInfo->info11a.antennaControl[2] = (A_UINT16)(tempValue  & 0x3f);

	offset++;
	tempValue = eepromRead(devNum, (offset + pLibDev->eepromStartLoc));
	pHeaderInfo->info11a.antennaControl[3] = (A_UINT16)((tempValue >> 10)  & 0x3f);
	pHeaderInfo->info11a.antennaControl[4] = (A_UINT16)((tempValue >> 4)  & 0x3f);
	pHeaderInfo->info11a.antennaControl[5] = (A_UINT16)((tempValue << 2)  & 0x3f);

	offset++;
	tempValue = eepromRead(devNum, (offset + pLibDev->eepromStartLoc));
	pHeaderInfo->info11a.antennaControl[5] = (A_UINT16)(((tempValue >> 14)  & 0x03) | pHeaderInfo->info11a.antennaControl[5]);
	pHeaderInfo->info11a.antennaControl[6] = (A_UINT16)((tempValue >> 8)  & 0x3f);
	pHeaderInfo->info11a.antennaControl[7] = (A_UINT16)((tempValue >> 2)  & 0x3f);
	pHeaderInfo->info11a.antennaControl[8] = (A_UINT16)((tempValue << 4)  & 0x3f);

	offset++;
	tempValue = eepromRead(devNum, (offset + pLibDev->eepromStartLoc));
	pHeaderInfo->info11a.antennaControl[8] = (A_UINT16)(((tempValue >> 12)  & 0x0f) | pHeaderInfo->info11a.antennaControl[8]);
	pHeaderInfo->info11a.antennaControl[9] = (A_UINT16)((tempValue >> 6)  & 0x3f);
	pHeaderInfo->info11a.antennaControl[10] = (A_UINT16)(tempValue & 0x3f);

	//11b and g antenna control
	//start by reading b
	pModeInfo = &(pHeaderInfo->info11b);
	offset = pOffsets->HDR_11B_COMMON;
	for (i = 0; i < 2; i++) {
		tempValue = eepromRead(devNum, (offset + pLibDev->eepromStartLoc));
		pModeInfo->antennaControl[0] = (A_UINT16)((tempValue << 4) & 0x3f);

		tempValue = eepromRead(devNum, (offset + 1 + pLibDev->eepromStartLoc));
		pModeInfo->antennaControl[0] = (A_UINT16)(((tempValue >> 12) & 0x0f) | pModeInfo->antennaControl[0]);
		pModeInfo->antennaControl[1] = (A_UINT16)((tempValue >> 6) & 0x3f);
		pModeInfo->antennaControl[2] = (A_UINT16)(tempValue  & 0x3f);

		tempValue = eepromRead(devNum, (offset + 2 + pLibDev->eepromStartLoc));
		pModeInfo->antennaControl[3] = (A_UINT16)((tempValue >> 10)  & 0x3f);
		pModeInfo->antennaControl[4] = (A_UINT16)((tempValue >> 4)  & 0x3f);
		pModeInfo->antennaControl[5] = (A_UINT16)((tempValue << 2)  & 0x3f);

		tempValue = eepromRead(devNum, (offset + 3 + pLibDev->eepromStartLoc));
		pModeInfo->antennaControl[5] = (A_UINT16)(((tempValue >> 14)  & 0x03) | pModeInfo->antennaControl[5]);
		pModeInfo->antennaControl[6] = (A_UINT16)((tempValue >> 8)  & 0x3f);
		pModeInfo->antennaControl[7] = (A_UINT16)((tempValue >> 2)  & 0x3f);
		pModeInfo->antennaControl[8] = (A_UINT16)((tempValue << 4)  & 0x3f);

		tempValue = eepromRead(devNum, (offset + 4 + pLibDev->eepromStartLoc));
		pModeInfo->antennaControl[8] = (A_UINT16)(((tempValue >> 12)  & 0x0f) | pModeInfo->antennaControl[8]);
		pModeInfo->antennaControl[9] = (A_UINT16)((tempValue >> 6)  & 0x3f);
		pModeInfo->antennaControl[10] = (A_UINT16)(tempValue & 0x3f);

		//change params to read g stuff
		pModeInfo = &(pHeaderInfo->info11g);
		offset = pOffsets->HDR_11G_COMMON;
	}

}

/**************************************************************************
* readHeaderInfo - Read eeprom header info
*
* RETURNS:
*/
void  readHeaderInfo
(
 A_UINT32			devNum,
 MDK_EEP_HEADER_INFO	*pHeaderInfo		//ptr to header struct to fill
)
{
	A_UINT32		tempValue;
	A_UINT16		offset;
	A_UINT16        i;
	MODE_HEADER_INFO	*pModeInfo;
    LIB_DEV_INFO *pLibDev = gLibInfo.pLibDevArray[devNum];
	A_UINT32        fullChainMask = 0x1; // backward compatiblity with single chain cards

	tempValue = eepromRead(devNum, (pOffsets->HDR_COUNTRY_CODE + pLibDev->eepromStartLoc));
	pHeaderInfo->countryRegCode = (A_UINT16)(tempValue);

	if(((pHeaderInfo->majorVersion == 3) && (pHeaderInfo->minorVersion >= 2))
			|| (pHeaderInfo->majorVersion >= 4)){
		pHeaderInfo->countryRegCode = (A_UINT16)(pHeaderInfo->countryRegCode & 0xfff);
		pHeaderInfo->countryCodeFlag = (A_UINT16)((tempValue >> 15) & 0x01);

	if(((pHeaderInfo->majorVersion == 3) && (pHeaderInfo->minorVersion >= 3))
			|| (pHeaderInfo->majorVersion >= 4)){
			pHeaderInfo->worldwideRoaming = (A_UINT16)((tempValue >> 14) & 0x01);
		}
	}
	else {
		pHeaderInfo->countryCodeFlag = 1;
	}


	tempValue = eepromRead(devNum, (pOffsets->HEADER_MAC_FEATURES + pLibDev->eepromStartLoc));
	pHeaderInfo->keyCacheSize = (A_UINT16)((tempValue >> 12) & 0xf);// Added by Giri
	pHeaderInfo->enableClip = (A_UINT16)((tempValue >> 9) & 0x1);// Added by Giri
	pHeaderInfo->maxNumQCU = (A_UINT16)((tempValue >> 4) & 0x1f);// Added by Giri
	pHeaderInfo->burstingDisable = (A_UINT16)((tempValue >> 3) & 0x1);// Added by Giri
	pHeaderInfo->fastFrameDisable = (A_UINT16)((tempValue >> 2) & 0x1);// Added by Giri
	pHeaderInfo->aesDisable = (A_UINT16)((tempValue >> 1) & 0x1);// Added by Giri
	pHeaderInfo->compressionDisable = (A_UINT16)(tempValue & 0x1);// Added by Giri

	if(((pHeaderInfo->majorVersion == 5) && (pHeaderInfo->minorVersion >= 3)) ||
		(pHeaderInfo->majorVersion >= 6)){
		tempValue = eepromRead(devNum, (pOffsets->HEADER_MAC_FEATURES + 1 + pLibDev->eepromStartLoc));
		pHeaderInfo->enableFCCMid = (A_UINT16)((tempValue >> 6) & 0x1);
		pHeaderInfo->enableJapanEvenU1 = (A_UINT16)((tempValue >> 7) & 0x1);
		pHeaderInfo->enableJapenU2 = (A_UINT16)((tempValue >> 8) & 0x1);
		pHeaderInfo->enableJapnMid = (A_UINT16)((tempValue >> 9) & 0x1);
		pHeaderInfo->disableJapanOddU1 = (A_UINT16)((tempValue >> 10) & 0x1);
		pHeaderInfo->enableJapanMode11aNew = (A_UINT16)((tempValue >> 11) & 0x1);
		pHeaderInfo->enableFCCDfsHt40 = (A_UINT16)((tempValue >> 12) & 0x1);
		pHeaderInfo->enableJapanHt40 = (A_UINT16)((tempValue >> 13) & 0x1);
		pHeaderInfo->enableJapanDfsHt40 = (A_UINT16)((tempValue >> 14) & 0x1);
	}

	tempValue = eepromRead(devNum, (pOffsets->HDR_MODE_DEVICE_INFO + pLibDev->eepromStartLoc));
	pHeaderInfo->info11a.turboDisable = (A_UINT16)((tempValue >> 15) & 0x01);// Added by Giri
	pHeaderInfo->RFKill = (A_UINT16)((tempValue >> 14) & 0x01);
	pHeaderInfo->deviceType = (A_UINT16)((tempValue >> 11) & 0x07);
	pHeaderInfo->info11g.turboDisable = (A_UINT16)((tempValue >> 3) & 0x01); // Added by Giri
	pHeaderInfo->Bmode = (A_UINT16)((tempValue >> 1) & 0x01);
	pHeaderInfo->Amode = (A_UINT16)(tempValue & 0x01);

	if(((pHeaderInfo->majorVersion == 3) && (pHeaderInfo->minorVersion >= 1))
			|| (pHeaderInfo->majorVersion >= 4)){
		pHeaderInfo->info11a.turbo.max2wPower = (A_UINT16)((tempValue >> 4) & 0x3f);
		pHeaderInfo->xtnd11a = (A_UINT16)((tempValue >> 3) & 0x01);
		pHeaderInfo->Gmode = (A_UINT16)((tempValue >> 2) & 0x01);

		tempValue = eepromRead(devNum, (pOffsets->HDR_ANTENNA_GAIN + pLibDev->eepromStartLoc));
		pHeaderInfo->antennaGain5 = (A_UINT16)((tempValue >> 8) & 0xff);
		pHeaderInfo->antennaGain2_4 = (A_UINT16)(tempValue & 0xff);
	}

	if(pHeaderInfo->majorVersion >= 4) {
		tempValue = eepromRead(devNum, (pOffsets->HDR_ANTENNA_GAIN + 1 + pLibDev->eepromStartLoc));
		pHeaderInfo->earStartLocation = (A_UINT16)(tempValue & 0xfff);
		pHeaderInfo->eepMap = (A_UINT16)((tempValue >> 14) & 0x3);
		pHeaderInfo->disableXR = (A_UINT16)(((tempValue >> 12) & 0x3) ? TRUE : FALSE);

		tempValue = eepromRead(devNum, (pOffsets->HDR_ANTENNA_GAIN + 2 + pLibDev->eepromStartLoc));
		pHeaderInfo->trgtPowerStartLocation = (A_UINT16)(tempValue & 0xfff);
		pHeaderInfo->enable32khz = (A_UINT16)((tempValue >> 15) & 0x1);

		if(((pHeaderInfo->majorVersion == 4) && (pHeaderInfo->minorVersion >= 5)) ||
			(pHeaderInfo->majorVersion >= 5)){
			pHeaderInfo->oldEnable32khz = (A_UINT16)((tempValue >> 15) & 0x1);
			pHeaderInfo->enable32khz = (A_UINT16)((tempValue >> 14) & 0x1);
		}
	}

	if(((pHeaderInfo->majorVersion == 4) && (pHeaderInfo->minorVersion >= 4)) ||
		(pHeaderInfo->majorVersion >= 5)){
		tempValue = eepromRead(devNum, (pOffsets->HDR_ANTENNA_GAIN + 3 + pLibDev->eepromStartLoc));
		pHeaderInfo->eepFileVersion = (A_UINT16)((tempValue >> 8) & 0xff);
		pHeaderInfo->earFileVersion = (A_UINT16)(tempValue & 0xff);

		tempValue = eepromRead(devNum, (pOffsets->HDR_ANTENNA_GAIN + 4 + pLibDev->eepromStartLoc));
		pHeaderInfo->artBuildNumber = (A_UINT16)((tempValue >> 10) & 0x3f);
		pHeaderInfo->earFileIdentifier = (A_UINT16)(tempValue & 0xff);
	}

	if(((pHeaderInfo->majorVersion == 4) && (pHeaderInfo->minorVersion >= 5)) ||
		(pHeaderInfo->majorVersion >= 5)){
		tempValue = eepromRead(devNum, (pOffsets->HDR_ANTENNA_GAIN + 5 + pLibDev->eepromStartLoc));
		pHeaderInfo->maskRadio0 = (A_UINT16)(tempValue & 0x3);
		pHeaderInfo->maskRadio1 = (A_UINT16)((tempValue >> 2) & 0x3);

		if(pHeaderInfo->eepMap >= 2) {
			pHeaderInfo->calStartLocation = (A_UINT16)((tempValue >> 4) & 0xfff);
		}
	}

	if(((pHeaderInfo->majorVersion == 4) && (pHeaderInfo->minorVersion >= 9)) ||
		(pHeaderInfo->majorVersion >= 5) ) {
		tempValue = eepromRead(devNum, (pOffsets->HDR_ANTENNA_GAIN + 6 + pLibDev->eepromStartLoc));
		if (isFalcon(devNum)) {
			fullChainMask = 0x3;
		}
		pHeaderInfo->tx_chain_mask = (A_UINT16)(((tempValue & 0x7) ^ 0x7) & fullChainMask);
		pHeaderInfo->rx_chain_mask = (A_UINT16)((((tempValue >> 3) & 0x7) ^ 0x7) & fullChainMask);
		pLibDev->libCfgParams.chain_mask_updated_from_eeprom = TRUE;
	}

	offset = pOffsets->HDR_11A_COMMON;
	tempValue = eepromRead(devNum, (offset + pLibDev->eepromStartLoc));
	pHeaderInfo->info11a.switchSettling = (A_UINT16)((tempValue >> 8) & 0x7f);
	pHeaderInfo->info11a.txrxAtten = (A_UINT16)((tempValue >> 2) & 0x3f);
	pHeaderInfo->info11a.antennaControl[0] = (A_UINT16)((tempValue << 4) & 0x3f);

	offset++;
	tempValue = eepromRead(devNum, (offset + pLibDev->eepromStartLoc));
	pHeaderInfo->info11a.antennaControl[0] = (A_UINT16)(((tempValue >> 12) & 0x0f) | pHeaderInfo->info11a.antennaControl[0]);
	pHeaderInfo->info11a.antennaControl[1] = (A_UINT16)((tempValue >> 6) & 0x3f);
	pHeaderInfo->info11a.antennaControl[2] = (A_UINT16)(tempValue  & 0x3f);

	offset++;
	tempValue = eepromRead(devNum, (offset + pLibDev->eepromStartLoc));
	pHeaderInfo->info11a.antennaControl[3] = (A_UINT16)((tempValue >> 10)  & 0x3f);
	pHeaderInfo->info11a.antennaControl[4] = (A_UINT16)((tempValue >> 4)  & 0x3f);
	pHeaderInfo->info11a.antennaControl[5] = (A_UINT16)((tempValue << 2)  & 0x3f);

	offset++;
	tempValue = eepromRead(devNum, (offset + pLibDev->eepromStartLoc));
	pHeaderInfo->info11a.antennaControl[5] = (A_UINT16)(((tempValue >> 14)  & 0x03) | pHeaderInfo->info11a.antennaControl[5]);
	pHeaderInfo->info11a.antennaControl[6] = (A_UINT16)((tempValue >> 8)  & 0x3f);
	pHeaderInfo->info11a.antennaControl[7] = (A_UINT16)((tempValue >> 2)  & 0x3f);
	pHeaderInfo->info11a.antennaControl[8] = (A_UINT16)((tempValue << 4)  & 0x3f);

	offset++;
	tempValue = eepromRead(devNum, (offset + pLibDev->eepromStartLoc));
	pHeaderInfo->info11a.antennaControl[8] = (A_UINT16)(((tempValue >> 12)  & 0x0f) | pHeaderInfo->info11a.antennaControl[8]);
	pHeaderInfo->info11a.antennaControl[9] = (A_UINT16)((tempValue >> 6)  & 0x3f);
	pHeaderInfo->info11a.antennaControl[10] = (A_UINT16)(tempValue & 0x3f);

	offset++;
	tempValue = eepromRead(devNum, (offset + pLibDev->eepromStartLoc));
	pHeaderInfo->info11a.adcDesiredSize = (A_INT8)((tempValue >> 8)  & 0xff);
	pHeaderInfo->info11a.ob_4 = (A_UINT16)((tempValue >> 5)  & 0x07);
	pHeaderInfo->info11a.db_4 = (A_UINT16)((tempValue >> 2)  & 0x07);
	pHeaderInfo->info11a.ob_3 = (A_UINT16)((tempValue << 1)  & 0x07);

	offset++;
	tempValue = eepromRead(devNum, (offset + pLibDev->eepromStartLoc));
	pHeaderInfo->info11a.ob_3 = (A_UINT16)(((tempValue >> 15)  & 0x01) | pHeaderInfo->info11a.ob_3);
	pHeaderInfo->info11a.db_3 = (A_UINT16)((tempValue >> 12)  & 0x07);
	pHeaderInfo->info11a.ob_2 = (A_UINT16)((tempValue >> 9)  & 0x07);
	pHeaderInfo->info11a.db_2 = (A_UINT16)((tempValue >> 6)  & 0x07);
	pHeaderInfo->info11a.ob_1 = (A_UINT16)((tempValue >> 3)  & 0x07);
	pHeaderInfo->info11a.db_1 = (A_UINT16)(tempValue & 0x07);

	offset++;
	tempValue = eepromRead(devNum, (offset + pLibDev->eepromStartLoc));
	pHeaderInfo->info11a.txEndToXLNAOn = (A_UINT16)((tempValue >> 8)  & 0xff);
	pHeaderInfo->info11a.thresh62 = (A_UINT16)(tempValue  & 0xff);

	offset++;
	tempValue = eepromRead(devNum, (offset + pLibDev->eepromStartLoc));
	pHeaderInfo->info11a.txEndToXPAOff = (A_UINT16)((tempValue >> 8)  & 0xff);
	pHeaderInfo->info11a.txFrameToXPAOn = (A_UINT16)(tempValue  & 0xff);

	offset++;
	tempValue = eepromRead(devNum, (offset + pLibDev->eepromStartLoc));
	pHeaderInfo->info11a.pgaDesiredSize = (A_INT8)((tempValue >> 8)  & 0xff);
	pHeaderInfo->info11a.noisefloorThresh = (A_INT8)(tempValue  & 0xff);

	offset++;
	tempValue = eepromRead(devNum, (offset + pLibDev->eepromStartLoc));
	if(pHeaderInfo->majorVersion >= 4) {
		pHeaderInfo->fixedBiasA = (A_UINT16)((tempValue >> 13)  & 0x01);
	}
	pHeaderInfo->info11a.xlnaGain = (A_UINT16)((tempValue >> 5)  & 0xff);
	pHeaderInfo->info11a.xgain = (A_UINT16)((tempValue >> 1)  & 0x0f);
	pHeaderInfo->info11a.xpd = (A_UINT16)(tempValue  & 0x01);

	if(((pHeaderInfo->majorVersion == 3) && (pHeaderInfo->minorVersion >= 3))
			|| (pHeaderInfo->majorVersion >= 4)){
		offset++;
		tempValue = eepromRead(devNum, (offset + pLibDev->eepromStartLoc));
		pHeaderInfo->info11a.falseDetectBackoff = (A_UINT16)((tempValue >> 6)  & 0x7f);
	}

	if(((pHeaderInfo->majorVersion == 3) && (pHeaderInfo->minorVersion >= 4))
			|| (pHeaderInfo->majorVersion >= 4)){
		pHeaderInfo->info11a.initialGainI = (A_UINT16)((tempValue >> 13) & 0x07);
		pHeaderInfo->info11a.xrTargetPower = (A_UINT16)(tempValue  & 0x3f);

		offset++;
		tempValue = eepromRead(devNum, (offset + pLibDev->eepromStartLoc));
		pHeaderInfo->info11a.initialGainI = (A_UINT16)(((tempValue & 0x07) << 3) | pHeaderInfo->info11a.initialGainI);
	}

	if(pHeaderInfo->majorVersion >= 4) {
		pHeaderInfo->info11a.iqCalQ = (A_UINT16)((tempValue >> 3) & 0x1f);
		pHeaderInfo->info11a.iqCalI = (A_UINT16)((tempValue >> 8) & 0x3f);

		offset++;
		if(pHeaderInfo->minorVersion >= 1) {
			tempValue = eepromRead(devNum, (offset + pLibDev->eepromStartLoc));
			pHeaderInfo->info11a.rxtxMargin = (A_UINT16)(tempValue & 0x3f);
		}
		if(((pHeaderInfo->majorVersion == 4) && (pHeaderInfo->minorVersion == 9)) ||
                   ((pHeaderInfo->majorVersion == 5) && (pHeaderInfo->minorVersion >= 4)) ||
                    (pHeaderInfo->majorVersion >= 6)){
			offset += 3; // skip the turbo params for 11a added with 5.0/5.1 mainline
			             // in locations 0xE2, 0xE3
			tempValue = eepromRead(devNum, (offset + pLibDev->eepromStartLoc));
			pHeaderInfo->info11a.phaseCal[0] = (A_UINT16)(tempValue & 0x3f);
			pHeaderInfo->info11a.phaseCal[1] = (A_UINT16)((tempValue >> 6) & 0x3f);
			pHeaderInfo->info11a.phaseCal[2] = (A_UINT16)((tempValue >> 12) & 0x3f);

			offset++;
			tempValue = eepromRead(devNum, (offset + pLibDev->eepromStartLoc));
			pHeaderInfo->info11a.phaseCal[2] |= (A_UINT16)(((tempValue >> 0) & 0x3) << 4);
			pHeaderInfo->info11a.phaseCal[3] = (A_UINT16)((tempValue >> 2) & 0x3f);
			pHeaderInfo->info11a.phaseCal[4] = (A_UINT16)((tempValue >> 8) & 0x3f);

			for (i=0; i<5; i++) {
				pHeaderInfo->info11a.phaseCal[i] *= 10;
			}
		}

	}

	//read the turbo params
	if(pHeaderInfo->majorVersion >= 5) {
		pHeaderInfo->info11a.turbo.switchSettling = (A_UINT16)((tempValue >> 6) & 0x7f);
		pHeaderInfo->info11a.turbo.txrxAtten = (A_UINT16)((tempValue >> 13) & 0x7);

		offset++;
		tempValue = eepromRead(devNum, (offset + pLibDev->eepromStartLoc));
		pHeaderInfo->info11a.turbo.txrxAtten = (A_UINT16)(((tempValue & 0x07) << 3) | pHeaderInfo->info11a.turbo.txrxAtten);
		pHeaderInfo->info11a.turbo.rxtxMargin = (A_UINT16)((tempValue >> 3) & 0x3f);
		pHeaderInfo->info11a.turbo.adcDesiredSize = (A_INT8)((tempValue >> 9) & 0x7f);

		offset++;
		tempValue = eepromRead(devNum, (offset + pLibDev->eepromStartLoc));
		pHeaderInfo->info11a.turbo.adcDesiredSize = (A_INT8)(((tempValue & 0x01) << 7) | pHeaderInfo->info11a.turbo.adcDesiredSize);
		pHeaderInfo->info11a.turbo.pgaDesiredSize = (A_INT8)((tempValue >> 1) & 0xff);
	}

	//11b and g params
	//start by reading b
	pModeInfo = &(pHeaderInfo->info11b);
	offset = pOffsets->HDR_11B_COMMON;
	for (i = 0; i < 2; i++) {
		tempValue = eepromRead(devNum, (offset + pLibDev->eepromStartLoc));
		pModeInfo->switchSettling = (A_UINT16)((tempValue >> 8) & 0x7f);
		pModeInfo->txrxAtten = (A_UINT16)((tempValue >> 2) & 0x3f);
		pModeInfo->antennaControl[0] = (A_UINT16)((tempValue << 4) & 0x3f);

		tempValue = eepromRead(devNum, (offset + 1 + pLibDev->eepromStartLoc));
		pModeInfo->antennaControl[0] = (A_UINT16)(((tempValue >> 12) & 0x0f) | pModeInfo->antennaControl[0]);
		pModeInfo->antennaControl[1] = (A_UINT16)((tempValue >> 6) & 0x3f);
		pModeInfo->antennaControl[2] = (A_UINT16)(tempValue  & 0x3f);

		tempValue = eepromRead(devNum, (offset + 2 + pLibDev->eepromStartLoc));
		pModeInfo->antennaControl[3] = (A_UINT16)((tempValue >> 10)  & 0x3f);
		pModeInfo->antennaControl[4] = (A_UINT16)((tempValue >> 4)  & 0x3f);
		pModeInfo->antennaControl[5] = (A_UINT16)((tempValue << 2)  & 0x3f);

		tempValue = eepromRead(devNum, (offset + 3 + pLibDev->eepromStartLoc));
		pModeInfo->antennaControl[5] = (A_UINT16)(((tempValue >> 14)  & 0x03) | pModeInfo->antennaControl[5]);
		pModeInfo->antennaControl[6] = (A_UINT16)((tempValue >> 8)  & 0x3f);
		pModeInfo->antennaControl[7] = (A_UINT16)((tempValue >> 2)  & 0x3f);
		pModeInfo->antennaControl[8] = (A_UINT16)((tempValue << 4)  & 0x3f);

		tempValue = eepromRead(devNum, (offset + 4 + pLibDev->eepromStartLoc));
		pModeInfo->antennaControl[8] = (A_UINT16)(((tempValue >> 12)  & 0x0f) | pModeInfo->antennaControl[8]);
		pModeInfo->antennaControl[9] = (A_UINT16)((tempValue >> 6)  & 0x3f);
		pModeInfo->antennaControl[10] = (A_UINT16)(tempValue & 0x3f);

		tempValue = eepromRead(devNum, (offset + 5 + pLibDev->eepromStartLoc));
		pModeInfo->adcDesiredSize = (A_INT8)((tempValue >> 8)  & 0xff);
		pModeInfo->ob_1 = (A_UINT16)((tempValue >> 4)  & 0x07);
		pModeInfo->db_1 = (A_UINT16)(tempValue & 0x07);

		tempValue = eepromRead(devNum, (offset + 6 + pLibDev->eepromStartLoc));
		pModeInfo->txEndToXLNAOn = (A_UINT16)((tempValue >> 8)  & 0xff);
		pModeInfo->thresh62 = (A_UINT16)(tempValue  & 0xff);

		tempValue = eepromRead(devNum, (offset + 7 + pLibDev->eepromStartLoc));
		pModeInfo->txEndToXPAOff = (A_UINT16)((tempValue >> 8)  & 0xff);
		pModeInfo->txFrameToXPAOn = (A_UINT16)(tempValue  & 0xff);

		tempValue = eepromRead(devNum, (offset + 8 + pLibDev->eepromStartLoc));
		pModeInfo->pgaDesiredSize = (A_INT8)((tempValue >> 8)  & 0xff);
		pModeInfo->noisefloorThresh = (A_INT8)(tempValue  & 0xff);

		tempValue = eepromRead(devNum, (offset + 9 + pLibDev->eepromStartLoc));
		if((pHeaderInfo->majorVersion >= 4) && (i == 1)) {
			pHeaderInfo->fixedBiasB = (A_UINT16)((tempValue >> 13)  & 0x01);
		}
		pModeInfo->xlnaGain = (A_UINT16)((tempValue >> 5)  & 0xff);
		pModeInfo->xgain = (A_UINT16)((tempValue >> 1)  & 0x0f);
		pModeInfo->xpd = (A_UINT8)(tempValue  & 0x01);

		if(((pHeaderInfo->majorVersion == 3) && (pHeaderInfo->minorVersion >= 3))
			|| (pHeaderInfo->majorVersion >= 4)){
			tempValue = eepromRead(devNum, (offset+10 + pLibDev->eepromStartLoc));
			pModeInfo->falseDetectBackoff = (A_UINT16)((tempValue >> 6)  & 0x7f);
		}

		//read the extra info for EEPROM version 3.4
		if(((pHeaderInfo->majorVersion == 3) && (pHeaderInfo->minorVersion >= 4))
			|| (pHeaderInfo->majorVersion >= 4)){
			pModeInfo->initialGainI = (A_UINT16)((tempValue >> 13) & 0x07);

			tempValue = eepromRead(devNum, (offset + 11 + pLibDev->eepromStartLoc));
			pModeInfo->initialGainI = (A_UINT16)(((tempValue & 0x07) << 3) | pModeInfo->initialGainI);
			pLibDev->p16kEepHeader->scaledOfdmCckDelta = (A_UINT16)((tempValue >> 3) & 0xff);

			if(((pHeaderInfo->majorVersion == 4) && (pHeaderInfo->minorVersion >= 6)) ||
				(pHeaderInfo->majorVersion >= 5)){
				pLibDev->p16kEepHeader->scaledCh14FilterCckDelta = (A_UINT16)((tempValue >> 11)  & 0x1f);
			}

			tempValue = eepromRead(devNum, (offset + 12 + pLibDev->eepromStartLoc));
			pModeInfo->calPier1 = (A_UINT16)(tempValue & 0xff);
			if(pModeInfo->calPier1 != 0xff) {
				pModeInfo->calPier1 = fbin2freq_2p4(devNum, pModeInfo->calPier1);
			}
			pModeInfo->calPier2 = (A_UINT16)((tempValue >> 8)  & 0xff);
			if(pModeInfo->calPier2 != 0xff) {
				pModeInfo->calPier2 = fbin2freq_2p4(devNum, pModeInfo->calPier2);
			}

			if(i == 1) {  //11g mode
 				tempValue = eepromRead(devNum, (offset + 13 + pLibDev->eepromStartLoc));
				pModeInfo->turbo.max2wPower = (A_UINT16)(tempValue & 0x7f);
				pModeInfo->xrTargetPower = (A_UINT16)((tempValue >> 7)  & 0x3f);

			}
		}

		if(pHeaderInfo->majorVersion >= 4) {
			if(i == 0) {
				tempValue = eepromRead(devNum, (offset + 13 + pLibDev->eepromStartLoc));
				pModeInfo->calPier3 = (A_UINT16)(tempValue & 0xff);
				if(pModeInfo->calPier3 != 0xff) {
					pModeInfo->calPier3 = fbin2freq_2p4(devNum, pModeInfo->calPier3);
				}

				if( ((pHeaderInfo->majorVersion == 4) && (pHeaderInfo->minorVersion >= 1)) ||
					(pHeaderInfo->majorVersion >= 5)){
					pModeInfo->rxtxMargin = (A_UINT16)((tempValue >> 8) & 0x3f);
				}
			}

			if(i == 1) {  //11g mode
				tempValue = eepromRead(devNum, (offset + 14 + pLibDev->eepromStartLoc));
				pModeInfo->calPier3 = (A_UINT16)(tempValue & 0xff);
				if(pModeInfo->calPier3 != 0xff) {
					pModeInfo->calPier3 = fbin2freq_2p4(devNum, pModeInfo->calPier3);
				}

				if( ((pHeaderInfo->majorVersion == 4) && (pHeaderInfo->minorVersion >= 1)) ||
					(pHeaderInfo->majorVersion >= 5)){
					pModeInfo->rxtxMargin = (A_UINT16)((tempValue >> 8) & 0x3f);
				}

				tempValue = eepromRead(devNum, (offset + 15 + pLibDev->eepromStartLoc));
				pModeInfo->iqCalQ = (A_UINT16)(tempValue & 0x1f);
				pModeInfo->iqCalI = (A_UINT16)((tempValue >> 5)  & 0x3f);

				if( ((pHeaderInfo->majorVersion == 4) && (pHeaderInfo->minorVersion >= 2)) ||
					(pHeaderInfo->majorVersion >= 5)){
					tempValue = eepromRead(devNum, (offset + 16 + pLibDev->eepromStartLoc));
					pHeaderInfo->ofdmCckGainDeltaX2 = (A_INT8)(tempValue & 0xFF);
				}

                                if(((pHeaderInfo->majorVersion == 4) && (pHeaderInfo->minorVersion == 9)) ||
                                   ((pHeaderInfo->majorVersion == 5) && (pHeaderInfo->minorVersion >= 4)) ||
                                   (pHeaderInfo->majorVersion >= 6)){					// skip 0x11E, 0x11F
					tempValue = eepromRead(devNum, (offset + 19 + pLibDev->eepromStartLoc));
					pModeInfo->phaseCal[0] = 10 * (tempValue & 0x3F);
					pModeInfo->phaseCal[1] = 10 * ((tempValue >> 6) & 0x3F);
                                  }
			}
		}
		if(pHeaderInfo->majorVersion >= 5) {
			if(i == 1) {  //11g mode
				//read the turbo params
				pModeInfo->turbo.switchSettling = (A_UINT16)((tempValue >> 8) & 0x7f);
				pModeInfo->turbo.txrxAtten = (A_UINT16)((tempValue >> 15) & 0x1);

				tempValue = eepromRead(devNum, (offset + 17 + pLibDev->eepromStartLoc));
				pModeInfo->turbo.txrxAtten = (A_UINT16)(((tempValue & 0x1f) << 1) | pModeInfo->turbo.txrxAtten);
				pModeInfo->turbo.rxtxMargin = (A_UINT16)((tempValue >> 5) & 0x3f);
				pModeInfo->turbo.adcDesiredSize = (A_INT8)((tempValue >> 10) & 0x1f);

				tempValue = eepromRead(devNum, (offset + 18 + pLibDev->eepromStartLoc));
				pModeInfo->turbo.adcDesiredSize = (A_INT8)(((tempValue & 0x7) << 5) | pModeInfo->turbo.adcDesiredSize);
				pModeInfo->turbo.pgaDesiredSize = (A_INT8)((tempValue >> 3) & 0xff);
			}
		}

		//change params to read g stuff
		pModeInfo = &(pHeaderInfo->info11g);
		offset = pOffsets->HDR_11G_COMMON;
	}

	//read the test groups
	offset = pOffsets->HDR_CTL;
	for( i = 0; i < pHeaderInfo->numCtl; i+=2) {
		tempValue = eepromRead(devNum, (offset + pLibDev->eepromStartLoc));
		offset++;
		pHeaderInfo->testGroups[i] = (A_UINT16)((tempValue >> 8) & 0xff);
		pHeaderInfo->testGroups[i+1] = (A_UINT16)(tempValue & 0xff);
	}

	//read b and g ob/db, store in ob/db_4
	if(((pHeaderInfo->majorVersion == 3) && (pHeaderInfo->minorVersion >= 1))
			|| (pHeaderInfo->majorVersion >= 4)){
		tempValue = eepromRead(devNum, (pOffsets->HDR_11B_SPECIFIC + pLibDev->eepromStartLoc));
		pHeaderInfo->info11b.db_4 = (A_UINT16)((tempValue >> 3) & 0x7);
		pHeaderInfo->info11b.ob_4 = (A_UINT16)(tempValue & 0x7);
		tempValue = eepromRead(devNum, (pOffsets->HDR_11G_SPECIFIC + pLibDev->eepromStartLoc));
		pHeaderInfo->info11g.db_4 = (A_UINT16)((tempValue >> 3) & 0x7);
		pHeaderInfo->info11g.ob_4 = (A_UINT16)(tempValue & 0x7);
	}


	//read corner cal info
#if 0
	if(pHeaderInfo->minorVersion >= 2) {
		offset = pOffsets->HDR_11A_SPECIFIC;
		tempValue = eepromRead(devNum, (offset + pLibDev->eepromStartLoc));
		offset++;
		pHeaderInfo->cornerCal[0].gSel = (A_UINT16)(tempValue & 0x1);
		pHeaderInfo->cornerCal[0].pd84 = (A_UINT16)((tempValue >> 1) & 0x1);
		pHeaderInfo->cornerCal[0].pd90 = (A_UINT16)((tempValue >> 2) & 0x1);
		pHeaderInfo->cornerCal[0].clip = (A_UINT16)((tempValue >> 3) & 0x7);
		pHeaderInfo->cornerCal[1].gSel = (A_UINT16)((tempValue >> 6) & 0x1);
		pHeaderInfo->cornerCal[1].pd84 = (A_UINT16)((tempValue >> 7) & 0x1);
		pHeaderInfo->cornerCal[1].pd90 = (A_UINT16)((tempValue >> 8) & 0x1);
		pHeaderInfo->cornerCal[1].clip = (A_UINT16)((tempValue >> 9) & 0x7);
		pHeaderInfo->cornerCal[2].gSel = (A_UINT16)((tempValue >> 12) & 0x1);
		pHeaderInfo->cornerCal[2].pd84 = (A_UINT16)((tempValue >> 13) & 0x1);
		pHeaderInfo->cornerCal[2].pd90 = (A_UINT16)((tempValue >> 14) & 0x1);
		pHeaderInfo->cornerCal[2].clip = (A_UINT16)((tempValue >> 15) & 0x1);

		tempValue = eepromRead(devNum, (offset + pLibDev->eepromStartLoc));
		pHeaderInfo->cornerCal[2].clip = (A_UINT16)(((tempValue & 0x3) << 1) | pHeaderInfo->cornerCal[2].clip);
		pHeaderInfo->cornerCal[3].gSel = (A_UINT16)((tempValue >> 2) & 0x1);
		pHeaderInfo->cornerCal[3].pd84 = (A_UINT16)((tempValue >> 3) & 0x1);
		pHeaderInfo->cornerCal[3].pd90 = (A_UINT16)((tempValue >> 4) & 0x1);
		pHeaderInfo->cornerCal[3].clip = (A_UINT16)((tempValue >> 5) & 0x7);
	}
#endif
	return;
}

/**************************************************************************
* programHeaderInfo - Program device registers with eeprom header info
*
* RETURNS:
*/
void  programHeaderInfo
(
 A_UINT32			devNum,
 MDK_EEP_HEADER_INFO	*pHeaderInfo,
 A_UINT16			freq,
 A_UCHAR			mode
)
{
	A_UINT32		tempOB=0;
	A_UINT32		tempDB=0;

	MODE_HEADER_INFO	*pModeInfo;
    LIB_DEV_INFO *pLibDev = gLibInfo.pLibDevArray[devNum];
	TURBO_HEADER_INFO   *pTurboInfo;

//	if(pLibDev->eepromHeaderApplied[mode]) {
//		return;
//	}

//printf("mode=%d\n", mode);
	switch(mode) {
	case MODE_11A:
		pModeInfo = &(pHeaderInfo->info11a);
		break;

	case MODE_11G:
	case MODE_11O:
		pModeInfo = &(pHeaderInfo->info11g);
		break;

	case MODE_11B:
		pModeInfo = &(pHeaderInfo->info11b);
		break;

	default:

		mError(devNum, EINVAL, "Device Number %d:Illegal mode passed to programHeaderInfo\n", devNum);
		return;
	} //end switch


/*
	   writeField(devNum, "bb_switch_settling", pModeInfo->switchSettling);
	   writeField(devNum, "bb_txrxatten", pModeInfo->txrxAtten);

*/

	pTurboInfo = &(pModeInfo->turbo);
	if (pHeaderInfo->majorVersion >= 5){
	   writeField(devNum, "bb_switch_settling",
		   pLibDev->turbo == TURBO_ENABLE ? pTurboInfo->switchSettling : pModeInfo->switchSettling);
	   writeField(devNum, "bb_txrxatten",
		   pLibDev->turbo == TURBO_ENABLE ? pTurboInfo->txrxAtten : pModeInfo->txrxAtten);
	   writeField(devNum, "bb_pga_desired_size",
		   pLibDev->turbo == TURBO_ENABLE ? pTurboInfo->pgaDesiredSize : pModeInfo->pgaDesiredSize);
	   writeField(devNum, "bb_adc_desired_size",
		   pLibDev->turbo == TURBO_ENABLE ? pTurboInfo->adcDesiredSize : pModeInfo->adcDesiredSize);
	}
	else {
	   writeField(devNum, "bb_switch_settling", pModeInfo->switchSettling);
	   writeField(devNum, "bb_txrxatten", pModeInfo->txrxAtten);
	   writeField(devNum, "bb_pga_desired_size", pModeInfo->pgaDesiredSize);
	   writeField(devNum, "bb_adc_desired_size", pModeInfo->adcDesiredSize);
	}
	if(((pHeaderInfo->majorVersion == 4) && (pHeaderInfo->minorVersion >= 1)) ||
	  (pHeaderInfo->majorVersion >= 5)){

		if (pHeaderInfo->majorVersion >= 5){
		   writeField(devNum, "bb_rxtx_margin_2ghz",
			   pLibDev->turbo == TURBO_ENABLE ? pTurboInfo->rxtxMargin : pModeInfo->rxtxMargin);
		}
		else {
		   writeField(devNum, "bb_rxtx_margin_2ghz", pModeInfo->rxtxMargin);
		}
	}


	if(((freq > 4000) && (freq < 5260)) || (mode != MODE_11A)) {
		tempOB = pModeInfo->ob_1;
		tempDB = pModeInfo->db_1;
	}
	else if ((freq >= 5260) && (freq < 5500)) {
		tempOB = pModeInfo->ob_2;
		tempDB = pModeInfo->db_2;
	}
	else if ((freq >= 5500) && (freq < 5725)) {
		tempOB = pModeInfo->ob_3;
		tempDB = pModeInfo->db_3;
	}
	else if (freq >= 5725) {
		tempOB = pModeInfo->ob_4;
		tempDB = pModeInfo->db_4;
	}

	writeField(devNum, "rf_ob", tempOB);
	writeField(devNum, "rf_db", tempDB);

	if (!isGriffin(pLibDev->swDevID)) { // exclude griffin
		if((mode != MODE_11A)&&
			((pHeaderInfo->majorVersion == 3) && (pHeaderInfo->minorVersion >= 1))
				|| (pHeaderInfo->majorVersion >= 4)){
			//write the b_ob and b_db which are held in ob/db_4
			writeField(devNum, "rf_b_ob", pModeInfo->ob_4);
			writeField(devNum, "rf_b_db", pModeInfo->db_4);
		}
		writeField(devNum, "bb_tx_end_to_xlna_on", pModeInfo->txEndToXLNAOn);
	}


	writeField(devNum, "bb_thresh62", pModeInfo->thresh62);

	if(pLibDev->mode == MODE_11A) {
		writeField(devNum, "bb_tx_end_to_xpaa_off", pModeInfo->txEndToXPAOff);
		writeField(devNum, "bb_tx_frame_to_xpaa_on", pModeInfo->txFrameToXPAOn);
	}
	else  {
		writeField(devNum, "bb_tx_end_to_xpab_off", pModeInfo->txEndToXPAOff);
		writeField(devNum, "bb_tx_frame_to_xpab_on", pModeInfo->txFrameToXPAOn);
	}


	if((pLibDev->p16kEepHeader->majorVersion <= 3) ||
		((pLibDev->p16kEepHeader->majorVersion >= 4) && (pLibDev->p16kEepHeader->eepMap == 0))) {
		writeField(devNum, "rf_xpd_gain", pModeInfo->xgain);
	}

	if(((pLibDev->swDevID & 0xff) <= 0x13) || ((pLibDev->swDevID & 0xff) == 0x15)) {
		writeField(devNum, "rf_plo_sel", pModeInfo->xpd);
		writeField(devNum, "rf_pwdxpd", !pModeInfo->xpd);
	}
	else if (!isGriffin(pLibDev->swDevID) && !isEagle(pLibDev->swDevID) && !isOwl(pLibDev->swDevID)){
		writeField(devNum, "rf_xpdsel", pModeInfo->xpd);
	}

	if (isFalcon(devNum)) {
#if !defined(FREEDOM_AP)
#if !defined(OWL_AP)
#ifndef __ATH_DJGPPDOS__
		forceAntennaTbl5513(devNum, pModeInfo->antennaControl);
#endif
#endif
#endif
	} else if ((pLibDev->swDevID & 0xff) >= 0x0012)
	{
//		setAntennaTbl5211(devNum, pModeInfo->antennaControl);
		if(!isOwl(pLibDev->swDevID)) {
			forceAntennaTbl5211(devNum, pModeInfo->antennaControl);
		}
	}

	if(((pHeaderInfo->majorVersion == 3) && (pHeaderInfo->minorVersion >= 4))
			|| ((pHeaderInfo->majorVersion >= 4) && (pHeaderInfo->eepMap != 2))){
		writeField(devNum, "rf_gain_I", pModeInfo->initialGainI);
	}

	if(pHeaderInfo->majorVersion >= 4) {
		if((pLibDev->mode != MODE_11B) &&
		   (((pLibDev->swDevID & 0xff) == 0x14) || ((pLibDev->swDevID & 0xff) >= 0x16))){
			//program the IQ cal values
			writeField(devNum, "bb_iqcorr_q_i_coff", pModeInfo->iqCalI);
			writeField(devNum, "bb_iqcorr_q_q_coff", pModeInfo->iqCalQ);
		}

		if((((pLibDev->swDevID & 0xff) == 0x14) || ((pLibDev->swDevID & 0xff) >= 0x16)) &&
			(!isGriffin(pLibDev->swDevID)) && !isEagle(pLibDev->swDevID) && !isOwl(pLibDev->swDevID)){
			writeField(devNum, "bb_do_iqcal", 1);
			writeField(devNum, "rf_Afixed_bias", pHeaderInfo->fixedBiasA);
			writeField(devNum, "rf_Bfixed_bias", pHeaderInfo->fixedBiasB);
		}
	}

#if 0
	if((mode == MODE_11A)&&(pHeaderInfo->minorVersion >= 2)) {
		writeCornerCal(devNum, freq, pHeaderInfo);
	}
#endif


	//set the flag to say we applied the eeprom
	pLibDev->eepromHeaderApplied[mode] = TRUE;

	return;
}

#if 0
void writeCornerCal
(
 A_UINT32 devNum,
 A_UINT16 channel,
 MDK_EEP_HEADER_INFO	*pHeaderInfo
)
{
	A_UINT16  Lch, Rch, Nch, jj;

	iGetLowerUpperValues(channel, cornerFixChannels, NUM_CORNER_FIX_CHANNELS, &Lch, &Rch);

	Nch = (abs(channel - Lch) >= abs(Rch - channel)) ? Rch : Lch;

	for (jj = 0; jj < NUM_CORNER_FIX_CHANNELS; jj++)
	{
		if (cornerFixChannels[jj] == Nch)
			break;
	}

//	uiPrintf("SNOOP:  %d: nearest channel is %d(jj=%d) (Lch=%d, Rch=%d)\n", channel, Nch,jj,Lch, Rch);
	writeField(devNum, "rf_rfgainsel", pHeaderInfo->cornerCal[jj].gSel);
	writeField(devNum, "rf_pwd_84", pHeaderInfo->cornerCal[jj].pd84);
	writeField(devNum, "rf_pwd_90", pHeaderInfo->cornerCal[jj].pd90);
	writeField(devNum, "bb_tx_clip", pHeaderInfo->cornerCal[jj].clip);
}
#endif

A_UINT16 readCtlInfo
(
 A_UINT32 devNum,
 A_UINT16 offset,
 MDK_RD_EDGES_POWER	*pRdEdgePwrInfo
)
{
    LIB_DEV_INFO *pLibDev = gLibInfo.pLibDevArray[devNum];
	A_UINT32 tempValue;
	A_UINT16 sizeCtl = 0;
	A_UINT16 ii;

	if((pLibDev->p16kEepHeader->majorVersion == 3) && (pLibDev->p16kEepHeader->minorVersion <= 2))
	{
		tempValue = eepromRead(devNum, offset++);
		pRdEdgePwrInfo[0].rdEdge = (A_UINT16)(( tempValue >> 9 ) & FREQ_MASK_16K);
		pRdEdgePwrInfo[1].rdEdge = (A_UINT16)(( tempValue >> 2 ) & FREQ_MASK_16K);
		pRdEdgePwrInfo[2].rdEdge = (A_UINT16)(( tempValue << 5 ) & FREQ_MASK_16K);

		tempValue = eepromRead(devNum, offset++);
		pRdEdgePwrInfo[2].rdEdge = (A_UINT16)((( tempValue >> 11 ) & 0x1f) | pRdEdgePwrInfo[2].rdEdge);
		pRdEdgePwrInfo[3].rdEdge = (A_UINT16)(( tempValue >> 4 ) & FREQ_MASK_16K);
		pRdEdgePwrInfo[4].rdEdge = (A_UINT16)(( tempValue << 3 ) & FREQ_MASK_16K);

		tempValue = eepromRead(devNum, offset++);
		pRdEdgePwrInfo[4].rdEdge = (A_UINT16)((( tempValue >> 13 ) & 0x7) | pRdEdgePwrInfo[4].rdEdge);
		pRdEdgePwrInfo[5].rdEdge = (A_UINT16)(( tempValue >> 6 ) & FREQ_MASK_16K);
		pRdEdgePwrInfo[6].rdEdge =  (A_UINT16)(( tempValue << 1 ) & FREQ_MASK_16K);

		tempValue = eepromRead(devNum, offset++);
		pRdEdgePwrInfo[6].rdEdge = (A_UINT16)((( tempValue >> 15 ) & 0x1) | pRdEdgePwrInfo[6].rdEdge);
		pRdEdgePwrInfo[7].rdEdge = (A_UINT16)(( tempValue >> 8 ) & FREQ_MASK_16K);

		pRdEdgePwrInfo[0].twice_rdEdgePower = (A_UINT16)(( tempValue >> 2 ) & POWER_MASK);
		pRdEdgePwrInfo[1].twice_rdEdgePower =  (A_UINT16)(( tempValue << 4 ) & POWER_MASK);

		tempValue = eepromRead(devNum, offset++);
		pRdEdgePwrInfo[1].twice_rdEdgePower = (A_UINT16)((( tempValue >> 12 ) & 0xf) | pRdEdgePwrInfo[1].twice_rdEdgePower);
		pRdEdgePwrInfo[2].twice_rdEdgePower = (A_UINT16)(( tempValue >> 6 ) & POWER_MASK);
		pRdEdgePwrInfo[3].twice_rdEdgePower = (A_UINT16)(tempValue & POWER_MASK);

		tempValue = eepromRead(devNum, offset++);
		pRdEdgePwrInfo[4].twice_rdEdgePower = (A_UINT16)(( tempValue >> 10 ) & POWER_MASK);
		pRdEdgePwrInfo[5].twice_rdEdgePower = (A_UINT16)(( tempValue >> 4 ) & POWER_MASK);
		pRdEdgePwrInfo[6].twice_rdEdgePower = (A_UINT16)(( tempValue << 2 ) & POWER_MASK);

		tempValue = eepromRead(devNum, offset++);
		pRdEdgePwrInfo[6].twice_rdEdgePower = (A_UINT16)((( tempValue >> 14 ) & 0x3) | pRdEdgePwrInfo[6].twice_rdEdgePower);
		pRdEdgePwrInfo[7].twice_rdEdgePower = (A_UINT16)(( tempValue >> 8 ) & POWER_MASK);
		sizeCtl = 7;
	}
	else
	{
		for (ii = 0; ii < NUM_16K_EDGES; ii+=2) {
			tempValue = eepromRead(devNum, offset++);
			pRdEdgePwrInfo[ii].rdEdge = (A_UINT16)(( tempValue >> 8 ) & NEW_FREQ_MASK_16K);
			pRdEdgePwrInfo[ii+1].rdEdge = (A_UINT16)(( tempValue ) & NEW_FREQ_MASK_16K);
		}

		for (ii = 0; ii < NUM_16K_EDGES; ii+=2) {
			tempValue = eepromRead(devNum, offset++);
			pRdEdgePwrInfo[ii+1].twice_rdEdgePower = (A_UINT16)(( tempValue ) & POWER_MASK);
			pRdEdgePwrInfo[ii+1].flag = (A_BOOL)((tempValue >> 6) & 0x01);
			pRdEdgePwrInfo[ii].twice_rdEdgePower = (A_UINT16)(( tempValue >> 8 ) & POWER_MASK);
			pRdEdgePwrInfo[ii].flag = (A_BOOL)((tempValue >> 14) & 0x01);
		}

		sizeCtl = 8;

	}
	return sizeCtl;
}


A_BOOL readTrgtPowers
(
 A_UINT32 devNum,
 A_UINT16 offset,
 MDK_TRGT_POWER_INFO *pPowerInfo,
 A_UINT16 mode
)
{
    LIB_DEV_INFO *pLibDev = gLibInfo.pLibDevArray[devNum];
	A_UINT32 tempValue;
	A_BOOL	 readPower=FALSE;

	if((pLibDev->p16kEepHeader->majorVersion == 3) && (pLibDev->p16kEepHeader->minorVersion <= 2))
	{
		tempValue = eepromRead(devNum, offset);
		pPowerInfo->testChannel = (A_UINT16)(( tempValue >> 9 ) & FREQ_MASK_16K);

		if(pPowerInfo->testChannel != 0) {
			//get the channel value and read rest of info
			if (mode == MODE_11A) {
				pPowerInfo->testChannel = fbin2freq(devNum, pPowerInfo->testChannel);
			}
			else {
				pPowerInfo->testChannel = fbin2freq_2p4(devNum, pPowerInfo->testChannel);
			}
			pPowerInfo->twicePwr6_24 = (A_INT16)(( tempValue >> 3 ) & POWER_MASK);
			pPowerInfo->twicePwr36 = (A_INT16)(( tempValue << 3  ) & POWER_MASK);

			tempValue = eepromRead(devNum, offset+1);
			pPowerInfo->twicePwr36 =
				(A_UINT16)(( ( tempValue >> 13 ) & 0x7 ) | pPowerInfo->twicePwr36);
			pPowerInfo->twicePwr48 = (A_INT16)(( tempValue >> 7 ) & POWER_MASK);
			pPowerInfo->twicePwr54 = (A_INT16)(( tempValue >> 1 ) & POWER_MASK);
			readPower = TRUE;
		}
	}
	else {
		tempValue = eepromRead(devNum, offset);
		pPowerInfo->testChannel = (A_UINT16)(( tempValue >> 8 ) & NEW_FREQ_MASK_16K);

		if(pPowerInfo->testChannel != 0) {
			//get the channel value and read rest of info
			if (mode == MODE_11A) {
				pPowerInfo->testChannel = fbin2freq(devNum, pPowerInfo->testChannel);
			}
			else {
				pPowerInfo->testChannel = fbin2freq_2p4(devNum, pPowerInfo->testChannel);
			}
			pPowerInfo->twicePwr6_24 = (A_INT16)(( tempValue >> 2 ) & POWER_MASK);
			pPowerInfo->twicePwr36 = (A_INT16)(( tempValue << 4  ) & POWER_MASK);

			tempValue = eepromRead(devNum, offset+1);
			pPowerInfo->twicePwr36 =
				(A_UINT16)(( ( tempValue >> 12 ) & 0xf ) | pPowerInfo->twicePwr36);
			pPowerInfo->twicePwr48 = (A_INT16)(( tempValue >> 6 ) & POWER_MASK);
			pPowerInfo->twicePwr54 = (A_INT16)(tempValue & POWER_MASK);
			readPower = TRUE;

		}

	}
	return readPower;
}


void readFreqPiers
(
 A_UINT32 devNum,
 A_UINT16 *pChannels,
 A_UINT16 numChannels
)
{
    LIB_DEV_INFO *pLibDev = gLibInfo.pLibDevArray[devNum];
	A_UINT16	offset;
	A_UINT32	tempValue;
	A_UINT16	i;


	offset = (A_UINT16)(pOffsets->GROUP1_11A_FREQ_PIERS + pLibDev->eepromStartLoc);

	if((pLibDev->p16kEepHeader->majorVersion == 3) && (pLibDev->p16kEepHeader->minorVersion <= 2))
	{
		tempValue = eepromRead(devNum, offset);
		pChannels[0] = (A_UINT16)(( tempValue >> 9 ) & FREQ_MASK_16K);
		pChannels[1] = (A_UINT16)(( tempValue >> 2 ) & FREQ_MASK_16K);
		pChannels[2] = (A_UINT16)(( tempValue << 5 ) & FREQ_MASK_16K);

		offset++;
		tempValue = eepromRead(devNum, offset);
		pChannels[2] = (A_UINT16)((( tempValue >> 11 ) & 0x1f) | pChannels[2]);
		pChannels[3] = (A_UINT16)(( tempValue >> 4 ) & FREQ_MASK_16K);
		pChannels[4] = (A_UINT16)(( tempValue << 3 ) & FREQ_MASK_16K);

		offset++;
		tempValue = eepromRead(devNum, offset);
		pChannels[4] = (A_UINT16)((( tempValue >> 13 ) & 0x7) | pChannels[4]);
		pChannels[5] = (A_UINT16)(( tempValue >> 6 ) & FREQ_MASK_16K);
		pChannels[6] =  (A_UINT16)(( tempValue << 1 ) & FREQ_MASK_16K);

		offset++;
		tempValue = eepromRead(devNum, offset);
		pChannels[6] = (A_UINT16)((( tempValue >> 15 ) & 0x1) | pChannels[6]);
		pChannels[7] = (A_UINT16)(( tempValue >> 8 ) & FREQ_MASK_16K);
		pChannels[8] =  (A_UINT16)(( tempValue >> 1 ) & FREQ_MASK_16K);
		pChannels[9] =  (A_UINT16)(( tempValue << 6 ) & FREQ_MASK_16K);

		offset++;
		tempValue = eepromRead(devNum, offset);
		pChannels[9] = (A_UINT16)((( tempValue >> 10 ) & 0x3f) | pChannels[9]);
	}
	else {
		for(i = 0; i < numChannels; i+=2) {
			tempValue = eepromRead(devNum, offset);
			pChannels[i] = (A_UINT16)((tempValue >> 8) & NEW_FREQ_MASK_16K);
			pChannels[i+1] = (A_UINT16)(tempValue & NEW_FREQ_MASK_16K);
			offset++;
		}
	}

	for(i = 0; i < numChannels; i++ ) {
		pChannels[i] = fbin2freq(devNum, pChannels[i]);
	}
}

/**************************************************************************
* get_eeprom_size - gets the size of the eeprom from 0x1c location and
checksumlength from 0x1b and part of 0x1c location .

*
*
* RETURNS: TRUE if OK, FALSE otherwise
*
*/

A_UINT32 getEepromSize(A_UINT32 devNum,A_UINT32 *eepromSizeLib, A_UINT32 *checkSumLengthLib)
{
	A_UINT32 eepromLower,eepromUpper;
	A_UINT32	size = 0 , length =0,checkSumLengthError=0;


	eepromLower = eepromRead(devNum,0x1B);
	eepromUpper = eepromRead(devNum,0x1C);

#ifndef NO_LIB_PRINT
//	printf("size of the eepromLower = %x \n",eepromLower);
//	printf("size of the eepromUpper = %x \n",eepromUpper);
#endif

	if((eepromUpper == 0x0000)|| (eepromLower == 0x0000))  {
		length = 0x400;
		size = 0x400;
	}
	if((eepromUpper == 0xffff) || (eepromLower == 0xffff))
	{
		length = 0x400;
		size = 0x400;
	}

	if(length!=0x400)
	{
		size = eepromUpper & 0x1f;
		size = 2 << (size + 9);
		size = size/2;
		eepromUpper = (eepromUpper & 0xffe0) >> 5 ;
		length = eepromUpper | eepromLower;
	}

	if(length > size)
	{
		printf("|||| WARNING CHECKSUM LENGTH IS GREATER THEAN EEPROM SIZE\n");
		checkSumLengthError =1;
		length = size;
	}

	if(!checkSumLengthError)
	{
		*checkSumLengthLib = length;
//		printf("CheckSumLength in get_eeprom_size() = %x\n",*checkSumLength);
		*eepromSizeLib = size;
//		printf("CheckSumLength in get_eeprom_size() = %x\n",*checkSumLength);
	return 0;
        }
	else
	{
		*checkSumLengthLib = length;
//		printf(" IN ELSE CheckSumLength in get_eeprom_size() = %x\n",*checkSumLength);
		*eepromSizeLib = size;
//		printf("IN ELSE CheckSumLength in get_eeprom_size() = %x\n",*checkSumLength);
		return 1;
	}
}

/**************************************************************************
* eepromVerifyChecksum - Verify EEPROM checksum
*
*
* RETURNS: TRUE if OK, FALSE otherwise
*
*/
A_BOOL eepromVerifyChecksum
(
 A_UINT32	devNum
)
{
	A_UINT32	i;
	A_UINT32 	*data;
	A_UINT16	addr=0xc0;
	A_UINT32	chksum = 0,length=0;

    LIB_DEV_INFO *pLibDev = gLibInfo.pLibDevArray[devNum];


	if ((pLibDev->eepromStartLoc != 0x000) && (pLibDev->eepromStartLoc != 0x400)) {
		mError(devNum, EINVAL, "eepromVerifyChecksum : illegal eepromStartLoc : 0x%x [legal: 0x00 or 0x400]\n", pLibDev->eepromStartLoc);
		return FALSE;
	}
	//This needs to be called again in lib incase its an AP or MDK
    getEepromSize(devNum,&eepromSizeLib,&checkSumLengthLib);
	length = checkSumLengthLib;

	data = (A_UINT32 *)calloc((length-addr),sizeof(A_UINT32));
	if(data==NULL)
	{
		printf("Memory not allocated in eepromVerifyChecksum\n");
		return FALSE;
	}
	for(i=0;i<length-addr;i++)
		data[i]=0xffff;

	eepromReadBlock(devNum,addr,length-addr,data );
	for (i=0x0; i < length-addr; i++) 	{
		//data = eepromRead(devNum,(addr + pLibDev->eepromStartLoc));
	//	printf("data[%x]= %x\n",i,data[i]);
		chksum ^= (data[i] & 0xffff);
	}
	//printf(" checksum in eepromverify check sum =%x\n", chksum);
	chksum &= 0xffff;
	if (chksum != 0xffff) {
		return FALSE;
	}

	return TRUE;
}


/**************************************************************************
* eepromWriteBlock - Call correct eepromWrite function
*
* Write a block of eeprom starting from the startOffset with
* the eepromValue
*
*/
MANLIB_API void eepromWriteBlock
(
	A_UINT32 devNum,
	A_UINT32 startOffset,
	A_UINT32 length,
	A_UINT32 *eepromValue
)
{
   	LIB_DEV_INFO *pLibDev = gLibInfo.pLibDevArray[devNum];
	A_UINT32 i = pLibDev->ar5kInitIndex;
	A_UINT32 j;

	if (checkDevNum(devNum) == FALSE) {
		mError(devNum, EINVAL, "Device Number %d:eepromWriteBlock\n", devNum);
		return;
	}

	if (ar5kInitData[i].pMacAPI->eepromWrite == NULL) {
		return;
	}
    if (isDragon(devNum)) {
        mError(devNum, EIO, "Dragon does not support writing EEPROM blocks yet\n");
        return;
    }

#ifdef OWL_PB42
 {//for Howl
 if(isHowlAP(devNum) || isFlashCalData()){// Howl uses block write
        A_UINT8 *pVal;
        A_UINT16 *pTempBuffer;
        A_UINT16 *pTempPtr;

        //copy the data into 16bit data buffer
        pTempBuffer = (A_UINT16 *)malloc(length * sizeof(A_UINT16));
        pTempPtr = pTempBuffer;
        for(j = 0; j < length; j++) {
//#ifdef ENDIAN_SWAP
//              *eepromValue = btol_l(*eepromValue);
//#endif
                *pTempPtr = *eepromValue & 0xffff;
                eepromValue++;
                pTempPtr++;
        }

        pVal = (A_UINT8 *)pTempBuffer;
// 16 bit addressing
//      q_uiPrintf("Write to eeprom @ %x : %x \n",eepromOffset,eepromValue);
        startOffset = startOffset << 1;
// write only the lower 2 bytes
        sysFlashConfigWrite(devNum, FLC_RADIOCFG, startOffset, pVal , length*sizeof(A_UINT16));
//      free(pTempBuffer);
        return;

   }//isHowlAP
 }//for Howl
#endif

//make AP eeprom block writing more efficient
#if defined(SPIRIT_AP) || defined(FREEDOM_AP) || (defined(OWL_AP) && (!defined(OWL_OB42)))
{
	A_UINT8 *pVal;
	A_UINT16 *pTempBuffer;
	A_UINT16 *pTempPtr;

	//copy the data into 16bit data buffer
	pTempBuffer = (A_UINT16 *)malloc(length * sizeof(A_UINT16));
	pTempPtr = pTempBuffer;
	for(j = 0; j < length; j++) {
#ifdef ARCH_BIG_ENDIAN
//#ifdef ENDIAN_SWAP
//		*eepromValue = btol_l(*eepromValue);
//#endif
#else 
#ifdef ENDIAN_SWAP
		*eepromValue = btol_l(*eepromValue);
#endif
#ifdef RSP_BLOCKSWAP
	*eepromValue = ((*eepromValue & 0xFF) << 8) | ((*eepromValue & 0xFF00) >> 8); // RSP-ENDIAN only for testing
#endif//RSP_BLOCKSWAP
#endif
		*pTempPtr = *eepromValue & 0xffff;
		eepromValue++;
		pTempPtr++;
	}

	pVal = (A_UINT8 *)pTempBuffer;
// 16 bit addressing
//	q_uiPrintf("Write to eeprom @ %x : %x \n",eepromOffset,eepromValue);
	startOffset = startOffset << 1;
// write only the lower 2 bytes
	sysFlashConfigWrite(devNum, FLC_RADIOCFG, startOffset, pVal , length*sizeof(A_UINT16));
//	free(pTempBuffer);
	return;
}
#else

if (isFalcon(devNum)) {
	pLibDev->blockFlashWriteOn = TRUE;
}

	for (j=0;j<length;j++) {
//..Siva..		ar5kInitData[i].pMacAPI->eepromWrite(devNum,(startOffset + j), *eepromValue);
        eepromWrite(devNum,(startOffset + j), *eepromValue);
		if (pLibDev->mdkErrno) return;
		eepromValue++;
	}

if (isFalcon(devNum)) {
	pLibDev->blockFlashWriteOn = FALSE;
}

#endif
	return;
}

/**************************************************************************
* eepromReadBlock - call correct eepromRead Function
*
* RETURNS: An array of 16 bit value from given offset (in a 32-bit array)
*/
MANLIB_API void eepromReadBlock
(
	A_UINT32 devNum,
	A_UINT32 startOffset,
	A_UINT32 length,
	A_UINT32 *eepromValue
)
{
    LIB_DEV_INFO *pLibDev = gLibInfo.pLibDevArray[devNum];
	A_UINT32 i = pLibDev->ar5kInitIndex;
	A_UINT32 j;

#ifdef RSP_BLOCKSWAP
	A_UINT32 swp; 
#endif

#ifdef OWL_PB42

    A_UINT16 temp_buf[2048];
#endif
	if (checkDevNum(devNum) == FALSE) {
		mError(devNum, EINVAL, "Device Number %d:eepromReadBlock\n", devNum);
		return;
	}
//    if (isDragon(devNum)) {
//       mError(devNum, EIO, "Dragon does not support reading EEPROM blocks yet\n");
//       return;
//    }


    if (pLibDev->devMap.remoteLib && (pLibDev->devMap.r_eepromReadBlock != NULL)) {
	   pLibDev->devMap.r_eepromReadBlock(devNum, startOffset, length, eepromValue);
	}
	else {
      if (ar5kInitData[i].pMacAPI->eepromRead == NULL) {
		return;
	  }

#ifdef OWL_PB42
	  if(isHowlAP(devNum) || isFlashCalData())
        {
		A_UINT16 temp_buf[2048];
                 startOffset=startOffset*2;
                 flash_read(devNum,2,temp_buf,length, startOffset); // 2 means cal data .. change it later..
                  for(j=0;j<length;j++){
                    eepromValue[j]=(A_UINT32)temp_buf[j];
                    if (pLibDev->mdkErrno) return;
                  }
        }
        else
       {
#endif
	  for  (j=0;j<length;j++)	{

	    #ifdef RSP_BLOCKSWAP
		  	  swp = eepromRead(devNum, startOffset + j); // RSP-ENDIAN
			  eepromValue[j] = ((swp & 0xFF) << 8) | ((swp & 0xFF00) >> 8); // only for testing
			  //printf("RSP-BLOCKSWAP (%s) %x-->%x\n",__func__,swp,eepromValue[j]);
	    #else
		  	  eepromValue[j] = eepromRead(devNum, startOffset + j);
	    #endif

              if (pLibDev->mdkErrno) return;
	  }
#ifdef OWL_PB42
     }// end of isHowlAP
#endif

	}
}

/**************************************************************************
* getEepromStruct - get pointer to specified eeprom struct
*
* Get and return to caller pointer to eeprom struct held by LIB_DEV_INFO
*
* RETURNS:
*/
MANLIB_API void getEepromStruct
(
 A_UINT32 devNum,
 A_UINT16 eepStructFlag,	//which eeprom strcut
 void **ppReturnStruct,		//return ptr to struct asked for
 A_UINT32 *pNumBytes		//return the size of the structure
)
{
    LIB_DEV_INFO *pLibDev = gLibInfo.pLibDevArray[devNum];

	switch(eepStructFlag) {
	case EEP_HEADER_16K:
		*ppReturnStruct = pLibDev->p16kEepHeader;
		*pNumBytes		= sizeof(MDK_EEP_HEADER_INFO);
		break;

	case EEP_HEADER_16K_CHAIN1:
		*ppReturnStruct = pLibDev->chain[1].p16kEepHeader;
		*pNumBytes		= sizeof(MDK_EEP_HEADER_INFO);
		break;

	case EEP_CHANNEL_INFO_16K:
		*ppReturnStruct = pLibDev->pCalibrationInfo;
		*pNumBytes = sizeof(MDK_PCDACS_ALL_MODES);
		break;

	case EEP_TRGT_POWER_16K:
		*ppReturnStruct = pLibDev->p16KTrgtPowerInfo;
		*pNumBytes = sizeof(MDK_TRGT_POWER_ALL_MODES);
		break;

	case EEP_RD_POWER_16K:
		*ppReturnStruct = pLibDev->p16KRdEdgesPower;
		*pNumBytes = sizeof(MDK_RD_EDGES_POWER) * NUM_16K_EDGES * pLibDev->p16kEepHeader->numCtl;
		break;

	case EEP_GEN3_CHANNEL_INFO:
		*ppReturnStruct = pLibDev->pGen3CalData;
		*pNumBytes = sizeof(EEPROM_FULL_DATA_STRUCT_GEN3);
		break;

	case EEP_GEN5_CHANNEL_INFO:
		*ppReturnStruct = pLibDev->pGen5CalData;
		*pNumBytes = sizeof(EEPROM_FULL_DATA_STRUCT_GEN5);
//		printf("SNOOP: size struct = %d, first uint16 = %d\n", *pNumBytes,
//			*((A_UINT16 *)ppReturnStruct));
		break;

#if !defined(FREEDOM_AP)
#if defined(LINUX)
	case EEP_AR6000:
		getAR6000Struct(devNum, ppReturnStruct, pNumBytes);
		break;
#endif
	case EEP_AR5416:
		getAR5416Struct(devNum, ppReturnStruct, pNumBytes);
		break;
#endif
	default:
		mError(devNum, EINVAL, "Device Number %d:getEepromStruct: Illegal structure type\n", devNum);
		return;

	}

	return;
}

//static A_INT16         minPower, midPower, maxPower;
//static A_UINT16        xpdGainValues[2];
//static A_UINT16		numPdGain;
static A_INT16         minCalPowerGen5_t2;
/**************************************************************************
* forcePowerTxMax - Write the max power values to device registers
*
*
* RETURNS:
*/
MANLIB_API void forcePowerTxMax
(
 A_UINT32        devNum,
 A_INT16        *pRatesPower
)
{
    LIB_DEV_INFO    *pLibDev = gLibInfo.pLibDevArray[devNum];
    A_UINT16        mask = 0x3f;
    A_UINT16        regOffset = 0x9934;
    A_UINT32        reg32;
    A_BOOL            paPreDEnable = 0;
    A_UINT16        powerValues[PWR_TABLE_SIZE];
    A_UINT16        pdadcValues[PDADC_TABLE_SIZE];
    A_UINT16        gainBoundaries[MAX_NUM_PDGAINS_PER_CHANNEL];
    MODE_HEADER_INFO *pModeInfo = NULL;
    A_UINT32        mode = pLibDev->mode;
    A_UINT16        i;
    A_UINT16        pwr_index_offset;
    double          cck_ofdm_correction;
    A_UINT16        numPdGainsUsed;
    A_UINT32        pdGainOverlap_t2;
    A_UINT32        regValue;
    A_INT16         minPower, maxPower;
    A_BOOL			falconTrue;
    A_INT16         pRatesPowerBkp[NUM_RATES];

	falconTrue = isFalcon(devNum);

	if (falconTrue) {
		for (i = 0; i < NUM_RATES; i++) {
			pRatesPowerBkp[i] = pRatesPower[i];
		}
	}

#if defined(LINUX)
	if (isDragon(devNum)) {
	    ar6000EepromSetTransmitPower(devNum, pLibDev->freqForResetDevice, (A_UINT16 *)pRatesPower);
		return;
	}
#endif

	if (isOwl(pLibDev->swDevID)) {
	    ar5416EepromSetTransmitPower(devNum, pLibDev->freqForResetDevice, (A_UINT16 *)pRatesPower);
		return;
	}

    //if this is a version 4.0 eeprom, then need to setup the pcdac table
    //and xpdgain any time when change the power.
    if (pLibDev->eePromLoad) {
        if((pLibDev->p16kEepHeader->majorVersion >= 4) && (pLibDev->p16kEepHeader->eepMap == 1)){
            switch(mode) {
            case MODE_11A:
                if (!pLibDev->p16kEepHeader->Amode) {
                    return;
                }
                pModeInfo = &(pLibDev->p16kEepHeader->info11a);
                break;

            case MODE_11G:
            case MODE_11O:
                if (!pLibDev->p16kEepHeader->Gmode) {
                    return;
                }
                pModeInfo = &(pLibDev->p16kEepHeader->info11g);
                mode = MODE_11G;
                break;

            case MODE_11B:
                if (!pLibDev->p16kEepHeader->Bmode) {
                    return;
                }
                pModeInfo = &(pLibDev->p16kEepHeader->info11b);
                break;

            }

            //calculate the max and min power from the target powers
            getMaxMinPower(pRatesPower, &minPower, &maxPower);

            if((pLibDev->swDevID & 0xff) >= 0x16) {
                pLibDev->eepData.numPdGain = 2;
            }
            else {
                pLibDev->eepData.numPdGain = 1;
            }
            //pass this to the function to get the pcdac table and xpdgain
            get_xpd_gain_and_pcdacs_for_powers(devNum, (A_UINT16)pLibDev->freqForResetDevice, pLibDev->pGen3RawData[mode],
                                pLibDev->eepData.numPdGain, pModeInfo->xgain, &minPower, &maxPower, &(pLibDev->eepData.midPower), pLibDev->eepData.xpdGainValues, powerValues);

            //write power tables to registers
            forcePCDACTable(devNum, powerValues);

            //write xpdgain
            if((pLibDev->swDevID & 0xff) >= 0x16) {
                writeField(devNum, "rf_pdgain_lo", pLibDev->eepData.xpdGainValues[0]);
                writeField(devNum, "rf_pdgain_hi", pLibDev->eepData.xpdGainValues[1]);
            }
            else {
                writeField(devNum, "rf_xpd_gain", pLibDev->eepData.xpdGainValues[0]);
            }

            //Note the pcdac table may no longer start at 0 power, could be negative or greater than 0.
            //need to offest the power values by the amount of maxPower
            if(maxPower < 63) {
                pwr_index_offset = (A_INT16)(63 - maxPower);
                pLibDev->pwrIndexOffset = pwr_index_offset;
                for(i = 0; i < NUM_RATES; i++) {
                    pRatesPower[i] = (A_INT16)(pRatesPower[i] + pwr_index_offset);
                    if (pRatesPower[i] > 63) pRatesPower[i] = 63;
                }
            } else {
                pLibDev->pwrIndexOffset = 0;
            }
        }
    }
    // program the pdadc table for griffin
    if((pLibDev->eePromLoad && pLibDev->eepData.infoValid) &&
        (pLibDev->p16kEepHeader->majorVersion >= 5) && (pLibDev->p16kEepHeader->eepMap == 2)) {

            switch(mode) {
            case MODE_11A:
                if (!pLibDev->p16kEepHeader->Amode) {
                    return;
                }
                pModeInfo = &(pLibDev->p16kEepHeader->info11a);
                break;

            case MODE_11G:
            case MODE_11O:
                if (!pLibDev->p16kEepHeader->Gmode) {
                    return;
                }
                pModeInfo = &(pLibDev->p16kEepHeader->info11g);
                mode = MODE_11G;
                break;

            case MODE_11B:
                if (!pLibDev->p16kEepHeader->Bmode) {
                    return;
                }
                pModeInfo = &(pLibDev->p16kEepHeader->info11b);
                break;

            }

            numPdGainsUsed = pLibDev->pGen5RawData[mode]->pDataPerChannel[0].numPdGains;
            //calculate the max and min power from the target powers
            getMaxMinPower(pRatesPower, &minPower, &maxPower);

            //FJC, replace this with a register read, field not present in hainan_derby config file
            //so can't do a get field for mode incase we are loading ear and using this config file
            //pdGainOverlap_t2 = (A_UINT16)getFieldForMode(devNum, "bb_pd_gain_overlap", mode, pLibDev->turbo);
            pdGainOverlap_t2 = REGR(devNum, TPCRG5_REG) & BB_PD_GAIN_OVERLAP_MASK;

            //pass this to the function to get the pdadc table, gain boundaries and pdgain list
            get_gain_boundaries_and_pdadcs_for_powers(devNum, (A_UINT16)pLibDev->freqForResetDevice, pLibDev->pGen5RawData[mode],
                                (A_UINT16)pdGainOverlap_t2, &(minCalPowerGen5_t2), gainBoundaries, pLibDev->eepData.xpdGainValues, pdadcValues);

            //write power tables to registers
            forcePDADCTable(devNum, pdadcValues, gainBoundaries);

            //write xpdgain
//            if(isGriffin(pLibDev->swDevID)) {
            if(pLibDev->p16kEepHeader->eepMap >= 2) {
                //can't use field name incase load ear
                //writeField(devNum, "bb_num_pd_gain", (numPdGainsUsed - 1));
                regValue = REGR(devNum, TPCRG1_REG);
                regValue = (regValue & BB_NUM_PD_GAIN_CLEAR) | ((numPdGainsUsed - 1) << BB_NUM_PD_GAIN_SHIFT);
                if (numPdGainsUsed >= 1) {
                    //writeField(devNum, "bb_pd_gain_setting1", pLibDev->eepData.xpdGainValues[0]);
                    regValue = (regValue & BB_PD_GAIN_SETTING1_CLEAR) | (pLibDev->eepData.xpdGainValues[0] << BB_PD_GAIN_SETTING1_SHIFT);
                }
                if (numPdGainsUsed >= 2) {
                    //writeField(devNum, "bb_pd_gain_setting2", pLibDev->eepData.xpdGainValues[1]);
                    regValue = (regValue & BB_PD_GAIN_SETTING2_CLEAR) | (pLibDev->eepData.xpdGainValues[1] << BB_PD_GAIN_SETTING2_SHIFT);
                }
                if (numPdGainsUsed >= 3) {
                    //writeField(devNum, "bb_pd_gain_setting2", pLibDev->eepData.xpdGainValues[1]);
                    regValue = (regValue & BB_PD_GAIN_SETTING3_CLEAR) | (pLibDev->eepData.xpdGainValues[2] << BB_PD_GAIN_SETTING3_SHIFT);
                }
                REGW(devNum, TPCRG1_REG, regValue);
            }

            //Note the pdadc table may not start at 0dBm power, could be negative or greater than 0.
            //need to offest the power values by the amount of minPower for griffin
            if(minCalPowerGen5_t2 != 0) {
                pwr_index_offset = (A_INT16)(0 - minCalPowerGen5_t2);
                pLibDev->pwrIndexOffset = pwr_index_offset;
                for(i = 0; i < NUM_RATES; i++) {
                    pRatesPower[i] = (A_INT16)(pRatesPower[i] + pwr_index_offset);
                    if (pRatesPower[i] > 63) pRatesPower[i] = 63;
                }
            } else {
                pLibDev->pwrIndexOffset = 0;
            }
    } // end Griffin

    if ( ((pLibDev->swDevID & 0xff) >= 0x0013) &&  // do it for all post-venice boards
         (!isGriffin(pLibDev->swDevID) && !isEagle(pLibDev->swDevID) && !isOwl(pLibDev->swDevID)) &&         // except for griffin & eagle
         (pLibDev->mode == MODE_11G) && (pLibDev->eePromLoad)) {
            if ((pLibDev->swDevID & 0xff) == 0x0013) {
                getPwrTableByModeCh(devNum, MODE_11G, pLibDev->freqForResetDevice, &(powerValues[0]));
            }
            cck_ofdm_correction = (double)pLibDev->p16kEepHeader->scaledOfdmCckDelta/10.0;
            if (pLibDev->freqForResetDevice == 2484) {
                cck_ofdm_correction -= (double)pLibDev->p16kEepHeader->scaledCh14FilterCckDelta/10.0;
            }
            forceChainPowerTxMaxVenice(devNum, &(pRatesPower[0]), &(powerValues[0]), cck_ofdm_correction, 0);

	    if (falconTrue) {
			forceChainPowerTxMax(devNum, (A_INT16 *)pRatesPowerBkp, 1);
	    }
            return;
    }

    reg32 = ( ((paPreDEnable & 1)<< 30) | ((pRatesPower[3] & mask) << 24) |
         ((paPreDEnable & 1)<< 22) | ((pRatesPower[2] & mask) << 16) |
         ((paPreDEnable & 1)<< 14) | ((pRatesPower[1] & mask) << 8) |
         ((paPreDEnable & 1)<< 6 ) |  (pRatesPower[0] & mask)  );
    REGW(devNum, regOffset, reg32);
    regOffset = 0x9938;
    reg32 = ( ((paPreDEnable & 1)<< 30) | ((pRatesPower[7] & mask) << 24) |
         ((paPreDEnable & 1)<< 22) | ((pRatesPower[6] & mask) << 16) |
         ((paPreDEnable & 1)<< 14) | ((pRatesPower[5] & mask) << 8) |
         ((paPreDEnable & 1)<< 6 ) |  (pRatesPower[4] & mask)  );
    REGW(devNum, regOffset, reg32);

    regOffset = 0x993c;

    //set max power to the power value at rate 6
//    REGW(devNum, regOffset, pRatesPower[0]);
    if((pLibDev->eePromLoad && pLibDev->eepData.infoValid) &&
        (pLibDev->p16kEepHeader->majorVersion >= 5)    && (pLibDev->p16kEepHeader->eepMap >= 2)) {
        REGW(devNum, regOffset, gainBoundaries[MAX_NUM_PDGAINS_PER_CHANNEL-1]);
    } else {
        REGW(devNum, regOffset, 63);
    }

    regOffset = 0xa234;
    reg32 = (((pRatesPower[10]  & mask) << 24) |
             ((pRatesPower[9] & mask) << 16) |
             ((pRatesPower[XR_POWER_INDEX]  & mask) << 8)  |
              (pRatesPower[8]  & mask) );
    REGW(devNum, regOffset, reg32);

    regOffset = 0xa238;
    reg32 = (((pRatesPower[14] & mask) << 24) |
             ((pRatesPower[13] & mask) << 16) |
             ((pRatesPower[12] & mask) << 8)  |
              (pRatesPower[11]  & mask) );
    REGW(devNum, regOffset, reg32);

	if (isFalcon(devNum)) {
		forceChainPowerTxMax(devNum, (A_INT16 *)pRatesPowerBkp, 1);
	}

	if (isOwl(pLibDev->swDevID)) {
		//set the new rates - temp fix for now all set to first power value
		reg32 = (((pRatesPower[0] & mask) << 24) |
				 ((pRatesPower[0] & mask) << 16) |
				 ((pRatesPower[0] & mask) << 8)  |
				  (pRatesPower[0] & mask) );
		REGW(devNum, 0xa38c, reg32);
		REGW(devNum, 0xa390, reg32);
		REGW(devNum, 0xa3cc, reg32);
		REGW(devNum, 0xa3d0, reg32);
		REGW(devNum, 0xa3d4, reg32);
//getch();
	}
}

/***********************************************************************************
* forceChainPowerTxMax - Write the max power values to device registers for chainNum
*
*
* RETURNS:
*/
MANLIB_API void forceChainPowerTxMax
(
 A_UINT32		devNum,
 A_INT16		*pRatesPower,
 A_UINT32       chainNum
)
{
    LIB_DEV_INFO	*pLibDev = gLibInfo.pLibDevArray[devNum];
	A_UINT16		mask = 0x3f;
	A_UINT16		regOffset = 0x9934;
	A_UINT32		reg32;
	A_BOOL			paPreDEnable = 0;
	A_UINT16		powerValues[PWR_TABLE_SIZE];
	MODE_HEADER_INFO *pModeInfo = NULL;
	A_UINT32        mode = pLibDev->mode;
	A_UINT16        i;
	A_UINT16        pwr_index_offset;
	double          cck_ofdm_correction;
	A_BOOL			falconTrue;
    A_INT16         minPower, maxPower;
	A_UINT16        xpdGainValues[2];
	A_UINT32        phaseCal;

	falconTrue = isFalcon(devNum);

	if (!falconTrue) {
		return;
	}

	//if this is a version 4.0 eeprom, then need to setup the pcdac table
	//and xpdgain any time when change the power.
	if (pLibDev->eePromLoad) {
		if((pLibDev->p16kEepHeader->majorVersion == 4) && (pLibDev->p16kEepHeader->eepMap == 1)){
			switch(mode) {
			case MODE_11A:
				if (!pLibDev->p16kEepHeader->Amode) {
					return;
				}
				pModeInfo = &(pLibDev->p16kEepHeader->info11a);
				break;

			case MODE_11G:
			case MODE_11O:
				if (!pLibDev->p16kEepHeader->Gmode) {
					return;
				}
				pModeInfo = &(pLibDev->p16kEepHeader->info11g);
				mode = MODE_11G;
				break;

			case MODE_11B:
				if (!pLibDev->p16kEepHeader->Bmode) {
					return;
				}
				pModeInfo = &(pLibDev->p16kEepHeader->info11b);
				break;

			}

			//calculate the max and min power from the target powers
			getMaxMinPower(pRatesPower, &minPower, &maxPower);

			if((pLibDev->swDevID & 0xff) >= 0x16) {
				pLibDev->eepData.numPdGain = 2;
			}
			else {
				pLibDev->eepData.numPdGain = 1;
			}
			//pass this to the function to get the pcdac table and xpdgain
//			get_xpd_gain_and_pcdacs_for_powers(devNum, (A_UINT16)pLibDev->freqForResetDevice, pLibDev->chain[chainNum].pGen3RawData[mode],
//								pLibDev->eepData.numPdGain, pModeInfo->xgain, &minPower, &maxPower, &midPower, xpdGainValues, powerValues);
            get_xpd_gain_and_pcdacs_for_powers(devNum, (A_UINT16)pLibDev->freqForResetDevice, pLibDev->pGen3RawData[mode],
                                pLibDev->eepData.numPdGain, pModeInfo->xgain, &minPower, &maxPower, &(pLibDev->eepData.midPower), xpdGainValues, powerValues);


			//write power tables to registers
			forceChainPCDACTable(devNum, powerValues, chainNum);

			//write xpdgain
			if((pLibDev->swDevID & 0xff) >= 0x16) {
				writeField(devNum, "rf_pdgain_lo", xpdGainValues[0]);
				writeField(devNum, "rf_pdgain_hi", xpdGainValues[1]);
			}
			else {
				writeField(devNum, "rf_xpd_gain", xpdGainValues[0]);
			}

			//Note the pcdac table may no longer start at 0 power, could be negative or greater than 0.
			//need to offest the power values by the amount of maxPower
			if(maxPower < 63) {
				pwr_index_offset = (A_INT16)(63 - maxPower);
				pLibDev->pwrIndexOffset = pwr_index_offset;
				for(i = 0; i < NUM_RATES; i++) {
					pRatesPower[i] = (A_INT16)(pRatesPower[i] + pwr_index_offset);
					if (pRatesPower[i] > 63) pRatesPower[i] = 63;
				}
			} else {
				pLibDev->pwrIndexOffset = 0;
			}
		}
	}

	if ( ((pLibDev->swDevID & 0xff) >= 0x0013) &&  // do it for all post-venice boards
		( pLibDev->mode == MODE_11G) && (pLibDev->eePromLoad)) {
			if ((pLibDev->swDevID & 0xff) == 0x0013) {
				getPwrTableByModeCh(devNum, MODE_11G, pLibDev->freqForResetDevice, &(powerValues[0]));
			}
			cck_ofdm_correction = (double)pLibDev->p16kEepHeader->scaledOfdmCckDelta/10.0;
			if (pLibDev->freqForResetDevice == 2484) {
				cck_ofdm_correction -= (double)pLibDev->p16kEepHeader->scaledCh14FilterCckDelta/10.0;
			}
			forceChainPowerTxMaxVenice(devNum, &(pRatesPower[0]), &(powerValues[0]), cck_ofdm_correction, chainNum);
			return;
	}
	reg32 = ( ((paPreDEnable & 1)<< 30) | ((pRatesPower[3] & mask) << 24) |
	     ((paPreDEnable & 1)<< 22) | ((pRatesPower[2] & mask) << 16) |
	     ((paPreDEnable & 1)<< 14) | ((pRatesPower[1] & mask) << 8) |
	     ((paPreDEnable & 1)<< 6 ) |  (pRatesPower[0] & mask)  );
	REGW(devNum, regOffset+ (chainNum*0x1000), reg32);
	regOffset = 0x9938;
	reg32 = ( ((paPreDEnable & 1)<< 30) | ((pRatesPower[7] & mask) << 24) |
	     ((paPreDEnable & 1)<< 22) | ((pRatesPower[6] & mask) << 16) |
	     ((paPreDEnable & 1)<< 14) | ((pRatesPower[5] & mask) << 8) |
	     ((paPreDEnable & 1)<< 6 ) |  (pRatesPower[4] & mask)  );
	REGW(devNum, regOffset + (chainNum*0x1000), reg32);

	regOffset = 0x993c;

	//set max power to the power value at rate 6

	REGW(devNum, regOffset + (chainNum*0x1000), 63); // come here only for falcon

	regOffset = 0xa234;
	reg32 = (((pRatesPower[10]  & mask) << 24) |
	         ((pRatesPower[9] & mask) << 16) |
			 ((pRatesPower[XR_POWER_INDEX]  & mask) << 8)  |
			  (pRatesPower[8]  & mask) );
	REGW(devNum, regOffset + (chainNum*0x1000), reg32);

	regOffset = 0xa238;
	reg32 = (((pRatesPower[14] & mask) << 24) |
	         ((pRatesPower[13] & mask) << 16) |
			 ((pRatesPower[12] & mask) << 8)  |
			  (pRatesPower[11]  & mask) );
	REGW(devNum, regOffset + (chainNum*0x1000), reg32);

	phaseCal = getPhaseCal(devNum, pLibDev->freqForResetDevice);
	if (pLibDev->libCfgParams.phaseDelta != UNINITIALIZED_PHASE_DELTA) {
		setChain(devNum, pLibDev->libCfgParams.chainSelect, pLibDev->libCfgParams.phaseDelta);
	} else {
		setChain(devNum, pLibDev->libCfgParams.chainSelect, phaseCal);
	}

}

/**************************************************************************
* getXpdgainForPower - Using the min, mid and max power from the last call
*                    to forcePowerTxMax, get the xpdgain value that would
*                    apply to the specified power
*
*
* RETURNS:
*/
MANLIB_API A_UINT16 getXpdgainForPower
(
 A_UINT32	devNum,
 A_INT32    powerIn
)
{
    LIB_DEV_INFO    *pLibDev = gLibInfo.pLibDevArray[devNum];
	A_UINT16 xpdGain= 0xdead;

/*	if((powerIn < minPower) || (powerIn > maxPower)) {
		//power asked for is outside bounds of power values last applied
		mError(devNum, EINVAL, "getXpdGainForPower: invalid power value passed in\n");
		return 0xdead;
	}
*/

	if (pLibDev->eepData.numPdGain == 1) {
		xpdGain = pLibDev->eepData.xpdGainValues[0];
	}
	else {
		if (powerIn > pLibDev->eepData.midPower) {
			xpdGain = pLibDev->eepData.xpdGainValues[0];
		}
		else {
			xpdGain = pLibDev->eepData.xpdGainValues[1];
		}
	}
	devNum = 0; //quiet warnings.
	return(xpdGain);
}


/**************************************************************************
* forceSinglePowerTxMax - Write the sane max power value to device registers
*
*
* RETURNS:
*/
MANLIB_API void forceSinglePowerTxMax
(
 A_UINT32		devNum,
 A_INT16		powerValue
)
{
    LIB_DEV_INFO	*pLibDev = gLibInfo.pLibDevArray[devNum];
	A_INT16 powerArray[Ar5416RateSize];   //make bigger for 11n rates
	A_UINT16 i;
	A_UINT16 numRates = NUM_RATES;

	if(isOwl(pLibDev->swDevID)) {
		numRates = Ar5416RateSize;
	}

	for (i = 0; i < numRates; i++) {
		powerArray[i] = powerValue;
	}
	forcePowerTxMax(devNum, powerArray);
}

void
getMaxMinPower(
 A_INT16 powerArray[],
 A_INT16 *minPower,
 A_INT16 *maxPower)
{
	A_UINT16 i;
	*minPower = 0xff;
	*maxPower = 0;
	for(i = 0; i < XR_POWER_INDEX; i++) {
		if(powerArray[i] >= *maxPower) {
			*maxPower = powerArray[i];
		}

		if(powerArray[i] <= *minPower) {
			*minPower = powerArray[i];
		}
	}
	return;
}

/**
 * - Function Name: getPowerIndex
 * - Description  : returns array index that will point to 2x the desired rf
 *                  output power (in dBm).  Note that if pLibDev->pwrIndexOffset
 *                  is zero, this function will return the same value as
 *                  getMaxPowerForRate()
 * - Arguments
 *     -
 * - Returns      :
 ******************************************************************************/
MANLIB_API A_UINT16 getPowerIndex(
	A_UINT32 devNum,
	A_INT32 twicePower)
{
    LIB_DEV_INFO	*pLibDev = gLibInfo.pLibDevArray[devNum];
	A_UINT16         retVal;
	A_INT32		     tmpVal;

	if(twicePower >= PWR_TABLE_SIZE) {
		if (twicePower == 0xFF) {
			return(0);
		} else {
			mError(devNum, EINVAL, "in getPowerIndex : Device Number %d : twicePower value should be less than %d\n", devNum, PWR_TABLE_SIZE);
			return(0xdead);
		}
	}

	tmpVal = twicePower + (A_INT32)pLibDev->pwrIndexOffset;
	if (tmpVal < 0) {
		retVal = 0;
	} else {
		retVal = (A_UINT16) tmpVal;
	}

	if (retVal >= PWR_TABLE_SIZE) {
		retVal = (PWR_TABLE_SIZE - 1);
	}

	return(retVal);

}
/* getPowerIndex */

MANLIB_API A_UINT16 getPcdacForPower
(
	A_UINT32 devNum,
	A_UINT32 freq,
	A_INT32 twicePower
)
{
    LIB_DEV_INFO	*pLibDev = gLibInfo.pLibDevArray[devNum];
	MDK_FULL_PCDAC_STRUCT  pcdacStruct;
	A_UINT16			powerValues[PWR_TABLE_SIZE];
	MDK_PCDACS_EEPROM	eepromPcdacs;

	//only override the power if the eeprom has been read
	if((!pLibDev->eePromLoad) || (!pLibDev->eepData.infoValid)) {
		return(0xdead);
	}

    if (isDragon(devNum)) {
	   mError(devNum, EIO, "Dragon does not have pcdacs based on a power index (getPcdacForPower)\n");
       return 0;
    }

	if(twicePower >= PWR_TABLE_SIZE) {
		mError(devNum, EINVAL, "Device Number %d:twicePower value should be less than %d\n", devNum, PWR_TABLE_SIZE);
		return(0xdead);
	}

	//fill out the full pcdac info for this channel
	pcdacStruct.channelValue = (A_UINT16)freq;
	//setup the pcdac struct to point to the correct info, based on mode
	switch(pLibDev->mode) {
	case MODE_11A:
		eepromPcdacs.numChannels = pLibDev->pCalibrationInfo->numChannels_11a;
		eepromPcdacs.pChannelList = pLibDev->pCalibrationInfo->Channels_11a;
		eepromPcdacs.pDataPerChannel = pLibDev->pCalibrationInfo->DataPerChannel_11a;
		break;

	case MODE_11G:
	case MODE_11O:
		eepromPcdacs.numChannels = pLibDev->pCalibrationInfo->numChannels_2_4;
		eepromPcdacs.pChannelList = pLibDev->pCalibrationInfo->Channels_11g;
		eepromPcdacs.pDataPerChannel = pLibDev->pCalibrationInfo->DataPerChannel_11g;
		break;

	case MODE_11B:
		eepromPcdacs.numChannels = pLibDev->pCalibrationInfo->numChannels_2_4;
		eepromPcdacs.pChannelList = pLibDev->pCalibrationInfo->Channels_11b;
		eepromPcdacs.pDataPerChannel = pLibDev->pCalibrationInfo->DataPerChannel_11b;
		break;

	}

	mapPcdacTable(&eepromPcdacs, &pcdacStruct);

	//get the power table from the pcdac table
	getPwrTable(&pcdacStruct, powerValues);

	return(powerValues[twicePower]);
}


MANLIB_API A_UINT32 getPhaseCal
(
 A_UINT32 devNum,
 A_UINT32 freq
)
{
    LIB_DEV_INFO	*pLibDev = gLibInfo.pLibDevArray[devNum];
	A_INT32         retVal = 0;
	A_UINT32        idxL = 0, idxR = 0;
	A_UINT32        lVal, rVal;

	if (!pLibDev->eepData.infoValid) {
		return(0);
	}

	if (pLibDev->mode == MODE_11G) {
		// 2412, 2472
		lVal = pLibDev->p16kEepHeader->info11g.phaseCal[0];
		rVal = pLibDev->p16kEepHeader->info11g.phaseCal[1];

		if (rVal > lVal) {
			if ((rVal - lVal) > 180) {
				lVal += 360;  // must've wrapped around 0
			}
		} else {
			if ((lVal-rVal) > 180) {
				rVal += 360;
			}
		}

		retVal = ( (A_INT32)(freq - 2412)*rVal + (A_INT32)(2472 - freq)*lVal );
		retVal = (A_INT32) ( retVal / (2472 - 2412) );

		if (retVal > 360) {
			retVal -= 360;
		}
		if (retVal < 0) {
			retVal += 360;
		}

	}

	if (pLibDev->mode == MODE_11A) {
		if (freq <= 5000) {
			idxL = 0;
			idxR = 1;
		} else if (freq >= 5800) {
			idxL = 3;
			idxR = 4;
		} else {
			idxL = (A_UINT32) ((freq - 5000)/200) ;
			if (freq == (5000 + 200*idxL)) {
				idxR = idxL;
			} else {
				idxR = idxL + 1;
			}
		}

		if (idxL == idxR) {
			retVal = pLibDev->p16kEepHeader->info11a.phaseCal[idxL];
		} else {
			rVal = pLibDev->p16kEepHeader->info11a.phaseCal[idxR];
			lVal = pLibDev->p16kEepHeader->info11a.phaseCal[idxL];

			if (rVal > lVal) {
				if ((rVal - lVal) > 180) {
					lVal += 360;  // must've wrapped around 0
				}
			} else {
				if ((lVal-rVal) > 180) {
					rVal += 360;
				}
			}

			retVal = ( (A_INT32)(freq - 5000 - 200*idxL)*rVal +
					   (A_INT32)(5000 + 200*idxR - freq)*lVal );
			retVal = (A_INT32) ( retVal / 200 );
			if (retVal > 360) {
				retVal -= 360;
			}
			if (retVal < 0) {
				retVal += 360;
			}

		}
	}

	return((A_UINT32) retVal);
}


MANLIB_API A_INT16 getMaxPowerForRate(
 A_UINT32 devNum,
 A_UINT32 freq,
 A_UINT32 rate)
{
    LIB_DEV_INFO	*pLibDev = gLibInfo.pLibDevArray[devNum];
	A_INT16		ratesArray[NUM_RATES];
	A_UINT16		dataRate;
	A_INT16     xrTargetPower = 0;

	//only override the power if the eeprom has been read
	if((!pLibDev->eePromLoad) || (!pLibDev->eepData.infoValid)) {
		return(0xfff);
	}

	if(isOwl(pLibDev->swDevID)) {
		return(get5416MaxPowerForRate(devNum, freq, rate));
	}

	//get and set the max power values
	getMaxRDPowerlistForFreq(devNum, (A_UINT16)freq, ratesArray);

    switch (rate) {
    case 6:
       dataRate = 0;
        break;
    case 9:
        dataRate = 1;
        break;
    case 12:
        dataRate = 2;
        break;
    case 18:
        dataRate = 3;
        break;
    case 24:
        dataRate = 4;
        break;
    case 36:
        dataRate = 5;
        break;
    case 48:
        dataRate = 6;
        break;
    case 54:
        dataRate = 7;
        break;
	case 0xea:
	case 0xeb:
	case 0xe1:
	case 0xe2:
	case 0xe3:
		switch(pLibDev->mode) {
		case MODE_11A:
			xrTargetPower = pLibDev->p16kEepHeader->info11a.xrTargetPower;
			break;

		case MODE_11G:
		case MODE_11O:
			xrTargetPower = pLibDev->p16kEepHeader->info11g.xrTargetPower;
			break;

		case MODE_11B:
			xrTargetPower = 0;
			break;
		}
		return(xrTargetPower);
		break;

	case 0xb1:
		dataRate = 8;
		break;
	case 0xb2:
		dataRate = 9;
		break;
	case 0xd2:
		dataRate = 10;
		break;
	case 0xb5:
		dataRate = 11;
		break;
	case 0xd5:
		dataRate = 12;
		break;
	case 0xbb:
		dataRate = 13;
		break;
	case 0xdb:
		dataRate = 14;
		break;
    default:
        mError(devNum, EINVAL, "Device Number %d:DATA rate does not match a known rate: %d\n", devNum,
            rate);
        return (0xfff);
        break;
    }

	return(ratesArray[dataRate]);
}

void applyFalseDetectBackoff
(
 A_UINT32 devNum,
 A_UINT32 freq,
 A_UINT32 backoffAmount
)
{
	A_UINT32 remainder;
    LIB_DEV_INFO	*pLibDev = gLibInfo.pLibDevArray[devNum];
	A_UINT32 fieldValue;

	remainder = freq % 32;
	fieldValue = getFieldForMode(devNum, "bb_cycpwr_thr1", pLibDev->mode, pLibDev->turbo);

	if ((remainder == 0) || ((remainder >= 10) && (remainder <= 22))) {
		//no need to apply backoff
//		printf("SNOOP: No Backoff\n");
		changeRegValueField(devNum, "bb_cycpwr_thr1", fieldValue);
	}
	else {
		//apply backoff
//		printf("SNOOP: Applying backoff of %d, total field value = %d\n", backoffAmount, fieldValue + backoffAmount);
		changeRegValueField(devNum, "bb_cycpwr_thr1", fieldValue + backoffAmount);
	}
}

MANLIB_API void supplyFalseDetectbackoff
(
	A_UINT32 devNum,
	A_UINT32 *pBackoffValues
)
{
	A_UINT16 i;
    LIB_DEV_INFO	*pLibDev = gLibInfo.pLibDevArray[devNum];

	for(i = 0; i < MAX_MODE; i++) {
		pLibDev->suppliedFalseDetBackoff[i] = pBackoffValues[i];
	}

	return;
}

/**************************************************************************
* forcePowerTx_Venice_Hack - hack hard coded to fix the gain_delta
* computation going from ofdm-to-cck rates. representative values
* obtained from an mb22 in pb22 platform.
*
*
* RETURNS:
*/
MANLIB_API void forcePowerTx_Venice_Hack
(
 A_UINT32		devNum,
 A_BOOL         maxRange     // maxRange or conservative
)
{
//    LIB_DEV_INFO *pLibDev = gLibInfo.pLibDevArray[devNum];
	A_UINT16	mask = 0x3f;
	A_UINT16	regOffset = 0x9934;
	A_UINT32	reg32;
	A_BOOL		paPreDEnable = 0;
	A_UINT32	cckPower;

	A_UINT16    ii;
	A_UINT16    pcdacs[64];

	A_UINT16    gainI_conservative[] = {29, 29, 29, 29, 29, 29, 29, 29, 21, 21, 21, 21, 21, 21, 21};
	A_UINT16    pcdac_conservative[] = {26, 26, 26, 26, 26, 26, 26, 26, 30, 30, 30, 30, 30, 30, 30};

	A_UINT16    gainI_maxRange[] = {42, 42, 42, 42, 42, 35, 29,  9, 21, 21, 21, 21, 21, 21, 21};
	A_UINT16    pcdac_maxRange[] = {50, 50, 50, 50, 50, 38, 26,  1, 30, 30, 30, 30, 30, 30, 30};

	A_UINT16	*pRatesPower = maxRange ? &(gainI_maxRange[0]) : &(gainI_conservative[0]);
	A_UINT16	*pRatesPcdac = maxRange ? &(pcdac_maxRange[0]) : &(pcdac_conservative[0]);


	reg32 = ( ((paPreDEnable & 1)<< 30) | ((pRatesPower[3] & mask) << 24) |
	     ((paPreDEnable & 1)<< 22) | ((pRatesPower[2] & mask) << 16) |
	     ((paPreDEnable & 1)<< 14) | ((pRatesPower[1] & mask) << 8) |
	     ((paPreDEnable & 1)<< 6 ) |  (pRatesPower[0] & mask)  );
	REGW(devNum, regOffset, reg32);
	regOffset = 0x9938;
	reg32 = ( ((paPreDEnable & 1)<< 30) | ((pRatesPower[7] & mask) << 24) |
	     ((paPreDEnable & 1)<< 22) | ((pRatesPower[6] & mask) << 16) |
	     ((paPreDEnable & 1)<< 14) | ((pRatesPower[5] & mask) << 8) |
	     ((paPreDEnable & 1)<< 6 ) |  (pRatesPower[4] & mask)  );
	REGW(devNum, regOffset, reg32);
	regOffset = 0x993c;

	//set max power to the power value at rate 6
	REGW(devNum, regOffset, pRatesPower[0]);

	cckPower = pRatesPower[8]; // fixes gainDelta for cck

	regOffset = 0xa234;
	reg32 = (((cckPower  & mask) << 24) |
	         ((cckPower & mask) << 16) |
			 ((cckPower & mask) << 8)  |
			  (cckPower & mask) );
	REGW(devNum, regOffset, reg32);

	regOffset = 0xa238;
	reg32 = (((cckPower & mask) << 24) |
	         ((cckPower & mask) << 16) |
			 ((cckPower & mask) << 8)  |
			  (cckPower & mask) );
	REGW(devNum, regOffset, reg32);

	for (ii=0; ii<64; ii++) {
		pcdacs[ii] = 0;
	}
	for (ii=0; ii<15; ii++) {
		pcdacs[pRatesPower[ii]] = pRatesPcdac[ii];
	}

	forcePCDACTable(devNum, &(pcdacs[0]));

}


/***************************************************************************
*  forceChainPowerTxMaxVenice  -  corrects for the gain-delta between ofdm and cck
*                            mode target powers. writes to the rate table and
*                            the power table.
*   CAUTION :
*            1. Do NOT write to the 64-entry power table after a call to this
*               routine as that will corrupt the fixes for gain-delta.
*
*   Conventions :
*            1. pRatesPower[ii] is the integer value of 2*(desired power
*				for the rate ii in dBm) to provide 0.5dB resolution. rate
*				mapping is as following :
*					[0..7]  --> ofdm 6, 9, .. 48, 54
*	                [8..14] --> cck 1L, 2L, 2S, .. 11L, 11S
*					[15]    --> XR (all rates get the same power)
*            2. pPowerValues[ii]  is the pcdac corresponding to ii/2 dBm.
****************************************************************************/
void forceChainPowerTxMaxVenice
(
 A_UINT32	devNum,
 A_INT16	*pRatesPower,
 A_UINT16	*pPowerValues,
 double     ofdm_cck_delta,
 A_UINT32   chainNum
)
{

    LIB_DEV_INFO	*pLibDev = gLibInfo.pLibDevArray[devNum];
	A_UINT16	mask = 0x3f;
	A_UINT16	regOffset;
	A_UINT32	reg32;
	A_BOOL		paPreDEnable = 0;
	A_INT16     pRatesIndex[NUM_RATES];
	A_UINT16    ii, jj, iter;
	A_UINT32	cck_index;
	A_INT16		gain_delta_adjust = pLibDev->p16kEepHeader->ofdmCckGainDeltaX2;
	A_UINT32    regDelta = 0x0000;

	if (isFalcon(devNum)) {
		regDelta = 0x1000 * chainNum;
	}

	// make a local copy of desired powers as initial indices
	memcpy(pRatesIndex, pRatesPower, NUM_RATES*sizeof(A_UINT16));

	// fix the CCK indices
	for (ii = 8; ii < 15; ii++) {
		// apply a gain_delta correction of -15 for CCK
		pRatesIndex[ii] = (A_INT16)(pRatesIndex[ii] - gain_delta_adjust);

		// Now check for contention with all ofdm target powers
		jj = 0;
		iter = 0;
		while (jj < 16) { // indicates not all ofdm rates checked for contention yet
			if (pRatesIndex[ii] < 0) {
				pRatesIndex[ii] = 0;
			}
			if (jj == 8) {
				jj = 15; // skip CCK rates
				continue;
			}
			if (pRatesIndex[ii] == pRatesPower[jj]) {
				if (pRatesPower[jj] == 0) {
					pRatesIndex[ii]++;
				} else {
					if (iter > 50) {  // to avoid pathological case of ofdm target powers 0 and 0.5dBm
						pRatesIndex[ii]++;
					} else {
						pRatesIndex[ii]--;
					}
				}
				jj = 0;  // check with all rates again
				iter++;
			} else {
				jj++;
			}
		}

		if (pRatesIndex[ii] >= PWR_TABLE_SIZE) pRatesIndex[ii] = PWR_TABLE_SIZE -1;
		cck_index = pRatesPower[ii] - (A_UINT32)(2*ofdm_cck_delta);
		if (cck_index < 0) cck_index = 0;
		pPowerValues[pRatesIndex[ii]] = pPowerValues[cck_index];
	}

	// write powerValues to 64-entry power table.
	if (isFalcon(devNum)) {
		forceChainPCDACTable(devNum, pPowerValues, chainNum);
	} else {
	        forcePCDACTable(devNum, pPowerValues);
	}

	// write the rate-to-power indices with corrected gain_delta between ofdm and cck
	regOffset = 0x9934;
	reg32 = ( ((paPreDEnable & 1)<< 30) | ((pRatesPower[3] & mask) << 24) |
	     ((paPreDEnable & 1)<< 22) | ((pRatesPower[2] & mask) << 16) |
	     ((paPreDEnable & 1)<< 14) | ((pRatesPower[1] & mask) << 8) |
	     ((paPreDEnable & 1)<< 6 ) |  (pRatesPower[0] & mask)  );
	REGW(devNum, regOffset + regDelta, reg32);
	regOffset = 0x9938;
	reg32 = ( ((paPreDEnable & 1)<< 30) | ((pRatesPower[7] & mask) << 24) |
	     ((paPreDEnable & 1)<< 22) | ((pRatesPower[6] & mask) << 16) |
	     ((paPreDEnable & 1)<< 14) | ((pRatesPower[5] & mask) << 8) |
	     ((paPreDEnable & 1)<< 6 ) |  (pRatesPower[4] & mask)  );
	REGW(devNum, regOffset + regDelta, reg32);

	regOffset = 0x993c;

	//set max power to the max possible
	REGW(devNum, regOffset + regDelta, 0x3F);

	regOffset = 0xa234;
	reg32 = (((pRatesIndex[10] & mask) << 24) |
	         ((pRatesIndex[9]  & mask) << 16) |
			 ((pRatesPower[15] & mask) <<  8)  |   // XR target power
			  (pRatesIndex[8]  & mask) );
	REGW(devNum, regOffset + regDelta, reg32);

	regOffset = 0xa238;
	reg32 = (((pRatesIndex[14] & mask) << 24) |
	         ((pRatesIndex[13] & mask) << 16) |
			 ((pRatesIndex[12] & mask) <<  8)  |
			  (pRatesIndex[11] & mask) );
	REGW(devNum, regOffset + regDelta, reg32);
}


void getPwrTableByModeCh (A_UINT32 devNum, A_UINT32 mode, A_UINT32 freq, A_UINT16 *powerValues)
{
    LIB_DEV_INFO		*pLibDev = gLibInfo.pLibDevArray[devNum];
	MDK_FULL_PCDAC_STRUCT	pcdacStruct;
	MDK_PCDACS_EEPROM		eepromPcdacs;

	//fill out the full pcdac info for this channel
	pcdacStruct.channelValue = (A_UINT16)freq;

	//setup the pcdac struct to point to the correct info, based on mode
	switch(mode) {
	case MODE_11A:
		eepromPcdacs.numChannels = pLibDev->pCalibrationInfo->numChannels_11a;
		eepromPcdacs.pChannelList = pLibDev->pCalibrationInfo->Channels_11a;
		eepromPcdacs.pDataPerChannel = pLibDev->pCalibrationInfo->DataPerChannel_11a;
		break;

	case MODE_11G:
	case MODE_11O:
		eepromPcdacs.numChannels = pLibDev->pCalibrationInfo->numChannels_2_4;
		eepromPcdacs.pChannelList = pLibDev->pCalibrationInfo->Channels_11g;
		eepromPcdacs.pDataPerChannel = pLibDev->pCalibrationInfo->DataPerChannel_11g;
		break;

	case MODE_11B:
		eepromPcdacs.numChannels = pLibDev->pCalibrationInfo->numChannels_2_4;
		eepromPcdacs.pChannelList = pLibDev->pCalibrationInfo->Channels_11b;
		eepromPcdacs.pDataPerChannel = pLibDev->pCalibrationInfo->DataPerChannel_11b;
		break;

	}
	mapPcdacTable(&eepromPcdacs, &pcdacStruct);

	//get the power table from the pcdac table
	getPwrTable(&pcdacStruct, powerValues);
}

A_BOOL readEEPData_16K
(
 A_UINT32	devNum
)
{
	if (!allocateEepStructs(devNum)) {
		mError(devNum, ENOMEM, "Device Number %d:Unable to allocate eeprom info structures\n", devNum);
		return FALSE;
	}
	//try to read the values from the EEPROM
	if(!readEepromIntoDataset(devNum)) {
		return FALSE;
	}
	return TRUE;
}

/**************************************************************************
* getMaxLinPowerx4 - Fetch the max linear power at curr channel
*
*
* RETURNS:   4x of max lin power
*/
MANLIB_API A_INT32 getMaxLinPowerx4
(
 A_UINT32		devNum
)
{
    LIB_DEV_INFO	*pLibDev = gLibInfo.pLibDevArray[devNum];

	return(pLibDev->maxLinPwrx4);
}

/***************************************************************************
*  getCtlPower  -  Given a CTL test group code, the channel and mode,
*                  find the power
*/
A_INT32
getCtlPower
(
 A_UINT32 devNum,
 A_UINT32 ctl,
 A_UINT32 freq,
 A_UINT32 mode,
 A_UINT32 turbo
)
{
    LIB_DEV_INFO			  *pLibDev = gLibInfo.pLibDevArray[devNum];
	A_INT32					  *pPowerValue;
	CTL_PWR_RANGE *ctlPowerChannelRange = NULL;
	A_UINT32			i = 0;
	A_UINT32			ctlEdgeCounter, ctlRangeCounter;
	A_BOOL				ctlMatchFound, gotLowChannel;
	MDK_RD_EDGES_POWER	*pRdEdgePwrInfo = pLibDev->p16KRdEdgesPower;
	A_UINT32			channelRangeDeltaLow = 5;  //initialize to 2GHz range, change below for 11a
	A_UINT32			channelRangeDeltaHigh = 5;  //initialize to 2GHz range, change below for 11a
	A_UINT32			previousChannel;

	//point to the correct structure to fill based on mode
	if(mode == MODE_11B) {
		ctlPowerChannelRange = savedCtlPowerInfo.ctlPowerChannelRangeCck;
		pPowerValue = &savedCtlPowerInfo.powerCck;

	}
	else {
		ctlPowerChannelRange = savedCtlPowerInfo.ctlPowerChannelRangeOfdm;
		pPowerValue = &savedCtlPowerInfo.powerOfdm;
	}
	savedCtlPowerInfo.channelApplied = freq;

	memset(ctlPowerChannelRange, 0, sizeof(CTL_PWR_RANGE) * NUM_16K_EDGES);

	//make the ctl have the correct mask based on mode and turbo flag
	ctl = ctl & ~0x7;     //clear any mode mask that may have come with the ctl

	switch (mode)
	{
	case MODE_11A:
		channelRangeDeltaLow = 20;
		channelRangeDeltaHigh = 20;
		if(turbo == TURBO_ENABLE) {
			ctl = ctl | CTL_MODE_11A_TURBO;
		}
		else {
			ctl = ctl | CTL_MODE_11A;
		}
		break;
	case MODE_11B:
		ctl = ctl | CTL_MODE_11B;
		break;

	case MODE_11G:
		if(turbo == TURBO_ENABLE) {
			ctl = ctl | CTL_MODE_11G_TURBO;
		}
		else {
			ctl = ctl | CTL_MODE_11G;
		}
		break;
	}

	//search through the ctls read from eeprom to see if there is a match
	ctlMatchFound = FALSE;
	while ((pLibDev->p16kEepHeader->testGroups[i] != 0) && (i < pLibDev->p16kEepHeader->numCtl)) {
		if(ctl == pLibDev->p16kEepHeader->testGroups[i]) {
			//found the ctl match
			ctlMatchFound = TRUE;
			break;
		}
		i++;
		pRdEdgePwrInfo+=NUM_16K_EDGES;
	}
	//note: on exit from loop. pRdEdgePwrInfo should point to the correct
	//eeprom info for the ctl

	//if no ctl is found in the eeprom, then return value that indicates
	//the power is not limited by any ctl in eepron
	if (!ctlMatchFound) {
		*pPowerValue = NO_CTL_IN_EEPROM;
		return(*pPowerValue);
	}

	//use the info in the ctl and construct an easy to parse list of channel
	//ranges and power limits
	gotLowChannel = FALSE;
	previousChannel = 0;
	for(ctlEdgeCounter = 0, ctlRangeCounter = 0; ctlEdgeCounter < NUM_16K_EDGES; ctlEdgeCounter++) {
		if(pRdEdgePwrInfo[ctlEdgeCounter].rdEdge == 0) {
			//skip this channel
			continue;
		}

		//want to include channels +-10MHz for 11a and +- 2.5MHz for 11g
		//however if the previous or next channel in the list, is less than
		//this, we need to alter the range
		if((pRdEdgePwrInfo[ctlEdgeCounter].rdEdge - previousChannel) < channelRangeDeltaLow) {
			channelRangeDeltaLow = pRdEdgePwrInfo[ctlEdgeCounter].rdEdge - previousChannel;
		}

		if((A_UINT32)(pRdEdgePwrInfo[ctlEdgeCounter+1].rdEdge - pRdEdgePwrInfo[ctlEdgeCounter].rdEdge) < channelRangeDeltaHigh) {
			channelRangeDeltaHigh = pRdEdgePwrInfo[ctlEdgeCounter+1].rdEdge - pRdEdgePwrInfo[ctlEdgeCounter].rdEdge;
		}

		if(pRdEdgePwrInfo[ctlEdgeCounter].flag == 0) {//ie its an edge
			if(!gotLowChannel) {  //the edge defines the range
				ctlPowerChannelRange[ctlRangeCounter].lowChannel = pRdEdgePwrInfo[ctlEdgeCounter].rdEdge - channelRangeDeltaLow/2;
				ctlPowerChannelRange[ctlRangeCounter].highChannel = pRdEdgePwrInfo[ctlEdgeCounter].rdEdge + channelRangeDeltaHigh/2;
				ctlPowerChannelRange[ctlRangeCounter].twicePower = pRdEdgePwrInfo[ctlEdgeCounter].twice_rdEdgePower;
				if(channelRangeDeltaHigh % 2) {
					//odd, channel delta add 1 to the upper range so cover full range
					ctlPowerChannelRange[ctlRangeCounter].highChannel += 1;
				}
			}
			else {
				//edge marks end of channel range
				ctlPowerChannelRange[ctlRangeCounter].highChannel = pRdEdgePwrInfo[ctlEdgeCounter].rdEdge + channelRangeDeltaHigh/2;
				//If an edge does mark the end of a channel range, then display information message if power is different
				if(ctlPowerChannelRange[ctlRangeCounter].twicePower != pRdEdgePwrInfo[ctlEdgeCounter].twice_rdEdgePower) {
					printf("INFORMATION: Band Edge %d, for ctl %d, marks end of inband range as well as edge, power is not same as inband power\n",
						pRdEdgePwrInfo[ctlEdgeCounter].rdEdge, ctl);
				}
				gotLowChannel = FALSE;
			}
			ctlRangeCounter ++;
		}
		else {
			//this is in band, either start or end channel
			if(!gotLowChannel) {  //this is the low channel
				ctlPowerChannelRange[ctlRangeCounter].lowChannel = pRdEdgePwrInfo[ctlEdgeCounter].rdEdge - channelRangeDeltaLow/2;
				ctlPowerChannelRange[ctlRangeCounter].twicePower = pRdEdgePwrInfo[ctlEdgeCounter].twice_rdEdgePower;
				gotLowChannel = TRUE;
			}
			else { //this is the high channel
				ctlPowerChannelRange[ctlRangeCounter].highChannel = pRdEdgePwrInfo[ctlEdgeCounter].rdEdge + channelRangeDeltaHigh/2;
				if(channelRangeDeltaHigh % 2) {
					//odd, channel delta add 1 to the upper range so cover full range
					ctlPowerChannelRange[ctlRangeCounter].highChannel += 1;
				}
				//check that the power for this inband end is the same as the power for the beginning
				if(ctlPowerChannelRange[ctlRangeCounter].twicePower != pRdEdgePwrInfo[ctlEdgeCounter].twice_rdEdgePower) {
					printf("INFORMATION: Power for band start channel %d does not match power for band end channel %d for ctl %d\n",
						pRdEdgePwrInfo[ctlEdgeCounter-1].rdEdge, pRdEdgePwrInfo[ctlEdgeCounter].rdEdge, ctl);
				}
				gotLowChannel = FALSE;
				ctlRangeCounter++;
			}
		}
		previousChannel = pRdEdgePwrInfo[ctlEdgeCounter].rdEdge;
	}

	//go through the channel range and look for a match against incoming channel
	for(i = 0; ctlPowerChannelRange[i].lowChannel; i++) {
		if((freq >= ctlPowerChannelRange[i].lowChannel) &&
			(freq < ctlPowerChannelRange[i].highChannel)) {
			//channel is within the ctl range, return the power
			*pPowerValue = ctlPowerChannelRange[i].twicePower;
			return(*pPowerValue);
		}
	}

	//channel is not within the ctl range
	*pPowerValue = NOT_CTL_LIMITED;
	return(*pPowerValue);
}

MANLIB_API void
getCtlPowerInfo
(
 A_UINT32 devNum,
 CTL_POWER_INFO *pCtlStruct
)
{
    LIB_DEV_INFO			  *pLibDev = gLibInfo.pLibDevArray[devNum];

	//check for the eeprom being loaded as an indication of the
	//structure being filled
	if((!pLibDev->eePromLoad) || (!pLibDev->eepData.infoValid)) {
		pCtlStruct->structureFilled = 0;
		return;
	}

	memcpy(pCtlStruct, &savedCtlPowerInfo, sizeof(CTL_POWER_INFO));
	pCtlStruct->structureFilled = 1;
}

// same routine can be called at cal time, or to reprogram with the
// dynamic ear generated from a previously cal'd card without re-cal'ing
MANLIB_API A_UINT16
getEARCalAtChannel
(
 A_UINT32 devNum,
 A_BOOL   atCal,  // 1 ==> no EEPROM, 0 ==> valid cal exists and has been read
 A_UINT16 channel,
 A_UINT32 *word,   // used as input for the case of atCal = 1
 A_UINT16 xpd_mask,
 A_UINT32 version_mask
)
{
    LIB_DEV_INFO	*pLibDev = gLibInfo.pLibDevArray[devNum];
	A_UINT16		regOffset = 0x9934;
//	A_UINT32		reg32;
//	A_BOOL			paPreDEnable = 0;
//	A_UINT16		powerValues[PWR_TABLE_SIZE];
	A_UINT16		pdadcValues[PDADC_TABLE_SIZE];
	A_UINT16        gainBoundaries[MAX_NUM_PDGAINS_PER_CHANNEL];
//	MODE_HEADER_INFO *pModeInfo = NULL;
//	A_UINT32        mode = MODE_11G;
	A_UINT16        i, ii;
	A_UINT16        startOffset = 0;
//	A_UINT16        pwr_index_offset;
//	double          cck_ofdm_correction;
	A_UINT16        numPdGainsUsed;
	A_UINT32        pdGainOverlap_t2;
	EEPROM_DATA_STRUCT_GEN5  *pTempGen5CalData;
	RAW_DATA_STRUCT_GEN5     *pTempGen5RawData;

//	A_UINT16        version_mask = 0x7FFF ;
	A_UINT16        cm = 0;      // channel modifier - none
	A_UINT16        d = 0;       // disabler
	A_UINT16        stage = 0x2; // after cal
	A_UINT16        type = 0x1;  // reg type - 1 : 16-bit addr followed by multiple 32-bit data
	A_UINT16        modality_mask = 0x7; // for griffin - 11b, 11g, 11g-turbo
	A_UINT16        num_writes;  // # of consecutive writes of type 1

	A_UINT16        last;
	A_UINT16        opcode;
	A_UINT16        start_bit;
	A_UINT16        num_bits;

	A_UINT32        reg_header;
	A_UINT32        temp32, regValue;
	A_UINT32        *retEAR;
	A_UINT32        en_pd_cal;
	A_UINT32        pd_cal_wait;
	A_UINT32        force_pd_gain;

	if (isDragon(devNum)) {
	    return 0;
	}

	if (atCal) {
		if(!initialize_datasets_forced_eeprom_gen5(devNum, &pTempGen5CalData, &pTempGen5RawData, word, xpd_mask)) {
			return (0);
		}
	} else {
		pTempGen5RawData = pLibDev->pGen5RawData[MODE_11G];
	}

	numPdGainsUsed = pTempGen5RawData->pDataPerChannel[0].numPdGains;

	//FJC, replace this with a register read, field not present in hainan_derby config file
	//so can't do a get field for mode incase we are loading ear and using this config file
//	pdGainOverlap_t2 = (A_UINT16)getFieldForMode(devNum, "bb_pd_gain_overlap", mode, pLibDev->turbo);
	pdGainOverlap_t2 = REGR(devNum, 0xa26c) & 0xf;

	//pass this to the function to get the pdadc table, gain boundaries and pdgain list
	get_gain_boundaries_and_pdadcs_for_powers(devNum, channel, pTempGen5RawData,
						(A_UINT16)pdGainOverlap_t2, &(minCalPowerGen5_t2), gainBoundaries, pLibDev->eepData.xpdGainValues, pdadcValues);

	startOffset = 0;
	retEAR = word; // point the return array to input array "word"
	retEAR[startOffset++] = ( (1 << 15) | version_mask);

	reg_header = (0x7FFF & ( ((d & 0x1) << 14) | ((cm & 0x1) << 13) |
		                    ((stage & 0x3) << 11) | ((type & 0x3) << 9) |
							(modality_mask & 0xFF) ) );


	//write pdadc table to EAR
	regOffset = 0x9800 + (672 << 2) ;
	ii = 0;
	num_writes = 3; // 4 consecutive writes
	for(i = 0; i < 32; i++) {
		if (ii >= 4) {
			ii = 0;
		}

		if (ii == 0) {
			retEAR[startOffset++] = reg_header;
			retEAR[startOffset++] = regOffset | num_writes;
		}

		temp32 = ((pdadcValues[4*i + 0] & 0xFF) << 0)  |
			     ((pdadcValues[4*i + 1] & 0xFF) << 8)  |
				 ((pdadcValues[4*i + 2] & 0xFF) << 16) |
				 ((pdadcValues[4*i + 3] & 0xFF) << 24) ;
		retEAR[startOffset++] = (temp32 >> 16) & 0xFFFF;
		retEAR[startOffset++] = temp32 & 0xFFFF;

		regOffset += 4;
		ii++;
	}
	regOffset = 0xa26c;
	num_writes = 0; // single reg write
	retEAR[startOffset++] = reg_header;
	retEAR[startOffset++] = regOffset | num_writes;
	temp32 = ( (gainBoundaries[3] & 0x3F) << 22 ) |
		     ( (gainBoundaries[2] & 0x3F) << 16 ) |
			 ( (gainBoundaries[1] & 0x3F) << 10 ) |
			 ( (gainBoundaries[0] & 0x3F) <<  4 ) |
			 (pdGainOverlap_t2 & 0xF);
	retEAR[startOffset++] = (temp32 >> 16) & 0xFFFF;
	retEAR[startOffset++] = temp32 & 0xFFFF;

	//write xpdgain
	regOffset = 0xa258;
	type = 3; // read-modify-write on bb_num_pd_gain
	reg_header = (0x7FFF & ( ((d & 0x1) << 14) | ((cm & 0x1) << 13) |
		                    ((stage & 0x3) << 11) | ((type & 0x3) << 9) |
							(modality_mask & 0xFF) ) );
	retEAR[startOffset++] = reg_header;
	last = 1;
	opcode = 0; // replace
	start_bit = 14;
	num_bits  = 16; // num_pd_gain + pd_gain_setting_1, _2 and _3,
	                // en_pd_cal, pd_cal_wait, force_pd_gain = 0.

	//FJC, replace this with a register read, field not present in hainan_derby config file
	//so can't do a get field for mode incase we are loading ear and using this config file
	regValue = REGR(devNum, 0xa258);
	//en_pd_cal   = (A_UINT16)getFieldForMode(devNum, "bb_enable_pd_calibrate", mode, pLibDev->turbo);
	en_pd_cal = (regValue >> 22) & 0x1;
	//pd_cal_wait = (A_UINT16)getFieldForMode(devNum, "bb_pd_calibrate_wait", mode, pLibDev->turbo);
	pd_cal_wait = (regValue >> 23) & 0x3f;
	force_pd_gain = 0;

	retEAR[startOffset++] = ((last & 0x1) << 15) | ((opcode & 0x7) << 10) |
		                  ((start_bit & 0x1F) << 5) | (num_bits & 0x1F) ;
	retEAR[startOffset++] = regOffset & 0xFFFF;
	retEAR[startOffset++] = ((force_pd_gain & 0x1) << 15) |
		                    ((pd_cal_wait & 0x3F) << 9) |
							((en_pd_cal & 0x1) << 8) |
							((0 & 0x3) << 6) |
		                    ((pLibDev->eepData.xpdGainValues[1] & 0x3) << 4) |
							((pLibDev->eepData.xpdGainValues[0] & 0x3) << 2) |
							(numPdGainsUsed - 1) & 0x3;

	return(startOffset);

	//Note the pdadc table may not start at 0dBm power, could be negative or greater than 0.
	//need to offest the power values by the amount of minPower for griffin
/*
	if(minCalPowerGen5_t2 != 0) {
		pwr_index_offset = (A_INT16)(0 - minCalPowerGen5_t2);
		pLibDev->pwrIndexOffset = pwr_index_offset;
		for(i = 0; i < NUM_RATES; i++) {
			pRatesPower[i] = (A_INT16)(pRatesPower[i] + pwr_index_offset);
			if (pRatesPower[i] > 63) pRatesPower[i] = 63;
		}
	} else {
		pLibDev->pwrIndexOffset = 0;
	}
*/
}

