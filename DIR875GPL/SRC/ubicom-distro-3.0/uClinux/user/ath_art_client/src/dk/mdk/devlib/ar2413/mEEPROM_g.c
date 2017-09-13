/* mEEPROM_g.c - contians functions for reading eeprom and getting            */
/*                 Vpd power settings for all channels for gen5 analog chips  */
/* Copyright (c) 2004 Atheros Communications, Inc., All Rights Reserved         */


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

#include "mEEPROM_g.h"


static A_UINT32 swDevId;

MANLIB_API A_BOOL setup_raw_dataset_gen5(A_UINT32 devNum, RAW_DATA_STRUCT_GEN5 *pRawDataset_gen5, A_UINT16 myNumRawChannels, A_UINT16 *pMyRawChanList, A_UINT32 swDeviceID)
{
	A_UINT16	i, j, kk, channelValue;	
	A_UINT32   xpd_mask;
	A_UINT16   numPdGainsUsed = 0;

	swDevId = swDeviceID;
	if(!allocateRawDataStruct_gen5(devNum, pRawDataset_gen5, myNumRawChannels)) {
		mError(devNum, EIO,"unable to allocate raw dataset (gen5)\n");
		return(0);
	}

	xpd_mask = pRawDataset_gen5->xpd_mask;	
	for (i = 0; i < 4; i++) {
		if ( ((xpd_mask >> i) & 0x1) > 0) {
			numPdGainsUsed++;
		}
	}

	for (i = 0; i < myNumRawChannels; i++) {
		channelValue = pMyRawChanList[i];

		pRawDataset_gen5->pChannels[i] = channelValue;
		
		pRawDataset_gen5->pDataPerChannel[i].channelValue = channelValue;
		pRawDataset_gen5->pDataPerChannel[i].numPdGains = numPdGainsUsed;
		kk = 0;
		for (j = 0; j < MAX_NUM_PDGAINS_PER_CHANNEL; j++) {
			pRawDataset_gen5->pDataPerChannel[i].pDataPerPDGain[j].pd_gain = j;
			if ( ((xpd_mask >> j) & 0x1) > 0) {
				//macRev = art_regRead(devNum, 0x20);

                if(isOwl(swDeviceID)) {
   					pRawDataset_gen5->pDataPerChannel[i].pDataPerPDGain[j].numVpd = NUM_POINTS_LAST_PDGAIN;   
                } else {
					pRawDataset_gen5->pDataPerChannel[i].pDataPerPDGain[j].numVpd = NUM_POINTS_OTHER_PDGAINS;
                }

                kk++;
				if (kk == 1) { 
					// lowest pd_gain corresponds to highest power and thus, has one more point 
					pRawDataset_gen5->pDataPerChannel[i].pDataPerPDGain[j].numVpd = NUM_POINTS_LAST_PDGAIN;
				}
//printf("SNOOP: i = %d, j=%d, xpd_mask = 0x%x, numVpd = %d [A]\n", i, j, xpd_mask, pRawDataset_gen5->pDataPerChannel[i].pDataPerPDGain[j].numVpd);
			} else {
				pRawDataset_gen5->pDataPerChannel[i].pDataPerPDGain[j].numVpd = 0;
//printf("SNOOP: i = %d, j=%d, xpd_mask = 0x%x, numVpd = %d [B]\n", i, j, xpd_mask, pRawDataset_gen5->pDataPerChannel[i].pDataPerPDGain[j].numVpd);			
			}
		}		
	}

	return(1);
}



A_BOOL	allocateRawDataStruct_gen5(A_UINT32 devNum, RAW_DATA_STRUCT_GEN5  *pRawDataset_gen5, A_UINT16 numChannels)
{
	A_UINT16	i, j, k;
	A_UINT16	numVpds = NUM_POINTS_LAST_PDGAIN;

	///allocate room for the channels
	pRawDataset_gen5->pChannels = (A_UINT16 *)malloc(sizeof(A_UINT16) * numChannels);
	if (NULL == pRawDataset_gen5->pChannels) {
		mError(devNum, EIO,"unable to allocate raw data struct (gen5)\n");
		return(0);
	}

	pRawDataset_gen5->pDataPerChannel = (RAW_DATA_PER_CHANNEL_GEN5 *)malloc(sizeof(RAW_DATA_PER_CHANNEL_GEN5) * numChannels);
	if (NULL == pRawDataset_gen5->pDataPerChannel) {
		mError(devNum, EIO,"unable to allocate raw data struct data per channel(gen5)\n");
		free(pRawDataset_gen5->pChannels);
		return(0);
	}
	
	pRawDataset_gen5->numChannels = numChannels;

	for(i = 0; i < numChannels; i ++) {
		pRawDataset_gen5->pDataPerChannel[i].pDataPerPDGain = NULL;
		
		pRawDataset_gen5->pDataPerChannel[i].pDataPerPDGain = (RAW_DATA_PER_PDGAIN_GEN5 *)malloc(sizeof(RAW_DATA_PER_PDGAIN_GEN5) * MAX_NUM_PDGAINS_PER_CHANNEL);
		if (NULL == pRawDataset_gen5->pDataPerChannel[i].pDataPerPDGain) {
			mError(devNum, EIO,"unable to allocate raw data struct pDataPerPDGain (gen5)\n");
			break; //will cleanup outside loop
		}
		for (j=0; j<MAX_NUM_PDGAINS_PER_CHANNEL ; j++) {
			pRawDataset_gen5->pDataPerChannel[i].pDataPerPDGain[j].Vpd = (A_UINT16 *)malloc(numVpds*sizeof(A_UINT16));
			if(pRawDataset_gen5->pDataPerChannel[i].pDataPerPDGain[j].Vpd == NULL) {
				mError(devNum, EIO,"unable to allocate Vpds for a pd_gain (gen5)\n");
				break;
			}
			pRawDataset_gen5->pDataPerChannel[i].pDataPerPDGain[j].pwr_t4 = (A_INT16 *)malloc(numVpds*sizeof(A_INT16));
			if(pRawDataset_gen5->pDataPerChannel[i].pDataPerPDGain[j].pwr_t4 == NULL) {
				mError(devNum, EIO,"unable to allocate pwr_t4 for a pd_gain (gen5)\n");
				if (pRawDataset_gen5->pDataPerChannel[i].pDataPerPDGain[j].Vpd != NULL) {
					free(pRawDataset_gen5->pDataPerChannel[i].pDataPerPDGain[j].Vpd);
				}
				break; // cleanup outside the j loop
			}
		}
		if (j != MAX_NUM_PDGAINS_PER_CHANNEL) { // malloc must've failed
			for (k=0; k<j; k++) {
				if (pRawDataset_gen5->pDataPerChannel[i].pDataPerPDGain[k].Vpd != NULL) {
					free(pRawDataset_gen5->pDataPerChannel[i].pDataPerPDGain[k].Vpd);
				}
				if (pRawDataset_gen5->pDataPerChannel[i].pDataPerPDGain[k].pwr_t4 != NULL) {
					free(pRawDataset_gen5->pDataPerChannel[i].pDataPerPDGain[k].pwr_t4);
				}
			}
			break; // cleanupo outside i loop
		}
	}

	if (i != numChannels) {
		//malloc must have failed, cleanup any allocs
		for (j = 0; j < i; j++) {
			for (k=0; k<MAX_NUM_PDGAINS_PER_CHANNEL; k++) {
				if (pRawDataset_gen5->pDataPerChannel[j].pDataPerPDGain[k].Vpd != NULL) {
					free(pRawDataset_gen5->pDataPerChannel[j].pDataPerPDGain[k].Vpd);
				}
				if (pRawDataset_gen5->pDataPerChannel[j].pDataPerPDGain[k].pwr_t4 != NULL) {
					free(pRawDataset_gen5->pDataPerChannel[j].pDataPerPDGain[k].pwr_t4);
				}
			}
			if (pRawDataset_gen5->pDataPerChannel[j].pDataPerPDGain != NULL) {
				free(pRawDataset_gen5->pDataPerChannel[j].pDataPerPDGain);
			}
		}

		mError(devNum, EIO,"Failed to allocate raw data struct (gen5), freeing anything already allocated\n");
		free(pRawDataset_gen5->pDataPerChannel);
		free(pRawDataset_gen5->pChannels);
		pRawDataset_gen5->numChannels = 0;
		return(0);
	}
	return(1);
}


MANLIB_API A_BOOL setup_EEPROM_dataset_gen5(A_UINT32 devNum, EEPROM_DATA_STRUCT_GEN5 *pEEPROMDataset_gen5, A_UINT16 myNumRawChannels, A_UINT16 *pMyRawChanList)
{
	A_UINT16   i, channelValue;
	A_UINT32   xpd_mask;
	A_UINT16   numPdGainsUsed = 0;

	if(!allocateEEPROMDataStruct_gen5(devNum, pEEPROMDataset_gen5, myNumRawChannels)) {
		mError(devNum, EIO,"unable to allocate EEPROM dataset (gen5)\n");
		return(0);
	}

	xpd_mask = pEEPROMDataset_gen5->xpd_mask;	
	for (i = 0; i < 4; i++) {
		if ( ((xpd_mask >> i) & 0x1) > 0) {
			numPdGainsUsed++;
		}
	}

	for (i = 0; i < myNumRawChannels; i++) {
		channelValue = pMyRawChanList[i];
		pEEPROMDataset_gen5->pChannels[i] = channelValue;
		pEEPROMDataset_gen5->pDataPerChannel[i].channelValue = channelValue;
		pEEPROMDataset_gen5->pDataPerChannel[i].numPdGains = numPdGainsUsed;
	}

	return(1);
}



A_BOOL	allocateEEPROMDataStruct_gen5(A_UINT32 devNum, EEPROM_DATA_STRUCT_GEN5  *pEEPROMDataset_gen5, A_UINT16 numChannels)
{

	//allocate room for the channels
	pEEPROMDataset_gen5->pChannels = (A_UINT16 *)malloc(sizeof(A_UINT16) * numChannels);
	if (NULL == pEEPROMDataset_gen5->pChannels) {
		mError(devNum, EIO,"unable to allocate EEPROM data struct (gen5)\n");
		return(0);
	}

	pEEPROMDataset_gen5->pDataPerChannel = (EEPROM_DATA_PER_CHANNEL_GEN5 *)malloc(sizeof(EEPROM_DATA_PER_CHANNEL_GEN5) * numChannels);
	if (NULL == pEEPROMDataset_gen5->pDataPerChannel) {
		mError(devNum, EIO,"unable to allocate EEPROM data struct data per channel(gen5)\n");
		if (pEEPROMDataset_gen5->pChannels != NULL) {
			free(pEEPROMDataset_gen5->pChannels);
		}
		return(0);
	}
	
	pEEPROMDataset_gen5->numChannels = numChannels;

	return(1);
}



A_BOOL read_Cal_Dataset_From_EEPROM_gen5(A_UINT32 devNum, EEPROM_DATA_STRUCT_GEN5 *pCalDataset, A_UINT32 start_offset, A_UINT32 maxPiers, A_UINT32 *words, A_UINT32 devlibMode) {

	A_UINT16	ii=0;
	A_UINT16	freqmask			= 0xff;
	A_UINT16	dbm_I_mask			= 0x1F; // 5-bits. 1dB step.
	A_UINT16	dbm_delta_mask		= 0xF;  // 4-bits. 0.5dB step.
	A_UINT16	Vpd_I_mask			= 0x7F; // 7-bits. 0-128
	A_UINT16	Vpd_delta_mask		= 0x3F; // 6-bits. 0-63
	A_UINT16	idx, numPiers;
	A_UINT16	freq[NUM_11A_EEPROM_CHANNELS];
//	LIB_DEV_INFO *pLibDev = gLibInfo.pLibDevArray[devNum];
//	MODE_HEADER_INFO	hdrInfo;
	EEPROM_DATA_PER_CHANNEL_GEN5  *currCh;

	idx = (A_UINT16)start_offset;
	ii = 0;
	// no 11a piers, but 11b and 11g piers added in eep_map = 2
	while (ii < maxPiers) {
		if ((words[idx] & freqmask) == 0) {
			idx++;
			break;
		} else {
			freq[ii] = fbin2freq_gen5((words[idx] & freqmask), devlibMode);
//printf ("this is nothing\n");
		}
		ii++;
		
		if (((words[idx] >> 8) & freqmask) == 0) {
			idx++;
			break;
		} else {
			freq[ii] = fbin2freq_gen5(((words[idx] >> 8) & freqmask), devlibMode);
		}
		ii++;
		idx++;
	}
	idx = (A_UINT16)(start_offset + (maxPiers / 2) );

//	hdrInfo = (devlibMode == MODE_11G) ? pLibDev->p16kEepHeader->info11g : pLibDev->p16kEepHeader->info11b;
//	ii = 0;
//	if (hdrInfo.calPier1 != 0xff) freq[ii++] = hdrInfo.calPier1;
//	if (hdrInfo.calPier2 != 0xff) freq[ii++] = hdrInfo.calPier2;
//	if (hdrInfo.calPier3 != 0xff) freq[ii++] = hdrInfo.calPier3;

	numPiers = ii;
	if (!setup_EEPROM_dataset_gen5(devNum, pCalDataset, numPiers, &(freq[0])) ) {
		mError(devNum, EIO,"unable to allocate cal dataset (gen5) in read_from_eeprom...\n");
		return(0);
	}

	for (ii=0; ii<pCalDataset->numChannels; ii++) {

		currCh = &(pCalDataset->pDataPerChannel[ii]);
		
		if (currCh->numPdGains > 0) {
			// read the first NUM_POINTS_OTHER_PDGAINS pwr and Vpd values for pdgain_0
			currCh->pwr_I[0] = (A_UINT16)(words[idx] & dbm_I_mask);
//printf("SNOOP: pwr_I_0 = %d[idx=%d]\n", currCh->pwr_I[0], idx);
			currCh->Vpd_I[0] = (A_UINT16)((words[idx] >> 5) & Vpd_I_mask);
//printf("SNOOP: Vpd_I_0 = %d\n", currCh->Vpd_I[0]);
			currCh->pwr_delta_t2[0][0] = (A_UINT16)((words[idx] >> 12) & dbm_delta_mask);
		
			idx++;
			currCh->Vpd_delta[0][0] = (A_UINT16)(words[idx] & Vpd_delta_mask);
			currCh->pwr_delta_t2[1][0] = (A_UINT16)((words[idx] >> 6) & dbm_delta_mask);
			currCh->Vpd_delta[1][0] = (A_UINT16)((words[idx] >> 10) & Vpd_delta_mask);

			idx++;
			currCh->pwr_delta_t2[2][0] = (A_UINT16)(words[idx] & dbm_delta_mask);
			currCh->Vpd_delta[2][0] = (A_UINT16)((words[idx] >> 4) & Vpd_delta_mask);
		}


		if (currCh->numPdGains > 1) {
			// read the first NUM_POINTS_OTHER_PDGAINS pwr and Vpd values for pdgain_1
			currCh->pwr_I[1] = (A_UINT16)((words[idx] >> 10) & dbm_I_mask);
//printf("SNOOP: pwr_I_1 = %d\n", currCh->pwr_I[1]);
			currCh->Vpd_I[1] = (A_UINT16)((words[idx] >> 15) & 0x1);

			idx++;
			currCh->Vpd_I[1] |= (A_UINT16)((words[idx] & 0x3F) << 1); // upper 6 bits
//printf("SNOOP: Vpd_I_1 = %d\n", currCh->Vpd_I[1]);
			currCh->pwr_delta_t2[0][1] = (A_UINT16)((words[idx] >> 6) & dbm_delta_mask);
			currCh->Vpd_delta[0][1] = (A_UINT16)((words[idx] >> 10) & Vpd_delta_mask);

			idx++;
			currCh->pwr_delta_t2[1][1] = (A_UINT16)(words[idx] & dbm_delta_mask);
			currCh->Vpd_delta[1][1] = (A_UINT16)((words[idx] >> 4) & Vpd_delta_mask);
			currCh->pwr_delta_t2[2][1] = (A_UINT16)((words[idx] >> 10) & dbm_delta_mask);			
			currCh->Vpd_delta[2][1] = (A_UINT16)((words[idx] >> 14) & 0x3);

			idx++;
			currCh->Vpd_delta[2][1] |= (A_UINT16)((words[idx] & 0xF) << 2); // upper 4 bits

		} else if (currCh->numPdGains == 1) {
			// read the last pwr and Vpd values for pdgain_0
			currCh->pwr_delta_t2[3][0] = (A_UINT16)((words[idx] >> 10) & dbm_delta_mask);
			currCh->Vpd_delta[3][0] = (A_UINT16)((words[idx] >> 14) & 0x3);

			idx++;
			currCh->Vpd_delta[3][0] |= (A_UINT16)((words[idx] & 0xF) << 2); // upper 4 bits
			
			idx++;
			// 4 words if numPdGains == 1
		}

		if (currCh->numPdGains > 2) {
			// read the first NUM_POINTS_OTHER_PDGAINS pwr and Vpd values for pdgain_2
			currCh->pwr_I[2] = (A_UINT16)((words[idx] >> 4) & dbm_I_mask);
			currCh->Vpd_I[2] = (A_UINT16)((words[idx] >> 9) & Vpd_I_mask);

			idx++;
			currCh->pwr_delta_t2[0][2] = (A_UINT16)((words[idx] >> 0) & dbm_delta_mask);
			currCh->Vpd_delta[0][2] = (A_UINT16)((words[idx] >> 4) & Vpd_delta_mask);
			currCh->pwr_delta_t2[1][2] = (A_UINT16)((words[idx] >> 10) & dbm_delta_mask);
			currCh->Vpd_delta[1][2] = (A_UINT16)((words[idx] >> 14) & 0x3);

			idx++;
			currCh->Vpd_delta[1][2] |= (A_UINT16)((words[idx] & 0xF) << 2); // upper 4 bits
			currCh->pwr_delta_t2[2][2] = (A_UINT16)((words[idx] >> 4) & dbm_delta_mask);
			currCh->Vpd_delta[2][2] = (A_UINT16)((words[idx] >> 8) & Vpd_delta_mask);


		} else  if (currCh->numPdGains == 2){
			// read the last pwr and Vpd values for pdgain_1
			currCh->pwr_delta_t2[3][1] = (A_UINT16)((words[idx] >> 4) & dbm_delta_mask);
			currCh->Vpd_delta[3][1] = (A_UINT16)((words[idx] >> 8) & Vpd_delta_mask);

			idx++;
			// 6 words if numPdGains == 2
		}

		if (currCh->numPdGains > 3) {
			// read the first NUM_POINTS_OTHER_PDGAINS pwr and Vpd values for pdgain_3
			currCh->pwr_I[3] = (A_UINT16)((words[idx] >> 14) & 0x3);

			idx++;
			currCh->pwr_I[3] |= (A_UINT16)(((words[idx] >> 0) & 0x7) << 2); // upper 3 bits
			currCh->Vpd_I[3] = (A_UINT16)((words[idx] >> 3) & Vpd_I_mask);
			currCh->pwr_delta_t2[0][3] = (A_UINT16)((words[idx] >> 10) & dbm_delta_mask);
			currCh->Vpd_delta[0][3] = (A_UINT16)((words[idx] >> 14) & 0x3);

			idx++;
			currCh->Vpd_delta[0][3] |= (A_UINT16)((words[idx] & 0xF) << 2); // upper 4 bits
			currCh->pwr_delta_t2[1][3] = (A_UINT16)((words[idx] >> 4) & dbm_delta_mask);
			currCh->Vpd_delta[1][3] = (A_UINT16)((words[idx] >> 8) & Vpd_delta_mask);
			currCh->pwr_delta_t2[2][3] = (A_UINT16)((words[idx] >> 14) & 0x3);

			idx++;
			currCh->pwr_delta_t2[2][3] |= (A_UINT16)(((words[idx] >> 0) & 0x3) << 2); // upper 2 bits
			currCh->Vpd_delta[2][3] = (A_UINT16)((words[idx] >> 2) & Vpd_delta_mask);
			currCh->pwr_delta_t2[3][3] = (A_UINT16)((words[idx] >> 8) & dbm_delta_mask); 
			currCh->Vpd_delta[3][3] = (A_UINT16)((words[idx] >> 12) & 0xF);

			idx++;
			currCh->Vpd_delta[3][3] |= (A_UINT16)(((words[idx] >> 0) & 0x3) << 4); // upper 2 bits

			idx++;
			// 12 words if numPdGains == 4

		} else  if (currCh->numPdGains == 3){
			// read the last pwr and Vpd values for pdgain_2
			currCh->pwr_delta_t2[3][2] = (A_UINT16)((words[idx] >> 14) & 0x3);

			idx++;
			currCh->pwr_delta_t2[3][2] |= (A_UINT16)(((words[idx] >> 0) & 0x3) << 2); // upper 2 bits
			currCh->Vpd_delta[3][2] = (A_UINT16)((words[idx] >> 2) & Vpd_delta_mask);

			idx++;
			// 9 words if numPdGains == 3
		}

	}

	ii = (A_UINT16) maxPiers; // to quiet compiler warnings
	return(1);
}



void eeprom_to_raw_dataset_gen5(A_UINT32 devNum, EEPROM_DATA_STRUCT_GEN5 *pCalDataset, RAW_DATA_STRUCT_GEN5 *pRawDataset) {

	A_UINT16	ii, jj, kk, ss;
	RAW_DATA_PER_PDGAIN_GEN5			*pRawXPD;
	EEPROM_DATA_PER_CHANNEL_GEN5	*pCalCh;	//ptr to array of info held per channel
	A_UINT16		xgain_list[MAX_NUM_PDGAINS_PER_CHANNEL];
	A_UINT16		xpd_mask;
	A_UINT32        numPdGainsUsed = 0;

	if (pRawDataset->xpd_mask != pCalDataset->xpd_mask) {
		mError(devNum, EIO,"xpd_mask values incompatible between raw and eeprom datasets\n");
		exit(0);
	}

	xgain_list[0] = 0xDEAD;
	xgain_list[1] = 0xDEAD;
	xgain_list[2] = 0xDEAD;
	xgain_list[3] = 0xDEAD;
 
	kk = 0;
	xpd_mask = pRawDataset->xpd_mask;

	for (jj = 0; jj < MAX_NUM_PDGAINS_PER_CHANNEL; jj++) {
		if (((xpd_mask >> (MAX_NUM_PDGAINS_PER_CHANNEL-jj-1)) & 1) > 0) {
			if (kk >= MAX_NUM_PDGAINS_PER_CHANNEL) {
				printf("A maximum of 4 pd_gains supported in eep_to_raw_data for gen5\n");
				exit(0);
			}
			xgain_list[kk++] = (A_UINT16) (MAX_NUM_PDGAINS_PER_CHANNEL-jj-1);			
		}
	}

	numPdGainsUsed = kk;

	pRawDataset->numChannels = pCalDataset->numChannels;
	for (ii = 0; ii < pRawDataset->numChannels; ii++) {
		pCalCh = &(pCalDataset->pDataPerChannel[ii]);
		pRawDataset->pDataPerChannel[ii].channelValue = pCalCh->channelValue;
//		pRawDataset->pDataPerChannel[ii].maxPower_t4  = pCalCh->maxPower_t4;
//		maxPower_t4 = pRawDataset->pDataPerChannel[ii].maxPower_t4;


		// numVpd has already been setup appropriately for the relevant pdGains

//printf("SNOOP: channel = %d\n", pCalCh->channelValue);
		for (jj = 0; jj<numPdGainsUsed; jj++) {
			ss = xgain_list[jj];  // use jj for calDataset and ss for rawDataset
			pRawXPD = &(pRawDataset->pDataPerChannel[ii].pDataPerPDGain[ss]);
			if (pRawXPD->numVpd < 1) {
				printf("ERROR : numVpd for ch[%d] = %d, pdgain = %d[%d] should not be 0 for xpd_mask = 0x%x\n", ii, 
					                  pCalCh->channelValue, ss, jj, xpd_mask);
				exit(0);
			}
				
			pRawXPD->pwr_t4[0] = (A_INT16)(4*pCalCh->pwr_I[jj]);
			pRawXPD->Vpd[0]    = pCalCh->Vpd_I[jj];
//printf("SNOOP: ss=%d, Vpd[0] = %d, pwr[0] = %d\n", ss, pRawXPD->Vpd[0], pRawXPD->pwr_t4[0]);
			for (kk = 1; kk < pRawXPD->numVpd; kk++) {
				pRawXPD->pwr_t4[kk] = (A_INT16)(pRawXPD->pwr_t4[kk-1] + 2*pCalCh->pwr_delta_t2[kk-1][jj]);
				pRawXPD->Vpd[kk] = (A_UINT16)(pRawXPD->Vpd[kk-1] + pCalCh->Vpd_delta[kk-1][jj]);
//printf("SNOOP: ss=%d, Vpd[%d] = %d, pwr[%d] = %d\n", ss, kk, pRawXPD->Vpd[kk], kk, pRawXPD->pwr_t4[kk]);
			} // loop over Vpds

		} // loop over pd_gains
	} // loop over channels
	devNum = 0;  //quiet warnings
}


/*
A_BOOL get_xpd_gain_and_pcdacs_for_powers
(
 A_UINT32				devNum,                         // In
 A_UINT16				channel,                         // In       
 RAW_DATA_STRUCT_GEN5	*pRawDataset,					// In
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
	RAW_DATA_PER_CHANNEL_GEN5	*pRawCh;	
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

	for (jj = 0; jj < MAX_NUM_PDGAINS_PER_CHANNEL; jj++) {
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
		Pmin = getPminAndPcdacTableFromPowerTable(&(pwr_table0[0]), pPCDACValues);
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
	    	Pmin = getPminAndPcdacTableFromTwoPowerTables(&(pwr_table0[0]), &(pwr_table1[0]), pPCDACValues, &Pmid);
			*pPowerMin = (A_INT16) (Pmin / 2);
			*pPowerMid = (A_INT16) (Pmid / 2);
			*pPowerMax = (A_INT16) (pwr_table0[63] / 2);
			pXpdGainValues[0] = xgain_list[0];
			pXpdGainValues[1] = xgain_list[1];
		} else {
			if ( (minPwr_t4  <= pwr_table1[63]) && (maxPwr_t4  <= pwr_table1[63])) {
				Pmin = getPminAndPcdacTableFromPowerTable(&(pwr_table1[0]), pPCDACValues);
				pXpdGainValues[0] = xgain_list[1];
				pXpdGainValues[1] = pXpdGainValues[0];
				*pPowerMin = (A_INT16) (Pmin / 2);
				*pPowerMid = (A_INT16) (pwr_table1[63] / 2);
				*pPowerMax = (A_INT16) (pwr_table1[63] / 2);
			} else {
				Pmin = getPminAndPcdacTableFromPowerTable(&(pwr_table0[0]), pPCDACValues);
				pXpdGainValues[0] = xgain_list[0];
				pXpdGainValues[1] = xgain_list[0];
				*pPowerMin = (A_INT16) (Pmin/2);
				*pPowerMid = (A_INT16) (pwr_table0[63] / 2);
				*pPowerMax = (A_INT16) (pwr_table0[63] / 2);
			}
		}
	}

	devNum = 0;   //quiet compiler

    return(TRUE);
}
*/

/*
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
*/

// returns indices surrounding the value in sorted integer lists. used for channel and pcdac lists
void mdk_GetLowerUpperIndex_Signed16 (
 A_INT16	value,			//value to search for
 A_INT16	*pList,			//ptr to the list to search
 A_UINT16	listSize,		//number of entries in list
 A_UINT32	*pLowerValue,	//return the lower index
 A_UINT32	*pUpperValue	//return the upper index	
)
{
	A_UINT16	i;
	A_INT16	listEndValue = *(pList + listSize - 1);
	A_INT16	target = value ;

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

A_BOOL initialize_datasets_gen5(A_UINT32 devNum, EEPROM_DATA_STRUCT_GEN5 *pCalDataset_gen5[], RAW_DATA_STRUCT_GEN5  *pRawDataset_gen5[]) {

	A_UINT32	words[400];
	A_UINT16	start_offset		= 0x150; // for 16k eeprom (0x2BE - 0x150) = 367. 0x2BE is the max possible end of CTL section.
												//add 10 to ignore dummy data written for driver
	A_UINT32	maxPiers;
	LIB_DEV_INFO *pLibDev = gLibInfo.pLibDevArray[devNum];
	A_UINT16    numEEPROMWordsPerChannel;
	A_UINT16    wordsForPdgains[] = {4,6,9,12}; // index is 1 less than numPdgains

//printf("SNOOP: initialize_datasets_gen5 : entered [%d, %d]\n", MODE_11B, MODE_11G);
	// read only upto the end of CTL section
	// 0x2BE is the max possible end of CTL section.
	// for 16k eeprom (0x2BE - 0x150) = 367. 
	start_offset = pLibDev->p16kEepHeader->calStartLocation;
	eepromReadBlock(devNum, (start_offset + pLibDev->eepromStartLoc), 367, words);


	// setup datasets for mode_11b first
	start_offset = 0;

	if (pLibDev->p16kEepHeader->Amode) {
		maxPiers = NUM_11A_EEPROM_CHANNELS;
		pCalDataset_gen5[MODE_11A] = (EEPROM_DATA_STRUCT_GEN5 *)malloc(sizeof(EEPROM_DATA_STRUCT_GEN5));
		if(NULL == pCalDataset_gen5[MODE_11A]) {
			mError(devNum, ENOMEM, "unable to allocate 11a gen5 cal data struct\n");
			return(0);
		}
		pCalDataset_gen5[MODE_11A]->xpd_mask = pLibDev->p16kEepHeader->info11a.xgain;
		if (!read_Cal_Dataset_From_EEPROM_gen5(devNum, pCalDataset_gen5[MODE_11A], start_offset, maxPiers,  &(words[0]), MODE_11A ) ) {
			mError(devNum, EIO,"unable to allocate cal dataset (gen5) for mode 11a\n");
			return(0);
		}		
		pRawDataset_gen5[MODE_11A] = (RAW_DATA_STRUCT_GEN5 *)malloc(sizeof(RAW_DATA_STRUCT_GEN5));
		if(NULL == pRawDataset_gen5[MODE_11A]) {
			mError(devNum, ENOMEM, "unable to allocate 11a gen5 raw data struct\n");
			return(0);
		}
		pRawDataset_gen5[MODE_11A]->xpd_mask = pLibDev->p16kEepHeader->info11a.xgain;
		setup_raw_dataset_gen5(devNum, pRawDataset_gen5[MODE_11A], pCalDataset_gen5[MODE_11A]->numChannels, pCalDataset_gen5[MODE_11A]->pChannels, swDevId);
		eeprom_to_raw_dataset_gen5(devNum, pCalDataset_gen5[MODE_11A], pRawDataset_gen5[MODE_11A]);

		// setup datasets for mode_11b next
		numEEPROMWordsPerChannel = wordsForPdgains[pCalDataset_gen5[MODE_11A]->pDataPerChannel[0].numPdGains - 1];
	    start_offset = (A_UINT16) (start_offset + pCalDataset_gen5[MODE_11A]->numChannels*numEEPROMWordsPerChannel + 5);
	} else {
		pRawDataset_gen5[MODE_11A] = NULL;
		pCalDataset_gen5[MODE_11A] = NULL;
	}

	if (pLibDev->p16kEepHeader->Bmode) {
		maxPiers = NUM_2_4_EEPROM_CHANNELS_GEN5;

		pCalDataset_gen5[MODE_11B] = (EEPROM_DATA_STRUCT_GEN5 *)malloc(sizeof(EEPROM_DATA_STRUCT_GEN5));
		if(NULL == pCalDataset_gen5[MODE_11B]) {
			mError(devNum, ENOMEM, "unable to allocate 11b gen5 cal data struct\n");
			return(0);
		}
		pCalDataset_gen5[MODE_11B]->xpd_mask = pLibDev->p16kEepHeader->info11b.xgain;
		if (!read_Cal_Dataset_From_EEPROM_gen5(devNum, pCalDataset_gen5[MODE_11B], start_offset, maxPiers,  &(words[0]), MODE_11B ) ) {
			mError(devNum, EIO,"unable to allocate cal dataset (gen5) for mode 11b\n");
			return(0);
		}
		pRawDataset_gen5[MODE_11B] = (RAW_DATA_STRUCT_GEN5 *)malloc(sizeof(RAW_DATA_STRUCT_GEN5));
		if(NULL == pRawDataset_gen5[MODE_11B]) {
			mError(devNum, ENOMEM, "unable to allocate 11b gen5 raw data struct\n");
			return(0);
		}
		
		pRawDataset_gen5[MODE_11B]->xpd_mask = pLibDev->p16kEepHeader->info11b.xgain;
		setup_raw_dataset_gen5(devNum, pRawDataset_gen5[MODE_11B], pCalDataset_gen5[MODE_11B]->numChannels, pCalDataset_gen5[MODE_11B]->pChannels, swDevId);
		eeprom_to_raw_dataset_gen5(devNum, pCalDataset_gen5[MODE_11B], pRawDataset_gen5[MODE_11B]);

		// setup datasets for mode_11g next
		numEEPROMWordsPerChannel = wordsForPdgains[pCalDataset_gen5[MODE_11B]->pDataPerChannel[0].numPdGains - 1];
		start_offset = (A_UINT16) (start_offset + pCalDataset_gen5[MODE_11B]->numChannels*numEEPROMWordsPerChannel + 2);
	} else {
		pRawDataset_gen5[MODE_11B] = NULL;
		pCalDataset_gen5[MODE_11B] = NULL;
	}

	if (pLibDev->p16kEepHeader->Gmode) {
		maxPiers = NUM_2_4_EEPROM_CHANNELS_GEN5;
		pCalDataset_gen5[MODE_11G] = (EEPROM_DATA_STRUCT_GEN5 *)malloc(sizeof(EEPROM_DATA_STRUCT_GEN5));
		if(NULL == pCalDataset_gen5[MODE_11G]) {
			mError(devNum, ENOMEM, "unable to allocate 11g gen5 cal data struct\n");
			return(0);
		}
		pCalDataset_gen5[MODE_11G]->xpd_mask = pLibDev->p16kEepHeader->info11g.xgain;
		if (!read_Cal_Dataset_From_EEPROM_gen5(devNum, pCalDataset_gen5[MODE_11G], start_offset, maxPiers,  &(words[0]), MODE_11G ) ) {
			mError(devNum, EIO,"unable to allocate cal dataset (gen5) for mode 11g\n");
			return(0);
		}		
		pRawDataset_gen5[MODE_11G] = (RAW_DATA_STRUCT_GEN5 *)malloc(sizeof(RAW_DATA_STRUCT_GEN5));
		if(NULL == pRawDataset_gen5[MODE_11G]) {
			mError(devNum, ENOMEM, "unable to allocate 11g gen5 raw data struct\n");
			return(0);
		}
		pRawDataset_gen5[MODE_11G]->xpd_mask = pLibDev->p16kEepHeader->info11g.xgain;
		setup_raw_dataset_gen5(devNum, pRawDataset_gen5[MODE_11G], pCalDataset_gen5[MODE_11G]->numChannels, pCalDataset_gen5[MODE_11G]->pChannels, swDevId);
		eeprom_to_raw_dataset_gen5(devNum, pCalDataset_gen5[MODE_11G], pRawDataset_gen5[MODE_11G]);
	} else {
		pRawDataset_gen5[MODE_11G] = NULL;
		pCalDataset_gen5[MODE_11G] = NULL;
	}

	// REMEMBER TO FREE THE CAL DATASETS HERE
//printf("SNOOP: initialize_datasets_gen5 : exited\n");
	return(1);
}


/*
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
*/

/*
A_INT16 getPminAndPcdacTableFromPowerTable(A_INT16 *pwrTable_t4, A_UINT16 retVals[]) {
	A_INT16	ii, jj, jj_max;
	A_INT16		Pmin, currPower, Pmax;
	
	// if the spread is > 31.5dB, keep the upper 31.5dB range
	if ((pwrTable_t4[63] - pwrTable_t4[0]) > 126) {
		Pmin = (A_INT16) (pwrTable_t4[63] - 126);
	} else {
		Pmin = pwrTable_t4[0];
	}

	Pmax = pwrTable_t4[63];
	jj_max = 63;
	// search for highest pcdac 0.25dB below maxPower
	while ((pwrTable_t4[jj_max] > (Pmax - 1) ) && (jj_max >= 0)){
		jj_max--;
	} 


	jj = jj_max;
	currPower = Pmax;
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
*/

/*
A_INT16 getPminAndPcdacTableFromTwoPowerTables(A_INT16 *pwrTableLXPD_t4, A_INT16 *pwrTableHXPD_t4, A_UINT16 retVals[], A_INT16 *Pmid) {
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
*/

void copyGen5EepromStruct
(
 EEPROM_FULL_DATA_STRUCT_GEN5 *pFullCalDataset_gen5,
 EEPROM_DATA_STRUCT_GEN5 *pCalDataset_gen5[]
)
{
	if (pCalDataset_gen5[MODE_11A] != NULL) {
		//copy the 11a structs
		pFullCalDataset_gen5->numChannels11a = pCalDataset_gen5[MODE_11A]->numChannels;
		pFullCalDataset_gen5->xpd_mask11a = pCalDataset_gen5[MODE_11A]->xpd_mask;
		memcpy(pFullCalDataset_gen5->pDataPerChannel11a, pCalDataset_gen5[MODE_11A]->pDataPerChannel, 
			sizeof(EEPROM_DATA_PER_CHANNEL_GEN5) * pCalDataset_gen5[MODE_11A]->numChannels);
	}

	if (pCalDataset_gen5[MODE_11B] != NULL) {
		//copy the 11b structs
		pFullCalDataset_gen5->numChannels11b = pCalDataset_gen5[MODE_11B]->numChannels;
		pFullCalDataset_gen5->xpd_mask11b = pCalDataset_gen5[MODE_11B]->xpd_mask;
		memcpy(pFullCalDataset_gen5->pDataPerChannel11b, pCalDataset_gen5[MODE_11B]->pDataPerChannel, 
			sizeof(EEPROM_DATA_PER_CHANNEL_GEN5) * pCalDataset_gen5[MODE_11B]->numChannels);
	}

	if (pCalDataset_gen5[MODE_11G] != NULL) {
		//copy the 11g structs
		pFullCalDataset_gen5->numChannels11g = pCalDataset_gen5[MODE_11G]->numChannels;
		pFullCalDataset_gen5->xpd_mask11g = pCalDataset_gen5[MODE_11G]->xpd_mask;
		memcpy(pFullCalDataset_gen5->pDataPerChannel11g, pCalDataset_gen5[MODE_11G]->pDataPerChannel, 
			sizeof(EEPROM_DATA_PER_CHANNEL_GEN5) * pCalDataset_gen5[MODE_11G]->numChannels);
	}
}

A_BOOL get_gain_boundaries_and_pdadcs_for_powers
(
 A_UINT32 devNum,                          // In
 A_UINT16 channel,                         // In       
 RAW_DATA_STRUCT_GEN5 *pRawDataset,        // In
 A_UINT16 pdGainOverlap_t2,                // In
 A_INT16 *pMinCalPower,                    // Out	(2 x min calibrated power)
 A_UINT16 pPdGainBoundaries[],             // Out
 A_UINT16 pPdGainValues[],                 // Out
 A_UINT16 pPDADCValues[]                   // Out 
 ) 
{
	A_UINT32  ii, jj, kk;
	A_INT32   ss; // potentially -ve index for taking care of pdGainOverlap
	A_UINT32  idxL, idxR;
	A_UINT32  numPdGainsUsed = 0;
	A_UINT16  VpdTable_L[MAX_NUM_PDGAINS_PER_CHANNEL][MAX_PWR_RANGE_IN_HALF_DB]; // filled out Vpd table for all pdGains (chanL)
	A_UINT16  VpdTable_R[MAX_NUM_PDGAINS_PER_CHANNEL][MAX_PWR_RANGE_IN_HALF_DB]; // filled out Vpd table for all pdGains (chanR)
	A_UINT16  VpdTable_I[MAX_NUM_PDGAINS_PER_CHANNEL][MAX_PWR_RANGE_IN_HALF_DB]; // filled out Vpd table for all pdGains (interpolated)
	// if desired to support -ve power levels in future, just change pwr_I_0 to signed 5-bits.
	A_INT16   Pmin_t2[MAX_NUM_PDGAINS_PER_CHANNEL];  // to accomodate -ve power levels later on. 
	A_INT16   Pmax_t2[MAX_NUM_PDGAINS_PER_CHANNEL];  // to accomodate -ve power levels later on
	A_UINT16  numVpd = 0;
	A_UINT16  Vpd_step;
	A_INT16   tmpVal ; 
	A_UINT32  sizeCurrVpdTable, maxIndex, tgtIndex;
	
	// get upper lower index
	mdk_GetLowerUpperIndex(channel, pRawDataset->pChannels, pRawDataset->numChannels, &(idxL), &(idxR));

//printf ("SNOOP: idxL = %d, idxR = %d, pdOverlap*2 = %d\n", idxL, idxR, pdGainOverlap_t2);
	for (ii = 0; ii < MAX_NUM_PDGAINS_PER_CHANNEL; ii++) {
		jj = MAX_NUM_PDGAINS_PER_CHANNEL - ii - 1; // work backwards 'cause highest pdGain for lowest power

		numVpd = pRawDataset->pDataPerChannel[idxL].pDataPerPDGain[jj].numVpd;
//printf("SNOOP: ii[%d], jj[%d], numVpd = %d\n", ii, jj, numVpd);
		if (numVpd > 0) {
			pPdGainValues[numPdGainsUsed] = pRawDataset->pDataPerChannel[idxL].pDataPerPDGain[jj].pd_gain;
			Pmin_t2[numPdGainsUsed] = pRawDataset->pDataPerChannel[idxL].pDataPerPDGain[jj].pwr_t4[0];
			if (Pmin_t2[numPdGainsUsed] > pRawDataset->pDataPerChannel[idxR].pDataPerPDGain[jj].pwr_t4[0]) {
				Pmin_t2[numPdGainsUsed] = pRawDataset->pDataPerChannel[idxR].pDataPerPDGain[jj].pwr_t4[0];
			}
			Pmin_t2[numPdGainsUsed] = (A_INT16)(Pmin_t2[numPdGainsUsed] / 2);

			Pmax_t2[numPdGainsUsed] = pRawDataset->pDataPerChannel[idxL].pDataPerPDGain[jj].pwr_t4[numVpd-1];
//printf ("SNOOP: idxL = %d, idxR = %d, numVpd = %d, pMax*4[%d] = %d\n", idxL, idxR, numVpd, numPdGainsUsed, Pmax_t2[numPdGainsUsed]);
			if (Pmax_t2[numPdGainsUsed] > pRawDataset->pDataPerChannel[idxR].pDataPerPDGain[jj].pwr_t4[numVpd-1]) {
				Pmax_t2[numPdGainsUsed] = pRawDataset->pDataPerChannel[idxR].pDataPerPDGain[jj].pwr_t4[numVpd-1];
			}
//printf ("SNOOP: pMax*4[%d] = %d\n", numPdGainsUsed, Pmax_t2[numPdGainsUsed]);
			Pmax_t2[numPdGainsUsed] = (A_INT16)(Pmax_t2[numPdGainsUsed] / 2);
//printf ("SNOOP: pMax*2[%d] = %d\n", numPdGainsUsed, Pmax_t2[numPdGainsUsed]);
//printf("SNOOP: currGainNum = %d, pdGain = %d, Pmin*2 = %d, Pmax*2 = %d\n", numPdGainsUsed, pPdGainValues[numPdGainsUsed], Pmin_t2[numPdGainsUsed], Pmax_t2[numPdGainsUsed]);
			fill_Vpd_Table(numPdGainsUsed, Pmin_t2[numPdGainsUsed], Pmax_t2[numPdGainsUsed], &(pRawDataset->pDataPerChannel[idxL].pDataPerPDGain[jj].pwr_t4[0]),
				           &(pRawDataset->pDataPerChannel[idxL].pDataPerPDGain[jj].Vpd[0]), numVpd, VpdTable_L);
			fill_Vpd_Table(numPdGainsUsed, Pmin_t2[numPdGainsUsed], Pmax_t2[numPdGainsUsed], &(pRawDataset->pDataPerChannel[idxR].pDataPerPDGain[jj].pwr_t4[0]),
				           &(pRawDataset->pDataPerChannel[idxR].pDataPerPDGain[jj].Vpd[0]), numVpd, VpdTable_R);
			for (kk = 0; kk < (A_UINT16)(Pmax_t2[numPdGainsUsed] - Pmin_t2[numPdGainsUsed]); kk++) {
				VpdTable_I[numPdGainsUsed][kk] = mdk_GetInterpolatedValue_Signed16(channel, pRawDataset->pChannels[idxL], pRawDataset->pChannels[idxR],
															(A_INT16)VpdTable_L[numPdGainsUsed][kk], (A_INT16)VpdTable_R[numPdGainsUsed][kk]);
			} // fill VpdTable_I for this pdGain
			numPdGainsUsed++;
		} // if this pdGain is used
	}

		*pMinCalPower = Pmin_t2[0];
		kk = 0; // index for the final table
		for (ii = 0; ii < (numPdGainsUsed ); ii++) {

			if (ii == (numPdGainsUsed - 1)) {
				// artifically stretch the boundary 2dB above the max measured data (~22dBm).
				// helps move gain boundary away from the highest tgt pwrs. 
				pPdGainBoundaries[ii] = (A_UINT16)(Pmax_t2[ii] + PD_GAIN_BOUNDARY_STRETCH_IN_HALF_DB);
			} else {
				pPdGainBoundaries[ii] = (A_UINT16)( (Pmax_t2[ii] + Pmin_t2[ii+1]) / 2 );
			}

			if (pPdGainBoundaries[ii] > 63) {
				pPdGainBoundaries[ii] = 63;
			}

//printf("SNOOP : pdGainBoundary[%d] = %d\n", ii, pPdGainBoundaries[ii]);

			// find starting index for this pdGain
			if (ii == 0) {
				ss = 0; // for the first pdGain, start from index 0
			} else {
				ss = (pPdGainBoundaries[ii-1] - Pmin_t2[ii]) - pdGainOverlap_t2 + 1;
				//ss = (pPdGainBoundaries[ii-1] - Pmin_t2[ii]) - pdGainOverlap_t2 ; // do overlap - 1 below per PH
			}

			Vpd_step = (A_UINT16)(VpdTable_I[ii][1] - VpdTable_I[ii][0]);
			Vpd_step = (A_UINT16)((Vpd_step < 1) ? 1 : Vpd_step);
			// -ve ss indicates need to extrapolate data below for this pdGain
			while ((ss < 0) && (kk < (PDADC_TABLE_SIZE-1))){
				tmpVal = (A_INT16)(VpdTable_I[ii][0] + ss*Vpd_step);
				pPDADCValues[kk++] = (A_UINT16)((tmpVal < 0) ? 0 : tmpVal);
				ss++;
			} // while ss < 0

			sizeCurrVpdTable = Pmax_t2[ii] - Pmin_t2[ii];
			tgtIndex = pPdGainBoundaries[ii] + pdGainOverlap_t2 - Pmin_t2[ii];
			// pick the smaller one
			maxIndex = (tgtIndex < sizeCurrVpdTable) ? tgtIndex : sizeCurrVpdTable;

			while ((ss < (A_INT16)maxIndex)  && (kk < (PDADC_TABLE_SIZE-1))){
				pPDADCValues[kk++] = VpdTable_I[ii][ss++];
			}
			
			Vpd_step = (A_UINT16)(VpdTable_I[ii][sizeCurrVpdTable-1] - VpdTable_I[ii][sizeCurrVpdTable-2]);
			Vpd_step = (A_UINT16)((Vpd_step < 1) ? 1 : Vpd_step);			
			// for last gain, pdGainBoundary == Pmax_t2, so will have to extrapolate
			if (tgtIndex > maxIndex) {  // need to extrapolate above
				while((ss <= (A_INT16)tgtIndex)  && (kk < (PDADC_TABLE_SIZE-1))){
					tmpVal = (A_UINT16)(VpdTable_I[ii][sizeCurrVpdTable-1] + 
											(ss-maxIndex)*Vpd_step);
					pPDADCValues[kk++] = (A_UINT16) ((tmpVal > 127) ? 127 : tmpVal);
					ss++;
				}
			} // extrapolated above

		} // for all pdGainUsed
		while (ii < MAX_NUM_PDGAINS_PER_CHANNEL) {
			pPdGainBoundaries[ii] = pPdGainBoundaries[ii-1];
			ii++;
		}

		while (kk < 128) {
			pPDADCValues[kk] = pPDADCValues[kk-1];
			kk++;
		}

//		for (ii=0; ii<MAX_NUM_PDGAINS_PER_CHANNEL; ii++) {
//			printf("SNOOP: %d : gainValue = %d, gainBoundary = %d\n", ii, pPdGainValues[ii], pPdGainBoundaries[ii]);
//		}
//		printf("SNOOP: NumPdGainsUsed = %d, minCalPwr = %d\npdadcTable = ", numPdGainsUsed, *pMinCalPower);
//		for (ii=0; ii<128; ii++) {
//			printf("[%d,%d],", ii, pPDADCValues[ii]);
//		}
//		printf("\n");
		
	numPdGainsUsed = devNum; // to quiet warnings

	return(TRUE);

}

// fill the Vpdlist for indices Pmax-Pmin
void fill_Vpd_Table(A_UINT32 pdGainIdx, A_INT16 Pmin, A_INT16  Pmax, A_INT16 *pwrList, 
					A_UINT16 *VpdList, A_UINT16 numIntercepts, A_UINT16 retVpdList[][64])
{
	A_UINT16  ii, jj, kk;
	A_INT16   currPwr = (A_INT16)(2*Pmin); // since Pmin is pwr*2 and pwrList is 4*pwr
	A_UINT32  idxL, idxR;

	ii = 0;
	jj = 0;

	if (numIntercepts < 2) {
		printf("ERROR : fill_Vpd_Table : numIntercepts should be at least 2\n");
		exit(0);
	}

	while (ii <= (A_UINT16)(Pmax - Pmin)) {
		mdk_GetLowerUpperIndex_Signed16(currPwr, pwrList, numIntercepts, &(idxL), &(idxR));
		if (idxR < 1) {
			idxR = 1; // extrapolate below
		}
		if (idxL == (A_UINT32)(numIntercepts - 1)) {
			idxL = numIntercepts - 2; // extrapolate above
		}
//printf("SNOOP: nI=%d,idxL:%d,idxR:%d --> ", numIntercepts, idxL, idxR);		
		if (pwrList[idxL] == pwrList[idxR]) {
			kk = VpdList[idxL];
		} else {
			kk = (A_UINT16)( ( (currPwr - pwrList[idxL])*VpdList[idxR]+ (pwrList[idxR] - currPwr)*VpdList[idxL])/(pwrList[idxR] - pwrList[idxL]));
		}

		retVpdList[pdGainIdx][ii] = kk;
//printf("[%d:(%d,%d)[cP%d,PL%d,PR%d,VL%d,VR%d]:%d]\n", ii, idxL, idxR, currPwr, pwrList[idxL], pwrList[idxR], VpdList[idxL], VpdList[idxR], retVpdList[pdGainIdx][ii]);
		ii++;
		currPwr += 2; // half dB steps
	}
}

A_UINT16 fbin2freq_gen5(A_UINT32 fbin, A_UINT32 mode)
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

A_BOOL initialize_datasets_forced_eeprom_gen5(A_UINT32 devNum, EEPROM_DATA_STRUCT_GEN5 **pDummyCalDataset_gen5, RAW_DATA_STRUCT_GEN5  **pDummyRawDataset_gen5, A_UINT32 *words, A_UINT16 xpd_mask) {

	A_UINT16	start_offset		= 0; // for this routine, must pass the array starting at 11g cal data
	
	A_UINT32	maxPiers;
//	A_UINT16    numEEPROMWordsPerChannel;
//	A_UINT16    wordsForPdgains[] = {4,6,9,12}; // index is 1 less than numPdgains

	if (words == NULL) {
		mError(devNum, EIO,"initialize_datasets_forced_eeprom_gen5 : null array pointer supplied. Exiting...\n");		
		return(0);
	}

	// setup datasets for mode_11b first
	start_offset = 0;
	maxPiers = NUM_2_4_EEPROM_CHANNELS_GEN5;

	*pDummyCalDataset_gen5 = (EEPROM_DATA_STRUCT_GEN5 *)malloc(sizeof(EEPROM_DATA_STRUCT_GEN5));
	if(NULL == (*pDummyCalDataset_gen5)) {
		mError(devNum, ENOMEM, "unable to allocate 11g gen5 cal data struct\n");
		return(0);
	}
	(*pDummyCalDataset_gen5)->xpd_mask = xpd_mask;
	if (!read_Cal_Dataset_From_EEPROM_gen5(devNum, (*pDummyCalDataset_gen5), start_offset, maxPiers,  &(words[0]), MODE_11G ) ) {
		mError(devNum, EIO,"unable to allocate cal dataset (gen5) for mode 11g\n");
		return(0);
	}		
	(*pDummyRawDataset_gen5) = (RAW_DATA_STRUCT_GEN5 *)malloc(sizeof(RAW_DATA_STRUCT_GEN5));
	if(NULL == (*pDummyRawDataset_gen5)) {
		mError(devNum, ENOMEM, "unable to allocate 11g gen5 raw data struct\n");
		return(0);
	}

	(*pDummyRawDataset_gen5)->xpd_mask = xpd_mask;
	setup_raw_dataset_gen5(devNum, (*pDummyRawDataset_gen5), (*pDummyCalDataset_gen5)->numChannels, (*pDummyCalDataset_gen5)->pChannels, swDevId);
	eeprom_to_raw_dataset_gen5(devNum, (*pDummyCalDataset_gen5), (*pDummyRawDataset_gen5));

	// REMEMBER TO FREE THE CAL DATASETS HERE
	return(1);
}
