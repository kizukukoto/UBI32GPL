/*
 * Copyright (c) 2002-2004 Atheros Communications, Inc.
 * All rights reserved.
 *
 * $Id: //depot/sw/src/dk/mdk/devlib/ar6000/mEep6000.h#4 $
 */
#ifndef _AR6000_EEPROM_H_
#define _AR6000_EEPROM_H_

extern A_BOOL
ar6000EepromAttach(A_UINT32 devNum);

extern void
ar6000EepromSetBoardValues(A_UINT32 devNum);

extern void
ar6000EepromSetTransmitPower(A_UINT32 devNum, A_UINT32 freq, A_UINT16 *pRates);

void
ar6000GetPowerPerRateTable(A_UINT32 devNum, A_UINT16 freq, A_INT16 *pRatesPower);

#endif  /* _AR6000_EEPROM_H_ */
