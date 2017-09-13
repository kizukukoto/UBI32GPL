/*
 *  Copyright © 2003 Atheros Communications, Inc.,  All Rights Reserved.
 *
 */

#ident  "ACI $Id: //depot/sw/src/dk/mdk/devlib/ar5513/mCfg513.c#3 $, $Header: //depot/sw/src/dk/mdk/devlib/ar5513/mCfg513.c#3 $"

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

#include "mCfg513.h"

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

#include "ar5513reg.h"

static   A_UINT32 shadowEeprom[0x800];
//static   lastSectorNum = 31; // for a 2M flash
//static   A_UINT32 lastSectorNum = 63; // for a 4M flash
static   A_UINT32 lastSectorNum = 0xFF; // Illegal value until autosized

static   A_BOOL firstBoot = TRUE;
static   A_BOOL autoSized = FALSE;  // flash size 2M or 4M ?

/**************************************************************************
 * setChannelAr5513 - Perform the algorithm to change the channel
 *					  for AR5513 based adapters
 *
 */
A_BOOL setChannelAr5513
(
 A_UINT32 devNum,
 A_UINT32 freq		// New channel
)
{
	A_UINT32 channelSel = 0;
	A_UINT32 bModeSynth = 0;
	A_UINT32 aModeRefSel = 0;
	A_UINT32 regVal = 0;
    LIB_DEV_INFO	*pLibDev = gLibInfo.pLibDevArray[devNum];
//printf("SNOOP: setChannelAr5513 : freq = %d\n", freq);
	if(freq < 4800) {
		if(((freq - 2192) % 5) == 0) {
			channelSel = ((freq-672)*2 - 3040)/10;
			bModeSynth = 0;
		}
		else if (((freq - 2224) % 5) == 0) {
			channelSel = ((freq-704)*2 - 3040)/10;
			bModeSynth = 1;
		}
		else {
			mError(devNum, EINVAL, "%d is an illegal derby driven channel\n", freq);
			return(0);
		}

		channelSel = (channelSel << 2) & 0xff;
		channelSel = reverseBits(channelSel, 8);
		if(freq == 2484) {
			REGW(devNum, 0xa204, REGR(devNum, 0xa204) | 0x10);
			if ( (pLibDev->swDevID == 0xe018) || (pLibDev->swDevID == 0xf018)) {
				REGW(devNum, 0xb204, REGR(devNum, 0xb204) | 0x10); // for chain1
			}
		}
	}
	else {
//		pLibDev->libCfgParams.refClock = REF_CLK_5;
//		printf("SNOOP: forcing ref_clk = 3.33 MHz\n");
		switch(pLibDev->libCfgParams.refClock) {
		case REF_CLK_DYNAMIC:
			if (( (freq % 20) == 0) && (freq >= 5120)){
				channelSel = reverseBits(((freq-4800)/20 << 2), 8);
				aModeRefSel = reverseBits(3, 2);
			} else if ( (freq % 10) == 0) {
				channelSel = reverseBits(((freq-4800)/10 << 1), 8);
				aModeRefSel = reverseBits(2, 2);
			} else if ((freq % 5) == 0) {
				channelSel = reverseBits((freq-4800)/5, 8);
				aModeRefSel = reverseBits(1, 2);
			}
			break;

		case REF_CLK_2_5:
			channelSel = reverseBits((A_UINT32)((freq-4800)/2.5), 8);
			aModeRefSel = reverseBits(0, 2);
			break;

		case REF_CLK_5:
			channelSel = reverseBits((freq-4800)/5, 8);
			aModeRefSel = reverseBits(1, 2);
			break;

		case REF_CLK_10:
			channelSel = reverseBits(((freq-4800)/10 << 1), 8);
			aModeRefSel = reverseBits(2, 2);
			break;

		case REF_CLK_20:
			channelSel = reverseBits(((freq-4800)/20 << 2), 8);
			aModeRefSel = reverseBits(3, 2);
			break;


		default:
			mError(devNum, EINVAL, "Error setChannelAr5212: illegal refClock value %d\n", 
					pLibDev->libCfgParams.refClock);
		}
	}

	// chain 0
	regVal = (channelSel << 4) | (aModeRefSel << 2) | (bModeSynth << 1)| (1 << 12) | 0x1;
	REGW(devNum, PHY_BASE_CHAIN_BOTH+(0x27<<2), (regVal & 0xff));
    regVal = (regVal >> 8) & 0x7f;
	REGW(devNum, PHY_BASE_CHAIN_BOTH+(0x36<<2), regVal);		

	// chain 1
/*	regVal = (channelSel << 4) | (aModeRefSel << 2) | (bModeSynth << 1)| (1 << 12) | 0x1;	
	REGW(devNum, PHY_BASE_CHAIN1+(0x27<<2), (regVal & 0xff)); // for chain1
	regVal = (regVal >> 8) & 0x7f;
	REGW(devNum, PHY_BASE_CHAIN1+(0x36<<2), regVal);		  // for chain1
*/
//printf("SNOOP: setting channel on both chains ...\n");
	mSleep(1);
	return(1);
}
/**************************************************************************
* eepromAutoSizeAr5513 - figure out whether 2M or 4M flash
*                        works for STM and NextFlash SPI flash
*
* RETURNS: TRUE/FALSE
*/
A_BOOL eepromAutoSizeAr5513
(
 A_UINT32 devNum
)
{
	A_UINT32  autoSizecmd, rddata, savedata;
	A_BOOL    retVal = FALSE;
	LIB_DEV_INFO *pLibDev = gLibInfo.pLibDevArray[devNum];

    autoSizecmd   = 0xab;
#ifdef FALCON_ART_CLIENT
    sysRegWrite(0x11300004, autoSizecmd);   // Read, Addr = 0
    sysRegWrite(0x11300008, 0x0);
    sysRegWrite(0x11300000, 0x114);    // Tx count 4 and Rx count = 1

    rddata = (sysRegRead(0x11300000) >> 16) & 0x1;
    while (rddata == 0x1) {
          rddata = (sysRegRead(0x11300000) >> 16) & 0x1;
		  printf(".");
    }

    rddata = sysRegRead(0x11300008) & 0xff;
#else
    REGW(devNum, 0x17004, autoSizecmd);   // Read, Addr = 0
    REGW(devNum, 0x17008, 0x0);
    REGW(devNum, 0x17000, 0x114);    // Tx count 4 and Rx count = 1

    rddata = (REGR(devNum, 0x17000) >> 16) & 0x1;
    while (rddata == 0x1) {
          rddata = (REGR(devNum, 0x17000) >> 16) & 0x1;
		  printf(".");
    }

    rddata = REGR(devNum, 0x17008) & 0xff;
#endif

	switch (rddata) {
	case 0x14 :
		printf("eepromAutoSizeAr5513 : 2M FLASH detected \n");
		lastSectorNum = 31;
		retVal = TRUE;
		break;
	case 0x15 :
		printf("eepromAutoSizeAr5513 : 4M FLASH detected \n");
		lastSectorNum = 63;
		retVal = TRUE;
		break;
	case 0x13 :
		printf("eepromAutoSizeAr5513 : Unsupported flash size (1MB)  detected. only 2M and 4M supported at this time\n");		
	default :
		retVal = FALSE;
		printf("eepromAutoSizeAr5513 : Flash autosizing failed : 0x%x.\n", rddata);
		break;
	}

	// try to see if its an eeprom
	if (!retVal) {
		savedata = spiEepromReadAr5513(devNum, 0x3FF);
		spiEepromWriteAr5513(devNum, 0x3FF, 0xDABA);
		rddata = spiEepromReadAr5513(devNum, 0x3FF);
		if (rddata == 0xDABA) {
			retVal = TRUE;
			pLibDev->libCfgParams.useEepromNotFlash = TRUE;
			printf("Found EEPROM instead.\n");
		}
		spiEepromWriteAr5513(devNum, 0x3FF, savedata);
	}

	return(retVal);
}

/**************************************************************************
* eepromReadAr5513 - Read from flash or EEPROM for given equivalent 16-bit  
*                    EEPROM offset and return the 16 bit value
*
* RETURNS: 16 bit value from given offset (in a 32-bit value)
*/
A_UINT32 eepromReadAr5513
(
 A_UINT32 devNum, 
 A_UINT32 eepromOffset
)
{
	A_UINT32  i, rddata, retVal, rdaddr, rdcmd;
	LIB_DEV_INFO *pLibDev = gLibInfo.pLibDevArray[devNum];

	if (!autoSized) {
		autoSized = eepromAutoSizeAr5513(devNum);
	}

	if (pLibDev->libCfgParams.useShadowEeprom) {
		return shadowEeprom[eepromOffset % 0x800] ; // ensure address never exceeds 0x800
	}

	if (pLibDev->libCfgParams.useEepromNotFlash) {
		return(spiEepromReadAr5513(devNum, eepromOffset));
	}

	i = lastSectorNum*0x10000 + 2*eepromOffset;
    rdaddr  = (i << 8) & 0xffffff00;
    rdcmd   = rdaddr + 3;

#ifdef FALCON_ART_CLIENT
    sysRegWrite(0x11300004, rdcmd);   // Read, Addr = 0
    sysRegWrite(0x11300008, 0x0);
    sysRegWrite(0x11300000, 0x124);    // Tx count 4 and Rx count = 2

    rddata = (sysRegRead(0x11300000) >> 16) & 0x1;
    while (rddata == 0x1) {
          rddata = (sysRegRead(0x11300000) >> 16) & 0x1;
//		  printf(".");
    }
//printf("\nreading data\n");
    rddata = sysRegRead(0x11300008) & 0xffff;
#else
    REGW(devNum, 0x17004, rdcmd);   // Read, Addr = 0
    REGW(devNum, 0x17008, 0x0);
    REGW(devNum, 0x17000, 0x124);    // Tx count 4 and Rx count = 2

    rddata = (REGR(devNum, 0x17000) >> 16) & 0x1;
    while (rddata == 0x1) {
          rddata = (REGR(devNum, 0x17000) >> 16) & 0x1;
//		  printf(".");
    }
//printf("\nreading data\n");
    rddata = REGR(devNum, 0x17008) & 0xffff;
#endif

	retVal = (rddata >> 8) | ((rddata & 0xFF) << 8); // do the byte-swap for sw image to work right
//printf("[0x%x:0x%x]", eepromOffset, retVal);
	return(retVal);
}

/**************************************************************************
* spiEepromReadAr5513 - Read from SPI EEPROM for 16-bit EEPROM 
*                       offset and return the 16 bit value
*
* RETURNS: 16 bit value from given offset (in a 32-bit value)
*/
A_UINT32 spiEepromReadAr5513
(
 A_UINT32 devNum, 
 A_UINT32 eepromOffset
)
{
	A_UINT32  byte, i, rddata, retVal, rdaddr, rdcmd;

	retVal = 0;

	for (byte=0; byte < 2; byte++) {
		i = 2*eepromOffset + byte;

		rdaddr  = (i << 16) & 0xffff0000;
		rdcmd   = rdaddr |  3;
#ifdef FALCON_ART_CLIENT
		sysRegWrite(0x11300004, rdcmd);   // Read, Addr = 0
		sysRegWrite(0x11300008, 0x0);
		sysRegWrite(0x11300000, (1<<19) | 0x113);    // Tx count = 3, RX count = 1

		rddata = (sysRegRead(0x11300000) >> 16) & 0x1;
		while (rddata == 0x1) {
			  rddata = (sysRegRead(0x11300000) >> 16) & 0x1;
		}

		rddata = sysRegRead(0x11300008) & 0xffff;
#else
		REGW(devNum, 0x17004, rdcmd);   // Read, Addr = 0
		REGW(devNum, 0x17008, 0x0);
		REGW(devNum, 0x17000, (1<<19) | 0x113);    // Tx count = 3, RX count = 1

		rddata = (REGR(devNum, 0x17000) >> 16) & 0x1;
		while (rddata == 0x1) {
			  rddata = (REGR(devNum, 0x17000) >> 16) & 0x1;
		}

		rddata = REGR(devNum, 0x17008) & 0xffff;
#endif
		retVal |= (rddata << 8*byte);
	}
	
	return(retVal);
}

void flashEraseSectorAr5513
(
 A_UINT32 devNum,
 A_UINT32 sector
)
{
	A_UINT32  i, rddata, eraddr, ercmd, erase_in_progress;
	LIB_DEV_INFO *pLibDev = gLibInfo.pLibDevArray[devNum];

	if (!autoSized) {
		autoSized = eepromAutoSizeAr5513(devNum);
	}

	if (pLibDev->libCfgParams.useEepromNotFlash) {
		return;
	}

	if ((sector == lastSectorNum) && (pLibDev->libCfgParams.flashCalSectorErased)) {
		return;
	}

	if (sector > lastSectorNum) {
		mError(devNum, ENOMEM, "Sector (%d) out of range. Maximum %d sectors supported.\n", sector, lastSectorNum);
		return;
	}

	i = sector*0x10000;

#ifdef FALCON_ART_CLIENT
    sysRegWrite(0x11300004, 0x6);      // WREN opcode
    sysRegWrite(0x11300008, 0x0);
    sysRegWrite(0x11300000, 0x101);    // Transmission count = 1

    rddata = (sysRegRead(0x11300000) >> 16) & 0x1;
    while (rddata == 0x1) {
          rddata = (sysRegRead(0x11300000) >> 16) & 0x1;
    }

    eraddr = (i << 8) & 0xffffff00;
    ercmd  = eraddr + 0xd8; 

    sysRegWrite(0x11300004, ercmd);   // Erase Sector
    sysRegWrite(0x11300008, 0x0);
    sysRegWrite(0x11300000, 0x104);    // Transmission count = 4

    rddata = (sysRegRead(0x11300000) >> 16) & 0x1;
    while (rddata == 0x1) {
          rddata = (sysRegRead(0x11300000) >> 16) & 0x1;
    }

    erase_in_progress = 1;
    while (erase_in_progress == 0x1) { 
      sysRegWrite(0x11300004, 0x5);      // Status reg read
      sysRegWrite(0x11300008, 0x0);
      sysRegWrite(0x11300000, 0x111);    // Tx and Rx count = 1

      rddata = (sysRegRead(0x11300000) >> 16) & 0x1;
      while (rddata == 0x1) {
            rddata = (sysRegRead(0x11300000) >> 16) & 0x1;
      }

      erase_in_progress = sysRegRead(0x11300008) & 0x1;
	}	
#else
    REGW(devNum, 0x17004, 0x6);      // WREN opcode
    REGW(devNum, 0x17008, 0x0);
    REGW(devNum, 0x17000, 0x101);    // Transmission count = 1

    rddata = (REGR(devNum, 0x17000) >> 16) & 0x1;
    while (rddata == 0x1) {
          rddata = (REGR(devNum, 0x17000) >> 16) & 0x1;
    }

    eraddr = (i << 8) & 0xffffff00;
    ercmd  = eraddr + 0xd8; 

    REGW(devNum, 0x17004, ercmd);   // Erase Sector
    REGW(devNum, 0x17008, 0x0);
    REGW(devNum, 0x17000, 0x104);    // Transmission count = 4

    rddata = (REGR(devNum, 0x17000) >> 16) & 0x1;
    while (rddata == 0x1) {
          rddata = (REGR(devNum, 0x17000) >> 16) & 0x1;
    }

    erase_in_progress = 1;
    while (erase_in_progress == 0x1) { 
      REGW(devNum, 0x17004, 0x5);      // Status reg read
      REGW(devNum, 0x17008, 0x0);
      REGW(devNum, 0x17000, 0x111);    // Tx and Rx count = 1

      rddata = (REGR(devNum, 0x17000) >> 16) & 0x1;
      while (rddata == 0x1) {
            rddata = (REGR(devNum, 0x17000) >> 16) & 0x1;
      }

      erase_in_progress = REGR(devNum, 0x17008) & 0x1;
	}	
#endif
	if (sector == lastSectorNum) {
		pLibDev->libCfgParams.flashCalSectorErased = TRUE;
	}

/*
	printf("\n\n***************************************\n\n");
	printf("SNOOP: ERASED SECTOR %d \n", sector);
	printf("\n\n***************************************\n\n");
*/
}

/**************************************************************************
* eepromReadAr5513 - Write from flash or EEPROM for given equivalent 16-bit  
*                    EEPROM offset and return the 16 bit value
*
* RETURNS: 16 bit value from given offset (in a 32-bit value)
*/
void eepromWriteAr5513
(
 A_UINT32 devNum, 
 A_UINT32 eepromOffset, 
 A_UINT32 eepromValue
)
{
	A_UINT32  i, wrdata, rddata, wraddr, wrcmd, write_in_progress;
	LIB_DEV_INFO *pLibDev = gLibInfo.pLibDevArray[devNum];

	if (!autoSized) {
		autoSized = eepromAutoSizeAr5513(devNum);
	}

	if (pLibDev->libCfgParams.useShadowEeprom) {
		shadowEeprom[eepromOffset % 0x800] = eepromValue; // ensure address never exceeds 0x800
		return;
	}

	if (pLibDev->libCfgParams.useEepromNotFlash) {
		spiEepromWriteAr5513(devNum, eepromOffset, eepromValue);
		return;
	}

	// to handle single eeprom location write transparently.
	if (!(pLibDev->blockFlashWriteOn)) {
		pLibDev->libCfgParams.flashCalSectorErased = FALSE;
		updateSingleEepromValueAR5513(devNum, eepromOffset, eepromValue);
		return;
	}

	if (!(pLibDev->libCfgParams.flashCalSectorErased)) {
		flashEraseSectorAr5513(devNum, lastSectorNum);
	}
#ifdef FALCON_ART_CLIENT
    sysRegWrite(0x11300004, 0x6);      // WREN opcode
    sysRegWrite(0x11300008, 0x0);
    sysRegWrite(0x11300000, 0x101);    // Transmission count = 1

    rddata = (sysRegRead(0x11300000) >> 16) & 0x1;
    while (rddata == 0x1) {
          rddata = (sysRegRead(0x11300000) >> 16) & 0x1;
    }

	i = lastSectorNum*0x10000 + 2*eepromOffset;
//    wrdata = eepromValue & 0xFFFF;
	wrdata = (eepromValue >> 8) | ((eepromValue & 0xFF) << 8); // do the byte-swap for sw image to work right
    wraddr = (i << 8) & 0xffffff00;
    wrcmd  = wraddr + 2;

    sysRegWrite(0x11300004, wrcmd);   // Page Program
    sysRegWrite(0x11300008, wrdata);
    sysRegWrite(0x11300000, 0x106);   // Transmission count = 6 (4 addr/opcode + 2 bytes)

    rddata = (sysRegRead(0x11300000) >> 16) & 0x1;
    while (rddata == 0x1) {
          rddata = (sysRegRead(0x11300000) >> 16) & 0x1;
    }

    write_in_progress = 1;
    while (write_in_progress == 0x1) { 
          sysRegWrite(0x11300004, 0x5);      // Status reg read
          sysRegWrite(0x11300008, 0x0);
          sysRegWrite(0x11300000, 0x111);    // Tx and Rx count = 1

          rddata = (sysRegRead(0x11300000) >> 16) & 0x1;
          while (rddata == 0x1) {
                rddata = (sysRegRead(0x11300000) >> 16) & 0x1;
          }

          write_in_progress = sysRegRead(0x11300008) & 0x1;
    }
#else
    REGW(devNum, 0x17004, 0x6);      // WREN opcode
    REGW(devNum, 0x17008, 0x0);
    REGW(devNum, 0x17000, 0x101);    // Transmission count = 1

    rddata = (REGR(devNum, 0x17000) >> 16) & 0x1;
    while (rddata == 0x1) {
          rddata = (REGR(devNum, 0x17000) >> 16) & 0x1;
    }

	i = lastSectorNum*0x10000 + 2*eepromOffset;
//    wrdata = eepromValue & 0xFFFF;
	wrdata = (eepromValue >> 8) | ((eepromValue & 0xFF) << 8); // do the byte-swap for sw image to work right
    wraddr = (i << 8) & 0xffffff00;
    wrcmd  = wraddr + 2;

    REGW(devNum, 0x17004, wrcmd);   // Page Program
    REGW(devNum, 0x17008, wrdata);
    REGW(devNum, 0x17000, 0x106);   // Transmission count = 6 (4 addr/opcode + 2 bytes)

    rddata = (REGR(devNum, 0x17000) >> 16) & 0x1;
    while (rddata == 0x1) {
          rddata = (REGR(devNum, 0x17000) >> 16) & 0x1;
    }

    write_in_progress = 1;
    while (write_in_progress == 0x1) { 
          REGW(devNum, 0x17004, 0x5);      // Status reg read
          REGW(devNum, 0x17008, 0x0);
          REGW(devNum, 0x17000, 0x111);    // Tx and Rx count = 1

          rddata = (REGR(devNum, 0x17000) >> 16) & 0x1;
          while (rddata == 0x1) {
                rddata = (REGR(devNum, 0x17000) >> 16) & 0x1;
          }

          write_in_progress = REGR(devNum, 0x17008) & 0x1;
    }
#endif
//	printf("SNOOP: writeeeprom: wrote 0x%x [0x%x] = 0x%x\n", i, eepromOffset, wrdata);
}

// Write to SPI EEPROM instead of FLASH.
void spiEepromWriteAr5513
(
 A_UINT32 devNum, 
 A_UINT32 eepromOffset, 
 A_UINT32 eepromValue
)
{
	A_UINT32  byte, i, wrdata, rddata, wraddr, wrcmd, write_in_progress;

	for (byte = 0; byte < 2; byte++) {

#ifdef FALCON_ART_CLIENT	
		sysRegWrite(0x11300004, 0x6);      // WREN opcode
		sysRegWrite(0x11300008, 0x0);
		sysRegWrite(0x11300000, (1<<19) | 0x101);    // Transmission count = 1

    
		rddata = (sysRegRead(0x11300000) >> 16) & 0x1;
		while (rddata == 0x1) {
			  rddata = (sysRegRead(0x11300000) >> 16) & 0x1;
		}
#else
		REGW(devNum, 0x17004, 0x6);      // WREN opcode
		REGW(devNum, 0x17008, 0x0);
		REGW(devNum, 0x17000, (1<<19) | 0x101);    // Transmission count = 1

    
		rddata = (REGR(devNum, 0x17000) >> 16) & 0x1;
		while (rddata == 0x1) {
			  rddata = (REGR(devNum, 0x17000) >> 16) & 0x1;
		}
#endif

		wrdata = (eepromValue >> 8*byte) & 0xff; // byte_wide

		i = 2*eepromOffset + byte;

		// wacky 16-bit address shifting
		wraddr = (i << 16) & 0xffff0000;
		wrcmd  = wraddr | (wrdata << 8) |  2;

#ifdef FALCON_ART_CLIENT	
		sysRegWrite(0x11300004, wrcmd);   // Page Program
		sysRegWrite(0x11300008, 0);
		sysRegWrite(0x11300000, (1<<19) | 0x104);    // 1 byte opcode, 2 byte address and 1 byte data
		rddata = (sysRegRead(0x11300000) >> 16) & 0x1;
		while (rddata == 0x1) {
			  rddata = (sysRegRead(0x11300000) >> 16) & 0x1;
		}
#else
		REGW(devNum, 0x17004, wrcmd);   // Page Program
		REGW(devNum, 0x17008, 0);
		REGW(devNum, 0x17000, (1<<19) | 0x104);    // 1 byte opcode, 2 byte address and 1 byte data
		rddata = (REGR(devNum, 0x17000) >> 16) & 0x1;
		while (rddata == 0x1) {
			  rddata = (REGR(devNum, 0x17000) >> 16) & 0x1;
		}
#endif
		write_in_progress = 1;
		while (write_in_progress == 0x1) {
#ifdef FALCON_ART_CLIENT
			  sysRegWrite(0x11300004, 0x5);      // Status reg read
			  sysRegWrite(0x11300008, 0x0);
			  sysRegWrite(0x11300000, (1<<19) | 0x111);    // Tx and Rx count = 1

			  rddata = (sysRegRead(0x11300000) >> 16) & 0x1;
			  while (rddata == 0x1) {
					rddata = (sysRegRead(0x11300000) >> 16) & 0x1;
			  }

			  rddata = sysRegRead(0x11300008);
#else
			  REGW(devNum, 0x17004, 0x5);      // Status reg read
			  REGW(devNum, 0x17008, 0x0);
			  REGW(devNum, 0x17000, (1<<19) | 0x111);    // Tx and Rx count = 1

			  rddata = (REGR(devNum, 0x17000) >> 16) & 0x1;
			  while (rddata == 0x1) {
					rddata = (REGR(devNum, 0x17000) >> 16) & 0x1;
			  }

			  rddata = REGR(devNum, 0x17008);
#endif
			  write_in_progress = rddata  & 0x1;
		}
	}
}

/* Reset the falcon mac */
void hwNewResetAr5513(
 	A_UINT32 devNum,
 	A_UINT32 resetMask)
{
	A_UINT32 reg;

    printf("SNOOP: using old reset\n");

//  cold reset to cpu
	//REGW(devNum, 0x14000, 0xF);
//	mSleep(5000);
	//mSleep(5000);

	reg = 0;
	if (resetMask & MAC_RESET) {
		reg = reg | FLCN_DSL_PCI_RC_MAC; 
	}
	if (resetMask & BB_RESET) {
		reg = reg | FLCN_DSL_PCI_RC_BB; 
	}
	if (resetMask & BUS_RESET) {
		reg = reg | FLCN_DSL_PCI_RC_PCI; 
	}

//  warm reset to mac/bb
//	REGW(devNum, 0x4000, 0x3);  
	REGW(devNum, (FLCN_PCI_DSL_PCI_ZERO_BASE + FLCN_DSL_PCI_RC), reg);

//  force sleep
//	REGW(devNum, 0x4004, 0x10000);
	REGW(devNum, (FLCN_PCI_DSL_PCI_ZERO_BASE + FLCN_DSL_PCI_SCR), FLCN_DSL_PCI_SCR_FORCE_SLEEP);

	mSleep(100);

//	reg = REGR(devNum, 0x4000);
//	printf("SNOOP: reg 4000 = 0x%x\n", reg);
//	reg = REGR(devNum, 0x4004);
//	printf("SNOOP: reg 4004 = 0x%x\n", reg);
	
//  force wake
//	REGW(devNum, 0x4004, 0);
	REGW(devNum, (FLCN_PCI_DSL_PCI_ZERO_BASE + FLCN_DSL_PCI_SCR), FLCN_DSL_PCI_SCR_FORCE_WAKE);
	
	mSleep(10);

//  mac/bb out of reset
//	REGW(devNum, 0x4000, 0);
	REGW(devNum, (FLCN_PCI_DSL_PCI_ZERO_BASE + FLCN_DSL_PCI_RC), FLCN_DSL_PCI_RC_OUT_OF_RESET);

	mSleep(100);

//	reg = REGR(devNum, 0x4000);
//	printf("SNOOP: reg 4000 = 0x%x\n", reg);
//	reg = REGR(devNum, 0x4004);
//	printf("SNOOP: reg 4004 = 0x%x\n", reg);

	// initialize GPIOs as output for fb40 7-seg LED display
	if (isFalconEmul(devNum)) {
		REGW(devNum, 0x14098, 0xff);
		REGW(devNum, 0x14090, 0x7f);  // display 8 for mdk resetdevice
	}

//  Clear all resets
//	REGW(devNum, 0x14004, 0x0);
	REGW(devNum, (FLCN_PCI_DSL_CONFIG_BASE + FLCN_DSL_CONFIG_WARM_RST),FLCN_DSL_CONFIG_WARM_RST_OUT_OF_RESET);

	mSleep(10);

//  display falcon rev
//	printf("SNOOP: Falcon rev : 0x%x\n", REGR(devNum, 0x14014));

	// Enable PCI. Enable MPEG and LocalBus as well if emulation
//	REGW(devNum, 0x14018, 0x3);
	if (isFalconEmul(devNum)) {  
		REGW(devNum, (FLCN_PCI_DSL_CONFIG_BASE + FLCN_DSL_CONFIG_IF_CTL),FLCN_DSL_CONFIG_IF_CTL_EN_ALL);
	} else {
		REGW(devNum, (FLCN_PCI_DSL_CONFIG_BASE + FLCN_DSL_CONFIG_IF_CTL),FLCN_DSL_CONFIG_IF_CTL_EN_PCI_ONLY);
	}

	mSleep(10);
	
	// Set Arbitration Control - AHB Master request processed for all interfaces
//	REGW(devNum, 0x14008, 0x1f);
	REGW(devNum, (FLCN_PCI_DSL_CONFIG_BASE + FLCN_DSL_CONFIG_ARB_CTL),FLCN_DSL_CONFIG_ARB_CTL_ALL_ARB);

	mSleep(10);
	
//  Endiandness Control - default boot up - wmac is in big endian mode
//  either : let it be and byteswap the tx/rx data/descriptor, 
//  or     : make everything little endian by writing 0 to 0x1400c
//  current implementation : config file does the byteswap.
//	REGW(devNum, 0x1400c, 0);
//	REGW(devNum, (FLCN_PCI_DSL_CONFIG_BASE + FLCN_DSL_CONFIG_ENDIAN_CTL),FLCN_DSL_CONFIG_ENDIAN_CTL_ALL_LITTLE);
	REGW(devNum, 0x0014, 0xF);

//	Bring out of sleep mode - already done
//	REGW(devNum, (FLCN_PCI_DSL_PCI_ZERO_BASE + FLCN_DSL_PCI_SCR), FLCN_DSL_PCI_SCR_FORCE_WAKE);
//	mSleep(10);

//  Set SDRAM refresh interval and other settings.
//	REGW(devNum, 0x12010, 0x96);
//	REGW(devNum, (FLCN_PCI_DSL_SDRAM_BASE + FLCN_DSL_SDRAM_REFRESH), FLCN_DSL_SDRAM_REFRESH_MDK_SETTING);

}

void pllProgramAr5513
(
 	A_UINT32 devNum,
 	A_UINT32 turbo
)
{
	LIB_DEV_INFO	*pLibDev = gLibInfo.pLibDevArray[devNum];
	A_UINT32 reset;

	reset = 0;

//	return;

//	REGW(devNum, 0xa200, 0); // why is it here ?

	if (isFalconEmul(devNum)) {
//		printf("SNOOP: skipping PLL programming for falcon Emul\n");

		return;

/*		if((pLibDev->mode == MODE_11A)||(turbo == TURBO_ENABLE)||(pLibDev->mode == MODE_11O))	{
			if(turbo == HALF_SPEED_MODE) {
//				REGW(devNum, 0x987c, 0x114e5) ;
				REGW(devNum, 0xc87c, 0xca) ;
				REGW(devNum, 0xd87c, 0xca) ;
			} 
	        else {
//				REGW(devNum, 0x987c, 0x114ea) ;
				REGW(devNum, 0xc87c, 0xea) ;
				REGW(devNum, 0xd87c, 0xea) ;				
			}
		} else {
			if(turbo == HALF_SPEED_MODE) {
//				REGW(devNum, 0x987c, 0x114cb);
				REGW(devNum, 0xc87c, 0xcb);
				REGW(devNum, 0xd87c, 0xcb);
			} else {
//				REGW(devNum, 0x987c, 0x114eb);
				REGW(devNum, 0xc87c, 0xeb);
				REGW(devNum, 0xd87c, 0xeb);
			}
		} 
*/

	} else { 
		// is falcon chip
		if((pLibDev->mode == MODE_11A)||(turbo == TURBO_ENABLE)||(pLibDev->mode == MODE_11O))	{
			if(turbo == HALF_SPEED_MODE) {
				REGW(devNum, 0x987c, 0x114e5) ;
			} 
	        else {
				REGW(devNum, 0x987c, 0x114ea) ;
			}
		} else {
			if(turbo == HALF_SPEED_MODE) {
				REGW(devNum, 0x987c, 0x114cb);
			} else {
				REGW(devNum, 0x987c, 0x114eb);
			}
		} 		


		// see dsl_regs.txt "pll programming" for details

		// RST_PLLC_CTL programming - set PLLC
/*		REGW(devNum, 0x14064, ( (0x2 << 0) | (15 << 2) | (0 << 7) |  // 150 MHz
			                    (0x4 << 8) | (5 << 11) | (1 << 14)|
								(0 << 17)  | (0 << 19) ) );
*/
#ifdef FALCON_ART_CLIENT
                sysRegWrite(0x11000064, 0xac5f);
#else
		REGW(devNum, 0x14064, 0xac5f); // for 184/92 MHz
#endif

/*		REGW(devNum, 0x14064, ( (0x2 << 0) | (6 << 2) | (0 << 7) |    // 30 MHz instead of 150 MHz
			                    (0x4 << 8) | (5 << 11) | (1 << 14)|
								(0 << 17)  | (0 << 19) ) );

*/
		// RST_PLLV_CTL programming - set PLLV
#ifdef FALCON_ART_CLIENT
		sysRegWrite(0x11000068, ( (0x2 << 0) | (27 << 2) | (0 << 7) | 
			                    (0x4 << 8) | (5 << 11) | (0 << 14)|
								(0 << 19) ) );
#else
		REGW(devNum, 0x14068, ( (0x2 << 0) | (27 << 2) | (0 << 7) | 
			                    (0x4 << 8) | (5 << 11) | (0 << 14)|
								(0 << 19) ) );
#endif

		// power down PLLv
//		REGW(devNum, 0x14068, (1 << 19) | REGR(devNum, 0x14068));

		// RST_CPUCLK_CTL programming - select cpu clk as PLLC div2_clk (150 MHz)
#ifdef FALCON_ART_CLIENT
		sysRegWrite(0x1100006C, ((0 << 0) | (0 << 2)));
#else
		REGW(devNum, 0x1406C, ((0 << 0) | (0 << 2)));
#endif

		// RST_AMBACLK_CTL programming - select sdram/ahb clk as PLLC div3_clk (100 MHz)
#ifdef FALCON_ART_CLIENT
		sysRegWrite(0x11000070, ((1 << 0) | (0 << 2)));  // div3_clk  [92 MHz]
#else
		REGW(devNum, 0x14070, ((1 << 0) | (0 << 2)));  // div3_clk  [92 MHz]
#endif
//		REGW(devNum, 0x14070, ((0 << 0) | (0 << 2)));  // div2_clk

		// RST_SYNCCLK_CTL programming - select sync clk as PLLV vclk (27 MHz)
#ifdef FALCON_ART_CLIENT
		sysRegWrite(0x11000074, ((2 << 0) | (0 << 2)));
#else
		REGW(devNum, 0x14074, ((2 << 0) | (0 << 2)));
#endif

		// RST_MPGDIVCLK_CTL programming - select mpeg-ts clk as PLLV vclk (27 MHz)
#ifdef FALCON_ART_CLIENT
		sysRegWrite(0x11000078, (2 << 0));
#else
		REGW(devNum, 0x14078, (2 << 0));
#endif

//		printf("SNOOP: programmed PLLs\n");
	}

	reset = 1;

	if (reset) {
		hwResetAr5513(devNum, MAC_RESET | BB_RESET);
	}
}


/**
 * - Function Name: hwResetAr5513
 * - Description  : reset the falcon mac
 * - Arguments
 *     -      
 * - Returns      : 
 ******************************************************************************/
void hwResetAr5513(
 	A_UINT32 devNum,
 	A_UINT32 resetMask)
{
	A_UINT32 i;
	A_UINT32 srev;
	A_UINT32 temp, modeSel;
    A_UINT32 mask;


    mask = resetMask;  // added to bypass build warning
//	printf("SNOOP: using new reset\n");
/*
    Verify Falcon's interf_mode_sel[2:0] pins are configured appropriately
	on the board :
	   interf_mode_sel[2:0]         Meaning
	   -------------------- -----------------------------
	      0							Local bus+MPEG-TS, with CPU
	      1							ERMA, with CPU
		  2							PCI, with CPU
		  3							CardBus, with CPU
		  4							PCI, without CPU   <-- default used for ART/mdk over PCI
		  5							CardBus, without CPU
		  6							MPEG-TS+GPIO, with CPU
		  7							<Invalid>
*/

/*
    Currently ART/MDK for falcon is only supported over PCI and CPU is disabled.
    If CPU is enabled, then CPU will boot from SPI flash.  The CPU's first
    fetch will be to location 0 in the flash.  You need to have a valid
    flash image sufficient to initialize the CPU starting at location 0 in
    the flash. 
*/

/*
Enable appropriate interfaces to arbitrate for the AHB bus
*/
	// If MPEG-TS+lclbus is in use, then write the value 0x0f to the
    // RST_AHB_ARB_CTL register at address 0x14008.
	
	// REGW(devNum, 0x14008, 0xF); // mpeg/localbus NOT used in PCI mode.

	// If PCI is in use then write the value 0x13 to the RST_AHB_ARB_CTL 
	// register at address 0x14008.
#ifdef FALCON_ART_CLIENT
	modeSel = ((sysRegRead(0x11000018) >> 24) & 0x7);
#else
	modeSel = ( (REGR(devNum, 0x14018) >> 24) & 0x7);
#endif

	if(((modeSel == 2) || (modeSel == 0)) && firstBoot){
		printf("waiting for bootrom ...\n");
		mSleep(7000);
		firstBoot = FALSE;
	}

#ifdef FALCON_ART_CLIENT
        sysRegWrite(0x11000008, (sysRegRead(0x11000008) | 0x1));
#else
	// disable CPU from arbitrating
//	REGW(devNum, 0x14008, (REGR(devNum, 0x14008) & 0xFFFFFFFE));
#endif
	
	if (modeSel == 4) {
		// may even take this out
#ifdef FALCON_ART_CLIENT
		sysRegWrite(0x11000000, 0x3);
#else
//        REGW(devNum, 0x14000, 0x3);
#endif
		mSleep(25);
#ifdef FALCON_ART_CLIENT
		temp = sysRegRead(0x11000000) & 0xf;
#else
		temp = REGR(devNum, 0x14000) & 0xf;
#endif
		while (temp != 0x0) {
		    mSleep(1);
#ifdef FALCON_ART_CLIENT
            temp = sysRegRead(0x11000000) & 0xf;
#else
			temp = REGR(devNum, 0x14000) & 0xf;
#endif
		}
	}

    // - doing pci core reset multiple times seems to hang the PC with falcon. avoid setting bit 4. 
	//	REGW(devNum, 0x4000, 0x13); // force reset 
#ifdef FALCON_ART_CLIENT
        sysRegWrite(0x10104000, 0x3);
	sysRegWrite(0x10104004, 0x10000);
#else
	REGW(devNum, 0x4000, 0x3); // force reset
	REGW(devNum, 0x4004, 0x10000); 
#endif
	//mSleep(100);
	mSleep(10); // cut back on sleep for responsiveness

#ifdef FALCON_ART_CLIENT

    sysRegWrite(0x10104004, 0x0);
    temp = sysRegRead(0x10104004);
#else
	REGW(devNum, 0x4004, 0);
	temp = REGR(devNum, 0x4004);
#endif

	while (0 != temp) {		
		mSleep(1);
#ifdef FALCON_ART_CLIENT
        temp = sysRegRead(0x10104004);
#else
		temp = REGR(devNum, 0x4004);
#endif
	}

#ifdef FALCON_ART_CLIENT
    sysRegWrite(0x10104000, 0x0);
	temp = sysRegRead(0x10104000);
#else
	REGW(devNum, 0x4000, 0);
    temp = REGR(devNum, 0x4000);
#endif

	while (0 != temp) {
		mSleep(1);
#ifdef FALCON_ART_CLIENT
		temp = sysRegRead(0x10104000);
#else
		temp = REGR(devNum, 0x4000);
#endif
	}
//  PLL program. done independently.

/*
    Initialize the memory controller.
*/

    // Set the refresh interval in the MEMCTL_SREFR register at address
	// 0x12010 to the value 500.  This is a safe default value.  It
    // is NOT the optimal value:
#ifdef FALCON_ART_CLIENT
    sysRegWrite(0x10300010, 500); 
#else
	REGW(devNum, 0x12010, 500);
#endif

	// Enable the use of the internal DRAM feedback clock by writing the
    // value '1' to bit [3] of the RST_MEMCTL register at address 0x14040
    // REMEMBER to do a read-modify-write.
#ifdef FALCON_ART_CLIENT
       sysRegWrite(0x11000040, (sysRegRead(0x1100040) | (1 << 3)));
#else
	REGW(devNum, 0x14040, (REGR(devNum, 0x14040) | (1 << 3)));
#endif
	// Calibrate the DRAM feedback clock delay lines. Two ways :
	//   + either cal for each card upon power-up, or
	//   + measure for several cards. if close enough and wide 
	//     enough "functional" margin, then prgram the empirical value.

	// program empirical value initially
	// NOTE : this value may need to be different for different boards.
	// TBD. (SNOOP)
#ifdef FALCON_ART_CLIENT
    temp = sysRegRead(0x11000040);
#else
	temp = REGR(devNum, 0x14040);
#endif
	temp &= ~(0xFF << 4); // null the coarse and fine field
	temp |= (FALCON_COARSE_DELAY << 4) | (FALCON_FINE_DELAY << 8);
#ifdef FALCON_ART_CLIENT
    sysRegWrite(0x11000040, temp);
#else
	REGW(devNum, 0x14040, temp);
#endif

/*
Bring the WMAC/WBB out of sleep
*/
	// Write a value to 0 to the PCI_SCR register at 0x4004
	// **** REGW(devNum, 0x4004, 0);

	//mSleep(250);  
	mSleep(25); // cut back on sleep for responsiveness
	// Verify that bit [16] of the PCI_CFG register at address
    // 0x4010 is 0.  If it's not zero, the WMAC/WBB failed to exit
    // sleep and this needs to be investigated.
	i = 0;
#ifdef FALCON_ART_CLIENT
	if (((sysRegRead(0x10104010) & 0x10000) == 0x10000) && (i < 4)){
#else
	if (((REGR(devNum, 0x4010) & 0x10000) == 0x10000) && (i < 4)){
#endif
		//mSleep(100);
		mSleep(20);
		printf("hwResetAR5513 : iter %d : waiting another 20ms for wmac/wbb to exit sleep\n", i);
	}
#ifdef FALCON_ART_CLIENT
	if(((sysRegRead(0x10104010) & 0x10000) == 0x10000)) {
#else
	if ((REGR(devNum, 0x4010) & 0x10000) == 0x10000) {
#endif
		printf("hwResetAR5513 : wmac/wbb failed to exit sleep after 4 iterations. abort.\n");
		mError(devNum, EINVAL, "wmac/wbb failed to exit sleep after 4 iterations. abort.\n");
		return;
	}
/*
Bring all interfaces out of warm reset
*/
	// Write a value of 0 to the RST_WARM_CTL register at address 0x14004
#ifdef FALCON_ART_CLIENT
	sysRegWrite(0x11000004, 0x0);
#else
	REGW(devNum, 0x14004, 0);
#endif

	// Pause for 100ms
	//mSleep(100);
	mSleep(10); // cut back on sleep for responsiveness
/*
    Sanity check: read the falcon silicon revision register (RST_SREV) at
    address 0x11000014.  Expected value is 0x70.
*/
#ifdef FALCON_ART_CLIENT
	srev = sysRegRead(0x11000014);
#else
	srev = REGR(devNum, 0x14014);
#endif

	if ((srev != 0x70) && (srev != 0x71) && (srev != 0x72)){
		printf("ERROR : Silicon revision (0x%x) differs from expected rev 0x70\n", srev);
		mError(devNum, EINVAL, "Silicon revision (0x%x) differs from expected rev 0x70\n", srev);
		return;
	}

	if (modeSel == 4) {
#ifdef FALCON_ART_CLIENT
                sysRegWrite(0x11000008, 0x12);
#else
		REGW(devNum, 0x14008, 0x12); // PCI only
#endif
	} else if (modeSel == 2) {
#ifdef FALCON_ART_CLIENT
                sysRegWrite(0x11000008, 0x13);
#else
		REGW(devNum, 0x14008, 0x13); // PCI + CPU
#endif
	} else if(modeSel == 0) {
#ifdef FALCON_ART_CLIENT
            sysRegWrite(0x11000008, 0xf);    // Local bus + CPU
#endif
        }

	
//  Endiandness Control - default boot up - wmac is in big endian mode
//  either : let it be and byteswap the tx/rx data/descriptor, 
//  or     : make everything little endian by writing 0 to 0x1400c
//  current implementation : config file does the byteswap.
//	REGW(devNum, 0x1400c, 0);
//  REGW(devNum, 0x1400c, 0x1400); // helps sw pci-only ndis driver
    REGW(devNum, 0x0014, 0xF);

/*
Initialize the WMAC. (config files take care of that)
*/

/*
Calibrate the WBB. (offset and noisefloor cal are done later in resetDevice)
*/
	// initialize GPIOs as output for fb40 7-seg LED display
	if (isFalconEmul(devNum)) {
#ifdef FALCON_ART_CLIENT
        sysRegWrite(0x11000098, 0xff);
        sysRegWrite(0x11000090, 0x7f);
#else
		REGW(devNum, 0x14098, 0xff);
		REGW(devNum, 0x14090, 0x7f);  // display 8 for mdk resetdevice
#endif
	}
}

/**
 * - Function Name: forceAntennaTbl5513
 * - Description  : 
 * - Arguments
 *     -      
 * - Returns      : 
 ******************************************************************************/
MANLIB_API void forceAntennaTbl5513(
 A_UINT32 devNum,
 A_UINT16 *pAntennaTbl
)
{
    LIB_DEV_INFO		*pLibDev = gLibInfo.pLibDevArray[devNum];
	MODE_HEADER_INFO	*pModeInfo;
	A_UINT16			*pAntennaTbl_chain1;

	writeField(devNum, "bb_switch_table_idle", pAntennaTbl[0]);
	writeField(devNum, "bb_switch_table_t1", pAntennaTbl[1]);
	writeField(devNum, "bb_switch_table_r1", pAntennaTbl[2]);
	writeField(devNum, "bb_switch_table_r1x1", pAntennaTbl[3]);
	writeField(devNum, "bb_switch_table_r1x2", pAntennaTbl[4]);
	writeField(devNum, "bb_switch_table_r1x12", pAntennaTbl[5]);
	writeField(devNum, "bb_switch_table_t2", pAntennaTbl[6]);
	writeField(devNum, "bb_switch_table_r2", pAntennaTbl[7]);
	writeField(devNum, "bb_switch_table_r2x1", pAntennaTbl[8]);
	writeField(devNum, "bb_switch_table_r2x2", pAntennaTbl[9]);
	writeField(devNum, "bb_switch_table_r2x12", pAntennaTbl[10]);

	switch(pLibDev->mode) {
	case MODE_11A:
		pModeInfo = &(pLibDev->chain[1].p16kEepHeader->info11a);
		break;

	case MODE_11G:
	case MODE_11O:
		pModeInfo = &(pLibDev->chain[1].p16kEepHeader->info11g);
		break;

	case MODE_11B:
		pModeInfo = &(pLibDev->chain[1].p16kEepHeader->info11b);
		break;

	default:
		mError(devNum, EINVAL, "Device Number %d:Illegal mode passed to forceAntennaTbl5513\n", devNum);
		return;
	} //end switch

	pAntennaTbl_chain1 = pModeInfo->antennaControl;

	writeField(devNum, "chn1_bb_switch_table_idle", pAntennaTbl_chain1[0]);
	writeField(devNum, "chn1_bb_switch_table_t1", pAntennaTbl_chain1[1]);
	writeField(devNum, "chn1_bb_switch_table_r1", pAntennaTbl_chain1[2]);
	writeField(devNum, "chn1_bb_switch_table_r1x1", pAntennaTbl_chain1[3]);
	writeField(devNum, "chn1_bb_switch_table_r1x2", pAntennaTbl_chain1[4]);
	writeField(devNum, "chn1_bb_switch_table_r1x12", pAntennaTbl_chain1[5]);
	writeField(devNum, "chn1_bb_switch_table_t2", pAntennaTbl_chain1[6]);
	writeField(devNum, "chn1_bb_switch_table_r2", pAntennaTbl_chain1[7]);
	writeField(devNum, "chn1_bb_switch_table_r2x1", pAntennaTbl_chain1[8]);
	writeField(devNum, "chn1_bb_switch_table_r2x2", pAntennaTbl_chain1[9]);
	writeField(devNum, "chn1_bb_switch_table_r2x12", pAntennaTbl_chain1[10]);

	return;
}

/**************************************************************************
* flashReadSectorAr5513 - Read a sector from flash in 4 byte width
*                    
*
* RETURNS: list of 32 bit values for entire sector
*/
A_BOOL flashReadSectorAr5513(
 A_UINT32 devNum, 
 A_UINT32 sectorNum,
 A_UINT32 *retList,
 A_UINT32 retListSize)
{
	A_UINT32  i, jj, rddata, rdaddr, rdcmd;

	if (!autoSized) {
		autoSized = eepromAutoSizeAr5513(devNum);
	}

	if (retListSize < (0x10000/4)) {
		mError(devNum, ENOMEM, "flashReadSectorAr5513: retListSize [%d] less than sector size\n", retListSize);
		return(FALSE);
	}

	if (sectorNum > lastSectorNum) {
		mError(devNum, ENOMEM, "flashReadSectorAr5513: sectorNum [%d] larger than lastSector size\n", sectorNum, lastSectorNum);
		return(FALSE);
	}

	// always read real flash data. never from shadowEeprom

	for (jj = 0; jj < (0x10000/4); jj++) {
		i = sectorNum*0x10000 + 4*jj;
		rdaddr  = (i << 8) & 0xffffff00;
		rdcmd   = rdaddr + 3;

#ifdef FALCON_ART_CLIENT
		sysRegWrite(0x11300004, rdcmd);   // Read, Addr = 0
		sysRegWrite(0x11300008, 0x0);
		sysRegWrite(0x11300000, 0x144);    // Tx count 4 and Rx count = 4

		rddata = (sysRegRead(0x11300000) >> 16) & 0x1;
		while (rddata == 0x1) {
			  rddata = (sysRegRead(0x11300000) >> 16) & 0x1;
		}
		rddata = sysRegRead(0x11300008);
#else
		REGW(devNum, 0x17004, rdcmd);   // Read, Addr = 0
		REGW(devNum, 0x17008, 0x0);
		REGW(devNum, 0x17000, 0x144);    // Tx count 4 and Rx count = 4

		rddata = (REGR(devNum, 0x17000) >> 16) & 0x1;
		while (rddata == 0x1) {
			  rddata = (REGR(devNum, 0x17000) >> 16) & 0x1;
		}
		rddata = REGR(devNum, 0x17008);
#endif
		retList[jj] = (rddata >> 24) | ((rddata & 0xFF0000) >> 8) | 
			     ((rddata & 0x0000FF00) << 8) | ((rddata & 0xFF) << 24); // do the byte-swap for sw image to work right
	}

	return(TRUE);
}

/**************************************************************************
* flashWriteSectorAr5513 - Write a sector to flash in 4 byte width
*                    
*
* RETURNS: list of 32 bit values for entire sector
*/
A_BOOL flashWriteSectorAr5513
(
 A_UINT32 devNum, 
 A_UINT32 sectorNum,
 A_UINT32 *retList,
 A_UINT32 retListSize
)
{
	A_UINT32  i, jj, wrdata, rddata, tmpdata, wraddr, wrcmd, write_in_progress;

	if (!autoSized) {
		autoSized = eepromAutoSizeAr5513(devNum);
	}

	if (retListSize > (0x10000/4)) {
		mError(devNum, ENOMEM, "flashWriteSectorAr5513: retListSize [%d] greater than sector size\n", retListSize);
		return(FALSE);
	}

	if (sectorNum > lastSectorNum) {
		mError(devNum, ENOMEM, "flashWriteSectorAr5513: sectorNum [%d] larger than lastSector size\n", sectorNum, lastSectorNum);
		return(FALSE);
	}

	flashEraseSectorAr5513(devNum, sectorNum);

	for (jj = 0; jj < retListSize; jj++) {
#ifdef FALCON_ART_CLIENT
		sysRegWrite(0x11300004, 0x6);      // WREN opcode
		sysRegWrite(0x11300008, 0x0);
		sysRegWrite(0x11300000, 0x101);    // Transmission count = 1

		rddata = (sysRegRead(0x11300000) >> 16) & 0x1;
		while (rddata == 0x1) {
			  rddata = (sysRegRead(0x11300000) >> 16) & 0x1;
		}
#else
		REGW(devNum, 0x17004, 0x6);      // WREN opcode
		REGW(devNum, 0x17008, 0x0);
		REGW(devNum, 0x17000, 0x101);    // Transmission count = 1

		rddata = (REGR(devNum, 0x17000) >> 16) & 0x1;
		while (rddata == 0x1) {
			  rddata = (REGR(devNum, 0x17000) >> 16) & 0x1;
		}
#endif
		i = sectorNum*0x10000 + 4*jj;

		tmpdata = retList[jj];

		wrdata = (tmpdata >> 24) | ((tmpdata & 0xFF0000) >> 8) | 
			     ((tmpdata & 0x0000FF00) << 8) | ((tmpdata & 0xFF) << 24); // do the byte-swap for sw image to work right
		wraddr = (i << 8) & 0xffffff00;
		wrcmd  = wraddr + 2;

#ifdef FALCON_ART_CLIENT
		sysRegWrite(0x11300004, wrcmd);   // Page Program
		sysRegWrite(0x11300008, wrdata);
		sysRegWrite(0x11300000, 0x108);   // Transmission count = 8 (4 addr/opcode + 4 bytes)

		rddata = (sysRegRead(0x11300000) >> 16) & 0x1;
		while (rddata == 0x1) {
			  rddata = (sysRegRead(0x11300000) >> 16) & 0x1;
		}
#else
		REGW(devNum, 0x17004, wrcmd);   // Page Program
		REGW(devNum, 0x17008, wrdata);
		REGW(devNum, 0x17000, 0x108);   // Transmission count = 8 (4 addr/opcode + 4 bytes)

		rddata = (REGR(devNum, 0x17000) >> 16) & 0x1;
		while (rddata == 0x1) {
			  rddata = (REGR(devNum, 0x17000) >> 16) & 0x1;
		}
#endif

		write_in_progress = 1;
		while (write_in_progress == 0x1) { 
#ifdef FALCON_ART_CLIENT
	 		  sysRegWrite(0x11300004, 0x5);      // Status reg read
			  sysRegWrite(0x11300008, 0x0);
			  sysRegWrite(0x11300000, 0x111);    // Tx and Rx count = 1

			  rddata = (sysRegRead(0x11300000) >> 16) & 0x1;
			  while (rddata == 0x1) {
					rddata = (sysRegRead(0x11300000) >> 16) & 0x1;
			  }

			  write_in_progress = sysRegRead(0x11300008) & 0x1;
#else
	 		  REGW(devNum, 0x17004, 0x5);      // Status reg read
			  REGW(devNum, 0x17008, 0x0);
			  REGW(devNum, 0x17000, 0x111);    // Tx and Rx count = 1

			  rddata = (REGR(devNum, 0x17000) >> 16) & 0x1;
			  while (rddata == 0x1) {
					rddata = (REGR(devNum, 0x17000) >> 16) & 0x1;
			  }

			  write_in_progress = REGR(devNum, 0x17008) & 0x1;
#endif
		}
	}
	return(TRUE);
}

/**
 * - Function Name: updateMacAddrAR5513
 * - Description  : 
 * - Arguments
 *     -      
 * - Returns      : 
 ******************************************************************************/
A_BOOL updateMacAddrAR5513 (
	A_UINT32 devNum,
	A_UINT8  wmacAddr[6] // wmac addr [0]-->lsb, [5]-->msb
)
{
	A_UINT32 boardData[0x4000];
	A_UINT32 boardDataSize = 0x4000;
	A_UINT32 mac_byte_offset[] = {96, 118, 124, 130, 136}; // wlan0Mac wlan1Mac lbMac mpegMac pciMac
	A_UINT32 offset;
    A_UINT32 lmac[5];
	A_UINT32 ii;
    A_UINT32 chksum = 0;
	A_UINT32 temp;
	A_UINT32 boardDataEndByte = 152;
	LIB_DEV_INFO *pLibDev = gLibInfo.pLibDevArray[devNum];

	lmac[0] = (wmacAddr[3] << 24) | (wmacAddr[2] << 16) | (wmacAddr[1] << 8) | wmacAddr[0];
	lmac[1] = lmac[0];
	lmac[2] = lmac[0] + 1;
	lmac[3] = lmac[0] + 2;
	lmac[4] = lmac[0] + 3;

	if (!autoSized) {
		autoSized = eepromAutoSizeAr5513(devNum);
	}

	if (pLibDev->libCfgParams.useEepromNotFlash) {
		return(TRUE);
	}

	if (!flashReadSectorAr5513(devNum, (lastSectorNum-1), &(boardData[0]), boardDataSize)) {
		printf("updateMacAddrAR5513 : Could not read board data sector. Exiting\n");
		return(FALSE);
	}

	for (ii=0; ii<5; ii++) {
		offset = (A_UINT32)(mac_byte_offset[ii] >> 2);
//		printf("SNOOP1: ii=%d : words at offset %d were : 0x%x, 0x%x\n", ii, offset, boardData[offset], boardData[offset+1]);
		if ((mac_byte_offset[ii] % 4) == 2) {
			temp = (wmacAddr[5] << 8) | wmacAddr[4];
			boardData[offset] = (boardData[offset] & 0xFFFF0000) | temp ;
			boardData[offset+1] = lmac[ii];
		} else if ((mac_byte_offset[ii] % 4) == 0) {
			temp = (wmacAddr[5] << 24) | (wmacAddr[4] << 16);
			boardData[offset] = temp | ((lmac[ii] >> 16) & 0xFF00) |
								((lmac[ii] >> 16) & 0xFF) ;
			boardData[offset+1] = ((lmac[ii] & 0xFFFF) << 16) |
											   (boardData[offset+1] & 0xFFFF);
		} else {
			printf("updateMacAddrAR5513 : mac offset [%d for ii=%d] not at 16-bit boundary. Exiting...\n", offset, ii);
			return(FALSE);
		}
//		printf("SNOOP2: ii=%d : words at offset %d were : 0x%x, 0x%x\n", ii, offset, boardData[offset], boardData[offset+1]);
	}

	boardData[1] &= 0xFFFF;
	
    chksum = 0; // Initialized
	for (ii=0; ii<(boardDataEndByte>>2); ii++) {
		chksum += (boardData[ii] >> 24) & 0xFF;
		chksum += (boardData[ii] >> 16) & 0xFF;
		chksum += (boardData[ii] >>  8) & 0xFF;
		chksum += (boardData[ii] >>  0) & 0xFF;
	}
	boardData[1] = (boardData[1] & 0xFFFF) | ((chksum & 0xFFFF) << 16);

	printf("chksum = 0x%x\n", chksum);

	if (!flashWriteSectorAr5513(devNum, (lastSectorNum-1), &(boardData[0]), 50)) { // since boarddata is less than 152 bytes
		printf("updateMacAddrAR5513 : Could not write board data sector. Exiting\n");
		return(FALSE);
	}

	return(TRUE);
}

/**
 * - Function Name: updateSingleEepromValueAR5513
 * - Description  : 
 * - Arguments
 *     -      
 * - Returns      : 
 ******************************************************************************/
A_BOOL updateSingleEepromValueAR5513(
	A_UINT32 devNum,
	A_UINT32 address,
	A_UINT32 value)
{
	A_UINT32 boardData[0x4000];
	A_UINT32 boardDataSize = 0x4000;
	A_UINT32 tmpVal;

	if (!autoSized) {
		autoSized = eepromAutoSizeAr5513(devNum);
	}

	if (!flashReadSectorAr5513(devNum, (lastSectorNum), &(boardData[0]), boardDataSize)) {
		printf("updateMacAddrAR5513 : Could not read data sector for single eeprom value. Exiting\n");
		return(FALSE);
	}

	tmpVal = boardData[address/2];
	if (address & 0x1) { // odd address
		boardData[address/2] = (tmpVal & 0xFFFF0000) | (value & 0xFFFF);
	} else {
		boardData[address/2] = (tmpVal & 0x0000FFFF) | ((value & 0xFFFF) << 16);
	}

	if (!flashWriteSectorAr5513(devNum, (lastSectorNum), &(boardData[0]), boardDataSize-4)) { // since boarddata is less than 152 bytes
		printf("updateMacAddrAR5513 : Could not write data sector for single eeprom value. Exiting\n");
		return(FALSE);
	}

	return(TRUE);
}


A_BOOL updateSectorChunkAR5513 
(
	A_UINT32 devNum,
	A_UINT32 *pData, // intended for pci config data
	A_UINT32 sizeData,
	A_UINT32 sectorNum,
	A_UINT32 dataOffset
)
{
	A_UINT32 sectorData[0x4000];
	A_UINT32 sectorDataSize = 0x4000;
	A_UINT32 ii;

	if ((sizeData + dataOffset) > sectorDataSize) {
		printf("updateSectorChunkAR5513 : update data runs past sector boundary [offset=%d, size=%d, sectorSize=%d]\n", dataOffset, sizeData, sectorDataSize);
		return(FALSE);
	}

	if (!autoSized) {
		autoSized = eepromAutoSizeAr5513(devNum);
	}

	if (sectorNum > lastSectorNum) {
		printf("updateSectorChunkAR5513 : update data sectorNum [%d] > lastSectorNum [%d]\n", sectorNum, lastSectorNum);
		return(FALSE);
	}

	if (!flashReadSectorAr5513(devNum, sectorNum, &(sectorData[0]), sectorDataSize)) {
		printf("updateSectorChunkAR5513 : Could not read data from sector %d. Exiting\n", sectorNum);
		return(FALSE);
	}

	for (ii=0; ii<sizeData; ii++) {
		sectorData[dataOffset+ii] = pData[ii];
	}

	if (!flashWriteSectorAr5513(devNum, sectorNum, &(sectorData[0]), sectorDataSize)) {
		printf("updateSectorChunkAR5513 : Could not write board data sector. Exiting\n");
		return(FALSE);
	}

	return(TRUE);
}

