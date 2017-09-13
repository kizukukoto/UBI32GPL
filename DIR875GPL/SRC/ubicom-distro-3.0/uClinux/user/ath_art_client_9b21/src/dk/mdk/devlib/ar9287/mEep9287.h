/** \file
 * COPYRIGHT (C) Atheros Communications Inc., 2009
 *
 * - Filename    : mEep9287.H
 * - Description : public eeprom routines for kiwe chipset
 * - $Id: //depot/sw/src/dk/mdk/devlib/ar9287/mEep9287.h#1 $
 * - $DateTime: 2009/01/21 15:51:11 $
 * - History :
 * - Date      Author     Modification
 ******************************************************************************/
#ifndef mEep9287_H
#define mEep9287_H

/**** Includes ****************************************************************/

/**** Definitions *************************************************************/

/**** Macros ******************************************************************/

/**** Declarations and externals **********************************************/

/**** Public function prototypes **********************************************/

extern A_BOOL
ar9287EepromAttach(A_UINT32 devNum);

extern void
ar9287EepromSetBoardValues(A_UINT32 devNum, A_UINT32 freq);

extern void
ar9287EepromSetTransmitPower(A_UINT32 devNum, A_UINT32 freq, A_UINT16 *pRates);

extern void
ar9287GetPowerPerRateTable(A_UINT32 devNum, A_UINT16 freq, A_INT16 *pRatesPower);
 
extern A_INT16 
get9287MaxPowerForRate(A_UINT32 devNum, A_UINT32 freq, A_UINT32 rate);
#endif  /* mEep9287_H */
