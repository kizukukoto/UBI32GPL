/*
 * Copyright (c) 2002-2006 Atheros Communications, Inc.
 * All rights reserved.
 *
 * $Id: //depot/sw/src/dk/mdk/devlib/ar9285/mEep9285.h#1 $
 */
#ifndef _AR9285_EEPROM_H_
#define _AR9285_EEPROM_H_

extern A_BOOL
ar9285EepromAttach(A_UINT32 devNum);

extern void
ar9285EepromSetBoardValues(A_UINT32 devNum, A_UINT32 freq);

extern void
ar9285EepromSetTransmitPower(A_UINT32 devNum, A_UINT32 freq, A_UINT16 *pRates);

extern void
ar9285GetPowerPerRateTable(A_UINT32 devNum, A_UINT16 freq, A_INT16 *pRatesPower);
 
extern A_INT16 
get9285MaxPowerForRate(A_UINT32 devNum, A_UINT32 freq, A_UINT32 rate);

#endif  /* _AR9285_EEPROM_H_ */
