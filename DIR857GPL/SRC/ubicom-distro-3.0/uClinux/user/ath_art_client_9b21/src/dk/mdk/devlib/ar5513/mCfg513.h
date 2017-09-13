/*
 *  Copyright © 2003 Atheros Communications, Inc.,  All Rights Reserved.
 *
 *  mcfg(5)513.h - Type definitions needed for device specific functions 
 */

#ifndef	_MCFG513_H
#define	_MCFG513_H

#ident  "ACI $Id: //depot/sw/src/dk/mdk/devlib/ar5513/mCfg513.h#1 $, $Header: //depot/sw/src/dk/mdk/devlib/ar5513/mCfg513.h#1 $"

A_BOOL setChannelAr5513
(
 A_UINT32 devNum,
 A_UINT32 freq		// New channel
);

A_BOOL eepromAutoSizeAr5513
(
 A_UINT32 devNum
);

// mapped transparently to flash or eeprom
A_UINT32 eepromReadAr5513
(
 A_UINT32 devNum, 
 A_UINT32 eepromOffset
);

void flashEraseSectorAr5513
(
 A_UINT32 devNum,
 A_UINT32 sector
);

// mapped transparently to flash or eeprom
void eepromWriteAr5513
(
 A_UINT32 devNum, 
 A_UINT32 eepromOffset, 
 A_UINT32 eepromValue
);

void hwResetAr5513
(
 	A_UINT32 devNum,
 	A_UINT32 resetMask
);

void pllProgramAr5513
(
 	A_UINT32 devNum,
 	A_UINT32 turbo
);

void hwNewResetAr5513
(
 	A_UINT32 devNum,
 	A_UINT32 resetMask
);

//A_BOOL flashReadSectorAr5513
//(
// A_UINT32 devNum, 
// A_UINT32 sectorNum,
// A_UINT32 *retList,
// A_UINT32 retListSize
//);


//A_BOOL flashWriteSectorAr5513
//(
// A_UINT32 devNum, 
// A_UINT32 sectorNum,
// A_UINT32 *retList,
// A_UINT32 retListSize
//);

// mapped to spi eeprom only (no flash)
void spiEepromWriteAr5513
(
 A_UINT32 devNum, 
 A_UINT32 eepromOffset, 
 A_UINT32 eepromValue
);

// mapped to spi eeprom only (no flash)
A_UINT32 spiEepromReadAr5513
(
 A_UINT32 devNum, 
 A_UINT32 eepromOffset
);

#endif //_MCFG513_H
