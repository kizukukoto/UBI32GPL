/*
 *  Copyright © 2005 Atheros Communications, Inc.,  All Rights Reserved.
 *
 */

#ident  "ACI $Id: //depot/sw/src/dk/mdk/devlib/ar6000/mCfg6000.c#10 $, $Header: //depot/sw/src/dk/mdk/devlib/ar6000/mCfg6000.c#10 $"

#ifdef VXWORKS
#include "vxworks.h"
#endif

#ifdef __ATH_DJGPPDOS__
#define __int64 long long
typedef unsigned long DWORD;
#define Sleep   delay
#endif  // #ifdef __ATH_DJGPPDOS__

#include <errno.h>
#include <stdio.h>
#include <string.h>
#ifndef VXWORKS
#include <malloc.h>
#endif
#include "wlantype.h"

#include "mCfg6000.h"
#include "mEep6000.h"
#include "mCfg211.h"

#include "athreg.h"
#include "manlib.h"
#include "mEeprom.h"
#include "mConfig.h"

#include "ar5211reg.h"

/* Refrence clock table, [XTAL][ChannelSpacing] */
static const 
A_UINT16 gRefClock[5][5] = {{8,  4,  4,  8,  16},
                            {20, 20, 20, 20, 20},
                            {40, 50, 100,200,400},
                            {40, 20, 20, 40, 40},
                            {8,  4,  4,  8,  16}};

static const 
A_UINT16 gXtalFreq[] = { 192,260,400,520,384};

/* Amode Selection table, [XTAL][Channel Spacing] */
static const 
A_UINT16 gAmodeRefSel[5][4] = {{0,0,1,2},
                               {0,0,0,0},
                               {0,1,2,3},
                               {0,0,1,1},
                               {0,0,1,2}};

/**************************************************************************
 * setChannelFractionalNAr6000 - Perform the algorithm to change the channel
 *                    for AR6000 - currently using override Integer mode to match Bank6 INI setting.
 *
 */
A_BOOL setChannelFractionalNAr6000
(
 A_UINT32 devNum,
 A_UINT32 freq
)
{
    A_UINT32 xTalFreq    = 2; // 40 MHz crystalclk
    A_UINT32 channelSel  = 0;
    A_UINT32 channelFrac = 0;  
    A_UINT32 bModeSynth  = 1;
    A_UINT32 aModeRefSel = 0;
    A_UINT32 reg32       = 0;
    A_UINT32 chanFreq = freq * 10;
    A_UINT32 refClock = (A_UINT32)gXtalFreq[xTalFreq];
    A_UINT32 temp;

    if (freq < 4800) {
        chanFreq *= 4;
        refClock *= 3;
    } else  {
        chanFreq *= 2;
        refClock *= 3;
    }
    if(freq == 2484) {
        REGW(devNum, 0xa204, REGR(devNum, 0xa204) | 0x10);
    }

    channelSel = chanFreq / refClock;
    channelSel = reverseBits(channelSel, 8);

    channelFrac = chanFreq % refClock;
    channelFrac = temp = channelFrac << 17;
    channelFrac = channelFrac / refClock;
    temp = (temp % refClock) * 2;
    if (temp >= refClock) channelFrac += 1;
    channelFrac = reverseBits(channelFrac, 17);

    reg32 = (channelFrac << 12) | (channelSel << 4) | (aModeRefSel << 2) | 
            (bModeSynth << 1) | (1 << 29) | 0x1;
    REGW(devNum, PHY_BASE + (0x27 << 2), reg32 & 0xff);

    reg32 >>= 8;
    REGW(devNum, PHY_BASE + (0x27 << 2), reg32 & 0xff);

    reg32 >>= 8;
    REGW(devNum, PHY_BASE + (0x27 << 2), reg32 & 0xff);

    reg32 >>= 8;
    REGW(devNum, PHY_BASE + (0x37 << 2), reg32 & 0xff);

    mSleep(1);
    return TRUE;
}


/**************************************************************************
 * setChannelAr6000 - Perform the algorithm to change the channel
 *                    for AR6000 - currently using override Integer mode to match Bank6 INI setting.
 *
 */
A_BOOL setChannelAr6000
(
 A_UINT32 devNum,
 A_UINT32 freq      // New channel
)
{
    A_UINT32 xTalFreq    = 2; // 40 MHz crystalclk
    A_UINT32 channelSel  = 0;
    A_UINT32 channelFrac = 0;
    A_UINT32 bModeSynth  = 0;
    A_UINT32 aModeRefSel = 0;
    A_UINT32 chanFreq    = freq * 10;
    A_UINT32 reg32       = 0;
    A_UINT32 refClock;
    A_UINT32 temp;

    if (freq < 4800) {
        chanFreq *= 4;
        refClock = (A_UINT32) gRefClock[xTalFreq][0];
    } else {
#ifdef REF20_10DIVS_WORK
        if ((freq % 20) == 0) {
            refClock = (A_UINT32) gRefClock[xTalFreq][4];
            aModeRefSel = (A_UINT32) gAmodeRefSel[xTalFreq][3];
            aModeRefSel = reverseBits(aModeRefSel, 2);
        } else if ((freq % 10) == 0) {
            refClock = (A_UINT32) gRefClock[xTalFreq][3];
            aModeRefSel = (A_UINT32) gAmodeRefSel[xTalFreq][2];
            aModeRefSel = reverseBits(aModeRefSel, 2);
        } else 
#endif
        if ((freq % 5) == 0) {
            refClock = (A_UINT32) gRefClock[xTalFreq][2];
            aModeRefSel = (A_UINT32) gAmodeRefSel[xTalFreq][1];
            aModeRefSel = reverseBits(aModeRefSel, 2);
        } else {
             refClock = (A_UINT32) gRefClock[xTalFreq][1];
            aModeRefSel = (A_UINT32) gAmodeRefSel[xTalFreq][0];
            aModeRefSel = reverseBits(aModeRefSel, 2);
        }       
        chanFreq *= 2;
    }
    if(freq == 2484) {
        REGW(devNum, 0xa204, REGR(devNum, 0xa204) | 0x10);
    }

    temp = chanFreq / refClock;
    channelSel = reverseBits(temp & 0xff, 8);
    channelFrac = reverseBits(((temp >> 8) & 0x7f) << 10, 17);

    reg32 = (channelFrac << 12) | (channelSel << 4) | (aModeRefSel << 2) | 
            (bModeSynth << 1) | (1 << 29) | 0x1;
    REGW(devNum, PHY_BASE + (0x27 << 2), reg32 & 0xff);

    reg32 >>= 8;
    REGW(devNum, PHY_BASE + (0x27 << 2), reg32 & 0xff);

    reg32 >>= 8;
    REGW(devNum, PHY_BASE + (0x27 << 2), reg32 & 0xff);

    reg32 >>= 8;
    REGW(devNum, PHY_BASE + (0x37 << 2), reg32 & 0xff);

    mSleep(1);
    return TRUE;
}

/**************************************************************************
* initPowerAr6000 - Set the power for the AR2413 chips
*
*/
void initPowerAr6000
(
    A_UINT32 devNum,
    A_UINT32 freq,
    A_UINT32  override,
    A_UCHAR  *pwrSettings
)
{
    LIB_DEV_INFO        *pLibDev = gLibInfo.pLibDevArray[devNum];

    //only override the power if the eeprom has been read
    if((!pLibDev->eePromLoad) || (!pLibDev->eepData.infoValid)) {
        return;
    }

    if((override) || (pwrSettings != NULL)) {
        mError(devNum, EINVAL, "Override of power not supported.  Disable eeprom load and change config file instead\n");
        return;
    }

    if (((pLibDev->eepData.version >> 12) & 0xF) == 15) {
        /* AR6000 specific struct and function programs eeprom header writes (board data overrides) */
        ar6000EepromSetBoardValues(devNum);

        /* AR6000 specific struct and function performs all tx power writes */      
        ar6000EepromSetTransmitPower(devNum, freq, NULL);
        return;
    } else {
        mError(devNum, EINVAL, "initPowerAr2413: Illegal eeprom version\n");
        return;
    }
    
    return;
}
