/*
 *  Copyright © 2005 Atheros Communications, Inc.,  All Rights Reserved.
 *
 */

#ident  "ACI $Id: //depot/sw/src/dk/mdk/devlib/ar5416/mCfg5416.c#65 $, $Header: //depot/sw/src/dk/mdk/devlib/ar5416/mCfg5416.c#65 $"

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
#include <stdlib.h>
#ifndef VXWORKS
#include <malloc.h>
#endif
#include "wlantype.h"
#include "ar5416Addac.h"
#include "ar5416Addac2_2.h"
#include "ar5416Addac_sowl.h"
#include "ar9280PciePhy_merlin.h"
#include "ar5416Addac_sowl1_1.h"

#ifdef OWL_PB42
#include "ar5416Addac_howl.h"
#endif

#include "ar5416reg.h"
#include "mEep5416.h"
#include "mEepStruct5416.h"
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

extern A_UINT32 gFreqMHz; /* from mConfig.c, used in merlin's pll setting */

#ifdef OWL_AP
A_UINT8 sysFlashConfigRead(A_UINT32 devNum, int fcl , int offset);
void sysFlashConfigWrite(A_UINT32 devNum, int fcl, int offset, A_UINT8 *data, int len);
#define FLC_RADIOCFG    2
#endif

#define NUM_ADDAC_WRITES_5416 (sizeof(ar5416Addac) / sizeof(*(ar5416Addac)))
#define NUM_ADDAC_WRITES_HOWL (sizeof(ar5416Addac_howl) / sizeof(*(ar5416Addac_howl)))
#define NUM_ADDAC_WRITES_5416_2_2 (sizeof(ar5416Addac2_2) / sizeof(*(ar5416Addac2_2)))
#define NUM_ADDAC_WRITES_5416_SOWL (sizeof(ar5416Addac_sowl) / sizeof(*(ar5416Addac_sowl)))
#define NUM_PCIEPHY_WRITES_9280_MERLIN (sizeof(ar9280PciePhy_clkreq_always_on_L1_merlin) / sizeof(*(ar9280PciePhy_clkreq_always_on_L1_merlin)))
A_UINT16 dummyFlashData[1024];

A_UINT16 xpa2biaslvlfreq[3][2]= {{0,0},{0,0},{0,0}};
A_UINT16 xpa5biaslvlfreq[3][2]= {{0,0},{0,0},{0,0}};

A_UINT16 freq2fbin(A_UINT16 freq);


/* Reset the mac */
void hwResetAr5416
(
    A_UINT32 devNum,
    A_UINT32 resetMask
)
{
#ifdef OWL_PB42
if(isHowlAP(devNum))
 {
     mSleep(10);
  hwRtcRegWrite (devNum,0x004c,0x3); 

  mSleep(10);
  hwRtcRegWrite(devNum,0x0040,0x0);
  mSleep(10);
  hwRtcRegWrite(devNum,0x0040,0x5);
  mSleep(10);
  while(hwRtcRegRead (devNum,0x0044) != 0x2){
     printf("<");
  }

  mSleep(10);
  hwRtcRegWrite(devNum,0x0000,0xf); 

  mSleep(10);
  hwRtcRegWrite(devNum,0x0000,0x0);
  mSleep(10);
  while(hwRtcRegRead (devNum,0x0000) != 0x0){
     printf("<");
     mSleep(1000);
  }

 }
 else
 {
#endif

#define MAX_RESETS_FOR_ID 6
    LIB_DEV_INFO    *pLibDev = gLibInfo.pLibDevArray[devNum];
    int i;
	int phy_id = 0;
    pLibDev->devMap.OScfgWrite(devNum, 0x40, 0x0);      // Disable Retry timeout

    // If this is Sowl 1.1, copy addac.h file to ar5416Addac_sowl array
    if(((pLibDev->macRev)&0x100) == 0x100) { // check only revision number bit
      memcpy(ar5416Addac_sowl, ar5416Addac_sowl1_1, sizeof(ar5416Addac_sowl));
    }


    for (i = 0; i < MAX_RESETS_FOR_ID; i++) {
        /* Reset sequence as of 11/15/2005 */
        REGW(devNum, 0x4000, 0x1);                          // Issue AHB reset
		mSleep(10);
        /* Reset RTC and poll for complete */
        REGW(devNum, 0x704C, 0x3);                          // force wake
		mSleep(10);
        if (pLibDev->generatedMerlinGainTable && isMerlin_Eng(pLibDev->macRev))
        {
#if (0) /* BAA: bypassing the RTC is causing problems */
                //merlin gain table gets generated right before ART main menu,
                //so if have gotten to the main menu and merlin 1.0 not longer 
                //want to do the RTC reset
//            printf("SNOOP: bypassing the RTC for merlin 1.0\n");
#endif            
            REGW(devNum, 0x7040, 0x0);                          // Force RTC reset
			mSleep(10);
            REGW(devNum, 0x7040, 0x1);                          // unreset RTC
        } else {
            REGW(devNum, 0x7040, 0x0);                          // Force RTC reset
			mSleep(10);
            REGW(devNum, 0x7040, 0x1);                          // unreset RTC
        }
		while (REGR(devNum, 0x7044) != 0x2) {

			mSleep(1);
        }

        /* Warm Reset MAC and poll for complete */
        REGW(devNum, 0x7000, 1);                            // reset MAC
		mSleep(10);
        REGW(devNum, 0x7000, 0);                            // Clear Cold and Warm Resets to chip and MAC
        while (REGR(devNum, 0x7000) != 0) {                 // Poll for reset clear
            mSleep(1);
        }

        REGW(devNum, 0x4000, 0);                            // Clear AHB/APB reset
		mSleep(10);
        REGW(devNum, 0x704C, 0x3);                          // force wake
		mSleep(10);
		if(!isSowl_Emulation(pLibDev->macRev) ||
		   !isMerlin_Fowl_Emulation(pLibDev->macRev)) {
            phy_id = REGR(devNum, PHY_CHIP_ID) & 0xF0;

            if((phy_id == 0x80) || (phy_id == 0xb0) || (phy_id == 0xd0)) { /* BAAmerlin: 0xd0 is for merlin */
				break;
			}
		} else {
			printf("SOWL or Merlin_Fowl Emulation is reset!\n");
			break;
		}
    }

    if (i == MAX_RESETS_FOR_ID) {
        printf("Failed to read valid PHY_CHIP_ID after %d resets\n", MAX_RESETS_FOR_ID);
        return;
    }

	if(isMerlin(pLibDev->swDevID) && isMerlinPcie(pLibDev->macRev)) {
	   /*
		* Disable PLL when in L0s as well as receiver clock when in L1.
		* This power saving option must be enabled through the Serdes.
		*
		* Programming the Serdes must go through the same 288 bit serial shift
		* register as the other analog registers.  Hence the 9 writes.
		*
		*/
		/* RX shut off when elecidle is asserted */
		/* CLKREQ activivity is enabled in L1 mode, bit 128 is 1, power consumption is little high */
	    for( i = 0; i < NUM_PCIEPHY_WRITES_9280_MERLIN; i++ ) {
              REGW(devNum, ar9280PciePhy_clkreq_always_on_L1_merlin[i][0], ar9280PciePhy_clkreq_always_on_L1_merlin[i][1]);
//			  printf(" Address is 0x%x, Value is 0x%x\n", ar9280PciePhy_clkreq_always_on_L1_merlin[i][0], ar9280PciePhy_clkreq_always_on_L1_merlin[i][1]);
        }

		mSleep(100);

		/* set bit 19 to allow forcing of pcie core into L1 state */
		REGW(devNum, 0x4014, REGR(devNum, 0x4014) | (1 << 19));
		
	}

    /* Disable PCIE PHY if PCI device for 2.X+ */
    if (IS_MAC_5416_2_0_UP(pLibDev->macRev) && ((REGR(devNum, F2_SREV) & 0xF0) == 0xD0)) {
	    REGW(devNum, 0x4040, 0x9248fc00);
        REGW(devNum, 0x4040, 0x24924924);
        REGW(devNum, 0x4040, 0x28000029);
        REGW(devNum, 0x4040, 0x57160824);
        REGW(devNum, 0x4040, 0x25980579);
        REGW(devNum, 0x4040, 0x0);
        REGW(devNum, 0x4040, 0x1aaabe40);
        REGW(devNum, 0x4040, 0xbe105554);
        REGW(devNum, 0x4040, 0x000e1007);
        REGW(devNum, 0x4044, 0x0);
    }
    resetMask = 0; // quiet ye old compiler

#ifdef OWL_PB42
 }// end of isHowlAP
#endif

}

void pllProgramAr5416
(
    A_UINT32 devNum,
    A_UINT32 turbo
)
{
    LIB_DEV_INFO    *pLibDev = gLibInfo.pLibDevArray[devNum];
    int i;
    int temp;

    turbo = 0; // quiet ye old compiler

    //printf(" pLibDev->libCfgParams.chainSelect is %d\n", pLibDev->libCfgParams.chainSelect);
    if( isSowl_Emulation(pLibDev->macRev) || isMerlin_Fowl_Emulation(pLibDev->macRev) ) {
        /* Change Hainan PLL to run at 24 MHz - not really full speed ofdm or cck/ofdm operation */
        if( pLibDev->libCfgParams.chainSelect >= 1 ) {
            REGW(devNum, 0xd47c, 0xe6);
    } else {
//        REGW(devNum, 0xd87c, 0xe6);
            REGW(devNum, 0xd47c, 0xe6);
        }
        mSleep(1000);

        printf("PLL reg 0xd47c is 0x%x\n", REGR(devNum, 0xd47c));

        REGW(devNum, 0x404c, 0x30);                      // Enable GPIO2 to be output
        REGW(devNum, 0x4048, 0x0);
        REGW(devNum, 0x4048, 0x4);                       // Pulse GPIO2 to cause DCM Reset
        mSleep(10);
        REGW(devNum, 0x4048, 0x0);
        mSleep(100);

        temp = (REGR(devNum, 0x74) >> 2) & 0x1;
        while( temp == 0x0 ) {
            temp = (REGR(devNum, 0x74) >> 2) & 0x1;  //Wait for DCM Release
        }


        printf("FPGA Rev ID = 0x%08x\n", REGR(devNum, 0x0070));
        printf("MAC  Rev ID = 0x%08x\n", REGR(devNum, 0x4020));
        printf("BB   Rev ID = 0x%08x\n", REGR(devNum, 0x9818));

        /* Setup external Hainan baseband chains */
        printf("SNOOP: CHAIN0 : Starting External Hainan Register Initialization ========\n");
        printf("SNOOP: Ext BB Rev ID = 0x%08x\n", REGR(devNum, 0xd818));

        REGW(devNum, 0xd400, 0x00000047);
        REGW(devNum, 0xd404, 0x00000000);    // Just turbo and short20 in this new register
        REGW(devNum, 0xd408, 0x00000502);    // TX Source is TSTDAC and enable TSTADC
        REGW(devNum, 0xd40c, 0x0b849233);
        REGW(devNum, 0xd410, 0x3d32e000);
        REGW(devNum, 0xd414, 0x0000076b);
        REGW(devNum, 0xd420, 0x04040402);
        REGW(devNum, 0xd424, 0x00000e0e);
        REGW(devNum, 0xd428, 0x0a020201);
        REGW(devNum, 0xd42c, 0x00020002);    // DAC and ADC always ON
        REGW(devNum, 0xd430, 0x00000000);
        REGW(devNum, 0xd434, 0x00000e0e);
        REGW(devNum, 0xd438, 0x0000c003);
        REGW(devNum, 0xd440, 0xaaa86420);
        REGW(devNum, 0xd444, 0x137216ad);
        REGW(devNum, 0xd448, 0x0018fab3);
        REGW(devNum, 0xd44c, 0x1284613c);
        REGW(devNum, 0xd450, 0x0de8a8d5);
        REGW(devNum, 0xd454, 0x00074859);
        REGW(devNum, 0xd458, 0x7e80bd3a);
        REGW(devNum, 0xd45c, 0x3277665e);
        REGW(devNum, 0xd460, 0x0000b910);
        REGW(devNum, 0xd464, 0x000347a6);
        REGW(devNum, 0xd468, 0x409a4190);
        REGW(devNum, 0xd46c, 0x050cb481);    // Enable Low SNR detection

        REGW(devNum, 0xd470, 0x00000000);
        REGW(devNum, 0xd474, 0x00000080);
        // REGW(devNum, 0xd478, 0x00000008);    // DAC and ADC clock select
        REGW(devNum, 0xd478, 0x00000004);    // DAC and ADC clock select

        REGW(devNum, 0xd51c, 2346);          // reg 71
        REGW(devNum, 0xd520, 0x00000000);    // reg 72

        REGW(devNum, 0xd524, 0x000a8f15);    // reg 73
        REGW(devNum, 0xd528, 0x00000001);    // reg 74 (phy warm reset deasserted)
        REGW(devNum, 0xd52c, 0x00000000);    // reg 75                       36=18dBm
        REGW(devNum, 0xd530, 0x00004883);    // reg 76; rrrr rrr- ---- ---- 0 100100 010 00001 1, probe off
        REGW(devNum, 0xd534, 0x1e1f2022);    // reg 77; 30,31,32,34 = 15.0, 15.5, 16.0, 17.0dBm
        REGW(devNum, 0xd538, 0x1a1b1c1d);    // reg 78; 26,27,28,29 = 13.0, 13.5, 14.0, 14.5dBm
        REGW(devNum, 0xd53c, 0x0000003f);    // reg 79; 33 = 16.5dBm
        REGW(devNum, 0xd540, 0x00000004);    // reg 80; time4_rd_gainf delay

        // REGW(devNum, 0xd944, 0x4fe010e0);    // reg 81; Previous reg 1
        REGW(devNum, 0xd544, 0x4fe01040);    // reg 81; Previous reg 1
        REGW(devNum, 0xd548, 0x00000000);    // reg 82; Service 0
        REGW(devNum, 0xd54c, 0x00000000);    // reg 83; Service 1
        REGW(devNum, 0xd550, 0x00000000);    // reg 84; Service 2
        REGW(devNum, 0xd554, 0x5d50f14c);    // reg 85; Radar detection

        REGW(devNum, 0xd50c, 0x00000000);    // reg 67

        REGW(devNum, 0xd41c, 0x00000001);    // Activate D2
        for( i=0; i<100; i++ ) {
            REGR(devNum, 0x4000); 
        }

        printf("0xd408 = 0x%08x\n", REGR(devNum, 0xd408));
        printf("0xd42c = 0x%08x\n", REGR(devNum, 0xd42c));
        printf("SNOOP: CHAIN0 : Completed External Hainan Register Initialization ========\n");
  } else {

#ifdef OWL_PB42
if(isHowlAP(devNum))
  {

            /* Modally Program the PLL */
            hwRtcRegWrite(devNum,0x0018,2000);   // Set the PLL settling delay
            if( pLibDev->mode == MODE_11A ) { /* TODO: Expand if() to cover 11A 20/40 mode */
                hwRtcRegWrite(devNum, 0x0014, 0x1450);
    } else {
                hwRtcRegWrite(devNum, 0x0014, 0x1458);
            }
            mSleep(1);

            /* Set the ADC/DAC bit shift chain */
            REGW(devNum, 0x9830, 1);  // Enable ADDAC shifting instead of analog shifting



     // addac programming for Howl 1.2; for 1.0 we need to change the addac file
        for (i = 0; i < NUM_ADDAC_WRITES_HOWL; i++) {
            REGW(devNum, ar5416Addac_howl[i][0], ar5416Addac_howl[i][1]);
        }

  }
else  //isHowlAP
 {

#endif

            /* Modally Program the PLL */
            REGW(devNum, 0x7018, 2000); // Set the PLL settling delay

            /* for merlin fast clock applies to 
             * 1. any 5MHz spacing channel in the 5G band 
             * 2. all 5G band channels if the fastClk5g flag equals "1" in the eeprom 
             */
            if( isMerlin(pLibDev->swDevID) ) {
                if( gFreqMHz > 4000 /* pLibDev->mode == MODE_11A */ ) {
                    if( (((gFreqMHz % 20) == 0) || ((gFreqMHz % 10) == 0)) && (!pLibDev->libCfgParams.fastClk5g)) {
                        REGW(devNum, 0x7014, 0x2850);
                        writeField(devNum, "bb_dyn_ofdm_cck_mode", 0x0);
                        writeField(devNum, "bb_disable_dyn_cck_det", 0x0);
/*                      printf(">>>FAST_CLK_MODE OFF  %d, mode %d, fastClk5g %d\n",      */
/*                             gFreqMHz, pLibDev->mode, pLibDev->libCfgParams.fastClk5g);*/
                    } else { /* apply 2G mode to improves EVM */
                        REGW(devNum, 0x7014, 0x142c);
                        writeField(devNum, "bb_dyn_ofdm_cck_mode", 0x1);
                        writeField(devNum, "bb_disable_dyn_cck_det", 0x1);
/*                      printf(">>>FAST_CLK_MODE ON  %d, mode %d, fastClk5g %d\n",       */
/*                             gFreqMHz, pLibDev->mode, pLibDev->libCfgParams.fastClk5g);*/
                    }
                } else {
/*                  printf(">>>freq %d, mode %d, fastClk5g %d\n",                    */
/*                         gFreqMHz, pLibDev->mode, pLibDev->libCfgParams.fastClk5g);*/
                    REGW(devNum, 0x7014, 0x142c);
                    writeField(devNum, "bb_dyn_ofdm_cck_mode", 0x1);
                    writeField(devNum, "bb_disable_dyn_cck_det", 0x0);
                }
            } else if( isSowl(pLibDev->macRev) ) {
                if( pLibDev->mode == MODE_11A ) {
                    REGW(devNum, 0x7014, 0x1450);
                } else {
                    REGW(devNum, 0x7014, 0x1458);
                }
            } else {
                if( pLibDev->mode == MODE_11A ) { /* TODO: Expand if() to cover 11A 20/40 mode */
                    REGW(devNum, 0x7014, 0x00EA);
                } else {
                    REGW(devNum, 0x7014, 0x00EB);
                }
            }
            mSleep(50);

            /* Set the ADC/DAC bit shift chain (except for Merlin) */
   if( !isMerlin(pLibDev->swDevID) ) {
                REGW(devNum, 0x9830, 1);  // Enable ADDAC shifting instead of analog shifting

     if( (pLibDev->macRev & 0xff) == (0xff) ) {
           // This is new chip -- check for sowl
       if( isSowl(pLibDev->macRev) ) {
                        // addac programming for sowl
          // Look eeprom xpa bias levels if eeprom is loaded
          A_UINT8 biaslevel;
          if(pLibDev->eepData.eepromChecked == 1) {
                if(pLibDev->freqForResetDevice != 0) {
                    MODAL_EEP_HEADER modalHeader;
                    modalHeader = pLibDev->ar5416Eep->modalHeader[pLibDev->mode];
                    if(modalHeader.xpaBiasLvl != 0xff) { 
                      biaslevel = modalHeader.xpaBiasLvl;
                    } else {
                      // Use freqeuncy specific xpa bias level
                      A_UINT16 resetFreqBin, freqBin, freqCount=0;

                      resetFreqBin = freq2fbin((A_UINT16)pLibDev->freqForResetDevice);
                      freqBin = (A_UINT16) (0xff & modalHeader.xpaBiasLvlFreq[0]);
                      biaslevel = (A_UINT8) (modalHeader.xpaBiasLvlFreq[0] >> 14);
                      freqCount++;
                      while(freqCount<3) {
                        if(modalHeader.xpaBiasLvlFreq[freqCount] == 0x0) break;
                        freqBin = (A_UINT16) (0xff & modalHeader.xpaBiasLvlFreq[freqCount]);
                        if(resetFreqBin >= freqBin) {
                          biaslevel = (A_UINT8)  (modalHeader.xpaBiasLvlFreq[freqCount] >> 14);
                        } else {
                          break;
                        }
                        freqCount++;
                      }
                    } // xpaBiasLvl
          
                    // apply bias level in addac register
                    if(pLibDev->mode == 0) {
                    // 5G
                        ar5416Addac_sowl[6][1] = (ar5416Addac_sowl[6][1] & (~0xc0)) | biaslevel << 6;
                    } else {
                        // 2G 
                        ar5416Addac_sowl[7][1] = (ar5416Addac_sowl[7][1] & (~0x18)) | biaslevel << 3;
                    }

                }  // freqForResetDevice
          } else {
              // look into xpabiaslvlfreq array to check for frequency dependent bias level
              A_UINT16 freqCount;
              if(pLibDev->mode == 0) {
                // 5G
                if(xpa5biaslvlfreq[0][0] != 0) {
                  biaslevel = (A_UINT8) xpa5biaslvlfreq[0][1];
                  freqCount = 1;
                  while(freqCount<3) {
                      if(pLibDev->freqForResetDevice >= xpa5biaslvlfreq[freqCount][0]) {
                         biaslevel = (A_UINT8) xpa5biaslvlfreq[freqCount][1];
                      } else {
                          break;
                      }
                      freqCount++;
                  }
                  ar5416Addac_sowl[6][1] = (ar5416Addac_sowl[6][1] & (~0xc0)) | biaslevel << 6;
                }

              } else if(pLibDev->mode == 1) {
                  //2G
                if(xpa2biaslvlfreq[0][0] != 0) {
                  biaslevel = (A_UINT8) xpa2biaslvlfreq[0][1];
                  freqCount = 1;
                  while(freqCount<3) {
                      if(pLibDev->freqForResetDevice >= xpa2biaslvlfreq[freqCount][0]) {
                         biaslevel = (A_UINT8) xpa2biaslvlfreq[freqCount][1];
                      } else {
                          break;
                      }
                      freqCount++;
                  }
                  ar5416Addac_sowl[7][1] = (ar5416Addac_sowl[7][1] & (~0x18)) | biaslevel << 3;
                }
              } // end of mode

          }  // open/close loop

          for (i = 0; i < NUM_ADDAC_WRITES_5416_SOWL; i++) {
            REGW(devNum, ar5416Addac_sowl[i][0], ar5416Addac_sowl[i][1]);
		  }			
		} else {
			printf("ERROR:: not SOWL\n");
		}
	} else {
      if (((pLibDev->macRev & 0x7) >= 2) ){
        for (i = 0; i < NUM_ADDAC_WRITES_5416_2_2; i++) {
            REGW(devNum, ar5416Addac2_2[i][0], ar5416Addac2_2[i][1]);
        }
	  } else {
        for (i = 0; i < NUM_ADDAC_WRITES_5416; i++) {
            REGW(devNum, ar5416Addac[i][0], ar5416Addac[i][1]);
		}
	  }
	}
#ifdef OWL_PB42
        } // end of isHowlAP
#endif

                REGW(devNum, 0x9830, 0);  // Reset to analog shifting
     }
   } /* if( !isMerlin(pLibDev->swDevID) ) */
}

#define EEPROM_MEM_OFFSET  0x2000
#define EEPROM_STATUS_READ 0x407c
#define EEPROM_BUSY_M      0x10000
/**************************************************************************
* eepromReadAr5416 - Read from given EEPROM offset and return the 16 bit value
*
* RETURNS: 16 bit value from given offset (in a 32-bit value)
*/
A_UINT32 eepromReadAr5416
(
 A_UINT32 devNum, 
 A_UINT32 eepromOffset
)
{


#ifdef OWL_PB42
{//for Howl
   if(isHowlAP(devNum) || isFlashCalData()){

        A_UINT32 eepromValue;

//      16 bit addressing
        eepromOffset = eepromOffset << 1;
        eepromValue = (sysFlashConfigRead(devNum, FLC_RADIOCFG, eepromOffset) << 8) |
                       sysFlashConfigRead(devNum, FLC_RADIOCFG, (eepromOffset + 1));
//      q_uiPrintf("Read from eeprom @ %x : %x \n",(eepromOffset >> 1),eepromValue);
        return eepromValue;

   }

}//for Howl
#endif


#if defined(OWL_AP) && (!defined(OWL_OB42))

        A_UINT32 eepromValue;
         
//      16 bit addressing
        eepromOffset = eepromOffset << 1;
#ifdef ARCH_BIG_ENDIAN
        eepromValue = (sysFlashConfigRead(devNum, FLC_RADIOCFG, eepromOffset) << 8) |
                       sysFlashConfigRead(devNum, FLC_RADIOCFG, (eepromOffset + 1));
#else 
	        eepromValue = (sysFlashConfigRead(devNum, FLC_RADIOCFG, eepromOffset+1) << 8) |
	                      (sysFlashConfigRead(devNum, FLC_RADIOCFG, eepromOffset));
#endif
//      q_uiPrintf("Read from eeprom @ %x : %x \n",(eepromOffset >> 1),eepromValue);
        return eepromValue;
#endif
#if defined(OWL_AP) && (!defined(OWL_OB42))
    return(dummyFlashData[eepromOffset]);
#else  
    int         to = 50000;
    A_UINT32    eepromValue;
    A_UINT32    status;

    //read the memory mapped eeprom location
    eepromValue = REGR(devNum, (EEPROM_MEM_OFFSET + (eepromOffset*4)));

    //check busy bit to see if eeprom read succeeded and get valid data in read register
    while (to > 0) {
        status = REGR(devNum, EEPROM_STATUS_READ) & EEPROM_BUSY_M;
        if (status == 0) {
            eepromValue = REGR(devNum, EEPROM_STATUS_READ) & 0xffff;
            return(eepromValue);
        }

        to--;
    }

    mError(devNum, EIO, "eepromReadar5416: eeprom read timeout on offset %d!\n", eepromOffset);
    return (0xdead); 
#endif //OWL_AP
#if 0

    int         to = 50000;
    A_UINT32    status;
    A_UINT32    eepromValue;
    LIB_DEV_INFO    *pLibDev = gLibInfo.pLibDevArray[devNum];

    //write the address
    REGW(devNum, F2_EEPROM_ADDR, eepromOffset);

    //issue read command
    REGW(devNum, F2_EEPROM_CMD, F2_EEPROM_CMD_READ);

    //wait for read to be done
    while (to > 0) {
        status = REGR(devNum, F2_EEPROM_STS);
        if (status & F2_EEPROM_STS_READ_COMPLETE) {
            //read the value
            eepromValue = REGR(devNum, F2_EEPROM_DATA) & 0xffff;
            return(eepromValue);
        }

        if(status & F2_EEPROM_STS_READ_ERROR) {
            mError(devNum, EIO, "eepromRead_AR5516: eeprom read failure on offset %d!\n", eepromOffset);
            return 0xdead;
        }
        to--;
    }
    mError(devNum, EIO, "eepromRead_AR5416: eeprom read timeout on offset %d!\n", eepromOffset);
    return 0xdead;
#endif
}


void eepromWriteAr5416
(
 A_UINT32 devNum, 
 A_UINT32 eepromOffset, 
 A_UINT32 eepromValue
)
{
 
    LIB_DEV_INFO  *pLibDev = gLibInfo.pLibDevArray[devNum];
    A_UINT16 *eepPtr;
    
    // Update current copy of eeprom data
    if(pLibDev->ar5416Eep) {
        eepPtr = (A_UINT16 *)pLibDev->ar5416Eep;
	eepPtr[eepromOffset - 0x100] = (A_UINT16)eepromValue;
    }


#ifdef OWL_PB42
{//is Howl

     if(isHowlAP(devNum) || isFlashCalData()){

            A_UINT8 *pVal;

            pVal = (A_UINT8 *)&eepromValue;

        // 16 bit addressing
        //      q_uiPrintf("Write to eeprom @ %x : %x \n",eepromOffset,eepromValue);
            eepromOffset = eepromOffset << 1;
        // write only the lower 2 bytes
            sysFlashConfigWrite(devNum, FLC_RADIOCFG, eepromOffset, (pVal+2) , 2);
            return;



     }
}//for Howl
#endif

#if defined(OWL_AP) && (!defined(OWL_OB42))

            A_UINT8 *pVal;

            pVal = (A_UINT8 *)&eepromValue;

        // 16 bit addressing
        //      q_uiPrintf("Write to eeprom @ %x : %x \n",eepromOffset,eepromValue);
            eepromOffset = eepromOffset << 1;
        // write only the lower 2 bytes
#ifdef ARCH_BIG_ENDIAN
            sysFlashConfigWrite(devNum, FLC_RADIOCFG, eepromOffset, (pVal+2) , 2);
#else 
	sysFlashConfigWrite(devNum, FLC_RADIOCFG, eepromOffset, pVal , 2); // RSP-ENDIAN writing only lower bytes does not work here since we are little-endian
#endif
            return;
#endif
#if defined(OWL_AP) && (!defined(OWL_OB42))
    dummyFlashData[eepromOffset] = eepromValue;
#else
    
    A_UINT32    status;
    A_UINT32 to = 50000;

    //gpio configuration, set GPIOs output to value set in output reg
    REGW(devNum, 0x4054, REGR(devNum, 0x4054) | 0x20000); 
	REGW(devNum, 0x4060, 0);
    REGW(devNum, 0x4064, 0);
//  mSleep(1);
    //GPIO3 always drive output
	REGW(devNum, 0x404c, 0xc0);  
//  mSleep(1);
    //Set 0 on GPIO3
	REGW(devNum, 0x4048, 0x0);
//  mSleep(1);

// printf("SNOOP: in eeprom write, eepromOffset = %x, register = %x, value = %x\n",
//     eepromOffset, (EEPROM_MEM_OFFSET + (eepromOffset*4)), eepromValue);
    //write to the memory mapped eeprom location        
    REGW(devNum, (EEPROM_MEM_OFFSET + (eepromOffset*4)), eepromValue);

    //check busy bit to see if eeprom write succeeded
    while (to > 0) {
        status = REGR(devNum, EEPROM_STATUS_READ) & EEPROM_BUSY_M;
        if (status == 0) {
            return;
        }

        to--;
    }

    mError(devNum, EIO, "eepromWritear5416: eeprom write timeout on offset %d!\n", eepromOffset);
#endif //OWL_AP
    return; 


#if 0
    int         to = 50000;
    A_UINT32    status;
    LIB_DEV_INFO    *pLibDev = gLibInfo.pLibDevArray[devNum];

    //write the address
    REGW(devNum, F2_EEPROM_ADDR, eepromOffset);

    //write the value
    REGW(devNum, F2_EEPROM_DATA, eepromValue);

    //issue write command
    REGW(devNum, F2_EEPROM_CMD, F2_EEPROM_CMD_WRITE);

    //wait for write to be done
    while (to > 0) {
        status = REGR(devNum, F2_EEPROM_STS);
        if (status & F2_EEPROM_STS_WRITE_COMPLETE) {
            return;
        }

        if(status & F2_EEPROM_STS_WRITE_ERROR) {
            mError(devNum, EIO, "eepromWrite:ar5416 eeprom write failure on offset %d!\n", eepromOffset);
            return;
        }
        to--;
    }

    mError(devNum, EIO, "eepromWritear5416: eeprom write timeout on offset %d!\n", eepromOffset);
    return;
#endif
}

/***********************************************************************************
 * setChannelAr5416 - AR5413 style channel change with data across two serial lines
 *
 */
A_BOOL setChannelAr5416(
 A_UINT32 devNum,
 A_UINT32 freq)
{
    A_UINT32 channelSel = 0;
    A_UINT32 bModeSynth = 0;
    A_UINT32 aModeRefSel = 0;
    A_UINT32 regVal = 0;
    LIB_DEV_INFO    *pLibDev = gLibInfo.pLibDevArray[devNum];

    if(freq < 4800) {
        if (((freq - 2272) % 5) == 0) {
            channelSel = (16 + (freq - 2272) / 5);

            bModeSynth = 0;
            channelSel = (channelSel << 2) & 0xff;
        } else if (((freq - 2274) % 5) == 0) {
            channelSel = (10 + (freq-2274)/5);

            bModeSynth = 1;
            channelSel = (channelSel << 2) & 0xff;
        } else if ((freq >= 2272) && (freq <= 2527)) {
            channelSel = freq - 2272;
            bModeSynth = 0;    //this is a don't care
        } else {
            mError(devNum, EINVAL, "%d is an illegal derby driven channel\n", freq);
            return(0);
        }
    
        channelSel = reverseBits(channelSel, 8);
        if(freq == 2484) {
            REGW(devNum, 0xa204, REGR(devNum, 0xa204) | 0x10);
        }
    } else {
        switch(pLibDev->libCfgParams.refClock) {
        case REF_CLK_2_5:
            channelSel = (A_UINT32)((freq-4800)/2.5);
            if (pLibDev->channelMasks & QUARTER_CHANNEL_MASK) {
                channelSel += 1;
            }
            channelSel = reverseBits(channelSel, 8);
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

        case REF_CLK_DYNAMIC:
        default:
            if (pLibDev->channelMasks & QUARTER_CHANNEL_MASK) {
                channelSel = reverseBits((A_UINT32)((freq-4800)/2.5 + 1), 8);
                aModeRefSel = reverseBits(0, 2);                
            } else if ( (freq % 20) == 0 ){
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
        }
    }

//    printf("SNOOP: aModeRefSel = %d\n", reverseBits(aModeRefSel, 2));
//    printf("SNOOP: channelSel (after reverse) = %x (%x)\n", channelSel, reverseBits(channelSel, 8));

    regVal = (channelSel << 8)  | // Channel Select in column 1 
             (aModeRefSel << 2) | // AmodeRef bits 2 & 3
             (bModeSynth << 1)  | // bModeSynth bit 1
             (1 << 5)           | // Bank 4 ID (bits 7-5 = 3b001)
             1;                   // Perform Channel Update (bit 0)
    REGW(devNum, 0x9800 + (0x37<<2), regVal);

    mSleep(1);
    return(1);
}
/* setChannelAr5416 */

/**
 * - Function Name: setChannelSynthAR928x
 * - Description  : set the synthesizer. Synthesizer runs in integer-N mode 
 *   for 11a band.  For 11b/g band, synthesizer runs in fractional-N mode.
 * - Arguments
 *     -      
 * - Returns      : none
 ******************************************************************************/
#define REF_FREQ (40) /* MHz */
void setChannelSynthAR928x(
    A_UINT32 devNum,
    A_UINT32 freqMHz)
{
    A_UINT32 channelSel   = 0;
    A_UINT32 channelFrac  = 0;  
    A_UINT32 reg32        = 0;
    A_UINT32 synthSettle  = 100;/* settle depends on XTAL and Operation mode*/
    A_UINT32 fracMode;
    A_UINT32 aModeRefSel;
    A_UINT32 bMode;
    A_UINT32 refdiva      = 24;
    float    chanFreq     = (float)freqMHz;
    float    refClock     = (float)REF_FREQ;
    float    ndiv         = 0 ;
    float    temp         = 0 ;
    float    whole        = 0 ;
    LIB_DEV_INFO* pLibDev = gLibInfo.pLibDevArray[devNum];

/********************************************************************************/
/* This is a new register in .13 radio
/* reg 29: synth control (.13 radio interface)
/* bmode       rw  1'b0    1'b0  # [29]    2.4GHz band select
/* Fracmode    rw  1'b0    1'b0  # [28]    (reserved) band selection (japan mode)
/* AmodeRefSel rw  2'h0    2'h0  # [27:26] (reserved) A mode channel spacing sel
/* channel     rw  9'h0    9'h0  # [25:17] synthesizer channel select
/* chanFrac    rw  17'h0   17'h0 # [16:0]  synthesizer fractional channel input
/********************************************************************************/

    if (freqMHz < 4800) {  /* 2GHz band */
        fracMode    = 1;
        bMode       = 1;
        aModeRefSel = 0;
        chanFreq   *= 4; 
        refClock   *= 3;
        ndiv        = chanFreq / refClock;
        channelSel  = (unsigned int)ndiv;
        temp        = ndiv - (float)channelSel;
        channelFrac = (unsigned int)(temp * 0x20000);
    } else  {  /* 5GHz band */
        fracMode    = 0;
        bMode       = 0;
        if((freqMHz % 20) == 0) {
            aModeRefSel = 3;
        }
        else if( (freqMHz % 10) == 0 ) {
            aModeRefSel = 2;
        }
        else {//5MHz channels
            /* JW experiment */
            fracMode = 1;
            aModeRefSel = 0;
            refdiva = 1;
        }
         
        writeField(devNum, "an_synth9_refdiva", refdiva);

        chanFreq   *= 2;
        refClock   /=(refdiva>>aModeRefSel);
        refClock   *= 3;
        ndiv        = chanFreq / refClock;
        channelSel  = (unsigned int)ndiv & 0x1ff;

        if( fracMode == 0 ) {
        channelFrac = (((unsigned int)ndiv >> 9) << 10);
    }
        else {
             whole       = (float)channelSel;
//             printf("whole  %f\n", whole);
             channelFrac = (A_UINT32)((ndiv - whole)* (float)0x20000);
        }
    }

    /* read/modify/write to synthesizer register */
    reg32  = REGR(devNum, 0x9874);
    reg32 &= 0xc0000000;
    reg32  = reg32               | 
             (bMode << 29)       | 
             (fracMode << 28)    | 
             (aModeRefSel << 26) | 
             (channelSel << 17)  | 
             (channelFrac << 0);
    REGW(devNum, 0x9874, reg32);

#if (0) /* debug */    
    if( freqMHz < 4800 ) {
        printf(" Freq_RF is  %d(KHz)\n", freqMHz*1000);  
        printf(" Freq_ref is %.4f\n", refClock/3.0);
        printf(" ndiv is     0x%x\n", ndiv);
        printf(" chansel is  0x%x\n", channelSel);
        printf(" chanfrac is 0x%x\n", channelFrac);
        printf(" reg32 is    0x%08lx\n\n", reg32);
    }
    else {
        printf(" Freq_RF is  %d(KHz)\n", freqMHz*1000);
//        printf(" Freq_ref is %.4f\n", refClock/3.0);
        printf(" refdiva     0x%x\n", refdiva);
        printf(" chanFreq    %f\n", chanFreq);
        printf(" refClock    %f\n", refClock);
        printf(" ndiv is     %f\n", ndiv);
        printf(" bMode       0x%x\n", bMode);
        printf(" fracMode    0x%x\n", fracMode);
        printf(" aModeRefSel 0x%x\n", aModeRefSel);
        printf(" chansel is  0x%x\n", channelSel);
        printf(" chanfrac is 0x%x\n", channelFrac);
        printf(" reg32 is    0x%08lx\n\n", reg32);
    }
#endif    

    /* Wait for the frequency synth to settle */
    mSleep(2);
    return;
}
/* setChannelSynthAR928x */

/** 
 * - Function Name: setChannelAr928x
 * - Description  :
 * - Arguments
 *     - 
 * - Returns      :                                                                
 *******************************************************************************/
A_BOOL setChannelAr928x(  
    A_UINT32 devNum,
    A_UINT32 freqMHz)
{
    /* set the synthesizer */
    setChannelSynthAR928x(devNum, freqMHz);

    /* Enable channel spreading for channel 14 */
    if(freqMHz == 2484) {
        REGW(devNum, 0xa204, REGR(devNum, 0xa204) | 0x10);
    }

    /* 
     * There is an issue if the calibration starts before
     * the base band timeout completes.  This could result in the
     * rx_clear false triggering.  As a workaround we add delay an
     * extra BASE_ACTIVATE_DELAY usecs to ensure this condition
     * does not happen.
     */
    mSleep(1);

    return TRUE;
}
/* setChannelAr928x */

/**
 * - Function Name: forceAntennaTbl5416
 * - Description  : 
 * - Arguments
 *     -      
 * - Returns      : 
 ******************************************************************************/
MANLIB_API void forceAntennaTbl5416(
 A_UINT32 devNum,
 A_UINT16 *pAntennaTbl
)
{
    LIB_DEV_INFO        *pLibDev = gLibInfo.pLibDevArray[devNum];

    writeField(devNum, "bb_switch_table_com_b",     pAntennaTbl[0]);
    writeField(devNum, "bb_switch_table_com_r",     pAntennaTbl[1]);
    writeField(devNum, "bb_switch_table_com_t",     pAntennaTbl[2]);
    writeField(devNum, "bb_switch_table_com_idle",  pAntennaTbl[3]);

    writeField(devNum, "bb_chn0_switch_table_b",    pAntennaTbl[4]);
    writeField(devNum, "bb_chn0_switch_table_rx12", pAntennaTbl[5]);
    if(!isMerlin(pLibDev->swDevID)) {
        writeField(devNum, "bb_chn0_switch_table_rx2",  pAntennaTbl[6]);
    }
    writeField(devNum, "bb_chn0_switch_table_rx1",  pAntennaTbl[7]);
    writeField(devNum, "bb_chn0_switch_table_r",    pAntennaTbl[8]);
    writeField(devNum, "bb_chn0_switch_table_t",    pAntennaTbl[9]);
    writeField(devNum, "bb_chn0_switch_table_idle", pAntennaTbl[10]);

    writeField(devNum, "bb_chn1_switch_table_b",    pAntennaTbl[11]);
    writeField(devNum, "bb_chn1_switch_table_rx12", pAntennaTbl[12]);
    if(!isMerlin(pLibDev->swDevID)) {
        writeField(devNum, "bb_chn1_switch_table_rx2",  pAntennaTbl[13]);
    }
    writeField(devNum, "bb_chn1_switch_table_rx1",  pAntennaTbl[14]);
    writeField(devNum, "bb_chn1_switch_table_r",    pAntennaTbl[15]);
    writeField(devNum, "bb_chn1_switch_table_t",    pAntennaTbl[16]);
    writeField(devNum, "bb_chn1_switch_table_idle", pAntennaTbl[17]);

    if(!isMerlin(pLibDev->swDevID)) {
        writeField(devNum, "bb_chn2_switch_table_b",    pAntennaTbl[18]);
        writeField(devNum, "bb_chn2_switch_table_rx12", pAntennaTbl[19]);
        writeField(devNum, "bb_chn2_switch_table_rx2",  pAntennaTbl[20]);
        writeField(devNum, "bb_chn2_switch_table_rx1",  pAntennaTbl[21]);
        writeField(devNum, "bb_chn2_switch_table_r",    pAntennaTbl[22]);
        writeField(devNum, "bb_chn2_switch_table_t",    pAntennaTbl[23]);
        writeField(devNum, "bb_chn2_switch_table_idle", pAntennaTbl[24]);
    }
    
    return;
}

/**************************************************************************
* initPowerAr5416 - Set the power for the AR5416 chips
*
*/
void initPowerAr5416
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
    if (((pLibDev->eepData.version >> 12) & 0xF) == AR5416_EEP_VER) {
        /* AR5416 specific struct and function programs eeprom header writes (board data overrides) */
        ar5416EepromSetBoardValues(devNum, freq);

        /* AR5416 specific struct and function performs all tx power writes */      
        ar5416EepromSetTransmitPower(devNum, freq, NULL);
        return;
    } else {
        mError(devNum, EINVAL, "initPowerAR5416: Illegal eeprom version\n");
        return;
    }
    
    return;
}

/*****************************************************************************
* Function name: changeAddacField
* Input:         Addac field name and value
* Output:        None
* Description:   Changes addac field value by writing on specific bits of addac
*                field array.
*****************************************************************************/
MANLIB_API void changeAddacField
(
 A_UINT32		  devNum,
 PARSE_FIELD_INFO *pFieldToChange
)
{
    A_UINT32 macRev;
    A_UINT32 value;
    macRev = REGR(devNum, 0x4020);
    if(isSowl(macRev)) {
        if(strcmp(pFieldToChange->fieldName, "xpa2biaslvl") == 0) {
            value = atoi(pFieldToChange->valueString);
            // printf("TEST:: xpa2biaslvl value was %08x\n", ar5416Addac_sowl[7][1]);
            ar5416Addac_sowl[7][1] = (ar5416Addac_sowl[7][1] & (~0x18)) | value << 3;
            // printf("TEST:: new value is %08x\n", ar5416Addac_sowl[7][1]);

        } else if(strcmp(pFieldToChange->fieldName, "xpa5biaslvl") == 0) {
            value = atoi(pFieldToChange->valueString);
            // printf("TEST:: xpa5biaslvl value was %08x\n", ar5416Addac_sowl[6][1]);
            ar5416Addac_sowl[6][1] = (ar5416Addac_sowl[6][1] & (~0xc0)) | value << 6;
            // printf("TEST:: new value is %08x\n", ar5416Addac_sowl[6][1]);
        }
    } else {
        printf("This is not SOWL. changeAddacField is valid for Sowl only\n");
    }
}

/*****************************************************************************
* Function name: saveXpaBiasLvlFreq
* Input:         freq and xpa bias level
* Output:        None
* Description:   Save xpa bias level and frequency table.
*****************************************************************************/
MANLIB_API void saveXpaBiasLvlFreq
(
  A_UINT32		  devNum,
  PARSE_FIELD_INFO *pFieldToChange,
  A_UINT16        biasLevel
)
{
    A_UINT32 macRev;
    A_UINT16 i;
    macRev = REGR(devNum, 0x4020);

    if(isSowl(macRev)) {
        if(strncmp(pFieldToChange->fieldName, "xpa5biaslvlFreq", strlen("xpa5biaslvlFreq")) == 0) {
            i=0;
            while(i<3) {
                if(xpa5biaslvlfreq[i][0] == 0) break;
                i++;
            }
            if(i==3) {
            //    printf("Error from saveXpaBiasLvlFreq - at most 3 freq brackets can be supported\n");
                return;
            }
            xpa5biaslvlfreq[i][0] = (A_UINT16) atoi(pFieldToChange->valueString);
            xpa5biaslvlfreq[i][1] = biasLevel;

        } else if(strncmp(pFieldToChange->fieldName, "xpa2biaslvlFreq", strlen("xpa2biaslvlFreq")) == 0) {
            i=0;
            while(i<3) {
                if(xpa2biaslvlfreq[i][0] == 0) break;
                i++;
            }
            if(i==3) {
            //    printf("Error from saveXpaBiasLvlFreq - at most 3 freq brackets can be supported\n");
                return;
            }
            xpa2biaslvlfreq[i][0] = (A_UINT16) atoi(pFieldToChange->valueString);
            xpa2biaslvlfreq[i][1] = biasLevel;
        }

    } else {
        printf("This is not SOWL. saveXpaBiasLvlFreq is valid for Sowl only\n");
    }  

}


A_UINT16 freq2fbin(A_UINT16 freq)
{
	if (freq == 0)
	{
		return 0;
	}

	if (freq > 3000)
	{
		return((A_UINT16)((freq-4800)/5) ) ;
	} else
	{
		if ((freq - 2300) > 256)
		{
			printf("Illegal channel %d. Only channels in 2.3-2.555G range supported.\n", freq);
			exit(0);
		}
		return( (A_UINT16)(freq - 2300));
	}
}
