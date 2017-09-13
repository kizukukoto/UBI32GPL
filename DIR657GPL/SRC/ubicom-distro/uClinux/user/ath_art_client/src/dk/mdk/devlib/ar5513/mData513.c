/*
 *  Copyright © 2003 Atheros Communications, Inc.,  All Rights Reserved.
 *
 */

#ident  "ACI $Id: //depot/sw/src/dk/mdk/devlib/ar5513/mData513.c#3 $, $Header: //depot/sw/src/dk/mdk/devlib/ar5513/mData513.c#3 $"

#ifdef VXWORKS
#include "vxworks.h"
#endif

#ifdef __ATH_DJGPPDOS__
#define __int64	long long
typedef unsigned long DWORD;
#define Sleep	delay
#endif	// #ifdef __ATH_DJGPPDOS__

#include <errno.h>
#include "mData513.h"
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
#include "stdio.h"

#define F2_D0_QCUMASK     0x1000 // MAC QCU Mask

void macAPIInitAr5513
(
 A_UINT32 devNum
) 
{
	LIB_DEV_INFO *pLibDev = gLibInfo.pLibDevArray[devNum];
	pLibDev->antRssiDescStatus = FALCON_ANT_RSSI_TX_STATUS_WORD;
	pLibDev->txDescStatus1 = FIRST_FALCON_TX_STATUS_WORD;
	pLibDev->txDescStatus2 = SECOND_FALCON_TX_STATUS_WORD;
	pLibDev->decryptErrMsk = VENICE_DESC_DECRYPT_ERROR;
	pLibDev->bitsToRxSigStrength = VENICE_BITS_TO_RX_SIG_STRENGTH; // legacy rssi location
	pLibDev->rxDataRateMsk = VENICE_DATA_RATE_MASK;
	
	return;
}

#define ENABLE_FAST_ANT_DIV		0x00002000
#define ENABLE_WEAK_OFDM_FAST_ANT_DIV		0x00200000

A_UINT32 setupAntennaAr5513
(
 A_UINT32 devNum, 
 A_UINT32 antenna, 
 A_UINT32* antModePtr   // retVal used by setDescr, createDescr etc...
)
{
	LIB_DEV_INFO *pLibDev = gLibInfo.pLibDevArray[devNum];
	int nRet = 0;
//	A_UINT32	switchTable[10];
	A_UINT32 tempValue;
	A_UINT32 chainSel = 0;
	A_UINT32 antChain0;
	A_UINT32 antChain1;
	A_UINT32 ii, jj;

	if (pLibDev->libCfgParams.chainSelect == 1) {
		chainSel = 1;
	}

	//update antenna
	if (antenna & USE_DESC_ANT) {
		//clear this bit
		*antModePtr = antenna & ~USE_DESC_ANT;
		
		if(*antModePtr > (DESC_ANT_B | DESC_CHAIN_2_ANT_B)) {
			mError(devNum, EINVAL, "setupAntenna: Illegal antenna setting value %08lx\n", antenna);
			return nRet;
		}

		antChain0 = (antenna & 0x1);
		antChain1 = ((antenna >> 16) & 0x1);
		// mc_default_antenna[3:0] = {reserved, chain_sel, rx_defAnt_ch1, rx_defAnt_ch0}
		
		*antModePtr = antChain0 | (antChain1 << 1) | (chainSel << 2);

		// setup the registers for receive antennas (of unsolicited traffic)
		REGW(devNum, F2_DEF_ANT, *antModePtr);
//		printf("SNOOP: setupAntenna : reg [0x%x] = 0x%x\n", F2_DEF_ANT, *antModePtr);

		// this actually disables the "use_ant_descr" because falcon does not support antenna spec in descr
		REGW(devNum, F2_STA_ID1, (REGR(devNum, F2_STA_ID1) | F2_STA_ID1_DEFANT_UPDATE ) &  ~F2_STA_ID1_USE_DEFANT ); // not use def ant

		//disable the fast antenna diversity
//		tempValue = (REGR(devNum, 0xa208) & ~ENABLE_FAST_ANT_DIV);
//		REGW(devNum, 0xa208, tempValue);

		tempValue = (REGR(devNum, 0x9970) & ~ENABLE_WEAK_OFDM_FAST_ANT_DIV);
		REGW(devNum, 0x9970, tempValue);

		// setup transmit and ack receive antennas in the keycache
		tempValue = ((0x7 << 0) | (0x0 << 3) | (0x00 << 4) | (1 << 9) | 
		     (antChain0 << 10) | (antChain1 << 11) | (antChain0 << 12) | (antChain1 << 13) | (chainSel << 14)) ;

		// setup broadcast and unicast key indices for appropriate antennas
		for(ii=0; ii<2; ii++) {
			jj = (ii > 0) ? FALCON_UNICAST_DEST_INDEX : FALCON_BROADCAST_DEST_INDEX;
			REGW(devNum, (F2_KEY_CACHE_START + ((8*jj)<<2) + 0x00), 0x00000000);
			REGW(devNum, (F2_KEY_CACHE_START + ((8*jj)<<2) + 0x04), 0x00000000);
			REGW(devNum, (F2_KEY_CACHE_START + ((8*jj)<<2) + 0x08), 0x00000000);
			REGW(devNum, (F2_KEY_CACHE_START + ((8*jj)<<2) + 0x0C), 0x00000000);
			REGW(devNum, (F2_KEY_CACHE_START + ((8*jj)<<2) + 0x10), 0x00000000);
			REGW(devNum, (F2_KEY_CACHE_START + ((8*jj)<<2) + 0x14), tempValue);
			REGW(devNum, (F2_KEY_CACHE_START + ((8*jj)<<2) + 0x18), 0xFFFFFFFC); // dummy addr. ensure lower 2 bits to be 0 (else multicast)
			REGW(devNum, (F2_KEY_CACHE_START + ((8*jj)<<2) + 0x1C), 0x0000FFFF);
		}
		
		*antModePtr = 0; // for falcon, descr bits [28:25] are 0 [unused]
	}
	else {
		//setup for normal antenna		// __TODO__
		mError(devNum, EIO, "setupAntennaAr5211: Normal antenna (alternating mode) not currently implemented\n");
		printf("setupAntennaAr5513 : not setting antennas.\n");
		return nRet;			
		//REGW(devNum, F2_STA_ID1, (REGR(devNum, F2_STA_ID1) & ~F2_STA_ID1_DESC_ANT)); 
	}
	nRet = 1;
	return nRet;
}

void writeRxDescriptorAr5513
( 
 A_UINT32 devNum, 
 A_UINT32 rxDescAddress
)
{
	REGW(devNum, F2_RXDP, FALCON_MEM_ADDR_MASK | rxDescAddress);
//printf("SNOOP: writeRxDescriptorAr5513 : here 1. reg [0x%x] = 0x%x (0x%x)\n", F2_RXDP, FALCON_MEM_ADDR_MASK | rxDescAddress, REGR(devNum, F2_RXDP));
}

void setDescriptorAr5513
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
	MDK_FALCON_DESC  *localFalconDescPtr = (MDK_FALCON_DESC *)localDescPtr;
	A_UINT32	numAttempts;
	A_BOOL      xrRate = 0;
	A_UINT32    enable_beamforming = 0;
	A_UINT32    dest_idx = (broadcast ? FALCON_BROADCAST_DEST_INDEX : FALCON_UNICAST_DEST_INDEX);

	antMode = 0; // to quiet warnings. not needed for falcon

	// make sure about the FALCON_MEM_ADDR_MASK for non-Null pointers
	localFalconDescPtr->bufferPhysPtr |= FALCON_MEM_ADDR_MASK;
	if ((localFalconDescPtr->nextPhysPtr & 0x7FFFFFFF) > 0) {
		localFalconDescPtr->nextPhysPtr |= FALCON_MEM_ADDR_MASK;
	}

	//check for this being an xr rate
	if (rateIndex >= LOWEST_XR_RATE_INDEX && rateIndex <= HIGHEST_XR_RATE_INDEX) {
		xrRate = 1;
	}

	if (pLibDev->libCfgParams.chainSelect == 2) {
		enable_beamforming = 1 ;
	}

	//create and write the 6 control words
    frameLen = (pktSize + FCS_FIELD) + ((pLibDev->wepEnable && (descNum < pLibDev->tx[queueIndex].numDesc)) ? WEP_ICV_FIELD : 0);
	localFalconDescPtr->hwControl[0] = frameLen | 0x01000000 |  //clear dest mask
				 (enable_beamforming << 13); 
	

    if (descNum == pLibDev->tx[queueIndex].numDesc - 1) {
        localFalconDescPtr->hwControl[0] |= DESC_TX_INTER_REQ;
    }
    
	if(xrRate) {
		localFalconDescPtr->hwControl[0] |= DESC_ENABLE_CTS;
	}

	localFalconDescPtr->hwControl[1] = pktSize | (broadcast ? (1 << BITS_TO_VENICE_NOACK) : 0) |
												(dest_idx << 13);



	//setup the remainder 2 control words for venice
	//for now only setup one group of retries
	//add 1 to retries to account for 1st attempt (but if retries is 15, don't add at moment)
	numAttempts = pLibDev->tx[queueIndex].retryValue;
	if(numAttempts < 0xf) {
		numAttempts++;
	}
	localFalconDescPtr->hwControl[2] = numAttempts << BITS_TO_DATA_TRIES0;
	localFalconDescPtr->hwControl[3] = rateValues[rateIndex] << BITS_TO_TX_DATA_RATE0;
	if(xrRate) {
		localFalconDescPtr->hwControl[3] |= (rateValues[0] << BITS_TO_RTS_CTS_RATE);
	}
	localFalconDescPtr->hwControl[4] = 0; // pktDur0, pktDur1 = 0
	localFalconDescPtr->hwControl[5] = 0; // pktDur2, pktDur3 = 0
}

/**************************************************************************
* txBeginConfig - program register ready for tx begin
*
*/
void txBeginConfigAr5513
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
	A_UINT32 temp;

	//clear the waitforPoll bit, incase also enabled for receive
//	if(pLibDev->libCfgParams.enableXR) {
//		REGW(devNum, 0x80c0, REGR(devNum, 0x80c0) & 0xFFFFFF7F);
//	}

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
	imrS0desc = 0;
	for( i = 0; i < activeQueueCount; i++ )
	{
		qcu = F2_QCU_0 << activeQueues[i];
		// program the queues
		REGW(devNum, F2_D0_QCUMASK +  ( 4 * pLibDev->tx[activeQueues[i]].dcuIndex ), 
					REGR(devNum, F2_D0_QCUMASK + (4 * pLibDev->tx[activeQueues[i]].dcuIndex )) |
					qcu );

//		oahuWorkAround(devNum);

		if (enableInterrupt) {
			imrS0desc = imrS0desc | qcu;
//			REGW(devNum, F2_IMR_S0,	(F2_IMR_S0_QCU_TXDESC_M & (qcu << F2_IMR_S0_QCU_TXDESC_S)));
		}

		//enable the clocks
		enable5513QueueClocks(devNum, activeQueues[i], pLibDev->tx[activeQueues[i]].dcuIndex); 
	}
	// Program the IMR_SO Register 
	imrS0desc = imrS0desc << F2_IMR_S0_QCU_TXDESC_S;
	imrS0 = (REGR(devNum,F2_IMR_S0) & F2_IMR_S0_QCU_TXDESC_M) | imrS0desc;
	REGW(devNum, F2_IMR_S0,	imrS0);
	
	//
	// QUEUE SETUP

	//enable tx DESC interrupt
	if (enableInterrupt) {
//	if(1) {
		REGW(devNum, F2_IMR, REGR(devNum, F2_IMR) | F2_IMR_TXDESC); 

		//clear ISR before enable
		REGR(devNum, F2_ISR_RAC);
		REGW(devNum, F2_IER, F2_IER_ENABLE);
	}
#ifdef FALCON_ART_CLIENT
#if 0
	// Disable Miscellaneous Interrupt register RST_MIMR
	sysRegWrite(0x11000024, 0);

	// Disable UART Interrupts UART_IER
	sysRegWrite(0x11100004, 0);

	// Disable Localbus Interrupts LB_IER
	sysRegWrite(0x10400508, 0);

	// Disable MPEG Interrupts MPG_IER
	sysRegWrite(0x10200508, 0);

	// new to falcon 2.0
	// disable cpu timer interrupt
	sysRegWrite(0x110000b8, ( sysRegRead(0x110000b8) & (~0x20)));

	// Enable PCI Interrupts PCI_IER
//	REGW(devNum, 0x10508, 1);

// mSleep(1000);
	// Enable global interrupt
	temp = sysRegRead(0x11000018);
	sysRegWrite(0x11000018, temp | (1 << 5));
#endif
#else
  if (isFalcon(devNum)) {
	// Disable Miscellaneous Interrupt register RST_MIMR
	REGW(devNum, 0x14024, 0);

	// Disable UART Interrupts UART_IER
	REGW(devNum, 0x15004, 0);

	// Disable Localbus Interrupts LB_IER
	REGW(devNum, 0x13508, 0);

	// Disable MPEG Interrupts MPG_IER
	REGW(devNum, 0x11508, 0);

	// new to falcon 2.0
	// disable cpu timer interrupt
	REGW(devNum, 0x140b8, ( REGR(devNum, 0x140b8) & (~0x20)));

	// Enable PCI Interrupts PCI_IER
//	REGW(devNum, 0x10508, 1);

// mSleep(1000);
	// Enable global interrupt
	temp = REGR(devNum, 0x14018);
	REGW(devNum, 0x14018, temp | (1 << 5));
  }
#endif
/*
	temp = REGR(devNum, F2_IER);
	printf("SNOOP: txbeginConfig : F2_IER = 0x%x\n", temp);
	temp = REGR(devNum, F2_IMR);
	printf("SNOOP: txbeginConfig : F2_IMR = 0x%x\n", temp);
	temp = REGR(devNum, F2_IMR_S0);
	printf("SNOOP: txbeginConfig : F2_IMR_S0 = 0x%x\n", temp);
*/

	//write TXDP
	for( i = 0; i < MAX_TX_QUEUE; i++ )
	{
		if ( pLibDev->tx[i].txEnable )
		{
			REGW(devNum,  F2_Q0_TXDP + (4 * i), FALCON_MEM_ADDR_MASK | pLibDev->tx[i].descAddress);
			enableFlag |= F2_QCU_0 << i;
//printf("SNOOP : TXDP for que %d = 0x%x\n", i, REGR(devNum, F2_Q0_TXDP + (4 * i)));
		}
	}

	//improve performance
	REGW(devNum, 0x1230, 0x10);
	
	//enable unicast reception - needed to receive acks
	REGW(devNum, F2_RX_FILTER, REGR(devNum, F2_RX_FILTER) | F2_RX_UCAST);
	//enable receive 
	REGW(devNum, F2_DIAG_SW, REGR(devNum, F2_DIAG_SW) & ~F2_DIAG_RX_DIS);

	REGW(devNum, F2_CR, F2_CR_RXE);

	//write TX enable bit
	REGW(devNum, F2_Q_TXE, REGR(devNum, F2_Q_TXE) | enableFlag );

//printf("SNOOP : TX enable : reg[0x%x] = 0x%x\n", F2_Q_TXE, REGR(devNum, F2_Q_TXE) );
	
	return;
}

void setContDescriptorAr5513
(
 A_UINT32 devNum,
 MDK_ATHEROS_DESC*  localDescPtr,
 A_UINT32 pktSize,
 A_UINT32 antMode,
 A_UCHAR  dataRate
)
{
	LIB_DEV_INFO *pLibDev = gLibInfo.pLibDevArray[devNum];
	MDK_FALCON_DESC  *localFalconDescPtr = (MDK_FALCON_DESC *)localDescPtr;
	A_UINT32    enable_beamforming = 0;

	if (pLibDev->libCfgParams.chainSelect == 2) {
		enable_beamforming = 1;
	}

	// make sure about the FALCON_MEM_ADDR_MASK for non-Null pointers
	localFalconDescPtr->bufferPhysPtr |= FALCON_MEM_ADDR_MASK;
	if ((localFalconDescPtr->nextPhysPtr & 0x7FFFFFFF) > 0) {
		localFalconDescPtr->nextPhysPtr |= FALCON_MEM_ADDR_MASK;
	}

	localFalconDescPtr->hwControl[0] = (pktSize + FCS_FIELD) | (enable_beamforming << 13);

	// clear no ack bit for better performance with slow laptops to avoid 
	// underruns. unicast with max retries gives enough time to dma
	localFalconDescPtr->hwControl[1] = pktSize | FALCON_UNICAST_DEST_INDEX;
	
	//set max attempts for 1 exchange for better performance
	localFalconDescPtr->hwControl[2] = 0xf << BITS_TO_DATA_TRIES0;
	localFalconDescPtr->hwControl[3] = rateValues[dataRate] << BITS_TO_TX_DATA_RATE0;
	localFalconDescPtr->hwControl[4] = 0; // pktDur0, pktDur1 = 0
	localFalconDescPtr->hwControl[5] = 0; // pktDur2, pktDur3 = 0

	devNum = 0;	//this is not used quieting warnings
	antMode = 0;	//this is not used quieting warnings
}

void enable5513QueueClocks
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

/*
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
*/

void txBeginContDataAr5513
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

//		oahuWorkAround(devNum);

		//enable the clocks
		enable5513QueueClocks(devNum, activeQueues[i], pLibDev->tx[activeQueues[i]].dcuIndex); 
	}

	//write TXDP
	for( i = 0; i < MAX_TX_QUEUE; i++ )
	{
		if ( pLibDev->tx[i].txEnable )
		{
			REGW(devNum,  F2_Q0_TXDP + (4 * i), FALCON_MEM_ADDR_MASK | pLibDev->tx[i].descAddress);
			enableFlag |= F2_QCU_0 << i;
//printf("SNOOP : contpacket : TXDP for que %d = 0x%x\n", i, REGR(devNum, F2_Q0_TXDP + (4 * i)));
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

void txBeginContFramedDataAr5513
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

//		oahuWorkAround(devNum);

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
		enable5513QueueClocks(devNum, activeQueues[i], pLibDev->tx[activeQueues[i]].dcuIndex); 	
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
			REGW(devNum,  F2_Q0_TXDP + (4 * i), (FALCON_MEM_ADDR_MASK | pLibDev->tx[i].descAddress));
			enableFlag |= F2_QCU_0 << i;
		}
	}
	

	//improve the performance and decrease the interframe spacing
	REGW(devNum, F2_D_GBL_IFS_SIFS, 100); //sifs
	REGW(devNum, F2_D_GBL_IFS_EIFS, 100); //eifs
	REGW(devNum, F2_D_FPCTL, 0x10); //enable prefetch
	REGW(devNum, F2_TIME_OUT, 0x2);  //ACK timeout

	//write TX enable bit, clear any disable bits set first
	REGW(devNum, F2_Q_TXD, 0);

	REGW(devNum, F2_Q_TXE, REGR(devNum, F2_Q_TXE) | enableFlag );
	REGW(devNum, F2_CR, F2_CR_RXE);

	queueIndex = 0; //this is not used quieting warnings
}

void setDescriptorEndPacketAr5513
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
	MDK_FALCON_DESC  *localFalconDescPtr = (MDK_FALCON_DESC *)localDescPtr;
	A_UINT32	numAttempts;
	A_UINT16	 queueIndex = pLibDev->selQueueIndex;

	setDescriptorAr5513(devNum, localDescPtr, pktSize, antMode, descNum, rateIndex, broadcast);

	numAttempts = pLibDev->tx[queueIndex].retryValue;
	if(numAttempts < 0xf) {
		numAttempts++;
	}
	//set a second set of attempts at rate 6.
	localFalconDescPtr->hwControl[2] = localFalconDescPtr->hwControl[2] | (numAttempts << BITS_TO_DATA_TRIES1);
	localFalconDescPtr->hwControl[3] = localFalconDescPtr->hwControl[3] | (rateValues[0] << BITS_TO_TX_DATA_RATE1);
}


void rxBeginConfigAr5513
(
 A_UINT32 devNum
)
{
	A_UINT32	regValue = 0;
	LIB_DEV_INFO *pLibDev = gLibInfo.pLibDevArray[devNum];
	A_UINT32    temp;

	//enable rx DESC_INT
	REGW(devNum, F2_IMR, (REGR(devNum, F2_IMR) | F2_IMR_RXDESC ));
	
	//need to set receive frame gap timeout to stop getting this interrupt on RXDESC 
	REGW(devNum, F2_RPGTO, (REGR(devNum, F2_RPGTO) & ~F2_RPGTO_MASK));
	REGW(devNum, F2_RPCNT, (REGR(devNum, F2_RPCNT) | F2_RPCNT_MASK));
	//clear out any RXDESC interrupts before enabling interrups, so we dont get
	//RXDESC right away
	regValue = REGR(devNum, F2_ISR_RAC); 

	REGW(devNum, F2_IER, F2_IER_ENABLE);

#ifdef FALCON_ART_CLIENT
#if 0
	// Disable Miscellaneous Interrupt register RST_MIMR
	sysRegWrite(0x11000024, 0);

	// Disable UART Interrupts UART_IER
	sysRegWrite(0x11100004, 0);

	// Disable Localbus Interrupts LB_IER
	sysRegWrite(0x10400508, 0);

	// Disable MPEG Interrupts MPG_IER
	sysRegWrite(0x10200508, 0);

    // new to falcon 2.0
	// disable cpu timer interrupt
	sysRegWrite(0x110000b8, ( sysRegRead(0x110000b8) & (~0x20)));


	// Enable PCI Interrupts PCI_IER
//	REGW(devNum, 0x10508, 1);

	// Enable global interrupt
	temp = sysRegRead(0x11000018);
	sysRegWrite(0x11000018, temp | (1 << 5));
#endif
#else
	// Disable Miscellaneous Interrupt register RST_MIMR
  if (isFalcon(devNum)) {
	REGW(devNum, 0x14024, 0);

	// Disable UART Interrupts UART_IER
	REGW(devNum, 0x15004, 0);

	// Disable Localbus Interrupts LB_IER
	REGW(devNum, 0x13508, 0);

	// Disable MPEG Interrupts MPG_IER
	REGW(devNum, 0x11508, 0);

    // new to falcon 2.0
	// disable cpu timer interrupt
	REGW(devNum, 0x140b8, ( REGR(devNum, 0x140b8) & (~0x20)));


	// Enable PCI Interrupts PCI_IER
//	REGW(devNum, 0x10508, 1);

	// Enable global interrupt
	temp = REGR(devNum, 0x14018);
	REGW(devNum, 0x14018, temp | (1 << 5));
  }
#endif

#ifdef _DEBUG
	if(REGR(devNum, F2_CR) & F2_CR_RXE) {
 		mError(devNum, EIO, "F2_CR_RXE is active before receive was enabled\n");
	}
#endif
	//clear RX disable bit
	REGW(devNum, F2_CR, REGR(devNum, F2_CR) & (~F2_CR_RXD));


	//write RXDP
	REGW(devNum, F2_RXDP, FALCON_MEM_ADDR_MASK | pLibDev->rx.descAddress);

#ifdef _DEBUG
	if(REGR(devNum, F2_RXDP) != ( FALCON_MEM_ADDR_MASK | pLibDev->rx.descAddress)) {
		mError(devNum, EIO, "F2_RXDP (0x%x) does not equal pLibDev->rx.descAddress (0x%x)\n", REGR(devNum, F2_RXDP), pLibDev->rx.descAddress);
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

void setPPM5513
(
 A_UINT32 devNum,
 A_UINT32 enablePPM
)
{
	LIB_DEV_INFO *pLibDev = gLibInfo.pLibDevArray[devNum];


	if(enablePPM) {
		if (pLibDev->libCfgParams.chainSelect == 2) {			
			REGW(devNum, F2_DIAG_SW, REGR(devNum, F2_DIAG_SW) | F2_DIAG_CHAN_INFO | F2_DUAL_CHAIN_CHAN_INFO);
		} else {
			REGW(devNum, F2_DIAG_SW, (REGR(devNum, F2_DIAG_SW) & ~F2_DUAL_CHAIN_CHAN_INFO) | F2_DIAG_CHAN_INFO);
		}
		REGW(devNum, PHY_FRAME_CONTROL1, REGR(devNum, PHY_FRAME_CONTROL1) |
			0x10000);
		pLibDev->rx.enablePPM = 1;
	}
	else {
		REGW(devNum, F2_DIAG_SW, REGR(devNum, F2_DIAG_SW) & ~F2_DIAG_CHAN_INFO & ~F2_DUAL_CHAIN_CHAN_INFO);
		REGW(devNum, PHY_FRAME_CONTROL1, REGR(devNum, PHY_FRAME_CONTROL1) &
			~0x10000);
		pLibDev->rx.enablePPM = 0;
	}
}

void setStatsPktDescAr5513
(
 A_UINT32 devNum,
 MDK_ATHEROS_DESC*  localDescPtr,
 A_UINT32 pktSize,
 A_UINT32 rateValue
)
{
	LIB_DEV_INFO *pLibDev = gLibInfo.pLibDevArray[devNum];
	A_UINT32 tempRateValue = rateValue;
	MDK_FALCON_DESC  *localFalconDescPtr = (MDK_FALCON_DESC *)localDescPtr;
	A_UINT32    enable_beamforming = 0;

	if (pLibDev->libCfgParams.chainSelect == 2) {
		enable_beamforming = 1;
	}

	// make sure about the FALCON_MEM_ADDR_MASK for non-Null pointers
	localFalconDescPtr->bufferPhysPtr |= FALCON_MEM_ADDR_MASK;
	if ((localFalconDescPtr->nextPhysPtr & 0x7FFFFFFF) > 0) {
		localFalconDescPtr->nextPhysPtr |= FALCON_MEM_ADDR_MASK;
	}

	localFalconDescPtr->hwControl[0] = (pktSize + FCS_FIELD) | (enable_beamforming << 13);

	if(pLibDev->mode == MODE_11G) {
		tempRateValue = rateValues[8];  //try an attempt at 1Mbps 
	}

	localFalconDescPtr->hwControl[1] = pktSize;

	localFalconDescPtr->hwControl[2] = (0xf << BITS_TO_DATA_TRIES0) | (0xf << BITS_TO_DATA_TRIES1);
	localFalconDescPtr->hwControl[3] = (rateValue << BITS_TO_TX_DATA_RATE0) | (tempRateValue << BITS_TO_TX_DATA_RATE1);
	devNum = 0;		//this is not used quieting warnings
	return;
}

void beginSendStatsPktAr5513
(
 A_UINT32 devNum, 
 A_UINT32 DescAddress
)
{
	REGW(devNum, F2_D0_RETRY_LIMIT, (REGR(devNum, F2_D0_RETRY_LIMIT) | MAX_RETRIES));
	
	REGW(devNum, F2_D0_QCUMASK , REGR(devNum, F2_D0_QCUMASK) | F2_QCU_0 );

	enable5513QueueClocks(devNum, 0, 0); 

	//clear the waitforPoll bit, incase also enabled for receive

	/*  No XR for falcon */
/*
	if(pLibDev->libCfgParams.enableXR) {
		REGW(devNum, 0x80c0, REGR(devNum, 0x80c0) & 0xFFFFFF7F);
	}
*/

   	REGW(devNum, F2_Q0_TXDP, FALCON_MEM_ADDR_MASK | DescAddress);
	REGW(devNum, F2_Q_TXE, REGR(devNum, F2_Q_TXE) | F2_QCU_0);
}

/**************************************************************************
* rxCleanupConfig - program register ready for rx begin
*
*/
void rxCleanupConfigAr5513
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
}

A_UINT32 sendTxEndPacketAr5513
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
	
	//write TXDP
	REGW(devNum, F2_Q0_TXDP + ( 4 * queueIndex), (FALCON_MEM_ADDR_MASK | pLibDev->tx[queueIndex].endPktDesc));

	//write TX enable bit
	REGW(devNum, F2_Q_TXE, REGR(devNum, F2_Q_TXE) | ( F2_QCU_0 << queueIndex ) );

	//wait for this packet to complete, by polling the done bit
	for(i = 0; i < MDK_PKT_TIMEOUT; i++) {
	   pLibDev->devMap.OSmemRead(devNum,  (pLibDev->tx[queueIndex].endPktDesc & FALCON_DESC_ADDR_MASK) + pLibDev->txDescStatus2,
				(A_UCHAR *)&status2, sizeof(status2));
#ifdef _DEBUG_MEM
       printf("End Desc contents \n");
       memDisplay(devNum, pLibDev->tx[queueIndex].endPktDesc, 12);
#endif

	   if(status2 & DESC_DONE) {
		   pLibDev->devMap.OSmemRead(devNum,  (pLibDev->tx[queueIndex].endPktDesc & FALCON_DESC_ADDR_MASK) + pLibDev->txDescStatus1,
				(A_UCHAR *)&status1, sizeof(status1));
			// If not ok and not sending broadcast frames (i.e. no response required)
			if( (!(status1 & DESC_FRM_XMIT_OK)) && (pLibDev->tx[queueIndex].broadcast == 0) ) {
				mError(devNum, EIO, "sendTxEndPacket5513: end packet not successfully sent, status = 0x%04X\n",
							status1);
				nRet = 0;
				return nRet;
			}
			break;
		}
		mSleep(1);
	}

	if (i == MDK_PKT_TIMEOUT) {
		mError(devNum, EIO, "sendTxEndPacket5513: timed out waiting for end packet done, QCU (0-15) = %d\n", 
							queueIndex);
		nRet = 0;
	}
	return nRet;
}
