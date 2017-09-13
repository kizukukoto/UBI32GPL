/*
 *  Copyright © 2002 Atheros Communications, Inc.,  All Rights Reserved.
 *
 *  mcfg(5)211.h - Type definitions needed for device specific functions 
 */

#ifndef	_MCFG211_H
#define	_MCFG211_H

#ident  "ACI $Id: //depot/sw/src/dk/mdk/devlib/ar5211/mCfg211.h#10 $, $Header: //depot/sw/src/dk/mdk/devlib/ar5211/mCfg211.h#10 $"

typedef struct freqMap {
	A_UINT16	requiredFreq;
	A_UINT16	sombreroChannelSel;
	A_CHAR		sombreroRefClk;
} FREQ_MAP;

void hwResetAr5211
(
 A_UINT32 devNum,
 A_UINT32 resetMask
);

void pllProgramAr5211
(
 A_UINT32 devNum,
 A_UINT32 turbo
);

A_BOOL setChannelAr5211
(
 A_UINT32 devNum,
 A_UINT32 freq		// New channel
);

A_BOOL setChannelAr5211_beanie
(
 A_UINT32 devNum,
 A_UINT32 freq		// New channel
);

void setSinglePowerAr5211
(
 A_UINT32 devNum, 
 A_UCHAR pcdac
);

void initPowerAr5211
(
	A_UINT32 devNum,
	A_UINT32 freq,
	A_UINT32  override,
	A_UCHAR  *pwrSettings
);


A_UINT32 eepromReadAr5211
(
 A_UINT32 devNum, 
 A_UINT32 eepromOffset
);

void eepromWriteAr5211
(
 A_UINT32 devNum, 
 A_UINT32 eepromOffset, 
 A_UINT32 eepromValue
);

A_BOOL attemptOtherChannel
(
 A_UINT32 devNum,
 A_UINT32 freq		
);

#endif //_MCFG211_H

