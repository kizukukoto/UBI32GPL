/*
 *  Copyright © 2003 Atheros Communications, Inc.,  All Rights Reserved.
 *
 */


#ifdef VXWORKS
#include "vxworks.h"
#endif

#ifdef __ATH_DJGPPDOS__
#define __int64	long long
typedef unsigned long DWORD;
#define Sleep	delay
#endif	// #ifdef __ATH_DJGPPDOS__

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#ifndef VXWORKS
#include <malloc.h>
#endif
#include "wlantype.h"
//#include "wlanPhy.h"

#include "mAni212.h"

#ifdef Linux
#include "../athreg.h"
#include "../manlib.h"
#include "../mEeprom.h"
#include "../mConfig.h"
//#include "../../devmld/art_if.h"
#else
#include "..\athreg.h"
#include "..\manlib.h"
#include "..\mEeprom.h"
#include "..\mConfig.h"
//#include "..\..\devmld\art_if.h"
#endif

#include "ar5211reg.h"
#include <string.h>

#include "art_ani.h"

// noise immunity params
A_CHAR niParams[][122] = {"bb_total_desired", "bb_coarse_high", "bb_coarse_low", "bb_firpwr"};
// barker immunity params
A_CHAR biParams[][122] = {"bb_weak_sig_thr_cck", "bb_firstep"};
// spur immunity params
A_CHAR siParams[][122] = {"bb_cycpwr_thr1", "bb_m1_thres", "bb_m2_thres", "bb_m1_thresh_low",
					  "bb_m2_thresh_low", "bb_m2count_thr", "bb_m2count_thr_low"};

static A_BOOL  artAniLadderConfigured[3] = {FALSE, FALSE, FALSE};

ART_ANI_LADDER niLadderAr5212 = { 5, //numLevelsInLadder
					4, //numParamsInLevel
					0, //defaultLevelNum
					0, //currLevelNum
					0, //currFD
					(A_UINT32)(ART_ANI_OFDM_TRIGGER_COUNT_LOW*1000/ART_ANI_MONITOR_PERIOD), // loTrig
					(A_UINT32)(ART_ANI_OFDM_TRIGGER_COUNT_HIGH*1000/ART_ANI_MONITOR_PERIOD), // hiTrig
					FALSE, //active
//					niParams, //paramName
					{"bb_total_desired", "bb_coarse_high", "bb_coarse_low", "bb_firpwr"},
					{ { {-34, -18, -52, -70},  "NI0"},
					  { {-41, -18, -56, -72},  "NI1"},
					  { {-48, -16, -60, -75},  "NI2"},
					  { {-55, -14, -64, -78},  "NI3"},
					  { {-62, -12, -70, -80},  "NI4"}
					}
} ; 

ART_ANI_LADDER biLadderAr5212 = { 3, //numLevelsInLadder
					2, //numParamsInLevel
					0, //defaultLevelNum
					0, //currLevelNum
					0, //currFD
					(A_UINT32)(ART_ANI_CCK_TRIGGER_COUNT_LOW*1000/ART_ANI_MONITOR_PERIOD), // loTrig
					(A_UINT32)(ART_ANI_CCK_TRIGGER_COUNT_HIGH*1000/ART_ANI_MONITOR_PERIOD), // hiTrig
					FALSE, //active
//					&(biParams[0]), //paramName
					{"bb_weak_sig_thr_cck", "bb_firstep"},
					{ { {8, 0},  "BI0"},
					  { {6, 4},  "BI1"},
					  { {6, 8},  "BI2"}
					}
} ; 

ART_ANI_LADDER siLadderAr5212 = { 2, //numLevelsInLadder
					7, //numParamsInLevel
					0, //defaultLevelNum
					0, //currLevelNum
					0, //currFD
					(A_UINT32)(ART_ANI_OFDM_TRIGGER_COUNT_LOW*1000/ART_ANI_MONITOR_PERIOD), // loTrig
					(A_UINT32)(ART_ANI_OFDM_TRIGGER_COUNT_HIGH*1000/ART_ANI_MONITOR_PERIOD), // hiTrig
					FALSE, //active
//					&(siParams[0]), //paramName
					{"bb_cycpwr_thr1", "bb_m1_thres", "bb_m2_thres", "bb_m1_thresh_low",
					  "bb_m2_thresh_low", "bb_m2count_thr", "bb_m2count_thr_low"},
					{ { {2, 127, 127, 127, 127, 31, 63},  "SI0"},
					  { {2, 0x4d, 0x40, 50, 40, 16, 48},  "SI1"}
					}
} ; 


A_BOOL configArtAniLadderAr5212
(
 A_UINT32 devNum,
 A_UINT32 artAniType		// NI/BI/SI
)
{
	LIB_DEV_INFO *pLibDev;
	ART_ANI_LADDER  *pSrcArtAniLadder;
	A_UINT32  ii, jj, kk;
	ART_ANI_SETUP *pArtAniSetup;

	pLibDev = gLibInfo.pLibDevArray[devNum];
	assert(pLibDev != NULL);

	pArtAniSetup = &(pLibDev->artAniSetup);
	assert(pArtAniSetup != NULL);
	
	assert((artAniType < 3) && (artAniType >= 0));

	switch (artAniType) {
	case ART_ANI_TYPE_NI :
		pSrcArtAniLadder = &(niLadderAr5212);
		break;
	case ART_ANI_TYPE_BI :
		pSrcArtAniLadder = &(biLadderAr5212);
		break;

	case ART_ANI_TYPE_SI :
		pSrcArtAniLadder = &(siLadderAr5212);
		break;
	default :
		return(FALSE);
	}

	pLibDev->artAniLadder[artAniType].numLevelsInLadder = pSrcArtAniLadder->numLevelsInLadder;
	pLibDev->artAniLadder[artAniType].numParamsInLevel	= pSrcArtAniLadder->numParamsInLevel;
	pLibDev->artAniLadder[artAniType].currLevelNum	    = pSrcArtAniLadder->currLevelNum;
	pLibDev->artAniLadder[artAniType].defaultLevelNum   = pSrcArtAniLadder->defaultLevelNum;
	pLibDev->artAniLadder[artAniType].currFD			= pSrcArtAniLadder->currFD;
	pLibDev->artAniLadder[artAniType].loTrig			= pSrcArtAniLadder->loTrig;
	pLibDev->artAniLadder[artAniType].hiTrig			= pSrcArtAniLadder->hiTrig;
	pLibDev->artAniLadder[artAniType].active			= pArtAniSetup->Enabled;

//	pLibDev->artAniLadder[artAniType].optLevel			= pSrcArtAniLadder->optLevel;

	pLibDev->artAniLadder[artAniType].currLevel			= &(pLibDev->artAniLadder[artAniType].optLevel[pLibDev->artAniLadder[artAniType].currLevelNum]);

//	pLibDev->artAniLadder[artAniType].paramName			= &(niParams[0]);
	jj = 0;
	while (jj < pSrcArtAniLadder->numParamsInLevel) {

		ii = 0;
		while (pSrcArtAniLadder->paramName[jj][ii] != '\0') {
			pLibDev->artAniLadder[artAniType].paramName[jj][ii] = pSrcArtAniLadder->paramName[jj][ii];
			ii++;
		}
		pLibDev->artAniLadder[artAniType].paramName[jj][ii] = '\0';

		ii = 0;
		while (ii < pSrcArtAniLadder->numLevelsInLadder) {
			pLibDev->artAniLadder[artAniType].optLevel[ii].paramVal[jj] = pSrcArtAniLadder->optLevel[ii].paramVal[jj];
			kk = 0;
			while(pSrcArtAniLadder->optLevel[ii].levelName[kk] != '\0') {
				pLibDev->artAniLadder[artAniType].optLevel[ii].levelName[kk] = pSrcArtAniLadder->optLevel[ii].levelName[kk];
				kk++;
			}
			pLibDev->artAniLadder[artAniType].optLevel[ii].levelName[kk] = '\0';
			ii++;
		}

		jj++;
	}

	return(TRUE);
}

A_BOOL	enableArtAniAr5212
(A_UINT32 devNum, 
 A_UINT32 artAniType
)
{
	if (!artAniLadderConfigured[artAniType]) {
		artAniLadderConfigured[artAniType] = TRUE;
		configArtAniLadderAr5212(devNum, artAniType);
	}
	gLibInfo.pLibDevArray[devNum]->artAniLadder[artAniType].active = TRUE;
	return(TRUE);
}

A_BOOL	disableArtAniAr5212
(A_UINT32 devNum, 
 A_UINT32 artAniType
)
{
	gLibInfo.pLibDevArray[devNum]->artAniLadder[artAniType].active = FALSE;
	return(TRUE);
}

A_BOOL	setArtAniLevelAr5212
(A_UINT32 devNum, 
 A_UINT32 artAniType, 
 A_UINT32 artAniLevel
)
{
	ART_ANI_LADDER *pArtAniLadder;

	pArtAniLadder = &(gLibInfo.pLibDevArray[devNum]->artAniLadder[artAniType]);

	if (!pArtAniLadder->active) 
		return (TRUE);

//Sleep(1000);
	pArtAniLadder->currLevelNum = artAniLevel;
	pArtAniLadder->currLevel = &(pArtAniLadder->optLevel[pArtAniLadder->currLevelNum]);

	programCurrArtAniLevelAr5212(devNum, artAniType);
	
	return (TRUE);
}


A_UINT32 getArtAniLevelAr5212
(A_UINT32 devNum, 
 A_UINT32 artAniType
)
{
	ART_ANI_LADDER *pArtAniLadder;

	pArtAniLadder = &(gLibInfo.pLibDevArray[devNum]->artAniLadder[artAniType]);
	return(pArtAniLadder->currLevelNum);
}

A_BOOL	setArtAniLevelMaxAr5212
(A_UINT32 devNum, 
 A_UINT32 artAniType
)
{
	ART_ANI_LADDER *pArtAniLadder;

	pArtAniLadder = &(gLibInfo.pLibDevArray[devNum]->artAniLadder[artAniType]);

	if (!pArtAniLadder->active) 
		return (TRUE);

	pArtAniLadder->currLevelNum = (pArtAniLadder->numLevelsInLadder - 1);
	pArtAniLadder->currLevel = &(pArtAniLadder->optLevel[pArtAniLadder->currLevelNum]);

	programCurrArtAniLevelAr5212(devNum, artAniType);
	
	return (TRUE);
}

A_BOOL	setArtAniLevelMinAr5212
(A_UINT32 devNum, 
 A_UINT32 artAniType
)
{
	ART_ANI_LADDER *pArtAniLadder;

	pArtAniLadder = &(gLibInfo.pLibDevArray[devNum]->artAniLadder[artAniType]);

	if (!pArtAniLadder->active) 
		return (TRUE);
//Sleep(1000);
	pArtAniLadder->currLevelNum = 0;
	pArtAniLadder->currLevel = &(pArtAniLadder->optLevel[pArtAniLadder->currLevelNum]);

	programCurrArtAniLevelAr5212(devNum, artAniType);
	
	return (TRUE);
}

A_BOOL	incrementArtAniLevelAr5212
(A_UINT32 devNum, 
 A_UINT32 artAniType
)
{
	ART_ANI_LADDER *pArtAniLadder;
	A_BOOL         retVal = TRUE;

	pArtAniLadder = &(gLibInfo.pLibDevArray[devNum]->artAniLadder[artAniType]);

	if (!pArtAniLadder->active) 
		return (TRUE);

	assert(pArtAniLadder->currLevelNum < pArtAniLadder->numLevelsInLadder);

	pArtAniLadder->currLevelNum++;
	if (pArtAniLadder->currLevelNum == pArtAniLadder->numLevelsInLadder) {
		pArtAniLadder->currLevelNum = (pArtAniLadder->numLevelsInLadder - 1);
		retVal = FALSE;
	}
	pArtAniLadder->currLevel = &(pArtAniLadder->optLevel[pArtAniLadder->currLevelNum]);

	programCurrArtAniLevelAr5212(devNum, artAniType);
	
	return (retVal);
}

A_BOOL	decrementArtAniLevelAr5212
(A_UINT32 devNum, 
 A_UINT32 artAniType
)
{
	ART_ANI_LADDER *pArtAniLadder;
	A_BOOL         retVal = TRUE;

	pArtAniLadder = &(gLibInfo.pLibDevArray[devNum]->artAniLadder[artAniType]);

	if (!pArtAniLadder->active) 
		return (TRUE);

	assert(pArtAniLadder->currLevelNum < pArtAniLadder->numLevelsInLadder);

	if (pArtAniLadder->currLevelNum > 0) {
		pArtAniLadder->currLevelNum--;
	} else {
		pArtAniLadder->currLevelNum = 0;
		retVal = FALSE;
	}

	pArtAniLadder->currLevel = &(pArtAniLadder->optLevel[pArtAniLadder->currLevelNum]);

	programCurrArtAniLevelAr5212(devNum, artAniType);
	
	return (retVal);
}

A_BOOL	programCurrArtAniLevelAr5212
(A_UINT32 devNum, 
 A_UINT32 artAniType
)
{
	ART_ANI_LADDER *pArtAniLadder;
	A_UINT32 ii;

	pArtAniLadder = &(gLibInfo.pLibDevArray[devNum]->artAniLadder[artAniType]);

	if (!pArtAniLadder->active) 
		return (TRUE);

	assert(pArtAniLadder->currLevelNum < pArtAniLadder->numLevelsInLadder);

//	printf("SNOOP: setting ANI type %d, level %d, numParamsinLevel = %d\n", artAniType, pArtAniLadder->currLevelNum, pArtAniLadder->numParamsInLevel);
//Sleep(1000);

	for (ii = 0; ii < pArtAniLadder->numParamsInLevel; ii++) {
		writeField(devNum, pArtAniLadder->paramName[ii], pArtAniLadder->currLevel->paramVal[ii]);
	}

	return(TRUE);
}

A_UINT32 getArtAniFalseDetectsAr5212
(
 A_UINT32 devNum, 
 A_UINT32 artAniFalseDetectType   // ofdm or cck
)
{
	LIB_DEV_INFO *pLibDev;

	pLibDev = gLibInfo.pLibDevArray[devNum];
	assert(pLibDev != NULL);

	pLibDev->artAniSetup.numOfdmFD = gLibInfo.pLibDevArray[devNum]->artAniLadder[ART_ANI_TYPE_NI].currFD;
	pLibDev->artAniSetup.numCckFD  = gLibInfo.pLibDevArray[devNum]->artAniLadder[ART_ANI_TYPE_BI].currFD;
	
	switch (artAniFalseDetectType) {
	case ART_ANI_FALSE_DETECT_OFDM :
		return(pLibDev->artAniSetup.numOfdmFD);
	case ART_ANI_FALSE_DETECT_CCK :
		return(pLibDev->artAniSetup.numCckFD);
	case ART_ANI_FALSE_DETECT_OFDM_AND_CCK :
	default :
		return(pLibDev->artAniSetup.numOfdmFD);
	}
}

A_BOOL measArtAniFalseDetectsAr5212
(
 A_UINT32 devNum, 
 A_UINT32 artAniFalseDetectType   // ofdm or cck
)
{
	A_UINT32 ofdm_fd = 0, cck_fd = 0;
	LIB_DEV_INFO *pLibDev;

	pLibDev = gLibInfo.pLibDevArray[devNum];
	assert(pLibDev != NULL);

	if (!gLibInfo.pLibDevArray[devNum]->artAniLadder[ART_ANI_TYPE_NI].active) 
		return (TRUE);

	// do the measurement
	switch (artAniFalseDetectType) {
	case ART_ANI_FALSE_DETECT_OFDM :
		//measure ofdm fd
		gLibInfo.pLibDevArray[devNum]->artAniLadder[ART_ANI_TYPE_NI].currFD = ofdm_fd;
		gLibInfo.pLibDevArray[devNum]->artAniLadder[ART_ANI_TYPE_SI].currFD = ofdm_fd;
		pLibDev->artAniSetup.numOfdmFD = gLibInfo.pLibDevArray[devNum]->artAniLadder[ART_ANI_TYPE_NI].currFD;
		break;
	case ART_ANI_FALSE_DETECT_CCK :
		//measure cck fd
		gLibInfo.pLibDevArray[devNum]->artAniLadder[ART_ANI_TYPE_BI].currFD = cck_fd;
		pLibDev->artAniSetup.numCckFD  = gLibInfo.pLibDevArray[devNum]->artAniLadder[ART_ANI_TYPE_BI].currFD;
		break;
	case ART_ANI_FALSE_DETECT_OFDM_AND_CCK :
	default :
		// measure both fd
		gLibInfo.pLibDevArray[devNum]->artAniLadder[ART_ANI_TYPE_NI].currFD = ofdm_fd;
		gLibInfo.pLibDevArray[devNum]->artAniLadder[ART_ANI_TYPE_SI].currFD = ofdm_fd;
		gLibInfo.pLibDevArray[devNum]->artAniLadder[ART_ANI_TYPE_BI].currFD = cck_fd;
		pLibDev->artAniSetup.numOfdmFD = gLibInfo.pLibDevArray[devNum]->artAniLadder[ART_ANI_TYPE_NI].currFD;
		pLibDev->artAniSetup.numCckFD  = gLibInfo.pLibDevArray[devNum]->artAniLadder[ART_ANI_TYPE_BI].currFD;
		break;
	}
	
	return (TRUE);
}

A_BOOL isArtAniOptimizedAr5212
(
 A_UINT32 devNum, 
 A_UINT32 artAniFalseDetectType   // ofdm or cck
)
{
	A_UINT32 ofdm_fd = 0, cck_fd = 0;
	LIB_DEV_INFO *pLibDev;

	pLibDev = gLibInfo.pLibDevArray[devNum];
	assert(pLibDev != NULL);

	if (!gLibInfo.pLibDevArray[devNum]->artAniLadder[ART_ANI_TYPE_NI].active) 
		return (TRUE);

	ofdm_fd = gLibInfo.pLibDevArray[devNum]->artAniLadder[ART_ANI_TYPE_NI].currFD;
	cck_fd  = gLibInfo.pLibDevArray[devNum]->artAniLadder[ART_ANI_TYPE_BI].currFD;

	switch (artAniFalseDetectType) {
	case ART_ANI_FALSE_DETECT_OFDM :
		if (ofdm_fd < gLibInfo.pLibDevArray[devNum]->artAniLadder[ART_ANI_TYPE_NI].loTrig) 
			return (TRUE);
		break;
	case ART_ANI_FALSE_DETECT_CCK :
		if (cck_fd < gLibInfo.pLibDevArray[devNum]->artAniLadder[ART_ANI_TYPE_BI].loTrig) 
			return (TRUE);
		break;
	case ART_ANI_FALSE_DETECT_OFDM_AND_CCK :
	default :
		if ((ofdm_fd < gLibInfo.pLibDevArray[devNum]->artAniLadder[ART_ANI_TYPE_NI].loTrig) &&
			(cck_fd < gLibInfo.pLibDevArray[devNum]->artAniLadder[ART_ANI_TYPE_BI].loTrig) )
			return (TRUE);
		break;
	}
	
	return (FALSE);
}

A_BOOL setArtAniFalseDetectIntervalAr5212
(
 A_UINT32 devNum,
 A_UINT32 artAniFalseDetectInterval		// in millisecond
)
{
	gLibInfo.pLibDevArray[devNum]->artAniSetup.monitorPeriod = artAniFalseDetectInterval;
	return(TRUE);
}

