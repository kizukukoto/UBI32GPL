/* devlibif.c - contians wrapper functions for devlib */

/* Copyright (c) 2000 Atheros Communications, Inc., All Rights Reserved */

#include <time.h>
#include <errno.h>
#ifndef VXWORKS
#include <malloc.h>
#endif

#include "wlantype.h"
#include "athreg.h"
#include "manlib.h"
#include "mConfig.h"

#include "dk_client.h"

#if defined(VXWORKS) 
#include "hw.h"
#include "hwext.h"
#else
#include "common_hw.h"
#endif

#include "perlarry.h"
#include "dk_cmds.h"

#include "dk_common.h"
#ifdef __ATH_DJGPPDOS__
#include <unistd.h>
#endif

#include "mEepStruct5416.h"

extern MDK_WLAN_DRV_INFO globDrvInfo; /* Global driver info */

static void printRxStats(RX_STATS_STRUCT *pRxStats);
static void printTxStats(TX_STATS_STRUCT *pTxStats);
A_UINT32 OScfgRead(A_UINT32 devNum,A_UINT32 regOffset);
void selectDevice (A_UINT32 devNum,A_UINT32 deviceType, DEVICE_MAP *devMap,MEM_SETUP *mem);


/******************************************************************************
 * Wrappers for setup function calls to manlib
 */

A_UINT32 m_eepromRead
(
 	A_UINT32 devNum, 
 	A_UINT32 eepromOffset
) 
{
	return eepromRead(devNum, eepromOffset);
}

void m_eepromWrite
(
 	A_UINT32 devNum, 
 	A_UINT32 eepromOffset, 
 	A_UINT32 eepromValue
)
{
	eepromWrite(devNum, eepromOffset, eepromValue);
}

PDWORDBUFFER m_eepromReadBlock
(
	A_UINT32 devNum,
	A_UINT32 startOffset,
	A_UINT32 length
) 
{
    	PDWORDBUFFER	pDwordBuffer = NULL;

	pDwordBuffer = (PDWORDBUFFER) A_MALLOC(sizeof(DWORDBUFFER));
    	if (!pDwordBuffer) {
        	return(NULL);
    	}

	pDwordBuffer->pDword = (A_UINT32 *)A_MALLOC(length * 4);
	if(!pDwordBuffer->pDword) {
		A_FREE(pDwordBuffer);
 		return(NULL);
 	}
	
	pDwordBuffer->Length = (A_UINT16)length * 4;

	eepromReadBlock(devNum, startOffset, length, pDwordBuffer->pDword);

    	/* 2 malloc's get free'd in typemap */
	return pDwordBuffer;
}


void m_eepromWriteBlock
(
 	A_UINT32 devNum, 
 	A_UINT32 startOffset, 
	PDWORDBUFFER pDwordBuffer
)
{
	eepromWriteBlock(devNum, startOffset, pDwordBuffer->Length/4,pDwordBuffer->pDword);
}


A_UINT32 m_resetDevice
(
 	A_UINT32 devNum, 
 	PDATABUFFER mac, 
 	PDATABUFFER bss, 
 	A_UINT32 freq, 
 	A_UINT32 turbo 
)
{
	MDK_WLAN_DEV_INFO *pdevInfo;
	PIPE_CMD pipeCmd;
	A_UINT16 devIndex;

	
	resetDevice(devNum, mac->pData, bss->pData, freq, turbo);
	
	devIndex = getDevIndex(devNum);
	
	if (devIndex != (A_UINT16)-1) {
		pdevInfo = globDrvInfo.pDevInfoArray[devIndex];

		if(!pdevInfo->pdkInfo->haveEvent) {
			pipeCmd.CMD_U.CREATE_EVENT_CMD.type = ISR_INTERRUPT;
			pipeCmd.CMD_U.CREATE_EVENT_CMD.persistent = 1;
			pipeCmd.CMD_U.CREATE_EVENT_CMD.param1 = 0;
			pipeCmd.CMD_U.CREATE_EVENT_CMD.param2 = 0;
			pipeCmd.CMD_U.CREATE_EVENT_CMD.param3 = 0;
			pipeCmd.CMD_U.CREATE_EVENT_CMD.eventHandle.eventID = DEVLIB_EVENT_ID;
			pipeCmd.CMD_U.CREATE_EVENT_CMD.eventHandle.f2Handle = (A_UINT16)devIndex;

			hwCreateEvent(devIndex, &pipeCmd);
			pdevInfo->pdkInfo->haveEvent = TRUE;
		} 
	}
	return 0;
}

void m_setResetParams
(
 	A_UINT32 devNum, 
 	A_CHAR fileName[256],
 	A_UINT32 eePromLoad,
	A_UINT32 forceCfgLoad,
	A_UINT32 mode,
	A_UINT16 initCodeFlag
)
{
	setResetParams(devNum, fileName,(A_BOOL)eePromLoad,(A_BOOL)forceCfgLoad,(A_UCHAR)mode, initCodeFlag);
}

void m_getDeviceInfo
(
 	A_UINT32 devNum, 
 	SUB_DEV_INFO *pDevStruct		
)
{
	getDeviceInfo(devNum, pDevStruct);
}

A_UINT32 m_checkRegs
(
 A_UINT32 devNum
)
{	
	printf("check regs called \n");
	return 0;
}

void m_changeChannel
(
 	A_UINT32 devNum,
 	A_UINT32 freq
)
{
    	changeChannel(devNum, freq);

	return;
}

A_UINT32 m_checkProm
(
 	A_UINT32 devNum,
 	A_UINT32 enablePrint
)
{
    	return checkProm(devNum, enablePrint);
}

void m_rereadProm
(
 	A_UINT32 devNum
)
{
    	rereadProm(devNum);
}

void m_txDataSetup
(
 	A_UINT32 devNum, 
 	A_UINT32 rateMask, 
 	PDATABUFFER dest, 
 	A_UINT32 numDescPerRate, 
 	A_UINT32 dataBodyLength, 
 	PDATABUFFER dataPattern, 
 	A_UINT32 retries, 
 	A_UINT32 antenna, 
 	A_UINT32 broadcast
)
{
	txDataSetup(devNum, rateMask, dest->pData, numDescPerRate, dataBodyLength, 
		dataPattern->pData, dataPattern->Length, retries, antenna, broadcast);

	return;
}

void m_txDataAggSetup
(
 	A_UINT32 devNum, 
 	A_UINT32 rateMask, 
 	PDATABUFFER dest, 
 	A_UINT32 numDescPerRate, 
 	A_UINT32 dataBodyLength, 
 	PDATABUFFER dataPattern, 
 	A_UINT32 retries, 
 	A_UINT32 antenna, 
 	A_UINT32 broadcast,
	A_UINT32 aggSize
)
{
    txDataAggSetup(devNum, rateMask, dest->pData, numDescPerRate, dataBodyLength, 
		dataPattern->pData, dataPattern->Length, retries, antenna, broadcast, aggSize);

	return;
}

void m_txDataBegin
(
 	A_UINT32 devNum,
 	A_UINT32 timeout,
 	A_UINT32 remoteStats
)
{
	txDataBegin(devNum, timeout, remoteStats);

	return;
}


void m_rxDataSetup
(
 	A_UINT32 devNum,
 	A_UINT32 numDesc,
 	A_UINT32 dataBodyLength,
 	A_UINT32 enablePPM
)
{
	rxDataSetup(devNum, numDesc, dataBodyLength, enablePPM);

	return;
}

void m_rxDataAggSetup
(
 	A_UINT32 devNum,
 	A_UINT32 numDesc,
 	A_UINT32 dataBodyLength,
 	A_UINT32 enablePPM,
	A_UINT32 aggSize
)
{
	rxDataAggSetup(devNum, numDesc, dataBodyLength, enablePPM, aggSize);
	return;
}


void m_txDataStart
(
	A_UINT32 devNum
)
{
	txDataStart(devNum);
	return;
}

void m_rxDataStart
(
	A_UINT32 devNum
)
{
	rxDataStart(devNum);
	return;
}

void m_rxDataBegin
(
 	A_UINT32 devNum,
 	A_UINT32 waitTime,
 	A_UINT32 timeout,
 	A_UINT32 remoteStats,
 	A_UINT32 enableCompare, 
 	PDATABUFFER dataPattern
)
{
	rxDataBegin(devNum, waitTime, timeout, remoteStats, enableCompare, dataPattern->pData, dataPattern->Length);

	return;
}

void m_txrxDataBegin
(
 	A_UINT32 devNum,
 	A_UINT32 waitTime,
 	A_UINT32 timeout,
 	A_UINT32 remoteStats,
 	A_UINT32 enableCompare, 
 	PDATABUFFER dataPattern
)
{
	txrxDataBegin(devNum, waitTime, timeout, remoteStats, enableCompare, dataPattern->pData, dataPattern->Length);

	return;
}

void m_iq_calibration(A_UINT32 devNum, IQ_FACTOR *iq_coeff)
{
	iq_calibration(devNum, iq_coeff);
	return;
}

void m_cleanupTxRxMemory
(
 	A_UINT32 devNum,
 	A_UINT32 flags
)
{
	cleanupTxRxMemory(devNum,flags);
	
	return;
}

PDWORDBUFFER m_txGetStats
(
 	A_UINT32 devNum, 
 	A_UINT32 rateInMb,
 	A_UINT32 remote
)
{
    	PDWORDBUFFER	pStruct = NULL;


	pStruct = (PDWORDBUFFER) A_MALLOC(sizeof(DWORDBUFFER));
    	if (!pStruct) {
        	return(NULL);
    	}

	pStruct->pDword = (A_UINT32 *)A_MALLOC(sizeof(TX_STATS_STRUCT));
	if(!pStruct->pDword) {
		A_FREE(pStruct);
		return(NULL);
	}
	
	pStruct->Length = sizeof(TX_STATS_STRUCT);
	txGetStats(devNum, rateInMb, remote, (TX_STATS_STRUCT *)pStruct->pDword);

	//check for errors
    	if (getMdkErrNo(devNum)) {
        	A_FREE(pStruct->pDword);
        	A_FREE(pStruct);
        	return(NULL);
    	}

    	/* 2 malloc's get free'd in typemap */
	return pStruct;
}

PDWORDBUFFER m_txPrintStats
(
 	A_UINT32 devNum, 
 	A_UINT32 rateInMb,
 	A_UINT32 remote
)
{
    	PDWORDBUFFER	pStruct = NULL;

	pStruct = m_txGetStats(devNum, rateInMb, remote);
	
	if (pStruct != NULL) printTxStats((TX_STATS_STRUCT *)pStruct->pDword);

    	/* 2 malloc's get free'd in typemap */
	return pStruct;
}

void printTxStats
(
 	TX_STATS_STRUCT *pTxStats
)
{
	uiPrintf("Good Packets           %10lu  ", pTxStats->goodPackets);
	uiPrintf("Ack sig strnth min     %10lu\n", pTxStats->ackSigStrengthMin);
	uiPrintf("Underruns              %10lu  ", pTxStats->underruns);
	uiPrintf("Ack sig strenth avg    %10lu\n", pTxStats->ackSigStrengthAvg);
	uiPrintf("Throughput             %10.2f  ",(float) pTxStats->throughput/1000);
	uiPrintf("Ack sig strenth max    %10lu\n", pTxStats->ackSigStrengthMax);
	uiPrintf("Excess retries         %10lu  ", pTxStats->excessiveRetries);
	uiPrintf("short retries 4        %10lu\n", pTxStats->shortRetry4); 
	uiPrintf("short retries 1        %10lu  ", pTxStats->shortRetry1); 
	uiPrintf("short retries 5        %10lu\n", pTxStats->shortRetry5); 
	uiPrintf("short retries 2        %10lu  ", pTxStats->shortRetry2); 
	uiPrintf("short retries 6-10     %10lu\n", pTxStats->shortRetry6to10); 
	uiPrintf("short retries 3        %10lu  ", pTxStats->shortRetry3); 
	uiPrintf("short retries 11-15    %10lu\n", pTxStats->shortRetry11to15); 
/*	uiPrintf("long  retries 1        %10lu  ", pTxStats->longRetry1); 
	uiPrintf("long  retries 2        %10lu\n", pTxStats->longRetry2); 
	uiPrintf("long  retries 3        %10lu  ", pTxStats->longRetry3); 
	uiPrintf("long  retries 4        %10lu\n", pTxStats->longRetry4); 
	uiPrintf("long  retries 5        %10lu  ", pTxStats->longRetry5); 
	uiPrintf("long  retries 6-10     %10lu\n", pTxStats->longRetry6to10); 
	uiPrintf("long  retries 11-15    %10lu\n", pTxStats->longRetry11to15);  */
}

PDWORDBUFFER m_rxGetStats
(
 	A_UINT32 devNum,
 	A_UINT32 rateInMb,
 	A_UINT32 remote
)
{

    	PDWORDBUFFER	pStruct = NULL;

	pStruct = (PDWORDBUFFER) A_MALLOC(sizeof(DWORDBUFFER));
    	if (!pStruct) {
        	return(NULL);
    	}

	pStruct->pDword = (A_UINT32 *)A_MALLOC(sizeof(RX_STATS_STRUCT));
	if(!pStruct->pDword) {
		A_FREE(pStruct);
		return(NULL);
	}
	pStruct->Length = sizeof(RX_STATS_STRUCT);
	rxGetStats(devNum, rateInMb, remote, (RX_STATS_STRUCT *)pStruct->pDword);

	//check for errors
//    	if (mdkErrno) {
//        	A_FREE(pStruct->pDword);
//        	A_FREE(pStruct);
//        	return(NULL);
//   	}

    	/* 2 malloc's get free'd in typemap */
	return pStruct;
}

PDWORDBUFFER m_rxPrintStats
(
 	A_UINT32 devNum,
 	A_UINT32 rateInMb,
 	A_UINT32 remote
)
{
    	PDWORDBUFFER	pStruct = NULL;

	pStruct = m_rxGetStats(devNum, rateInMb, remote);
	
	if (pStruct != NULL) printRxStats((RX_STATS_STRUCT *)pStruct->pDword);

    	/* 2 malloc's get free'd in typemap */
	return pStruct;
}

void printRxStats
(
 	RX_STATS_STRUCT *pRxStats
)
{
	uiPrintf("Good Packets           %10lu  ", pRxStats->goodPackets);
	uiPrintf("Data sig strenth min   %10lu\n", pRxStats->DataSigStrengthMin);
	uiPrintf("CRC-Failure Packets    %10lu  ", pRxStats->crcPackets);
	uiPrintf("Data sig strenth avg   %10lu\n", pRxStats->DataSigStrengthAvg);
	uiPrintf("Bad CRC Miscomps       %10lu  ", pRxStats->bitMiscompares);
	uiPrintf("Data sig strenth max   %10lu\n", pRxStats->DataSigStrengthMax);
	uiPrintf("Good Packet Miscomps   %10lu  ", pRxStats->bitErrorCompares);
	uiPrintf("PPM min                %10ld\n", pRxStats->ppmMin);
	uiPrintf("Single dups            %10lu  ", pRxStats->singleDups);
	uiPrintf("PPM avg                %10ld\n", pRxStats->ppmAvg);
	uiPrintf("Multiple dups          %10lu  ", pRxStats->multipleDups);
	uiPrintf("PPM max                %10ld\n", pRxStats->ppmMax);
#ifdef MAUI
	uiPrintf("Decrypt errors         %10lu\n", pRxStats->decrypErrors);
#endif
	return;
}

// Display RX Data
PDATABUFFER m_rxGetData
(
 	A_UINT32 devNum,
 	A_UINT32 bufferNum,
 	A_UINT16 sizeBuffer
)
{
    	PDATABUFFER	pStruct = NULL;


	pStruct = (PDATABUFFER) A_MALLOC(sizeof(DATABUFFER));
	if (!pStruct) {
		return(NULL);
	}

	pStruct->pData = (A_UCHAR *)A_MALLOC(sizeBuffer);
	if(!pStruct->pData) {
		A_FREE(pStruct);
		return(NULL);
	}
	pStruct->Length = sizeBuffer;
	rxGetData(devNum, bufferNum, pStruct->pData, sizeBuffer);

	//check for errors
    	if (getMdkErrNo(devNum)) {
        	A_FREE(pStruct->pData);
        	A_FREE(pStruct);
        	return(NULL);
    	}

	return pStruct;
}

// Continuous Transmit Functions
void m_txContBegin
(
 	A_UINT32 devNum, 
 	A_UINT32 type, 
 	A_UINT32 typeOption1,
 	A_UINT32 typeOption2, 
 	A_UINT32 antenna
)
{
	txContBegin(devNum, type, typeOption1, typeOption2, antenna);

	return;

}

void m_txContFrameBegin
(
 	A_UINT32 devNum, 
	A_UINT32 length,
	A_UINT32 ifswait,
 	A_UINT32 typeOption1,
 	A_UINT32 typeOption2, 
 	A_UINT32 antenna,
	A_BOOL   performStabilizePower,
	A_UINT32 numDescriptors,
	A_UCHAR *dest
)
{
	txContFrameBegin(devNum, length, ifswait, typeOption1, typeOption2, antenna, performStabilizePower, numDescriptors, dest);

	return;

}

void m_txContEnd
(
 	A_UINT32 devNum
)
{
    	txContEnd(devNum);

	return;
}

void m_setAntenna
(
 	A_UINT32 devNum, 
 	A_UINT32 antenna
)
{
    	setAntenna(devNum, antenna);

	return;
}

void m_setPowerScale
(
 	A_UINT32 devNum, 
 	A_UINT32 powerScale
)
{
    	setPowerScale(devNum, powerScale);

	return;
}

void m_setTransmitPower
(
 	A_UINT32 devNum, 
 	PDATABUFFER txPowerArray
)
{
	if (txPowerArray->Length != 17) {
		uiPrintf("Error in m_setTransmitPower: Given txPowerArray is not 17 bytes in size: %d\n", txPowerArray->Length);
		return;
	}
	setTransmitPower(devNum, txPowerArray->pData);

	return;
}

void m_setSingleTransmitPower
(
 	A_UINT32 devNum, 
 	A_UCHAR pcdac 
)
{
	setSingleTransmitPower(devNum, pcdac);

	return;
}

void m_specifySubSystemID
(
 	A_UINT32 devNum, 
 	A_UINT16 subsystemID 
)
{
	specifySubSystemID(devNum, subsystemID);

	return;
}

void m_devSleep
(
 	A_UINT32 devNum
)
{
	devSleep(devNum);

	return;
}

void m_getEepromStruct
(
 	A_UINT32 devNum,
 	A_UINT16 eepStructFlag,
	void **ppReturnStruct,
	A_UINT32 *pSizeStruct
)
{
	getEepromStruct(devNum, eepStructFlag, ppReturnStruct, pSizeStruct);

	return;
}

void m_changeField
(
 	A_UINT32 devNum,
 	A_CHAR *fieldName, 
 	A_UINT32 newValue
)
{
	changeField(devNum, fieldName, newValue);
   
    	return;
}


void m_changeMultipleFieldsAllModes
(
 	A_UINT32		devNum,
 	PARSE_MODE_INFO *fieldsToChange, 
 	A_UINT32 numFields
)
{
	changeMultipleFieldsAllModes(devNum, fieldsToChange, numFields);
   
    return;
}

void m_changeMultipleFields
(
 	A_UINT32		devNum,
 	PARSE_FIELD_INFO *fieldsToChange, 
 	A_UINT32 numFields
)
{
	changeMultipleFields(devNum, fieldsToChange, numFields);
   
    return;
}

/***************************************************************************
*  Function name: m_changeAddacField
*  Input: Device num and field (name and value)
*  Description: Changes an addac field
***************************************************************************/
void m_changeAddacField
(
    A_UINT32		  devNum,
    PARSE_FIELD_INFO *pFieldToChange   
)
{
		changeAddacField(devNum, pFieldToChange);
}

/***************************************************************************
*  Function name: m_saveXpaBiasLvlFreq
*  Input: Device num and field (name and value) and bias level
*  Description: Saves xpa bias level vs freq array
***************************************************************************/
void m_saveXpaBiasLvlFreq
(
    A_UINT32		  devNum,
    PARSE_FIELD_INFO *pFieldToChange,
    A_CHAR *level
)
{
    A_UINT16 biasLevel = (A_UINT16) atoi(level);
	saveXpaBiasLvlFreq(devNum, pFieldToChange, biasLevel);
}

void m_enableWep
(
 	A_UINT32 devNum,
 	A_UCHAR  key
)
{
	enableWep(devNum, key);
   
    	return;
}

void m_enablePAPreDist
(
 	A_UINT32 devNum,
 	A_UINT16 rate,
 	A_UINT32 power
)
{
	//enablePAPreDist(devNum, rate, power);
   
    	return;

}

void m_dumpRegs
(
 	A_UINT32 devNum
)
{
	dumpPciRegValues(devNum);

	return;
}

void m_dumpPciWrites
(
 	A_UINT32 devNum
)
{
	displayPciRegWrites(devNum);

	return;
}

A_BOOL m_testLib
(
 	A_UINT32 devNum,
 	A_UINT32 timeout
)
{
	A_BOOL ret;

	ret = testLib(devNum, timeout);

	return ret;
}

void m_displayFieldValues 
(
 	A_UINT32 devNum,
 	A_CHAR *fieldName,
	A_UINT32 *baseValue,
	A_UINT32 *turboValue

)
{
	getField(devNum, fieldName, baseValue, turboValue);

	return;
}

A_UINT32 m_getFieldValue
(
 	A_UINT32 devNum,
 	A_CHAR   *fieldName,
 	A_UINT32 turbo,
	A_UINT32 *baseValue,
	A_UINT32 *turboValue

)
{
	getField(devNum, fieldName, baseValue, turboValue);

	return 0;
}

A_INT32 m_getFieldForMode
(
 	A_UINT32 devNum,
 	A_CHAR   *fieldName,
 	A_UINT32 mode,
	A_UINT32 turbo

)
{
	return getFieldForMode(devNum, fieldName, mode, turbo);
}

A_UINT32 m_readField
(
 	A_UINT32 devNum,
 	A_CHAR   *fieldName,
 	A_UINT32 printValue,
	A_UINT32 *unsignedValue,
	A_INT32 *signedValue,
	A_BOOL  *signedFlag

)
{
	readField(devNum, fieldName, unsignedValue, signedValue, signedFlag);

	return 0;
}

void m_writeField
(
 	A_UINT32 devNum,
 	A_CHAR *fieldName, 
 	A_UINT32 newValue
)
{
	writeField(devNum, fieldName, newValue);

    	return;
}

void m_forceSinglePCDACTable
(
	A_UINT32 devNum,
	A_UINT16 pcdac
)
{
	ForceSinglePCDACTable(devNum,pcdac);

	return;
}

void m_forcePCDACTable
(
	A_UINT32 devNum,
	A_UINT16 *pcdac
)
{
	forcePCDACTable(devNum,pcdac);

	return;
}

void m_forcePowerTxMax
(
	A_UINT32 devNum,
	A_UINT32 length,
	A_UINT16 *pRatesPower
)
{
    if (length == 0) {
		pRatesPower = NULL;
	}
	else if (length != Ar5416RateSize) {
        uiPrintf("m_forcePowerTxMax: RatesPower should be a 36 byte array. given %d\n", length);
        return;
    }

	forcePowerTxMax(devNum, (A_INT16 *) pRatesPower);

	return;
}

void m_forceSinglePowerTxMax
(
	A_UINT32 devNum,
	A_UINT16 txPower
)
{
	forceSinglePowerTxMax(devNum,txPower);
	return;
}

void m_setQueue
(
 	A_UINT32 devNum,
	A_UINT32 qcuNumber
)
{
	setQueue(devNum,qcuNumber);

    	return;
}

void m_mapQueue
(
 	A_UINT32 devNum,
	A_UINT32 qcuNumber,
	A_UINT32 dcuNumber
)
{
	mapQueue(devNum,qcuNumber,dcuNumber);

    	return;
}

void m_clearKeyCache
(
 	A_UINT32 devNum
)
{
	clearKeyCache(devNum);

    	return;
}

void m_getMacAddr
(
 	A_UINT32 devNum,
	A_UINT16 wmac,
	A_UINT16 instNo,
	A_UINT8 *macAddr
)
{
	getMacAddr(devNum,wmac,instNo, macAddr);

   	return;
}


A_UINT16 m_closeDevice
(
    A_UINT32 devNum
)
{
	A_UINT16 devIndex;

	devIndex = getDevIndex(devNum);
	// Close manufacturing library structs for this device
	closeDevice(devNum);

    // Close low level structs for this device
	if (devIndex != (A_UINT16)-1) {
  		deviceCleanup(devIndex);
	}
	return (devIndex);
}

#ifndef __ATH_DJGPPDOS__
/**************************************************************************
* OSregRead - MLIB command for reading a register
*
* RETURNS: value read
*/
A_UINT32 OSregRead
(
    	A_UINT32 devNum,
    	A_UINT32 regOffset
)
{
	A_UINT32         regReturn;
	A_UINT16 devIndex;

	// no need to check for invalid devindex as these are called from lib and should 
	// have a valid devindex
	devIndex = getDevIndex(devNum);

	/* read the register */
	regReturn = hwMemRead32(devIndex,regOffset); 

	return(regReturn);
}

/**************************************************************************
* OSregWrite - MLIB command for writing a register
*
*/
void OSregWrite
(
	A_UINT32 devNum,
    A_UINT32 regOffset,
    A_UINT32 regValue
)
{
	A_UINT16 devIndex;
    MDK_WLAN_DEV_INFO    *pdevInfo;

	// no need to check for invalid devindex as these are called from lib and should 
	// have a valid devindex
	devIndex = getDevIndex(devNum);
	pdevInfo = globDrvInfo.pDevInfoArray[devIndex];

	/* write the register */
	hwMemWrite32(devIndex,regOffset, regValue); 

#ifdef OWL_AP
	// This is a workaround for Merlin analog reg write bug
	if(isMerlin(gLibInfo.pLibDevArray[devNum]->swDevID)) {
	    if(((regOffset & 0xffff) >= 0x7800) && ((regOffset & 0xffff) <= 0x7898)) {
    	        mSleep(1); 
	    }
	}
#endif

	if(pdevInfo->pdkInfo->printPciWrites) 
	{
		uiPrintf("0x%04x 0x%08x\n", regOffset & 0xffff, regValue);
	} 
}

/**************************************************************************
* OScfgRead - MLIB command for reading a pci configuration register
*
* RETURNS: value read
*/
A_UINT32 OScfgRead
(
	A_UINT32 devNum,
    A_UINT32 regOffset
)
{
    A_UINT32         regReturn;
	A_UINT16 devIndex;

	// no need to check for invalid devindex as these are called from lib and should 
	// have a valid devindex
	devIndex = getDevIndex(devNum);

	regReturn = hwCfgRead32(devIndex, regOffset); 

    return(regReturn);
}

/**************************************************************************
* OScfgWrite - MLIB command for writing a pci config register
*
*/
void OScfgWrite
(
	A_UINT32 devNum,
   	A_UINT32 regOffset,
   	A_UINT32 regValue
)
{
	A_UINT16 devIndex;

	// no need to check for invalid devindex as these are called from lib and should 
	// have a valid devindex
	devIndex = getDevIndex(devNum);

	hwCfgWrite32(devIndex, regOffset, regValue); 
}

/**************************************************************************
* OSmemRead - MLIB command to read a block of memory
*
* read a block of memory
*
* RETURNS: An array containing the bytes read
*/
void OSmemRead
(
	A_UINT32 devNum,
   	A_UINT32 physAddr, 
	A_UCHAR  *bytesRead,
   	A_UINT32 length
)
{
	A_UINT16 devIndex;

	// no need to check for invalid devindex as these are called from lib and should 
	// have a valid devindex
	devIndex = getDevIndex(devNum);
	
   	/* check to see if the size will make us bigger than the send buffer */
   	if (length > MAX_MEMREAD_BYTES) {
       		uiPrintf("Error: OSmemRead length too large, can only read %x bytes\n", MAX_MEMREAD_BYTES);
		return;
   	}

	if (bytesRead == NULL) {
       		uiPrintf("Error: OSmemRead received a NULL ptr to the bytes to read - please preallocate\n");
		return;
    	}

	if(hwMemReadBlock(devIndex, bytesRead, physAddr, length) == -1) {
		uiPrintf("Error: OSmemRead failed call to hwMemReadBlock()\n");
		return;
	}
}

/**************************************************************************
* OSmemWrite - MLIB command to write a block of memory
*
*/
void OSmemWrite
(
	A_UINT32 devNum,
    	A_UINT32 physAddr,
	A_UCHAR  *bytesWrite,
	A_UINT32 length
)
{
	A_UINT16 devIndex;

	// no need to check for invalid devindex as these are called from lib and should 
	// have a valid devindex
	devIndex = getDevIndex(devNum);
	
   	/* check to see if the size will make us bigger than the send buffer */
   	if (length > MAX_MEMREAD_BYTES) {
        uiPrintf("Error: OSmemWrite length too large, can only read %x bytes\n", MAX_MEMREAD_BYTES);
		return;
   	}
	if (bytesWrite == NULL) {
        uiPrintf("Error: OSmemWrite received a NULL ptr to the bytes to write\n");
		return;	
	}

	if(hwMemWriteBlock(devIndex, bytesWrite, length, &(physAddr)) == -1) {
		uiPrintf("Error:  OSmemWrite failed call to hwMemWriteBlock()\n");
		return;
    }
}

ISR_EVENT getISREvent
(
 	A_UINT32 devNum
)
{
    MDK_WLAN_DEV_INFO    *pdevInfo;
	static EVENT_STRUCT locEventSpace;
	// Initialize event to invalid.
	ISR_EVENT event = {0, 0};
	A_UINT16 devIndex;

	// no need to check for invalid devindex as these are called from lib and should 
	// have a valid devindex
	devIndex = getDevIndex(devNum);
	
	pdevInfo = globDrvInfo.pDevInfoArray[devIndex];

#ifdef JUNGO
    	if (pdevInfo->pdkInfo->pSharedInfo->anyEvents) {
		//call into the kernel plugin to get the event
		if (hwGetNextEvent(devIndex,(void *)&locEventSpace) != -1) {
		if (locEventSpace.type == ISR_INTERRUPT) {
				event.valid = 1;
				event.ISRValue = locEventSpace.result;
		#ifdef MAUI
		{
			A_UINT32 i;

			for (i=0;i<5;i++) {
				event.additionalInfo[i] = locEventSpace.additionalParams[i]; 
			}
		}
		#endif
			}
			else {
				uiPrintf("Error: getISREvent - found a non-ISR event in a client - is this possible???\n");
				uiPrintf("If this has become a possibility... then remove this error check\n");
			}
		}
		else {
			uiPrintf("Error: getISREvent Unable to get event from kernel plugin when it said we had one\n");
		}
	}

#else
	locEventSpace.type = 0;
	if (hwGetNextEvent(devIndex,&locEventSpace) != -1) {
		if(locEventSpace.type == ISR_INTERRUPT) {
			event.valid = 1;
			event.ISRValue = locEventSpace.result;
		}	
		#ifdef MAUI
		{
			A_UINT32 i;
				for (i=0;i<5;i++) {
				event.additionalInfo[i] = locEventSpace.additionalParams[i]; 
			}
		}
		#endif
	}
#endif
	return(event);
}


/**************************************************************************
* initializeMLIB - initialization call to MLIB
*
* RETURNS: value read
*/
A_INT32 initializeMLIB(MDK_WLAN_DEV_INFO *pdevInfo)
{
	static DEVICE_MAP devMap;
	A_UINT32 devNum;

	devMap.DEV_CFG_ADDRESS = 0;
	devMap.DEV_CFG_RANGE = MAX_CFG_OFFSET;
#ifdef JUNGO
	devMap.DEV_MEMORY_ADDRESS = (unsigned long) pdevInfo->pdkInfo->dma_mem_addr;	
	devMap.DEV_REG_ADDRESS = pdevInfo->pdkInfo->pSharedInfo->devMapAddress;
#else
	devMap.DEV_MEMORY_ADDRESS = (unsigned long) pdevInfo->pdkInfo->memPhyAddr;
	devMap.DEV_REG_ADDRESS = pdevInfo->pdkInfo->f2MapAddress;
#endif
	devMap.DEV_MEMORY_RANGE = pdevInfo->pdkInfo->memSize;
	devMap.DEV_REG_RANGE = LIB_REG_RANGE;
	devMap.OScfgRead = OScfgRead;
	devMap.OScfgWrite = OScfgWrite;
	devMap.OSmemRead = OSmemRead;
	devMap.OSmemWrite = OSmemWrite;
	devMap.OSregRead = OSregRead;
	devMap.OSregWrite = OSregWrite;
	devMap.getISREvent = getISREvent;
    devMap.remoteLib = 0;
#ifdef SOC_LINUX
	devMap.OSapRegRead32 = apRegRead32;
	devMap.OSapRegWrite32 = apRegWrite32;
#endif

	devMap.devIndex = (A_UINT16)pdevInfo->pdkInfo->devIndex;
	devNum = initializeDevice(devMap);

	// Setup quick access device to handle table
	assert(devNum < LIB_MAX_DEV);

    	// Point library at MDKclient memory map
    	useMDKMemory(devNum, pdevInfo->pbuffMapBytes, pdevInfo->pnumBuffs);
	//devMap.devIndex = pdevInfo->pdkInfo->devIndex;

	return devNum;
}
#endif

void closeMLIB(A_UINT32 devNum)
{
	closeDevice(devNum);
}

void m_devlibCleanup()
{
	devlibCleanup();
}

void m_changeCal
(
  A_UINT32 calFlags
)
{
	enableCal = calFlags;
	return;
}

/**************************************************************************
* setPciWritesFlag - Change the value to flag for printing pci writes data
*
* RETURNS: N/A
*/
void changePciWritesFlag
(
	A_UINT32 devNum,
	A_UINT32 flag
)
{
    MDK_WLAN_DEV_INFO    *pdevInfo;
	A_UINT16 devIndex;

	devIndex = (A_UINT16)getDevIndex(devNum);
	pdevInfo = globDrvInfo.pDevInfoArray[devIndex];

	pdevInfo->pdkInfo->printPciWrites = (A_BOOL)flag;
	return;
}

