/*
 *  Copyright © 2002 Atheros Communications, Inc.,  All Rights Reserved.
 *
 *  mcfg(5)210.h - Type definitions needed for device specific functions 
 */

#ifndef	_MCFG210_H
#define	_MCFG210_H

#ident  "ACI $Id: //depot/sw/src/dk/mdk/devlib/ar5210/mCfg210.h#4 $, $Header: //depot/sw/src/dk/mdk/devlib/ar5210/mCfg210.h#4 $"

void hwResetAr5210
(
 A_UINT32 devNum,
 A_UINT32 resetMask
);

void pllProgramAr5210
(
 A_UINT32 devNum,
 A_UINT32 turbo
);

void initPowerAr5210
(
	A_UINT32 devNum,
	A_UINT32 freq,
	A_UINT32  override,
	A_UCHAR  *pwrSettings
);


A_BOOL setChannelAr5210
(
 A_UINT32 devNum,
 A_UINT32 freq		// New channel
);

A_UINT32 eepromReadAr5210
(
 A_UINT32 devNum, 
 A_UINT32 eepromOffset
);

void eepromWriteAr5210
(
 A_UINT32 devNum, 
 A_UINT32 eepromOffset, 
 A_UINT32 eepromValue
);

void setSinglePowerAr5210
(
 A_UINT32 devNum, 
 A_UCHAR pcdac
);

#endif //_MCFG210_H

