/*
 *  Copyright © 2002 Atheros Communications, Inc.,  All Rights Reserved.
 *
 */

#ident  "ACI $Id: //depot/sw/src/dk/mdk/devlib/ar5210/mData210.c#25 $, $Header: //depot/sw/src/dk/mdk/devlib/ar5210/mData210.c#25 $"

#ifdef __ATH_DJGPPDOS__
#define __int64	long long
typedef unsigned long DWORD;
#define Sleep	delay
#endif	// #ifdef __ATH_DJGPPDOS__

#ifdef VXWORKS
#include "vxworks.h"
#endif
#include <errno.h>
#include <stdio.h>
#include "wlantype.h"
#ifdef Linux
#include "../mdata.h"
#include "../athreg.h"
#include "../manlib.h"
#include "../mEeprom.h"
#include "../mConfig.h"
#else
#include "..\mdata.h"
#include "..\athreg.h"
#include "..\manlib.h"
#include "..\mEeprom.h"
#include "..\mConfig.h"
#endif
#include "mData210.h"

#include "ar5210reg.h"

void macAPIInitAr5210
(
 A_UINT32 devNum
) 
{
	LIB_DEV_INFO *pLibDev = gLibInfo.pLibDevArray[devNum];
	pLibDev->txDescStatus1 = FIRST_STATUS_WORD;
	pLibDev->txDescStatus2 = SECOND_STATUS_WORD;
	pLibDev->decryptErrMsk = DESC_DECRYPT_ERROR;
	pLibDev->bitsToRxSigStrength = BITS_TO_RX_SIG_STRENGTH;
	pLibDev->rxDataRateMsk = DATA_RATE_MASK;
	
	return;
}

void setRetryLimitAr5210
( 
 A_UINT32 devNum ,
 A_UINT16 queueIndex
)
{
	LIB_DEV_INFO *pLibDev = gLibInfo.pLibDevArray[devNum];
    REGW(devNum, F2_RETRY_LMT, pLibDev->tx[queueIndex].retryValue);
}



A_UINT32 setupAntennaAr5210
(
 A_UINT32 devNum, 
 A_UINT32 antenna, 
 A_UINT32* antModePtr 
)
{
	A_UINT32 nRet = 0;
	//update antenna
 	if (antenna & USE_DESC_ANT) {
		//clear this bit
		*antModePtr = antenna & ~USE_DESC_ANT;
		
        if(*antModePtr > DESC_ANT_B) {
			mError(devNum, EINVAL, "setupAntenna: Illegal antenna setting value %08lx\n", antenna);
			return nRet;
		}

		// setup the register to use the descriptor antenna	// __TODO__
		REGW(devNum, F2_STA_ID1, (REGR(devNum, F2_STA_ID1) | F2_STA_ID1_DESC_ANT)); 
	}
	else {
		//setup for normal antenna		// __TODO__
		REGW(devNum, F2_STA_ID1, (REGR(devNum, F2_STA_ID1) & ~F2_STA_ID1_DESC_ANT)); 
	}
	nRet = 1;
	return nRet;
}


A_UINT32 sendTxEndPacketAr5210
(
 A_UINT32 devNum, 
 A_UINT16 queueIndex
)
{
	LIB_DEV_INFO *pLibDev = gLibInfo.pLibDevArray[devNum];
	A_UINT32 nRet = 1;
	A_UINT32 i = 0;
    A_UINT32	status1 = 0;
	A_UINT32	status2 = 0;

	//set max retries
	REGW(devNum, F2_RETRY_LMT, (REGR(devNum, F2_RETRY_LMT) | MAX_RETRIES));	
	
	//write TXDP
	REGW(devNum, F2_TXDP0, pLibDev->tx[queueIndex].endPktDesc);	

  pLibDev->devMap.OSmemWrite(devNum,  pLibDev->tx[queueIndex].endPktDesc + FIRST_STATUS_WORD,
				(A_UCHAR *)&status1, sizeof(status1));

	//write TX enable bit
	REGW(devNum, F2_CR, REGR(devNum, F2_CR) | F2_CR_TXE0);
	
	//wait for this packet to complete, by polling the done bit
	for(i = 0; i < MDK_PKT_TIMEOUT; i++) {
	   pLibDev->devMap.OSmemRead(devNum,  pLibDev->tx[queueIndex].endPktDesc + SECOND_STATUS_WORD,
				(A_UCHAR *)&status2, sizeof(status2));

	   if(status2 & DESC_DONE) {
		   pLibDev->devMap.OSmemRead(devNum,  pLibDev->tx[queueIndex].endPktDesc + FIRST_STATUS_WORD,
				(A_UCHAR *)&status1, sizeof(status1));
			// If not ok and not sending broadcast frames (i.e. no response required)
			if( (!(status1 & DESC_FRM_XMIT_OK)) && (pLibDev->tx[queueIndex].broadcast == 0) ) {
				mError(devNum, EIO, "sendTxEndPacket5210: end packet not successfully sent, status = 0x%04X\n",
							status1);
				nRet = 0;
				return nRet;
			}
			break;
		}
		mSleep(1);
	}

	if (i == MDK_PKT_TIMEOUT) {
		mError(devNum, EIO, "sendTxEndPacket5210: timed out waiting for end packet done\n");
		nRet = 0;
	}
	return nRet;
}


void setDescriptorAr5210
(
 A_UINT32 devNum,
 MDK_ATHEROS_DESC*  localDescPtr,
 A_UINT32 pktSize,
 A_UINT32 antMode,
 A_UINT32 descNum,
 A_UINT32 rateIndex,
 A_UINT32 broadcast
)
{
	LIB_DEV_INFO *pLibDev = gLibInfo.pLibDevArray[devNum];
	A_UINT16	 queueIndex = pLibDev->selQueueIndex;
	A_UINT32 frameLen = 0;

	//create and write the 2 control words
    frameLen = (pktSize + FCS_FIELD) + ((pLibDev->wepEnable && (descNum < pLibDev->tx[queueIndex].numDesc)) ? WEP_ICV_FIELD : 0);
	localDescPtr->hwControl[0] = ( (rateValues[rateIndex] << BITS_TO_TX_XMIT_RATE) | 0x01000000 |  //clear dest mask
				 ((sizeof(WLAN_DATA_MAC_HEADER3) + 8 ) << BITS_TO_TX_HDR_LEN) |
//					 ((sizeof(WLAN_DATA_MAC_HEADER3) + (pLibDev->wepEnable ? WEP_ICV_FIELD : 0)) << BITS_TO_TX_HDR_LEN) |
				 (antMode << BITS_TO_TX_ANT_MODE) |
				  frameLen | 
                 ((pLibDev->wepEnable && (descNum < pLibDev->tx[queueIndex].numDesc)) ? (1 << BITS_TO_ENCRYPT_VALID) : 0));
    if (descNum == pLibDev->tx[queueIndex].numDesc - 1) {
        localDescPtr->hwControl[0] |= DESC_TX_INTER_REQ;
    }
    
    localDescPtr->hwControl[1] = ((pLibDev->wepEnable && (descNum < pLibDev->tx[queueIndex].numDesc))? (pLibDev->wepKey << BITS_TO_ENCRYPT_KEY) : 0) |
                                ((pLibDev->wepEnable && (descNum < pLibDev->tx[queueIndex].numDesc)) ? (frameLen - 6) : pktSize);

	broadcast = 0;		//this is not used quieting warnings
}

void setStatsPktDescAr5210
(
 A_UINT32 devNum,
 MDK_ATHEROS_DESC*  localDescPtr,
 A_UINT32 pktSize,
 A_UINT32 rateValue
)
{
	localDescPtr->hwControl[0] = ((rateValue << BITS_TO_TX_XMIT_RATE) | 
				 (sizeof(WLAN_DATA_MAC_HEADER3) << BITS_TO_TX_HDR_LEN) |
				 (pktSize + 4));

	localDescPtr->hwControl[1] = pktSize;
	devNum = 0;		//this is not used quieting warnings
	return;
}

A_UINT32 txGetDescRateAr5210
(
 A_UINT32 devNum,
 A_UINT32 descAddr,
 A_BOOL   *ht40,
 A_BOOL   *shortGi
)
{
	LIB_DEV_INFO *pLibDev = gLibInfo.pLibDevArray[devNum];
	A_UINT32 descRate;

	pLibDev->devMap.OSmemRead(devNum, descAddr + FIRST_CONTROL_WORD, 
						(A_UCHAR *)&(descRate), sizeof(descRate));
	descRate = (descRate >> BITS_TO_TX_XMIT_RATE) & DATA_RATE_MASK;
    *ht40 = 0;
    *shortGi = 0;
	return(descRate);
}

void setContDescriptorAr5210
(
 A_UINT32 devNum,
 MDK_ATHEROS_DESC*  localDescPtr,
 A_UINT32 pktSize,
 A_UINT32 antMode,
 A_UCHAR  dataRate
)
{
	localDescPtr->hwControl[0] = ( (rateValues[dataRate] << BITS_TO_TX_XMIT_RATE) | 
				 (sizeof(WLAN_DATA_MAC_HEADER3) << BITS_TO_TX_HDR_LEN) |
				 (antMode << BITS_TO_TX_ANT_MODE) |
				 (pktSize + FCS_FIELD) );

	localDescPtr->hwControl[1] = pktSize;
	devNum = 0;		//this is not used quieting warnings
}

void txBeginContDataAr5210
(
 A_UINT32 devNum,
 A_UINT16 queueIndex
)
{
	LIB_DEV_INFO *pLibDev = gLibInfo.pLibDevArray[devNum];

    REGW(devNum, F2_TXDP0, pLibDev->tx[queueIndex].descAddress);

    // Put PCU and DMA in continuous data mode
	//disable encryption since packet has no header
	REGW(devNum, F2_DIAG_SW, REGR(devNum, F2_DIAG_SW) | F2_DIAG_ENCRYPT_DIS);

    REGW(devNum, F2_TXCFG, REGR(devNum, F2_TXCFG) | F2_TXCFG_CONT_EN);
    REGW(devNum, 0x8074, REGR(devNum, 0x8074) | 1);
    REGW(devNum, F2_CR, F2_CR_TXE0);
}


void txBeginContFramedDataAr5210
(
 A_UINT32 devNum,
 A_UINT16 queueIndex,
 A_UINT32 ifswait,
 A_BOOL   retries
)
{
	LIB_DEV_INFO *pLibDev = gLibInfo.pLibDevArray[devNum];

	// Set DIFS to a smaller value to decrease time between frames
    REGW(devNum, F2_IFS0, (REGR(devNum, F2_IFS0) & ~F2_IFS0_DIFS_M) | (120 << F2_IFS0_DIFS_S));
    // Zero out the Contention Window value
	REGW(devNum, F2_RETRY_LMT, REGR(devNum, F2_RETRY_LMT) & ~F2_RETRY_LMT_CW_M);

    // start hardware
    REGW(devNum, F2_TXDP0, pLibDev->tx[queueIndex].descAddress);
    REGW(devNum, F2_CR, F2_CR_TXE0);
	ifswait = retries = 0;   //to quiet compiler warnings, not used for now
}

void txEndContFramedDataAr5210
(
 A_UINT32 devNum,
 A_UINT16 queueIndex
)
{
	LIB_DEV_INFO *pLibDev = gLibInfo.pLibDevArray[devNum];
    A_UINT32 nextPhysPtr;
	A_UINT16 i;
	A_UINT32 value;

    //zero out the link pointer
	nextPhysPtr = 0;
   	pLibDev->devMap.OSmemWrite(devNum, pLibDev->tx[queueIndex].descAddress, 
					(A_UCHAR *)&nextPhysPtr, sizeof(nextPhysPtr));

	//wait for txe to go clear
	for(i = 0; i < 2000; i++) {
		value = REGR(devNum, F2_CR);
		if(!(value & F2_CR_TXE0)) {
			break;
		}
		mSleep(1);
	}
	if(i == 2000) {
		mError(devNum, EIO, "Timed out waiting for txContEnd\n");
	}

    // Reset the Contention Window value
    REGW(devNum, F2_RETRY_LMT, (REGR(devNum, F2_RETRY_LMT) & ~F2_RETRY_LMT_CW_M) | (15 << F2_RETRY_LMT_CW_S));

    // Reset SIFS/DIFS to the normal value
    if(pLibDev->turbo == TURBO_ENABLE) {
		// Set SIFS and DIFS timers - SIFS set to 8us, DIFS to 20us
		REGW(devNum, F2_IFS0, (0x1E0 & F2_IFS0_SIFS_M) |
                ((0x5A0 << F2_IFS0_DIFS_S) & F2_IFS0_DIFS_M));
    }
    else {
		REGW(devNum, F2_IFS0, (0x230 & F2_IFS0_SIFS_M) |
                ((0x500 << F2_IFS0_DIFS_S) & F2_IFS0_DIFS_M));
    }
	return;
}



/**************************************************************************
* txBeginConfig - program register ready for tx begin
*
*/
void txBeginConfigAr5210
(
 A_UINT32 devNum,
 A_BOOL	  enableInterrupt
)
{
	LIB_DEV_INFO *pLibDev = gLibInfo.pLibDevArray[devNum];
	A_UINT16	queueIndex = pLibDev->selQueueIndex;

	//enable tx DESC interrupt
	if(enableInterrupt) {
		REGW(devNum, F2_IMR, REGR(devNum, F2_IMR) | F2_IMR_TXDESC); 

		//clear ISR before enable
		REGR(devNum, F2_ISR);
		REGW(devNum, F2_IER, F2_IER_ENABLE);
	}

	//write TXDP
	REGW(devNum, F2_TXDP0, pLibDev->tx[queueIndex].descAddress);

	//enable unicast reception - needed to receive acks
	REGW(devNum, F2_RX_FILTER, REGR(devNum, F2_RX_FILTER) | F2_RX_UCAST);
	//enable receive 
	REGW(devNum, F2_DIAG_SW, REGR(devNum, F2_DIAG_SW) & ~F2_DIAG_RX_DIS);

	//write TX enable bit
	REGW(devNum, F2_CR, REGR(devNum, F2_CR) | F2_CR_TXE0 | F2_CR_RXE);
	return;
}


void beginSendStatsPktAr5210
(
 A_UINT32  DevNum, 
 A_UINT32 DescAddress
)
{
    REGW(DevNum, F2_RETRY_LMT, (REGR(DevNum, F2_RETRY_LMT) | MAX_RETRIES));
	
   	REGW(DevNum, F2_TXDP0, DescAddress);
	REGW(DevNum, F2_CR, REGR(DevNum, F2_CR) | F2_CR_TXE0);
}


void writeRxDescriptorAr5210
( 
 A_UINT32 devNum, 
 A_UINT32 rxDescAddress
 )
{
	REGW(devNum, F2_RXDP, rxDescAddress);
}


/**************************************************************************
* rxBeginConfig - program register ready for rx begin
*
*/
void rxBeginConfigAr5210
(
 A_UINT32 devNum
)
{
	A_UINT32	regValue = 0;
	LIB_DEV_INFO *pLibDev = gLibInfo.pLibDevArray[devNum];

	//enable rx DESC_INT
	REGW(devNum, F2_IMR, (REGR(devNum, F2_IMR) | F2_IMR_RXDESC));
	
	//need to set receive frame gap timeout to stop getting this interrupt on RXDESC 
	REGW(devNum, F2_RPGTO, (REGR(devNum, F2_RPGTO) & ~F2_RPGTO_MASK));
	REGW(devNum, F2_RPCNT, (REGR(devNum, F2_RPCNT) | F2_RPCNT_MASK));
	//clear out any RXDESC interrupts before enabling interrups, so we dont get
	//RXDESC right away
	regValue = REGR(devNum, F2_ISR); 
	REGW(devNum, F2_IER, F2_IER_ENABLE);

#ifdef _DEBUG
	if(REGR(devNum, F2_CR) & F2_CR_RXE) {
		mError(devNum, EIO, "F2_CR_RXE is active before receive was enabled\n");
	}
#endif

	//write RXDP
	REGW(devNum, F2_RXDP, pLibDev->rx.descAddress);

#ifdef _DEBUG
	if(REGR(devNum, F2_RXDP) != pLibDev->rx.descAddress) {
		mError(devNum, EIO, "F2_RXDP does not equal pLibDev->rx.descAddress\n");
	}
#endif

	//enable unicast and broadcast reception 
	REGW(devNum, F2_RX_FILTER, REGR(devNum, F2_RX_FILTER) | F2_RX_UCAST | F2_RX_BCAST);

	//enable receive 
	REGW(devNum, F2_DIAG_SW, REGR(devNum, F2_DIAG_SW) & ~F2_DIAG_RX_DIS);
	
	//write RX enable bit
	REGW(devNum, F2_CR, REGR(devNum, F2_CR) | F2_CR_RXE);

	return;
}


/**************************************************************************
* rxCleanupConfig - program register ready for rx begin
*
*/
void rxCleanupConfigAr5210
(
 A_UINT32 devNum
)
{
    A_UINT32 tops, mibc, bcr, imr;

	//set the receive disable register
	REGW(devNum, F2_DIAG_SW, REGR(devNum, F2_DIAG_SW) | F2_DIAG_RX_DIS);

	//clear rx DESC_INT
	REGW(devNum, F2_IMR, (REGR(devNum, F2_IMR) & ~F2_IMR_RXDESC));
	
	//disable IER
    REGW(devNum, F2_IER, F2_IER_DISABLE);

	//clear unicast and broadcast reception 
	REGW(devNum, F2_RX_FILTER, REGR(devNum, F2_RX_FILTER) & ~(F2_RX_UCAST | F2_RX_BCAST));

    tops = REGR(devNum, F2_TOPS);
    mibc = REGR(devNum, F2_MIBC);
    bcr = REGR(devNum, F2_BCR);
    imr = REGR(devNum, F2_IMR);

	//disabling RX - requires DMA RESET for now
	REGW(devNum, F2_RC, F2_RC_DMA);
	
	mSleep(1);
	REGW(devNum, F2_RC, 0);

    // Restore Reset Registers
	REGW(devNum, F2_TOPS, tops);
    REGW(devNum, F2_MIBC, mibc);
    REGW(devNum, F2_BCR, bcr);
    REGW(devNum, F2_IMR, imr);
}

/**************************************************************************
* txCleanupConfig - cleanup register state bits we set
*
*/
void txCleanupConfigAr5210
(
 A_UINT32 devNum
)
{
	//set the receive disable register
	REGW(devNum, F2_DIAG_SW, REGR(devNum, F2_DIAG_SW) | F2_DIAG_RX_DIS);

	//clear tx DESC interrupt
	REGW(devNum, F2_IMR, REGR(devNum, F2_IMR) & ~F2_IMR_TXDESC); 

	//clear IER enable 
	REGW(devNum, F2_IER, F2_IER_DISABLE);

	//clear unicast reception 
	REGW(devNum, F2_RX_FILTER, REGR(devNum, F2_RX_FILTER) & ~F2_RX_UCAST);
	return;
}



void setPPM5210
(
 A_UINT32 devNum,
 A_UINT32 enablePPM
)
{
	LIB_DEV_INFO *pLibDev = gLibInfo.pLibDevArray[devNum];
    if(enablePPM) {
        REGW(devNum, F2_DIAG_SW, REGR(devNum, F2_DIAG_SW) | F2_DIAG_CHAN_INFO);
        REGW(devNum, PHY_FRAME_CONTROL, REGR(devNum, PHY_FRAME_CONTROL) |
            0x10000);
        pLibDev->rx.enablePPM = 1;
    }
    else {
       REGW(devNum, F2_DIAG_SW, REGR(devNum, F2_DIAG_SW) & ~F2_DIAG_CHAN_INFO);
       REGW(devNum, PHY_FRAME_CONTROL, REGR(devNum, PHY_FRAME_CONTROL) &
            ~0x10000);
        pLibDev->rx.enablePPM = 0;
    }
} 



A_UINT32 isTxdescEvent5210
(
 A_UINT32 eventISRValue
)
{
	return eventISRValue & F2_ISR_TXDESC;
}


A_UINT32 isRxdescEvent5210
(
 A_UINT32 eventISRValue
)
{
	return eventISRValue & F2_ISR_RXDESC;
}

A_UINT32 isTxComplete5210
(
 A_UINT32 devNum
)
{
	return(~(REGR(devNum, F2_CR) & F2_CR_TXE0));
}

void disableRx5210
(
 A_UINT32 devNum
)
{
	REGW(devNum, F2_DIAG_SW, F2_DIAG_RX_DIS);
}

void enableRx5210
(
 A_UINT32 devNum
)
{
	REGW(devNum, F2_DIAG_SW, REGR(devNum, F2_DIAG_SW) & ~F2_DIAG_RX_DIS);
}

void setQueueAr5210
(
	A_UINT32 devNum,
	A_UINT32 qcuNumber
)
{
	if (qcuNumber > 1) {
		mError(devNum, EINVAL,"Queue number can be either 0 or 1\n");
		return;
	}
#ifndef NO_LIB_PRINT		
	printf("Queue No 1 is not currently used \n"); 
	printf("Using queue 0 by default \n");
#endif
		
	devNum = 0;			// used to quiet warning messages
	qcuNumber = 0;		// used to quiet warning messages
}

void mapQueueAr5210
(
	A_UINT32 devNum,
	A_UINT32 qcuNumber,
	A_UINT32 dcuNumber
)
{
	mError(devNum, EINVAL,"mapQueue is not supported for this chip\n");

	devNum = 0;			// used to quiet warning messages
	qcuNumber = 0;		// used to quiet warning messages
	dcuNumber = 0;		// used to quiet warning messages
}

void clearKeyCacheAr5210
(
	A_UINT32 devNum
)
{
#ifndef NO_LIB_PRINT		
	printf("ClearKeyCache is not currently implemented \n");
#endif
	devNum = 0;			// used to quiet warning messages
}

void AGCDeafAr5210
(
 A_UINT32 devNum
)
{
	//quiet warnings
	devNum = 0;
	return;
}

void AGCUnDeafAr5210
(
  A_UINT32 devNum
)
{
	//quiet warnings
	devNum = 0;
	return;
}
