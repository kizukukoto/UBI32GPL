/*
 *  Copyright © 2005 Atheros Communications, Inc.,  All Rights Reserved.
 *
 *  mcfg5416.h - Type definitions needed for device specific functions 
 */

#ifndef	_MCFG5416_H
#define	_MCFG5416_H

#ident  "ACI $Id: //depot/sw/src/dk/mdk/devlib/ar5416/mCfg5416.h#6 $, $Header: //depot/sw/src/dk/mdk/devlib/ar5416/mCfg5416.h#6 $"

void pllProgramAr5416
(
 	A_UINT32 devNum,
 	A_UINT32 turbo
);

void hwResetAr5416
(
 	A_UINT32 devNum,
 	A_UINT32 resetMask
);

A_UINT32 eepromReadAr5416
(
 A_UINT32 devNum, 
 A_UINT32 eepromOffset
);


void eepromWriteAr5416
(
 A_UINT32 devNum, 
 A_UINT32 eepromOffset, 
 A_UINT32 eepromValue
);

A_BOOL setChannelAr5416
(
 A_UINT32 devNum,
 A_UINT32 freq
);

A_BOOL setChannelAr928x(
    A_UINT32 devNum,
    A_UINT32 freq
 );

A_BOOL setChannelSwCfgFilter(
    A_UINT32 devNum,
    A_UINT32 freq
 );

void initPowerAr5416
(
    A_UINT32 devNum,
    A_UINT32 freq,
    A_UINT32  override,
    A_UCHAR  *pwrSettings
);

#endif //_MCFG5416_H
