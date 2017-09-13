/* mCont.c - contians continuous transmit functions */
/* Copyright (c) 2001 Atheros Communications, Inc., All Rights Reserved */

#ident  "ACI $Id: //depot/sw/src/dk/mdk/devlib/mCont.c#71 $, $Header: //depot/sw/src/dk/mdk/devlib/mCont.c#71 $"

/* 
Revsision history
--------------------
1.0       Created.
*/
// #include "vxdrv.h"

#ifdef __ATH_DJGPPDOS__
#include <unistd.h>
#ifndef EILSEQ  
    #define EILSEQ EIO
#endif	// EILSEQ

#define __int64	long long
typedef unsigned long DWORD;
#define Sleep	delay
#endif	// #ifdef __ATH_DJGPPDOS__

#include "wlantype.h"
#include "ar5210reg.h"
#include "athreg.h"
#include "manlib.h"
#include "mdata.h"
#include "mEeprom.h"
#include "mConfig.h"
#include "mDevtbl.h"
#include "mIds.h"
#include <assert.h>
#include <math.h>
#include <time.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>

#define SINE_BUFFER_SIZE 2000
#ifndef PI	
#define PI 3.14159265358979323846
#endif

void lclSineWave(A_UINT32 frequency, A_UINT32 amplitude, 
                 A_UCHAR  sine1[2000], A_UCHAR  sine2[2000], A_UINT32 turbo);
static A_UCHAR PN7Data[] = {0xfe, 0xa9, 0x9d, 0xd2, 0xc6, 0xf6, 0xb6, 0x48, 
                            0xe1, 0x7c, 0xae, 0x68, 0x9e, 0x28, 0x60, 0x80};
static A_UCHAR PN9Data[] = {0xff, 0x87, 0xb8, 0x59, 0xb7, 0xa1, 0xcc, 0x24, 
                            0x57, 0x5e, 0x4b, 0x9c, 0x0e, 0xe9, 0xea, 0x50, 
                            0x2a, 0xbe, 0xb4, 0x1b, 0xb6, 0xb0, 0x5d, 0xf1, 
                            0xe6, 0x9a, 0xe3, 0x45, 0xfd, 0x2c, 0x53, 0x18, 
                            0x0c, 0xca, 0xc9, 0xfb, 0x49, 0x37, 0xe5, 0xa8, 
                            0x51, 0x3b, 0x2f, 0x61, 0xaa, 0x72, 0x18, 0x84, 
                            0x02, 0x23, 0x23, 0xab, 0x63, 0x89, 0x51, 0xb3, 
                            0xe7, 0x8b, 0x72, 0x90, 0x4c, 0xe8, 0xfb, 0xc0};

static A_BOOL stabilizePower(A_UINT32 devNum, A_UCHAR  dataRate, A_UINT32 antenna, 
							 A_UINT32 type);

/** 
 * - Function Name: ar928xSetSingleCarrier
 * - Description  :
 * - Arguments
 *     - 
 * - Returns      :                                                                
 *******************************************************************************/
void ar928xSetSingleCarrier( A_UINT32 devNum ) {
    LIB_DEV_INFO *pLibDev = gLibInfo.pLibDevArray[devNum];

    REGW(devNum, 0x983c, ((0x7ff<<11) | 0x7ff));
    REGW(devNum, 0x9808, ((1<<7) | (1<<1)));
    REGW(devNum, 0x982c, 0x80008000);

    if(pLibDev->mode == MODE_11G) { /* 2GHz */
        /* chain zero */
        if((pLibDev->libCfgParams.tx_chain_mask & 0x01) == 0x01)
        {
            REGW(devNum, 0x788c, (0x1 << 29) | (0x1 << 20) | 
                 (0x1 << 18) | (0x1 << 16) | 
                 (0x1 << 12) | (0x1 << 10) | 
                 (0x1 << 7)  | (0x1 << 4)  | 
                 (0x1 << 2)); 
        }
        /* chain one */
        if ((pLibDev->libCfgParams.tx_chain_mask & 0x02) == 0x02 ) 
        {
            REGW(devNum, 0x788c, (0x1 << 29) | (0x1 << 20) | 
                 (0x3 << 18) | (0x1 << 16) | 
                 (0x3 << 12) | (0x3 << 10) | 
                 (0x1 << 7)  | (0x1 << 4)  | 
                 (0x1 << 2)); 
        }
    }

    if( pLibDev->mode == MODE_11A ) { /* 5GHz */
        /* chain zero */
        if( (pLibDev->libCfgParams.tx_chain_mask & 0x01) == 0x01 )
        {
            REGW(devNum, 0x788c, (0x1 << 29) | (0x0 << 20) |
                 (0x0 << 18) | (0x1 << 16) | 
                 (0x1 << 12) | (0x1 << 10) |
                 (0x1 << 7)  | (0x1 << 4)  |
                 (0x1 << 2)); 
        }
        /* chain one */
        if ((pLibDev->libCfgParams.tx_chain_mask & 0x02) == 0x02 ) 
        {
            REGW(devNum, 0x788c, (0x1 << 29) | (0x0 << 20) | 
                 (0x0 << 18) | (0x1 << 16) |
                 (0x3 << 12) | (0x3 << 10) | 
                 (0x1 << 7)  | (0x1 << 4)  | 
                 (0x1 << 2)); 
        }
    }

    /* force xpa on */
    if( (pLibDev->libCfgParams.tx_chain_mask & 0x01) == 0x01 ) {
        REGW(devNum, 0xa3d8, ((0x1<<1) | 1)); /* for chain zero */
    }
    if( (pLibDev->libCfgParams.tx_chain_mask & 0x10) == 0x10 ) {
        REGW(devNum, 0xa3d8, ((0x3<<1) | 1)); /* for chain one */
    }
}
/* ar928xSetSingleCarrier */

/**
 * - Function Name: txContBegin
 * - Description  : place device in continuous transmit mode
 * - Arguments
 *     -      
 * - Returns      : 
 ******************************************************************************/
MANLIB_API void txContBegin
( 
 A_UINT32 devNum,      /* device number */
 A_UINT32 type,        /* tx99, single carrier, etc */
 A_UINT32 typeOption1, /* dac IQ */
 A_UINT32 typeOption2, /* dac IQ */
 A_UINT32 antenna
)
{
   	LIB_DEV_INFO *pLibDev;
    A_UCHAR      sineBuffer1[SINE_BUFFER_SIZE], sineBuffer2[SINE_BUFFER_SIZE];
    A_UINT32	 i, pktSize=0, dataPatLength;
    A_UCHAR      *pData, dataRate;
    MDK_ATHEROS_DESC localDesc;
    MDK_ATHEROS_DESC localDescRx;
	A_UINT16	queueIndex;
	A_UINT32    value;
	A_UINT32    xpaaHigh = 0;
	A_UINT32    xpabHigh = 0;
    A_UINT32    maxConstIQValue = 511;

	if (checkDevNum(devNum) == FALSE) {
		mError(devNum, EINVAL, "Device Number %d:txContBegin\n", devNum);
		return;
	}
#if (0) 	
    if( gLibInfo.pLibDevArray[devNum]->devState < RESET_STATE ) {
        mError(devNum, EILSEQ, "Device Number %d:txContBegin: Device should be out of Reset before continuous transmit\n", devNum);
        return;
    }
#endif    
    pLibDev = gLibInfo.pLibDevArray[devNum];

	pLibDev->selQueueIndex = 0;
	pLibDev->tx[0].dcuIndex = 0;

	queueIndex = pLibDev->selQueueIndex;

    dataRate = 0;

	switch(type) {
	case CONT_SINE:
        mError(devNum, EINVAL, "Device Number %d:txContBegin: The SINE function is being retooled to provide more accurate\n \
            data and is not available in this version of the library.\n", devNum);
		if((typeOption1 < 1) || (typeOption1 > 100)) {
			mError(devNum, EINVAL, 
                "txContBegin: SINE amplitude must be between 1 and 100 inclusively - not: %d\n", 
                typeOption1);
			return;
		}
		if((typeOption2 < 1000) || (typeOption2 > 10000)) {
			mError(devNum, EINVAL, 
                "txContBegin: SINE frequency must be between 1000 kHz and 10,000 kHz - not: %d\n", 
				typeOption2);
			return;
		}
        return;
		break;
	case CONT_DATA:
    case CONT_FRAMED_DATA:
            if( typeOption1 > RANDOM_PATTERN ) {
                mError(devNum, EINVAL, 
                       "Device Number %d:txContBegin: DATA pattern does not match a known value: %d\n", devNum, 
                       typeOption1);
                return;
            }
            //decode the rate code
            dataRate = (A_UCHAR)(rate2bin(typeOption2));
            if( dataRate == UNKNOWN_RATE_CODE ) {
                mError(devNum, EINVAL, "Device Number %d:txContBegin: DATA rate does not match a known rate: %d\n", devNum, 
                       typeOption2);
                return;
        } else {
                dataRate--; // Returned value is index + 1
            }
        break;
	case CONT_SINGLE_CARRIER:
        /* set merlin to single carrier mode */
        if( isMerlin(pLibDev->swDevID) ) {
            ar928xSetSingleCarrier(devNum);
            return;
        }

        //change xpa value
        xpaaHigh = getFieldForMode(devNum, "bb_xpaa_active_high", pLibDev->mode, pLibDev->turbo);
        xpabHigh = getFieldForMode(devNum, "bb_xpab_active_high", pLibDev->mode, pLibDev->turbo);
        if( pLibDev->mode == MODE_11A ) {
            writeField(devNum, "bb_xpaa_active_high", !xpaaHigh);
        }
        else {
            writeField(devNum, "bb_xpab_active_high", !xpabHigh);
        }

//		resetDevice(devNum, pLibDev->macAddr.octets, pLibDev->bssAddr.octets, pLibDev->freqForResetDevice, pLibDev->turbo);
        if( isOwl(pLibDev->swDevID) ) {
            maxConstIQValue = 2047;
        }
        if( typeOption1 > maxConstIQValue ) {
            mError(devNum, EINVAL, "Device Number %d:txContBegin: SINGLE_CARRIER constant I value greater than %d: %d\n", devNum, maxConstIQValue, typeOption1);
            return;
        }
        if( typeOption2 > maxConstIQValue ) {
            mError(devNum, EINVAL, "Device Number %d:txContBegin: SINGLE_CARRIER constant Q value greater than %d: %d\n", devNum, maxConstIQValue, typeOption2);
            return;
        }
		break;
	default:
		mError(devNum, EINVAL, "Device Number %d:txContBegin: type must be set to a valid pattern type\n", devNum);
		return;
	}
        pLibDev->tx[queueIndex].contType = type;

        //cleanup any stuff from previous transmit
        if( pLibDev->tx[queueIndex].pktAddress || pLibDev->tx[queueIndex].descAddress ) {
            memFree(devNum, pLibDev->tx[queueIndex].pktAddress);
            pLibDev->tx[queueIndex].pktAddress = 0;
            memFree(devNum, pLibDev->tx[queueIndex].descAddress);
            pLibDev->tx[queueIndex].descAddress = 0;
            pLibDev->tx[queueIndex].txEnable = 0;
        }
	//update antenna
	//IF using Single carrier, then need to switch the antenna to the opposite
	//to what is expected.  This is because the baseband is in "receive" mode in the
	//case of single carrier, and so uses the opposite antenna to what is set for
	//transmit mode
	if(type == CONT_SINGLE_CARRIER) {
		antenna = 
		(antenna == (USE_DESC_ANT|DESC_ANT_A)) ? 
                    (USE_DESC_ANT|DESC_ANT_B) : (USE_DESC_ANT|DESC_ANT_A);
	}
	if (!ar5kInitData[pLibDev->ar5kInitIndex].pMacAPI->setupAntenna(devNum, antenna, &antenna))
	{
		return;
	}

   	// send a few packets first to step-up the transmit power
    // Setup the data packet
        pData = sineBuffer1;
        switch( typeOption1 ) {
            case ONES_PATTERN:
                *((A_UINT32 *)pData) = 0xFFFFFFFF;
                dataPatLength = 4;
                break;
            case REPEATING_5A:
                *((A_UINT32 *)pData) = 0x5A5A5A5A;
                dataPatLength = 4;
                break;
            case COUNTING_PATTERN:
                for( i = 0; i < 256; i++ ) {
                    pData[i] = (A_UCHAR)i;
                }
                dataPatLength = 256;
                break;
            case PN7_PATTERN:
                pData = PN7Data;
                dataPatLength = sizeof(PN7Data);
                break;
            case PN9_PATTERN:
                pData = PN9Data;
                dataPatLength = sizeof(PN9Data);
                break;
            case REPEATING_10:
                *((A_UINT32 *)pData) = 0xAAAAAAAA;
                dataPatLength = 4;
                break;
            case RANDOM_PATTERN:
                srand( (unsigned)time( NULL ) );
                for( i = 0; i < 256; i++ ) {
                    pData[i] = (A_UCHAR)(rand() & 0xFF);
                }
                dataPatLength = 256;
                break;
            default:  // Use Zeroes Pattern
                *((A_UINT32 *)pData) = 0;
                dataPatLength = 4;
                break;
        }

//	if((type != CONT_SINGLE_CARRIER)&&(type != CONT_SINE)) {
	if(type != CONT_SINE) {
		// Add a local self-linked rx descriptor and buffer to stop receive overrun
		// cleanup descriptors created by the last begin
		if (pLibDev->rx.rxEnable || pLibDev->rx.bufferAddress) {
			memFree(devNum, pLibDev->rx.bufferAddress);
			pLibDev->rx.bufferAddress = 0;
			memFree(devNum, pLibDev->rx.descAddress);
			pLibDev->rx.descAddress = 0;
			pLibDev->rx.rxEnable = 0;
			pLibDev->rx.numDesc = 0;
			pLibDev->rx.bufferSize = 0;
		}

		pLibDev->rx.descAddress = memAlloc( devNum, sizeof(MDK_ATHEROS_DESC));
		if (0 == pLibDev->rx.descAddress) {
			mError(devNum, ENOMEM, "Device Number %d:txContBegin: unable to allocate memory for rx-descriptor to prevent overrun\n", devNum);
			return;
		}
		pLibDev->rx.bufferAddress = memAlloc(devNum, 512);
		if (0 == pLibDev->rx.bufferAddress) {
			mError(devNum, ENOMEM, "Device Number %d:txContBegin: unable to allocate memory for rx-buffer to prevent overrun\n", devNum);
			return;
		}

		localDescRx.bufferPhysPtr = pLibDev->rx.bufferAddress;
		localDescRx.nextPhysPtr = pLibDev->rx.descAddress;
		localDescRx.hwControl[1] = pLibDev->rx.bufferSize;
		localDescRx.hwControl[0] = 0;
		writeDescriptor(devNum, pLibDev->rx.descAddress, &localDescRx);

		//go into AGC deaf mode
		ar5kInitData[pLibDev->ar5kInitIndex].pMacAPI->AGCDeaf(devNum);

		//write RXDP
		ar5kInitData[pLibDev->ar5kInitIndex].pMacAPI->writeRxDescriptor(devNum, pLibDev->rx.descAddress);

//printf("SNOOP:dataRate=%x:antenna=%x:type=%d\n", dataRate, antenna, type);
		//stabilize the power before going into continuous mode
		if(!stabilizePower(devNum, dataRate, antenna, type)) {
			return;
		}

		//allocate memory for cont packet and descriptor
		pLibDev->tx[queueIndex].descAddress = memAlloc( devNum, sizeof(MDK_ATHEROS_DESC) );
//printf("SNOOP:txContBegin:tx%d:descAddr=%x\n", queueIndex, pLibDev->tx[queueIndex].descAddress);
	//memDisplay(devNum, pLibDev->tx[queueIndex].descAddress, 8);
		if (0 == pLibDev->tx[queueIndex].descAddress) {
			mError(devNum, ENOMEM, "Device Number %d:stabilize: unable to allocate memory for descriptors\n", devNum);
			return;
		}

 		//setup the transmit packet
		if(type == CONT_DATA) {
			//set this flag so that the packet will be created without the header
			pLibDev->specialTx100Pkt = 1;
		}

		createTransmitPacket(devNum, MDK_NORMAL_PKT, NULL, 1, 2300, pData, 
			dataPatLength, 1, queueIndex, &(pktSize), &(pLibDev->tx[queueIndex].pktAddress));

		// Setup descriptor template - only the desc addresses change per descriptor
		// write buffer ptr to descriptor
		localDesc.bufferPhysPtr = pLibDev->tx[queueIndex].pktAddress;

		ar5kInitData[pLibDev->ar5kInitIndex].pMacAPI->setContDescriptor( devNum, &localDesc, pktSize, 
							antenna, dataRate );
		
		//set the next pointer to point to itself
		localDesc.nextPhysPtr = pLibDev->tx[queueIndex].descAddress;
		
		//descriptor will be written later
	}


    pLibDev->devState = CONT_ACTIVE_STATE;
    pLibDev->rx.rxEnable = 0;


    // The rest of the code is broken into three sections for the three different data transmit types.
    switch(type) {
	case CONT_SINE:
//        memFree(devNum, pLibDev->tx[queueIndex].descAddress);
//   		memFree(devNum, pLibDev->tx[queueIndex].pktAddress);

        pLibDev->tx[queueIndex].descAddress = memAlloc(devNum, sizeof(MDK_ATHEROS_DESC));
       	if (0 == pLibDev->tx[queueIndex].descAddress) {
            mError(devNum, ENOMEM, "Device Number %d:txContBegin: unable to allocate memory for descriptors\n", devNum);
            return;
        }
	
        //allocate enough memory for the packet and write it down
	    pLibDev->tx[queueIndex].pktAddress = memAlloc(devNum, SINE_BUFFER_SIZE * 2);
       	if (0 == pLibDev->tx[queueIndex].pktAddress) {
            mError(devNum, ENOMEM, "Device Number %d:txContBegin: unable to allocate memory for SINE packet\n", devNum);
            return;
        }
        pktSize = SINE_BUFFER_SIZE * 2;
        lclSineWave(typeOption1, typeOption2, sineBuffer1, sineBuffer2, pLibDev->turbo);
    	pLibDev->devMap.OSmemWrite(devNum, pLibDev->tx[queueIndex].pktAddress, 
					sineBuffer1, SINE_BUFFER_SIZE);
    	pLibDev->devMap.OSmemWrite(devNum, pLibDev->tx[queueIndex].pktAddress + SINE_BUFFER_SIZE, 
					sineBuffer2, SINE_BUFFER_SIZE);

        // Setup the SINE descriptor
        localDesc.nextPhysPtr = 0;

		// write buffer ptr to descriptor
        localDesc.bufferPhysPtr = pLibDev->tx[queueIndex].pktAddress;

		//create and write the 2 control words
		localDesc.hwControl[0] = ( (rateValues[0] << BITS_TO_TX_XMIT_RATE) | 
					 (0x18 << BITS_TO_TX_HDR_LEN) |
					 (antenna << BITS_TO_TX_ANT_MODE) |
					 (pktSize + FCS_FIELD) );

		localDesc.hwControl[1] = pktSize;

        writeDescriptor(devNum, pLibDev->tx[queueIndex].descAddress, &localDesc);

        borrowAnalogAccess(devNum);

		// Turn on Tx
   	    REGW(devNum, PHY_BASE+(50<<2),0x00060406);
		mSleep(1);
        // Set PA on
        REGW(devNum, PHY_BASE+(50<<2),0x00060606);
        mSleep(1);

        returnAnalogAccess(devNum);

        // Program the num of lines to loop on 
        REGW(devNum, 0x8074, (1 << 14) | (((pktSize/2)-1) << 3) | 0x3);
        mSleep(1);

        // Turn on DAC  
        REGW(devNum, PHY_BASE+(11<<2), 0x0ffc0ffc);
        mSleep(1);

        // Enable PCU tstDAC
        REGW(devNum, (PHY_BASE+(2<<2)),(REGR(devNum, PHY_BASE+(2<<2)) | 0x80) & ~0x3);
        mSleep(1);

        // Start the hardware
        REGW(devNum, F2_TXDP0, pLibDev->tx[queueIndex].descAddress);
        mSleep(1);

        REGW(devNum, F2_TXCFG, REGR(devNum, F2_TXCFG) | F2_TXCFG_CONT_EN);

        REGW(devNum, F2_CR, F2_CR_TXE0);

		// Wait for loop to load
        mSleep(2);
		// Enable looping
	    REGW(devNum, 0x8074, REGR(devNum, 0x8074) | 0x4);
        mSleep(1);

        break;
   	
    case CONT_SINGLE_CARRIER:
		// --------- set constant values -----------
        if( isOwl(pLibDev->swDevID) ) {
            REGW(devNum, PHY_BASE+(15<<2), (typeOption2 << 11) | typeOption1);
		} else {
            REGW(devNum, PHY_BASE+(15<<2), (typeOption2 << 9) | typeOption1);
        }

        // --------- Force TSTDAC with value in Reg 15 -----
        REGW(devNum, PHY_BASE+(2<<2), 
             (REGR(devNum, PHY_BASE+(2<<2)) & 0xffffff7d) | (0x1 << 7) | (0x1 << 1));

        // --------- Manually Turn on the DAC -----------
        if( isOwl(pLibDev->swDevID) ) {
            REGW(devNum, PHY_BASE+(11<<2), 0x80008000); // Turn on the DAC
        } else {
            REGW(devNum, PHY_BASE+(11<<2), 0x80038ffc); // Turn on the DAC
        }

        // --------- turn on rf -----------
        if( isOwl(pLibDev->swDevID) ) {
            A_UINT32 txChainM; 
            A_UINT32 tempReg;
	        if(pLibDev->swDevID == SW_DEVICE_ID_OWL) {
                txChainM = pLibDev->libCfgParams.tx_chain_mask | 0x1; // Force chain 0 always enabled - turned off if necessary in mConfig.c
            } else {
                txChainM = pLibDev->libCfgParams.tx_chain_mask; 
            }
            /* Write common switch table TX state bits over RX state */
            tempReg = REGR(devNum, 0x9964);
            REGW(devNum, 0x9964, ((tempReg & 0x000000F0) << 4) | (tempReg & 0xFFFFF0FF));

            value = 0x01400018;
            if( pLibDev->mode != MODE_11A ) {
                value |= 0x00800000;
            }
            if( pLibDev->libCfgParams.ht40_enable ) {
                value &= 0xff9fffff; // BW_ST to 0
            }

            value |= ((txChainM & 0x1) << 9) | ((txChainM & 0x2) << 13) | ((txChainM & 0x4) << 17);
            //printf("SNOOP: BNK3 Val: 0x%08X\n", value);
            REGW(devNum, PHY_BASE+(0x3C << 2), value);
            mSleep(10);
            value |= ((txChainM & 0x1) << 8) | ((txChainM & 0x2) << 12) | ((txChainM & 0x4) << 16);
            //printf("SNOOP: BNK3 Val: 0x%08X\n", value);
            REGW(devNum, PHY_BASE+(0x3C << 2), value);
        } else if((pLibDev->swDevID & 0x00ff) <= 0x0013) {
			REGW(devNum, PHY_BASE+(0x36<<2), 0x0604066);
			mSleep(10);
			REGW(devNum, PHY_BASE+(0x36<<2), 0x0606066);
        } else if ((pLibDev->swDevID & 0x00ff) >= 0x0014){
            value = 0x10a098c2;
            if( pLibDev->mode != MODE_11A ) {
                value |= 0x400000;
            }
			REGW(devNum, PHY_BASE+(0x37<<2), value);
            mSleep(10);
            value |= 0x4000;
            REGW(devNum, PHY_BASE+(0x37<<2), value);
		}

//printf("SNOOP: dumping pci writes:\n");
//displayPciRegWrites(devNum);
//getch();
	    //revert back to existing xpa value.
		changeField(devNum, "bb_xpaa_active_high", xpaaHigh);
	    changeField(devNum, "bb_xpab_active_high", xpabHigh);
        break;

    case CONT_DATA:  
        //reset data create flag
        pLibDev->specialTx100Pkt = 0;

        //Resetup the first transmit descriptor with the More flag set.
        localDesc.hwControl[1] = (pktSize & 0xfff) | DESC_MORE;

        writeDescriptor(devNum, pLibDev->tx[queueIndex].descAddress, &localDesc);

        // start hardware
        ar5kInitData[pLibDev->ar5kInitIndex].pMacAPI->txBeginContData(devNum, queueIndex);
        break;

    case CONT_FRAMED_DATA:  
        writeDescriptor(devNum, pLibDev->tx[queueIndex].descAddress, &localDesc);
#ifdef OWL_PB42
        if( !isHowlAP(devNum) ) {
            ar5kInitData[pLibDev->ar5kInitIndex].pMacAPI->txBeginContFramedData(devNum, queueIndex, 0, 1);
        }
        else {
            ar5kInitData[pLibDev->ar5kInitIndex].pMacAPI->txBeginContFramedData(devNum, queueIndex, 1, 1);
        }
#else
        ar5kInitData[pLibDev->ar5kInitIndex].pMacAPI->txBeginContFramedData(devNum, queueIndex, 0, 1);
#endif
        break;
    }
}
/* txContBegin */

/**************************************************************************
* txContEnd - Remove device from continuous transmit mode
*
*/
MANLIB_API void txContEnd
( 
 A_UINT32 devNum
)
{
  	LIB_DEV_INFO *pLibDev;
	A_UINT16	queueIndex = 0;

	if (checkDevNum(devNum) == FALSE) {
		mError(devNum, EINVAL, "Device Number %d:txContEnd\n", devNum);
		return;
	}
	if (gLibInfo.pLibDevArray[devNum]->devState != CONT_ACTIVE_STATE) {
		mError(devNum, EILSEQ, "Device Number %d:txContEnd: This device is not running continuous transmit - bad function sequence\n", devNum);
		return;
	}
    pLibDev = gLibInfo.pLibDevArray[devNum];
	queueIndex = pLibDev->selQueueIndex;

    if (pLibDev->tx[queueIndex].contType != CONT_FRAMED_DATA) {
        mError(devNum, EINVAL, "Device Number %d:txContEnd: this function does not support stopping continuous transmit except for CONT_FRAMED_DATA (tx99)\n", devNum);
        return;
    }

	//stop the transmit at mac level
	ar5kInitData[pLibDev->ar5kInitIndex].pMacAPI->txEndContFramedData(devNum, queueIndex);

   	//cleanup descriptor/buffer usage
	if (pLibDev->tx[queueIndex].pktAddress || pLibDev->tx[queueIndex].descAddress) {
		memFree(devNum, pLibDev->tx[queueIndex].pktAddress);
		pLibDev->tx[queueIndex].pktAddress = 0;
		memFree(devNum, pLibDev->tx[queueIndex].descAddress);
		pLibDev->tx[queueIndex].descAddress = 0;
   		pLibDev->tx[queueIndex].txEnable = 0;
	}
    pLibDev->devState = RESET_STATE;
}


/**************************************************************************
* stabilizePower - send out a few packets to get power up
*
*/
A_BOOL stabilizePower
(
	A_UINT32 devNum,
	A_UCHAR  dataRate,
	A_UINT32 antenna,
	A_UINT32 type
)
{
  	LIB_DEV_INFO *pLibDev;
	A_UINT32	 numDesc;
	A_UINT32	 pktSize;
	A_UINT16	 queueIndex = 0;
	A_UINT32	 descAddress;
    MDK_ATHEROS_DESC localDesc;
	A_UINT32	 i;  //ch;
	A_UINT32	 timeout = 4000;
	A_UINT32		txComplete;

    pLibDev = gLibInfo.pLibDevArray[devNum];

	if (pLibDev->mode == MODE_11B)
	{
		numDesc = 10;
	} else
	{
		numDesc = 20;
	}
	pLibDev->tx[queueIndex].descAddress = memAlloc( devNum, numDesc * sizeof(MDK_ATHEROS_DESC) );
	//printf("SNOOP::stabilizePower::Allocate %x descs at %x\n", numDesc, pLibDev->tx[queueIndex].descAddress);
	//memDisplay(devNum, pLibDev->tx[queueIndex].descAddress, 32);
	if (0 == pLibDev->tx[queueIndex].descAddress) {
		mError(devNum, ENOMEM, "Device Number %d:stabilize: unable to allocate memory for descriptors\n", devNum);
		return FALSE;
	}

 	//setup the transmit packet
    createTransmitPacket(devNum, MDK_NORMAL_PKT, NULL, numDesc, 0, 0, 
        1, 1, queueIndex, &(pktSize), &(pLibDev->tx[queueIndex].pktAddress));

	//printf("SNOOP::stabilizePower::numDesc=%d:pktSize=%d\n", numDesc, pktSize);

    // Setup descriptor template - only the desc addresses change per descriptor
    // write buffer ptr to descriptor
 	if (isFalcon(devNum) || isDragon(devNum)) {
		localDesc.bufferPhysPtr = FALCON_MEM_ADDR_MASK | (pLibDev->tx[queueIndex].pktAddress);
	} else {
                localDesc.bufferPhysPtr = pLibDev->tx[queueIndex].pktAddress;
	}

	ar5kInitData[pLibDev->ar5kInitIndex].pMacAPI->setDescriptor( devNum, &localDesc, pktSize, 
							antenna, 0, dataRate, 1 );
    descAddress = pLibDev->tx[queueIndex].descAddress;

	// put this check outside the for loop for speed
	if (isFalcon(devNum) || isDragon(devNum)) {
		for (i = 0; i < numDesc; i++) {

			//write link pointer
			if (i == numDesc - 1) { //ie its the last descriptor
				localDesc.nextPhysPtr = 0;
			}
			else {			
				localDesc.nextPhysPtr = FALCON_MEM_ADDR_MASK | (descAddress + sizeof(MDK_ATHEROS_DESC));
			}

			writeDescriptor(devNum, descAddress, &localDesc);

			//increment descriptor address
			descAddress += sizeof(MDK_ATHEROS_DESC);
		}
	} else {
	for (i = 0; i < numDesc; i++) {

		//write link pointer
		if (i == numDesc - 1) { //ie its the last descriptor
			localDesc.nextPhysPtr = 0;
		}
		else {
			localDesc.nextPhysPtr = (descAddress + sizeof(MDK_ATHEROS_DESC));
		}

		writeDescriptor(devNum, descAddress, &localDesc);

		//increment descriptor address
		descAddress += sizeof(MDK_ATHEROS_DESC);
	}
	}

	//start the transmit 
    pLibDev->tx[queueIndex].txEnable = 1;		//This is new.  Funcs need to know what queue to activate

	//start the transmit
	REGW(devNum, 0x58, 0x8120);
	//printf("SNOOP::Press any key to Create next packet\n"); ch = getchar();
	ar5kInitData[pLibDev->ar5kInitIndex].pMacAPI->txBeginConfig(devNum, 0);

	if(pLibDev->mode == MODE_11B) {
		timeout = 9000;
	}

	//printf("SNOOP::stabilizePower::xmit enabled:TXDP in q0=%x\n", REGR(devNum, 0x800));
	//memDisplay(devNum, REGR(devNum, 0x800), 32);

	//printf("SNOOP::Press any key to read status\n"); ch = getchar();
	for(i = 0; i < timeout; i++) {
	txComplete = ar5kInitData[pLibDev->ar5kInitIndex].pMacAPI->isTxComplete(devNum);
		if(txComplete) {
			ar5kInitData[pLibDev->ar5kInitIndex].pMacAPI->rxCleanupConfig(devNum);
            break;
        }
        mSleep(1);
    }
    if(i == timeout) {
        mError(devNum, EIO, "Device Number %d:stabilize power packet transmit took longer than %d seconds\n", devNum, 
			timeout/1000);
        return FALSE;
    }
	
	
	//###########################Need to experiment with this delay
    if((type == CONT_DATA) && (pLibDev->mode == MODE_11B)) {
    	mSleep(10);
  	}
    	mSleep(10);
	
	//cleanup the descriptors used
	memFree(devNum, pLibDev->tx[queueIndex].pktAddress);
	pLibDev->tx[queueIndex].pktAddress = 0;
	memFree(devNum, pLibDev->tx[queueIndex].descAddress);
	pLibDev->tx[queueIndex].descAddress = 0;

	//printf("SNOOP::return from stabilizePower\n");

	return TRUE;
}

/**************************************************************************
* lclSineWave- Create a Sine Wave Data packet filling 4k bytes
*
*/
void lclSineWave 
(
 A_UINT32 amplitude,
 A_UINT32 frequency,
 A_UCHAR  sine1[SINE_BUFFER_SIZE],
 A_UCHAR  sine2[SINE_BUFFER_SIZE],
 A_UINT32 turbo
)
{
    A_UINT32 i, j, samples;
    double step, radian1, radian2, amp, freq, samp; 
    A_INT32 idata1, qdata1, idata2, qdata2;

    amp = amplitude / 100.0;
    freq = frequency / 1000.0;

    samples = SINE_BUFFER_SIZE;
	samp = samples;
    if (turbo) {
	    step = (2 * PI * freq * 25) / samp;
    } 
    else {
	    step = (2 * PI * freq * 50) / samp;
    }

    for (i=0, j = (samples/2); i < (samples/2); i++, j++) {
	    radian1 = i * step;
        radian2 = j * step;

	    idata1 = (A_INT32)((sin(radian1)) * 255 * amp);
	    qdata1 = (A_INT32)((cos(radian1)) * 255 * amp);
	    sine1[(2*i)+1] = (A_UCHAR)(((idata1 + 256)>>1) & 0xff);
	    sine1[(2*i)] = (A_UCHAR)(((qdata1 + 256)>>1) & 0xff);

	    idata2 = (A_INT32)((sin(radian2)) * 255 * amp);
	    qdata2 = (A_INT32)((cos(radian2)) * 255 * amp);
	    sine2[(2*i)+1] = (A_UCHAR)(((idata2 + 256)>>1) & 0xff);
	    sine2[(2*i)] = (A_UCHAR)(((qdata2 + 256)>>1) & 0xff);
    }
}

#define F2_D0_LCL_IFS     0x1040 // MAC DCU-specific IFS settings
#define F2_D_LCL_IFS_CWMIN_M	   0x000003FF // Mask for CW_MIN
#define F2_D_LCL_IFS_CWMAX_M	   0x000FFC00 // Mask for CW_MAX
#define F2_D_LCL_IFS_CWMAX_S	   10		  // Shift for CW_MAX
#define F2_D_LCL_IFS_AIFS_M		   0x0FF00000 // Mask for AIFS
#define F2_D_LCL_IFS_AIFS_S		   20         // Shift for AIFS

/**************************************************************************
* txContFrameBegin - Place device in continuous Burst Frame transmit mode
*
*/
MANLIB_API void txContFrameBegin
( 
 A_UINT32 devNum,  
 A_UINT32 length,  // in bytes
 A_UINT32 ifswait, // in units of slots
 A_UINT32 typeOption1,  // dataPattern
 A_UINT32 typeOption2,  // dataRate: 6, 9, 12, ...
 A_UINT32 antenna,      // ...
 A_BOOL   performStabilizePower,  // flag whether to send 1st 20 pkts: 0=>no, 1=>yes
 A_UINT32 numDescriptors,	// number of descriptors. 0=>forever.
 A_UCHAR  *dest				//destination to send end packet to if in finite mode
)
{
   	LIB_DEV_INFO *pLibDev;
    A_UCHAR      sineBuffer1[SINE_BUFFER_SIZE];
    A_UINT32	 descAddress, lastDescAddress, pktSize, dataPatLength, i;
    A_UCHAR      *pData, dataRate;
    MDK_ATHEROS_DESC localDesc;
    MDK_ATHEROS_DESC localDescRx;
	A_UINT16	queueIndex;
	A_BOOL		infiniteDescMode = FALSE;
	A_BOOL		retries = FALSE;
	A_UINT32		txComplete = 0;
	A_UINT32	timeoutComputed; // in ms

	if (checkDevNum(devNum) == FALSE) {
		mError(devNum, EINVAL, "Device Number %d:txContFrameBegin\n", devNum);
		return;
	}
	if (gLibInfo.pLibDevArray[devNum]->devState < RESET_STATE) {
		mError(devNum, EILSEQ, "Device Number %d:txContFrameBegin: Device should be out of Reset before continuous transmit\n", devNum);
		return;
	}
    pLibDev = gLibInfo.pLibDevArray[devNum];


	pLibDev->selQueueIndex = 0;
	pLibDev->tx[0].dcuIndex = 0;

	queueIndex = pLibDev->selQueueIndex;

    dataRate = 0;

	if (isFalcon(devNum) && (pLibDev->libCfgParams.chainSelect == 2) ) {
		writeField(devNum, "bb_use_static_txbf_cntl", 1);
		writeField(devNum, "bb_static_txbf_enable", 1);
		writeField(devNum, "bb_static_txbf_index", 0);
		writeField(devNum, "bb_use_static_rx_coef_idx", 0);
	}

	if(typeOption1 > RANDOM_PATTERN) {
		mError(devNum, EINVAL, 
               "Device Number %d:txContFrameBegin: DATA pattern does not match a known value: %d\n", devNum, 
			typeOption1);
		return;
	}

	//decode the rate code
    dataRate = (A_UCHAR)(rate2bin(typeOption2));
	if(dataRate == UNKNOWN_RATE_CODE) {
        mError(devNum, EINVAL, "Device Number %d:txContBegin: DATA rate does not match a known rate: %d\n", devNum, 
            typeOption2);
        return;
    } else {
        dataRate--; // Returned value is index + 1
    }

    pLibDev->tx[queueIndex].contType = CONT_FRAMED_DATA;

	//cleanup any stuff from previous transmit
	if (pLibDev->tx[queueIndex].pktAddress || pLibDev->tx[queueIndex].descAddress) {
		memFree(devNum, pLibDev->tx[queueIndex].pktAddress);
		pLibDev->tx[queueIndex].pktAddress = 0;
		memFree(devNum, pLibDev->tx[queueIndex].descAddress);
		pLibDev->tx[queueIndex].descAddress = 0;
   		pLibDev->tx[queueIndex].txEnable = 0;
	}

	//setup the antenna
	if (!ar5kInitData[pLibDev->ar5kInitIndex].pMacAPI->setupAntenna(devNum, antenna, &antenna))
	{
		return;
	}

   	// send a few packets first to step-up the transmit power
    // Setup the data packet
    pData = sineBuffer1;
    switch (typeOption1 ) {
    case ONES_PATTERN:
        *((A_UINT32 *)pData) = 0xFFFFFFFF;
        dataPatLength = 4;
        break;
    case REPEATING_5A:
        *((A_UINT32 *)pData) = 0x5A5A5A5A;
        dataPatLength = 4;
        break;
    case COUNTING_PATTERN:
        for(i = 0; i < 256; i++) {
            pData[i] = (A_UCHAR)i;
        }
        dataPatLength = 256;
        break;
    case PN7_PATTERN:
        pData = PN7Data;
        dataPatLength = sizeof(PN7Data);
        break;
    case PN9_PATTERN:
        pData = PN9Data;
        dataPatLength = sizeof(PN9Data);
        break;
    case RANDOM_PATTERN:
        srand( (unsigned)time( NULL ) );
        for(i = 0; i < 256; i++) {
            pData[i] = (A_UCHAR)(rand() & 0xFF);
        }
        dataPatLength = 256;
        break;
    default:  // Use Zeroes Pattern
        *((A_UINT32 *)pData) = 0;
        dataPatLength = 4;
        break;
    }

	// Add a local self-linked rx descriptor and buffer to stop receive overrun
	// cleanup descriptors created by the last begin
	if (pLibDev->rx.rxEnable || pLibDev->rx.bufferAddress) {
		memFree(devNum, pLibDev->rx.bufferAddress);
		pLibDev->rx.bufferAddress = 0;
		memFree(devNum, pLibDev->rx.descAddress);
		pLibDev->rx.descAddress = 0;
		pLibDev->rx.rxEnable = 0;
		pLibDev->rx.numDesc = 0;
		pLibDev->rx.bufferSize = 0;
	}

	pLibDev->rx.descAddress = memAlloc( devNum, sizeof(MDK_ATHEROS_DESC));
	if (0 == pLibDev->rx.descAddress) {
		mError(devNum, ENOMEM, "Device Number %d:txContBegin: unable to allocate memory for rx-descriptor to prevent overrun\n", devNum);
		return;
	}
	pLibDev->rx.bufferAddress = memAlloc(devNum, 512);
	if (0 == pLibDev->rx.bufferAddress) {
		mError(devNum, ENOMEM, "Device Number %d:txContBegin: unable to allocate memory for rx-buffer to prevent overrun\n", devNum);
		return;
	}

	if (isFalcon(devNum) || isDragon(devNum) ) {
		localDescRx.bufferPhysPtr = FALCON_MEM_ADDR_MASK | pLibDev->rx.bufferAddress;
		localDescRx.nextPhysPtr   = FALCON_MEM_ADDR_MASK | pLibDev->rx.descAddress;
	} else {
         	localDescRx.bufferPhysPtr = pLibDev->rx.bufferAddress;
	        localDescRx.nextPhysPtr = pLibDev->rx.descAddress;
	}

	localDescRx.hwControl[1] = pLibDev->rx.bufferSize;
	localDescRx.hwControl[0] = 0;
	writeDescriptor(devNum, pLibDev->rx.descAddress, &localDescRx);

	//go into AGC deaf mode
	ar5kInitData[pLibDev->ar5kInitIndex].pMacAPI->AGCDeaf(devNum);

	//write RXDP
	ar5kInitData[pLibDev->ar5kInitIndex].pMacAPI->writeRxDescriptor(devNum, pLibDev->rx.descAddress);

	//stabilize the power before going into continuous mode
	if(performStabilizePower > 0) {
		if(!stabilizePower(devNum, dataRate,antenna, CONT_FRAMED_DATA)) {
			return;
		}
	}

	//setup the descriptors for transmission
	if(numDescriptors == 0) {
		//this means infinite mode
		infiniteDescMode = 1;
		numDescriptors = 1;
		retries = TRUE;
	}

    pLibDev->devState = CONT_ACTIVE_STATE;
    pLibDev->rx.rxEnable = 0;

	pLibDev->tx[queueIndex].descAddress = memAlloc( devNum, numDescriptors * sizeof(MDK_ATHEROS_DESC) );
	if (0 == pLibDev->tx[queueIndex].descAddress) {
		mError(devNum, ENOMEM, "Device Number %d:stabilize: unable to allocate memory for descriptors\n", devNum);
		return;
	}

 	//setup the transmit packet
    createTransmitPacket(devNum, MDK_NORMAL_PKT, NULL, numDescriptors, length, pData, 
			dataPatLength, 1, queueIndex, &(pktSize), &(pLibDev->tx[queueIndex].pktAddress));

    // Setup descriptor template - only the desc addresses change per descriptor
    // write buffer ptr to descriptor
	if (isFalcon(devNum) || isDragon(devNum) ) {
		localDesc.bufferPhysPtr = FALCON_MEM_ADDR_MASK | pLibDev->tx[queueIndex].pktAddress;
	} else {
                localDesc.bufferPhysPtr = pLibDev->tx[queueIndex].pktAddress;
	}

	ar5kInitData[pLibDev->ar5kInitIndex].pMacAPI->setContDescriptor( devNum, &localDesc, pktSize, 
							antenna, dataRate );

    descAddress = pLibDev->tx[queueIndex].descAddress;

	for (i = 0; i < numDescriptors; i++) {

		//write link pointer
		if (i == (numDescriptors - 1)) { //ie its the last descriptor
			if(infiniteDescMode) {
				if (isFalcon(devNum) || isDragon(devNum) ) {
					localDesc.nextPhysPtr = FALCON_MEM_ADDR_MASK | pLibDev->tx[queueIndex].descAddress;
				} else {
				localDesc.nextPhysPtr = pLibDev->tx[queueIndex].descAddress;
			}
			}
			else {
				localDesc.nextPhysPtr = 0;
				lastDescAddress = descAddress;
			}
		}
		else {
			if (isFalcon(devNum) || isDragon(devNum) ) {
				localDesc.nextPhysPtr = FALCON_MEM_ADDR_MASK | (descAddress + sizeof(MDK_ATHEROS_DESC));
			} else {
			        localDesc.nextPhysPtr = (descAddress + sizeof(MDK_ATHEROS_DESC));
			}
		}

		writeDescriptor(devNum, descAddress, &localDesc);

		//increment descriptor address
		descAddress += sizeof(MDK_ATHEROS_DESC);
	}

	//start the transmit 
    pLibDev->tx[queueIndex].txEnable = 1;		//This is new.  Funcs need to know what queue to activate
	ar5kInitData[pLibDev->ar5kInitIndex].pMacAPI->txBeginContFramedData(devNum, queueIndex, ifswait, retries);

	if (!infiniteDescMode)
	{
		if (pLibDev->mode == MODE_11B)
		{

			timeoutComputed = 10 + numDescriptors*((A_UINT32)((30+length)*8*10.0/typeOption2) + 6 + 9*ifswait + 20)/1000;
		} else
		{
			timeoutComputed = 10 + numDescriptors*((30+length)*8/typeOption2 + 6 + 9*ifswait + 20)/1000;
		}

		for(i = 0; i < timeoutComputed*50000; i++)
		{
			txComplete = ar5kInitData[pLibDev->ar5kInitIndex].pMacAPI->isTxComplete(devNum);
			if(txComplete) {
				ar5kInitData[pLibDev->ar5kInitIndex].pMacAPI->rxCleanupConfig(devNum);
				break;
			}
		}
		pLibDev->devState = RESET_STATE;

		if(i == timeoutComputed) {
			mError(devNum, EIO, "Device Number %d:stabilize power packet transmit took longer than %d seconds\n", devNum, 
				timeoutComputed/1000);
			return;
		}

		//come out of AGC deaf mode
		ar5kInitData[pLibDev->ar5kInitIndex].pMacAPI->AGCUnDeaf(devNum);
		
		//send the end packet
		
		createEndPacket(devNum, queueIndex, dest, antenna,0);
		sendEndPacket(devNum, queueIndex);
	}
}
