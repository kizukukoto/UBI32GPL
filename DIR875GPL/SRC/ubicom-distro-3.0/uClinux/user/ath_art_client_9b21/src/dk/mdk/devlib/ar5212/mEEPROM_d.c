/* mEEPROM_d.c - contians functions for reading eeprom and getting            */
/*                 pcdac power settings for all channels for gen3 analog chips  */
/* Copyright (c) 2001 Atheros Communications, Inc., All Rights Reserved         */


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
#include "../athreg.h"
#include "../manlib.h"
#include "../mEeprom.h"
#include "../mConfig.h"
#include "../mDevtbl.h"
#include <errno.h>

#include "mEEPROM_d.h"

#define  FALCON_BAND_EIRP_HEADROOM_IN_DB   6

MANLIB_API A_BOOL setup_raw_dataset_gen3(A_UINT32 devNum, RAW_DATA_STRUCT_GEN3 *pRawDataset_gen3, A_UINT16 myNumRawChannels, A_UINT16 *pMyRawChanList)
{
	A_UINT16	i, j, channelValue;	

	if(!allocateRawDataStruct_gen3(devNum, pRawDataset_gen3, myNumRawChannels)) {
		mError(devNum, EIO,"unable to allocate raw dataset (gen3)\n");
		return(0);
	}


	for (i = 0; i < myNumRawChannels; i++) {
		channelValue = pMyRawChanList[i];

		pRawDataset_gen3->pChannels[i] = channelValue;
		
		pRawDataset_gen3->pDataPerChannel[i].channelValue = channelValue;

		for (j = 0; j < NUM_XPD_PER_CHANNEL; j++) {
			pRawDataset_gen3->pDataPerChannel[i].pDataPerXPD[j].xpd_gain = j;
			pRawDataset_gen3->pDataPerChannel[i].pDataPerXPD[j].numPcdacs = 0;
		}		
		pRawDataset_gen3->pDataPerChannel[i].pDataPerXPD[0].numPcdacs = 4;
		pRawDataset_gen3->pDataPerChannel[i].pDataPerXPD[3].numPcdacs = 3;
	}

	return(1);
}

A_BOOL	allocateRawDataStruct_gen3(A_UINT32 devNum, RAW_DATA_STRUCT_GEN3  *pRawDataset_gen3, A_UINT16 numChannels)
{
	A_UINT16	i, j, k;
	A_UINT16	numPcdacs = NUM_POINTS_XPD0;

	///allocate room for the channels
	pRawDataset_gen3->pChannels = (A_UINT16 *)malloc(sizeof(A_UINT16) * numChannels);
	if (NULL == pRawDataset_gen3->pChannels) {
		mError(devNum, EIO,"unable to allocate raw data struct (gen3)\n");
		return(0);
	}

	pRawDataset_gen3->pDataPerChannel = (RAW_DATA_PER_CHANNEL_GEN3 *)malloc(sizeof(RAW_DATA_PER_CHANNEL_GEN3) * numChannels);
	if (NULL == pRawDataset_gen3->pDataPerChannel) {
		mError(devNum, EIO,"unable to allocate raw data struct data per channel(gen3)\n");
		free(pRawDataset_gen3->pChannels);
		return(0);
	}
	
	pRawDataset_gen3->numChannels = numChannels;

	for(i = 0; i < numChannels; i ++) {
		pRawDataset_gen3->pDataPerChannel[i].pDataPerXPD = NULL;
		
		pRawDataset_gen3->pDataPerChannel[i].pDataPerXPD = (RAW_DATA_PER_XPD_GEN3 *)malloc(sizeof(RAW_DATA_PER_XPD_GEN3) * NUM_XPD_PER_CHANNEL);
		if (NULL == pRawDataset_gen3->pDataPerChannel[i].pDataPerXPD) {
			mError(devNum, EIO,"unable to allocate raw data struct pDataPerXPD (gen3)\n");
			break; //will cleanup outside loop
		}
		for (j=0; j<NUM_XPD_PER_CHANNEL ; j++) {
			pRawDataset_gen3->pDataPerChannel[i].pDataPerXPD[j].pcdac = (A_UINT16 *)malloc(numPcdacs*sizeof(A_UINT16));
			if(pRawDataset_gen3->pDataPerChannel[i].pDataPerXPD[j].pcdac == NULL) {
				mError(devNum, EIO,"unable to allocate pcdacs for an xpd_gain (gen3)\n");
				break;
			}
			pRawDataset_gen3->pDataPerChannel[i].pDataPerXPD[j].pwr_t4 = (A_INT16 *)malloc(numPcdacs*sizeof(A_INT16));
			if(pRawDataset_gen3->pDataPerChannel[i].pDataPerXPD[j].pwr_t4 == NULL) {
				mError(devNum, EIO,"unable to allocate pwr_t4 for an xpd_gain (gen3)\n");
				if (pRawDataset_gen3->pDataPerChannel[i].pDataPerXPD[j].pcdac != NULL) {
					free(pRawDataset_gen3->pDataPerChannel[i].pDataPerXPD[j].pcdac);
				}
				break; // cleanup outside the j loop
			}
		}
		if (j != NUM_XPD_PER_CHANNEL) { // malloc must've failed
			for (k=0; k<j; k++) {
				if (pRawDataset_gen3->pDataPerChannel[i].pDataPerXPD[k].pcdac != NULL) {
					free(pRawDataset_gen3->pDataPerChannel[i].pDataPerXPD[k].pcdac);
				}
				if (pRawDataset_gen3->pDataPerChannel[i].pDataPerXPD[k].pwr_t4 != NULL) {
					free(pRawDataset_gen3->pDataPerChannel[i].pDataPerXPD[k].pwr_t4);
				}
			}
			break; // cleanupo outside i loop
		}
	}

	if (i != numChannels) {
		//malloc must have failed, cleanup any allocs
		for (j = 0; j < i; j++) {
			for (k=0; k<NUM_XPD_PER_CHANNEL; k++) {
				if (pRawDataset_gen3->pDataPerChannel[j].pDataPerXPD[k].pcdac != NULL) {
					free(pRawDataset_gen3->pDataPerChannel[j].pDataPerXPD[k].pcdac);
				}
				if (pRawDataset_gen3->pDataPerChannel[j].pDataPerXPD[k].pwr_t4 != NULL) {
					free(pRawDataset_gen3->pDataPerChannel[j].pDataPerXPD[k].pwr_t4);
				}
			}
			if (pRawDataset_gen3->pDataPerChannel[j].pDataPerXPD != NULL) {
				free(pRawDataset_gen3->pDataPerChannel[j].pDataPerXPD);
			}
		}

		mError(devNum, EIO,"Failed to allocate raw data struct (gen3), freeing anything already allocated\n");
		free(pRawDataset_gen3->pDataPerChannel);
		free(pRawDataset_gen3->pChannels);
		pRawDataset_gen3->numChannels = 0;
		return(0);
	}
	return(1);
}



MANLIB_API A_BOOL setup_EEPROM_dataset_gen3(A_UINT32 devNum, EEPROM_DATA_STRUCT_GEN3 *pEEPROMDataset_gen3, A_UINT16 myNumRawChannels, A_UINT16 *pMyRawChanList)
{
	A_UINT16 i,  channelValue;

	if(!allocateEEPROMDataStruct_gen3(devNum, pEEPROMDataset_gen3, myNumRawChannels)) {
		mError(devNum, EIO,"unable to allocate EEPROM dataset (gen3)\n");
		return(0);
	}


	for (i = 0; i < myNumRawChannels; i++) {
		channelValue = pMyRawChanList[i];
		pEEPROMDataset_gen3->pChannels[i] = channelValue;
		pEEPROMDataset_gen3->pDataPerChannel[i].channelValue = channelValue;
	}

	return(1);
}

A_BOOL	allocateEEPROMDataStruct_gen3(A_UINT32 devNum, EEPROM_DATA_STRUCT_GEN3  *pEEPROMDataset_gen3, A_UINT16 numChannels)
{

	//allocate room for the channels
	pEEPROMDataset_gen3->pChannels = (A_UINT16 *)malloc(sizeof(A_UINT16) * numChannels);
	if (NULL == pEEPROMDataset_gen3->pChannels) {
		mError(devNum, EIO,"unable to allocate EEPROM data struct (gen3)\n");
		return(0);
	}

	pEEPROMDataset_gen3->pDataPerChannel = (EEPROM_DATA_PER_CHANNEL_GEN3 *)malloc(sizeof(EEPROM_DATA_PER_CHANNEL_GEN3) * numChannels);
	if (NULL == pEEPROMDataset_gen3->pDataPerChannel) {
		mError(devNum, EIO,"unable to allocate EEPROM data struct data per channel(gen3)\n");
		if (pEEPROMDataset_gen3->pChannels != NULL) {
			free(pEEPROMDataset_gen3->pChannels);
		}
		return(0);
	}
	
	pEEPROMDataset_gen3->numChannels = numChannels;

	return(1);
}


A_BOOL read_Cal_Dataset_From_EEPROM(A_UINT32 devNum, EEPROM_DATA_STRUCT_GEN3 *pCalDataset, A_UINT32 start_offset, A_UINT32 maxPiers, A_UINT32 *words, A_UINT32 devlibMode) {

	A_UINT16	ii;
	A_UINT16	dbmmask				= 0xff;
	A_UINT16	pcdac_delta_mask	= 0x1f;
	A_UINT16	pcdac_mask			= 0x3f;
	A_UINT16	freqmask			= 0xff;
	A_UINT16	idx, numPiers;
	A_UINT16	freq[NUM_11A_EEPROM_CHANNELS];
	LIB_DEV_INFO *pLibDev = gLibInfo.pLibDevArray[devNum];
	MODE_HEADER_INFO	hdrInfo;

	idx = (A_UINT16)start_offset;
	ii = 0;
	if (devlibMode == MODE_11A) {
		while (ii < maxPiers) {
			
			if ((words[idx] & freqmask) == 0) {
				idx++;
				break;
			} else {
				freq[ii] = fbin2freq_gen3((words[idx] & freqmask), devlibMode);
			}
			ii++;
			
			if (((words[idx] >> 8) & freqmask) == 0) {
				idx++;
				break;
			} else {
				freq[ii] = fbin2freq_gen3(((words[idx] >> 8) & freqmask), devlibMode);
			}
			ii++;
			idx++;
		}
		idx = (A_UINT16)(start_offset + (NUM_11A_EEPROM_CHANNELS / 2) );
	} else {
		hdrInfo = (devlibMode == MODE_11G) ? pLibDev->p16kEepHeader->info11g : pLibDev->p16kEepHeader->info11b;
		ii = 0;
		if (hdrInfo.calPier1 != 0xff) freq[ii++] = hdrInfo.calPier1;
		if (hdrInfo.calPier2 != 0xff) freq[ii++] = hdrInfo.calPier2;
		if (hdrInfo.calPier3 != 0xff) freq[ii++] = hdrInfo.calPier3;
	}
	numPiers = ii;

	if (!setup_EEPROM_dataset_gen3(devNum, pCalDataset, numPiers, &(freq[0])) ) {
		mError(devNum, EIO,"unable to allocate cal dataset (gen3) in read_from_eeprom...\n");
		return(0);
	}

	for (ii=0; ii<pCalDataset->numChannels; ii++) {

		pCalDataset->pDataPerChannel[ii].pwr1_xg0 = (A_INT16)((words[idx] & dbmmask) - ((words[idx] >> 7) & 0x1)*256);
		pCalDataset->pDataPerChannel[ii].pwr2_xg0 = (A_INT16)(((words[idx] >> 8) & dbmmask) - ((words[idx] >> 15) & 0x1)*256);
		
		idx++;
		pCalDataset->pDataPerChannel[ii].pwr3_xg0 = (A_INT16)((words[idx] & dbmmask) - ((words[idx] >> 7) & 0x1)*256);
		pCalDataset->pDataPerChannel[ii].pwr4_xg0 = (A_INT16)(((words[idx] >> 8) & dbmmask) - ((words[idx] >> 15) & 0x1)*256);
		
		idx++;
		pCalDataset->pDataPerChannel[ii].pcd2_delta_xg0 = (A_UINT16)(words[idx] & pcdac_delta_mask);
		pCalDataset->pDataPerChannel[ii].pcd3_delta_xg0 = (A_UINT16)((words[idx] >> 5) & pcdac_delta_mask);
		pCalDataset->pDataPerChannel[ii].pcd4_delta_xg0 = (A_UINT16)((words[idx] >> 10) & pcdac_delta_mask);

		idx++;
		pCalDataset->pDataPerChannel[ii].pwr1_xg3 = (A_INT16)((words[idx] & dbmmask) - ((words[idx] >> 7) & 0x1)*256);
		pCalDataset->pDataPerChannel[ii].pwr2_xg3 = (A_INT16)(((words[idx] >> 8) & dbmmask) - ((words[idx] >> 15) & 0x1)*256);

		idx++;
		pCalDataset->pDataPerChannel[ii].pwr3_xg3 = (A_INT16)((words[idx] & dbmmask) - ((words[idx] >> 7) & 0x1)*256);
		if((pLibDev->p16kEepHeader->majorVersion == 4) && (pLibDev->p16kEepHeader->minorVersion < 3)) {
			pCalDataset->pDataPerChannel[ii].maxPower_t4 = (A_INT16)(((words[idx] >> 8) & dbmmask) - ((words[idx] >> 15) & 0x1)*256);		
			pCalDataset->pDataPerChannel[ii].pcd1_xg0 = 1;
		} else {
			pCalDataset->pDataPerChannel[ii].maxPower_t4 = pCalDataset->pDataPerChannel[ii].pwr4_xg0;		
			pCalDataset->pDataPerChannel[ii].pcd1_xg0 = (A_UINT16)((words[idx] >> 8) & pcdac_mask);
		}

		idx++;
}

	return(1);
}


void eeprom_to_raw_dataset_gen3(A_UINT32 devNum, EEPROM_DATA_STRUCT_GEN3 *pCalDataset, RAW_DATA_STRUCT_GEN3 *pRawDataset) {

	A_UINT16	ii, jj, kk;
	A_INT16		maxPower_t4;
	RAW_DATA_PER_XPD_GEN3			*pRawXPD;
	EEPROM_DATA_PER_CHANNEL_GEN3	*pCalCh;	//ptr to array of info held per channel
	A_UINT16		xgain_list[2];
	A_UINT16		xpd_mask;

	pRawDataset->xpd_mask = pCalDataset->xpd_mask;

	xgain_list[0] = 0xDEAD;
	xgain_list[1] = 0xDEAD;
 
	kk = 0;
	xpd_mask = pRawDataset->xpd_mask;

	for (jj = 0; jj < NUM_XPD_PER_CHANNEL; jj++) {
		if (((xpd_mask >> jj) & 1) > 0) {
			if (kk > 1) {
				printf("A maximum of 2 xpd_gains supported in eep_to_raw_data\n");
				exit(0);
			}
			xgain_list[kk++] = (A_UINT16) jj;			
		}
	}

	pRawDataset->numChannels = pCalDataset->numChannels;
	for (ii = 0; ii < pRawDataset->numChannels; ii++) {
		pCalCh = &(pCalDataset->pDataPerChannel[ii]);
		pRawDataset->pDataPerChannel[ii].channelValue = pCalCh->channelValue;
		pRawDataset->pDataPerChannel[ii].maxPower_t4  = pCalCh->maxPower_t4;
		maxPower_t4 = pRawDataset->pDataPerChannel[ii].maxPower_t4;
	
		if (xgain_list[1] == 0xDEAD) {	// case of single xpd_gain cal
			for (jj=0; jj<NUM_XPD_PER_CHANNEL; jj++) {
				pRawDataset->pDataPerChannel[ii].pDataPerXPD[jj].numPcdacs = 0;
			}

			jj = xgain_list[0];
			pRawXPD = &(pRawDataset->pDataPerChannel[ii].pDataPerXPD[jj]);
			pRawXPD->numPcdacs = 4;
			pRawXPD->pcdac[0] = pCalCh->pcd1_xg0;
			pRawXPD->pcdac[1] = (A_UINT16)(pRawXPD->pcdac[0] + pCalCh->pcd2_delta_xg0);
			pRawXPD->pcdac[2] = (A_UINT16)(pRawXPD->pcdac[1] + pCalCh->pcd3_delta_xg0);
			pRawXPD->pcdac[3] = (A_UINT16)(pRawXPD->pcdac[2] + pCalCh->pcd4_delta_xg0);

			pRawXPD->pwr_t4[0] = pCalCh->pwr1_xg0;
			pRawXPD->pwr_t4[1] = pCalCh->pwr2_xg0;
			pRawXPD->pwr_t4[2] = pCalCh->pwr3_xg0;
			pRawXPD->pwr_t4[3] = pCalCh->pwr4_xg0;
	
		} else {
			for (jj=0; jj<NUM_XPD_PER_CHANNEL; jj++) {
				pRawDataset->pDataPerChannel[ii].pDataPerXPD[jj].numPcdacs = 0;
			}

			pRawDataset->pDataPerChannel[ii].pDataPerXPD[xgain_list[0]].pcdac[0] = pCalCh->pcd1_xg0;
			pRawDataset->pDataPerChannel[ii].pDataPerXPD[xgain_list[1]].pcdac[0] = 20;
			pRawDataset->pDataPerChannel[ii].pDataPerXPD[xgain_list[1]].pcdac[1] = 35;
			pRawDataset->pDataPerChannel[ii].pDataPerXPD[xgain_list[1]].pcdac[2] = 63;

			jj = xgain_list[0];
			pRawXPD = &(pRawDataset->pDataPerChannel[ii].pDataPerXPD[jj]);
			pRawXPD->numPcdacs = 4;
			pRawXPD->pcdac[1] = (A_UINT16)(pRawXPD->pcdac[0] + pCalCh->pcd2_delta_xg0);
			pRawXPD->pcdac[2] = (A_UINT16)(pRawXPD->pcdac[1] + pCalCh->pcd3_delta_xg0);
			pRawXPD->pcdac[3] = (A_UINT16)(pRawXPD->pcdac[2] + pCalCh->pcd4_delta_xg0);
			pRawXPD->pwr_t4[0] = pCalCh->pwr1_xg0;
			pRawXPD->pwr_t4[1] = pCalCh->pwr2_xg0;
			pRawXPD->pwr_t4[2] = pCalCh->pwr3_xg0;
			pRawXPD->pwr_t4[3] = pCalCh->pwr4_xg0;
				
			jj = xgain_list[1];
			pRawXPD = &(pRawDataset->pDataPerChannel[ii].pDataPerXPD[jj]);
			pRawXPD->numPcdacs = 3;

			pRawXPD->pwr_t4[0] = pCalCh->pwr1_xg3;
			pRawXPD->pwr_t4[1] = pCalCh->pwr2_xg3;
			pRawXPD->pwr_t4[2] = pCalCh->pwr3_xg3;
		}
	}
	devNum = 0;  //quiet warnings
}


A_BOOL get_xpd_gain_and_pcdacs_for_powers
(
 A_UINT32				devNum,                         // In
 A_UINT16				channel,                         // In       
 RAW_DATA_STRUCT_GEN3	*pRawDataset,					// In
 A_UINT32				numXpdGain,                      // In
 A_UINT32				xpdGainMask,                      // In     - desired xpd_gain
 A_INT16				*pPowerMin,                      // In/Out	(2 x power)
 A_INT16				*pPowerMax,                      // In/Out	(2 x power)
 A_INT16				*pPowerMid,                      // Out		(2 x power)
 A_UINT16				pXpdGainValues[],               // Out
 A_UINT16				pPCDACValues[]                  // Out 
)
{
	A_UINT32	ii, jj, kk;
	A_INT16		minPwr_t4, maxPwr_t4, Pmin, Pmid;

	A_UINT32	chan_idx_L, chan_idx_R;
	A_UINT16	chan_L, chan_R;
	
	A_INT16		pwr_table0[64];
	A_INT16		pwr_table1[64];
	RAW_DATA_PER_CHANNEL_GEN3	*pRawCh;	
	A_UINT16	pcdacs[10];
	A_INT16		powers[10];
	A_UINT16	numPcd;
	A_INT16		powTableLXPD[2][64];
	A_INT16		powTableHXPD[2][64];
	A_INT16		tmpPowerTable[64];
	A_UINT16		xgain_list[2];
	A_UINT16		xpd_mask;
	LIB_DEV_INFO *pLibDev = gLibInfo.pLibDevArray[devNum];
	
	if (pRawDataset == NULL) {
		mError(devNum, EINVAL,"NULL dataset pointer. This mode may not be supported.\n");
		return(FALSE);
	}
	
	if ((xpdGainMask & pRawDataset->xpd_mask) < 1) {
		mError(devNum, EINVAL,"desired xpdGainMask not supported by calibrated xpd_mask\n");
		return(FALSE);
	}

	maxPwr_t4 = (A_INT16)(2*(*pPowerMax));   // pwr_t2 -> pwr_t4
	minPwr_t4 = (A_INT16)(2*(*pPowerMin));	  // pwr_t2 -> pwr_t4



	xgain_list[0] = 0xDEAD;
	xgain_list[1] = 0xDEAD;
 
	kk = 0;
	xpd_mask = pRawDataset->xpd_mask;

	for (jj = 0; jj < NUM_XPD_PER_CHANNEL; jj++) {
		if (((xpd_mask >> jj) & 1) > 0) {
			if (kk > 1) {
				printf("A maximum of 2 xpd_gains supported in eep_to_raw_data\n");
				exit(0);
			}
			xgain_list[kk++] = (A_UINT16) jj;			
		}
	}

	mdk_GetLowerUpperIndex(channel, &(pRawDataset->pChannels[0]), pRawDataset->numChannels, &(chan_idx_L), &(chan_idx_R));

	kk = 0;
	for (ii=chan_idx_L; ii<=chan_idx_R; ii++) {
		pRawCh = &(pRawDataset->pDataPerChannel[ii]);
		if (xgain_list[1] == 0xDEAD) {
			jj = xgain_list[0];
			numPcd = pRawCh->pDataPerXPD[jj].numPcdacs;
			memcpy(&(pcdacs[0]), &(pRawCh->pDataPerXPD[jj].pcdac[0]), numPcd*sizeof(A_UINT16));
			memcpy(&(powers[0]), &(pRawCh->pDataPerXPD[jj].pwr_t4[0]), numPcd*sizeof(A_INT16));
			if (!mdk_getFullPwrTable(devNum, numPcd, &(pcdacs[0]), &(powers[0]), pRawCh->maxPower_t4, &(tmpPowerTable[0]))) {
				return(FALSE);
			} else {
				memcpy(&(powTableLXPD[kk][0]), &(tmpPowerTable[0]), 64*sizeof(A_INT16));
			}
		} else {
			jj = xgain_list[0];
			numPcd = pRawCh->pDataPerXPD[jj].numPcdacs;
			memcpy(&(pcdacs[0]), &(pRawCh->pDataPerXPD[jj].pcdac[0]), numPcd*sizeof(A_UINT16));
			memcpy(&(powers[0]), &(pRawCh->pDataPerXPD[jj].pwr_t4[0]), numPcd*sizeof(A_INT16));
			if (!mdk_getFullPwrTable(devNum, numPcd, &(pcdacs[0]), &(powers[0]), pRawCh->maxPower_t4, &(tmpPowerTable[0]))) {
				return(FALSE);
			} else {
				memcpy(&(powTableLXPD[kk][0]), &(tmpPowerTable[0]), 64*sizeof(A_INT16));
			}

			jj = xgain_list[1];
			numPcd = pRawCh->pDataPerXPD[jj].numPcdacs;
			memcpy(&(pcdacs[0]), &(pRawCh->pDataPerXPD[jj].pcdac[0]), numPcd*sizeof(A_UINT16));
			memcpy(&(powers[0]), &(pRawCh->pDataPerXPD[jj].pwr_t4[0]), numPcd*sizeof(A_INT16));
			if (!mdk_getFullPwrTable(devNum, numPcd, &(pcdacs[0]), &(powers[0]), pRawCh->maxPower_t4, &(tmpPowerTable[0]))) {
				return(FALSE);
			} else {
				memcpy(&(powTableHXPD[kk][0]), &(tmpPowerTable[0]), 64*sizeof(A_INT16));
			}
		}
			
		kk++;
	}
	
	chan_L = pRawDataset->pChannels[chan_idx_L];
	chan_R = pRawDataset->pChannels[chan_idx_R];
	kk = chan_idx_R - chan_idx_L;

	pLibDev->maxLinPwrx4 = mdk_GetInterpolatedValue_Signed16(channel, chan_L, chan_R, 
										pRawDataset->pDataPerChannel[chan_idx_L].pDataPerXPD[0].pwr_t4[2],
										pRawDataset->pDataPerChannel[chan_idx_R].pDataPerXPD[0].pwr_t4[2]);

	if (xgain_list[1] == 0xDEAD) {
		for (jj=0; jj<64; jj++) {
			pwr_table0[jj] = mdk_GetInterpolatedValue_Signed16(channel, chan_L, chan_R, powTableLXPD[0][jj], powTableLXPD[kk][jj]);			
		}
		Pmin = getPminAndPcdacTableFromPowerTable(devNum, &(pwr_table0[0]), pPCDACValues);
		*pPowerMin = (A_INT16) (Pmin / 2);
		*pPowerMid = (A_INT16) (pwr_table0[63] / 2);
		*pPowerMax = (A_INT16) (pwr_table0[63] / 2);
		pXpdGainValues[0] = xgain_list[0];
		pXpdGainValues[1] = pXpdGainValues[0];
	} else {
		for (jj=0; jj<64; jj++) {
			pwr_table0[jj] = mdk_GetInterpolatedValue_Signed16(channel, chan_L, chan_R, powTableLXPD[0][jj], powTableLXPD[kk][jj]);			
			pwr_table1[jj] = mdk_GetInterpolatedValue_Signed16(channel, chan_L, chan_R, powTableHXPD[0][jj], powTableHXPD[kk][jj]);			
		}
		if (numXpdGain == 2) {
	    	Pmin = getPminAndPcdacTableFromTwoPowerTables(devNum, &(pwr_table0[0]), &(pwr_table1[0]), pPCDACValues, &Pmid);
			*pPowerMin = (A_INT16) (Pmin / 2);
			*pPowerMid = (A_INT16) (Pmid / 2);
			*pPowerMax = (A_INT16) (pwr_table0[63] / 2);
			pXpdGainValues[0] = xgain_list[0];
			pXpdGainValues[1] = xgain_list[1];
		} else {
			if ( (minPwr_t4  <= pwr_table1[63]) && (maxPwr_t4  <= pwr_table1[63])) {
				Pmin = getPminAndPcdacTableFromPowerTable(devNum, &(pwr_table1[0]), pPCDACValues);
				pXpdGainValues[0] = xgain_list[1];
				pXpdGainValues[1] = pXpdGainValues[0];
				*pPowerMin = (A_INT16) (Pmin / 2);
				*pPowerMid = (A_INT16) (pwr_table1[63] / 2);
				*pPowerMax = (A_INT16) (pwr_table1[63] / 2);
			} else {
				Pmin = getPminAndPcdacTableFromPowerTable(devNum, &(pwr_table0[0]), pPCDACValues);
				pXpdGainValues[0] = xgain_list[0];
				pXpdGainValues[1] = xgain_list[0];
				*pPowerMin = (A_INT16) (Pmin/2);
				*pPowerMid = (A_INT16) (pwr_table0[63] / 2);
				*pPowerMax = (A_INT16) (pwr_table0[63] / 2);
			}
		}
	}

	if (isFalcon(devNum)) {
// since eirp_limited mode in falcon can't handle different Pmax values for 2 chains
// make it fixed 0-31.5 dBm
		*pPowerMax  = 63;
//		*pPowerMax += 2*FALCON_BAND_EIRP_HEADROOM_IN_DB;
	}

	devNum = 0;   //quiet compiler

    return(TRUE);
}
	

A_INT16 mdk_GetInterpolatedValue_Signed16(A_UINT16 target, A_UINT16 srcLeft, A_UINT16 srcRight, 
							 A_INT16 targetLeft, A_INT16 targetRight)
{
  A_INT16 returnValue;

  if (srcRight != srcLeft) {
		returnValue = (A_INT16)( ( (target - srcLeft)*targetRight + (srcRight - target)*targetLeft)/(srcRight - srcLeft));
  } 
  else {
		returnValue = targetLeft;
  }
  return (returnValue);
}


// returns indices surrounding the value in sorted integer lists. used for channel and pcdac lists
void mdk_GetLowerUpperIndex (
 A_UINT16	value,			//value to search for
 A_UINT16	*pList,			//ptr to the list to search
 A_UINT16	listSize,		//number of entries in list
 A_UINT32	*pLowerValue,	//return the lower value
 A_UINT32	*pUpperValue	//return the upper value	
)
{
	A_UINT16	i;
	A_UINT16	listEndValue = *(pList + listSize - 1);
	A_UINT16	target = value ;

	//see if value is lower than the first value in the list
	//if so return first value
	if (target <= (*pList)) {
		*pLowerValue = 0;
		*pUpperValue = 0;
		return;
	}
  
	//see if value is greater than last value in list
	//if so return last value
	if (target >= listEndValue) {
		*pLowerValue = listSize - 1;
		*pUpperValue = listSize - 1;
		return;
	}

	//look for value being near or between 2 values in list
	for(i = 0; i < listSize; i++) {
		//if value is close to the current value of the list 
		//then target is not between values, it is one of the values
		if (pList[i] == target) {
			*pLowerValue = i;
			*pUpperValue = i;
			return;
		}

		//look for value being between current value and next value
		//if so return these 2 values
		if (target < pList[i + 1]) {
			*pLowerValue = i;
			*pUpperValue = i + 1;
			return;
		}
	}
} 

A_UINT16 fbin2freq_gen3(A_UINT32 fbin, A_UINT32 mode)
{
	A_UINT16 returnValue; 

	if(mode == MODE_11A) {
		returnValue = (A_UINT16)(4800 + 5*fbin);
	}
	else {
		returnValue = (A_UINT16)(2300 + fbin);
	}
	return returnValue;
}

A_BOOL initialize_datasets(A_UINT32 devNum, EEPROM_DATA_STRUCT_GEN3 *pCalDataset_gen3[], RAW_DATA_STRUCT_GEN3  *pRawDataset_gen3[]) {

	A_UINT32	words[400];
	A_UINT16	start_offset		= 0x150; // for 16k eeprom (0x2BE - 0x150) = 367. 0x2BE is the max possible end of CTL section.
	A_UINT32	maxPiers;
	LIB_DEV_INFO *pLibDev = gLibInfo.pLibDevArray[devNum];

	// read only upto the end of CTL section
	// 0x2BE is the max possible end of CTL section.
	// for 16k eeprom (0x2BE - 0x150) = 367. 
	eepromReadBlock(devNum, (start_offset + pLibDev->eepromStartLoc), 367, words);


	// setup datasets for mode_11a first
	start_offset = 0;

	if (pLibDev->p16kEepHeader->Amode) {
		maxPiers = NUM_11A_EEPROM_CHANNELS;
	
		pCalDataset_gen3[MODE_11A] = (EEPROM_DATA_STRUCT_GEN3 *)malloc(sizeof(EEPROM_DATA_STRUCT_GEN3));
		if(NULL == pCalDataset_gen3[MODE_11A]) {
			mError(devNum, ENOMEM, "unable to allocate 11a gen3 cal data struct\n");
			return(0);
		}
		if (!read_Cal_Dataset_From_EEPROM(devNum, pCalDataset_gen3[MODE_11A], start_offset, maxPiers,  &(words[0]), MODE_11A ) ) {
			mError(devNum, EIO,"unable to allocate cal dataset (gen3) for mode 11a\n");
			return(0);
		}
		pCalDataset_gen3[MODE_11A]->xpd_mask = pLibDev->p16kEepHeader->info11a.xgain;
		pRawDataset_gen3[MODE_11A] = (RAW_DATA_STRUCT_GEN3 *)malloc(sizeof(RAW_DATA_STRUCT_GEN3));
		if(NULL == pRawDataset_gen3[MODE_11A]) {
			mError(devNum, ENOMEM, "unable to allocate 11a gen3 raw data struct\n");
			return(0);
		}
		setup_raw_dataset_gen3(devNum, pRawDataset_gen3[MODE_11A], pCalDataset_gen3[MODE_11A]->numChannels, pCalDataset_gen3[MODE_11A]->pChannels);
		if(pCalDataset_gen3[MODE_11A]->xpd_mask == 0) {
			mError(devNum, EIO,"illegal xpd_mask mode 11a, must be non-zero (gen3)\n");
			return(0);
		}
		eeprom_to_raw_dataset_gen3(devNum, pCalDataset_gen3[MODE_11A], pRawDataset_gen3[MODE_11A]);


		// setup datasets for mode_11b next
		start_offset = (A_UINT16)(5 + pCalDataset_gen3[MODE_11A]->numChannels*5 );
	} else {
		pRawDataset_gen3[MODE_11A] = NULL;
		pCalDataset_gen3[MODE_11A] = NULL;
	}

	if (pLibDev->p16kEepHeader->Bmode) {
		maxPiers = NUM_2_4_EEPROM_CHANNELS;

		pCalDataset_gen3[MODE_11B] = (EEPROM_DATA_STRUCT_GEN3 *)malloc(sizeof(EEPROM_DATA_STRUCT_GEN3));
		if(NULL == pCalDataset_gen3[MODE_11B]) {
			mError(devNum, ENOMEM, "unable to allocate 11b gen3 cal data struct\n");
			return(0);
		}
		if (!read_Cal_Dataset_From_EEPROM(devNum, pCalDataset_gen3[MODE_11B], start_offset, maxPiers,  &(words[0]), MODE_11B ) ) {
			mError(devNum, EIO,"unable to allocate cal dataset (gen3) for mode 11b\n");
			return(0);
		}
		pCalDataset_gen3[MODE_11B]->xpd_mask = pLibDev->p16kEepHeader->info11b.xgain;
		pRawDataset_gen3[MODE_11B] = (RAW_DATA_STRUCT_GEN3 *)malloc(sizeof(RAW_DATA_STRUCT_GEN3));
		if(NULL == pRawDataset_gen3[MODE_11B]) {
			mError(devNum, ENOMEM, "unable to allocate 11b gen3 raw data struct\n");
			return(0);
		}
		setup_raw_dataset_gen3(devNum, pRawDataset_gen3[MODE_11B], pCalDataset_gen3[MODE_11B]->numChannels, pCalDataset_gen3[MODE_11B]->pChannels);
		if(pCalDataset_gen3[MODE_11B]->xpd_mask == 0) {
			mError(devNum, EIO,"illegal xpd_mask (mode 11b), must be non-zero (gen3)\n");
			return(0);
		}
		eeprom_to_raw_dataset_gen3(devNum, pCalDataset_gen3[MODE_11B], pRawDataset_gen3[MODE_11B]);

		// setup datasets for mode_11g next
		start_offset = (A_UINT16) (start_offset + pCalDataset_gen3[MODE_11B]->numChannels*5);
	} else {
		pRawDataset_gen3[MODE_11B] = NULL;
		pCalDataset_gen3[MODE_11B] = NULL;
	}

	if (pLibDev->p16kEepHeader->Gmode) {
		maxPiers = NUM_2_4_EEPROM_CHANNELS;
		pCalDataset_gen3[MODE_11G] = (EEPROM_DATA_STRUCT_GEN3 *)malloc(sizeof(EEPROM_DATA_STRUCT_GEN3));
		if(NULL == pCalDataset_gen3[MODE_11G]) {
			mError(devNum, ENOMEM, "unable to allocate 11g gen3 cal data struct\n");
			return(0);
		}
		if (!read_Cal_Dataset_From_EEPROM(devNum, pCalDataset_gen3[MODE_11G], start_offset, maxPiers,  &(words[0]), MODE_11G ) ) {
			mError(devNum, EIO,"unable to allocate cal dataset (gen3) for mode 11g\n");
			return(0);
		}
		pCalDataset_gen3[MODE_11G]->xpd_mask = pLibDev->p16kEepHeader->info11g.xgain;
		pRawDataset_gen3[MODE_11G] = (RAW_DATA_STRUCT_GEN3 *)malloc(sizeof(RAW_DATA_STRUCT_GEN3));
		if(NULL == pRawDataset_gen3[MODE_11G]) {
			mError(devNum, ENOMEM, "unable to allocate 11g gen3 raw data struct\n");
			return(0);
		}
		setup_raw_dataset_gen3(devNum, pRawDataset_gen3[MODE_11G], pCalDataset_gen3[MODE_11G]->numChannels, pCalDataset_gen3[MODE_11G]->pChannels);
		if(pCalDataset_gen3[MODE_11G]->xpd_mask == 0) {
			mError(devNum, EIO,"illegal xpd_mask mode 11g, must be non-zero (gen3)\n");
			return(0);
		}
		eeprom_to_raw_dataset_gen3(devNum, pCalDataset_gen3[MODE_11G], pRawDataset_gen3[MODE_11G]);
	} else {
		pRawDataset_gen3[MODE_11G] = NULL;
		pCalDataset_gen3[MODE_11G] = NULL;
	}

	// REMEMBER TO FREE THE CAL DATASETS HERE

	return(1);
}

// expand the pcdac intercepts to generate full pcdacs-to-power table for pcdacs 1..63. 
//    capped with maxPower
A_BOOL mdk_getFullPwrTable(A_UINT32 devNum, A_UINT16 numPcdacs, A_UINT16 *pcdacs, A_INT16 *power, A_INT16 maxPower, A_INT16 *retVals) {

	A_UINT16	ii;
	A_UINT16	idxL = 0;
	A_UINT16	idxR = 1;

	if (numPcdacs < 2) {
		mError(devNum, EINVAL, "at least 2 pcdac values needed in mdk_getFullPwrTable - [%d]\n", numPcdacs);
		return(FALSE);
	}

	for (ii=0; ii<64; ii++) {
		if ((ii>pcdacs[idxR]) && (idxR < (numPcdacs-1))) {
			idxL++;
			idxR++;
		}
		retVals[ii] = mdk_GetInterpolatedValue_Signed16(ii, pcdacs[idxL], pcdacs[idxR], power[idxL], power[idxR]);
		if (retVals[ii] >= maxPower) {
			while (ii<64) {
				retVals[ii++] = maxPower;
			}
		}
	}
	return(TRUE);
}


A_INT16 getPminAndPcdacTableFromPowerTable(A_UINT32 devNum, A_INT16 *pwrTable_t4, A_UINT16 retVals[]) {
	A_INT16	ii, jj, jj_max;
	A_INT16		Pmin, currPower, Pmax;
	
	// if the spread is > 31.5dB, keep the upper 31.5dB range
	if ((pwrTable_t4[63] - pwrTable_t4[0]) > 126) {
		Pmin = (A_INT16) (pwrTable_t4[63] - 126);
	} else {
		Pmin = pwrTable_t4[0];
	}

/*
	if (Pmin >= 0) {
		Pmin = 0;
	}
*/

	Pmax = pwrTable_t4[63];
	jj_max = 63;
	// search for highest pcdac 0.25dB below maxPower
	while ((pwrTable_t4[jj_max] > (Pmax - 1) ) && (jj_max >= 0)){
		jj_max--;
	} 


	jj = jj_max;
	currPower = Pmax;
	if (isFalcon(devNum)) {
// since eirp_limited mode in falcon can't handle different Pmax values for 2 chains
// make it fixed 0-31.5 dBm
		currPower = 126;
		Pmin = 0;
//		currPower += 4*FALCON_BAND_EIRP_HEADROOM_IN_DB;
//		Pmin      += 4*FALCON_BAND_EIRP_HEADROOM_IN_DB;
	}

	for (ii=63; ii>=0; ii--) {
		while ((jj<64) && (jj>0) && (pwrTable_t4[jj] >= currPower)) {
			jj--;
		}
		if (jj == 0) {
			while (ii >= 0) {
				retVals[ii] = retVals[ii+1];
				ii--;
			}
			break;
		}
		retVals[ii] = jj;
		currPower -= 2;  // corresponds to a 0.5dB step
	}
	return(Pmin);
}

A_INT16 getPminAndPcdacTableFromTwoPowerTables(A_UINT32 devNum, A_INT16 *pwrTableLXPD_t4, A_INT16 *pwrTableHXPD_t4, A_UINT16 retVals[], A_INT16 *Pmid) {
	A_INT16		ii, jj, jj_max;
	A_INT16		Pmin, currPower, Pmax;
	A_INT16		*pwrTable_t4;
	A_UINT16	msbFlag = 0x40;  // turns on the 7th bit of the pcdac
	
	// if the spread is > 31.5dB, keep the upper 31.5dB range
	if ((pwrTableLXPD_t4[63] - pwrTableHXPD_t4[0]) > 126) {
		Pmin = (A_INT16)(pwrTableLXPD_t4[63] - 126);		
	} else {
		Pmin = pwrTableHXPD_t4[0];
	}

/*
	if (Pmin >= 0) {
		Pmin = 0;
	}
*/
	Pmax = pwrTableLXPD_t4[63];
	jj_max = 63;
	// search for highest pcdac 0.25dB below maxPower
	while ((pwrTableLXPD_t4[jj_max] > (Pmax - 1) ) && (jj_max >= 0)){
		jj_max--;
	} 


	*Pmid = pwrTableHXPD_t4[63];
	jj = jj_max;
	ii = 63;
	currPower = Pmax;
	if (isFalcon(devNum)) {
// since eirp_limited mode in falcon can't handle different Pmax values for 2 chains
// make it fixed 0-31.5 dBm
		currPower = 126;
		Pmin = 0;
//		currPower += 4*FALCON_BAND_EIRP_HEADROOM_IN_DB;
//		Pmin      += 4*FALCON_BAND_EIRP_HEADROOM_IN_DB;
	}
	pwrTable_t4 = &(pwrTableLXPD_t4[0]);
	while(ii >= 0) {
		if ((currPower <= *Pmid) || ( (jj == 0) && (msbFlag == 0x40))){
			msbFlag = 0x00;
			pwrTable_t4 = &(pwrTableHXPD_t4[0]);
			jj = 63;
		}
		while ((jj>0) && (pwrTable_t4[jj] >= currPower)) {
			jj--;
		}
		if ((jj == 0) && (msbFlag == 0x00)) {
			while (ii >= 0) {
				retVals[ii] = retVals[ii+1];
				ii--;
			}
			break;
		}
		retVals[ii] = (A_UINT16)(jj | msbFlag);
		currPower -= 2;  // corresponds to a 0.5dB step
		ii--;
	}
	return(Pmin);
}


void
copyGen3EepromStruct
(
 EEPROM_FULL_DATA_STRUCT_GEN3 *pFullCalDataset_gen3,
 EEPROM_DATA_STRUCT_GEN3 *pCalDataset_gen3[]
)
{
	if (pCalDataset_gen3[MODE_11A] != NULL) {
		//copy the 11a structs
		pFullCalDataset_gen3->numChannels11a = pCalDataset_gen3[MODE_11A]->numChannels;
		pFullCalDataset_gen3->xpd_mask11a = pCalDataset_gen3[MODE_11A]->xpd_mask;
		memcpy(pFullCalDataset_gen3->pDataPerChannel11a, pCalDataset_gen3[MODE_11A]->pDataPerChannel, 
			sizeof(EEPROM_DATA_PER_CHANNEL_GEN3) * pCalDataset_gen3[MODE_11A]->numChannels);
	}

	if (pCalDataset_gen3[MODE_11B] != NULL) {
		//copy the 11b structs
		pFullCalDataset_gen3->numChannels11b = pCalDataset_gen3[MODE_11B]->numChannels;
		pFullCalDataset_gen3->xpd_mask11b = pCalDataset_gen3[MODE_11B]->xpd_mask;
		memcpy(pFullCalDataset_gen3->pDataPerChannel11b, pCalDataset_gen3[MODE_11B]->pDataPerChannel, 
			sizeof(EEPROM_DATA_PER_CHANNEL_GEN3) * pCalDataset_gen3[MODE_11B]->numChannels);
	}

	if (pCalDataset_gen3[MODE_11G] != NULL) {
		//copy the 11g structs
		pFullCalDataset_gen3->numChannels11g = pCalDataset_gen3[MODE_11G]->numChannels;
		pFullCalDataset_gen3->xpd_mask11g = pCalDataset_gen3[MODE_11G]->xpd_mask;
		memcpy(pFullCalDataset_gen3->pDataPerChannel11g, pCalDataset_gen3[MODE_11G]->pDataPerChannel, 
			sizeof(EEPROM_DATA_PER_CHANNEL_GEN3) * pCalDataset_gen3[MODE_11G]->numChannels);
	}
}


