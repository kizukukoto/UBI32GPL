/*
 *  Copyright © 2003 Atheros Communications, Inc.,  All Rights Reserved.
 *
 */

#ident  "ACI $Id: //depot/sw/src/dk/mdk/devlib/ar5212/mCfg212.c#15 $, $Header: //depot/sw/src/dk/mdk/devlib/ar5212/mCfg212.c#15 $"

#ifdef VXWORKS
#include "vxworks.h"
#endif

#ifdef __ATH_DJGPPDOS__
#define __int64	long long
typedef unsigned long DWORD;
#define Sleep	delay
#endif	// #ifdef __ATH_DJGPPDOS__

#include <errno.h>
#include <stdio.h>
#ifndef VXWORKS
#include <malloc.h>
#endif
#include "wlantype.h"

#include "mCfg212.h"
#include "mCfg211.h"

#ifdef Linux
#include "../athreg.h"
#include "../manlib.h"
#include "../mEeprom.h"
#include "../mConfig.h"
#else
#include "..\athreg.h"
#include "..\manlib.h"
#include "..\mEeprom.h"
#include "..\mConfig.h"
#endif

#if (1)
#ifdef __ATH_DJGPPDOS__
#include "mlibif_dos.h"
#endif //__ATH_DJGPPDOS__
#endif

void pllProgramAr5212
(
 	A_UINT32 devNum,
 	A_UINT32 turbo
)
{
	LIB_DEV_INFO	*pLibDev = gLibInfo.pLibDevArray[devNum];
	A_UINT32 reset;

	reset = 0;

	REGW(devNum, 0xa200, 0);
	if((pLibDev->swDevID == 0xa014)||(pLibDev->swDevID == 0xa016)) { //freedom derby
		if((pLibDev->mode == MODE_11A)||(turbo == TURBO_ENABLE)||(pLibDev->mode == MODE_11O))	{
//			REGW(devNum, 0x987c, 0x04) ;
			if(turbo == HALF_SPEED_MODE) {
				REGW(devNum, 0x987c, 0x15d4) ; 
			}
			else if(turbo == QUARTER_SPEED_MODE) {
				REGW(devNum, 0x987c, 0x16d4) ; 
			}
			else {
				REGW(devNum, 0x987c, 0x14d4) ;
			}
		} else {
//			REGW(devNum, 0x987c, 0xd6) ;
			if(turbo == HALF_SPEED_MODE) {
				REGW(devNum, 0x987c, 0x15d6) ; 
			}
			else if(turbo == QUARTER_SPEED_MODE) {
				REGW(devNum, 0x987c, 0x16d6) ; 
			}
			else {
				REGW(devNum, 0x987c, 0x14d6) ;
			}
		}
		mSleep(2);
	}
	else if(pLibDev->swDevID == 0xa013) { //freedom sombrero/beanie
		if((pLibDev->mode == MODE_11A)||(turbo == TURBO_ENABLE)||(pLibDev->mode == MODE_11O))	{
			REGW(devNum, 0x987c, 0x05) ;
		} else {
			REGW(devNum, 0x987c, 0x4b) ;
		}
		mSleep(2);
	}
	else if((pLibDev->swDevID & 0xff) == 0x0013) {
		if((pLibDev->mode == MODE_11A)||(turbo == TURBO_ENABLE)||(pLibDev->mode == MODE_11O))	{
			if(turbo == HALF_SPEED_MODE) {
				REGW(devNum, 0x987c, 0x8a) ; // clk_ref * 10 / 4
			}
			else if(turbo == QUARTER_SPEED_MODE) {
				REGW(devNum, 0x987c, 0x2aa) ; // clk_ref * 20 / 4 /4
			}
			else {
				REGW(devNum, 0x987c, 0xaa) ; // clk_ref * 20 / 4
			}
		}
		else {
			if(turbo == HALF_SPEED_MODE) {
				REGW(devNum, 0x987c, 0x8b); // clk_ref * 11 / 4
			} else if(turbo == QUARTER_SPEED_MODE) {
				REGW(devNum, 0x987c, 0x2ab); // clk_ref * 22 / 4 / 4
			} else {
				REGW(devNum, 0x987c, 0xab); // clk_ref * 22 / 4
			}
		} 		
	} 
	else if(((pLibDev->swDevID & 0xff) == 0x0014)||((pLibDev->swDevID & 0xff) >= 0x0016)) { //venice derby
		if((pLibDev->mode == MODE_11A)||(turbo == TURBO_ENABLE)||(pLibDev->mode == MODE_11O))	{
			if(turbo == HALF_SPEED_MODE) {
				REGW(devNum, 0x987c, 0x1ea) ; // clk_ref * 2
			}
			else if(turbo == QUARTER_SPEED_MODE) {
				REGW(devNum, 0x987c, 0x2ea) ; // clk_ref * 4 / 4
			}
			else {
				REGW(devNum, 0x987c, 0xea) ; // clk_ref * 4
			}
		}
		else {
			if(turbo == HALF_SPEED_MODE) {
				REGW(devNum, 0x987c, 0x1eb);  // clk_ref * 11 / 5
			} else if(turbo == QUARTER_SPEED_MODE) {
				REGW(devNum, 0x987c, 0x2eb);  // clk_ref * 22 / 5 / 4
			} else {
				REGW(devNum, 0x987c, 0xeb);  // clk_ref * 22 / 5
			}
		} 		
	} 
	else {
		mError(devNum, EIO, "pllProgramAr5212: unsupported devID = %x. PLL not programmed\n",
			pLibDev->swDevID);
	}
	reset = 1;

	if (reset) {
		hwResetAr5211(devNum, MAC_RESET | BB_RESET);
	}
}
