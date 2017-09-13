/*
 *  Copyright © 2002 Atheros Communications, Inc.,  All Rights Reserved.
 */

#ident  "ACI $Id: //depot/sw/src/dk/mdk/devlib/ar5211/mEEP211.c#13 $, $Header: //depot/sw/src/dk/mdk/devlib/ar5211/mEEP211.c#13 $"

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

#include "mEEP211.h"
#include "ar5211reg.h"


A_BOOL readCalData_gen2
(
 A_UINT32 devNum,
 MDK_PCDACS_ALL_MODES	*pEepromData
)
{
    LIB_DEV_INFO *pLibDev = gLibInfo.pLibDevArray[devNum];
	MDK_DATA_PER_CHANNEL	*pChannelData = NULL;
	A_UINT32			offset = 0;
	A_UINT16			numChannels = 0;
	A_UINT16			*pChannels = NULL;
	A_UINT16			mode;
	A_UINT32			i, j;
	A_UINT32			tempValue;

	// Group 1: frequency pier locations readback
	readFreqPiers(devNum, pEepromData->Channels_11a, pEepromData->numChannels_11a);
	
	//Group 2:  readback data for all frequency piers
	for(mode = 0; mode <= MAX_MODE; mode++) {
		switch(mode) {
		case MODE_11A:
			offset = pOffsets->GROUP2_11A_RAW_PWR;
			numChannels = pEepromData->numChannels_11a;
			pChannelData = pEepromData->DataPerChannel_11a;
			pChannels = pEepromData->Channels_11a;
			break;

		case MODE_11G:
		case MODE_11O:
			offset = pOffsets->GROUP4_11G_RAW_PWR;
			numChannels = pEepromData->numChannels_2_4;
			pChannelData = pEepromData->DataPerChannel_11g;
			pChannels = pEepromData->Channels_11g;
			break;
		
		case MODE_11B:
			offset = pOffsets->GROUP3_11B_RAW_PWR;
			numChannels = pEepromData->numChannels_2_4;
			pChannelData = pEepromData->DataPerChannel_11b;
			pChannels = pEepromData->Channels_11b;

			break;
		default:
			mError(devNum, EIO, "Bad mode in readEepromIntoDataset, internal software error, should not get here\n");
			return(FALSE);
		} //end switch

		offset += pLibDev->eepromStartLoc;

		for(i = 0; i < numChannels; i++) {
			pChannelData->channelValue = pChannels[i];
			
			tempValue = eepromRead(devNum, offset++);
			pChannelData->pcdacMax = (A_UINT16)(( tempValue >> 10 ) & PCDAC_MASK);
			pChannelData->pcdacMin = (A_UINT16)(( tempValue >> 4  ) & PCDAC_MASK);
			pChannelData->PwrValues[0] = (A_UINT16)(( tempValue << 2  ) & POWER_MASK);
    
			tempValue = eepromRead(devNum, offset++);
			pChannelData->PwrValues[0] = (A_UINT16)(((tempValue >> 14 ) & 0x3 ) | pChannelData->PwrValues[0]);
			pChannelData->PwrValues[1] = (A_UINT16)((tempValue >> 8 ) & POWER_MASK);
			pChannelData->PwrValues[2] = (A_UINT16)((tempValue >> 2 ) & POWER_MASK);
			pChannelData->PwrValues[3] = (A_UINT16)((tempValue << 4 ) & POWER_MASK);
    
			tempValue = eepromRead(devNum, offset++);
			pChannelData->PwrValues[3] = (A_UINT16)(( ( tempValue >> 12 ) & 0xf ) | pChannelData->PwrValues[3]);
			pChannelData->PwrValues[4] = (A_UINT16)(( tempValue >> 6 ) & POWER_MASK);
			pChannelData->PwrValues[5] =  (A_UINT16)(tempValue  & POWER_MASK);
    
			tempValue = eepromRead(devNum, offset++);
			pChannelData->PwrValues[6]	= (A_UINT16)(( tempValue >> 10 ) & POWER_MASK);
			pChannelData->PwrValues[7]	= (A_UINT16)(( tempValue >> 4  ) & POWER_MASK);
			pChannelData->PwrValues[8] = (A_UINT16)(( tempValue << 2  ) & POWER_MASK);
    
			tempValue = eepromRead(devNum, offset++);
			pChannelData->PwrValues[8] = (A_UINT16)(( ( tempValue >> 14 ) & 0x3 ) | pChannelData->PwrValues[8]);
			pChannelData->PwrValues[9] = (A_UINT16)(( tempValue >> 8 ) & POWER_MASK);
			pChannelData->PwrValues[10] = (A_UINT16)(( tempValue >> 2 ) & POWER_MASK);
    
			getPcdacInterceptsFromPcdacMinMax(pChannelData->pcdacMin, 
						pChannelData->pcdacMax, pChannelData->PcdacValues) ;
			
			for(j = 0; j < pChannelData->numPcdacValues; j++) {
				pChannelData->PwrValues[j] = (A_UINT16)(PWR_STEP*pChannelData->PwrValues[j]); //Note these values are scaled up.
			}
			pChannelData++;
		}
	}
	return(TRUE);
}



