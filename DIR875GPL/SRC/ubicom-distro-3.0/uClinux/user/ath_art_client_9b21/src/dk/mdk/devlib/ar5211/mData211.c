/*
 *  Copyright © 2002 Atheros Communications, Inc.,  All Rights Reserved.
 *
 */

#ident  "ACI $Id: //depot/sw/src/dk/mdk/devlib/ar5211/mData211.c#61 $, $Header: //depot/sw/src/dk/mdk/devlib/ar5211/mData211.c#61 $"

#ifdef VXWORKS
#include "vxworks.h"
#endif

#ifdef __ATH_DJGPPDOS__
#define __int64	long long
typedef unsigned long DWORD;
#define Sleep	delay
#endif	// #ifdef __ATH_DJGPPDOS__

#include <errno.h>
#include "mData211.h"
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
#include "..\mDevtbl.h"
#if defined(COBRA_AP) && defined(PCI_INTERFACE)
#include "..\mIds.h"
#endif

#endif

#include "ar5211reg.h"
#include "stdio.h"

#if defined(COBRA_AP) && defined(PCI_INTERFACE)
#include "ar531xPlusreg.h"
#endif

#ifdef __ATH_DJGPPDOS__
#include "mlibif_dos.h"
#endif //__ATH_DJGPPDOS__

void macAPIInitAr5211
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

void setRetryLimitAr5211
( 
 A_UINT32 devNum ,
 A_UINT16 queueIndex
)
{
	A_UINT32 regValue;
	LIB_DEV_INFO *pLibDev = gLibInfo.pLibDevArray[devNum];

	int index = pLibDev->tx[queueIndex].dcuIndex;	// DCU 0...10 (11)  
	regValue = REGR(devNum, F2_D0_RETRY_LIMIT + ( 4 * index ) );
	REGW(devNum, F2_D0_RETRY_LIMIT + ( 4 * index ), 
		(regValue & 0xfffffff0) | pLibDev->tx[queueIndex].retryValue);
}


void setRetryLimitAllAr5211
( 
 A_UINT32 devNum,
 A_UINT16 reserved
)
{
	LIB_DEV_INFO *pLibDev = gLibInfo.pLibDevArray[devNum];
	A_UINT32 i = 0;
	A_UINT32 index = 0;
	A_UINT32 regValue;

	for( i = 0; i < MAX_TX_QUEUE; i++ )
	{
		if ( pLibDev->tx[i].txEnable )
		{
			index = pLibDev->tx[i].dcuIndex;	// DCU 0...10 (11)  
			regValue = REGR(devNum, F2_D0_RETRY_LIMIT + ( 4 * index ) );
		    REGW(devNum, F2_D0_RETRY_LIMIT + ( 4 * index ), 
				 (regValue & 0xfffffff0) | pLibDev->tx[i].retryValue);

			regValue = REGR(devNum, F2_D0_RETRY_LIMIT + ( 4 * index ) );
		}
	}
	reserved = 0; //this is not used quieting warnings
}

#define ENABLE_FAST_ANT_DIV		0x00002000

A_UINT32 setupAntennaAr5211
(
 A_UINT32 devNum, 
 A_UINT32 antenna, 
 A_UINT32* antModePtr 
)
{
	LIB_DEV_INFO *pLibDev = gLibInfo.pLibDevArray[devNum];
	int nRet = 0;
	A_UINT32	switchTable[10];
	A_UINT32 tempValue;

	//update antenna
	if (antenna & USE_DESC_ANT) {
		//clear this bit
		*antModePtr = antenna & ~USE_DESC_ANT;
		
		if(*antModePtr > DESC_ANT_B) {
			mError(devNum, EINVAL, "setupAntenna: Illegal antenna setting value %08lx\n", antenna);
			return nRet;
		}

		//antenna A or B is one more than in crete
		(*antModePtr)++;


		// setup the registers
		REGW(devNum, F2_DEF_ANT, *antModePtr);
		REGW(devNum, F2_STA_ID1, REGR(devNum, F2_STA_ID1) & ~(F2_STA_ID1_DEFANT_UPDATE | F2_STA_ID1_USE_DEFANT) ); 
		//printf("  IN IF F2_DEF_ANT Antenna =%x REGR(devNum, F2_STA_ID1)=%x\n",REGR(devNum, F2_DEF_ANT),REGR(devNum, F2_STA_ID1));
		//REGW(devNum, F2_STA_ID1, (REGR(devNum, F2_STA_ID1) & ~F2_STA_ID1_USE_DEFANT)); 

		//disable the fast antenna diversity
		tempValue = (REGR(devNum, 0xa208) & ~ENABLE_FAST_ANT_DIV);
		REGW(devNum, 0xa208, tempValue);

		if((pLibDev->swDevID == 0xf11b)) { // || (pLibDev->devID == 0x0012) || (pLibDev->devID == 0xff12)) {
			//problem getting antenna switching working.  Hardcode the switch table
			//do a write field so changes immediately
			if(pLibDev->mode == MODE_11A) {
				if(*antModePtr == 1) switchTable[0] = 0x01; else switchTable[0] = 0x02;
				if(*antModePtr == 1) switchTable[1] = 0x21; else switchTable[1] = 0x22;
				if(*antModePtr == 1) switchTable[2] = 0x01; else switchTable[2] = 0x02;
				if(*antModePtr == 1) switchTable[3] = 0x21; else switchTable[3] = 0x22;
				if(*antModePtr == 1) switchTable[4] = 0x02; else switchTable[4] = 0x01;
				if(*antModePtr == 1) switchTable[5] = 0x02; else switchTable[5] = 0x01;
				if(*antModePtr == 1) switchTable[6] = 0x22; else switchTable[6] = 0x21;
				if(*antModePtr == 1) switchTable[7] = 0x02; else switchTable[7] = 0x01;
				if(*antModePtr == 1) switchTable[8] = 0x22; else switchTable[8] = 0x21;
				if(*antModePtr == 1) switchTable[9] = 0x01; else switchTable[9] = 0x02;
			}
			else
			{
				if(*antModePtr == 1) switchTable[0] = 0x00; else switchTable[0] = 0x00;
				if(*antModePtr == 1) switchTable[1] = 0x10; else switchTable[1] = 0x10;
				if(*antModePtr == 1) switchTable[2] = 0x04; else switchTable[2] = 0x08;
				if(*antModePtr == 1) switchTable[3] = 0x14; else switchTable[3] = 0x18;
				if(*antModePtr == 1) switchTable[4] = 0x08; else switchTable[4] = 0x04;
				if(*antModePtr == 1) switchTable[5] = 0x00; else switchTable[5] = 0x00;
				if(*antModePtr == 1) switchTable[6] = 0x10; else switchTable[6] = 0x10;
				if(*antModePtr == 1) switchTable[7] = 0x08; else switchTable[7] = 0x04;
				if(*antModePtr == 1) switchTable[8] = 0x18; else switchTable[8] = 0x14;
				if(*antModePtr == 1) switchTable[9] = 0x04; else switchTable[9] = 0x08;

			}
			writeField(devNum, "bb_switch_table_r1x12", switchTable[0]);
			writeField(devNum, "bb_switch_table_r1x2", switchTable[1]);
			writeField(devNum, "bb_switch_table_r1x1", switchTable[2]);
			writeField(devNum, "bb_switch_table_r1", switchTable[3]);
			writeField(devNum, "bb_switch_table_t1", switchTable[4]);
			writeField(devNum, "bb_switch_table_r2x12", switchTable[5]);
			writeField(devNum, "bb_switch_table_r2x2", switchTable[6]);
			writeField(devNum, "bb_switch_table_r2x1", switchTable[7]);
			writeField(devNum, "bb_switch_table_r2", switchTable[8]);
			writeField(devNum, "bb_switch_table_t2", switchTable[9]);
		}
	}
	else {
			//setup for normal antenna		// __TODO__
		// mError(devNum, EIO, "setupAntennaAr5211: Normal antenna (alternating mode) not currently implemented\n"); Giri -07/16/04
		REGW(devNum, F2_DEF_ANT, antenna+1);
#if _DEBUG
		printf(" Using/Setting the default anntenna =%x\n",antenna+1);
#endif
		REGW(devNum, F2_STA_ID1, REGR(devNum, F2_STA_ID1) | (F2_STA_ID1_DEFANT_UPDATE | F2_STA_ID1_USE_DEFANT) ); 
		#if _DEBUG
		printf("ELSE F2_DEF_ANT Antenna =%x REGR(devNum, F2_STA_ID1)=%x\n",REGR(devNum, F2_DEF_ANT),REGR(devNum, F2_STA_ID1));
		#endif
		return nRet;			
		//REGW(devNum, F2_STA_ID1, (REGR(devNum, F2_STA_ID1) & ~F2_STA_ID1_DESC_ANT)); 
	}
	nRet = 1;
	return nRet;
}


A_UINT32 sendTxEndPacketAr5211
(
 A_UINT32 devNum, 
 A_UINT16 queueIndex
)
{
	LIB_DEV_INFO *pLibDev = gLibInfo.pLibDevArray[devNum];
	A_UINT32 nRet = 1;
	A_UINT32 i = 0;
	A_UINT32 index = pLibDev->tx[queueIndex].dcuIndex;	// DCU 0...10 (11)  
    A_UINT32	status1 = 0;
	A_UINT32	status2 = 0;
	A_UINT32    tempValue;

	//successful transmission of frames, so send special end packet
	//set max retries
	REGW(devNum, F2_D0_RETRY_LIMIT + ( 4 * index ), 
				(REGR(devNum, F2_D0_RETRY_LIMIT + ( 4 * index ) ) | MAX_RETRIES));
	//setRetryLimitAr5211( devNum, queueIndex );

    //zero out the status info
	pLibDev->devMap.OSmemWrite(devNum,  pLibDev->tx[queueIndex].endPktDesc + pLibDev->txDescStatus2,
				(A_UCHAR *)&status2, sizeof(status2));

    pLibDev->devMap.OSmemWrite(devNum,  pLibDev->tx[queueIndex].endPktDesc + pLibDev->txDescStatus1,
				(A_UCHAR *)&status1, sizeof(status1));

	//enable unicast reception - needed to receive acks
	REGW(devNum, F2_RX_FILTER, REGR(devNum, F2_RX_FILTER) | F2_RX_UCAST);
	
	//enable receive 
	REGW(devNum, F2_DIAG_SW, REGR(devNum, F2_DIAG_SW) & ~F2_DIAG_RX_DIS);

	//make sure the sifs time is restored
	tempValue = getFieldForMode(devNum, "mc_sifs_dcu", pLibDev->mode, pLibDev->turbo);
	writeField(devNum, "mc_sifs_dcu", tempValue);
	tempValue = getFieldForMode(devNum, "mc_ack_timeout", pLibDev->mode, pLibDev->turbo);
	writeField(devNum, "mc_ack_timeout", tempValue);
#ifdef DEBUG_MEMORY
printf("SNOOP::reg8014=%x:reg1030=%x:reg1070=%x:reg10b0=%x\n", REGR(devNum, 0x8014), REGR(devNum, 0x1030), REGR(devNum, 0x1070), REGR(devNum, 0x10b0));
	
printf("SNOOP::Writing TXDP(%d) with value %x\n", queueIndex, pLibDev->tx[queueIndex].endPktDesc);
#endif
	//write TXDP
#if defined(COBRA_AP) && defined(PCI_INTERFACE)
    if(isCobra(pLibDev->swDevID)) {
			REGW(devNum, F2_Q0_TXDP + ( 4 * queueIndex), pLibDev->tx[queueIndex].endPktDesc);
	}
	else {
		REGW(devNum, F2_Q0_TXDP + ( 4 * queueIndex), pLibDev->tx[queueIndex].endPktDesc | HOST_PCI_SDRAM_BASEADDR);
	}
#else
	REGW(devNum, F2_Q0_TXDP + ( 4 * queueIndex), pLibDev->tx[queueIndex].endPktDesc);
#endif

#ifdef DEBUG_MEMORY
printf("SNOOP::Before xmit enable set:txe=%x\n", REGR(devNum, F2_Q_TXE));
printf("SNOOP::TXDP=%x:TXE=%x\n", REGR(devNum, 0x800), REGR(devNum, F2_Q_TXE));
printf("SNOOP::Memory at TXDP %x location is \n", REGR(devNum, F2_Q0_TXDP + (4*queueIndex)));
memDisplay(devNum, REGR(devNum, F2_Q0_TXDP + (4*queueIndex)), 8);
#endif




	//write TX enable bit
	REGW(devNum, F2_Q_TXE, REGR(devNum, F2_Q_TXE) | ( F2_QCU_0 << queueIndex ) );
#ifdef DEBUG_MEMORY
printf("SNOOP::After xmit enable set:txe=%x\n", REGR(devNum, F2_Q_TXE));
#endif

// Wait for ever
/*
printf("Waiting for ever to have continuous xmit\n");
	for(i=0;i<100;) {
		printf("TXDP=%x::", REGR(devNum, 0x800));
	}
*/

	
	//wait for this packet to complete, by polling the done bit
	for(i = 0; ((i < MDK_PKT_TIMEOUT) && (!pLibDev->txProbePacketNext)); i++) {
/*
printf("SNOOP::Iteration %d *************\n", i);
printf("SNOOP::channel clear = %x\n", REGR(devNum, 0x9c38));
printf("SNOOP::TXDP=%x:TXE=%x\n", REGR(devNum, 0x800), REGR(devNum, F2_Q_TXE));
printf("SNOOP::Memory at TXDP %x location is \n", REGR(devNum, 0x800));
memDisplay(devNum, REGR(devNum, 0x800), 8);
printf("SNOOP::Memory at endpkt address %x location is \n", pLibDev->tx[queueIndex].endPktDesc);
memDisplay(devNum, pLibDev->tx[queueIndex].endPktDesc, 8);
printf("SNOOP::endpkt frame at %x is \n", pLibDev->tx[queueIndex].endPktAddr);
memDisplay(devNum, pLibDev->tx[queueIndex].endPktAddr, 8);
*/



	   pLibDev->devMap.OSmemRead(devNum,  pLibDev->tx[queueIndex].endPktDesc + pLibDev->txDescStatus2,
				(A_UCHAR *)&status2, sizeof(status2));

#ifdef DEBUG_MEMORY
printf("SNOOP::epd=%x:s2offset=%d:status2=%x\n", pLibDev->tx[queueIndex].endPktDesc, pLibDev->txDescStatus2, status2);
#endif
	   if(status2 & DESC_DONE) {
#ifdef DEBUG_MEMORY
printf("SNOOP::End Desc done\n");
#endif
		   pLibDev->devMap.OSmemRead(devNum,  pLibDev->tx[queueIndex].endPktDesc + pLibDev->txDescStatus1,
				(A_UCHAR *)&status1, sizeof(status1));
			// If not ok and not sending broadcast frames (i.e. no response required)
			if( (!(status1 & DESC_FRM_XMIT_OK)) && (pLibDev->tx[queueIndex].broadcast == 0) ) {
				mError(devNum, EIO, "sendTxEndPacket5211: end packet not successfully sent, status1 = 0x%04X:status2=%04X\n",
							status1, status2);
				nRet = 0;
				return nRet;
			}
			break;
		}
		mSleep(100);
	}

	if (i == MDK_PKT_TIMEOUT) {
		mError(devNum, EIO, "sendTxEndPacket5211: timed out waiting for end packet done, QCU (0-15) = %d\n", 
							queueIndex);
		nRet = 0;
	}
	return nRet;
}


void setDescriptorAr5211
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
//				 ((sizeof(WLAN_DATA_MAC_HEADER3) + 8 ) << BITS_TO_TX_HDR_LEN) |
//					 ((sizeof(WLAN_DATA_MAC_HEADER3) + (pLibDev->wepEnable ? WEP_ICV_FIELD : 0)) << BITS_TO_TX_HDR_LEN) |
				 (antMode << BITS_TO_TX_ANT_MODE) |
				  frameLen | 
                 ((pLibDev->wepEnable && (descNum < pLibDev->tx[queueIndex].numDesc)) ? (1 << BITS_TO_ENCRYPT_VALID) : 0));
    if (descNum == pLibDev->tx[queueIndex].numDesc - 1) {
        localDescPtr->hwControl[0] |= DESC_TX_INTER_REQ;
    }
       
    localDescPtr->hwControl[1] = ((pLibDev->wepEnable && (descNum < pLibDev->tx[queueIndex].numDesc)) ? 
													(pLibDev->wepKey << BITS_TO_ENCRYPT_KEY) : 0) |
                                ((pLibDev->wepEnable && (descNum < pLibDev->tx[queueIndex].numDesc)) ? 
													(frameLen - 6) : pktSize) |
								(broadcast ? (1 << BITS_TO_NOACK) : 0);
}


void setStatsPktDescAr5211
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

void setContDescriptorAr5211
(
 A_UINT32 devNum,
 MDK_ATHEROS_DESC*  localDescPtr,
 A_UINT32 pktSize,
 A_UINT32 antMode,
 A_UCHAR  dataRate
)
{
	localDescPtr->hwControl[0] = ( (rateValues[dataRate] << BITS_TO_TX_XMIT_RATE) | 
//				 (sizeof(WLAN_DATA_MAC_HEADER3) << BITS_TO_TX_HDR_LEN) |
				 (antMode << BITS_TO_TX_ANT_MODE) |
				 (pktSize + FCS_FIELD) );

	//clear no ack bit for better performance
	localDescPtr->hwControl[1] = pktSize;// | (1 << BITS_TO_NOACK);
	devNum = 0;	//this is not used quieting warnings
}

A_UINT32 txGetDescRateAr5211
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

/**************************************************************************
* txBeginConfig - program register ready for tx begin
*
*/
void txBeginConfigAr5211
(
 A_UINT32 devNum,
 A_BOOL	  enableInterrupt
)
{
	LIB_DEV_INFO *pLibDev = gLibInfo.pLibDevArray[devNum];
	A_UINT32 enableFlag = 0;
	A_UINT32 i = 0;
	A_UINT32 qcu = 0;
	A_UINT16 activeQueues[MAX_TX_QUEUE] = {0};	// Index of the active queue
	A_UINT16 activeQueueCount = 0;	// active queues count
	A_UINT32 imrS0,imrS0desc;	

	//clear the waitforPoll bit, incase also enabled for receive
	if(pLibDev->libCfgParams.enableXR) {
		REGW(devNum, 0x80c0, REGR(devNum, 0x80c0) & 0xFFFFFF7F);
	}

	if((pLibDev->txProbePacketNext) && (pLibDev->tx[PROBE_QUEUE].txEnable)) {
		activeQueues[activeQueueCount] = (A_UINT16)PROBE_QUEUE;
		activeQueueCount++;
	}
	else {
		for( i = 0; i < MAX_TX_QUEUE; i++ )
		{
			if ( pLibDev->tx[i].txEnable )
			{
				activeQueues[activeQueueCount] = (A_UINT16)i;
				activeQueueCount++;
			}
		}
	}

	// QUEUE SETUP
	// THIS NEEDS TO MOVED TO A BETTER PLACE
	//
	imrS0desc = 0;
	for( i = 0; i < activeQueueCount; i++ )
	{
		qcu = F2_QCU_0 << activeQueues[i];
		// program the queues
		REGW(devNum, F2_D0_QCUMASK +  ( 4 * pLibDev->tx[activeQueues[i]].dcuIndex ), 
					REGR(devNum, F2_D0_QCUMASK + (4 * pLibDev->tx[activeQueues[i]].dcuIndex )) |
					qcu );

		oahuWorkAround(devNum);

		if (enableInterrupt) {
			imrS0desc = imrS0desc | qcu;
//			REGW(devNum, F2_IMR_S0,	(F2_IMR_S0_QCU_TXDESC_M & (qcu << F2_IMR_S0_QCU_TXDESC_S)));
		}

		//enable the clocks
		enable5211QueueClocks(devNum, activeQueues[i], pLibDev->tx[activeQueues[i]].dcuIndex); 
	}
	// Program the IMR_SO Register 
	imrS0desc = imrS0desc << F2_IMR_S0_QCU_TXDESC_S;
	imrS0 = (REGR(devNum,F2_IMR_S0) & ~F2_IMR_S0_QCU_TXDESC_M) | imrS0desc;
	REGW(devNum, F2_IMR_S0,	imrS0);
	
	//
	// QUEUE SETUP

	//enable tx DESC interrupt
	if (enableInterrupt) {
		REGW(devNum, F2_IMR, REGR(devNum, F2_IMR) | F2_IMR_TXDESC); 

		//clear ISR before enable
		REGR(devNum, F2_ISR_RAC);
		REGW(devNum, F2_IER, F2_IER_ENABLE);
	}

	//write TXDP 
	for( i = 0; i < activeQueueCount; i++ )
	{
		if ( pLibDev->tx[activeQueues[i]].txEnable )
		{
#if defined(COBRA_AP) && defined(PCI_INTERFACE)
		    if(isCobra(pLibDev->swDevID)) {
			    REGW(devNum,  F2_Q0_TXDP + (4 * activeQueues[i]), pLibDev->tx[activeQueues[i]].descAddress);
			}
			else {
			    REGW(devNum,  F2_Q0_TXDP + (4 * activeQueues[i]), pLibDev->tx[activeQueues[i]].descAddress | HOST_PCI_SDRAM_BASEADDR);
			}
#else
			REGW(devNum,  F2_Q0_TXDP + (4 * activeQueues[i]), pLibDev->tx[activeQueues[i]].descAddress);
#endif
			enableFlag |= F2_QCU_0 << activeQueues[i];
		}
	}

	//improve performance
	REGW(devNum, 0x1230, 0x10);
	
	//enable unicast reception - needed to receive acks
	REGW(devNum, F2_RX_FILTER, REGR(devNum, F2_RX_FILTER) | F2_RX_UCAST);
	//enable receive 
	REGW(devNum, F2_DIAG_SW, REGR(devNum, F2_DIAG_SW) & ~F2_DIAG_RX_DIS);

	REGW(devNum, F2_CR, F2_CR_RXE);

#ifdef DEBUG_MEMORY
printf("SNOOP::txBeginConfigAr5211::Written TXDP=%x:TXE=%x\n", REGR(devNum, 0x800), REGR(devNum, 0x840));
#endif

	//write TX enable bit
	REGW(devNum, F2_Q_TXE, REGR(devNum, F2_Q_TXE) | enableFlag );
	
	return;
}

void txBeginContDataAr5211
(
 A_UINT32 devNum,
 A_UINT16 queueIndex
)
{
	LIB_DEV_INFO *pLibDev = gLibInfo.pLibDevArray[devNum];
	A_UINT16	activeQueues[MAX_TX_QUEUE] = {0};	// Index of the active queue
	A_UINT16	activeQueueCount = 0;	// active queues count
	A_UINT32 i = 0;
	A_UINT32 qcu = 0;
	A_UINT32 enableFlag = 0;

	for( i = 0; i < MAX_TX_QUEUE; i++ )
	{
		if ( pLibDev->tx[i].txEnable )
		{
			activeQueues[activeQueueCount] = (A_UINT16)i;
			activeQueueCount++;
		}
	}

	// QUEUE SETUP
	// THIS NEEDS TO MOVED TO A BETTER PLACE
	//
	for( i = 0; i < activeQueueCount; i++ )
	{
		qcu = F2_QCU_0 << activeQueues[i];
		// program the queues
		REGW(devNum, F2_D0_QCUMASK +  ( 4 * pLibDev->tx[activeQueues[i]].dcuIndex ), 
					REGR(devNum, F2_D0_QCUMASK + (4 * pLibDev->tx[activeQueues[i]].dcuIndex )) |
					qcu );

		oahuWorkAround(devNum);

		//enable the clocks
		enable5211QueueClocks(devNum, activeQueues[i], pLibDev->tx[activeQueues[i]].dcuIndex); 
	}

	//write TXDP
	for( i = 0; i < MAX_TX_QUEUE; i++ )
	{
		if ( pLibDev->tx[i].txEnable )
		{
#if defined(COBRA_AP) && defined(PCI_INTERFACE)
		    if(isCobra(pLibDev->swDevID)) {
				REGW(devNum,  F2_Q0_TXDP + (4 * i), pLibDev->tx[i].descAddress);
			}
			else {
				REGW(devNum,  F2_Q0_TXDP + (4 * i), pLibDev->tx[i].descAddress | HOST_PCI_SDRAM_BASEADDR);
			}
#else
			REGW(devNum,  F2_Q0_TXDP + (4 * i), pLibDev->tx[i].descAddress);
#endif
			enableFlag |= F2_QCU_0 << i;
		}
	}
	
    // Put PCU and DMA in continuous data mode
    REGW(devNum, 0x8054, REGR(devNum, 0x8054) | 1);

	//disable encryption since packet has no header
	REGW(devNum, F2_DIAG_SW, REGR(devNum, F2_DIAG_SW) | F2_DIAG_ENCRYPT_DIS);
	
	//write TX enable bit
	REGW(devNum, F2_Q_TXE, REGR(devNum, F2_Q_TXE) | enableFlag );

	queueIndex = 0;	//this is not used quieting warnings
}


void txBeginContFramedDataAr5211
(
 A_UINT32 devNum,
 A_UINT16 queueIndex,
 A_UINT32 ifswait,
 A_BOOL	  retries
)
{
	LIB_DEV_INFO *pLibDev = gLibInfo.pLibDevArray[devNum];
	A_UINT16	activeQueues[MAX_TX_QUEUE] = {0};	// Index of the active queue
	A_UINT16	activeQueueCount = 0;	// active queues count
	A_UINT32 i = 0;
	A_UINT32 qcu = 0;
	A_UINT32 enableFlag = 0;
	A_UINT32 tempRegValue;
	A_UINT32 turboScale = 1;

	for( i = 0; i < MAX_TX_QUEUE; i++ )
	{
		if ( pLibDev->tx[i].txEnable )
		{
			activeQueues[activeQueueCount] = (A_UINT16)i;
			activeQueueCount++;
		}
	}

	for( i = 0; i < activeQueueCount; i++ )
	{
		qcu = F2_QCU_0 << activeQueues[i];
		// program the queues
		REGW(devNum, F2_D0_QCUMASK +  ( 4 * pLibDev->tx[activeQueues[i]].dcuIndex ), 
					REGR(devNum, F2_D0_QCUMASK + (4 * pLibDev->tx[activeQueues[i]].dcuIndex )) |
					qcu );

		oahuWorkAround(devNum);

		//program the IFS register, set AIFS and CW to 0
		tempRegValue = REGR(devNum, F2_D0_LCL_IFS + (4 * pLibDev->tx[activeQueues[i]].dcuIndex )) & ~F2_D_LCL_IFS_AIFS_M
					| ((ifswait << F2_D_LCL_IFS_AIFS_S) & F2_D_LCL_IFS_AIFS_M);
		tempRegValue = tempRegValue & ~(F2_D_LCL_IFS_CWMIN_M | F2_D_LCL_IFS_CWMAX_M);
		REGW(devNum, F2_D0_LCL_IFS +  ( 4 * pLibDev->tx[activeQueues[i]].dcuIndex ), 
					tempRegValue);
		
		
		//set the retries to max - gives better performance
		if(retries) {
			REGW(devNum, F2_D0_RETRY_LIMIT + (4 * pLibDev->tx[activeQueues[i]].dcuIndex ), 0xffffffff);
		}
		else {
			REGW(devNum, F2_D0_RETRY_LIMIT + (4 * pLibDev->tx[activeQueues[i]].dcuIndex ), 0x0);
		}

		//enable the clocks
		enable5211QueueClocks(devNum, activeQueues[i], pLibDev->tx[activeQueues[i]].dcuIndex); 	
	}

	//write TXDP
	for( i = 0; i < MAX_TX_QUEUE; i++ )
	{
		if ( pLibDev->tx[i].txEnable )
		{
			//enable early QCU disable
			tempRegValue = REGR(devNum, F2_Q0_MISC + (4 * i));
			tempRegValue |= 0x800;
			REGW(devNum, F2_Q0_MISC + (4 * i ), tempRegValue);
#if defined(COBRA_AP) && defined(PCI_INTERFACE)
		    if(isCobra(pLibDev->swDevID)) {
                REGW(devNum,  F2_Q0_TXDP + (4 * i), pLibDev->tx[i].descAddress);
			}
			else {
				REGW(devNum,  F2_Q0_TXDP + (4 * i), pLibDev->tx[i].descAddress | HOST_PCI_SDRAM_BASEADDR);
			}
#else
			REGW(devNum,  F2_Q0_TXDP + (4 * i), pLibDev->tx[i].descAddress);
#endif
			enableFlag |= F2_QCU_0 << i;
		}
	}
	

	//improve the performance and decrease the interframe spacing
	if(pLibDev->turbo == TURBO_ENABLE) {
		turboScale = 2;
	}
	else {
		turboScale = 1;
	}
	REGW(devNum, F2_D_GBL_IFS_SIFS, 100*turboScale); //sifs
	REGW(devNum, F2_D_GBL_IFS_EIFS, 100*turboScale); //eifs
	REGW(devNum, F2_D_FPCTL, 0x10); //enable prefetch
	REGW(devNum, F2_TIME_OUT, 0x2);  //ACK timeout

	//write TX enable bit, clear any disable bits set first
	REGW(devNum, F2_Q_TXD, 0);

	REGW(devNum, F2_Q_TXE, REGR(devNum, F2_Q_TXE) | enableFlag );
	REGW(devNum, F2_CR, F2_CR_RXE);

	queueIndex = 0; //this is not used quieting warnings
}


void txEndContFramedDataAr5211
(
 A_UINT32 devNum,
 A_UINT16 queueIndex
)
{
	LIB_DEV_INFO *pLibDev = gLibInfo.pLibDevArray[devNum];
	A_UINT16	activeQueues[MAX_TX_QUEUE] = {0};	// Index of the active queue
	A_UINT16	activeQueueCount = 0;	// active queues count
	A_UINT32 disableFlag = 0;
	A_UINT16 i;
	A_UINT32 pendingCount = 0;
	A_CHAR  fieldName[MAX_NAME_LENGTH];
	A_UINT32 tempValue;
//	A_UINT32	zero_desc = 0;

/*
	pLibDev->devMap.OSmemWrite(devNum, pLibDev->tx[queueIndex].descAddress, (A_UCHAR*)&zero_desc, sizeof(zero_desc));
	pLibDev->devMap.OSmemWrite(devNum, pLibDev->tx[queueIndex].descAddress+4, (A_UCHAR*)&zero_desc, sizeof(zero_desc));
	pLibDev->devMap.OSmemWrite(devNum, pLibDev->tx[queueIndex].descAddress+8, (A_UCHAR*)&zero_desc, sizeof(zero_desc));
	pLibDev->devMap.OSmemWrite(devNum, pLibDev->tx[queueIndex].descAddress+0xc, (A_UCHAR*)&zero_desc, sizeof(zero_desc));
	pLibDev->devMap.OSmemWrite(devNum, pLibDev->tx[queueIndex].descAddress+0x10, (A_UCHAR*)&zero_desc, sizeof(zero_desc));
	pLibDev->devMap.OSmemWrite(devNum, pLibDev->tx[queueIndex].descAddress+0x14, (A_UCHAR*)&zero_desc, sizeof(zero_desc));
	pLibDev->devMap.OSmemWrite(devNum, pLibDev->tx[queueIndex].descAddress+0x18, (A_UCHAR*)&zero_desc, sizeof(zero_desc));
*/

	//write to the txDisable 
	for( i = 0; i < MAX_TX_QUEUE; i++ )
	{
		if ( pLibDev->tx[i].txEnable )
		{
			disableFlag |= F2_QCU_0 << i;
		}
	}

	//write TX disable bit
	REGW(devNum, F2_Q_TXD, disableFlag );



	for(i = 0; i < 2000; i++) {
		if(!REGR(devNum, F2_Q_TXE)) {
			break;
		}
		mSleep(1);
	}
	
	if(i == 2000) {
		mError(devNum, EIO, "Timed out waiting for txContEnd\n");
		return;
	}

	//make sure all the queues have stopped
	for( i = 0; i < MAX_TX_QUEUE; i++ )
	{
		if ( pLibDev->tx[i].txEnable )
		{
			activeQueues[activeQueueCount] = (A_UINT16)i;
			activeQueueCount++;
		}
	}

	for(i = 0; i < activeQueueCount; i++) {
		//wait for the pending frames to go to zero
		for(i = 0; i < 2000; i++) {
			pendingCount = REGR(devNum, F2_Q0_STS + (4 * activeQueues[i])) &  F2_Q_STS_PEND_FR_CNT_M;
			if(!pendingCount) {
				break;
			}
			mSleep(1);
		}
		
		if(i == 2000) {
			mError(devNum, EIO, "Timed out waiting for pending count 0 on QCU %d\n", activeQueues[i]);
		}

		//restore the AIFS register fields, registers were updated with direct register writes,
		//so can use a getField to get the original value of the fields
		sprintf(fieldName, "mc_aifs_dcu%d", pLibDev->tx[activeQueues[i]].dcuIndex);
		tempValue = getFieldForMode(devNum, fieldName, pLibDev->mode, pLibDev->turbo);
		writeField(devNum, fieldName, tempValue);

		sprintf(fieldName, "mc_cwmax_dcu%d", pLibDev->tx[activeQueues[i]].dcuIndex);
		tempValue = getFieldForMode(devNum, fieldName, pLibDev->mode, pLibDev->turbo);
		writeField(devNum, fieldName, tempValue);

		sprintf(fieldName, "mc_cwmin_dcu%d", pLibDev->tx[activeQueues[i]].dcuIndex);
		tempValue = getFieldForMode(devNum, fieldName, pLibDev->mode, pLibDev->turbo);
		writeField(devNum, fieldName, tempValue);

	}

	//restore the global register values
	tempValue = getFieldForMode(devNum, "mc_sifs_dcu", pLibDev->mode, pLibDev->turbo);
	writeField(devNum, "mc_sifs_dcu", tempValue);
	tempValue = getFieldForMode(devNum, "mc_ack_timeout", pLibDev->mode, pLibDev->turbo);
	writeField(devNum, "mc_ack_timeout", tempValue);
	tempValue = getFieldForMode(devNum, "mc_eifs_dcu", pLibDev->mode, pLibDev->turbo);
	writeField(devNum, "mc_eifs_dcu", tempValue);

	queueIndex = 0; //this is not used quieting warnings
}

void beginSendStatsPktAr5211
(
 A_UINT32 devNum, 
 A_UINT32 DescAddress
)
{
	LIB_DEV_INFO *pLibDev = gLibInfo.pLibDevArray[devNum];
	REGW(devNum, F2_D0_RETRY_LIMIT, (REGR(devNum, F2_D0_RETRY_LIMIT) | MAX_RETRIES));
	
	REGW(devNum, F2_D0_QCUMASK , REGR(devNum, F2_D0_QCUMASK) | F2_QCU_0 );

	oahuWorkAround(devNum);

	//enable the clocks
	enable5211QueueClocks(devNum, 0, 0); 

	//clear the waitforPoll bit, incase also enabled for receive
	if(pLibDev->libCfgParams.enableXR) {
		REGW(devNum, 0x80c0, REGR(devNum, 0x80c0) & 0xFFFFFF7F);
	}

   	REGW(devNum, F2_Q0_TXDP, DescAddress);
	REGW(devNum, F2_Q_TXE, REGR(devNum, F2_Q_TXE) | F2_QCU_0);
}


void writeRxDescriptorAr5211
( 
 A_UINT32 devNum, 
 A_UINT32 rxDescAddress
)
{
	REGW(devNum, F2_RXDP, rxDescAddress);
}


void rxBeginConfigAr5211
(
 A_UINT32 devNum
)
{
	A_UINT32	regValue = 0;
	LIB_DEV_INFO *pLibDev = gLibInfo.pLibDevArray[devNum];

	//enable rx DESC_INT
	REGW(devNum, F2_IMR, (REGR(devNum, F2_IMR) | F2_IMR_RXDESC ));
	
	//need to set receive frame gap timeout to stop getting this interrupt on RXDESC 
	REGW(devNum, F2_RPGTO, (REGR(devNum, F2_RPGTO) & ~F2_RPGTO_MASK));
	REGW(devNum, F2_RPCNT, (REGR(devNum, F2_RPCNT) | F2_RPCNT_MASK));
	//clear out any RXDESC interrupts before enabling interrups, so we dont get
	//RXDESC right away
	regValue = REGR(devNum, F2_ISR_RAC); 

	REGW(devNum, F2_IER, F2_IER_ENABLE);

#ifdef _DEBUG
	if(REGR(devNum, F2_CR) & F2_CR_RXE) {
 		mError(devNum, EIO, "F2_CR_RXE is active before receive was enabled\n");
	}
#endif
	//clear RX disable bit
	REGW(devNum, F2_CR, REGR(devNum, F2_CR) & (~F2_CR_RXD));

	//write RXDP
#if defined(COBRA_AP) && defined(PCI_INTERFACE)
6    if(isCobra(pLibDev->swDevID)) {
		REGW(devNum, F2_RXDP, pLibDev->rx.descAddress);
	}
	else {
		REGW(devNum, F2_RXDP, pLibDev->rx.descAddress | HOST_PCI_SDRAM_BASEADDR);
	}
#else		
	REGW(devNum, F2_RXDP, pLibDev->rx.descAddress);
#endif

#ifdef _DEBUG
	if(REGR(devNum, F2_RXDP) != pLibDev->rx.descAddress) {
		mError(devNum, EIO, "F2_RXDP does not equal pLibDev->rx.descAddress\n");
	}
#endif

	//enable unicast and broadcast reception 
	if(pLibDev->rx.rxMode == RX_FIXED_NUMBER) {
		REGW(devNum, F2_RX_FILTER, REGR(devNum, F2_RX_FILTER) | F2_RX_BEACON );
	}
	else {
		REGW(devNum, F2_RX_FILTER, REGR(devNum, F2_RX_FILTER) | F2_RX_UCAST | F2_RX_BCAST );
	}

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
void rxCleanupConfigAr5211
(
 A_UINT32 devNum
)
{
//    A_UINT32 tops, mibc, imr;

	//write RX disable bit
	REGW(devNum, F2_CR, REGR(devNum, F2_CR) | F2_CR_RXD);

	//set the receive disable register
	REGW(devNum, F2_DIAG_SW, REGR(devNum, F2_DIAG_SW) | F2_DIAG_RX_DIS);

	//clear rx DESC_INT
	REGW(devNum, F2_IMR, (REGR(devNum, F2_IMR) & ~F2_IMR_RXDESC));
	
	//disable IER
    REGW(devNum, F2_IER, F2_IER_DISABLE);

	//clear unicast and broadcast reception 
	REGW(devNum, F2_RX_FILTER, REGR(devNum, F2_RX_FILTER) & ~(F2_RX_UCAST | F2_RX_BCAST));

	return;

/*    tops = REGR(devNum, F2_TOPS);
    mibc = REGR(devNum, F2_MIBC);
    //bcr = REGR(devNum, F2_BCR);	// __TODO__
    imr = REGR(devNum, F2_IMR);

	//disabling RX - requires DMA RESET for now
	//REGW(devNum, F2_RC, F2_RC_DMA);
	REGW(devNum, F2_RC, F2_RC_BB);	// __TODO__
	
	mSleep(1);
	REGW(devNum, F2_RC, 0);

    // Restore Reset Registers
	REGW(devNum, F2_TOPS, tops);
    REGW(devNum, F2_MIBC, mibc);
    //REGW(devNum, F2_BCR, bcr);
    REGW(devNum, F2_IMR, imr);
*/
}

/**************************************************************************
* txCleanupConfig - cleanup register state bits we set
*
*/
void txCleanupConfigAr5211
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

	//disable the clocks
	disable5211QueueClocks(devNum); 

	return;
}



void setPPM5211
(
 A_UINT32 devNum,
 A_UINT32 enablePPM
)
{
	LIB_DEV_INFO *pLibDev = gLibInfo.pLibDevArray[devNum];


	if(enablePPM) {
		REGW(devNum, F2_DIAG_SW, REGR(devNum, F2_DIAG_SW) | F2_DIAG_CHAN_INFO);
		REGW(devNum, PHY_FRAME_CONTROL1, REGR(devNum, PHY_FRAME_CONTROL1) |
			0x10000);
		pLibDev->rx.enablePPM = 1;
	}
	else {
		REGW(devNum, F2_DIAG_SW, REGR(devNum, F2_DIAG_SW) & ~F2_DIAG_CHAN_INFO);
		REGW(devNum, PHY_FRAME_CONTROL1, REGR(devNum, PHY_FRAME_CONTROL1) &
			~0x10000);
		pLibDev->rx.enablePPM = 0;
	}
}


A_UINT32 isTxdescEvent5211
(
 A_UINT32 eventISRValue
)
{
	return eventISRValue & F2_ISR_TXDESC;
}

A_UINT32 isRxdescEvent5211
(
 A_UINT32 eventISRValue
)
{
	return eventISRValue & F2_ISR_RXDESC;
}


A_UINT32 isTxComplete5211
(
 A_UINT32 devNum
)
{	
	LIB_DEV_INFO *pLibDev = gLibInfo.pLibDevArray[devNum];
	A_UINT32 i = 0;
//	A_UINT32 qcu = 0;
	A_UINT32 enableFlag = 0;
	A_UINT32 regValue;

	for( i = 0; i < MAX_TX_QUEUE; i++ )
	{
		if ( pLibDev->tx[i].txEnable )
		{
			enableFlag |= F2_QCU_0 << i;
		}
	}
	regValue = REGR(devNum, F2_Q_TXE); 
	return(!( regValue & enableFlag));
}

//Note these are the same implementation at 5211, however the offset of the 
//register is different 
void disableRx5211
(
 A_UINT32 devNum
)
{
	REGW(devNum, F2_DIAG_SW, F2_DIAG_RX_DIS);
}

void enableRx5211
(
 A_UINT32 devNum
)
{
	REGW(devNum, F2_DIAG_SW, REGR(devNum, F2_DIAG_SW) & ~F2_DIAG_RX_DIS);
}

void enable5211QueueClocks
(
	A_UINT32 devNum,
	A_UINT32 qcuIndex,
	A_UINT32 dcuIndex
)
{
	A_UINT32 clockEnable;

	clockEnable = ~((0x01 << qcuIndex) | 0x10000 << dcuIndex);
	REGW(devNum, F2_QDCKLGATE, (REGR(devNum, F2_QDCKLGATE) & clockEnable)); 
	return;
}

void disable5211QueueClocks
(
	A_UINT32 devNum
)
{
	REGW(devNum, 0x005c, 0x3ffffff);
	return;
}

void oahuWorkAround
(
	A_UINT32 devNum
)
{
	A_UINT32 regValue;

	//WORKAROUND needed
	//read then write to the D_GBL_IFS_EIFS reg
	regValue = REGR(devNum, 0x10b0);
	REGW(devNum, 0x10b0, regValue);

}

// Set the selQueueIndex to the qcu number
// This number is used by the subsequent txDataSetup calls.

void setQueueAr5211
(
	A_UINT32 devNum,
	A_UINT32 qcuNumber
)
{
	LIB_DEV_INFO *pLibDev = gLibInfo.pLibDevArray[devNum];

	if (qcuNumber > 9) {
		mError(devNum, EINVAL,"Queue number should be between 0 and 9 \n");
		return;
	}
	pLibDev->selQueueIndex = (A_UINT16)qcuNumber;
}

void mapQueueAr5211
(
	A_UINT32 devNum,
	A_UINT32 qcuNumber,
	A_UINT32 dcuNumber
)
{
	LIB_DEV_INFO *pLibDev = gLibInfo.pLibDevArray[devNum];

	if (qcuNumber > 9) {
		mError(devNum, EINVAL,"QCU number should be between 0 and 9 \n");
		return;
	}
	if (dcuNumber > 9) {
		mError(devNum, EINVAL,"DCU number should be between 0 and 9 \n");
		return;
	}

	pLibDev->tx[qcuNumber].dcuIndex = (A_UINT16)dcuNumber;
}

void clearKeyCacheAr5211
(
	A_UINT32 devNum
)
{
#ifndef NO_LIB_PRINT		
	printf("ClearKeyCache is not currently implemented \n");
#endif
	devNum = 0; // used to quiet warning messages
}

static A_UINT32 bb_thresh62_val_preDeaf;
static A_UINT32 bb_rssi_thr1a_val_preDeaf;
static A_UINT32 bb_cycpwr_thr1_val_preDeaf;
static A_UINT32 bb_force_agc_clear_val_preDeaf;

void AGCDeafAr5211
(
 A_UINT32 devNum
)
{
	LIB_DEV_INFO *pLibDev = gLibInfo.pLibDevArray[devNum];
	
	//writeField(devNum, "bb_thresh62", 127);
	bb_thresh62_val_preDeaf = getFieldForMode(devNum, "bb_thresh62", pLibDev->mode, pLibDev->turbo);

	bb_force_agc_clear_val_preDeaf = getFieldForMode(devNum, "bb_force_agc_clear", pLibDev->mode, pLibDev->turbo);
	REGW(devNum, 0x9800, REGR(devNum, 0x9800) | 0x10000000 );

	REGW(devNum, 0x9864, REGR(devNum, 0x9864) | 0x7f000 );
	//writeField(devNum, "bb_rssi_thr1a", 127);
	bb_rssi_thr1a_val_preDeaf = getFieldForMode(devNum, "bb_rssi_thr1a", pLibDev->mode, pLibDev->turbo);
	REGW(devNum, 0x9924, REGR(devNum, 0x9924) | 0x7f00fe);
	//writeField(devNum, "bb_cycpwr_thr1", 127);
	bb_cycpwr_thr1_val_preDeaf = getFieldForMode(devNum, "bb_cycpwr_thr1", pLibDev->mode, pLibDev->turbo);
	return;
}

void AGCUnDeafAr5211
(
  A_UINT32 devNum
)
{
	writeField(devNum, "bb_thresh62", bb_thresh62_val_preDeaf);
	//REGW(devNum, 0x9864, REGR(devNum, 0x9864) | 0x7f000 );
	writeField(devNum, "bb_rssi_thr1a", bb_rssi_thr1a_val_preDeaf);
	//REGW(devNum, 0x9924, REGR(devNum, 0x9924) | 0x7f00fe);
	writeField(devNum, "bb_cycpwr_thr1", bb_cycpwr_thr1_val_preDeaf);
	writeField(devNum, "bb_force_agc_clear", bb_force_agc_clear_val_preDeaf);
}
