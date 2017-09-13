/*
 * Copyright (c) 2002-2006 Atheros Communications, Inc.
 * All rights reserved.
 *
 * $Id: //depot/sw/src/dk/mdk/devlib/ar5416/mEep5416.h#3 $
 */
#ifndef _AR5416_EEPROM_H_
#define _AR5416_EEPROM_H_

extern A_BOOL
ar5416EepromAttach(A_UINT32 devNum);

extern void
ar5416EepromSetBoardValues(A_UINT32 devNum, A_UINT32 freq);

extern void
ar5416EepromSetTransmitPower(A_UINT32 devNum, A_UINT32 freq, A_UINT16 *pRates);

void
ar5416GetPowerPerRateTable(A_UINT32 devNum, A_UINT16 freq, A_INT16 *pRatesPower);
 
extern A_INT16 
get5416MaxPowerForRate(A_UINT32 devNum, A_UINT32 freq, A_UINT32 rate);

extern void
ar5416OpenLoopPowerControlTempCompensation(A_UINT32 devNum, A_UINT32 initPDADC);

#endif  /* _AR5416_EEPROM_H_ */
