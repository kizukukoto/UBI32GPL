/*
 *  Copyright © 2003 Atheros Communications, Inc.,  All Rights Reserved.
 *
 *  mcfg(5)212.h - Type definitions needed for device specific functions 
 */

#ifndef	_MCFG413_H
#define	_MCFG413_H

#ident  "ACI $Id: //depot/sw/src/dk/mdk/devlib/ar2413/mCfg413.h#4 $, $Header: //depot/sw/src/dk/mdk/devlib/ar2413/mCfg413.h#4 $"

A_BOOL setChannelAr2413
(
 A_UINT32 devNum,
 A_UINT32 freq		// New channel
);

void initPowerAr2413
(
	A_UINT32 devNum,
	A_UINT32 freq,
	A_UINT32  override,
	A_UCHAR  *pwrSettings
);

void pllProgramAr5413
(
 	A_UINT32 devNum,
 	A_UINT32 turbo
);

#endif //_MCFG413_H
