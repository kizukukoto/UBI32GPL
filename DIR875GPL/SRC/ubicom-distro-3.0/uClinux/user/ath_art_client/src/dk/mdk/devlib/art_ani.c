/* art_ani.c - contians ANI functions for ART and mdk                   */
/* Copyright (c) 2003 Atheros Communications, Inc., All Rights Reserved */

/* 
Revsision history
--------------------
1.0       Created.
*/
#ifdef __ATH_DJGPPDOS__
#include <unistd.h>
#ifndef EILSEQ  
    #define EILSEQ EIO
#endif	// EILSEQ

#define __int64	long long
#define HANDLE long
typedef unsigned long DWORD;
#define Sleep	delay
#endif	// #ifdef __ATH_DJGPPDOS__

#ifdef VXWORKS
#include "vxworks.h"
#endif

#include <assert.h>

#include "wlantype.h"
#include "athreg.h"
#include "manlib.h"
#include "mDevtbl.h"
#include "mConfig.h"
#include "art_ani.h"

static A_BOOL automaticNoiseImmunityEnabled = FALSE;

A_BOOL configArtAniSetup
(
 A_UINT32 devNum,
 A_UINT32 artAniEnable,
 A_UINT32 artAniReuse
)
{
	ART_ANI_SETUP *pArtAniSetup;
	LIB_DEV_INFO *pLibDev;

	pLibDev = gLibInfo.pLibDevArray[devNum];
	assert(pLibDev != NULL);

	pArtAniSetup = &(gLibInfo.pLibDevArray[devNum]->artAniSetup);

	assert(pArtAniSetup != NULL);

	pArtAniSetup->monitorPeriod			= ART_ANI_MONITOR_PERIOD;
	pArtAniSetup->Enabled				= artAniEnable;
	pArtAniSetup->currBILevel			= 0;
	pArtAniSetup->currNILevel			= 0;
	pArtAniSetup->currSILevel			= 0;
	pArtAniSetup->numChannelsInHistory	= 0;
	pArtAniSetup->Reuse					= artAniReuse;

	return(TRUE);
}

A_BOOL enableArtAniSetup
(
 A_UINT32 devNum
)
{
	LIB_DEV_INFO *pLibDev;
	A_INT32      devIndex;

	pLibDev = gLibInfo.pLibDevArray[devNum];
	assert(pLibDev != NULL);

	pLibDev->artAniSetup.Enabled  = ART_ANI_ENABLED;

	devIndex = pLibDev->ar5kInitIndex;

	// check for valid initIndex as this routine may be called before the first reset
	if ((devIndex == UNKNOWN_INIT_INDEX)||(devIndex < 0)) {
		devIndex = findDevTableEntry(devNum);
		if (devIndex < 0) {
			return(FALSE);
		}
	}

    ar5kInitData[devIndex].pArtAniAPI->enableArtAni(devNum, ART_ANI_TYPE_NI);
    ar5kInitData[devIndex].pArtAniAPI->enableArtAni(devNum, ART_ANI_TYPE_BI);
    ar5kInitData[devIndex].pArtAniAPI->enableArtAni(devNum, ART_ANI_TYPE_SI);

	return(TRUE);
}

A_BOOL disableArtAniSetup
(
 A_UINT32 devNum
)
{
	LIB_DEV_INFO *pLibDev;
	A_INT32      devIndex;

	pLibDev = gLibInfo.pLibDevArray[devNum];
	assert(pLibDev != NULL);

	pLibDev->artAniSetup.Enabled  = ART_ANI_DISABLED;

	devIndex = pLibDev->ar5kInitIndex;

	// check for valid initIndex as this routine may be called before the first reset
	if ((devIndex == UNKNOWN_INIT_INDEX)||(devIndex < 0)) {
		devIndex = findDevTableEntry(devNum);
		if (devIndex < 0) {
			return(FALSE);
		}
	}

    ar5kInitData[devIndex].pArtAniAPI->disableArtAni(devNum, ART_ANI_TYPE_NI);
    ar5kInitData[devIndex].pArtAniAPI->disableArtAni(devNum, ART_ANI_TYPE_BI);
    ar5kInitData[devIndex].pArtAniAPI->disableArtAni(devNum, ART_ANI_TYPE_SI);

	return(TRUE);
}

A_BOOL tweakArtAni
(
 A_UINT32 devNum,
 A_UINT32 prev_freq,
 A_UINT32 curr_freq
)
{
	LIB_DEV_INFO *pLibDev;

	pLibDev = gLibInfo.pLibDevArray[devNum];
	assert(pLibDev != NULL);

	if (!pLibDev->artAniSetup.Enabled) {
		return (TRUE);
	}

	if ((prev_freq == curr_freq) && artAniHistoryExists(devNum, curr_freq) && pLibDev->artAniSetup.Reuse){
		artAniRecallLevels(devNum, curr_freq);
	} else {
		if (automaticNoiseImmunityEnabled) {
			artAniOptimizeLevels(devNum, curr_freq);
		}
	}

	artAniProgramCurrLevels(devNum);
	
	return(TRUE);
}


A_BOOL artAniHistoryExists
(
 A_UINT32 devNum, 
 A_UINT32 curr_freq
)
{
	A_UINT32 ii;
	LIB_DEV_INFO *pLibDev;

	pLibDev = gLibInfo.pLibDevArray[devNum];
	assert(pLibDev != NULL);
	
	assert((pLibDev->artAniSetup.numChannelsInHistory) < ART_ANI_CHANNEL_HISTORY_SIZE); 
	for (ii = 0; ii < pLibDev->artAniSetup.numChannelsInHistory; ii++) {
		if (curr_freq == pLibDev->artAniSetup.chanListHistory[ii]) 
			return(TRUE);
	}
	return(FALSE);
}

void artAniRecallLevels
(
 A_UINT32 devNum, 
 A_UINT32 curr_freq
)
{
	A_UINT32 ii;
	A_UINT32 chIndex = ART_ANI_CHANNEL_HISTORY_SIZE;
	LIB_DEV_INFO *pLibDev;

	pLibDev = gLibInfo.pLibDevArray[devNum];
	assert(pLibDev != NULL);
	
	assert((pLibDev->artAniSetup.numChannelsInHistory) < ART_ANI_CHANNEL_HISTORY_SIZE); 
	for (ii = 0; ii < pLibDev->artAniSetup.numChannelsInHistory; ii++) {
		if (curr_freq == pLibDev->artAniSetup.chanListHistory[ii]) {
			chIndex = ii;
			break;
		}
	}

	// this routine should not have been called unless curr_freq existed in history
	assert(chIndex < ART_ANI_CHANNEL_HISTORY_SIZE);
	pLibDev->artAniSetup.currNILevel = pLibDev->artAniSetup.niLevelHistory[chIndex];
	pLibDev->artAniSetup.currBILevel = pLibDev->artAniSetup.biLevelHistory[chIndex];
	pLibDev->artAniSetup.currSILevel = pLibDev->artAniSetup.siLevelHistory[chIndex];
}

void artAniOptimizeLevels
(
 A_UINT32 devNum, 
 A_UINT32 curr_freq
)
{
	LIB_DEV_INFO *pLibDev;	

	pLibDev = gLibInfo.pLibDevArray[devNum];
	assert(pLibDev != NULL);
	// start with min NI/BI/SI
	ar5kInitData[pLibDev->ar5kInitIndex].pArtAniAPI->setArtAniLevelMin(devNum, ART_ANI_TYPE_NI);
	ar5kInitData[pLibDev->ar5kInitIndex].pArtAniAPI->setArtAniLevelMin(devNum, ART_ANI_TYPE_BI);
	ar5kInitData[pLibDev->ar5kInitIndex].pArtAniAPI->setArtAniLevelMin(devNum, ART_ANI_TYPE_SI);
	// measure false detects
	artAniMeasureFalseDetects(devNum, ART_ANI_FALSE_DETECT_OFDM_AND_CCK);
	//ar5kInitData[pLibDev->ar5kInitIndex].pArtAniAPI->getArtAniFalseDetects(devNum, ART_ANI_FALSE_DETECT_OFDM_AND_CCK);

	// see if any optimization needed
	if (artAniOptimized(devNum, ART_ANI_FALSE_DETECT_OFDM_AND_CCK)) {
		artAniUpdateLevels(devNum);
		return;
	}
	// see if NI helps
	ar5kInitData[pLibDev->ar5kInitIndex].pArtAniAPI->setArtAniLevelMax(devNum, ART_ANI_TYPE_NI);
	if (artAniOptimized(devNum, ART_ANI_FALSE_DETECT_OFDM_AND_CCK)) {
		artAniFindMinLevel(devNum, ART_ANI_TYPE_NI);
		artAniUpdateLevels(devNum);
		return;
	} else {
		if (pLibDev->mode == MODE_11B) {
			// see if BI helps
			ar5kInitData[pLibDev->ar5kInitIndex].pArtAniAPI->setArtAniLevelMax(devNum, ART_ANI_TYPE_BI);
			if (artAniOptimized(devNum, ART_ANI_FALSE_DETECT_OFDM_AND_CCK)) {
				artAniFindMinLevel(devNum, ART_ANI_TYPE_BI);
				artAniFindMinLevel(devNum, ART_ANI_TYPE_NI);
				artAniUpdateLevels(devNum);
				return;
			} else {
				ar5kInitData[pLibDev->ar5kInitIndex].pArtAniAPI->setArtAniLevelMin(devNum, ART_ANI_TYPE_BI);
				ar5kInitData[pLibDev->ar5kInitIndex].pArtAniAPI->setArtAniLevelMin(devNum, ART_ANI_TYPE_NI);
			}
		} else {  // running with OFDM
			// see if SI helps
			ar5kInitData[pLibDev->ar5kInitIndex].pArtAniAPI->setArtAniLevelMax(devNum, ART_ANI_TYPE_SI);
			if (artAniOptimized(devNum, ART_ANI_FALSE_DETECT_OFDM_AND_CCK)) {
				artAniFindMinLevel(devNum, ART_ANI_TYPE_SI);
				artAniFindMinLevel(devNum, ART_ANI_TYPE_NI);
				artAniUpdateLevels(devNum);
				return;
			} else {
				ar5kInitData[pLibDev->ar5kInitIndex].pArtAniAPI->setArtAniLevelMin(devNum, ART_ANI_TYPE_SI);
				ar5kInitData[pLibDev->ar5kInitIndex].pArtAniAPI->setArtAniLevelMin(devNum, ART_ANI_TYPE_NI);
			}
		}
	}
	artAniUpdateLevels(devNum);

	// to quiet warnings
	curr_freq = 0; 
}

void artAniProgramCurrLevels
(
 A_UINT32 devNum
)
{
	LIB_DEV_INFO *pLibDev;

	pLibDev = gLibInfo.pLibDevArray[devNum];
	assert(pLibDev != NULL);
	
    ar5kInitData[pLibDev->ar5kInitIndex].pArtAniAPI->setArtAniLevel(devNum, ART_ANI_TYPE_NI, pLibDev->artAniSetup.currNILevel);
	ar5kInitData[pLibDev->ar5kInitIndex].pArtAniAPI->programCurrArtAniLevel(devNum, ART_ANI_TYPE_NI);

	ar5kInitData[pLibDev->ar5kInitIndex].pArtAniAPI->setArtAniLevel(devNum, ART_ANI_TYPE_BI, pLibDev->artAniSetup.currBILevel);
	ar5kInitData[pLibDev->ar5kInitIndex].pArtAniAPI->programCurrArtAniLevel(devNum, ART_ANI_TYPE_BI);

	ar5kInitData[pLibDev->ar5kInitIndex].pArtAniAPI->setArtAniLevel(devNum, ART_ANI_TYPE_SI, pLibDev->artAniSetup.currSILevel);
	ar5kInitData[pLibDev->ar5kInitIndex].pArtAniAPI->programCurrArtAniLevel(devNum, ART_ANI_TYPE_SI);
}

void artAniMeasureFalseDetects
(
 A_UINT32 devNum, 
 A_UINT32 artAniFalseDetectType
)
{
	LIB_DEV_INFO *pLibDev;

	pLibDev = gLibInfo.pLibDevArray[devNum];
	assert(pLibDev != NULL);

	ar5kInitData[pLibDev->ar5kInitIndex].pArtAniAPI->measArtAniFalseDetects(devNum, artAniFalseDetectType);
}

// make a frsh measurement and find out if the false detect criterion is met
A_BOOL artAniOptimized
(
 A_UINT32 devNum,
 A_UINT32 artAniFalseDetectType
)
{
	LIB_DEV_INFO *pLibDev;

	pLibDev = gLibInfo.pLibDevArray[devNum];
	assert(pLibDev != NULL);

	ar5kInitData[pLibDev->ar5kInitIndex].pArtAniAPI->measArtAniFalseDetects(devNum, artAniFalseDetectType);
	return(ar5kInitData[pLibDev->ar5kInitIndex].pArtAniAPI->isArtAniOptimized(devNum, artAniFalseDetectType));
}

// find min level for immunity that passes false detect criteria
void artAniFindMinLevel
(
 A_UINT32 devNum, 
 A_UINT32 artAniType
)
{
	A_BOOL   prev_state;
	A_BOOL   curr_state;
	LIB_DEV_INFO *pLibDev;

	pLibDev = gLibInfo.pLibDevArray[devNum];
	assert(pLibDev != NULL);

	prev_state = artAniOptimized(devNum, ART_ANI_FALSE_DETECT_OFDM_AND_CCK);

	// better start optimizing from a passing state
	if(!prev_state)
		return;

	while (ar5kInitData[pLibDev->ar5kInitIndex].pArtAniAPI->decrementArtAniLevel(devNum, artAniType)) {
		curr_state = artAniOptimized(devNum, ART_ANI_FALSE_DETECT_OFDM_AND_CCK);
		if (prev_state && !curr_state) {
			ar5kInitData[pLibDev->ar5kInitIndex].pArtAniAPI->incrementArtAniLevel(devNum, artAniType);
			return;
		} else {
			prev_state = curr_state;
		}
	}
}

void artAniUpdateLevels
(
 A_UINT32 devNum
)
{
	LIB_DEV_INFO *pLibDev;

	pLibDev = gLibInfo.pLibDevArray[devNum];
	assert(pLibDev != NULL);

    pLibDev->artAniSetup.currNILevel = ar5kInitData[pLibDev->ar5kInitIndex].pArtAniAPI->getArtAniLevel(devNum, ART_ANI_TYPE_NI);
	pLibDev->artAniSetup.currBILevel = ar5kInitData[pLibDev->ar5kInitIndex].pArtAniAPI->getArtAniLevel(devNum, ART_ANI_TYPE_BI);
	pLibDev->artAniSetup.currSILevel = ar5kInitData[pLibDev->ar5kInitIndex].pArtAniAPI->getArtAniLevel(devNum, ART_ANI_TYPE_SI);

	artAniUpdateHistory(devNum);
}

void artAniUpdateHistory
(
 A_UINT32 devNum
)
{
	A_UINT32 ii;
	A_UINT32 curr_freq;
	A_UINT32 chIndex = ART_ANI_CHANNEL_HISTORY_SIZE;
	LIB_DEV_INFO *pLibDev;

	pLibDev = gLibInfo.pLibDevArray[devNum];
	assert(pLibDev != NULL);
	
	curr_freq = pLibDev->freqForResetDevice;

	assert((pLibDev->artAniSetup.numChannelsInHistory) < ART_ANI_CHANNEL_HISTORY_SIZE); 
	for (ii = 0; ii < pLibDev->artAniSetup.numChannelsInHistory; ii++) {
		if (curr_freq == pLibDev->artAniSetup.chanListHistory[ii]) {
			chIndex = ii;
			break;
		}
	}

	assert(chIndex <= ART_ANI_CHANNEL_HISTORY_SIZE);

	if (chIndex == ART_ANI_CHANNEL_HISTORY_SIZE) { // history doesn't exist for this chan
		chIndex = pLibDev->artAniSetup.numChannelsInHistory;
		pLibDev->artAniSetup.numChannelsInHistory++;
	}

	
	pLibDev->artAniSetup.chanListHistory[chIndex] = curr_freq;

	pLibDev->artAniSetup.niLevelHistory[chIndex] = pLibDev->artAniSetup.currNILevel;
	pLibDev->artAniSetup.biLevelHistory[chIndex] = pLibDev->artAniSetup.currBILevel;
	pLibDev->artAniSetup.siLevelHistory[chIndex] = pLibDev->artAniSetup.currSILevel;
}

MANLIB_API A_UINT32 getArtAniLevel
(
 A_UINT32 devNum,
 A_UINT32 artAniType
)
{
	LIB_DEV_INFO *pLibDev;

	pLibDev = gLibInfo.pLibDevArray[devNum];
	assert(pLibDev != NULL);

    return(pLibDev->artAniSetup.currNILevel = ar5kInitData[pLibDev->ar5kInitIndex].pArtAniAPI->getArtAniLevel(devNum, artAniType));
}


MANLIB_API void setArtAniLevel
(
 A_UINT32 devNum,
 A_UINT32 artAniType,
 A_UINT32 artAniLevel
)
{
	LIB_DEV_INFO *pLibDev;

	pLibDev = gLibInfo.pLibDevArray[devNum];
	assert(pLibDev != NULL);

    pLibDev->artAniSetup.currNILevel = ar5kInitData[pLibDev->ar5kInitIndex].pArtAniAPI->setArtAniLevel(devNum, artAniType, artAniLevel);
}



