/* mData.c - contians frame transmit functions */
/* Copyright (c) 2001 Atheros Communications, Inc., All Rights Reserved */

#ident  "ACI $Id: //depot/sw/src/dk/mdk/devlib/mData.c#157 $, $Header: //depot/sw/src/dk/mdk/devlib/mData.c#157 $"

/*
Revsision history
--------------------
1.0       Created.
*/

//#define DEBUG_MEMORY

#ifdef VXWORKS
#include "timers.h"
#include "vxdrv.h"
#endif

#ifdef __ATH_DJGPPDOS__
#include <unistd.h>
#ifndef EILSEQ
    #define EILSEQ EIO
#endif  // EILSEQ

#define __int64 long long
typedef unsigned long DWORD;
#define HANDLE long
#define Sleep   delay
#endif  // #ifdef __ATH_DJGPPDOS__

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include "wlantype.h"
#include "athreg.h"
#include "manlib.h"
#include "mdata.h"
#include "stats_routines.h"
#include "mEeprom.h"
#include "mConfig.h"
#ifndef VXWORKS
#include <malloc.h>
#ifdef LINUX
#include "linuxdrv.h"
#else
#include "ntdrv.h"
#endif
#endif // VXWorks
#include "mData210.h"
#include "mData211.h"
#include "mData212.h"
#ifndef __ATH_DJGPPDOS__
#include "mCfg513.h"
#include "mData513.h"
#endif
#include "mData5416.h"
#include "mDevtbl.h"
#include "mIds.h"

//#ifdef LINUX
//#undef ARCH_BIG_ENDIAN
//#endif

#ifdef ARCH_BIG_ENDIAN
#include "endian_func.h"
#endif

#if defined(COBRA_AP) && defined(PCI_INTERFACE)
#include "ar531xPlusreg.h"
#endif

static void mdkExtractAddrAndSequence(A_UINT32 devNum, RX_STATS_TEMP_INFO *pStatsInfo);
static void mdkCountDuplicatePackets(A_UINT32   devNum,
                RX_STATS_TEMP_INFO  *pStatsInfo, A_BOOL *pIsDuplicate);
static void mdkGetSignalStrengthStats(SIG_STRENGTH_STATS *pStats, A_INT8    signalStrength);
static void txAccumulateStats(A_UINT32 devNum, A_UINT32 txTime, A_UINT16 queueIndex);
static void mdkExtractRxStats(A_UINT32 devNum, RX_STATS_TEMP_INFO   *pStatsInfo);
static void sendStatsPkt(A_UINT32 devNum, A_UINT32 rate, A_UINT16 StatsType, A_UCHAR *dest);
static A_BOOL mdkExtractRemoteStats(A_UINT32 devNum, RX_STATS_TEMP_INFO *pStatsInfo);
static void comparePktData(A_UINT32 devNum, RX_STATS_TEMP_INFO  *pStatsInfo);
static void extractPPM(A_UINT32 devNum, RX_STATS_TEMP_INFO *pStatsInfo);
static A_UINT32 countBits(A_UINT32 mismatchBits);
static void fillCompareBuffer(A_UCHAR *pBuffer, A_UINT32 compareBufferSize,
                A_UCHAR *pDataPattern, A_UINT32 dataPatternLength);
static void fillRxDescAndFrame(A_UINT32 devNum, RX_STATS_TEMP_INFO *statsInfo);

#if defined(__ATH_DJGPPDOS__)
static A_UINT32 milliTime(void);
#endif

////////////////////// __TODO__   ////////////////////////////////////////////////////////////////////////////////
MANLIB_API void txDataStart(A_UINT32 devNum);
MANLIB_API void txDataComplete(A_UINT32 devNum, A_UINT32 timeout, A_UINT32 remoteStats);

A_UINT32 buf_ptr;
A_UCHAR *tmp_pktDataPtr;

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static char bitCount[256] = {
//  0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f
    0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4,     // 0X
    1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,     // 1X
    1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,     // 2X
    2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,     // 3X
    1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,     // 4X
    2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,     // 5X
    2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,     // 6X
    3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,     // 7X
    1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,     // 8X
    2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,     // 9X
    2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,     // aX
    3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,     // bX
    2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,     // cX
    3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,     // dX
    3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,     // eX
    4, 5, 5, 6, 5, 6, 6, 7, 5, 6, 6, 7, 6, 7, 7, 8      // fX
};

// A quick lookup translating from rate 6, 9, 12... to the stats_struct array index
//static const A_UCHAR StatsRateArray[19] =
//  {0, 0, 1, 2, 3, 0, 4, 0, 5, 0,  0,  0,  6,  0,  0,  0,  7,  0,  8};
//#define rate2bin(x) (StatsRateArray[((x)/3)])

// A quick lookup translating from IEEE rate field to the stats bin
//static const A_UCHAR IEEErateArray[8] = {7, 5, 3, 1, 8, 6, 4, 2};
//#define descRate2bin(x) (IEEErateArray[(x)-8])

/**************************************************************************
* txDataSetupNoEndPacket - create packet and descriptors for transmission
*                          with no end packet. added to be used for falcon
*                          11g synthesizer phase offset.
*/
MANLIB_API void txDataSetupNoEndPacket
(
 A_UINT32 devNum,
 A_UINT32 rateMask,
 A_UCHAR *dest,
 A_UINT32 numDescPerRate,
 A_UINT32 dataBodyLength,
 A_UCHAR *dataPattern,
 A_UINT32 dataPatternLength,
 A_UINT32 retries,
 A_UINT32 antenna,
 A_UINT32 broadcast
)
{
    LIB_DEV_INFO *pLibDev = gLibInfo.pLibDevArray[devNum];

    pLibDev->noEndPacket = TRUE;

    txDataSetup(devNum, rateMask, dest, numDescPerRate, dataBodyLength,
                dataPattern, dataPatternLength, retries, antenna, broadcast);
}

/**************************************************************************
* txDataAggSetup - create packet and descriptors for transmission
*
*/
MANLIB_API void txDataAggSetup(
 A_UINT32 devNum,
 A_UINT32 rateMask,
 A_UCHAR *dest,
 A_UINT32 numDescPerRate,
 A_UINT32 dataBodyLength,
 A_UCHAR *dataPattern,
 A_UINT32 dataPatternLength,
 A_UINT32 retries,
 A_UINT32 antenna,
 A_UINT32 broadcast,
 A_UINT32 aggSize)
{
    LIB_DEV_INFO* pLibDev = gLibInfo.pLibDevArray[devNum];
    A_UCHAR broadcastAddr[] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
    MDK_PACKET_HEADER* pMdkHeader;
    WLAN_QOS_DATA_MAC_HEADER3 *pPktHeader;
    WLAN_BAR_CONTROL_MAC_HEADER *pBarHeader;
    MDK_5416_2_DESC *localDescBuffer, *localDescPtr, *localDescAggBlockPtr, *barLocalDesc, *barLocalDescPtr;
    A_UINT16 mdkPktType = MDK_NORMAL_PKT;
    A_UINT16 numRates;
    A_UINT16 queueIndex;
    A_UINT16 numAttempts;
	A_UINT32 aggFrameNum;
	A_UINT32 barDescAddr;
	A_UINT32 barBufAddr;
	A_UINT32 txBufAddr;
    A_UINT32 descAddress;
    A_UINT32 i, j;
    A_UINT32 totalAggLen, frameLen, pkt_pad, txControl1, txControl2, txBufLen;
    A_UINT32 amountToWrite;
    A_UINT32 antMode = 0;
    A_UINT32 ht40, short_gi, dup, ext_only;
    A_UINT32 packetSize;
    A_UINT32 triggerLvl, bitMask;
    A_UCHAR  r, rateIdxs[numRateCodes], rateIndex;
    A_UCHAR* pTransmitPkt;
    A_UCHAR* pPkt;
	A_UCHAR* pBarLocalBuffer;
    A_BOOL   probePkt = FALSE;

	pLibDev->yesAgg = TRUE;
	assert(pLibDev->aggSize <= 64); // aggSize cannot be more than 64
	pLibDev->aggSize = aggSize;

	pLibDev->mdkPacketType = MDK_NORMAL_PKT;
// printf("Descriptor per rate: %d\n", numDescPerRate);
// printf("Rate mask: %d\n", rateMask);
    //overloading the broadcast param with additional flags
    if(broadcast & PROBE_PKT) {
        probePkt = TRUE;
        mdkPktType = MDK_PROBE_PKT;

        //setup the probe packets on a different queue
        pLibDev->txProbePacketNext = TRUE;
        queueIndex = PROBE_QUEUE;
        if(pLibDev->backupSelectQueueIndex != PROBE_QUEUE) { //safety check so we don't backup probe_queue index
            pLibDev->backupSelectQueueIndex = pLibDev->selQueueIndex;
        }
        pLibDev->selQueueIndex = PROBE_QUEUE;
    } else {
        pLibDev->selQueueIndex = 0;
        queueIndex = pLibDev->selQueueIndex;
    }

    //cleanup broadcast flag after cleaning up overloaded flags
    broadcast = broadcast & 0x1;

#ifdef DEBUG_MEMORY
printf("SNOOP::txDataSetup::broadcast=%x\n", broadcast);
printf("SNOOP::dest %x:%x:%x:%x:%x:%x\n", dest[6], dest[5], dest[4], dest[3], dest[2], dest[1], dest[0]);
#endif

    pLibDev->tx[queueIndex].dcuIndex = 0;


    pLibDev->tx[queueIndex].retryValue = retries;
    //verify some of the arguments
    if (checkDevNum(devNum) == FALSE) {
        mError(devNum, EINVAL, "Device Number %d:txDataSetup\n", devNum);
        return;
    }

    if(pLibDev->devState < RESET_STATE) {
        mError(devNum, EILSEQ, "Device Number %d:txDataSetup: device not in reset state - resetDevice must be run first\n", devNum);
        return;
    }

    if (0 == numDescPerRate) {
        mError(devNum, EINVAL, "Device Number %d:txDataSetup: must create at least 1 descriptor per rate\n", devNum);
        return;
    }

    /* we must take the MDK pkt header into consideration here otherwise it could make the
     * body size greater than an 802.11 pkt body
     */
    if(dataBodyLength > (MAX_PKT_BODY_SIZE - sizeof(MDK_PACKET_HEADER) - sizeof(WLAN_DATA_MAC_HEADER3) - FCS_FIELD)) {
        mError(devNum, EINVAL, "Device Number %d:txDataSetup: packet body size must be less than %d [%d - (%d + %d + %d)]\n", devNum,
                                (MAX_PKT_BODY_SIZE - sizeof(MDK_PACKET_HEADER) - sizeof(WLAN_DATA_MAC_HEADER3) - FCS_FIELD),
                                MAX_PKT_BODY_SIZE,
                                sizeof(MDK_PACKET_HEADER),
                                sizeof(WLAN_DATA_MAC_HEADER3),
                                FCS_FIELD);
        return;
    }

    //take a copy of the dest address
    memcpy(pLibDev->tx[queueIndex].destAddr.octets, dest, WLAN_MAC_ADDR_SIZE);
    pLibDev->tx[queueIndex].dataBodyLen = dataBodyLength;

    if (!ar5kInitData[pLibDev->ar5kInitIndex].pMacAPI->setupAntenna(devNum, antenna, &antMode)) {
        return;
    }

    //cleanup any stuff from previous iteration
    if (pLibDev->tx[queueIndex].pktAddress) {
        memFree(devNum, pLibDev->tx[queueIndex].pktAddress);
        pLibDev->tx[queueIndex].pktAddress = 0;
        memFree(devNum, pLibDev->tx[queueIndex].descAddress);
        pLibDev->tx[queueIndex].descAddress = 0;

    }
    if(pLibDev->tx[queueIndex].barDescAddress) {
        memFree(devNum, pLibDev->tx[queueIndex].barDescAddress);
        pLibDev->tx[queueIndex].barDescAddress = 0;
        memFree(devNum, pLibDev->tx[queueIndex].barPktAddress);
        pLibDev->tx[queueIndex].barPktAddress = 0;
        pLibDev->tx[queueIndex].txEnable = 0;
    }
    if(pLibDev->tx[queueIndex].endPktAddr) {
        memFree(devNum, pLibDev->tx[queueIndex].endPktAddr);
        memFree(devNum, pLibDev->tx[queueIndex].endPktDesc);
        pLibDev->tx[queueIndex].endPktAddr = 0;
        pLibDev->tx[queueIndex].endPktDesc = 0;
    }

    //create rate array and findout how many rates exist
    for (r = 0, numRates = 0; r < 32; r++) {
        if (rateMask & (1 << r)) {
            rateIdxs[numRates++] = r;
        }
    }

    for (; r < 48; r++) {
        if (pLibDev->libCfgParams.rateMaskMcs20 & (1 << (r - 32))) {
            rateIdxs[numRates++] = r;
        }
    }
    for (; r < 64; r++) {
        if (pLibDev->libCfgParams.rateMaskMcs40 & (1 << (r - 48))) {
            rateIdxs[numRates++] = r;
        }
    }

    if (!numRates) {
        mError(devNum, EINVAL, "txDataSetup: No rates specified in rateMask(s)\n");
        return;
    }

    //create the required number of descriptors
    pLibDev->tx[queueIndex].numDesc = numDescPerRate * numRates;

 	aggFrameNum = numDescPerRate / aggSize;
//	printf("TEST: Total agg frames %d\n", aggFrameNum);

	// Change frame trigger level -- this will increase fifo depth before start of transmit

	triggerLvl = REGR(devNum, 0x30);
	bitMask = 0x3f<<4;
	bitMask = ~bitMask;
	triggerLvl = (triggerLvl & bitMask);
	triggerLvl = triggerLvl | 0x210;  // reigger level 33
	REGW(devNum, 0x30, triggerLvl);
	triggerLvl = 0x1234;
	triggerLvl = REGR(devNum, 0x30);

   //Allocate memory for required number of descriptors
    pLibDev->tx[queueIndex].numDesc = numDescPerRate;
    pLibDev->tx[queueIndex].descAddress =     memAlloc( devNum, pLibDev->tx[queueIndex].numDesc * sizeof(MDK_5416_2_DESC));
    if (0 == pLibDev->tx[queueIndex].descAddress) {
        mError(devNum, ENOMEM, "Device Number %d:txDataSetup1: unable to allocate client memory for %d descriptors\n", devNum, pLibDev->tx[queueIndex].numDesc);
        return;
    }

    // Allocate memory for number of agg frame descriptor
    barDescAddr =     memAlloc( devNum, aggFrameNum * sizeof(MDK_5416_2_DESC));
    if (0 == barDescAddr) {
        mError(devNum, ENOMEM, "Device Number %d:txDataSetup2: unable to allocate client memory for %d descriptors\n", devNum, aggFrameNum);
        return;
    }
	pLibDev->tx[queueIndex].barDescAddress = barDescAddr;

	// Allocate memory for tx data
	packetSize = dataBodyLength + sizeof(WLAN_QOS_DATA_MAC_HEADER3) + 2 + sizeof(MDK_PACKET_HEADER);
	if(packetSize%4 != 0)
		packetSize += (4-packetSize%4);
	txBufAddr = memAlloc(devNum, packetSize*aggSize);
    if (0 == txBufAddr) {
        mError(devNum, ENOMEM, "Device Number %d:txDataSetup3: unable to allocate client memory for %d descriptors\n", devNum, 0x10000);
        return;
    }
	pLibDev->tx[queueIndex].pktAddress = txBufAddr;

    // Allocate memory for BAR data
	barBufAddr = memAlloc(devNum, 24);
    if (0 == barBufAddr) {
        mError(devNum, ENOMEM, "Device Number %d:txDataSetup4: unable to allocate client memory for %d descriptors\n", devNum, 24);
        return;
    }
	pLibDev->tx[queueIndex].barPktAddress = barBufAddr;

    descAddress = pLibDev->tx[queueIndex].descAddress;
	numAttempts = (A_UINT16)pLibDev->tx[queueIndex].retryValue;
	if(pLibDev->tx[queueIndex].retryValue < 0xf)
		numAttempts++;

	assert(pLibDev->libCfgParams.tx_chain_mask); // Needs to be at least 1

	// Allocate local memory for desc and populate it
    localDescBuffer =     (MDK_5416_2_DESC *)malloc(pLibDev->tx[queueIndex].numDesc * sizeof(MDK_5416_2_DESC));
	localDescPtr = localDescBuffer;
	memset(localDescBuffer, 0, pLibDev->tx[queueIndex].numDesc * sizeof(MDK_5416_2_DESC));

    for(i=0; i<aggFrameNum; i++)
	{
		if (rateMask & RATE_GROUP) {
            rateIndex = rateIdxs[i / numDescPerRate];
		} else {
            rateIndex = rateIdxs[i % numRates];
		}

		assert(rateIndex < 64);
		dup = !IS_HT40_RATE_INDEX(rateIndex) && pLibDev->libCfgParams.ht40_enable && (pLibDev->libCfgParams.extended_channel_op & EXT_CHANNEL_DUP_MASK);
		ext_only = !IS_HT40_RATE_INDEX(rateIndex) && pLibDev->libCfgParams.ht40_enable && (pLibDev->libCfgParams.extended_channel_op & EXT_CHANNEL_ONLY_MASK);
		assert(dup + ext_only <= 1);
		short_gi = pLibDev->libCfgParams.short_gi_enable && IS_HT40_RATE_INDEX(rateIndex);
		ht40 = pLibDev->libCfgParams.ht40_enable && (IS_HT40_RATE_INDEX(rateIndex) || dup);

		totalAggLen = 0;
		localDescAggBlockPtr = localDescPtr;
		for(j=0; j<aggSize; j++)
		{
			frameLen = packetSize;
			if((j != (aggSize-1)) && (frameLen%4 != 0)) {
				pkt_pad = 4 - frameLen%4;
			} else {
				pkt_pad = 0;
			}
			txControl1 = frameLen; // | (1<<24);   // clear dest mask
			totalAggLen = totalAggLen + frameLen + pkt_pad + 4;
			txBufLen = frameLen - 2;

			if( j == (aggSize - 1))
				txControl2 = (2 << 29) | txBufLen | (broadcast ? (1<<24) : 0);  // | (1<<25) bit 25, insert timestamp
			else
				txControl2 = (3 << 29) | txBufLen | (broadcast ? (1<<24) : 0);  // (1<<25)

			if(j == (aggSize-1)){
				localDescPtr->nextPhysPtr = barDescAddr+i*sizeof(MDK_5416_2_DESC);
			} else {
				localDescPtr->nextPhysPtr = descAddress+(i*aggSize+j+1)*sizeof(MDK_5416_2_DESC);
			}
			localDescPtr->bufferPhysPtr = txBufAddr + j*(packetSize);
			localDescPtr->hwControl[0] = txControl1;
			localDescPtr->hwControl[1] = txControl2;
			localDescPtr->hwControl[2] = numAttempts << 16;     // Set number of attempts
			localDescPtr->hwControl[3] = rateValues[rateIndex] << OWL_BITS_TO_TX_DATA_RATE0; // 0x8f; // hardcode rates
            if(pLibDev->swDevID == SW_DEVICE_ID_OWL) {
	            localDescPtr->hwControl[7] = ((pLibDev->libCfgParams.tx_chain_mask | 0x1) & 0x7) << 2 | (short_gi << 1) | ht40; // 20_40, GI, chain_sel
            } else {
                localDescPtr->hwControl[7] = ((pLibDev->libCfgParams.tx_chain_mask) & 0x7) << 2 | (short_gi << 1) | ht40; // 20_40, GI, chain_sel
            }
            localDescPtr++;		
		}  // j loop ends		

		// If this is last agg block set interrupt on done bit
		if(i==(aggFrameNum-1))
		{
			localDescAggBlockPtr->hwControl[0] |= DESC_TX_INTER_REQ;
		}

		for(j=0; j<aggSize; j++)
		{
			localDescAggBlockPtr->hwControl[6] = totalAggLen;  // Set agg len
			localDescAggBlockPtr++;
		}

	}  // end of i loop

    // Copy tx descriptors to device memory
    writeDescriptors(devNum, pLibDev->tx[queueIndex].descAddress, (MDK_ATHEROS_DESC *)localDescBuffer, pLibDev->tx[queueIndex].numDesc);
    free(localDescBuffer);

//printf("SNOOP::Data Packet Desc contents are\n");
//memDisplay(devNum, pLibDev->tx[queueIndex].descAddress, sizeof(MDK_5416_2_DESC)/sizeof(A_UINT32));
//memDisplay(devNum, pLibDev->tx[queueIndex].descAddress+sizeof(MDK_5416_2_DESC), sizeof(MDK_5416_2_DESC)/sizeof(A_UINT32));
//memDisplay(devNum, pLibDev->tx[queueIndex].descAddress+2*sizeof(MDK_5416_2_DESC), sizeof(MDK_5416_2_DESC)/sizeof(A_UINT32));
//memDisplay(devNum, pLibDev->tx[queueIndex].descAddress+3*sizeof(MDK_5416_2_DESC), sizeof(MDK_5416_2_DESC)/sizeof(A_UINT32));


	// create local bar descriptors and populate these
    barLocalDesc = (MDK_5416_2_DESC *)malloc(aggFrameNum * sizeof(MDK_5416_2_DESC));
	barLocalDescPtr = barLocalDesc;
	memset(barLocalDesc, 0, aggFrameNum*sizeof(MDK_5416_2_DESC));
	for(i=0; i<aggFrameNum; i++)
	{
		if(i == (aggFrameNum - 1)) {
			barLocalDescPtr->nextPhysPtr = 0;
//			barLocalDescPtr->hwControl[0] |= DESC_TX_INTER_REQ;
		} else {
			barLocalDescPtr->nextPhysPtr = pLibDev->tx[queueIndex].descAddress + (aggSize*(i+1))*sizeof(MDK_5416_2_DESC);
		}

		barLocalDescPtr->bufferPhysPtr = barBufAddr;
		barLocalDescPtr->hwControl[0] = 24; // | (1<<24);     // bar packet len
		barLocalDescPtr->hwControl[1] = 20; // bar buffer len
		barLocalDescPtr->hwControl[2] = (10<<16);  // 10 retries for BAR
		barLocalDescPtr->hwControl[3] = 0x80;      // send at legacy rate
        if((pLibDev->swDevID == SW_DEVICE_ID_OWL)||((pLibDev->swDevID == SW_DEVICE_ID_HOWL))){
            barLocalDescPtr->hwControl[7] = ((pLibDev->libCfgParams.tx_chain_mask | 0x1) & 0x7) << 2 ; 
        } else {
            barLocalDescPtr->hwControl[7] = ((pLibDev->libCfgParams.tx_chain_mask) & 0x7) << 2 ; 
        }
		barLocalDescPtr++;		
	}

	// copy bar descriptors to device memory
    writeDescriptors(devNum, barDescAddr, (MDK_ATHEROS_DESC *)barLocalDesc, aggFrameNum);
    free(barLocalDesc);

//printf("SNOOP::Bar desc contents are\n");
//memDisplay(devNum, barDescAddr, sizeof(MDK_5416_2_DESC)/sizeof(A_UINT32));
//memDisplay(devNum, barDescAddr + sizeof(MDK_5416_2_DESC), sizeof(MDK_5416_2_DESC)/sizeof(A_UINT32));

	// allocate local memories for packets and create packets
	pTransmitPkt = (A_UCHAR *)malloc(0x1000);  // need to change this number later
	pPkt = pTransmitPkt;
	memset(pTransmitPkt, 0, 0x1000);

	pPktHeader = (WLAN_QOS_DATA_MAC_HEADER3 *)pTransmitPkt;
	//create the packet Header, all fields are listed here to enable easy changing
    pPktHeader->frameControl.protoVer = 0;
    pPktHeader->frameControl.fType = FRAME_DATA;
	pPktHeader->frameControl.fSubtype = SUBT_QOS;
    pPktHeader->frameControl.ToDS = 0;
    pPktHeader->frameControl.FromDS = 0;
    pPktHeader->frameControl.moreFrag = 0;
    pPktHeader->frameControl.retry = 0;
    pPktHeader->frameControl.pwrMgt = 0;
    pPktHeader->frameControl.moreData = 0;
    pPktHeader->frameControl.wep = 0;
    pPktHeader->frameControl.order = 0;

    memcpy(pPktHeader->address1.octets, broadcast ? broadcastAddr : dest, WLAN_MAC_ADDR_SIZE);
    memcpy(pPktHeader->address2.octets, pLibDev->macAddr.octets, WLAN_MAC_ADDR_SIZE);
    memcpy(pPktHeader->address3.octets, pLibDev->bssAddr.octets, WLAN_MAC_ADDR_SIZE);

    WLAN_SET_FRAGNUM(pPktHeader->seqControl, 0);
    WLAN_SET_SEQNUM(pPktHeader->seqControl, 0);

	// Add qos control field
	pPktHeader->qosControl.tid = 0;
	pPktHeader->qosControl.eosp = 0;
	pPktHeader->qosControl.ackpolicy = 0;
	pPktHeader->qosControl.txop = 0;

    // skip encryption for now
    pPkt += sizeof(WLAN_QOS_DATA_MAC_HEADER3);
	pPkt += 2;

    //fill in packet type
    pMdkHeader = (MDK_PACKET_HEADER *)pPkt;
#ifdef ARCH_BIG_ENDIAN
	pMdkHeader->pktType = btol_s(pLibDev->mdkPacketType);
    pMdkHeader->numPackets = btol_s(pLibDev->tx[queueIndex].numDesc);
#else
	pMdkHeader->pktType = pLibDev->mdkPacketType;
    pMdkHeader->numPackets = pLibDev->tx[queueIndex].numDesc;
#endif

    //fill in the repeating pattern
    pPkt += sizeof(MDK_PACKET_HEADER);
    while (dataBodyLength) {
        if(dataBodyLength > dataPatternLength) {
            amountToWrite = dataPatternLength;
            dataBodyLength -= dataPatternLength;
        }
        else {
            amountToWrite = dataBodyLength;
            dataBodyLength = 0;
        }

        memcpy(pPkt, dataPattern, amountToWrite);
        pPkt += amountToWrite;
    }

	// Write packets to memory -- one packets at a time, increment seq num in between
	for(j=0; j<aggSize; j++)
	{
	    WLAN_SET_SEQNUM(pPktHeader->seqControl, j);
	    pLibDev->devMap.OSmemWrite(devNum, txBufAddr + j*(packetSize), pTransmitPkt, packetSize);
	}

    free(pTransmitPkt);

//printf("SNOOP:: 1st tx packet\n");
//memDisplay(devNum, txBufAddr, 10);

    // Create BAR packets -- copy from mdk agg script
    pBarLocalBuffer = (A_UCHAR *)malloc(24);
	memset(pBarLocalBuffer, 0, 24);
	pBarHeader = (WLAN_BAR_CONTROL_MAC_HEADER *) pBarLocalBuffer;

    pBarHeader->frameControl.protoVer = 0;
    pBarHeader->frameControl.fType = FRAME_CTRL;
	pBarHeader->frameControl.fSubtype = SUBT_QOS;
    pBarHeader->frameControl.ToDS = 0;
    pBarHeader->frameControl.FromDS = 0;
    pBarHeader->frameControl.moreFrag = 0;
    pBarHeader->frameControl.retry = 0;
    pBarHeader->frameControl.pwrMgt = 0;
    pBarHeader->frameControl.moreData = 0;
    pBarHeader->frameControl.wep = 0;
    pBarHeader->frameControl.order = 0;

    memcpy(pBarHeader->address1.octets, broadcast ? broadcastAddr : dest, WLAN_MAC_ADDR_SIZE);
    memcpy(pBarHeader->address2.octets, pLibDev->macAddr.octets, WLAN_MAC_ADDR_SIZE);

	pBarHeader->barControl = 0x7ff00005;
    pLibDev->devMap.OSmemWrite(devNum, barBufAddr, pBarLocalBuffer, 24);
	free(pBarLocalBuffer);

//printf("SNOOP:: Bar packet\n");
//memDisplay(devNum, barBufAddr, 10);

    //create the end packet
	pLibDev->mdkPacketType = MDK_LAST_PKT;
    createEndPacket(devNum, queueIndex, dest, antMode, probePkt);

    // Set broadcast for Begin
    if(broadcast) {
        pLibDev->tx[queueIndex].broadcast = 1;
    }
    else {
        pLibDev->tx[queueIndex].broadcast = 0;
    }

    pLibDev->tx[queueIndex].txEnable = 1;
    ar5kInitData[pLibDev->ar5kInitIndex].pMacAPI->setRetryLimit( devNum, queueIndex );

	return;
}
/* txDataAggSetup */


/**************************************************************************
* txDataSetup - create packet and descriptors for transmission
*
*/
MANLIB_API void txDataSetup(
 A_UINT32 devNum,
 A_UINT32 rateMask,
 A_UCHAR *dest,
 A_UINT32 numDescPerRate,
 A_UINT32 dataBodyLength,
 A_UCHAR *dataPattern,
 A_UINT32 dataPatternLength,
 A_UINT32 retries,
 A_UINT32 antenna,
 A_UINT32 broadcast)
{
    LIB_DEV_INFO *pLibDev = gLibInfo.pLibDevArray[devNum];
    A_UCHAR     r, rateIdxs[numRateCodes], rateIndex;
    A_UINT16    i, j, numRates;
    A_UINT32    descAddress, dIndex, descOp;
    A_UINT32    antMode = 0;
    A_UINT32    pktSize;
    A_UINT32    pktAddress;
    MDK_ATHEROS_DESC    *localDescPtr;         //pointer to current descriptor being filled
    MDK_ATHEROS_DESC    *localDescBuffer;      //create a local buffer to create descriptors
    A_UINT16     queueIndex;
    A_BOOL      probePkt = FALSE;
    A_UINT16    mdkPktType = MDK_NORMAL_PKT;
    A_UINT32    *dPtr, lastDesc = 0, intrBit ;
    A_UINT32    falconAddrMod = 0;


	pLibDev->mdkPacketType = MDK_NORMAL_PKT;
//printf("Descriptor per rate: %d\n", numDescPerRate);
//printf("Rate mask: %d\n", rateMask);
    //overloading the broadcast param with additional flags
    if(broadcast & PROBE_PKT) {
        probePkt = TRUE;
        mdkPktType = MDK_PROBE_PKT;

        //setup the probe packets on a different queue
        pLibDev->txProbePacketNext = TRUE;
        queueIndex = PROBE_QUEUE;
        if(pLibDev->backupSelectQueueIndex != PROBE_QUEUE) { //safety check so we don't backup probe_queue index
            pLibDev->backupSelectQueueIndex = pLibDev->selQueueIndex;
        }
        pLibDev->selQueueIndex = PROBE_QUEUE;
    } else {
        pLibDev->selQueueIndex = 0;
        queueIndex = pLibDev->selQueueIndex;
    }

    //cleanup broadcast flag after cleaning up overloaded flags
    broadcast = broadcast & 0x1;

#ifdef DEBUG_MEMORY
printf("SNOOP::txDataSetup::broadcast=%x\n", broadcast);
printf("SNOOP::dest %x:%x:%x:%x:%x:%x\n", dest[6], dest[5], dest[4], dest[3], dest[2], dest[1], dest[0]);
#endif

    pLibDev->tx[queueIndex].dcuIndex = 0;


    pLibDev->tx[queueIndex].retryValue = retries;
    //verify some of the arguments
    if (checkDevNum(devNum) == FALSE) {
        mError(devNum, EINVAL, "Device Number %d:txDataSetup\n", devNum);
        return;
    }

    if(pLibDev->devState < RESET_STATE) {
        mError(devNum, EILSEQ, "Device Number %d:txDataSetup: device not in reset state - resetDevice must be run first\n", devNum);
        return;
    }

    if (0 == numDescPerRate) {
        mError(devNum, EINVAL, "Device Number %d:txDataSetup: must create at least 1 descriptor per rate\n", devNum);
        return;
    }

    /* we must take the MDK pkt header into consideration here otherwise it could make the
     * body size greater than an 802.11 pkt body
     */
    if(dataBodyLength > (MAX_PKT_BODY_SIZE - sizeof(MDK_PACKET_HEADER) - sizeof(WLAN_DATA_MAC_HEADER3) - FCS_FIELD))
    {
        mError(devNum, EINVAL, "Device Number %d:txDataSetup: packet body size must be less than %d [%d - (%d + %d + %d)]\n", devNum,
                                (MAX_PKT_BODY_SIZE - sizeof(MDK_PACKET_HEADER) - sizeof(WLAN_DATA_MAC_HEADER3) - FCS_FIELD),
                                MAX_PKT_BODY_SIZE,
                                sizeof(MDK_PACKET_HEADER),
                                sizeof(WLAN_DATA_MAC_HEADER3),
                                FCS_FIELD);
        return;
    }

    //cleanup any stuff from previous iteration
    if (pLibDev->tx[queueIndex].pktAddress) {
        memFree(devNum, pLibDev->tx[queueIndex].pktAddress);
        pLibDev->tx[queueIndex].pktAddress = 0;
        memFree(devNum, pLibDev->tx[queueIndex].descAddress);
        pLibDev->tx[queueIndex].descAddress = 0;
        pLibDev->tx[queueIndex].txEnable = 0;
    }
    if(pLibDev->tx[queueIndex].endPktAddr) {
        memFree(devNum, pLibDev->tx[queueIndex].endPktAddr);
        memFree(devNum, pLibDev->tx[queueIndex].endPktDesc);
        pLibDev->tx[queueIndex].endPktAddr = 0;
        pLibDev->tx[queueIndex].endPktDesc = 0;
    }
    if(pLibDev->tx[queueIndex].barDescAddress) {
        memFree(devNum, pLibDev->tx[queueIndex].barDescAddress);
        pLibDev->tx[queueIndex].barDescAddress = 0;
        memFree(devNum, pLibDev->tx[queueIndex].barPktAddress);
        pLibDev->tx[queueIndex].barPktAddress = 0;
    }

    //create rate array and findout how many rates exist
    for (r = 0, numRates = 0; r < 32; r++) {
        if (rateMask & (1 << r)) {
            rateIdxs[numRates++] = r;
        }
    }
//printf("Num rate: %d\n", numRates);
//pLibDev->libCfgParams.rateMaskMcs20 = 0;
//pLibDev->libCfgParams.rateMaskMcs40 = 0;

//printf("rateMaskMcs20: 0x%x\n", pLibDev->libCfgParams.rateMaskMcs20);
//printf("rateMaskMcs40: 0x%x\n", pLibDev->libCfgParams.rateMaskMcs40);


    for (; r < 48; r++) {
        if (pLibDev->libCfgParams.rateMaskMcs20 & (1 << (r - 32))) {
            rateIdxs[numRates++] = r;
        }
    }
    for (; r < 64; r++) {
        if (pLibDev->libCfgParams.rateMaskMcs40 & (1 << (r - 48))) {
            rateIdxs[numRates++] = r;
        }
    }

    if (!numRates) {
        mError(devNum, EINVAL, "txDataSetup: No rates specified in rateMask(s)\n");
        return;
    }

//printf("Num rates: %d\n", numRates);

#ifdef _DEBUG
//    printf("\nSNOOP: txDataSetup: %d rates mask 0x%08x mcs20 0x%08x mcs40 0x%08x\n\n",
//        numRates, rateMask, pLibDev->libCfgParams.rateMaskMcs20, pLibDev->libCfgParams.rateMaskMcs40);
#endif

    //create the required number of descriptors
    pLibDev->tx[queueIndex].numDesc     = numDescPerRate * numRates;
    pLibDev->tx[queueIndex].descAddress =    memAlloc( devNum, pLibDev->tx[queueIndex].numDesc * sizeof(MDK_ATHEROS_DESC));

    if (0 == pLibDev->tx[queueIndex].descAddress) {
        mError(devNum, ENOMEM, "Device Number %d:txDataSetup: unable to allocate client memory for %d descriptors\n",
               devNum, pLibDev->tx[queueIndex].numDesc);
        return;
    }

	//setup the transmit packets
	if (pLibDev->noEndPacket) {
		createTransmitPacket(devNum,
                             MDK_SKIP_STATS_PKT,
                             dest,
                             pLibDev->tx[queueIndex].numDesc,
                             dataBodyLength,
                             dataPattern,
                             dataPatternLength,
                             broadcast,
                             queueIndex,
                             &pktSize,
                             &(pLibDev->tx[queueIndex].pktAddress));
	} else {
		createTransmitPacket(devNum,
                             mdkPktType,
                             dest,
                             pLibDev->tx[queueIndex].numDesc,
                             dataBodyLength,
                             dataPattern,
                             dataPatternLength,
                             broadcast,
                             queueIndex,
                             &pktSize,
                             &(pLibDev->tx[queueIndex].pktAddress));
	}

    //take a copy of the dest address
    memcpy(pLibDev->tx[queueIndex].destAddr.octets, dest, WLAN_MAC_ADDR_SIZE);
    pLibDev->tx[queueIndex].dataBodyLen = dataBodyLength;

    if (!ar5kInitData[pLibDev->ar5kInitIndex].pMacAPI->setupAntenna(devNum, antenna, &antMode)) {
        return;
    }

    descAddress = pLibDev->tx[queueIndex].descAddress;
    pktAddress = pLibDev->tx[queueIndex].pktAddress;

    if (isFalcon(devNum) || isDragon(devNum)) {
        falconAddrMod = (A_UINT32)FALCON_MEM_ADDR_MASK;
    }

    localDescBuffer = (MDK_ATHEROS_DESC *)malloc(sizeof(MDK_ATHEROS_DESC) * pLibDev->tx[queueIndex].numDesc);

//printf("TX number of packets: %d\n", pLibDev->tx[queueIndex].numDesc);

    if (!localDescBuffer) {
        mError(devNum, ENOMEM, "Device Number %d:txDataSetup: unable to allocate host memory for %d tx descriptors\n", devNum, pLibDev->tx[queueIndex].numDesc);
        return;
    }
    memset(localDescBuffer, 0, sizeof(MDK_ATHEROS_DESC) * pLibDev->tx[queueIndex].numDesc);

    dIndex = 0;
    localDescPtr = localDescBuffer;
    for (i = 0, j = 0; i < pLibDev->tx[queueIndex].numDesc; i++) {
		//write buffer ptr to descriptor
#if defined(COBRA_AP) && defined(PCI_INTERFACE)
        if(isCobra(pLibDev->swDevID)) {
        localDescPtr->bufferPhysPtr = pktAddress;
        } else {
            localDescPtr->bufferPhysPtr = pktAddress | HOST_PCI_SDRAM_BASEADDR;
        }
#else
        localDescPtr->bufferPhysPtr = pktAddress;
#endif

        if (rateMask & RATE_GROUP) {
            rateIndex = rateIdxs[i / numDescPerRate];
        } else {
            rateIndex = rateIdxs[i % numRates];
        }

        ar5kInitData[pLibDev->ar5kInitIndex].pMacAPI->setDescriptor(devNum, localDescPtr, pktSize,
                            antMode, i, rateIndex, broadcast );

        //write link pointer
        if (i == (pLibDev->tx[queueIndex].numDesc - 1)) { //ie its the last descriptor
            localDescPtr->nextPhysPtr = 0;
        } else {
#if defined(COBRA_AP) && defined(PCI_INTERFACE)
            if(isCobra(pLibDev->swDevID)) {
                localDescPtr->nextPhysPtr = (descAddress + sizeof(MDK_ATHEROS_DESC));
            }
            else {
                localDescPtr->nextPhysPtr = (descAddress + sizeof(MDK_ATHEROS_DESC)) | HOST_PCI_SDRAM_BASEADDR;
            }
#else
            localDescPtr->nextPhysPtr = falconAddrMod | (descAddress + sizeof(MDK_ATHEROS_DESC));
#endif
        }

        if(rateMask & RATE_GROUP) {
            if (j == (numDescPerRate - 1)) {
                j = 0;
                if (pLibDev->devMap.remoteLib) {
                    dPtr = (A_UINT32 *)localDescPtr;

                    lastDesc = LAST_DESC_NEXT << DESC_INFO_LAST_DESC_BIT_START;
                    localDescPtr->hwControl[0] &= ~DESC_TX_INTER_REQ;
                    intrBit = 0;
                    descOp = ((sizeof(MDK_ATHEROS_DESC)/sizeof(A_UINT32))+1) << DESC_OP_WORD_OFFSET_BIT_START;

                    if (i == (pLibDev->tx[queueIndex].numDesc-1)) {
                        lastDesc = LAST_DESC_NULL << DESC_INFO_LAST_DESC_BIT_START;
                        intrBit = DESC_TX_INTER_REQ_START <<  DESC_OP_INTR_BIT_START;
                        descOp = intrBit | \
                                 ((numDescPerRate-1)  << DESC_OP_NDESC_OFFSET_BIT_START) | \
                                 (2 << DESC_OP_WORD_OFFSET_BIT_START);
                    }

                    pLibDev->devMap.r_createDescriptors(devNum, (pLibDev->tx[queueIndex].descAddress + \
                            (dIndex * sizeof(MDK_ATHEROS_DESC))),  \
                            lastDesc | numDescPerRate |  \
                            (sizeof(MDK_ATHEROS_DESC)/sizeof(A_UINT32) << DESC_INFO_NUM_DESC_WORDS_BIT_START), \
                            0, \
                            descOp, \
                            (A_UINT32 *)localDescPtr);

                    dIndex = i+1;
                }
            } else {
                j++;
            }
        }


        //increment descriptor address
        descAddress += sizeof(MDK_ATHEROS_DESC);
        localDescPtr ++;
    }

    //write all the descriptors in one shot
    if (!pLibDev->devMap.remoteLib || !(rateMask & RATE_GROUP)) {
        writeDescriptors(devNum, pLibDev->tx[queueIndex].descAddress, localDescBuffer, pLibDev->tx[queueIndex].numDesc);

#ifdef DEBUG_MEMORY
printf("SNOOP::Desc contents are\n");
memDisplay(devNum, pLibDev->tx[queueIndex].descAddress, sizeof(MDK_ATHEROS_DESC)/sizeof(A_UINT32));
#endif
    }
    free(localDescBuffer);


#ifdef DEBUG_MEMORY
printf("SNOOP::createEndPacket to \n");
printf("SNOOP::dest %x:%x:%x:%x:%x:%x\n", dest[6], dest[5], dest[4], dest[3], dest[2], dest[1], dest[0]);
#endif

    //create the end packet
	pLibDev->mdkPacketType = MDK_LAST_PKT;
    createEndPacket(devNum, queueIndex, dest, antMode, probePkt);

    // Set broadcast for Begin
    if(broadcast) {
        pLibDev->tx[queueIndex].broadcast = 1;
    }
    else {
        pLibDev->tx[queueIndex].broadcast = 0;
    }

    pLibDev->tx[queueIndex].txEnable = 1;
    ar5kInitData[pLibDev->ar5kInitIndex].pMacAPI->setRetryLimit( devNum, queueIndex );
    return;
}


/**
 * - Function Name: cleanupTxRxMemory
 * - Description  :
 * - Arguments
 *     -
 * - Returns      :
 ******************************************************************************/
MANLIB_API void cleanupTxRxMemory(
 A_UINT32 devNum,
 A_UINT32 flags)
{
    LIB_DEV_INFO *pLibDev = gLibInfo.pLibDevArray[devNum];
    A_UINT16     queueIndex;

    pLibDev->selQueueIndex = 0;
    pLibDev->tx[0].dcuIndex = 0;

    queueIndex = pLibDev->selQueueIndex;

    if(flags & TX_CLEAN) {
        if (pLibDev->tx[queueIndex].pktAddress) {
            memFree(devNum, pLibDev->tx[queueIndex].pktAddress);
            pLibDev->tx[queueIndex].pktAddress = 0;
            memFree(devNum, pLibDev->tx[queueIndex].descAddress);
            pLibDev->tx[queueIndex].descAddress = 0;
            pLibDev->tx[queueIndex].txEnable = 0;
        }
        if(pLibDev->tx[queueIndex].endPktAddr) {
            memFree(devNum, pLibDev->tx[queueIndex].endPktAddr);
            memFree(devNum, pLibDev->tx[queueIndex].endPktDesc);
            pLibDev->tx[queueIndex].endPktAddr = 0;
            pLibDev->tx[queueIndex].endPktDesc = 0;
        }
        if(pLibDev->tx[PROBE_QUEUE].pktAddress) {
            memFree(devNum, pLibDev->tx[PROBE_QUEUE].pktAddress);
            pLibDev->tx[PROBE_QUEUE].pktAddress = 0;
            memFree(devNum, pLibDev->tx[PROBE_QUEUE].descAddress);
            pLibDev->tx[PROBE_QUEUE].descAddress = 0;
            pLibDev->tx[PROBE_QUEUE].txEnable = 0;
        }
        if(pLibDev->tx[queueIndex].barDescAddress) {
            memFree(devNum, pLibDev->tx[queueIndex].barDescAddress);
            pLibDev->tx[queueIndex].barDescAddress = 0;
            memFree(devNum, pLibDev->tx[queueIndex].barPktAddress);
            pLibDev->tx[queueIndex].barPktAddress = 0;
        }

    }
    if(flags & RX_CLEAN) {
        if (pLibDev->rx.rxEnable || pLibDev->rx.bufferAddress) {
            memFree(devNum, pLibDev->rx.bufferAddress);
            pLibDev->rx.bufferAddress = 0;
            memFree(devNum, pLibDev->rx.descAddress);
            pLibDev->rx.descAddress = 0;
            pLibDev->rx.rxEnable = 0;
        }
    }

}

void
createEndPacket
(
 A_UINT32 devNum,
 A_UINT16 queueIndex,
 A_UCHAR  *dest,
 A_UINT32 antMode,
 A_BOOL   probePkt
)
{
    LIB_DEV_INFO *pLibDev = gLibInfo.pLibDevArray[devNum];
    A_UINT32 pktSize;
    MDK_ATHEROS_DESC    localDesc;
    A_UINT32            retryValue;
    A_UINT16            endPktType = MDK_LAST_PKT;

    if (pLibDev->noEndPacket) {
        return;
    }

    memset(&localDesc, 0, sizeof(MDK_ATHEROS_DESC));
    pLibDev->tx[queueIndex].endPktDesc =
            memAlloc( devNum, 1 * sizeof(MDK_ATHEROS_DESC));
    if (probePkt) {
        endPktType = MDK_PROBE_LAST_PKT;
    }


    // this is the special end descriptor
    if(pLibDev->yesAgg == TRUE) {
		createTransmitPacketAggr(devNum, endPktType, dest, 1, 0, NULL, 0, 0,
			queueIndex, pLibDev->aggSize, &pktSize, &(pLibDev->tx[queueIndex].endPktAddr));
	} else {
		createTransmitPacket(devNum, endPktType, dest, 1, 0, NULL, 0, 0,
			queueIndex, &pktSize, &(pLibDev->tx[queueIndex].endPktAddr));
	}

    //write buffer ptr to descriptor
#if defined(COBRA_AP) && defined(PCI_INTERFACE)
        if(isCobra(pLibDev->swDevID)) {
            localDesc.bufferPhysPtr = pLibDev->tx[queueIndex].endPktAddr;
        }
        else {
            localDesc.bufferPhysPtr = pLibDev->tx[queueIndex].endPktAddr | HOST_PCI_SDRAM_BASEADDR;
        }
#else
    localDesc.bufferPhysPtr = pLibDev->tx[queueIndex].endPktAddr;
#endif

    //Venice needs to have the retries set for end packets
    //so take a copy and artificially set it
    retryValue = pLibDev->tx[queueIndex].retryValue;
    if(probePkt) {
        pLibDev->tx[queueIndex].retryValue = 0;
    }
    else {
        pLibDev->tx[queueIndex].retryValue = 0xf;
    }


    if (pLibDev->libCfgParams.enableXR) {
        if (isOwl(pLibDev->swDevID)) {
            assert(0);
        } else if (isFalcon(devNum) || isDragon(devNum)) {
#if !defined(FREEDOM_AP)
#if !defined(OWL_AP)
#ifndef __ATH_DJGPPDOS__
            setDescriptorEndPacketAr5513( devNum, &localDesc, pktSize,
                                           antMode, 0, 15, 0);
#endif
#endif
#endif
        } else {
                setDescriptorEndPacketAr5212( devNum, &localDesc, pktSize,
                                       antMode, 0, 15, 0);
        }
    }
    else if (((pLibDev->swDevID & 0x00ff) >= 0x0013) && (pLibDev->mode == MODE_11G) && (!pLibDev->turbo)) {
        if (isOwl(pLibDev->swDevID)) {
            setDescriptorEndPacketAr5416( devNum, &localDesc, pktSize,
                                           antMode, 0, 8, 0);  // 1Mbps-Long rate
        } else if (isFalcon(devNum) || isDragon(devNum)) {
#if !defined(FREEDOM_AP)
#if !defined(OWL_AP)
#ifndef __ATH_DJGPPDOS__
            setDescriptorEndPacketAr5513( devNum, &localDesc, pktSize,
                                           antMode, 0, 8, 0);
#endif
#endif
#endif
        } else {
                setDescriptorEndPacketAr5212( devNum, &localDesc, pktSize,
                                       antMode, 0, 8, 0);
        }
    } else {
        if (isOwl(pLibDev->swDevID)) {
            setDescriptorEndPacketAr5416( devNum, &localDesc, pktSize,
                                           antMode, 0, 0, 0);
        } else {
			ar5kInitData[pLibDev->ar5kInitIndex].pMacAPI->setDescriptor(devNum, &localDesc, pktSize,
                        antMode, 0, 0, 0);
		}
    }
    //restore the retry value
    pLibDev->tx[queueIndex].retryValue = retryValue;

    localDesc.nextPhysPtr = 0;

    writeDescriptor(devNum, pLibDev->tx[queueIndex].endPktDesc, &localDesc);
}

void
sendEndPacket
(
 A_UINT32 devNum,
 A_UINT16 queueIndex
)
{
    LIB_DEV_INFO *pLibDev = gLibInfo.pLibDevArray[devNum];

    if (pLibDev->noEndPacket) {
        return;
    }

#ifdef DEBUG_MEMORY
printf("SNOOP:sendEndPacket::start Send end packet\n");
#endif

    //successful transmission of frames, so send special end packet
    ar5kInitData[pLibDev->ar5kInitIndex].pMacAPI->sendTxEndPacket(devNum, queueIndex );
}

/**************************************************************************
* enableWep - setup the keycache with default keys and set an internel
*             wep enable flag
*             For now the software will hardcode some keys in here.
*             eventually change the interface on here to allow
*             keycache to be setup.
*
*/
MANLIB_API void enableWep
(
 A_UINT32 devNum,
 A_UCHAR key       //which of the default keys to use
)
{
    LIB_DEV_INFO *pLibDev = gLibInfo.pLibDevArray[devNum];

    if (checkDevNum(devNum) == FALSE) {
        mError(devNum, EINVAL, "Device Number %d:enableWep\n", devNum);
        return;
    }

    if(pLibDev->devState < RESET_STATE) {
        mError(devNum, EILSEQ, "Device Number %d:enableWep: device not in reset state - resetDevice must be run first\n", devNum);
        return;
    }

    //program keys 1-3 in the key cache.
    REGW(devNum, 0x9000, 0x22222222);
    REGW(devNum, 0x9004, 0x11);
    REGW(devNum, 0x9014, 0x00);         //40 bit key
    REGW(devNum, 0x901c, 0x8000);       //valid bit
    REGW(devNum, 0x9010, 0x22222222);
    REGW(devNum, 0x9024, 0x11);
    REGW(devNum, 0x9034, 0x00);         //40 bit key
    REGW(devNum, 0x903c, 0x8000);       //valid bit
    REGW(devNum, 0x9040, 0x22222222);
    REGW(devNum, 0x9044, 0x11);
    REGW(devNum, 0x9054, 0x00);         //40 bit key
    REGW(devNum, 0x905c, 0x8000);       //valid bit
    REGW(devNum, 0x9060, 0x22222222);
    REGW(devNum, 0x9064, 0x11);
    REGW(devNum, 0x9074, 0x00);         //40 bit key
    REGW(devNum, 0x907c, 0x8000);       //valid bit

    REGW(devNum, 0x8800, 0x22222222);
    REGW(devNum, 0x8804, 0x11);
    REGW(devNum, 0x8814, 0x00);         //40 bit key
    REGW(devNum, 0x881c, 0x8000);       //valid bit
    REGW(devNum, 0x8810, 0x22222222);
    REGW(devNum, 0x8824, 0x11);
    REGW(devNum, 0x8834, 0x00);         //40 bit key
    REGW(devNum, 0x883c, 0x8000);       //valid bit
    REGW(devNum, 0x8840, 0x22222222);
    REGW(devNum, 0x8844, 0x11);
    REGW(devNum, 0x8854, 0x00);         //40 bit key
    REGW(devNum, 0x885c, 0x8000);       //valid bit
    REGW(devNum, 0x8860, 0x22222222);
    REGW(devNum, 0x8864, 0x11);
    REGW(devNum, 0x8874, 0x00);         //40 bit key
    REGW(devNum, 0x887c, 0x8000);       //valid bit
    pLibDev->wepEnable = 1;
    pLibDev->wepKey = key;
}

/**************************************************************************
* txDataBegin - start transmission
*
*/
MANLIB_API void txDataBegin
(
 A_UINT32 devNum,
 A_UINT32 timeout,
 A_UINT32 remoteStats
)
{
    txDataStart( devNum );
#ifndef _IQV
    txDataComplete( devNum, timeout, remoteStats );
#endif
}

/**************************************************************************
* txDataStart - start transmission
*
*/
MANLIB_API void txDataStart(
 A_UINT32 devNum)
{
    LIB_DEV_INFO *pLibDev = gLibInfo.pLibDevArray[devNum];
    MDK_ATHEROS_DESC    localDesc;
    static A_UINT16 activeQueues[MAX_TX_QUEUE] = {0};   // Index of the active queue
    A_UINT16    activeQueueCount = 0;                   // active queues count
    int i = 0;

    memset(&localDesc, 0, sizeof(MDK_ATHEROS_DESC));

    //if we are sending probe packets, then only enable the probe queue
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

    for( i = 0; i < activeQueueCount; i++ )
    {
        if (checkDevNum(devNum) == FALSE) {
            mError(devNum, EINVAL, "Device Number %d:txDataStart\n", devNum);
            return;
        }

        if(pLibDev->devState < RESET_STATE) {
            mError(devNum, EILSEQ, "Device Number %d:txDataStart: device not in reset state - resetDevice must be run first\n", devNum);
            return;
        }

        if (!pLibDev->tx[activeQueues[i]].txEnable) {
            mError(devNum, EILSEQ,
                "Device Number %d:txDataStart: txDataSetup must successfully complete before running txDataBegin\n", devNum);
            return;
        }

        //zero out the local tx stats structures
        memset(&(pLibDev->tx[activeQueues[i]].txStats[0]), 0, sizeof(TX_STATS_STRUCT) * STATS_BINS);
        pLibDev->tx[activeQueues[i]].haveStats = 0;
    }

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

    // Add a local self-linked rx descriptor and buffer to stop receive overrun
    pLibDev->rx.descAddress = memAlloc( devNum, sizeof(MDK_ATHEROS_DESC));
    if (0 == pLibDev->rx.descAddress) {
        mError(devNum, ENOMEM, "Device Number %d:txDataStart: unable to allocate memory for rx-descriptor to prevent overrun\n", devNum);
        return;
    }
    pLibDev->rx.bufferSize = 512;
    pLibDev->rx.bufferAddress = memAlloc(devNum, pLibDev->rx.bufferSize);
    if (0 == pLibDev->rx.bufferAddress) {
        mError(devNum, ENOMEM, "Device Number %d:txDataStart: unable to allocate memory for rx-buffer to prevent overrun\n", devNum);
        return;
    }
#if defined(COBRA_AP) && defined(PCI_INTERFACE)
        if(isCobra(pLibDev->swDevID)) {
        localDesc.bufferPhysPtr = pLibDev->rx.bufferAddress;
        localDesc.nextPhysPtr = pLibDev->rx.descAddress;
    }
    else {
        localDesc.bufferPhysPtr = pLibDev->rx.bufferAddress | HOST_PCI_SDRAM_BASEADDR;
        localDesc.nextPhysPtr = pLibDev->rx.descAddress | HOST_PCI_SDRAM_BASEADDR;
    }
#else
    localDesc.bufferPhysPtr = pLibDev->rx.bufferAddress;
    localDesc.nextPhysPtr = pLibDev->rx.descAddress;
#endif
    localDesc.hwControl[1] = pLibDev->rx.bufferSize;
    localDesc.hwControl[0] = 0;
    writeDescriptor(devNum, pLibDev->rx.descAddress, &localDesc);

    //write RXDP
#if defined(COBRA_AP) && defined(PCI_INTERFACE)
    if(isCobra(pLibDev->swDevID)) {
        ar5kInitData[pLibDev->ar5kInitIndex].pMacAPI->writeRxDescriptor(devNum, pLibDev->rx.descAddress);
    }
    else {
        ar5kInitData[pLibDev->ar5kInitIndex].pMacAPI->writeRxDescriptor(devNum, pLibDev->rx.descAddress | HOST_PCI_SDRAM_BASEADDR);
    }
#else
    ar5kInitData[pLibDev->ar5kInitIndex].pMacAPI->writeRxDescriptor(devNum, pLibDev->rx.descAddress);
#endif

    //program registers
    ar5kInitData[pLibDev->ar5kInitIndex].pMacAPI->txBeginConfig(devNum, 1);

    pLibDev->start=milliTime();

}   // txDataStart

MANLIB_API void txDataComplete(
 A_UINT32 devNum,
 A_UINT32 timeout,
 A_UINT32 remoteStats)
{
    LIB_DEV_INFO *pLibDev = gLibInfo.pLibDevArray[devNum];
    ISR_EVENT   event;
    A_UINT32    finish;
    A_UINT32    i = 0;
    A_UINT32    statsLoop = 0;
    static A_UINT32 deltaTimeArray[MAX_TX_QUEUE] = {0}; // array to store the delta time
    static A_UINT16 activeQueues[MAX_TX_QUEUE] = {0};   // Index of the active queue
    A_UINT16    activeQueueCount = 0;   // active queues count
    A_UINT16    wfqCount = 0;           // wait for queues count
    A_UINT32    startTime;
    A_UINT32    curTime;

    //if we are sending probe packets, then only enable the probe queue
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
    wfqCount = activeQueueCount;

    startTime = milliTime();

#ifdef DEBUG_MEMORY
printf("SNOOP::txDataComplete::TXDP=%x:TXE=%x\n", REGR(devNum, 0x800), REGR(devNum, 0x840));
printf("SNOOP::txDataComplete::Memory at TXDP %x location is \n", REGR(devNum, 0x800));
memDisplay(devNum, REGR(devNum, 0x800), 12);
printf("SNOOP::txDataComplete::Frame contents at %x pointed location is \n", REGR(devNum, 0x800));
pLibDev->devMap.OSmemRead(devNum, REGR(devNum, 0x800)+4, (A_UCHAR*)&buf_ptr, sizeof(buf_ptr));
memDisplay(devNum, buf_ptr, 10);
printf("SNOOP::txDataComplete::Memory at baseaddress %x location is \n", pLibDev->tx[0].descAddress);
memDisplay(devNum, pLibDev->tx[0].descAddress, 12);
printf("SNOOP::txDataComplete::Frame contents at %x pointed location is \n", pLibDev->tx[0].descAddress);
pLibDev->devMap.OSmemRead(devNum, (pLibDev->tx[0].descAddress+4), (A_UCHAR*)&buf_ptr, sizeof(buf_ptr));
memDisplay(devNum, buf_ptr, 10);
printf("reg8000=%x:reg8004=%x:reg8008=%x:reg800c=%x:reg8014=%x\n", REGR(devNum, 0x8000), REGR(devNum, 0x8004), REGR(devNum, 0x8008), REGR(devNum, 0x800c), REGR(devNum, 0x8014));
#endif

    //wait for event
    for (i = 0; i < timeout && wfqCount; i++)
    {
        event = pLibDev->devMap.getISREvent(devNum);

        if (event.valid)
        {
#ifdef DEBUG_MEMORY
printf("SNOOP::txDataComplete::Got event\n");
#endif

            //see if it is the TX_EOL interrupt
			if ( ar5kInitData[pLibDev->ar5kInitIndex].pMacAPI->isTxdescEvent(event.ISRValue)) {
                /*event.ISRValue & F2_ISR_TXDESC*/
                //This is the event we are waiting for
                //stop the clock
                finish=milliTime();
                deltaTimeArray[pLibDev->selQueueIndex] = finish - pLibDev->start;   // __TODO__  GET THE CORRECT INDEX

                wfqCount--;
                if ( !wfqCount )
                {
                    break;
                }
                continue;   // skip the sleep; we got an interrupt, maybe there is more in the event queue
            } else {
//				printf("TEST:: Not a valid event\n");
			}
        }
        curTime = milliTime();
        if (curTime > (startTime+timeout)) {
            i = timeout;
            break;
        }
        mSleep(1);
    }
//printf("\n>>> txDataComplete:: REG 0x9924(cycpwr_thr1 7:1) : 0x%x\n", REGR(devNum, 0x9924));
//printf(">>> txDataComplete:: REG 0x9858(firstep 17:12)   : 0x%x\n", REGR(devNum, 0x9858));
//printf(">>> txDataComplete:: REG 0x9840(firstep_low 11:6): 0x%x\n", REGR(devNum, 0x9840));

    if (i == timeout)
    {
        mError(devNum, EIO, "Device Number %d:txDataComplete: timeout reached before all frames transmitted\n", devNum);
        finish=milliTime();
        // conintue anyway;
    }

    // Get the stats for all active queues and send end packets
    //
    for( i = 0; i < activeQueueCount; i++ )
    {
        if( !(remoteStats & SKIP_STATS_COLLECTION) ) {
            txAccumulateStats(devNum, deltaTimeArray[activeQueues[i]], activeQueues[i] );
        }

        //successful transmission of frames, so send special end packet
        //ar5kInitData[pLibDev->ar5kInitIndex].pMacAPI->sendTxEndPacket(devNum, activeQueues[i] );
        if(!pLibDev->txProbePacketNext) {
            sendEndPacket(devNum, activeQueues[i]);
        }

    }

    //send stats packets if enabled
    if((remoteStats == ENABLE_STATS_SEND) && (!pLibDev->txProbePacketNext)){
#ifdef DEBUG_MEMORY
printf("SNOOP::txDataComplete::enable stats send\n");
#endif

        // send the stats for all the active queues
        for( i = 0; i < activeQueueCount; i++ )
        {
            for(statsLoop = 1; statsLoop < STATS_BINS; statsLoop++)
            {
                if((pLibDev->tx[activeQueues[i]].txStats[statsLoop].excessiveRetries > 0) ||
                (pLibDev->tx[activeQueues[i]].txStats[statsLoop].goodPackets > 0)) {
                    sendStatsPkt(devNum, statsLoop, MDK_TX_STATS_PKT, pLibDev->tx[activeQueues[i]].destAddr.octets);
                }
            }
            sendStatsPkt(devNum, 0, MDK_TX_STATS_PKT, pLibDev->tx[activeQueues[i]].destAddr.octets);
        }
    }

    //cleanup
    ar5kInitData[pLibDev->ar5kInitIndex].pMacAPI->txCleanupConfig(devNum);

    // reset "noEndPacket" flag
    pLibDev->noEndPacket = FALSE;

    // Reset yes aggregation flag
    pLibDev->yesAgg = FALSE;
    ar5kInitData[pLibDev->ar5kInitIndex].pMacAPI->setRetryLimit( devNum, 0 );

    if(pLibDev->txProbePacketNext) {
        //clear probe packet next flag for next time.
        pLibDev->txProbePacketNext = 0;
        pLibDev->tx[PROBE_QUEUE].txEnable = 0;
        pLibDev->selQueueIndex = pLibDev->backupSelectQueueIndex;
    }
    return;
}

/**************************************************************************
* rxDataSetup - create packet and descriptors for reception
*
*/
MANLIB_API void rxDataSetup
(
 A_UINT32 devNum,
 A_UINT32 numDesc,
 A_UINT32 dataBodyLength,
 A_UINT32 enablePPM
)
{
    LIB_DEV_INFO *pLibDev = gLibInfo.pLibDevArray[devNum];

	pLibDev->yesAgg = FALSE;

    internalRxDataSetup(devNum, numDesc, dataBodyLength, enablePPM, RX_NORMAL);
    return;
}


/**************************************************************************
* rxDataAggSetup - create packet and descriptors for reception of an aggregate frame
*
*/
MANLIB_API void rxDataAggSetup
(
 A_UINT32 devNum,
 A_UINT32 numDesc,
 A_UINT32 dataBodyLength,
 A_UINT32 enablePPM,
 A_UINT32 aggSize
)
{
    LIB_DEV_INFO *pLibDev = gLibInfo.pLibDevArray[devNum];

	pLibDev->yesAgg = TRUE;
	pLibDev->aggSize = aggSize;
    internalRxDataSetup(devNum, numDesc, dataBodyLength, enablePPM, RX_NORMAL);
    return;
}

MANLIB_API void rxDataSetupFixedNumber
(
 A_UINT32 devNum,
 A_UINT32 numDesc,
 A_UINT32 dataBodyLength,
 A_UINT32 enablePPM
)
{
    internalRxDataSetup(devNum, numDesc, dataBodyLength, enablePPM, RX_FIXED_NUMBER);
    return;
}

void internalRxDataSetup
(
 A_UINT32 devNum,
 A_UINT32 numDesc,
 A_UINT32 dataBodyLength,
 A_UINT32 enablePPM,
 A_UINT32 mode
)
{
    A_UINT32    i;
    A_UINT32    bufferAddress;
    A_UINT32    buffAddrIncrement=0, lastDesc;
    A_UINT32    descAddress;
    MDK_ATHEROS_DESC    *localDescPtr;         //pointer to current descriptor being filled
    MDK_ATHEROS_DESC    *localDescBuffer;      //create a local buffer to create descriptors
    A_UINT32    sizeBufferMem, intrBit, descOp;
    A_UINT32    falconAddrMod = 0;
    A_BOOL      falconTrue = FALSE;
    A_UINT32    ppm_data_padding = PPM_DATA_SIZE;

    LIB_DEV_INFO *pLibDev = gLibInfo.pLibDevArray[devNum];

    if (checkDevNum(devNum) == FALSE) {
        mError(devNum, EINVAL, "Device Number %d:rxDataSetup\n", devNum);
        return;
    }
    if (pLibDev->devState < RESET_STATE) {
        mError(devNum, EILSEQ, "Device Number %d:rxDataSetup: device not in reset state - resetDevice must be run first\n", devNum);
        return;
    }

    falconTrue = isFalcon(devNum) || isDragon(devNum);
    if (falconTrue) {
        if (pLibDev->libCfgParams.chainSelect == 2) {
            ppm_data_padding = PPM_DATA_SIZE_FALCON_DUAL_CHAIN;
        } else {
            ppm_data_padding = PPM_DATA_SIZE_FALCON_SINGLE_CHAIN;
        }
    }
    if (isOwl(pLibDev->swDevID)) {
        /* PPM data moved into register space */
        ppm_data_padding = 0;
    }

    if((pLibDev->mode == MODE_11B) && (enablePPM)) {
        //not supported for 11b mode.
        enablePPM = 0;
    }

    // cleanup descriptors created last time
    if (pLibDev->rx.rxEnable || pLibDev->rx.bufferAddress) {
        memFree(devNum, pLibDev->rx.bufferAddress);
        pLibDev->rx.bufferAddress = 0;
        memFree(devNum, pLibDev->rx.descAddress);
        pLibDev->rx.descAddress = 0;
        pLibDev->rx.rxEnable = 0;
    }

    pLibDev->rx.overlappingDesc = FALSE;
    pLibDev->rx.numDesc = numDesc + 11;
    pLibDev->rx.numExpectedPackets = numDesc;
    pLibDev->rx.rxMode = mode;

    /*
     * create descriptors, create eleven extra descriptors and buffers:
     * - one to receive the special "last packet"
     * - up to 9 to receive the stats packet (incase this is enabled)
     * - one linked to itself to prevent overruns
     */
    pLibDev->rx.descAddress = memAlloc( devNum, pLibDev->rx.numDesc * sizeof(MDK_ATHEROS_DESC));
    if (0 == pLibDev->rx.descAddress) {
        mError(devNum, ENOMEM, "Device Number %d:rxDataSetup: unable to allocate memory for %d descriptors\n", devNum, pLibDev->rx.numDesc);
        return;
    }

    // If getting the PPM data - increase the data body length before upsizing for stats
    if (enablePPM) {
        dataBodyLength += ppm_data_padding;
    }

    //create buffers
    //make sure dataBody length is large enough to take a stats packet, if not then
    //pad it out.  The 8 accounts for the rateBin labels at the front of the packets
    if (dataBodyLength < (sizeof(TX_STATS_STRUCT) + sizeof(RX_STATS_STRUCT) + 8)) {
        dataBodyLength = sizeof(TX_STATS_STRUCT) + sizeof(RX_STATS_STRUCT) + 8;
    }

	if(pLibDev->yesAgg == TRUE) {
		pLibDev->rx.bufferSize = (dataBodyLength + sizeof(MDK_PACKET_HEADER)
                    + sizeof(WLAN_QOS_DATA_MAC_HEADER3) + FCS_FIELD+0x2); // margin of 0x20 bytes
	} else {
		pLibDev->rx.bufferSize = dataBodyLength + sizeof(MDK_PACKET_HEADER)
                    + sizeof(WLAN_DATA_MAC_HEADER3) + FCS_FIELD;
	}
    /* The above calculation makes the receive buffer the exact size.  Want to
     * add some extra bytes to end and also make sure buffers and 32 byte aligned.
     * will have at least 0x20 spare bytes
     */
    if ((pLibDev->rx.bufferSize & (cache_line_size - 1)) > 0) {
        pLibDev->rx.bufferSize += (0x20 - (pLibDev->rx.bufferSize & (cache_line_size - 1)));
    }

    if ((pLibDev->rx.bufferSize + 0x20) < 4096) {
        pLibDev->rx.bufferSize += 0x20;
    }

    sizeBufferMem = pLibDev->rx.numDesc * pLibDev->rx.bufferSize;

    //check to see if should try overlapping buffers
    if (sizeBufferMem > pLibDev->devMap.DEV_MEMORY_RANGE) {

//		if(pLibDev->yesAgg == TRUE) {
//			mError(devNum, ENOMEM, "Device Number %d:rxDataSetup: unable to allocate memory for buffers (even overlapped):%d\n", devNum, sizeBufferMem);
//			return;
//		}

        //allocate 1 full buffer (as spare) then add headers + 32 bytes for each required descriptor
        if(pLibDev->yesAgg == TRUE) {
		  sizeBufferMem = pLibDev->rx.bufferSize +
            (pLibDev->rx.numDesc * (sizeof(MDK_PACKET_HEADER) + sizeof(WLAN_QOS_DATA_MAC_HEADER3) + 0x20
                                    + (enablePPM ? ppm_data_padding : 0) ));
		} else {
		  sizeBufferMem = pLibDev->rx.bufferSize +
            (pLibDev->rx.numDesc * (sizeof(MDK_PACKET_HEADER) + sizeof(WLAN_DATA_MAC_HEADER3) + 0x22
                                    + (enablePPM ? ppm_data_padding : 0) ));
		}

        if (sizeBufferMem > pLibDev->devMap.DEV_MEMORY_RANGE) {
            mError(devNum, ENOMEM, "Device Number %d:rxDataSetup: unable to allocate memory for buffers (even overlapped):%d\n", devNum, sizeBufferMem);
            return;
        }

        pLibDev->rx.overlappingDesc = TRUE;
    }


    pLibDev->rx.bufferAddress = memAlloc(devNum, sizeBufferMem);

    if (0 == pLibDev->rx.bufferAddress) {
        mError(devNum, ENOMEM, "Device Number %d:rxDataSetup: unable to allocate memory for buffers:%d\n", devNum, sizeBufferMem);
        return;
    }

    //initialize descriptors
#if defined(COBRA_AP) && defined(PCI_INTERFACE)
    if(isCobra(pLibDev->swDevID)) {
        descAddress = pLibDev->rx.descAddress;
        bufferAddress = pLibDev->rx.bufferAddress;

    if (falconTrue) {
        falconAddrMod = FALCON_MEM_ADDR_MASK ;
    }

    else {
        descAddress = pLibDev->rx.descAddress | HOST_PCI_SDRAM_BASEADDR;
        bufferAddress = pLibDev->rx.bufferAddress | HOST_PCI_SDRAM_BASEADDR;
    }
#else
    descAddress = pLibDev->rx.descAddress;
    bufferAddress = pLibDev->rx.bufferAddress;
#endif
    localDescBuffer = (MDK_ATHEROS_DESC *)malloc(sizeof(MDK_ATHEROS_DESC) * pLibDev->rx.numDesc);
    if (localDescBuffer == NULL) {
        mError(devNum, ENOMEM, "Device Number %d:txDataSetup: unable to allocate local memory for descriptors\n", devNum);
        return;
    }
    memset(localDescBuffer, 0, sizeof(MDK_ATHEROS_DESC) * pLibDev->rx.numDesc);

    localDescPtr = localDescBuffer;
    for (i = 0; i < pLibDev->rx.numDesc; i++)
	{
        localDescPtr->bufferPhysPtr = falconAddrMod | bufferAddress;

        //update the link pointer
        if (i == pLibDev->rx.numDesc - 1) { //ie its the last descriptor
            //link it to itself
            localDescPtr->nextPhysPtr = falconAddrMod | descAddress;
            pLibDev->rx.lastDescAddress = descAddress;
        }
        else {
            localDescPtr->nextPhysPtr = falconAddrMod | (descAddress + sizeof(MDK_ATHEROS_DESC));
        }
        //update the buffer size, and set interrupt for first descriptor
        localDescPtr->hwControl[1] = pLibDev->rx.bufferSize;
        if (i == 0) {
             localDescPtr->hwControl[1] |= DESC_RX_INTER_REQ;
        }
//      else if ((i == (numDesc  - 1)) && (mode == RX_FIXED_NUMBER))
//      {
//           localDescPtr->hwControl[1] |= DESC_RX_INTER_REQ;
//      }

        //increment descriptor address
        descAddress += sizeof(MDK_ATHEROS_DESC);
        localDescPtr ++;

        if(pLibDev->rx.overlappingDesc) {
			if(pLibDev->yesAgg == 1) {
				bufferAddress += (sizeof(MDK_PACKET_HEADER) + sizeof(WLAN_QOS_DATA_MAC_HEADER3) + 0x24  + (enablePPM ? ppm_data_padding : 0));
				buffAddrIncrement = (sizeof(MDK_PACKET_HEADER) + sizeof(WLAN_QOS_DATA_MAC_HEADER3) + 0x24  + (enablePPM ? ppm_data_padding : 0));
			} else {
				bufferAddress += (sizeof(MDK_PACKET_HEADER) + sizeof(WLAN_DATA_MAC_HEADER3) + 0x22  + (enablePPM ? ppm_data_padding : 0));
				buffAddrIncrement = (sizeof(MDK_PACKET_HEADER) + sizeof(WLAN_DATA_MAC_HEADER3) + 0x22  + (enablePPM ? ppm_data_padding : 0));
			}
        } else {
            bufferAddress += pLibDev->rx.bufferSize;
            buffAddrIncrement = pLibDev->rx.bufferSize;
        }
	}

    if (pLibDev->devMap.remoteLib) {
        lastDesc = LAST_DESC_LOOP << DESC_INFO_LAST_DESC_BIT_START;
        intrBit = DESC_RX_INTER_REQ_START <<  DESC_OP_INTR_BIT_START;

        if (!isFalcon(devNum) && !isDragon(devNum)) {
           descOp = intrBit | 0 | (2 << DESC_OP_WORD_OFFSET_BIT_START);
        }
        else{
           descOp = intrBit | 0 | (3 << DESC_OP_WORD_OFFSET_BIT_START);
        }

        pLibDev->devMap.r_createDescriptors(devNum, pLibDev->rx.descAddress, \
                            lastDesc |  pLibDev->rx.numDesc |  \
                            (sizeof(MDK_ATHEROS_DESC)/sizeof(A_UINT32) << DESC_INFO_NUM_DESC_WORDS_BIT_START), \
                            buffAddrIncrement | (1 << BUF_ADDR_INC_CLEAR_BUF_BIT_START), \
                            descOp, \
                            (A_UINT32 *)localDescBuffer);
    } else {
        //write all the descriptors in one shot
        writeDescriptors(devNum, pLibDev->rx.descAddress, localDescBuffer, pLibDev->rx.numDesc);
    }
    free(localDescBuffer);

    ar5kInitData[pLibDev->ar5kInitIndex].pMacAPI->setPPM(devNum, enablePPM);

    pLibDev->rx.rxEnable = 1;

    return;
}

/**************************************************************************
* rxDataBegin - Start and complete reception
*
*/
MANLIB_API void rxDataBegin
(
 A_UINT32 devNum,
 A_UINT32 waitTime,
 A_UINT32 timeout,
 A_UINT32 remoteStats,
 A_UINT32 enableCompare,
 A_UCHAR *dataPattern,
 A_UINT32 dataPatternLength
)
{
    LIB_DEV_INFO *pLibDev = gLibInfo.pLibDevArray[devNum];
    pLibDev->mdkErrno = 0;
    rxDataStart(devNum);
    if (pLibDev->mdkErrno == 0) {
        rxDataComplete(devNum, waitTime, timeout, remoteStats, enableCompare, dataPattern, dataPatternLength);
    }
}

/**************************************************************************
* rxDataBeginFixedNumber - Start and complete reception
*
*/
MANLIB_API void rxDataBeginFixedNumber
(
 A_UINT32 devNum,
 A_UINT32 timeout,
 A_UINT32 remoteStats,
 A_UINT32 enableCompare,
 A_UCHAR *dataPattern,
 A_UINT32 dataPatternLength
)
{
    rxDataStart(devNum);
    rxDataCompleteFixedNumber(devNum, timeout, remoteStats, enableCompare, dataPattern, dataPatternLength);
}


MANLIB_API void SaveMacAddress
(
 A_UINT32 devNum,
 A_CHAR *file_name
)
{
  FILE * pFile;
  A_UINT8 macAddr[6];
  getMacAddr(devNum,0,0,macAddr);
  pFile = fopen (file_name,"w");
  printf("MAC ADRRES: %02x %02x %02x %02x %02x %02x \n",macAddr[0],macAddr[1],macAddr[2],macAddr[3],macAddr[4],macAddr[5]);
  if (pFile!=NULL)
  {
	fputs("MAC ADDRESS: ",pFile);
    fprintf (pFile, "%02x-%02x-%02x-%02x-%02x-%02x",macAddr[0],macAddr[1],macAddr[2],macAddr[3],macAddr[4],macAddr[5]);
    fclose (pFile);
  }
  printf("MAC ADRRESS has been stored in %s\n",file_name);
}

/**************************************************************************
* rxDataStart - Start reception
*
*/
MANLIB_API void rxDataStart
(
 A_UINT32 devNum
)
{
    LIB_DEV_INFO *pLibDev = gLibInfo.pLibDevArray[devNum];

    if (checkDevNum(devNum) == FALSE) {
        mError(devNum, EINVAL, "Device Number %d:rxDataBegin\n", devNum);
        return;
    }

    if(pLibDev->devState < RESET_STATE) {
        mError(devNum, EILSEQ, "Device Number %d:rxDataBegin: device not in reset state - resetDevice must be run first\n", devNum);
        return;
    }

    if (!pLibDev->rx.rxEnable) {
        mError(devNum, EILSEQ,
            "Device Number %d:rxDataBegin: rxDataSetup must successfully complete before running rxDataBegin\n", devNum);
        return;
    }
    // zero out the stats structure
    memset(&(pLibDev->rx.rxStats[0][0]), 0, sizeof(RX_STATS_STRUCT) * STATS_BINS * MAX_TX_QUEUE);
    pLibDev->rx.haveStats = 0;

    // program registers
    ar5kInitData[pLibDev->ar5kInitIndex].pMacAPI->rxBeginConfig(devNum);
}

/**************************************************************************
* rxDataComplete - Complete reception
*
*/
MANLIB_API void rxDataComplete
(
 A_UINT32 devNum,
 A_UINT32 waitTime,
 A_UINT32 timeout,
 A_UINT32 remoteStats,
 A_UINT32 enableCompare,
 A_UCHAR *dataPattern,
 A_UINT32 dataPatternLength
)
{
    ISR_EVENT   event;
    A_UINT32    i, statsLoop, jj;
    A_UINT32    numDesc = 0;
    LIB_DEV_INFO *pLibDev = gLibInfo.pLibDevArray[devNum];
    RX_STATS_TEMP_INFO  statsInfo;
    A_BOOL      statsSent = FALSE;
    A_BOOL      rxComplete = FALSE;
    A_UINT16    queueIndex = pLibDev->selQueueIndex;
    A_UINT32    startTime;
    A_UINT32    curTime;
    A_BOOL      skipStats = FALSE;
    A_UINT16    numStatsToSkip = 0;
    A_UINT32    tempBuff = 0;
    A_BOOL      falconTrue = isFalcon(devNum) || isDragon(devNum), ht40 = FALSE;
    A_UINT32    ppm_data_padding = PPM_DATA_SIZE;
    A_UINT32    status1_offset, status2_offset; // legacy status1 - datalen/rate,
    A_UINT32    expected_num_packets = 0;
    A_UINT32    count1, count2;
    A_BOOL      gotCount1, gotCount2;
    A_UINT32    count1Timestamp;
	A_UINT32    totalRxTime;
A_UINT32 tempCount = 0;
#ifdef _IQV
	FILE *fp;
	FILE *fpStart;
//#define DEBUG_RXPER
#endif
    expected_num_packets = pLibDev->rx.numDesc;
    expected_num_packets = 100*(expected_num_packets / 100);
    //    printf("SNOOP: numDesc = %d, expected packets = %d\n", pLibDev->rx.numDesc, expected_num_packets);

    if (isOwl(pLibDev->swDevID)) {
        ppm_data_padding = 0;
        status1_offset = FIRST_OWL_RX_STATUS_WORD;
        status2_offset = SECOND_OWL_RX_STATUS_WORD;
    } else if (falconTrue) {
        status1_offset = FIRST_FALCON_RX_STATUS_WORD;
        status2_offset = SECOND_FALCON_RX_STATUS_WORD;
        if (pLibDev->libCfgParams.chainSelect == 2) {
            ppm_data_padding = PPM_DATA_SIZE_FALCON_DUAL_CHAIN;
        } else {
            ppm_data_padding = PPM_DATA_SIZE_FALCON_SINGLE_CHAIN;
        }
    } else {
        status1_offset = FIRST_STATUS_WORD;
        status2_offset = SECOND_STATUS_WORD;
    }

    startTime = milliTime();

    timeout = timeout * 2;

#ifdef DEBUG_MEMORY
printf("Desc Base address = %x:RXDP=%x:RXE/RXD=%x\n", pLibDev->rx.descAddress, REGR(devNum, 0xc), REGR(devNum, 0x8));
memDisplay(devNum, pLibDev->rx.descAddress, 12);
printf("reg8000=%x:reg8004=%x:reg8008=%x:reg800c=%x:reg8014=%x\n", REGR(devNum, 0x8000), REGR(devNum, 0x8004), REGR(devNum, 0x8008), REGR(devNum, 0x800c), REGR(devNum, 0x8014));

#endif

 #ifdef _IQV
//  to inform IQFact that ready for sending packets
	fpStart = fopen("log/PerStart.txt", "w");
	if (fpStart)
	{
		fprintf(fpStart, "start!");    // for sync with descriptor read.
		fclose(fpStart);
	}
	#ifdef DEBUG_RXPER
		printf("I am waiting....\n");		// _IQV
	#endif	//DEBUG_RXPER
#endif
    // wait for event
    event.valid = 0;
    event.ISRValue = 0;
    for (i = 0; i < waitTime; i++)
    {
        event = pLibDev->devMap.getISREvent(devNum);
        if (event.valid) {
            if(ar5kInitData[pLibDev->ar5kInitIndex].pMacAPI->isRxdescEvent(event.ISRValue)) {
	#ifdef _IQV
        #ifdef DEBUG_RXPER
				printf("I detected event....i:%d\n", i);		// _IQV
	#endif	//DEBUG_RXPER
	#endif //_IQV
			mSleep(150);
                break;
            }
        }

        curTime = milliTime();
        if (curTime > (startTime+waitTime)) {
            i = waitTime;
            break;
        }
        mSleep(1);
    }

#ifdef _IQV
	// wait for packets all be sent
	#ifdef DEBUG_RXPER
		printf("I am out of event waiting ....\n");		// _IQV
	#endif	//DEBUG_RXPER
		fp = fopen("log/PerDone.txt", "r");
		while (!fp)
		{
			Sleep(100);
			fp = fopen("log/PerDone.txt", "r");
		}
		if (fp){
			fclose(fp);
			DeleteFile("log/PerDone.txt");
		}
	#ifdef DEBUG_RXPER
		printf("I am out of event waiting and done sending all packets....\n");		// _IQV
	#endif	//DEBUG_RXPER
#endif

#ifdef DEBUG_MEMORY
printf("waitTime = %d\n", waitTime);
printf("Desc Base address = %x:RXDP=%x:RXE/RXD=%x\n", pLibDev->rx.descAddress, REGR(devNum, 0xc), REGR(devNum, 0x8));
memDisplay(devNum, pLibDev->rx.descAddress, sizeof(MDK_ATHEROS_DESC)/sizeof(A_UINT32));
printf("reg8000=%x:reg8004=%x:reg8008=%x:reg800c=%x:reg8014=%x\n", REGR(devNum, 0x8000), REGR(devNum, 0x8004), REGR(devNum, 0x8008), REGR(devNum, 0x800c), REGR(devNum, 0x8014));

#endif

    // This is a special case to allow the script multitest.pl to send and receive between
    // two cards in the same machine.  A wait time of 1 will setup RX and exit
    // A second rxDataBegin will collect the statistics.
    if (waitTime == 1) {
        return;
    } else if ((i == waitTime) && (waitTime !=0)) {
        mError(devNum, EIO, "Device Number %d:rxDataBegin: nothing received within %d millisecs (waitTime)\n", devNum, waitTime);
        ar5kInitData[pLibDev->ar5kInitIndex].pMacAPI->rxCleanupConfig(devNum);
        return;
    }

    i = 0;
    memset(&statsInfo, 0, sizeof(RX_STATS_TEMP_INFO));
    for(statsLoop = 0; statsLoop < STATS_BINS; statsLoop++) {
        statsInfo.sigStrength[statsLoop].rxMinSigStrength = 127;
        if (falconTrue ) {
            for (jj = 0; jj < 4; jj++ ) {
                statsInfo.multAntSigStrength[statsLoop].rxMinSigStrengthAnt[jj] = 127;
                statsInfo.multAntSigStrength[statsLoop].rxMaxSigStrengthAnt[jj] = -127;
            }
        }
        if (isOwl(pLibDev->swDevID)) {
            for (jj = 0; jj < 3; jj++ ) {
                statsInfo.ctlAntStrength[jj][statsLoop].rxMinSigStrength = 127;
                statsInfo.extAntStrength[jj][statsLoop].rxMinSigStrength = 127;
                statsInfo.ctlAntStrength[jj][statsLoop].rxMaxSigStrength = -127;
                statsInfo.extAntStrength[jj][statsLoop].rxMaxSigStrength = -127;
            }
        }
    }

    if(remoteStats & SKIP_SOME_STATS) {
        //extract and set number to skip
        skipStats = 1;
        numStatsToSkip = (A_UINT16)((remoteStats & NUM_TO_SKIP_M) >> NUM_TO_SKIP_S);
    }

    if(pLibDev->rx.enablePPM) {
        for( i = 0; i < MAX_TX_QUEUE; i++ )
        {
            for(statsLoop = 0; statsLoop < STATS_BINS; statsLoop++)
            {
                pLibDev->rx.rxStats[i][statsLoop].ppmMax = -1000;
                pLibDev->rx.rxStats[i][statsLoop].ppmMin = 1000;
            }
        }
    }

    statsInfo.descToRead = pLibDev->rx.descAddress;
    if (falconTrue) {
        statsInfo.descToRead &= FALCON_DESC_ADDR_MASK;
    }

    if (enableCompare) {
        //create a max size compare buffer for easy compare
        statsInfo.pCompareData = (A_UCHAR *)malloc(FRAME_BODY_SIZE);

        if(!statsInfo.pCompareData) {
            mError(devNum, ENOMEM, "Device Number %d:rxDataBegin: Unable to allocate memory for compare buffer\n", devNum);
            ar5kInitData[pLibDev->ar5kInitIndex].pMacAPI->rxCleanupConfig(devNum);
            return;
        }

        fillCompareBuffer(statsInfo.pCompareData, FRAME_BODY_SIZE, dataPattern, dataPatternLength);
    }

    startTime=milliTime();
    count1 = 0;
    count2 = 0;
    gotCount1 = FALSE;
    gotCount2 = FALSE;
    count1Timestamp = 0;


    //sit in polling loop, looking for descriptors to be done.
    do {
        //read descriptor status words

        // Read descriptor at once
        fillRxDescAndFrame(devNum, &statsInfo);

//		memDisplay(devNum, statsInfo.bufferAddr, 15);

//		printf("Status1: 0x%08x\t Status2: 0x%08x\n", statsInfo.status1, statsInfo.status2);

        if (statsInfo.status2 & DESC_DONE) {
tempCount++;
            if (falconTrue) {
                statsInfo.bufferAddr &= FALCON_DESC_ADDR_MASK;
            }

            if (!(remoteStats & LEAVE_DESC_STATUS)) {
                //zero out status
                pLibDev->devMap.OSmemWrite(devNum, statsInfo.descToRead + status1_offset,
                        (A_UCHAR *)&tempBuff, sizeof(tempBuff));
                pLibDev->devMap.OSmemWrite(devNum, statsInfo.descToRead + status2_offset,
                        (A_UCHAR *)&tempBuff, sizeof(tempBuff));
            }

            if (isOwl(pLibDev->swDevID)) {
                A_UINT32 tempRateIndex;
                tempRateIndex = *( (A_UINT32 *)((A_UCHAR *)(&(statsInfo.desc)) + 28) );
                ht40 = (A_BOOL)((tempRateIndex >> 1) & 0x1);

                if (isOwl_2_Plus(pLibDev->macRev)) {

                    tempRateIndex = *( (A_UINT32 *)((A_UCHAR *)(&(statsInfo.desc)) + 16) );
                    statsInfo.descRate = (tempRateIndex >> 24) & pLibDev->rxDataRateMsk;
                } else {
                    statsInfo.descRate = (tempRateIndex >> 2) & pLibDev->rxDataRateMsk;
                }
            } else {
                statsInfo.descRate = ((statsInfo.status1 >> BITS_TO_RX_DATA_RATE) & pLibDev->rxDataRateMsk);
            }
            statsInfo.descRate = descRate2RateIndex(statsInfo.descRate, ht40);

            // Initialize loop variables
            statsInfo.badPacket = 0;
            statsInfo.gotHeaderInfo = FALSE; // TODO: FIX for multi buffer packet support
            statsInfo.illegalBuffSize = 0;
            statsInfo.controlFrameReceived = 0;
            statsInfo.mdkPktType = 0;

            // Increase buffer address for PPM
            if(pLibDev->rx.enablePPM) {
                statsInfo.bufferAddr += ppm_data_padding;
            }

            //reset our timeout counter
            i = 0;

            if (numDesc == pLibDev->rx.numDesc - 1) {
                //This is the final looped rx desc and done bit is set, we ran out of receive descriptors,
                //so return with an error
                mError(devNum, ENOMEM, "Device Number %d:rxDataComplete:  ran out of receive descriptors\n", devNum);
                break;
            }

            mdkExtractAddrAndSequence(devNum, &statsInfo);

#ifdef DEBUG_MEMORY
printf("SNOOP::mdkPkt s1=%X:s2=%X:Type = %x\n", statsInfo.status1, statsInfo.status2, statsInfo.mdkPktType);
printf("SNOOP::Process packet:bP=%d:cFR=%d:iBS=%d, numDesc=%d\n", statsInfo.badPacket, statsInfo.controlFrameReceived, statsInfo.illegalBuffSize, numDesc);
#endif
            //only process packets if things are good
            if ( !((statsInfo.status1 & DESC_MORE) || statsInfo.badPacket || statsInfo.controlFrameReceived ||
                statsInfo.illegalBuffSize) ) {
#ifdef DEBUG_MEMORY
printf("SNOOP::Processing packet\n");
#endif
                //check for this being "last packet" or stats packet
                //mdkExtractAddrAndSequence also pulled mdkPkt type info from packet
                if ((statsInfo.status2 & DESC_FRM_RCV_OK)
                    && (statsInfo.mdkPktType == MDK_LAST_PKT))
                {
//					printf("TEST:: MDK_LAST_PKT received\n");
                    //if were not expecting remote stats then we are done
                    if (!(remoteStats & ENABLE_STATS_RECEIVE)) {
                        rxComplete = TRUE;
                    }

                    //we have done with receive so can send stats
                    if(remoteStats & ENABLE_STATS_SEND) {
                        for (i = 0; i < MAX_TX_QUEUE; i++ )
                        {
                            for(statsLoop = 1; statsLoop < STATS_BINS; statsLoop++) {
                                if((pLibDev->rx.rxStats[i][statsLoop].crcPackets > 0) ||
                                    (pLibDev->rx.rxStats[i][statsLoop].goodPackets > 0)) {
                                    sendStatsPkt(devNum, statsLoop, MDK_RX_STATS_PKT, statsInfo.addrPacket.octets);
                                }
                            }
                        }
                        sendStatsPkt(devNum, 0, MDK_RX_STATS_PKT, statsInfo.addrPacket.octets);
                        statsSent = TRUE;
                    }
                } else if((statsInfo.status2 & DESC_FRM_RCV_OK)
                     && (statsInfo.mdkPktType >= MDK_TX_STATS_PKT)
                     && (statsInfo.mdkPktType <= MDK_TXRX_STATS_PKT))
                {
                    rxComplete = mdkExtractRemoteStats(devNum, &statsInfo);
                } else if (statsInfo.mdkPktType == MDK_NORMAL_PKT) {
                    if (enableCompare) {
                        comparePktData(devNum, &statsInfo);
                    }
                    if (pLibDev->rx.enablePPM) {
                        extractPPM(devNum, &statsInfo);
                    }

                    if(skipStats && (numStatsToSkip != 0)) {
                        numStatsToSkip --;
                    } else {

                        mdkExtractRxStats(devNum, &statsInfo);
                    }
                } else if ((statsInfo.mdkPktType == MDK_PROBE_PKT) ||
                        (statsInfo.mdkPktType == MDK_PROBE_LAST_PKT)) {
                    //Want to ignore probe packets, do nothing

                } else {
                    if(statsInfo.status2 & DESC_FRM_RCV_OK) {
                        mError(devNum, EIO, "Device Number %d:A good matching packet with an unknown MDK_PKT type detected MDK_PKT: %d\n", devNum,
                            statsInfo.mdkPktType);
                        {
     //                       A_UINT32 iIndex;
     //                       printf("Frame Info\n");
     //                       for(iIndex=0; iIndex<statsInfo.frame_info_len+10; iIndex++) {
     //                           printf("%X ", statsInfo.frame_info[iIndex]);
     //                       }
     //                       printf("\n");
     //                       printf("PPM Data Info\n");
     //                       for(iIndex=0; iIndex<statsInfo.ppm_data_len; iIndex++) {
     //                           printf("%X ", statsInfo.ppm_data[iIndex]);
     //                       }
     //                       printf("\n");
	 //						printf("Frame Len: %d, desc num: %d\n", (statsInfo.status1 & 0xfff), numDesc);
                        }
                    }
                    if (statsInfo.status2 & DESC_CRC_ERR)
                    {
                        pLibDev->rx.rxStats[statsInfo.qcuIndex][0].crcPackets++;
                        pLibDev->rx.rxStats[statsInfo.qcuIndex][statsInfo.descRate].crcPackets++;
                        pLibDev->rx.haveStats = 1;
                    }
                    else if (statsInfo.status2 & pLibDev->decryptErrMsk)
                    {
                        pLibDev->rx.rxStats[statsInfo.qcuIndex][0].decrypErrors++;
                        pLibDev->rx.rxStats[statsInfo.qcuIndex][statsInfo.descRate].decrypErrors++;
                        pLibDev->rx.haveStats = 1;
                    }


                }
            }

            if (rxComplete) {

				pLibDev->rx.rxStats[statsInfo.qcuIndex][0].endTime = statsInfo.rxTimeStamp;
				totalRxTime = pLibDev->rx.rxStats[statsInfo.qcuIndex][0].endTime - pLibDev->rx.rxStats[statsInfo.qcuIndex][0].startTime;
				if(totalRxTime > 0)	{
					pLibDev->rx.rxStats[statsInfo.qcuIndex][0].rxThroughPut = (A_UINT32)(1000.0 * (8.0*pLibDev->rx.rxStats[statsInfo.qcuIndex][0].byteCount)/totalRxTime); // kbps
                } else {
					pLibDev->rx.rxStats[statsInfo.qcuIndex][0].rxThroughPut = 0xFFFFFFFF;  // -1?
				}

//				printf("ST: %d\tET: %d\tTT: %d\tTput: %d\n", pLibDev->rx.rxStats[statsInfo.qcuIndex][0].startTime, pLibDev->rx.rxStats[statsInfo.qcuIndex][0].endTime, totalRxTime, pLibDev->rx.rxStats[statsInfo.qcuIndex][0].rxThroughPut);
//				printf("Byte count: %d\n", pLibDev->rx.rxStats[statsInfo.qcuIndex][0].byteCount);

				// calculate average EVM
				if(pLibDev->rx.rxStats[statsInfo.qcuIndex][0].goodPackets > 1)
				{
/*
                    printf("\nGOOD packets: %d\n",pLibDev->rx.rxStats[statsInfo.qcuIndex][0].goodPackets);
					printf("Total EVM: %d, %d\n", pLibDev->rx.rxStats[statsInfo.qcuIndex][0].evm_stream0,
                                                  pLibDev->rx.rxStats[statsInfo.qcuIndex][0].evm_stream1);
*/
                    /* calculate EVM based for the number of frames sent */
                    if( pLibDev->yesAgg ) {
                        /* for aggregate pkts, need to divide by agg size */
                        /* we multiply by 10 so we can carry one digit after
                         * the decimal point, hence, need to divide by 10 to get
                         * actual EVM
                         */
                        pLibDev->rx.rxStats[statsInfo.qcuIndex][0].evm_stream0 =
                        (A_UINT32) ((float)pLibDev->rx.rxStats[statsInfo.qcuIndex][0].evm_stream0 * 10.0 /
                                    ((float)pLibDev->rx.rxStats[statsInfo.qcuIndex][0].goodPackets / (float)pLibDev->aggSize));
                        pLibDev->rx.rxStats[statsInfo.qcuIndex][0].evm_stream1 =
                        (A_UINT32) ((float)pLibDev->rx.rxStats[statsInfo.qcuIndex][0].evm_stream1 * 10.0 /
                                    ((float)pLibDev->rx.rxStats[statsInfo.qcuIndex][0].goodPackets / (float)pLibDev->aggSize));
                    }
                    else {
                        pLibDev->rx.rxStats[statsInfo.qcuIndex][0].evm_stream0 =
                        (A_UINT32) ((float)pLibDev->rx.rxStats[statsInfo.qcuIndex][0].evm_stream0 * 10.0 /
                                    (float)pLibDev->rx.rxStats[statsInfo.qcuIndex][0].goodPackets);
                        pLibDev->rx.rxStats[statsInfo.qcuIndex][0].evm_stream1 =
                        (A_UINT32) ((float)pLibDev->rx.rxStats[statsInfo.qcuIndex][0].evm_stream1 * 10.0 /
                                    (float)pLibDev->rx.rxStats[statsInfo.qcuIndex][0].goodPackets);
                    }
				}
                break;
            }

            //get next descriptor to process
            statsInfo.descToRead += sizeof(MDK_ATHEROS_DESC);
            numDesc ++;
        } else {
#ifndef _IQV
            curTime = milliTime();

//			printf("I: %d\tCurTime: %d\tStart Time: %d\t, Timeout: %d\n", i, curTime, startTime, timeout);
	        // printf("SNOOP: i=%d, numPkts = %d, expected = %d (%d)\n", i, numDesc, expected_num_packets, pLibDev->rx.numDesc);
	        if ((curTime > (startTime+300)) && (isOwl(pLibDev->swDevID))) {
	          if (!gotCount1 && (numDesc > 90*expected_num_packets/100)) {
                  if (!gotCount1) {
                      gotCount1 = TRUE;
                      count1 = numDesc;
                      count1Timestamp = curTime;
                  }
              }
              if (gotCount1 && !gotCount2 && (curTime > (count1Timestamp+50))) {
                  gotCount2 = TRUE;
                  count2 = numDesc;
              }
              if (gotCount1 && gotCount2) {
                  if (count2 > count1) {
                      gotCount1 = FALSE;
                      gotCount2 = FALSE;
                  } else {
  		              //printf("SNOOP: OWL : got more that 90 precent of expected packets : %d [%d]. punting\n", numDesc, expected_num_packets);
  		              break;
                  }
              }
	        }
            if (curTime > (startTime+timeout)) {
                i = timeout;

				printf("TIMEOUT: rxDataComplete, %d milisec passed without DESC_DONE\n", timeout);

                break;
            }
            mSleep(1);
            i++;
#else
			#ifdef DEBUG_RXPER
				printf("statsInfo.status2: %x	", statsInfo.status2);
				printf("pLibDev->rx.numDesc: %d, numDesc: %d\n", pLibDev->rx.numDesc, numDesc);
			#endif	//DEBUG_RXPER
			rxComplete = TRUE;
			break;
#endif		// _IQV
        }

    } while (i < timeout);

    milliTime();
//printf("SNOOP: processed %d descriptors\n", tempCount);

    if ((remoteStats & ENABLE_STATS_SEND) && !statsSent) {
        for( i = 0; i < MAX_TX_QUEUE; i++ ) {
            for(statsLoop = 1; statsLoop < STATS_BINS; statsLoop++) {
                if((pLibDev->rx.rxStats[i][statsLoop].crcPackets > 0) ||
                    (pLibDev->rx.rxStats[i][statsLoop].goodPackets > 0))
                {
                    sendStatsPkt(devNum, statsLoop, MDK_RX_STATS_PKT, statsInfo.addrPacket.octets);
                }
            }
            sendStatsPkt(devNum, 0, MDK_RX_STATS_PKT, statsInfo.addrPacket.octets);
        }
    }

    //cleanup
    ar5kInitData[pLibDev->ar5kInitIndex].pMacAPI->rxCleanupConfig(devNum);

    if(pLibDev->tx[queueIndex].txEnable) {
        //write back the retry value
        //REGW(devNum, F2_RETRY_LMT, pLibDev->tx[queueIndex].retryValue);   __TODO__
    }

    //do any other register cleanup required
    if (enableCompare) {
        free(statsInfo.pCompareData);
    }

    if ((i == timeout) && (rxComplete != TRUE)) {
        mError(devNum, EIO,
            "Device Number %d:rxDataBegin: timeout reached, without receiving all packets.  Number received = %lu\n", devNum,
            numDesc);
        ar5kInitData[pLibDev->ar5kInitIndex].pMacAPI->rxCleanupConfig(devNum);
    }

    return;
}
/**************************************************************************
* rxDataCompleteFixedNumber - receive until get required amount of frames
*
*/
MANLIB_API void rxDataCompleteFixedNumber
(
 A_UINT32 devNum,
 A_UINT32 timeout,
 A_UINT32 remoteStats,
 A_UINT32 enableCompare,
 A_UCHAR *dataPattern,
 A_UINT32 dataPatternLength
)
{
    ISR_EVENT   event;
    A_UINT32    i,  jj, statsLoop;
    A_UINT32    numDesc = 0;
    LIB_DEV_INFO *pLibDev = gLibInfo.pLibDevArray[devNum];
    RX_STATS_TEMP_INFO  statsInfo;
    A_UINT32    startTime = 0;
    A_UINT32    curTime;
    A_BOOL      skipStats = FALSE;
    A_UINT16     numStatsToSkip = 0;
    A_UINT32     numReceived = 0;
    A_BOOL       done = FALSE, ht40 = FALSE;
    A_BOOL      falconTrue = isFalcon(devNum) || isDragon(devNum);

    startTime = milliTime();
    if(remoteStats & ENABLE_STATS_RECEIVE) {
        mError(devNum, EINVAL, "Device Number %d:Stats receive is not supported in this mode.  Returning...\n", devNum);
        return;
    }

    // wait for event
    event.valid = 0;
    event.ISRValue = 0;
    for (i = 0; i < timeout; i++)
    {
        event = pLibDev->devMap.getISREvent(devNum);
        if (event.valid) {
            if(ar5kInitData[pLibDev->ar5kInitIndex].pMacAPI->isRxdescEvent(event.ISRValue)) {
                //reset the starttime clock so we timeout from first received
                startTime = milliTime();
                break;
            }
        }

        curTime = milliTime();
        if (curTime > (startTime+timeout)) {
            i = timeout;
            break;
        }
        mSleep(1);
    }


    if (i == timeout) {
        mError(devNum, EIO, "Device Number %d:rxDataBeginFixedNumber: did not recieved required number within %d millisecs (timeout)\n", devNum, timeout);
        ar5kInitData[pLibDev->ar5kInitIndex].pMacAPI->rxCleanupConfig(devNum);
        //fall through and gather stats anyway incase we received some
    }

    i = 0;

    memset(&statsInfo, 0, sizeof(RX_STATS_TEMP_INFO));
    for(statsLoop = 0; statsLoop < STATS_BINS; statsLoop++) {
        statsInfo.sigStrength[statsLoop].rxMinSigStrength = 127;
        if (falconTrue) {
            for (jj = 0; jj < 4; jj++ ) {
                statsInfo.multAntSigStrength[statsLoop].rxMinSigStrengthAnt[jj] = 127;
                statsInfo.multAntSigStrength[statsLoop].rxMaxSigStrengthAnt[jj] = -127;
            }
        }
        if (isOwl(pLibDev->swDevID)) {
            for (jj = 0; jj < 3; jj++ ) {
                statsInfo.ctlAntStrength[jj][statsLoop].rxMinSigStrength = 127;
                statsInfo.extAntStrength[jj][statsLoop].rxMinSigStrength = 127;
                statsInfo.ctlAntStrength[jj][statsLoop].rxMaxSigStrength = -127;
                statsInfo.extAntStrength[jj][statsLoop].rxMaxSigStrength = -127;
            }
        }
    }

    if(remoteStats & SKIP_SOME_STATS) {
        //extract and set number to skip
        skipStats = 1;
        numStatsToSkip = (A_UINT16)((remoteStats & NUM_TO_SKIP_M) >> NUM_TO_SKIP_S);
    }

    if(pLibDev->rx.enablePPM) {
        for( i = 0; i < MAX_TX_QUEUE; i++ )
        {
            for(statsLoop = 0; statsLoop < STATS_BINS; statsLoop++)
            {
                pLibDev->rx.rxStats[i][statsLoop].ppmMax = -1000;
                pLibDev->rx.rxStats[i][statsLoop].ppmMin = 1000;
            }
        }
    }

    statsInfo.descToRead = pLibDev->rx.descAddress;
    if (enableCompare) {
        //create a max size compare buffer for easy compare
        statsInfo.pCompareData = (A_UCHAR *)malloc(FRAME_BODY_SIZE);

        if(!statsInfo.pCompareData) {
            mError(devNum, ENOMEM, "Device Number %d:rxDataBegin: Unable to allocate memory for compare buffer\n", devNum);
            ar5kInitData[pLibDev->ar5kInitIndex].pMacAPI->rxCleanupConfig(devNum);
            return;
        }

        fillCompareBuffer(statsInfo.pCompareData, FRAME_BODY_SIZE, dataPattern, dataPatternLength);
    }

    //sit in polling loop, looking for descriptors to be done.
    do {
        fillRxDescAndFrame(devNum, &statsInfo);

        if (statsInfo.status2 & DESC_DONE) {

            if (isOwl(pLibDev->swDevID)) {
                A_UINT32 tempRateIndex;

                tempRateIndex = *( (A_UINT32 *)((A_UCHAR *)(&(statsInfo.desc)) + 28) );
                ht40 = (A_BOOL)((tempRateIndex >> 1) & 0x1);

                if (isOwl_2_Plus(pLibDev->macRev)) {
                    tempRateIndex = *( (A_UINT32 *)((A_UCHAR *)(&(statsInfo.desc)) + 16) );
                    statsInfo.descRate = (tempRateIndex >> 24) & pLibDev->rxDataRateMsk;
                } else {
                    statsInfo.descRate = (tempRateIndex >> 2) & pLibDev->rxDataRateMsk;
                }
            } else {
                statsInfo.descRate = ((statsInfo.status1 >> BITS_TO_RX_DATA_RATE) & pLibDev->rxDataRateMsk);
            }
            statsInfo.descRate = descRate2RateIndex(statsInfo.descRate, ht40);

            // Initialize loop variables
            statsInfo.badPacket = 0;
            statsInfo.gotHeaderInfo = FALSE; // TODO: FIX for multi buffer packet support
            statsInfo.illegalBuffSize = 0;
            statsInfo.controlFrameReceived = 0;
            statsInfo.mdkPktType = 0;

            //just gather simple statistics on the packets received.
            // Increase buffer address for PPM
            if(pLibDev->rx.enablePPM && !isOwl(pLibDev->swDevID)) {
                statsInfo.bufferAddr += PPM_DATA_SIZE;
            }

            //reset our timeout counter
            i = 0;

            if (numDesc == pLibDev->rx.numDesc - 1) {
                //This is the final looped rx desc we are done, don't count this one
                printf("Device Number %d:got to looped descriptor\n", devNum);
                done = TRUE;
            }
            //only process packets if things are good
            if ( !(statsInfo.status1 & DESC_MORE) ) {
                if (enableCompare) {
                    comparePktData(devNum, &statsInfo);
                }
                if (pLibDev->rx.enablePPM) {
                    extractPPM(devNum, &statsInfo);
                }

                if(skipStats && (numStatsToSkip != 0)) {
                    numStatsToSkip --;
                } else {
                    mdkExtractAddrAndSequence(devNum, &statsInfo);
                    if(!statsInfo.badPacket) {
                        mdkExtractRxStats(devNum, &statsInfo);
                        numReceived++;

                    }
                }
                if (numReceived == pLibDev->rx.numExpectedPackets) {
                    done = TRUE;
                }
            }

            //get next descriptor to process
            statsInfo.descToRead += sizeof(MDK_ATHEROS_DESC);
            numDesc ++;
        } else {
            if(i != timeout) {
                curTime = milliTime();
                if (curTime > (startTime+timeout)) {
                    i = timeout;
                    ar5kInitData[pLibDev->ar5kInitIndex].pMacAPI->rxCleanupConfig(devNum);
                    continue;
                }
                mSleep(1);
                i++;
            } else {
                if(!statsInfo.status2) {
                    //timeout and no more descriptors to gather stats from
                    done = TRUE;
                }
            }
        }

    } while (!done);

    if (remoteStats & ENABLE_STATS_SEND) {
        for ( i = 0; i < MAX_TX_QUEUE; i++ ) {
            for (statsLoop = 1; statsLoop < STATS_BINS; statsLoop++) {
                if ((pLibDev->rx.rxStats[i][statsLoop].crcPackets > 0) ||
                    (pLibDev->rx.rxStats[i][statsLoop].goodPackets > 0))
                {
                    sendStatsPkt(devNum, statsLoop, MDK_RX_STATS_PKT, statsInfo.addrPacket.octets);
                }
            }
            sendStatsPkt(devNum, 0, MDK_RX_STATS_PKT, statsInfo.addrPacket.octets);
        }
    }

    //do any other register cleanup required
    if (enableCompare) {
        free(statsInfo.pCompareData);
    }
    return;
}

/**************************************************************************
* checkBeaconRSSI - check the rssi of beacons
*
*/
MANLIB_API void checkBeaconRSSI
(
 A_UINT32 devNum,
 A_UINT32 timeout,
 A_UINT32 remoteStats,
 A_UINT32 enableCompare,
 A_UCHAR *dataPattern,
 A_UINT32 dataPatternLength,
 A_CHAR * GIVEN_SSID,
 A_INT32 rssiThreshold,
 A_UINT16 antenna,
 A_INT32 * result
)
{
    ISR_EVENT   event;
    A_UINT32    i,  jj, statsLoop;
    A_UINT32    numDesc = 0;
    LIB_DEV_INFO *pLibDev = gLibInfo.pLibDevArray[devNum];
    RX_STATS_TEMP_INFO  statsInfo;
    A_UINT32    startTime = 0;
    A_UINT32    curTime;
    A_BOOL      skipStats = FALSE;
    A_UINT16     numStatsToSkip = 0;
    A_UINT32     numReceived = 0;
    A_BOOL       done = FALSE, ht40 = FALSE;
    A_BOOL      falconTrue = isFalcon(devNum) || isDragon(devNum);

    rxDataStart(devNum);


    startTime = milliTime();
    if(remoteStats & ENABLE_STATS_RECEIVE) {
        mError(devNum, EINVAL, "Device Number %d:Stats receive is not supported in this mode.  Returning...\n", devNum);
        return;
    }

    // wait for event
    event.valid = 0;
    event.ISRValue = 0;
    for (i = 0; i < timeout; i++)
    {
        event = pLibDev->devMap.getISREvent(devNum);
        if (event.valid) {
            if(ar5kInitData[pLibDev->ar5kInitIndex].pMacAPI->isRxdescEvent(event.ISRValue)) {
                //reset the starttime clock so we timeout from first received
                startTime = milliTime();
                break;
            }
        }

        curTime = milliTime();
        if (curTime > (startTime+timeout)) {
            i = timeout;
            break;
        }
        mSleep(1);
    }


    if (i == timeout) {
        mError(devNum, EIO, "Device Number %d:rxDataBeginFixedNumber: did not recieved required number within %d millisecs (timeout)\n", devNum, timeout);
        ar5kInitData[pLibDev->ar5kInitIndex].pMacAPI->rxCleanupConfig(devNum);
        //fall through and gather stats anyway incase we received some
    }

    i = 0;

    memset(&statsInfo, 0, sizeof(RX_STATS_TEMP_INFO));
    for(statsLoop = 0; statsLoop < STATS_BINS; statsLoop++) {
        statsInfo.sigStrength[statsLoop].rxMinSigStrength = 127;
        if (falconTrue) {
            for (jj = 0; jj < 4; jj++ ) {
                statsInfo.multAntSigStrength[statsLoop].rxMinSigStrengthAnt[jj] = 127;
                statsInfo.multAntSigStrength[statsLoop].rxMaxSigStrengthAnt[jj] = -127;
            }
        }
        if (isOwl(pLibDev->swDevID)) {
            for (jj = 0; jj < 3; jj++ ) {
                statsInfo.ctlAntStrength[jj][statsLoop].rxMinSigStrength = 127;
                statsInfo.extAntStrength[jj][statsLoop].rxMinSigStrength = 127;
                statsInfo.ctlAntStrength[jj][statsLoop].rxMaxSigStrength = -127;
                statsInfo.extAntStrength[jj][statsLoop].rxMaxSigStrength = -127;
            }
        }
    }

    if(remoteStats & SKIP_SOME_STATS) {
        //extract and set number to skip
        skipStats = 1;
        numStatsToSkip = (A_UINT16)((remoteStats & NUM_TO_SKIP_M) >> NUM_TO_SKIP_S);
    }

    if(pLibDev->rx.enablePPM) {
        for( i = 0; i < MAX_TX_QUEUE; i++ )
        {
            for(statsLoop = 0; statsLoop < STATS_BINS; statsLoop++)
            {
                pLibDev->rx.rxStats[i][statsLoop].ppmMax = -1000;
                pLibDev->rx.rxStats[i][statsLoop].ppmMin = 1000;
            }
        }
    }

    statsInfo.descToRead = pLibDev->rx.descAddress;
    if (enableCompare) {
        //create a max size compare buffer for easy compare
        statsInfo.pCompareData = (A_UCHAR *)malloc(FRAME_BODY_SIZE);

        if(!statsInfo.pCompareData) {
            mError(devNum, ENOMEM, "Device Number %d:rxDataBegin: Unable to allocate memory for compare buffer\n", devNum);
            ar5kInitData[pLibDev->ar5kInitIndex].pMacAPI->rxCleanupConfig(devNum);
            return;
        }

        fillCompareBuffer(statsInfo.pCompareData, FRAME_BODY_SIZE, dataPattern, dataPatternLength);
    }

  //initializing variables
  A_INT8 rssi;
  //A_INT32 mac_index=0;
  //A_UINT32 temp=0;
  A_INT32 bitsToRxSigStrength=0;
    //sit in polling loop, looking for descriptors to be done.
  if(antenna)
      bitsToRxSigStrength=8;//RSSI for ANT B is 8-15 bits in the RX status word; For ANT A, it is 0-7
    do {
        fillRxDescAndFrame(devNum, &statsInfo);

        if (statsInfo.status2 & DESC_DONE) {

            if (isOwl(pLibDev->swDevID)) {
                A_UINT32 tempRateIndex;

                tempRateIndex = *( (A_UINT32 *)((A_UCHAR *)(&(statsInfo.desc)) + 28) );
                ht40 = (A_BOOL)((tempRateIndex >> 1) & 0x1);

                if (isOwl_2_Plus(pLibDev->macRev)) {
                    tempRateIndex = *( (A_UINT32 *)((A_UCHAR *)(&(statsInfo.desc)) + 16) );
                    statsInfo.descRate = (tempRateIndex >> 24) & pLibDev->rxDataRateMsk;
                } else {
                    statsInfo.descRate = (tempRateIndex >> 2) & pLibDev->rxDataRateMsk;
                }
            } else {
                statsInfo.descRate = ((statsInfo.status1 >> BITS_TO_RX_DATA_RATE) & pLibDev->rxDataRateMsk);
            }
            statsInfo.descRate = descRate2RateIndex(statsInfo.descRate, ht40);

            // Initialize loop variables
            statsInfo.badPacket = 0;
            statsInfo.gotHeaderInfo = FALSE; // TODO: FIX for multi buffer packet support
            statsInfo.illegalBuffSize = 0;
            statsInfo.controlFrameReceived = 0;
            statsInfo.mdkPktType = 0;

            //just gather simple statistics on the packets received.
            // Increase buffer address for PPM
            if(pLibDev->rx.enablePPM && !isOwl(pLibDev->swDevID)) {
                statsInfo.bufferAddr += PPM_DATA_SIZE;
            }

            //reset our timeout counter
            i = 0;

            if (numDesc == pLibDev->rx.numDesc - 1) {
                //This is the final looped rx desc we are done, don't count this one
                printf("Device Number %d:got to looped descriptor\n", devNum);
                done = TRUE;
            }
            //only process packets if things are good
            if ( !(statsInfo.status1 & DESC_MORE) ) {
                if (enableCompare) {
                    comparePktData(devNum, &statsInfo);
                }
                if (pLibDev->rx.enablePPM) {
                    extractPPM(devNum, &statsInfo);
                }

                if(skipStats && (numStatsToSkip != 0)) {
                    numStatsToSkip --;
                } else {
                   //=========================================================================//







				   					//  RSSI //
				   					A_UINT16			 frameControlValue;
				   					//FRAME_CONTROL       *pFrameControl;
				   					WLAN_MACADDR		 frameMacAddr = {0,0,0,0,0,0};
				   					// functionality for SSID
				   					A_UINT32 address=statsInfo.bufferAddr;
				   					A_UINT32 iIndex, memValue,ssid_length,k=0;
				   					A_CHAR  SSID[100];

                    //find out the type of frame
                    pLibDev->devMap.OSmemRead(devNum, address, (A_UCHAR *)&memValue, sizeof(memValue));

					if(statsInfo.status2 & DESC_FRM_RCV_OK) // take only the valid frames

						if(memValue==0x80){ // process only the beacons

                            memcpy((A_UCHAR *)&frameControlValue, (A_UCHAR*)&(statsInfo.frame_info[0]), sizeof(frameControlValue));

							// compute the mac address
  							for (i = 0; i < WLAN_MAC_ADDR_SIZE; i++){
								memcpy( &(frameMacAddr.octets[i]), &(statsInfo.frame_info[0]) + ADDRESS2_START + i, sizeof(frameMacAddr.octets[i]));
							}

							// find out the ssid_length
							pLibDev->devMap.OSmemRead(devNum, address+(9*4), (A_UCHAR *)&memValue, sizeof(memValue));
							ssid_length=(memValue&0x0000ff00)>>8;
							if(ssid_length==0){ // hidden SSID
								rssi=(A_INT8)((statsInfo.status4 >> bitsToRxSigStrength) & SIG_STRENGTH_MASK);
								if(rssi>=rssiThreshold){
									printf("\nFound one beacon with hidden SSID and RSSI above threshold :%d\n",rssiThreshold);
									printf("MAC Addr: ");
									for(i=0;i<5;i++)
										printf("%02x-",frameMacAddr.octets[i]);
									printf("%02x",frameMacAddr.octets[5]);
									printf(" RSSI= %d\n",rssi);
									*result=1;
									return;
								}
							}
							else if((ssid_length>0)&&(ssid_length<=32)){ // ssid length <= 32							
								SSID[k++]=(A_CHAR)((memValue>>16)&0x000000ff);
								SSID[k++]=(A_CHAR)((memValue>>24)&0x000000ff);
								if(ssid_length>2)
									for(iIndex=10; iIndex<(11+((ssid_length-3)/4)); iIndex++){
											pLibDev->devMap.OSmemRead(devNum, address+(iIndex*4), (A_UCHAR *)&memValue, sizeof(memValue));
											for(i=0;((i<4)&&(k<ssid_length));i++,k++){
													SSID[k]=(A_CHAR)((memValue>>i*8)&0x000000ff);
											}
									}
								SSID[k]=0;
								//compare the SSIDs
								if( !strcmp(SSID,GIVEN_SSID)){
									rssi=(A_INT8)((statsInfo.status4 >> bitsToRxSigStrength) & SIG_STRENGTH_MASK);
									if(rssi>=rssiThreshold){
										printf("\nFound one beacon with SSID: \"%s\" and RSSI above threshold :%d\n",GIVEN_SSID,rssiThreshold);
										printf("MAC Addr: ");
										for(i=0;i<5;i++)
											printf("%02x-",frameMacAddr.octets[i]);
										printf("%02x",frameMacAddr.octets[5]);
										printf(" RSSI= %d\n",rssi);
										*result=1;
										return;
									}	

								}

							}
                        } // if memValue==80

				   					numReceived++;
				   				}
				   				if(numReceived == pLibDev->rx.numExpectedPackets) {
				   					done = TRUE;
				   				}
				   			}
				               //statsInfo.descToRead=temp;
				   			//get next descriptor to process
				   			statsInfo.descToRead += sizeof(MDK_ATHEROS_DESC);
				   			numDesc ++;
				   		}
				   		else {
				   			if(i != timeout) {
				   				curTime = milliTime();
				   				if (curTime > (startTime+timeout)) {
				   					i = timeout;
				   					ar5kInitData[pLibDev->ar5kInitIndex].pMacAPI->rxCleanupConfig(devNum);
				   					continue;
				   				}
				   				mSleep(1);
				   				i++;
				   			}
				   			else {
				   				if(!statsInfo.status2) {
				   					//timeout and no more descriptors to gather stats from
				   					done = TRUE;
				   				}
				   			}
				   		}

				   	} while (!done);
				   	printf("Could not find a beacon with SSID: %s and RSSI above threshold: %d\n",GIVEN_SSID,rssiThreshold);
				      *result=0;

				   	//do any other register cleanup required
				   	if(enableCompare) {
				   		free(statsInfo.pCompareData);
				   	}
				   	return;
}

MANLIB_API A_BOOL rxLastDescStatsSnapshot
(
 A_UINT32 devNum,
 RX_STATS_SNAPSHOT *pRxStats
)
{
    A_UINT32  compStatus[2];
    LIB_DEV_INFO *pLibDev = gLibInfo.pLibDevArray[devNum];
    A_BOOL      falconTrue = FALSE, ht40 = FALSE;
    A_UINT32    ppm_data_padding = PPM_DATA_SIZE;
    A_UINT32    ant_rssi;
    A_UINT32    status1_offset, status2_offset, status3_offset; // legacy status1 - datalen/rate,
                                                                // legacy status2 - done/error/timestamp
                                                                // new with falcon status3 - rssi for 4 ant
    A_UINT32    evm0, evm1, evm2;


    falconTrue = isFalcon(devNum) || isDragon(devNum);

    if (isOwl(pLibDev->swDevID)) {
        ppm_data_padding = 0;
        status1_offset = FIRST_OWL_RX_STATUS_WORD;
        status2_offset = SECOND_OWL_RX_STATUS_WORD;
        status3_offset = OWL_ANT_RSSI_RX_STATUS_WORD;
    } else if (falconTrue) {
        status1_offset = FIRST_FALCON_RX_STATUS_WORD;
        status2_offset = SECOND_FALCON_RX_STATUS_WORD;
        status3_offset = FALCON_ANT_RSSI_RX_STATUS_WORD;
        if (pLibDev->libCfgParams.chainSelect == 2) {
            ppm_data_padding = PPM_DATA_SIZE_FALCON_DUAL_CHAIN;
        } else {
            ppm_data_padding = PPM_DATA_SIZE_FALCON_SINGLE_CHAIN;
        }
    } else {
        status1_offset = FIRST_STATUS_WORD;
        status2_offset = SECOND_STATUS_WORD;
        status3_offset = status2_offset; // to avoid illegal value. should never be used besides multAnt chips
    }


    memset(pRxStats, 0, sizeof(RX_STATS_SNAPSHOT));
    //read the complete status of the last descriptor
    pLibDev->devMap.OSmemRead(devNum, pLibDev->rx.lastDescAddress + status1_offset,
                (A_UCHAR *)&compStatus, 4);
    pLibDev->devMap.OSmemRead(devNum, pLibDev->rx.lastDescAddress + status2_offset,
                ((A_UCHAR *)&compStatus) + 4, 4);


    //extract the stats if the done bit is set
    if(!compStatus[1] & DESC_DONE) {
        //descriptor is not done
        return FALSE;
    }

    //check in case we only got half the status written
    if(!compStatus[0] || !compStatus[1]) {
        //we read an incomplete descriptor
        printf("Device Number %d:SNOOP: read an incomplete descriptor\n", devNum);
        return FALSE;
    }

    if(compStatus[0] & DESC_MORE) {
        //more bit is set, ignore
        printf("Device Number %d:SNOOP: more bit set on descriptor, ignoring\n", devNum);
        return FALSE;
    }

    //read stats
    if(compStatus[1] & DESC_FRM_RCV_OK) {
        pRxStats->goodPackets = 1;

        if (isOwl(pLibDev->swDevID)) {
            pLibDev->devMap.OSmemRead(devNum, pLibDev->rx.lastDescAddress + status3_offset,
                (A_UCHAR *)&(pRxStats->DataSigStrength), sizeof(pRxStats->DataSigStrength));
            pRxStats->DataSigStrength = (A_INT8)((pRxStats->DataSigStrength >> pLibDev->bitsToRxSigStrength) & SIG_STRENGTH_MASK);
        } else {
            pRxStats->DataSigStrength = (A_INT8)((compStatus[0] >> pLibDev->bitsToRxSigStrength) & SIG_STRENGTH_MASK);
        }
        if (falconTrue) {
            pRxStats->chStr  = (compStatus[0] >> 14) & 0x1;
            pRxStats->ch0Sel = (compStatus[0] >> 28) & 0x1;
            pRxStats->ch1Sel = (compStatus[0] >> 29) & 0x1;
            pRxStats->ch0Req = (compStatus[0] >> 30) & 0x1;
            pRxStats->ch1Req = (compStatus[0] >> 31) & 0x1;

            pLibDev->devMap.OSmemRead(devNum, pLibDev->rx.lastDescAddress + status3_offset,
                        (A_UCHAR *)&ant_rssi, 4);
            pRxStats->DataSigStrengthPerAnt[0] = (A_INT8)((ant_rssi >>  0) & SIG_STRENGTH_MASK);
            pRxStats->DataSigStrengthPerAnt[1] = (A_INT8)((ant_rssi >>  8) & SIG_STRENGTH_MASK);
            pRxStats->DataSigStrengthPerAnt[2] = (A_INT8)((ant_rssi >> 16) & SIG_STRENGTH_MASK);
            pRxStats->DataSigStrengthPerAnt[3] = (A_INT8)((ant_rssi >> 24) & SIG_STRENGTH_MASK);
        }
        if (isOwl(pLibDev->swDevID)) {
            pLibDev->devMap.OSmemRead(devNum, pLibDev->rx.lastDescAddress + 28,
                (A_UCHAR *)&(pRxStats->dataRate), sizeof(pRxStats->dataRate));
            ht40 = (A_BOOL)((pRxStats->dataRate >> 1) & 0x1);

            if (isOwl_2_Plus(pLibDev->macRev)) {
                pLibDev->devMap.OSmemRead(devNum, pLibDev->rx.lastDescAddress + OWL_RX_PRIM_CHAIN_STATUS_WORD,
                (A_UCHAR *)&(pRxStats->dataRate), sizeof(pRxStats->dataRate));
                pRxStats->dataRate = (pRxStats->dataRate >> 24) & pLibDev->rxDataRateMsk;

				// Get EVM numbers
                pLibDev->devMap.OSmemRead(devNum, pLibDev->rx.lastDescAddress + 0x24,
                    (A_UCHAR *)&(evm0), sizeof(evm0));
                pLibDev->devMap.OSmemRead(devNum, pLibDev->rx.lastDescAddress + 0x28,
                    (A_UCHAR *)&(evm1), sizeof(evm1));
                pLibDev->devMap.OSmemRead(devNum, pLibDev->rx.lastDescAddress + 0x2c,
                    (A_UCHAR *)&(evm2), sizeof(evm2));

			    if(ht40 == 1) {
				  pRxStats->evm_stream0 = (((evm0 >> 24) & 0xff) + ((evm0 >> 16) & 0xff)
					+ ((evm0 >> 8) & 0xff) + (evm0 & 0xff) + ((evm1 >> 8) & 0xff) + (evm1 & 0xff))/6;

				  pRxStats->evm_stream1 = (((evm1 >> 24) & 0xff) + ((evm1 >> 16) & 0xff)
					+ ((evm2 >> 24) & 0xff) + ((evm2 >> 16) & 0xff) + ((evm2 >> 8) & 0xff) + (evm2 & 0xff))/6;
				} else {
				  pRxStats->evm_stream0 = (((evm0 >> 24) & 0xff) + ((evm0 >> 16) & 0xff)
					+ ((evm0 >> 8) & 0xff) + (evm0 & 0xff))/4;
				  pRxStats->evm_stream1 = (((evm1 >> 24) & 0xff) + ((evm1 >> 16) & 0xff)
					+ ((evm1 >> 8) & 0xff) + (evm1 & 0xff))/4;

//                  printf("EVM: %d,  %d\n", pRxStats->evm_stream0, pRxStats->evm_stream1);
				}

            } else {
                pRxStats->dataRate = (pRxStats->dataRate >> 2) & pLibDev->rxDataRateMsk;
            }

            /* Record the MIMO per antenna RSSIs */
            pLibDev->devMap.OSmemRead(devNum, pLibDev->rx.lastDescAddress + OWL_RX_PRIM_CHAIN_STATUS_WORD,
                (A_UCHAR *)&(ant_rssi), sizeof(ant_rssi));
            pRxStats->DataSigStrengthPerAnt[0] = (A_INT8)((ant_rssi >>  0) & SIG_STRENGTH_MASK);
            pRxStats->DataSigStrengthPerAnt[1] = (A_INT8)((ant_rssi >>  8) & SIG_STRENGTH_MASK);
            pRxStats->DataSigStrengthPerAnt[2] = (A_INT8)((ant_rssi >> 16) & SIG_STRENGTH_MASK);

            pLibDev->devMap.OSmemRead(devNum, pLibDev->rx.lastDescAddress + status3_offset,
                (A_UCHAR *)&(ant_rssi), sizeof(ant_rssi));
            pRxStats->DataSigStrengthPerExtAnt[0] = (A_INT8)((ant_rssi >>  0) & SIG_STRENGTH_MASK);
            pRxStats->DataSigStrengthPerExtAnt[1] = (A_INT8)((ant_rssi >>  8) & SIG_STRENGTH_MASK);
            pRxStats->DataSigStrengthPerExtAnt[2] = (A_INT8)((ant_rssi >> 16) & SIG_STRENGTH_MASK);
        } else {
            pRxStats->dataRate = (A_UINT16)((compStatus[0] >> BITS_TO_RX_DATA_RATE) & pLibDev->rxDataRateMsk);
        }
        pRxStats->dataRate = descRate2RateIndex(pRxStats->dataRate, ht40) - 1;

        pRxStats->bodySize = (A_UINT16)(compStatus[0] & DESC_DATA_LENGTH_FIELDS);

        //remove the 802.11 header, FCS and mdk packet header from this length
        pRxStats->bodySize = pRxStats->bodySize - (sizeof(WLAN_DATA_MAC_HEADER3) + sizeof(MDK_PACKET_HEADER) + FCS_FIELD);
    } else {
        if (compStatus[1] & DESC_CRC_ERR)
        {
            pRxStats->crcPackets = 1;
        }
        else if (compStatus[1] & pLibDev->decryptErrMsk)
        {
            pRxStats->decrypErrors = 1;
        }
    }

    //zero out the completion status
    compStatus[0] = 0;
    pLibDev->devMap.OSmemWrite(devNum, pLibDev->rx.lastDescAddress + status1_offset,
                (A_UCHAR *)&compStatus, sizeof(compStatus[0]));
    pLibDev->devMap.OSmemWrite(devNum, pLibDev->rx.lastDescAddress + status2_offset,
                (A_UCHAR *)&compStatus, sizeof(compStatus[0]));

    return TRUE;
}

/**************************************************************************
* txrxDataBegin - Start transmit and reception
*
*/
MANLIB_API void txrxDataBegin
(
 A_UINT32 devNum,
 A_UINT32 waitTime,
 A_UINT32 timeout,
 A_UINT32 remoteStats,
 A_UINT32 enableCompare,
 A_UCHAR *dataPattern,
 A_UINT32 dataPatternLength
)
{
    ISR_EVENT   event;
    A_UINT32    i, jj, statsLoop;
    A_BOOL      txComplete = FALSE;
    A_BOOL      rxComplete = FALSE;
    LIB_DEV_INFO *pLibDev = gLibInfo.pLibDevArray[devNum];
    A_UINT32    numDesc = 0;
    RX_STATS_TEMP_INFO  rxStatsInfo;
    A_BOOL      statsSent = FALSE;
    A_UINT32 start, finish;
    A_BOOL      lastPktReceived = FALSE;
    A_BOOL      statsPktReceived = FALSE;
    A_UINT16    queueIndex = pLibDev->selQueueIndex;
    A_UINT32 startTime;
    A_UINT32 curTime;
    A_BOOL      skipStats = FALSE;
    A_UINT16     numStatsToSkip = 0;
    A_BOOL      falconTrue = FALSE;
    A_UINT32    ppm_data_padding = PPM_DATA_SIZE;
    A_BOOL      ht40 = FALSE;

    falconTrue = isFalcon(devNum) || isDragon(devNum);

    if (isOwl(pLibDev->swDevID)) {
        ppm_data_padding = 0;
    } else if (falconTrue) {
        if (pLibDev->libCfgParams.chainSelect == 2) {
            ppm_data_padding = PPM_DATA_SIZE_FALCON_DUAL_CHAIN;
        } else {
            ppm_data_padding = PPM_DATA_SIZE_FALCON_SINGLE_CHAIN;
        }
    }

    timeout = timeout * 10;

    if (checkDevNum(devNum) == FALSE) {
        mError(devNum, EINVAL, "Device Number %d:txrxDataBegin\n", devNum);
        return;
    }

    if(pLibDev->devState < RESET_STATE) {
        mError(devNum, EILSEQ, "Device Number %d:txrxDataBegin: device not in reset state - resetDevice must be run first\n", devNum);
        return;
    }

    if (!pLibDev->rx.rxEnable || !pLibDev->tx[queueIndex].txEnable) {
        mError(devNum, EILSEQ,
            "Device Number %d:txrxDataBegin: txDataSetup and rxDataSetup must complete before running txrxDataBegin\n", devNum);
        return;
    }

    //zero out the stats structure
    memset(&(pLibDev->tx[queueIndex].txStats[0]), 0, sizeof(TX_STATS_STRUCT) * STATS_BINS);
    pLibDev->tx[queueIndex].haveStats = 0;
    memset(&(pLibDev->rx.rxStats[0][0]), 0, sizeof(RX_STATS_STRUCT) * STATS_BINS * MAX_TX_QUEUE);
    pLibDev->rx.haveStats = 0;
    if(remoteStats & ENABLE_STATS_RECEIVE) {
        memset(&pLibDev->txRemoteStats[0], 0, sizeof(TX_STATS_STRUCT) * STATS_BINS);
        memset(&(pLibDev->rxRemoteStats[0][0]), 0, sizeof(RX_STATS_STRUCT) * STATS_BINS * MAX_TX_QUEUE);
    }

    // start receive first
    ar5kInitData[pLibDev->ar5kInitIndex].pMacAPI->rxBeginConfig(devNum);

#ifdef DEBUG_MEMORY
printf("SNOOP::waitTime for receive=%d\n", waitTime);
printf("SNOOP::RX desc contents of %X\n", pLibDev->rx.descAddress);
memDisplay(devNum, pLibDev->rx.descAddress, sizeof(MDK_ATHEROS_DESC)/sizeof(A_UINT32));
pLibDev->devMap.OSmemRead(devNum, (pLibDev->rx.descAddress+4), (A_UCHAR*)&buf_ptr, sizeof(buf_ptr));
printf("SNOOP::RX desc pointed frame contents at %X\n", buf_ptr);
memDisplay(devNum, buf_ptr, 30);
#endif

    //wait for event
    startTime = milliTime();
    event.valid = 0;
    event.ISRValue = 0;
    for (i = 0; i < waitTime; i++) {
        event = pLibDev->devMap.getISREvent(devNum);
        if (event.valid) {
            if(ar5kInitData[pLibDev->ar5kInitIndex].pMacAPI->isRxdescEvent(event.ISRValue)) {
				mSleep(150);
                break;
            }
        }
        curTime = milliTime();
        if (curTime > (startTime+waitTime)) {
            i = waitTime;
            break;
        }
        mSleep(1);
    }

    if ((i == waitTime) && (waitTime !=0)) {
        mError(devNum, EIO, "Device Number %d:txrxDataBegin: nothing received within %d millisecs (waitTime)\n", devNum, waitTime);
        ar5kInitData[pLibDev->ar5kInitIndex].pMacAPI->rxCleanupConfig(devNum);
        return;
    }

    i = 0;
    memset(&rxStatsInfo, 0, sizeof(RX_STATS_TEMP_INFO));
    for(statsLoop = 0; statsLoop < STATS_BINS; statsLoop++) {
        rxStatsInfo.sigStrength[statsLoop].rxMinSigStrength = 127;
        if (falconTrue ) {
            for (jj = 0; jj < 4; jj++ ) {
                rxStatsInfo.multAntSigStrength[statsLoop].rxMinSigStrengthAnt[jj] = 127;
                rxStatsInfo.multAntSigStrength[statsLoop].rxMaxSigStrengthAnt[jj] = -127;
            }
        }
        if (isOwl(pLibDev->swDevID)) {
            for (jj = 0; jj < 3; jj++ ) {
                rxStatsInfo.ctlAntStrength[jj][statsLoop].rxMinSigStrength = 127;
                rxStatsInfo.extAntStrength[jj][statsLoop].rxMinSigStrength = 127;
                rxStatsInfo.ctlAntStrength[jj][statsLoop].rxMaxSigStrength = -127;
                rxStatsInfo.extAntStrength[jj][statsLoop].rxMaxSigStrength = -127;
            }
        }
    }

    if(remoteStats & SKIP_SOME_STATS) {
        //extract and set number to skip
        skipStats = 1;
        numStatsToSkip = (A_UINT16)((remoteStats & NUM_TO_SKIP_M) >> NUM_TO_SKIP_S);
    }

    if(pLibDev->rx.enablePPM) {
        for( i = 0; i < MAX_TX_QUEUE; i++ )
        {
            for(statsLoop = 0; statsLoop < STATS_BINS; statsLoop++) {
                pLibDev->rx.rxStats[i][statsLoop].ppmMax = -1000;
                pLibDev->rx.rxStats[i][statsLoop].ppmMin = 1000;
            }
        }
    }

    rxStatsInfo.descToRead = pLibDev->rx.descAddress;

    if (enableCompare) {
        //create a max size compare buffer for easy compare
        rxStatsInfo.pCompareData = (A_UCHAR *)malloc(FRAME_BODY_SIZE);

        if(!rxStatsInfo.pCompareData) {
            mError(devNum, ENOMEM, "Device Number %d:txrxDataBegin: Unable to allocate memory for compare buffer\n", devNum);
            ar5kInitData[pLibDev->ar5kInitIndex].pMacAPI->rxCleanupConfig(devNum);
            return;
        }

        fillCompareBuffer(rxStatsInfo.pCompareData, FRAME_BODY_SIZE, dataPattern, dataPatternLength);
    }

    //start transmit
    ar5kInitData[pLibDev->ar5kInitIndex].pMacAPI->txBeginConfig(devNum, 1);
    start= milliTime();

    startTime = milliTime();
    do {
        if (!txComplete) {
            event = pLibDev->devMap.getISREvent(devNum);
        }

        //check for transmit
        if ((event.valid) &&
            (ar5kInitData[pLibDev->ar5kInitIndex].pMacAPI->isTxdescEvent(event.ISRValue)) && !txComplete)
        {
            finish= milliTime();

#ifdef DEBUG_MEMORY
printf("SNOOP::txrxDataBegin::Got txdesc event\n");
printf("SNOOP::Status of frames sent \n");
printf("SNOOP::txrxDataBegin::TXDP=%x:TXE=%x\n", REGR(devNum, 0x800), REGR(devNum, 0x840));
printf("SNOOP::txrxDataBegin::Memory at TXDP %x location is \n", REGR(devNum, 0x800));
memDisplay(devNum, REGR(devNum, 0x800), sizeof(MDK_ATHEROS_DESC)/sizeof(A_UINT32));
printf("SNOOP::txrxDataBegin::Frame contents at %x pointed location is \n", REGR(devNum, 0x800));
pLibDev->devMap.OSmemRead(devNum, REGR(devNum, 0x800)+4, (A_UCHAR*)&buf_ptr, sizeof(buf_ptr));
memDisplay(devNum, buf_ptr, 30);
printf("SNOOP::txrxDataBegin::Memory at baseaddress %x location is \n", pLibDev->tx[0].descAddress);
memDisplay(devNum, pLibDev->tx[0].descAddress, sizeof(MDK_ATHEROS_DESC)/sizeof(A_UINT32));
printf("SNOOP::txrxDataBegin::Frame contents at %x pointed location is \n", pLibDev->tx[0].descAddress);
pLibDev->devMap.OSmemRead(devNum, (pLibDev->tx[0].descAddress+4), (A_UCHAR*)&buf_ptr, sizeof(buf_ptr));
memDisplay(devNum, buf_ptr, 30);
printf("SNOOP::Register contents are\n");
printf("reg8000=%x:reg8004=%x:reg8008=%x:reg800c=%x:reg8014=%x\n", REGR(devNum, 0x8000), REGR(devNum, 0x8004), REGR(devNum, 0x8008), REGR(devNum, 0x800c), REGR(devNum, 0x8014));
#endif

            //ar5kInitData[pLibDev->ar5kInitIndex].pMacAPI->sendTxEndPacket(devNum, queueIndex );
            sendEndPacket(devNum, queueIndex);

            if(pLibDev->libCfgParams.enableXR) {
                REGW(devNum, XRMODE_REG, REGR(devNum, XRMODE_REG) | (0x1 << BITS_TO_XR_WAIT_FOR_POLL));
            }

            txComplete = TRUE;
            txAccumulateStats(devNum, finish - start, queueIndex);
        }

		if(txComplete) {
			mSleep(100);
			fillRxDescAndFrame(devNum, &rxStatsInfo);
		}

        if (rxStatsInfo.status2 & DESC_DONE) {
            if (isOwl(pLibDev->swDevID)) {
                A_UINT32 tempRateIndex;

                tempRateIndex = *( (A_UINT32 *)((A_UCHAR *)(&(rxStatsInfo.desc)) + 28) );
                ht40 = (A_BOOL)((tempRateIndex >> 1) & 0x1);
                if (isOwl_2_Plus(pLibDev->macRev)) {
                    tempRateIndex = *( (A_UINT32 *)((A_UCHAR *)(&(rxStatsInfo.desc)) + 16) );
                    rxStatsInfo.descRate = (tempRateIndex >> 24) & pLibDev->rxDataRateMsk;
                } else {
                    rxStatsInfo.descRate = (tempRateIndex >> 2) & pLibDev->rxDataRateMsk;
                }
            } else {
                rxStatsInfo.descRate = ((rxStatsInfo.status1 >> BITS_TO_RX_DATA_RATE) & pLibDev->rxDataRateMsk);
            }
            rxStatsInfo.descRate = descRate2RateIndex(rxStatsInfo.descRate, ht40);
            if (falconTrue) {
                rxStatsInfo.bufferAddr &= FALCON_DESC_ADDR_MASK;
            }

            // Initialize loop variables
            rxStatsInfo.badPacket = 0;
            rxStatsInfo.gotHeaderInfo = FALSE; // TODO: FIX for multi buffer packet support
            rxStatsInfo.illegalBuffSize = 0;
            rxStatsInfo.controlFrameReceived = 0;
            rxStatsInfo.mdkPktType = 0;
            //reset our timeout counter
            i = 0;

            // Increase buffer address for PPM
            if(pLibDev->rx.enablePPM) {
                rxStatsInfo.bufferAddr += ppm_data_padding;
            }

            if (numDesc == pLibDev->rx.numDesc - 1) {
                //This is the final looped rx desc and done bit is set, we ran out of receive descriptors,
                //so return with an error
                mError(devNum, ENOMEM, "Device Number %d:txrxDataBegin:  ran out of receive descriptors\n", devNum);
                break;
            }
            mdkExtractAddrAndSequence(devNum, &rxStatsInfo);

#ifdef DEBUG_MEMORY
printf("SNOOP::rxStatsInfo structure\n");
printf("SNOOP::descToRead=%X\n", rxStatsInfo.descToRead);
printf("SNOOP::descRate=%X\n", rxStatsInfo.descRate);
printf("SNOOP::status1=%X\n", rxStatsInfo.status1);
printf("SNOOP::status2=%X\n", rxStatsInfo.status2);
printf("SNOOP::bufferAddr=%X\n", rxStatsInfo.bufferAddr);
printf("SNOOP::totalBytes=%X\n", rxStatsInfo.totalBytes);
printf("SNOOP::mdkPktType=%X\n", rxStatsInfo.mdkPktType);
printf("SNOOP::frame_info_len=%X\n", rxStatsInfo.frame_info_len);
printf("SNOOP::ppm_data_len=%X\n", rxStatsInfo.ppm_data_len);
printf("SNOOP::RX desc contents of %X\n", pLibDev->rx.descAddress);
memDisplay(devNum, pLibDev->rx.descAddress, sizeof(MDK_ATHEROS_DESC)/sizeof(A_UINT32));
pLibDev->devMap.OSmemRead(devNum, (pLibDev->rx.descAddress+4), (A_UCHAR*)&buf_ptr, sizeof(buf_ptr));
printf("SNOOP::RX desc pointed frame contents at %X\n", buf_ptr);
memDisplay(devNum, buf_ptr, 30);
#endif

            //only process packets if things are good
            if ( !((rxStatsInfo.status1 & DESC_MORE) || rxStatsInfo.badPacket || rxStatsInfo.controlFrameReceived ||
                rxStatsInfo.illegalBuffSize) ) {

                //check for this being "last packet" or stats packet
                //mdkExtractAddrAndSequence also pulled mdkPkt type info from packet
                if ((rxStatsInfo.status2 & DESC_FRM_RCV_OK)
                    && (rxStatsInfo.mdkPktType == MDK_LAST_PKT))
                {
                    lastPktReceived = TRUE;

                    //if were not expecting remote stats then we are done
                    if (!(remoteStats & ENABLE_STATS_RECEIVE)) {
                        rxComplete = TRUE;
                    }

                    //we have done with receive so can send stats
                    if ((remoteStats & ENABLE_STATS_SEND) && txComplete) {
                        for (i = 0; i < 1 /*MAX_TX_QUEUE*/; i++ ) {
                            for (statsLoop = 1; statsLoop < STATS_BINS; statsLoop++) {
                                // Need to check if we have transmit OR receive stats for this rate
                                if ((pLibDev->rx.rxStats[i][statsLoop].crcPackets > 0) ||
                                    (pLibDev->rx.rxStats[i][statsLoop].goodPackets > 0) ||
                                    (pLibDev->tx[queueIndex].txStats[statsLoop].excessiveRetries > 0) ||
                                    (pLibDev->tx[queueIndex].txStats[statsLoop].goodPackets > 0))
                                {
                                    sendStatsPkt(devNum, statsLoop, MDK_TXRX_STATS_PKT, pLibDev->tx[queueIndex].destAddr.octets);
                                }
                            }
                            sendStatsPkt(devNum, 0, MDK_TXRX_STATS_PKT, pLibDev->tx[queueIndex].destAddr.octets);
                            statsSent = TRUE;
                        }
                    }
                } else if ((rxStatsInfo.status2 & DESC_FRM_RCV_OK)
                     && (rxStatsInfo.mdkPktType >= MDK_TX_STATS_PKT)
                     && (rxStatsInfo.mdkPktType <= MDK_TXRX_STATS_PKT))
                {
                    statsPktReceived = mdkExtractRemoteStats(devNum, &rxStatsInfo);
                    rxComplete = statsPktReceived;
                } else if (rxStatsInfo.mdkPktType == MDK_NORMAL_PKT){
                    if (enableCompare) {
                        comparePktData(devNum, &rxStatsInfo);
                    }
                    if (pLibDev->rx.enablePPM) {
                        extractPPM(devNum, &rxStatsInfo);
                    }

                    if (skipStats && (numStatsToSkip != 0)) {
                        numStatsToSkip --;
                    } else {
                        mdkExtractRxStats(devNum, &rxStatsInfo);
                    }

                } else {
                    if (rxStatsInfo.status2 & DESC_FRM_RCV_OK) {
                        mError(devNum, EIO, "Device Number %d:A good matching packet with an unknown MDK_PKT type detected MDK_PKT: %d\n", devNum,
                            rxStatsInfo.mdkPktType);
                        {
//                            A_UINT32 iIndex;
//                            printf("Frame Info\n");
//                            for(iIndex=0; iIndex<rxStatsInfo.frame_info_len; iIndex++) {
//                                printf("%X ", rxStatsInfo.frame_info[iIndex]);
//                            }
//                            printf("\n");
//                            printf("PPM Data Info\n");
//                            for(iIndex=0; iIndex<rxStatsInfo.ppm_data_len; iIndex++) {
//                                printf("%X ", rxStatsInfo.ppm_data[iIndex]);
//                            }
//                            printf("\n");
                        }
                    }
                    if (rxStatsInfo.status2 & DESC_CRC_ERR) {
                        pLibDev->rx.rxStats[rxStatsInfo.qcuIndex][0].crcPackets++;
                        pLibDev->rx.rxStats[rxStatsInfo.qcuIndex][rxStatsInfo.descRate].crcPackets++;
                        pLibDev->rx.haveStats = 1;
                    } else if (rxStatsInfo.status2 & pLibDev->decryptErrMsk) {
                        pLibDev->rx.rxStats[rxStatsInfo.qcuIndex][0].decrypErrors++;
                        pLibDev->rx.rxStats[rxStatsInfo.qcuIndex][rxStatsInfo.descRate].decrypErrors++;
                        pLibDev->rx.haveStats = 1;
                    }

                }
            }

            if(txComplete && rxComplete) {
                break;
            }

            //get next descriptor to process
            rxStatsInfo.descToRead += sizeof(MDK_ATHEROS_DESC);
            numDesc ++;
        } else {
            curTime = milliTime();
            if (curTime > (startTime+timeout)) {
                i = timeout;
                break;
            }
            mSleep(1);
            i++;
        }
    } while (i < timeout);

    if (i == timeout) {
        if (!txComplete) {
            mError(devNum, EIO, "Device Number %d:txrxDataBegin: timeout reached before all frames transmitted\n", devNum);
        } else {
            mError(devNum, EIO,
            "Device Number %d:txrxDataBegin: timeout reached, without receiving all packets.  Number received = %lu\n", devNum,
            numDesc);

            if (!lastPktReceived) {
                mError(devNum, EIO,
                "Device Number %d:txrxDataBegin: timeout reached, without receiving last packet\n", devNum);
            }

            if (!statsPktReceived && (remoteStats == ENABLE_STATS_RECEIVE)) {
                mError(devNum, EIO,
                "Device Number %d:txrxDataBegin: timeout reached, without receiving stats packet\n", devNum);
            }

        }
    }

    if ((remoteStats & ENABLE_STATS_SEND) && !statsSent) {
        for ( i = 0; i < MAX_TX_QUEUE; i++ ) {
            for (statsLoop = 1; statsLoop < STATS_BINS; statsLoop++) {
                // Need to check if we have transmit OR receive stats for this rate
                if ((pLibDev->rx.rxStats[i][statsLoop].crcPackets > 0) ||
                    (pLibDev->rx.rxStats[i][statsLoop].goodPackets > 0) ||
                    (pLibDev->tx[queueIndex].txStats[statsLoop].excessiveRetries > 0) ||
                    (pLibDev->tx[queueIndex].txStats[statsLoop].goodPackets > 0))
                {
                    sendStatsPkt(devNum, statsLoop, MDK_TXRX_STATS_PKT, pLibDev->tx[queueIndex].destAddr.octets);
                }
            }
            sendStatsPkt(devNum, 0, MDK_TXRX_STATS_PKT, pLibDev->tx[queueIndex].destAddr.octets);
        }
    }

    //cleanup
    ar5kInitData[pLibDev->ar5kInitIndex].pMacAPI->txCleanupConfig(devNum);
    ar5kInitData[pLibDev->ar5kInitIndex].pMacAPI->rxCleanupConfig(devNum);

    //write back the retry value
    ar5kInitData[pLibDev->ar5kInitIndex].pMacAPI->setRetryLimit( devNum, 0 );   //__TODO__

    if(enableCompare) {
        free(rxStatsInfo.pCompareData);
    }

    return;
}

/**************************************************************************
* txGetStats - Read and return the tx stats values
*
* remote: set to 1 if want to get stats that were sent from a remote stn
*/
MANLIB_API void txGetStats
(
 A_UINT32 devNum,
 A_UINT32 rateInMb,
 A_UINT32 remote,
 TX_STATS_STRUCT *pReturnStats
)
{
    LIB_DEV_INFO *pLibDev = gLibInfo.pLibDevArray[devNum];
    A_UINT16    queueIndex = pLibDev->selQueueIndex;
    A_UINT32  rateBinIndex = rate2bin(rateInMb);

    memset(pReturnStats, 0, sizeof(TX_STATS_STRUCT));

    if (checkDevNum(devNum) == FALSE) {
        mError(devNum, EINVAL, "Device Number %d:txGetStats\n", devNum);
        return;
    }
    if (remote) {
        //check to see if we received remote stats
        if ((pLibDev->remoteStats == MDK_TX_STATS_PKT) ||
            (pLibDev->remoteStats == MDK_TXRX_STATS_PKT))
        {
            memcpy(pReturnStats, &(pLibDev->txRemoteStats[rateBinIndex]), sizeof(TX_STATS_STRUCT));
        } else {
            mError(devNum, EIO, "Device Number %d:txGetStats: no remote stats were received\n", devNum);
            return;
        }
    } else {

        if(pLibDev->tx[queueIndex].haveStats) {
            memcpy(pReturnStats, &(pLibDev->tx[queueIndex].txStats[rateBinIndex]), sizeof(TX_STATS_STRUCT));
        } else {
            mError(devNum, EIO, "Device Number %d:txGetStats: no tx stats have been collected\n", devNum);
            return;
        }

    }
    return;
}

/**************************************************************************
* rxGetStats - Read and return the rx stats values
*
* remote: set to 1 if want to get stats that were sent from a remote stn
*/
MANLIB_API void rxGetStats
(
 A_UINT32 devNum,
 A_UINT32 rateInMb,
 A_UINT32 remote,
 RX_STATS_STRUCT *pReturnStats
)
{
    LIB_DEV_INFO *pLibDev = gLibInfo.pLibDevArray[devNum];
    A_UINT32 rateBinIndex = rate2bin(rateInMb);

    //work around for venice to get to the correct CCK rate bins
    if(((pLibDev->swDevID & 0xff) >= 0x0013) && (pLibDev->mode == MODE_11B)){
        if((rateBinIndex > 0) && (rateBinIndex < 9)) {
            if(rateBinIndex == 1){
                rateBinIndex += 8;
            }
            else {
                rateBinIndex += 7;
            }
        }
    }

    memset(pReturnStats, 0, sizeof(RX_STATS_STRUCT));

	// copy rx tput into right bin index
	pLibDev->rx.rxStats[0][rateBinIndex].rxThroughPut = pLibDev->rx.rxStats[0][0].rxThroughPut;

	// copy evm average
	pLibDev->rx.rxStats[0][rateBinIndex].evm_stream0 = pLibDev->rx.rxStats[0][0].evm_stream0;
	pLibDev->rx.rxStats[0][rateBinIndex].evm_stream1 = pLibDev->rx.rxStats[0][0].evm_stream1;
    if (checkDevNum(devNum) == FALSE) {
        mError(devNum, EINVAL, "Device Number %d:rxGetStats\n", devNum);
        return;
    }
    if ( /*(rateInMb > 54) ||*/ ((rateBinIndex == 0) && (rateInMb)) ) {
        mError(devNum, EINVAL, "Device Number %d:rxGetStats: Invalid rateInMb value %d - use 0 for all rates\n", devNum,
            rateInMb);
        return;
    }

    if (remote) {
        //check to see if we received remote stats
        if ((pLibDev->remoteStats == MDK_RX_STATS_PKT) ||
            (pLibDev->remoteStats == MDK_TXRX_STATS_PKT)) {
            memcpy(pReturnStats, &(pLibDev->rxRemoteStats[0][rateBinIndex]), sizeof(RX_STATS_STRUCT));
        }
        else {
            mError(devNum, EIO, "Device Number %d:rxGetStats: no remote stats were received\n", devNum);
            return;
        }
    } else {
        if(pLibDev->rx.haveStats) { // __TODO__
            memcpy(pReturnStats, &(pLibDev->rx.rxStats[0][rateBinIndex]), sizeof(RX_STATS_STRUCT));
        }
        else {
            mError(devNum, EIO, "Device Number %d:rxGetStats: no rx stats have been collected\n", devNum);
            return;
        }
    }
    return;
}

/**************************************************************************
* rxGetData - Get data from a particular descriptor
*
*
*/
MANLIB_API void rxGetData
(
 A_UINT32 devNum,
 A_UINT32 bufferNum,
 A_UCHAR *pReturnBuffer,
 A_UINT32 sizeBuffer
)
{
    LIB_DEV_INFO *pLibDev = gLibInfo.pLibDevArray[devNum];
    A_UINT32 readSize, readAddr;
    A_UINT32 status;
    A_UINT32 descAddr;
    A_UINT32 status2_offset;

    if (isOwl(pLibDev->swDevID)) {
        status2_offset = SECOND_OWL_RX_STATUS_WORD;
    } else if (isFalcon(devNum) || isDragon(devNum)) {
        status2_offset = SECOND_FALCON_RX_STATUS_WORD;
    } else {
        status2_offset = SECOND_STATUS_WORD;
    }

    if (checkDevNum(devNum) == FALSE) {
        mError(devNum, EINVAL, "Device Number %d:rxGetData\n", devNum);
        return;
    }
    if(pLibDev->devState < RESET_STATE) {
        mError(devNum, EILSEQ, "Device Number %d:rxGetData: device not out of reset state - resetDevice must be run first\n", devNum);
    return;
    }
    if(pLibDev->rx.bufferAddress == 0) {
        mError(devNum, EILSEQ, "Device Number %d:rxGetData: receive buffer is empty - no valid data to read\n", devNum);
        return;
    }
    if(bufferNum > pLibDev->rx.numDesc) {
        mError(devNum, EINVAL, "Device Number %d:rxGetData: Asking for a buffer beyond the number setup for receive\n", devNum);
        return;
    }

    readSize = A_MIN(pLibDev->rx.bufferSize, sizeBuffer);
    readAddr = pLibDev->rx.bufferAddress + (pLibDev->rx.bufferSize * bufferNum);
    //read only if the descriptor is good, otherwise, move onto next descriptor
    descAddr = pLibDev->rx.descAddress + (sizeof(MDK_ATHEROS_DESC) * bufferNum);
    while (bufferNum < pLibDev->rx.numDesc) {
        pLibDev->devMap.OSmemRead(devNum, descAddr + status2_offset,
                    (A_UCHAR *)&status, sizeof(status));

        if (status & DESC_DONE) {
            if(status & DESC_FRM_RCV_OK) {
                pLibDev->devMap.OSmemRead(devNum, readAddr, pReturnBuffer, readSize);
                return;
            }
        } else {
            //found a descriptor that's not done, assume all others are same
            mError(devNum, EIO, "Device Number %d:rxGetData: Found descriptor not done before valid receive descriptor", devNum);
            return;
        }
        descAddr += sizeof(MDK_ATHEROS_DESC);
        readAddr += pLibDev->rx.bufferSize;
        bufferNum++;
    }
    // if got to here then ran out of descriptors
    mError(devNum, EIO, "Device Number %d:rxGetData: Unable to find a descriptor with RX_OK\n", devNum);
}

/********************************************************************************************
* Function name: iq_calibration
* Description: this function outputs IQ calibration data
*
*/
/*
MANLIB_API void iq_calibration(A_UINT32 devNum, IQ_FACTOR *iq_coeff)
{
    LIB_DEV_INFO *pLibDev = gLibInfo.pLibDevArray[devNum];
	A_UINT32 rddata, wrdata;
	A_UINT32 pwr_meas_i, pwr_meas_q, iq_corr_meas;
	
	int i_coff, q_coff, iqcorr_neg;

	if(!isSowl(pLibDev->macRev)) {
		printf("Don't need IQ calibration for current device\n");
		return;
	}

	printf("Connect another station in contTX mode in all 3 chains\n");

	// force chain mask to 7x7
	if(pLibDev->libCfgParams.rx_chain_mask < 0x7)
	{
		printf("Force rx chain mask to 0x7 and tx chain mask to 0x7\n");
		printf("Try again\n");
		return;
	}

    REGW(devNum, 0x99f0, 0x0);      // enable iq cal

    rddata = REGR(devNum, 0x9920);
    wrdata = (rddata & 0xffff0fff) | (7 << 12) | (0x1 << 16);
	REGW(devNum, 0x9920, wrdata);         

    rddata = (REGR(devNum, 0x9920) >> 16) & 0x1;
	while (rddata == 0x1) {
       rddata = (REGR(devNum, 0x9920) >> 16) & 0x1;
	}

	pwr_meas_i   = REGR(devNum, 0x9c10) & 0xffffffff;
	pwr_meas_q   = REGR(devNum, 0x9c14) & 0xffffffff;
	iq_corr_meas = REGR(devNum, 0x9c18) & 0xffffffff;
    //printf("Ch0 iq_corr_meas = 0x%08x\n", iq_corr_meas);
    //printf("Ch0 pwr_meas_i = 0x%08x\n", pwr_meas_i);
    //printf("Ch0 pwr_meas_q = 0x%08x\n", pwr_meas_q);

	iqcorr_neg = 0;
	if (iq_corr_meas > 0x80000000) {
	   iq_corr_meas = (0xffffffff - iq_corr_meas) + 1;
	   iqcorr_neg = 1;
	}

	i_coff = (int)((((float)iq_corr_meas)/(((float)pwr_meas_i+(float)pwr_meas_q)/2.0))*128);
	q_coff = (int)((sqrt((float)pwr_meas_i/((float)pwr_meas_q)) - 1)*128);

	// Negate i_coff if iqcorr_neg == 0
	i_coff = i_coff & 0x3f;
	if (iqcorr_neg == 0x0) {
		i_coff = 0x40 - i_coff;
	}

	if (i_coff == 0x40) {
		i_coff = 0x0;
	}

	if ((q_coff > 0xf) && (q_coff < 0x80000000)) {
		q_coff = 0xf;
	} else if (q_coff <= -16) {
		q_coff = 0x10;
	} else {
		q_coff = q_coff & 0x1f;
	}

    iq_coeff->i0 = i_coff;
    iq_coeff->q0 = q_coff;

	printf("Chn0 : Icoff = %x  Qcoff = %x\n", i_coff, q_coff);

    writeField(devNum, "bb_ch0_iqcorr_q_i_coff", i_coff);
    writeField(devNum, "bb_ch0_iqcorr_q_q_coff", q_coff);
    writeField(devNum, "bb_ch0_iqcorr_enable", 1);

    pwr_meas_i   = REGR(devNum, 0xac10) & 0xffffffff;
    pwr_meas_q   = REGR(devNum, 0xac14) & 0xffffffff;
    iq_corr_meas = REGR(devNum, 0xac18) & 0xffffffff;
    //printf("Ch1 iq_corr_meas = 0x%08x\n", iq_corr_meas);
    //printf("Ch1 pwr_meas_i = 0x%08x\n", pwr_meas_i);
    //printf("Ch1 pwr_meas_q = 0x%08x\n", pwr_meas_q);

    iqcorr_neg = 0;
	if (iq_corr_meas > 0x80000000) {
	   iq_corr_meas = (0xffffffff - iq_corr_meas) + 1;
	   iqcorr_neg = 1;
	}

    i_coff = (int)(((((float)iq_corr_meas))/(((float)pwr_meas_i+(float)pwr_meas_q)/2))*128);
    q_coff = (int)((sqrt((float)pwr_meas_i/((float)pwr_meas_q)) - 1)*128);
   
    i_coff = i_coff & 0x3f;
    if (iqcorr_neg == 0x0) {
       i_coff = 0x40 - i_coff;
    }   
   
    if (i_coff == 0x40) {
      i_coff = 0x0;
    }   
    if ((q_coff > 0xf) && (q_coff < 0x80000000)) {
       q_coff = 0xf;
    } else if (q_coff <= -16) {
       q_coff = 0x10;
    } else {
       q_coff = q_coff & 0x1f;
    }   

    iq_coeff->i1 = i_coff;
    iq_coeff->q1 = q_coff;

    printf("Chn1 : Icoff = %x  Qcoff = %x\n", i_coff, q_coff);
    writeField(devNum, "bb_ch1_iqcorr_q_i_coff", i_coff);
    writeField(devNum, "bb_ch1_iqcorr_q_q_coff", q_coff);
    writeField(devNum, "bb_ch1_iqcorr_enable", 1);

    pwr_meas_i   = REGR(devNum, 0xbc10) & 0xffffffff;
    pwr_meas_q   = REGR(devNum, 0xbc14) & 0xffffffff;
    iq_corr_meas = REGR(devNum, 0xbc18) & 0xffffffff;
    //printf("Ch2 iq_corr_meas = 0x%08x\n", iq_corr_meas);
    //printf("Ch2 pwr_meas_i = 0x%08x\n", pwr_meas_i);
    //printf("Ch2 pwr_meas_q = 0x%08x\n", pwr_meas_q);
    
    iqcorr_neg = 0;

    if (iq_corr_meas > 0x80000000) {
      iq_corr_meas = (0xffffffff - iq_corr_meas) + 1;
	  iqcorr_neg = 1;
    }

    i_coff = (int)(((((float)iq_corr_meas))/(((float)pwr_meas_i+(float)pwr_meas_q)/2))*128);
    q_coff = (int)((sqrt((float)pwr_meas_i/((float)pwr_meas_q)) - 1)*128);
   
    i_coff = i_coff & 0x3f;
    if (iqcorr_neg == 0x0) {
       i_coff = 0x40 - i_coff;
    }
   
    if (i_coff == 0x40) {
      i_coff = 0x0;
    }
    if ((q_coff > 0xf) && (q_coff < 0x80000000)) {
       q_coff = 0xf;
    } else if (q_coff <= -16) {
       q_coff = 0x10;
    } else {
       q_coff = q_coff & 0x1f;
    }

    iq_coeff->i2 = i_coff;
    iq_coeff->q2 = q_coff;
	
    printf("Chn2 : Icoff = %x  Qcoff = %x\n", i_coff, q_coff);
   
	writeField(devNum, "bb_ch2_iqcorr_q_i_coff", i_coff);
    writeField(devNum, "bb_ch2_iqcorr_q_q_coff", q_coff);
    writeField(devNum, "bb_ch2_iqcorr_enable", 1);

	return;
}
*/

/********************************************************************************************
* Function name: iq_calibration
* Description: this function outputs IQ calibration data
*
*/

#define TOTAL_SAMPLES 64
#define IQ_CAL_NUM_SAMPLES 0x2

MANLIB_API void iq_calibration(A_UINT32 devNum, IQ_FACTOR *iq_coeff)
{
    LIB_DEV_INFO *pLibDev = gLibInfo.pLibDevArray[devNum];
	A_UINT32 rddata, wrdata;
	A_UINT32 pwr_meas_i_chn0, pwr_meas_q_chn0, iq_corr_meas_chn0;
	A_UINT32 pwr_meas_i_chn1, pwr_meas_q_chn1, iq_corr_meas_chn1;
	A_UINT32 pwr_meas_i_chn2, pwr_meas_q_chn2, iq_corr_meas_chn2;
	A_UINT32 total_pwr_meas_i_chn0 = 0;
	A_UINT32 total_pwr_meas_q_chn0 = 0;
	A_UINT32 total_pwr_meas_i_chn1 = 0;
	A_UINT32 total_pwr_meas_q_chn1 = 0;
	A_UINT32 total_pwr_meas_i_chn2 = 0;
	A_UINT32 total_pwr_meas_q_chn2 = 0;
	A_INT32 total_iq_corr_meas_chn0 = 0;
	A_INT32 total_iq_corr_meas_chn1 = 0;
	A_INT32 total_iq_corr_meas_chn2 = 0;


	int i_coff, q_coff, iqcorr_neg;
	int i;

	if(!isSowl(pLibDev->macRev)) {
		printf("Don't need IQ calibration for current device\n");
        iq_coeff->i0 = 0;
        iq_coeff->q0 = 0;
        iq_coeff->i1 = 0;
        iq_coeff->q1 = 0;
        iq_coeff->i2 = 0;
        iq_coeff->q2 = 0;

		return;
	}

	printf("Connect another station in contTX mode in all 3 chains\n");

	// force chain mask to 7x7
	if(pLibDev->libCfgParams.rx_chain_mask < 0x7)
	{
		printf("Force rx chain mask to 0x7 and tx chain mask to 0x7\n");
		printf("Try again\n");
		return;
	}

	for(i=0; i<TOTAL_SAMPLES; i++)
	{
		rddata = REGR(devNum, 0x9920);
		wrdata = (rddata & 0xffff0fff) | (IQ_CAL_NUM_SAMPLES << 12);
		REGW(devNum, 0x9920, wrdata);

	    rddata = REGR(devNum, 0x99f0);
		wrdata = rddata & 0xfffffffc;    // set gain_dc_iq_cal_mode
        REGW(devNum, 0x99f0, wrdata);

	    rddata = REGR(devNum, 0x9920);
		wrdata = rddata | (0x1 << 16);
		REGW(devNum, 0x9920, wrdata);	 // start IQ Cal

        rddata = (REGR(devNum, 0x9920) >> 16) & 0x1;
	    while (rddata == 0x1) {
          rddata = (REGR(devNum, 0x9920) >> 16) & 0x1;
		}

		pwr_meas_i_chn0   = REGR(devNum, 0x9c10) & 0xffffffff;
	    pwr_meas_q_chn0   = REGR(devNum, 0x9c14) & 0xffffffff;
		iq_corr_meas_chn0 = REGR(devNum, 0x9c18) & 0xffffffff;

		total_pwr_meas_i_chn0   = total_pwr_meas_i_chn0 + pwr_meas_i_chn0;
		total_pwr_meas_q_chn0   = total_pwr_meas_q_chn0 + pwr_meas_q_chn0;

	    if (iq_corr_meas_chn0 > 0x80000000) {
		   iq_corr_meas_chn0 = (0xffffffff - iq_corr_meas_chn0) + 1;
		   total_iq_corr_meas_chn0 = total_iq_corr_meas_chn0 - iq_corr_meas_chn0;
		} else {
			total_iq_corr_meas_chn0 = total_iq_corr_meas_chn0 + iq_corr_meas_chn0;
		}

        pwr_meas_i_chn1   = REGR(devNum, 0xac10) & 0xffffffff;
        pwr_meas_q_chn1   = REGR(devNum, 0xac14) & 0xffffffff;
        iq_corr_meas_chn1 = REGR(devNum, 0xac18) & 0xffffffff;

        total_pwr_meas_i_chn1   = total_pwr_meas_i_chn1 + pwr_meas_i_chn1;
        total_pwr_meas_q_chn1   = total_pwr_meas_q_chn1 + pwr_meas_q_chn1;
        if (iq_corr_meas_chn1 > 0x80000000) {
          iq_corr_meas_chn1 = (0xffffffff - iq_corr_meas_chn1) + 1;
          total_iq_corr_meas_chn1 = total_iq_corr_meas_chn1 - iq_corr_meas_chn1;
        } else {
            total_iq_corr_meas_chn1 = total_iq_corr_meas_chn1 + iq_corr_meas_chn1;
        }

        pwr_meas_i_chn2   = REGR(devNum, 0xbc10) & 0xffffffff;
        pwr_meas_q_chn2   = REGR(devNum, 0xbc14) & 0xffffffff;
        iq_corr_meas_chn2 = REGR(devNum, 0xbc18) & 0xffffffff;

        total_pwr_meas_i_chn2   = total_pwr_meas_i_chn2 + pwr_meas_i_chn2;
        total_pwr_meas_q_chn2   = total_pwr_meas_q_chn2 + pwr_meas_q_chn2;
        if (iq_corr_meas_chn2 > 0x80000000) {
          iq_corr_meas_chn2 = (0xffffffff - iq_corr_meas_chn2) + 1;
          total_iq_corr_meas_chn2 = total_iq_corr_meas_chn2 - iq_corr_meas_chn2;
        } else {
            total_iq_corr_meas_chn2 = total_iq_corr_meas_chn2 + iq_corr_meas_chn2;
        }
	}

	if (total_iq_corr_meas_chn0 > 0) {
		iqcorr_neg = 0;
	} else {
		total_iq_corr_meas_chn0 = -total_iq_corr_meas_chn0;
		iqcorr_neg = 1;
	}

	i_coff = (int)(((float)total_iq_corr_meas_chn0/(((float)total_pwr_meas_i_chn0+(float)total_pwr_meas_q_chn0)/2.0))*128);
	q_coff = (int)((sqrt((float)total_pwr_meas_i_chn0/(float)total_pwr_meas_q_chn0) - 1)*128);

	// Negate i_coff if iqcorr_neg == 0
	i_coff = i_coff & 0x3f;
	if (iqcorr_neg == 0x0) {
		i_coff = 0x40 - i_coff;
	}

	if (i_coff == 0x40) {
		i_coff = 0x0;
	}

    if ((q_coff > 0xf) && (q_coff < 0x80000000)) {
		q_coff = 0xf;
	} else if (q_coff <= -16) {
		q_coff = 0x10;
	} else {
		q_coff = q_coff & 0x1f;
	}

    iq_coeff->i0 = i_coff;
    iq_coeff->q0 = q_coff;

	printf("Chn0 : Icoff = %x  Qcoff = %x\n", i_coff, q_coff);

//  rddata = reg_read(0x9920);
//  wrdata = $rddata | (1 << 11) | (i_coff << 5) | q_coff;  # Write the I and Q correction for chain0
//  reg_write(0x9920, $wrdata);

    writeField(devNum, "bb_ch0_iqcorr_q_i_coff", i_coff);
    writeField(devNum, "bb_ch0_iqcorr_q_q_coff", q_coff);
    writeField(devNum, "bb_ch0_iqcorr_enable", 1);

    if (total_iq_corr_meas_chn1 > 0) {
      iqcorr_neg = 0;
    } else {
        total_iq_corr_meas_chn1 = -total_iq_corr_meas_chn1;
        iqcorr_neg = 1;
    }

    i_coff = (int)(((float)total_iq_corr_meas_chn1/(((float)total_pwr_meas_i_chn1+(float)total_pwr_meas_q_chn1)/2.0))*128);
    q_coff = (int)((sqrt((float)total_pwr_meas_i_chn1/(float)total_pwr_meas_q_chn1) - 1)*128);

    i_coff = i_coff & 0x3f;
    if (iqcorr_neg == 0x0) {
       i_coff = 0x40 - i_coff;
    }

    if (i_coff == 0x40) {
      i_coff = 0x0;
    }
    if ((q_coff > 0xf) && (q_coff < 0x80000000)) {
       q_coff = 0xf;
    } else if (q_coff <= -16) {
       q_coff = 0x10;
    } else {
       q_coff = q_coff & 0x1f;
    }

    iq_coeff->i1 = i_coff;
    iq_coeff->q1 = q_coff;

    printf("Chn1 : Icoff = %x  Qcoff = %x\n", i_coff, q_coff);
//  rddata = reg_read(0xa920);
//  wrdata = rddata | (1 << 11) | (i_coff << 5) | q_coff;  # Write the I and Q correction for chain1
//  reg_write(0xa920, wrdata);

    writeField(devNum, "bb_ch1_iqcorr_q_i_coff", i_coff);
    writeField(devNum, "bb_ch1_iqcorr_q_q_coff", q_coff);
    writeField(devNum, "bb_ch1_iqcorr_enable", 1);

    if (total_iq_corr_meas_chn2 > 0) {
      iqcorr_neg = 0;
    } else {
        total_iq_corr_meas_chn2 = -total_iq_corr_meas_chn2;
        iqcorr_neg = 1;
    }

    i_coff = (int)(((float)total_iq_corr_meas_chn2/(((float)total_pwr_meas_i_chn2+(float)total_pwr_meas_q_chn2)/2.0))*128);
    q_coff = (int)((sqrt((float)total_pwr_meas_i_chn2/(float)total_pwr_meas_q_chn2) - 1)*128);

    i_coff = i_coff & 0x3f;
    if (iqcorr_neg == 0x0) {
       i_coff = 0x40 - i_coff;
    }

    if (i_coff == 0x40) {
      i_coff = 0x0;
    }
    if ((q_coff > 0xf) && (q_coff < 0x80000000)) {
       q_coff = 0xf;
    } else if (q_coff <= -16) {
       q_coff = 0x10;
    } else {
       q_coff = q_coff & 0x1f;
    }

    iq_coeff->i2 = i_coff;
    iq_coeff->q2 = q_coff;

    printf("Chn2 : Icoff = %x  Qcoff = %x\n", i_coff, q_coff);

//   rddata = reg_read(0xb920);
//   wrdata = rddata | (1 << 11) | (i_coff << 5) | q_coff;  # Write the I and Q correction for chain2
//   reg_write(0xb920, wrdata);

	writeField(devNum, "bb_ch2_iqcorr_q_i_coff", i_coff);
    writeField(devNum, "bb_ch2_iqcorr_q_q_coff", q_coff);
    writeField(devNum, "bb_ch2_iqcorr_enable", 1);

	return;
}


/**************************************************************************
* createTransmitPacketAggr - create aggregrate packet for transmission
*
*/
void createTransmitPacketAggr
(
 A_UINT32 devNum,
 A_UINT16 mdkType,
 A_UCHAR *dest,
 A_UINT32 numDesc,
 A_UINT32 dataBodyLength,
 A_UCHAR *dataPattern,
 A_UINT32 dataPatternLength,
 A_UINT32 broadcast,
 A_UINT16 queueIndex,           // QCU index
 A_UINT32 aggSize,
 A_UINT32 *pPktSize,            //return size
 A_UINT32 *pPktAddr				//return address
)
{
   WLAN_QOS_DATA_MAC_HEADER3  *pPktHeader;
//    WLAN_DATA_MAC_HEADER3  *pPktHeader;
    LIB_DEV_INFO *pLibDev = gLibInfo.pLibDevArray[devNum];
    A_UINT32 amountToWrite;
    A_UCHAR broadcastAddr[] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
    MDK_PACKET_HEADER *pMdkHeader;
    A_UCHAR *pTransmitPkt;
    A_UCHAR *pPkt;
    A_UINT32 *pIV;
	A_UINT16 i;

    //allocate enough memory for the packet
    *pPktSize = dataBodyLength;

    if (!pLibDev->specialTx100Pkt) {
      *pPktSize += sizeof(WLAN_QOS_DATA_MAC_HEADER3) + 2 + sizeof(MDK_PACKET_HEADER); // add 2 bytes to end header in dword boundary ??
//        *pPktSize += sizeof(WLAN_DATA_MAC_HEADER3) + sizeof(MDK_PACKET_HEADER); // add 2 bytes to end header in dword boundary ??
        if(pLibDev->wepEnable && (mdkType == MDK_NORMAL_PKT)) {
            *pPktSize += (WEP_IV_FIELD + 2);
        }
    }

    *pPktAddr = memAlloc(devNum, (0x1000)*(aggSize));   // Allocate memory for aggregated buffer
    if (0 == *pPktAddr) {
        mError(devNum, ENOMEM, "Device Number %d:createTransmitPacket: unable to allocate memory for transmit buffer\n", devNum);
        return;
    }

    //allocate local memory to create the packet in, so only do one block write of packet
    pTransmitPkt = pPkt = (A_UCHAR *)malloc(*pPktSize);
    if(!pPkt) {
        mError(devNum, ENOMEM, "Device Number %d:Unable to allocate local memory for packet\n", devNum);
        return;
    }

  pPktHeader = (WLAN_QOS_DATA_MAC_HEADER3 *)pPkt;

    //create the packet Header, all fields are listed here to enable easy changing
    pPktHeader->frameControl.protoVer = 0;
    pPktHeader->frameControl.fType = FRAME_DATA;
	pPktHeader->frameControl.fSubtype = SUBT_QOS;
    pPktHeader->frameControl.ToDS = 0;
    pPktHeader->frameControl.FromDS = 0;
    pPktHeader->frameControl.moreFrag = 0;
    pPktHeader->frameControl.retry = 0;
    pPktHeader->frameControl.pwrMgt = 0;
    pPktHeader->frameControl.moreData = 0;
    pPktHeader->frameControl.wep = ((pLibDev->wepEnable) && (mdkType == MDK_NORMAL_PKT)) ? 1 : 0;
    pPktHeader->frameControl.order = 0;

// Swap the bytes of the frameControl
#if 0
// #ifdef ARCH_BIG_ENDIAN
    {
        A_UINT16* ptr;

        ptr = (A_UINT16 *)(&(pPktHeader->frameControl));
        *ptr = btol_s(*ptr);
    }
#endif
    pPktHeader->durationNav = 0;
// Swap the bytes of the durationNav
#if 0
//#ifdef ARCH_BIG_ENDIAN
    {
        A_UINT16* ptr;
        ptr = (A_UINT16 *)(&(pPktHeader->durationNav));
        *ptr = btol_s(*ptr);
    }
#endif
    memcpy(pPktHeader->address1.octets, broadcast ? broadcastAddr : dest, WLAN_MAC_ADDR_SIZE);
    memcpy(pPktHeader->address2.octets, pLibDev->macAddr.octets, WLAN_MAC_ADDR_SIZE);
    memcpy(pPktHeader->address3.octets, pLibDev->bssAddr.octets, WLAN_MAC_ADDR_SIZE);
//printf("SNOOP:: frame with\n");
//if (!broadcast) printf("SNOOP::dest %x:%x:%x:%x:%x:%x\n", dest[6], dest[5], dest[4], dest[3], dest[2], dest[1], dest[0]);
//printf("SNOOP::macaddr %x:%x:%x:%x:%x:%x\n", pLibDev->macAddr.octets[6], pLibDev->macAddr.octets[5], pLibDev->macAddr.octets[4], pLibDev->macAddr.octets[3], pLibDev->macAddr.octets[2], pLibDev->macAddr.octets[1], pLibDev->macAddr.octets[0]);
//printf("SNOOP::bssAddr %x:%x:%x:%x:%x:%x\n", pLibDev->bssAddr.octets[6], pLibDev->bssAddr.octets[5], pLibDev->bssAddr.octets[4], pLibDev->bssAddr.octets[3], pLibDev->bssAddr.octets[2], pLibDev->bssAddr.octets[1], pLibDev->bssAddr.octets[0]);
    WLAN_SET_FRAGNUM(pPktHeader->seqControl, 0);
    WLAN_SET_SEQNUM(pPktHeader->seqControl, 0xfed);

	// Add qos control field
// WLAN_QOS_DATA_MAC_HEADER3
	pPktHeader->qosControl.tid = 0;
	pPktHeader->qosControl.eosp = 0;
	pPktHeader->qosControl.ackpolicy = 0;
	pPktHeader->qosControl.txop = 0;


#if 0
//#ifdef ARCH_BIG_ENDIAN
    {
        A_UINT16* ptr;

        ptr = (A_UINT16 *)(&(pPktHeader->seqControl));
        *ptr = btol_s(*ptr);
    }
#endif

    //fill in the packet body, if special Tx100 packet, don't move on the pointer
    if(!pLibDev->specialTx100Pkt) {
      pPkt += sizeof(WLAN_QOS_DATA_MAC_HEADER3);
//        pPkt += sizeof(WLAN_DATA_MAC_HEADER3);
    }

	// Skip 2 bytes so that wep IV is aligned on 4 bytes boundary
	// WLAN_QOS_DATA_MAC_HEADER3
	 pPkt += 2;

    //fill in the IV if required
    if (pLibDev->wepEnable && (mdkType == MDK_NORMAL_PKT)) {
        pIV = (A_UINT32 *)pPkt;

        *pIV = 0;
//        *pIV = (pLibDev->wepKey << 30) | 0x123456;  //fixed IV for now
        *pIV = 0xffffffff;
        pPkt += 4;
        *pPkt = 0xff;
        pPkt++;
        if (pLibDev->wepKey > 3) {
            *pPkt = 0x0;
        }
        else {
            *pPkt = (A_UCHAR)(pLibDev->wepKey << 6);
        }
        pPkt++;
    }

    //fill in packet type
    pMdkHeader = (MDK_PACKET_HEADER *)pPkt;
#ifdef ARCH_BIG_ENDIAN
    pMdkHeader->pktType = btol_s(mdkType);
    pMdkHeader->numPackets = btol_l(numDesc);
#else
    pMdkHeader->pktType = mdkType;
    pMdkHeader->numPackets = numDesc;
#endif
    //pMdkHeader->qcuIndex = queueIndex;        // __TODO__

    //fill in the repeating pattern, if special Tx100 packet, don't move on the pointer
    if(!pLibDev->specialTx100Pkt) {
        pPkt += sizeof(MDK_PACKET_HEADER);
    }
    while (dataBodyLength) {
        if(dataBodyLength > dataPatternLength) {
            amountToWrite = dataPatternLength;
            dataBodyLength -= dataPatternLength;
        }
        else {
            amountToWrite = dataBodyLength;
            dataBodyLength = 0;
        }

        memcpy(pPkt, dataPattern, amountToWrite);
        pPkt += amountToWrite;
    }

// WANRING :: TEST ONLY CHANGES for OWL
// Do memory writes for all the packets
	for(i=0; i<aggSize; i++)
    {
	    pLibDev->devMap.OSmemWrite(devNum, *pPktAddr + i*0x200, (A_UCHAR *)pTransmitPkt, *pPktSize);
	}


    /* write the packet to physical memory
     * need to check to see if the packet will be greater than 2000 bytes if so
     * need to perform the write in 2 steps
     */
/*    if (*pPktSize > 2000) {
        pLibDev->devMap.OSmemWrite(devNum, *pPktAddr, (A_UCHAR *)pTransmitPkt, 2000);
        if (*pPktSize > 4000) {
            pLibDev->devMap.OSmemWrite(devNum, (*pPktAddr + 2000), (A_UCHAR *)(pTransmitPkt + 2000), 2000);
            pLibDev->devMap.OSmemWrite(devNum, (*pPktAddr + 4000), (A_UCHAR *)(pTransmitPkt + 4000), *pPktSize - 4000);
        } else {
            pLibDev->devMap.OSmemWrite(devNum, (*pPktAddr + 2000), (A_UCHAR *)(pTransmitPkt + 2000), *pPktSize - 2000);
        }
    }
    else {
        pLibDev->devMap.OSmemWrite(devNum, *pPktAddr, (A_UCHAR *)pTransmitPkt, *pPktSize);
    }
*/
#ifdef DEBUG_MEMORY
printf("SNOOP::Frame contents are\n");
memDisplay(devNum, *pPktAddr, 20);
#endif

    //free the local memory
    free(pTransmitPkt);
    queueIndex = 0; //this is not used, quieting warnings

	return;
}


/**************************************************************************
* createTransmitPacket - create packet for transmission
*
*/
void createTransmitPacket
(
 A_UINT32 devNum,
 A_UINT16 mdkType,
 A_UCHAR *dest,
 A_UINT32 numDesc,
 A_UINT32 dataBodyLength,
 A_UCHAR *dataPattern,
 A_UINT32 dataPatternLength,
 A_UINT32 broadcast,
 A_UINT16 queueIndex,           // QCU index
 A_UINT32 *pPktSize,            //return size
 A_UINT32 *pPktAddr             //return address
)
{
    WLAN_DATA_MAC_HEADER3  *pPktHeader;
    LIB_DEV_INFO *pLibDev = gLibInfo.pLibDevArray[devNum];
    A_UINT32 amountToWrite;
    A_UCHAR broadcastAddr[] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
    MDK_PACKET_HEADER *pMdkHeader;
    A_UCHAR *pTransmitPkt;
    A_UCHAR *pPkt;
    A_UINT32 *pIV;

    //allocate enough memory for the packet
    *pPktSize = dataBodyLength;

    if (!pLibDev->specialTx100Pkt) {
        *pPktSize += sizeof(WLAN_DATA_MAC_HEADER3) + sizeof(MDK_PACKET_HEADER);
        if(pLibDev->wepEnable && (mdkType == MDK_NORMAL_PKT)) {
            *pPktSize += (WEP_IV_FIELD + 2);
        }
    }

    *pPktAddr = memAlloc(devNum, *pPktSize);
    if (0 == *pPktAddr) {
        mError(devNum, ENOMEM, "Device Number %d:createTransmitPacket: unable to allocate memory for transmit buffer\n", devNum);
        return;
    }

    //allocate local memory to create the packet in, so only do one block write of packet
    pTransmitPkt = pPkt = (A_UCHAR *)malloc(*pPktSize);
    if(!pPkt) {
        mError(devNum, ENOMEM, "Device Number %d:Unable to allocate local memory for packet\n", devNum);
        return;
    }

    pPktHeader = (WLAN_DATA_MAC_HEADER3 *)pPkt;

    //create the packet Header, all fields are listed here to enable easy changing
    pPktHeader->frameControl.protoVer = 0;
    pPktHeader->frameControl.fType = FRAME_DATA;
	pPktHeader->frameControl.fSubtype = SUBT_DATA;
    pPktHeader->frameControl.ToDS = 0;
    pPktHeader->frameControl.FromDS = 0;
    pPktHeader->frameControl.moreFrag = 0;
    pPktHeader->frameControl.retry = 0;
    pPktHeader->frameControl.pwrMgt = 0;
    pPktHeader->frameControl.moreData = 0;
    pPktHeader->frameControl.wep = ((pLibDev->wepEnable) && (mdkType == MDK_NORMAL_PKT)) ? 1 : 0;
    pPktHeader->frameControl.order = 0;
// Swap the bytes of the frameControl
#if 0
//#ifdef ARCH_BIG_ENDIAN
    {
        A_UINT16* ptr;

        ptr = (A_UINT16 *)(&(pPktHeader->frameControl));
        *ptr = btol_s(*ptr);
    }
#endif
//  pPktHeader->durationNav.clearBit = 0;
//  pPktHeader->durationNav.duration = 0;
    pPktHeader->durationNav = 0;  // aman redefined the struct to be just A_UINT16
// Swap the bytes of the durationNav
#if 0
//#ifdef ARCH_BIG_ENDIAN
    {
        A_UINT16* ptr;

        ptr = (A_UINT16 *)(&(pPktHeader->durationNav));
        *ptr = btol_s(*ptr);
    }
#endif
    memcpy(pPktHeader->address1.octets, broadcast ? broadcastAddr : dest, WLAN_MAC_ADDR_SIZE);
    memcpy(pPktHeader->address2.octets, pLibDev->macAddr.octets, WLAN_MAC_ADDR_SIZE);
    memcpy(pPktHeader->address3.octets, pLibDev->bssAddr.octets, WLAN_MAC_ADDR_SIZE);
//printf("SNOOP:: frame with\n");
//if (!broadcast) printf("SNOOP::dest %x:%x:%x:%x:%x:%x\n", dest[6], dest[5], dest[4], dest[3], dest[2], dest[1], dest[0]);
//printf("SNOOP::macaddr %x:%x:%x:%x:%x:%x\n", pLibDev->macAddr.octets[6], pLibDev->macAddr.octets[5], pLibDev->macAddr.octets[4], pLibDev->macAddr.octets[3], pLibDev->macAddr.octets[2], pLibDev->macAddr.octets[1], pLibDev->macAddr.octets[0]);
//printf("SNOOP::bssAddr %x:%x:%x:%x:%x:%x\n", pLibDev->bssAddr.octets[6], pLibDev->bssAddr.octets[5], pLibDev->bssAddr.octets[4], pLibDev->bssAddr.octets[3], pLibDev->bssAddr.octets[2], pLibDev->bssAddr.octets[1], pLibDev->bssAddr.octets[0]);
    WLAN_SET_FRAGNUM(pPktHeader->seqControl, 0);
    WLAN_SET_SEQNUM(pPktHeader->seqControl, 0xfed);
#if 0
//#ifdef ARCH_BIG_ENDIAN
    {
        A_UINT16* ptr;

        ptr = (A_UINT16 *)(&(pPktHeader->seqControl));
        *ptr = btol_s(*ptr);
    }
#endif

    //fill in the packet body, if special Tx100 packet, don't move on the pointer
    if(!pLibDev->specialTx100Pkt) {
        pPkt += sizeof(WLAN_DATA_MAC_HEADER3);
    }

    //fill in the IV if required
    if (pLibDev->wepEnable && (mdkType == MDK_NORMAL_PKT)) {
        pIV = (A_UINT32 *)pPkt;

        *pIV = 0;
//        *pIV = (pLibDev->wepKey << 30) | 0x123456;  //fixed IV for now
        *pIV = 0xffffffff;
        pPkt += 4;
        *pPkt = 0xff;
        pPkt++;
        if (pLibDev->wepKey > 3) {
            *pPkt = 0x0;
        }
        else {
            *pPkt = (A_UCHAR)(pLibDev->wepKey << 6);
        }
        pPkt++;
    }

    //fill in packet type
    pMdkHeader = (MDK_PACKET_HEADER *)pPkt;
#ifdef ARCH_BIG_ENDIAN
    pMdkHeader->pktType = btol_s(mdkType);
    pMdkHeader->numPackets = btol_l(numDesc);
#else
    pMdkHeader->pktType = mdkType;
    pMdkHeader->numPackets = numDesc;
#endif
    //pMdkHeader->qcuIndex = queueIndex;        // __TODO__

    //fill in the repeating pattern, if special Tx100 packet, don't move on the pointer
    if(!pLibDev->specialTx100Pkt) {
        pPkt += sizeof(MDK_PACKET_HEADER);
    }
    while (dataBodyLength) {
        if(dataBodyLength > dataPatternLength) {
            amountToWrite = dataPatternLength;
            dataBodyLength -= dataPatternLength;
        }
        else {
            amountToWrite = dataBodyLength;
            dataBodyLength = 0;
        }

        memcpy(pPkt, dataPattern, amountToWrite);
        pPkt += amountToWrite;
    }

    /* write the packet to physical memory
     * need to check to see if the packet will be greater than 2000 bytes if so
     * need to perform the write in 2 steps
     */
    if (*pPktSize > 2000) {
        pLibDev->devMap.OSmemWrite(devNum, *pPktAddr, (A_UCHAR *)pTransmitPkt, 2000);
        if (*pPktSize > 4000) {
            pLibDev->devMap.OSmemWrite(devNum, (*pPktAddr + 2000), (A_UCHAR *)(pTransmitPkt + 2000), 2000);
            pLibDev->devMap.OSmemWrite(devNum, (*pPktAddr + 4000), (A_UCHAR *)(pTransmitPkt + 4000), *pPktSize - 4000);
        } else {
            pLibDev->devMap.OSmemWrite(devNum, (*pPktAddr + 2000), (A_UCHAR *)(pTransmitPkt + 2000), *pPktSize - 2000);
        }
    }
    else {
        pLibDev->devMap.OSmemWrite(devNum, *pPktAddr, (A_UCHAR *)pTransmitPkt, *pPktSize);
    }
#ifdef DEBUG_MEMORY
printf("SNOOP::Frame contents are\n");
memDisplay(devNum, *pPktAddr, 20);
#endif

    //free the local memory
    free(pTransmitPkt);
    queueIndex = 0; //this is not used, quieting warnings
    return;
}

/**************************************************************************
* calculateRateThroughput - For the given rate, calculate throughput
*
*/
void calculateRateThroughput
(
 A_UINT32 devNum,
 A_UINT32 descRate,
 A_UINT32 queueIndex
)
{
    LIB_DEV_INFO *pLibDev = gLibInfo.pLibDevArray[devNum];
    A_UINT32 descStartTime;
    A_UINT32 descEndTime;
    A_UINT32 bitsReceived;
    float newTxTime = 0;

    descStartTime = pLibDev->tx[queueIndex].txStats[descRate].startTime;
    descEndTime = pLibDev->tx[queueIndex].txStats[descRate].endTime;


    if(descStartTime < descEndTime) {

//printf("pLibDev->tx[queueIndex].txStats[descRate].goodPackets: %d\n", pLibDev->tx[queueIndex].txStats[descRate].goodPackets);
//printf("pLibDev->tx[queueIndex].txStats[descRate].firstPktGood: %d\n", pLibDev->tx[queueIndex].txStats[descRate].firstPktGood);
//printf("descStartTime: %d\n", descStartTime);
//printf("descEndTime: %d\n", descEndTime);

        bitsReceived = (pLibDev->tx[queueIndex].txStats[descRate].goodPackets - pLibDev->tx[queueIndex].txStats[descRate].firstPktGood) \
            * (pLibDev->tx[queueIndex].dataBodyLen + sizeof(MDK_PACKET_HEADER)) * 8;
        newTxTime = (float)((descEndTime - descStartTime) * 1024)/1000;  //puts the time in millisecs
        pLibDev->tx[queueIndex].txStats[descRate].newThroughput = (A_UINT32)((float)bitsReceived / newTxTime);
    }
}

/**************************************************************************
* txAccumulateStats - walk through all descriptors and accumulate the stats
*
*/
void txAccumulateStats
(
 A_UINT32 devNum,
 A_UINT32 txTime,
 A_UINT16 queueIndex
)
{
    LIB_DEV_INFO *pLibDev = gLibInfo.pLibDevArray[devNum];

#ifdef DEBUG_MEMORY
printf("SNOOP::Collecting stats:txTime=%d\n", txTime);
#endif

    if (pLibDev->devMap.remoteLib && !isDragon(devNum)) {
        pLibDev->devMap.r_fillTxStats(devNum, pLibDev->tx[queueIndex].descAddress, pLibDev->tx[queueIndex].numDesc, pLibDev->tx[queueIndex].dataBodyLen, txTime, &(pLibDev->tx[queueIndex].txStats[0]));
    } else {
       fillTxStats(devNum, pLibDev->tx[queueIndex].descAddress, pLibDev->tx[queueIndex].numDesc, pLibDev->tx[queueIndex].dataBodyLen, txTime, &(pLibDev->tx[queueIndex].txStats[0]));
    }

    if (isFalcon(devNum) || isDragon(devNum)) {
        // transmit descriptor does not fill txTimeStamp for falcon.
        pLibDev->tx[queueIndex].txStats[0].newThroughput = pLibDev->tx[queueIndex].txStats[0].throughput;
    }

    pLibDev->tx[queueIndex].haveStats = 1;

    if (pLibDev->tx[queueIndex].txStats[0].underruns > 0) {
        printf("Device Number %d:SNOOP: Getting underruns %d\n", devNum, pLibDev->tx[queueIndex].txStats[0].underruns);
//FJC_DEBUG, take this back out when put the prefetch back in again
        pLibDev->adjustTxThresh = 1;
    }
#ifdef DEBUG_MEMORY
printf("SNOOP::Collecting stats done\n");
#endif
    return;
}

/**************************************************************************
* sendStatsPkt - Create and send a stats packet to other station
*
*/
void sendStatsPkt
(
 A_UINT32 devNum,
 A_UINT32 rate,                  // rate of bin to send
 A_UINT16 StatsType,             // which type of stats packet to send
 A_UCHAR *dest                   // address to send to
)
{
    A_UCHAR *pPktData = NULL;
    LIB_DEV_INFO *pLibDev = gLibInfo.pLibDevArray[devNum];
    A_UINT32 PktSize;
    A_UINT32 PktAddress;
    A_UINT32 DescAddress;
    A_UINT32 DataBodyLength = 0;
    A_UINT32 status1=0;
    A_UINT32 status2=0;
    A_UINT32 i;
    MDK_ATHEROS_DESC localDesc;
    A_UINT16    queueIndex = pLibDev->selQueueIndex;

    // error check statsType
    if ((StatsType > MDK_TXRX_STATS_PKT) || (StatsType < MDK_TX_STATS_PKT)) {
        mError(devNum, EINVAL, "Device Number %d:sendStatsPkt: illegal StatsType passed in\n", devNum);
        return;
    }
    if (rate > STATS_BINS - 1) {
        mError(devNum, EINVAL, "Device Number %d:sendStatsPkt: illegal rate bin for send packet %d\n", devNum, rate);
        return;
    }
    memset(&localDesc, 0, sizeof(MDK_ATHEROS_DESC));

    //create the packet
    if(StatsType == MDK_TXRX_STATS_PKT) {
        /* check to see what stats have been gathered.  If neither, then don't
         * send the stats, if only one set of stats have been gathered then send that
         * if both sets have been gathered then send both
         */
        if(!pLibDev->tx[queueIndex].haveStats && !pLibDev->rx.haveStats) {
            mError(devNum, EINVAL, "Device Number %d:sendStatsPkt: neither tx nor rx stats have been collected\n", devNum);
            return;
        } else if (!pLibDev->tx[queueIndex].haveStats) {
            //dont have tx stats, send only rx, change stats type to RX and handle later
            StatsType = MDK_RX_STATS_PKT;
        } else if (!pLibDev->rx.haveStats) {
            //dont have rx stats, send only tx, change stats type to RX and handle later
            StatsType = MDK_TX_STATS_PKT;
        } else {
            //send both tx and rx stats
            DataBodyLength = sizeof(TX_STATS_STRUCT) + sizeof(RX_STATS_STRUCT) + 2 * sizeof(rate);
            pPktData = (A_UCHAR *)malloc(DataBodyLength);
            if (!pPktData) {
                mError(devNum, ENOMEM, "Device Number %d:sendStatsPkt: unable to allocate memory for TX and RX stats structs\n", devNum);
                return;
            }
#ifdef ARCH_BIG_ENDIAN
            *(A_UINT32 *)(pPktData) = btol_l(rate);
            swapAndCopyBlock_l((void *)(pPktData + sizeof(rate)),(void *)(&(pLibDev->tx[queueIndex].txStats[rate])), sizeof(TX_STATS_STRUCT));
            *(A_UINT32 *)(pPktData + sizeof(TX_STATS_STRUCT) + sizeof(rate)) = btol_l(rate);
            //#####FJC - rxStats queue info is hardcoded to 0.  We don't currently send que info in mdkpkt
            swapAndCopyBlock_l((void *)(pPktData + sizeof(TX_STATS_STRUCT) + 2 * sizeof(rate)),(void *)(&(pLibDev->rx.rxStats[0][rate])), sizeof(RX_STATS_STRUCT));
#else
            *(A_UINT32 *)(pPktData) = rate;
            memcpy(pPktData + sizeof(rate), &(pLibDev->tx[queueIndex].txStats[rate]), sizeof(TX_STATS_STRUCT));
            *(A_UINT32 *)(pPktData + sizeof(TX_STATS_STRUCT) + sizeof(rate)) = rate;
            //#####FJC - rxStats queue info is hardcoded to 0.  We don't currently send que info in mdkpkt
            memcpy(pPktData + sizeof(TX_STATS_STRUCT) + 2 * sizeof(rate), &(pLibDev->rx.rxStats[0][rate]), sizeof(RX_STATS_STRUCT));
#endif
        }
    }

    if(StatsType == MDK_TX_STATS_PKT) {
        if(!pLibDev->tx[queueIndex].haveStats) {
            mError(devNum, EINVAL, "Device Number %d:sendStatsPkt: no tx stats have been collected\n", devNum);
            return;
        }
        DataBodyLength = sizeof(TX_STATS_STRUCT) + sizeof(rate);
        pPktData = (A_UCHAR *)malloc(DataBodyLength);
        if (!pPktData) {
            mError(devNum, ENOMEM, "Device Number %d:sendStatsPkt: unable to allocate memory for TX stats structs\n", devNum);
            return;
        }
#ifdef ARCH_BIG_ENDIAN
        *(A_UINT32 *)(pPktData) = btol_l(rate);
        swapAndCopyBlock_l((void *)(pPktData + sizeof(rate)),(void *)(&(pLibDev->tx[queueIndex].txStats[rate])), sizeof(TX_STATS_STRUCT));
#else
        *(A_UINT32 *)(pPktData) = rate;
        memcpy(pPktData + sizeof(rate), &(pLibDev->tx[queueIndex].txStats[rate]), sizeof(TX_STATS_STRUCT));
#endif
    }

    if (StatsType == MDK_RX_STATS_PKT) {
        if(!pLibDev->rx.haveStats) {
            mError(devNum, EINVAL, "Device Number %d:sendStatsPkt: no rx stats have been collected\n", devNum);
            return;
        }
        DataBodyLength = sizeof(RX_STATS_STRUCT) + sizeof(rate);
        pPktData = (A_UCHAR *)malloc(DataBodyLength);
        if(!pPktData) {
            mError(devNum, ENOMEM, "Device Number %d:sendStatsPkt: unable to allocate memory for TX stats structs\n", devNum);
            return;
        }
#ifdef ARCH_BIG_ENDIAN
        *(A_UINT32 *)(pPktData) = btol_l(rate);
        //#####FJC - rxStats queue info is hardcoded to 0.  We don't currently send que info in mdkpkt
        swapAndCopyBlock_l((void *)(pPktData + sizeof(rate)),(void *)(&(pLibDev->rx.rxStats[0][rate])), sizeof(RX_STATS_STRUCT));

#else
        *(A_UINT32 *)(pPktData) = rate;
        //#####FJC - rxStats queue info is hardcoded to 0.  We don't currently send que info in mdkpkt
        memcpy(pPktData + sizeof(rate), &(pLibDev->rx.rxStats[0][rate]), sizeof(RX_STATS_STRUCT));

#endif
    }

    createTransmitPacket(devNum, StatsType, dest, 1, DataBodyLength, pPktData,
                DataBodyLength, 0, 0, &PktSize, &PktAddress);

    //data should be copied to the packet memory, so can free TXRX memory
    if (pPktData) {
        free(pPktData);
    }

    //create the descriptor
    DescAddress = memAlloc( devNum, sizeof(MDK_ATHEROS_DESC));
    if (0 == DescAddress) {
        mError(devNum, ENOMEM, "Device Number %d:sendStatsPkt: unable to allocate memory for descriptor\n", devNum);
        return;
    }

    //create descriptor
#if defined(COBRA_AP) && defined(PCI_INTERFACE)
    if (isCobra(pLibDev->swDevID)) {
        localDesc.bufferPhysPtr = PktAddress;
    } else {
        localDesc.bufferPhysPtr = PktAddress | HOST_PCI_SDRAM_BASEADDR;
    }
#else
    localDesc.bufferPhysPtr = PktAddress;
#endif

    ar5kInitData[pLibDev->ar5kInitIndex].pMacAPI->setStatsPktDesc(devNum, &localDesc, PktSize, rateValues[0]);

    localDesc.nextPhysPtr = 0;

    writeDescriptor(devNum, DescAddress, &localDesc);
#ifdef DEBUG_MEMORY
printf("SNOOP::Desc contents are\n");
memDisplay(devNum, DescAddress, sizeof(MDK_ATHEROS_DESC)/sizeof(A_UINT32));
#endif

    //send the packet
    //set max retries
#if defined(COBRA_AP) && defined(PCI_INTERFACE)
    if (isCobra(pLibDev->swDevID)) {
        ar5kInitData[pLibDev->ar5kInitIndex].pMacAPI->beginSendStatsPkt(devNum, DescAddress);
    } else {
        ar5kInitData[pLibDev->ar5kInitIndex].pMacAPI->beginSendStatsPkt(devNum, DescAddress | HOST_PCI_SDRAM_BASEADDR);
    }
#else
    ar5kInitData[pLibDev->ar5kInitIndex].pMacAPI->beginSendStatsPkt(devNum, DescAddress);
#endif

    //poll done bit, waiting for descriptor to be complete
    for(i = 0; i < MDK_PKT_TIMEOUT+10; i++) {
       pLibDev->devMap.OSmemRead(devNum,  DescAddress + pLibDev->txDescStatus2,
                (A_UCHAR *)&status2, sizeof(status2));
       if(status2 & DESC_DONE) {
           pLibDev->devMap.OSmemRead(devNum,  DescAddress + pLibDev->txDescStatus1,
                (A_UCHAR *)&status1, sizeof(status1));
           if(!(status1 & DESC_FRM_XMIT_OK)) {
                mError(devNum, EIO, "Device Number %d:sendStsPkt: remote stats packet not successfully sent, status = %08lx\n", devNum, status1);
                memFree(devNum, DescAddress);
                memFree(devNum, PktAddress);
                return;
            }
#ifdef DEBUG_MEMORY
printf("SNOOP::sendStatsPkt for rate %d done\n", rate);
#endif
            break;
        }
#ifdef OWL_AP
		if (i == 5) {
			//give up
			break;
		}
#endif
       mSleep(1);

    }

    if (i == MDK_PKT_TIMEOUT) {
        mError(devNum, EIO, "Device Number %d:sendStsPkt: timed out waiting for stats packet done\n", devNum);

    }
    memFree(devNum, DescAddress);
    memFree(devNum, PktAddress);


    return;
}

/**************************************************************************
* writeDescriptor - write descriptor to physical memory
*
*/
void writeDescriptor
(
 A_UINT32   devNum,
 A_UINT32   descAddress,
 MDK_ATHEROS_DESC *pDesc
)
{
#ifndef THIN_CLIENT_BUILD
    LIB_DEV_INFO *pLibDev = gLibInfo.pLibDevArray[devNum];
    pLibDev->devMap.OSmemWrite(devNum, descAddress, (A_UCHAR *)pDesc, sizeof(MDK_ATHEROS_DESC));
#endif
}

/**************************************************************************
* writeDescriptors - write a group of descriptors to physical memory
*
*/
#define MAX_MEMREAD_BYTES       2048
void writeDescriptors
(
 A_UINT32   devNumIndex,
 A_UINT32   descAddress,
 MDK_ATHEROS_DESC *pDesc,
 A_UINT32   numDescriptors
)
{
#ifdef THIN_CLIENT_BUILD
   hwMemWriteBlock(devNumIndex, pDesc, sizeof(MDK_ATHEROS_DESC), descAddress);
#else
    LIB_DEV_INFO *pLibDev = gLibInfo.pLibDevArray[devNumIndex];
    A_UINT32 sizeToWrite;
    A_UCHAR *blockPtr;

    //need to check for block of descriptors being greater than the write size per cmd,
    //if so need to split the block
    sizeToWrite = sizeof(MDK_ATHEROS_DESC) * numDescriptors;
    blockPtr = (A_UCHAR *)pDesc;
    while(sizeToWrite > MAX_MEMREAD_BYTES) {
        pLibDev->devMap.OSmemWrite(devNumIndex, descAddress, blockPtr, MAX_MEMREAD_BYTES);
        blockPtr += MAX_MEMREAD_BYTES;
        descAddress += MAX_MEMREAD_BYTES;
        sizeToWrite -= MAX_MEMREAD_BYTES;
    }

    pLibDev->devMap.OSmemWrite(devNumIndex, descAddress, (A_UCHAR *)blockPtr, sizeToWrite);
#endif
}

/**************************************************************************
* mdkExtractAddrAndSequence() - Extract address and sequence from packets
*
* Checks to see if we still need to get the address and sequence control
* for the current packet, if so then try to extract from this buffer.
* Make the assumption that the header is contained in one buffer. May add
* support for multiple buffers later if needed.
*
* RETURNS:
*/
void mdkExtractAddrAndSequence
(
 A_UINT32           devNum,
 RX_STATS_TEMP_INFO *pStatsInfo
)
{
    A_UINT32             i;
    A_INT32              pktBodySize;
    FRAME_CONTROL       *pFrameControl;
    FRAME_CONTROL        frameControlValue;
    LIB_DEV_INFO        *pLibDev = gLibInfo.pLibDevArray[devNum];
    WLAN_MACADDR         frameMacAddr = {0,0,0,0,0,0};
    MDK_PACKET_HEADER    mdkPktInfo;
    A_BOOL               falconTrue = FALSE;
    A_UINT32             ppm_data_padding = PPM_DATA_SIZE;

    if(pStatsInfo->gotHeaderInfo)
    {
        /* we've already done this for this packet, get out */
        return;
    }

    falconTrue = isFalcon(devNum) || isDragon(devNum);
    if (falconTrue) {
        if (pLibDev->libCfgParams.chainSelect == 2) {
            ppm_data_padding = PPM_DATA_SIZE_FALCON_DUAL_CHAIN;
        } else {
            ppm_data_padding = PPM_DATA_SIZE_FALCON_SINGLE_CHAIN;
        }
    }
    if (isOwl(pLibDev->swDevID)) {
        ppm_data_padding = 0;
    }

    /* if got to here then assume that this is the first buffer
     * of the packet.  For now are assuming that the header is
     * fully contained in this first buffer
     */

    /* do some error checking */

    /* calculate pkt body size before reading any buffer contents */
	if(pLibDev->yesAgg == TRUE ) {
      pktBodySize = (pStatsInfo->status1 & DESC_DATA_LENGTH_FIELDS) - sizeof(WLAN_QOS_DATA_MAC_HEADER3) - 2
                    - FCS_FIELD;
	} else {
      pktBodySize = (pStatsInfo->status1 & DESC_DATA_LENGTH_FIELDS) - sizeof(WLAN_DATA_MAC_HEADER3)
                    - FCS_FIELD;
	}

    if (pLibDev->rx.enablePPM) {
        pktBodySize -= ppm_data_padding;
    }

    /* A non-MDK packet has snuck into our receive */
    //Only need this filter on for crete, don't expect to come across it with maui, want to know about it if do
    if(pktBodySize < (A_INT32)(sizeof(MDK_PACKET_HEADER))) {
        pStatsInfo->badPacket = TRUE;
        pStatsInfo->gotHeaderInfo = TRUE;
        //Only need this filter on for crete, don't expect to come across it with maui,
        //want to know about it if do
        if (pLibDev->swDevID != 0x0007) {
            if(pStatsInfo->status2 & DESC_FRM_RCV_OK) {
//              mError(devNum, EIO, "\nDevice Number %d:mdkExtractAddrAndSequence: A packet smaller than a valid MDK packet was received\n", devNum);
            }
        }
        return;
    }


    /* first check to see if this is a control packet, is so don't
     * have any sequence num checking to do
     */
     memcpy((A_UCHAR *)&frameControlValue, (A_UCHAR*)&(pStatsInfo->frame_info[0]), sizeof(frameControlValue));

     pFrameControl = (FRAME_CONTROL *)&frameControlValue;

#if defined(COBRA_AP) && defined(LINUX)
{
int j,k;
for(j = 0; j < 1000; j++) {k +=j * 2;}
}
#endif

/*
printf("framecontrol = %x\n", frameControlValue);
printf("protoVer=%x:ftype=%x:st=%x:td=%x:fd=%x:mf=%x:r=%x:pm=%x:md=%d:wep=%d:order=%d\n", \
    pFrameControl->protoVer, \
    pFrameControl->fType, \
    pFrameControl->fSubtype, \
    pFrameControl->ToDS, \
    pFrameControl->FromDS, \
    pFrameControl->moreFrag, \
    pFrameControl->retry, \
    pFrameControl->pwrMgt, \
    pFrameControl->moreData, \
    pFrameControl->wep, \
    pFrameControl->order \
    );
*/

if ( ((pFrameControl->fType == FRAME_CTRL) || (pFrameControl->fType == FRAME_MGT)) && (pLibDev->rx.rxMode != RX_FIXED_NUMBER))
    {
        pStatsInfo->controlFrameReceived = TRUE;
        pStatsInfo->gotHeaderInfo = TRUE;
//      mError(devNum, EIO, "Device Number %d:mdkExtractAddrAndSequence: Not extracting address info, control frame received\n", devNum);
        return;
    }

    // Recheck pkt body size now that we know we've read the frame control
    pktBodySize -= (pFrameControl->wep ?  (WEP_IV_FIELD + 2 + WEP_ICV_FIELD) : 0);
    if(pktBodySize < (A_INT32)(sizeof(MDK_PACKET_HEADER))) {
        pStatsInfo->badPacket = TRUE;
        pStatsInfo->gotHeaderInfo = TRUE;
        if(pStatsInfo->status2 & DESC_FRM_RCV_OK) {
//          mError(devNum, EIO, "Device Number %d:mdkExtractAddrAndSequence: A packet smaller than a valid MDK packet was received, second check\n", devNum);
        }
        return;
    }

    //get the MDK packet info from the start of the packet
	if(pLibDev->yesAgg == TRUE) {
      memcpy( (A_UCHAR *)&mdkPktInfo, &(pStatsInfo->frame_info[0]) + sizeof(WLAN_QOS_DATA_MAC_HEADER3) + 2 + (pFrameControl->wep ? (WEP_IV_FIELD + 2) : 0), sizeof(mdkPktInfo));
	} else {
      memcpy( (A_UCHAR *)&mdkPktInfo, &(pStatsInfo->frame_info[0]) + MAC_HDR_DATA3_SIZE + (pFrameControl->wep ? (WEP_IV_FIELD + 2) : 0), sizeof(mdkPktInfo));
	}

#ifdef ARCH_BIG_ENDIAN
    mdkPktInfo.pktType = ltob_s(mdkPktInfo.pktType);
    mdkPktInfo.numPackets = ltob_l(mdkPktInfo.numPackets);
#endif
    pStatsInfo->mdkPktType = mdkPktInfo.pktType;
    pStatsInfo->qcuIndex = 0; //mdkPktInfo.qcuIndex;        // __TODO__

#ifdef DEBUG_MEMORY
printf("SNOOP::Frame contents are\n");
memDisplay(devNum, pStatsInfo->bufferAddr, 20*4);
printf("SNOOP::Address 2 offset = %x\n", ADDRESS2_START);
#endif

    /* should now be able to copy the mac address and seq control */
    for (i = 0; i < WLAN_MAC_ADDR_SIZE; i++) {
        memcpy( &(frameMacAddr.octets[i]), &(pStatsInfo->frame_info[0]) + ADDRESS2_START + i, sizeof(frameMacAddr.octets[i]));
    }

    // see if this is the mac address we expect.  If we don't already have a mac address for
    //compare then grab this one, otherwise compare it against the one we hold
    if (pLibDev->rx.rxMode == RX_FIXED_NUMBER) {
        //need to compare against our BSSID
        if(A_MACADDR_COMP(&pLibDev->bssAddr, &frameMacAddr)) {
            pStatsInfo->badPacket = TRUE;
            pStatsInfo->gotHeaderInfo = TRUE;
            return;
        }
    } else if (pStatsInfo->gotMacAddr) {
        if (A_MACADDR_COMP(&(pStatsInfo->addrPacket), &frameMacAddr)) {
            //mac addresses don't match, need to flag this as a bad packet
            //we need to traverse the descriptors to get to the end of it,
            //but we don't want to gather stats on it
            pStatsInfo->badPacket = TRUE;
            pStatsInfo->gotHeaderInfo = TRUE;
            return;
        }
    } else {
        //we don't have mac address of received packets. Assume this is the address to
        //receive from so take a copy of it if this is an MDK style PACKET
        if((pStatsInfo->mdkPktType >= MDK_NORMAL_PKT) && (pStatsInfo->mdkPktType <= MDK_TXRX_STATS_PKT)
			&& (pStatsInfo->status2 & DESC_FRM_RCV_OK)) {
            A_MACADDR_COPY(&frameMacAddr, &pStatsInfo->addrPacket);
            pStatsInfo->gotMacAddr = TRUE;
        } else {
            //don't mark it bad if its a decrypt error.  We need to let these through
            if ((pStatsInfo->status2 & 0x1f) != 0x11) {
                pStatsInfo->badPacket = TRUE;
            }
            pStatsInfo->gotHeaderInfo = TRUE;
#ifdef _DEBUG
//            mError(devNum, EIO, "Device Number %d:mdkExtractAddrAndSequence: A non-MDK packet was received before any MDK packets - ignoring\n", devNum);
#endif
            return;
        }
    }

    memcpy( (A_UCHAR *)&(pStatsInfo->seqCurrentPacket), &(pStatsInfo->frame_info[0]) + SEQ_CONTROL_START, sizeof(pStatsInfo->seqCurrentPacket));
    pStatsInfo->retry = (A_BOOL)(pFrameControl->retry);
    pStatsInfo->gotHeaderInfo = TRUE;

    return;
}

/**************************************************************************
* extractRxStats() - Extract and save receive descriptor statistics
*
* Extract and save required statistics.
*
* RETURNS:
*/
void mdkExtractRxStats
(
 A_UINT32 devNum,
 RX_STATS_TEMP_INFO *pStatsInfo
)
{
    A_BOOL          isDuplicate;
    LIB_DEV_INFO *pLibDev = gLibInfo.pLibDevArray[devNum];
    RX_STATS_STRUCT  *rxStatsTemp;
    MULT_ANT_SIG_STRENGTH_STATS  *multAntStatsTemp;
    A_UINT32          ii, jj, kk, ll;
    A_INT32           rssi_max = -128;
	A_UINT16 agg, moreAgg;

    /* first check for good packets */
    if (pStatsInfo->status2 & DESC_FRM_RCV_OK)
    {

		// If this is first good packet record start time stamp for tput calculation
		if (pLibDev->rx.rxStats[pStatsInfo->qcuIndex][0].goodPackets == 0 )
		{
			pLibDev->rx.rxStats[pStatsInfo->qcuIndex][0].startTime = pStatsInfo->rxTimeStamp;
		}

        pLibDev->rx.rxStats[pStatsInfo->qcuIndex][pStatsInfo->descRate].goodPackets ++;
        pLibDev->rx.rxStats[pStatsInfo->qcuIndex][0].goodPackets ++;


        /* do any duplicate packet processing */
        mdkCountDuplicatePackets(devNum, pStatsInfo, &isDuplicate);

        /* add to total number of bytes count if not duplicate */
        if (!isDuplicate) {
			pLibDev->rx.rxStats[pStatsInfo->qcuIndex][0].byteCount += (pStatsInfo->status1 & 0xfff);
            pStatsInfo->totalBytes += (pStatsInfo->status1 & 0xfff);
        }

        /* do signal strength processing */
        if (isOwl(pLibDev->swDevID)) {
			// If this is agg frame collect signal strength only a agg frame boundary
			if(pLibDev->yesAgg == 1) {
				moreAgg = (A_UINT16)((pStatsInfo->status2 >> 16) & 0x1); /* checking more_agg[16] in the Rx descriptor (RXS12) */
				agg = (A_UINT16)((pStatsInfo->status2 >> 17) & 0x1); /* checking aggregate[17] in the Rx descriptor (RXS12) */
				if((moreAgg == 0) && (agg == 1)) {
				  mdkGetSignalStrengthStats(&(pStatsInfo->sigStrength[pStatsInfo->descRate]),
					(A_INT8)((pStatsInfo->status3 >> pLibDev->bitsToRxSigStrength) & SIG_STRENGTH_MASK));
                  mdkGetSignalStrengthStats(&(pStatsInfo->sigStrength[0]),
                    (A_INT8)((pStatsInfo->status3 >> pLibDev->bitsToRxSigStrength) & SIG_STRENGTH_MASK));

                  for (ii = 0; ii < 3; ii++) {
                    /* Control antennas */
                    mdkGetSignalStrengthStats(&(pStatsInfo->ctlAntStrength[ii][pStatsInfo->descRate]),
                       (A_INT8)((pStatsInfo->status4 >> (ii * 8)) & SIG_STRENGTH_MASK));
                    mdkGetSignalStrengthStats(&(pStatsInfo->ctlAntStrength[ii][0]),
                       (A_INT8)((pStatsInfo->status4 >> (ii * 8)) & SIG_STRENGTH_MASK));

                    /* Extension antennas */
                    mdkGetSignalStrengthStats(&(pStatsInfo->extAntStrength[ii][pStatsInfo->descRate]),
                      (A_INT8)((pStatsInfo->status3 >> (ii * 8)) & SIG_STRENGTH_MASK));
                    mdkGetSignalStrengthStats(&(pStatsInfo->extAntStrength[ii][0]),
                      (A_INT8)((pStatsInfo->status3 >> (ii * 8)) & SIG_STRENGTH_MASK));
				  }
				  // collect evm number
				  pLibDev->rx.rxStats[pStatsInfo->qcuIndex][0].evm_stream0 += pStatsInfo->evm_stream0;
				  pLibDev->rx.rxStats[pStatsInfo->qcuIndex][0].evm_stream1 += pStatsInfo->evm_stream1;

				}
			} else {
              mdkGetSignalStrengthStats(&(pStatsInfo->sigStrength[pStatsInfo->descRate]),
                (A_INT8)((pStatsInfo->status3 >> pLibDev->bitsToRxSigStrength) & SIG_STRENGTH_MASK));
              mdkGetSignalStrengthStats(&(pStatsInfo->sigStrength[0]),
                (A_INT8)((pStatsInfo->status3 >> pLibDev->bitsToRxSigStrength) & SIG_STRENGTH_MASK));

              for (ii = 0; ii < 3; ii++) {
                /* Control antennas */
                mdkGetSignalStrengthStats(&(pStatsInfo->ctlAntStrength[ii][pStatsInfo->descRate]),
                (A_INT8)((pStatsInfo->status4 >> (ii * 8)) & SIG_STRENGTH_MASK));
                mdkGetSignalStrengthStats(&(pStatsInfo->ctlAntStrength[ii][0]),
                (A_INT8)((pStatsInfo->status4 >> (ii * 8)) & SIG_STRENGTH_MASK));

                /* Extension antennas */
                mdkGetSignalStrengthStats(&(pStatsInfo->extAntStrength[ii][pStatsInfo->descRate]),
                (A_INT8)((pStatsInfo->status3 >> (ii * 8)) & SIG_STRENGTH_MASK));
                mdkGetSignalStrengthStats(&(pStatsInfo->extAntStrength[ii][0]),
                (A_INT8)((pStatsInfo->status3 >> (ii * 8)) & SIG_STRENGTH_MASK));
			  }
			  // collect evm number
			  pLibDev->rx.rxStats[pStatsInfo->qcuIndex][0].evm_stream0 += pStatsInfo->evm_stream0;
			  pLibDev->rx.rxStats[pStatsInfo->qcuIndex][0].evm_stream1 += pStatsInfo->evm_stream1;

			}
        } else {
            mdkGetSignalStrengthStats(&(pStatsInfo->sigStrength[pStatsInfo->descRate]),
                (A_INT8)((pStatsInfo->status1 >> pLibDev->bitsToRxSigStrength) & SIG_STRENGTH_MASK));
            mdkGetSignalStrengthStats(&(pStatsInfo->sigStrength[0]),
                (A_INT8)((pStatsInfo->status1 >> pLibDev->bitsToRxSigStrength) & SIG_STRENGTH_MASK));
        }

        if (isFalcon(devNum) || isDragon(devNum)) {
            /* do multi antenna signal strength and chainSel/Req processing for falcon */
            mdkProcessMultiAntSigStrength(devNum, pStatsInfo);

            for (kk=0; kk<2; kk++) {
                /* Perform collection on descRate bin and in all rates bin 0 */
                ll = (kk>0) ? pStatsInfo->descRate : 0;
                rxStatsTemp = &(pLibDev->rx.rxStats[pStatsInfo->qcuIndex][ll]);
                multAntStatsTemp = &(pStatsInfo->multAntSigStrength[ll]);

                rssi_max = -128;
                jj = 0;

                for (ii=0; ii<4; ii++) {
                    rxStatsTemp->RSSIPerAntAvg[ii] = (A_INT32) multAntStatsTemp->rxAvrgSignalStrengthAnt[ii];
                    rxStatsTemp->RSSIPerAntMax[ii] = (A_INT32) multAntStatsTemp->rxMaxSigStrengthAnt[ii];
                    rxStatsTemp->RSSIPerAntMin[ii] = (A_INT32) multAntStatsTemp->rxMinSigStrengthAnt[ii];
                    if (rxStatsTemp->RSSIPerAntAvg[ii] > rssi_max) {
                        rssi_max = rxStatsTemp->RSSIPerAntAvg[ii];
                        jj = ii; // pick the ant with max rssi_avg to report
                    }
                }

                rxStatsTemp->maxRSSIAnt = jj;
                rxStatsTemp->DataSigStrengthAvg = rxStatsTemp->RSSIPerAntAvg[jj];
                rxStatsTemp->DataSigStrengthMax = rxStatsTemp->RSSIPerAntMax[jj];
                rxStatsTemp->DataSigStrengthMin = rxStatsTemp->RSSIPerAntMin[jj];
                for (ii=0; ii<2; ii++) {
                    rxStatsTemp->Chain0AntReq[ii] = pStatsInfo->Chain0AntReq[ll].Count[ii];
                    rxStatsTemp->Chain1AntReq[ii] = pStatsInfo->Chain1AntReq[ll].Count[ii];
                    rxStatsTemp->Chain0AntSel[ii] = pStatsInfo->Chain0AntSel[ll].Count[ii];
                    rxStatsTemp->Chain1AntSel[ii] = pStatsInfo->Chain1AntSel[ll].Count[ii];
                    rxStatsTemp->ChainStrong[ii]  = pStatsInfo->ChainStrong[ll].Count[ii];
                }
            }
        } else {
            pLibDev->rx.rxStats[pStatsInfo->qcuIndex][pStatsInfo->descRate].DataSigStrengthAvg = pStatsInfo->sigStrength[pStatsInfo->descRate].rxAvrgSignalStrength;
            pLibDev->rx.rxStats[pStatsInfo->qcuIndex][pStatsInfo->descRate].DataSigStrengthMax = pStatsInfo->sigStrength[pStatsInfo->descRate].rxMaxSigStrength;
            pLibDev->rx.rxStats[pStatsInfo->qcuIndex][pStatsInfo->descRate].DataSigStrengthMin = pStatsInfo->sigStrength[pStatsInfo->descRate].rxMinSigStrength;
            pLibDev->rx.rxStats[pStatsInfo->qcuIndex][0].DataSigStrengthAvg = pStatsInfo->sigStrength[0].rxAvrgSignalStrength;
            pLibDev->rx.rxStats[pStatsInfo->qcuIndex][0].DataSigStrengthMax = pStatsInfo->sigStrength[0].rxMaxSigStrength;
            pLibDev->rx.rxStats[pStatsInfo->qcuIndex][0].DataSigStrengthMin = pStatsInfo->sigStrength[0].rxMinSigStrength;
            if (isOwl(pLibDev->swDevID)) {
                for (ii = 0; ii < 3; ii++) {
                    /* Control antennas */
                    pLibDev->rx.rxStats[pStatsInfo->qcuIndex][pStatsInfo->descRate].RSSIPerAntAvg[ii] = pStatsInfo->ctlAntStrength[ii][pStatsInfo->descRate].rxAvrgSignalStrength;
                    pLibDev->rx.rxStats[pStatsInfo->qcuIndex][pStatsInfo->descRate].RSSIPerAntMax[ii] = pStatsInfo->ctlAntStrength[ii][pStatsInfo->descRate].rxMaxSigStrength;
                    pLibDev->rx.rxStats[pStatsInfo->qcuIndex][pStatsInfo->descRate].RSSIPerAntMin[ii] = pStatsInfo->ctlAntStrength[ii][pStatsInfo->descRate].rxMinSigStrength;
                    pLibDev->rx.rxStats[pStatsInfo->qcuIndex][0].RSSIPerAntAvg[ii] = pStatsInfo->ctlAntStrength[ii][0].rxAvrgSignalStrength;
                    pLibDev->rx.rxStats[pStatsInfo->qcuIndex][0].RSSIPerAntMax[ii] = pStatsInfo->ctlAntStrength[ii][0].rxMaxSigStrength;
                    pLibDev->rx.rxStats[pStatsInfo->qcuIndex][0].RSSIPerAntMin[ii] = pStatsInfo->ctlAntStrength[ii][0].rxMinSigStrength;

                    /* Extension antennas */
                    pLibDev->rx.rxStats[pStatsInfo->qcuIndex][pStatsInfo->descRate].RSSIPerExtAntAvg[ii] = pStatsInfo->extAntStrength[ii][pStatsInfo->descRate].rxAvrgSignalStrength;
                    pLibDev->rx.rxStats[pStatsInfo->qcuIndex][pStatsInfo->descRate].RSSIPerExtAntMax[ii] = pStatsInfo->extAntStrength[ii][pStatsInfo->descRate].rxMaxSigStrength;
                    pLibDev->rx.rxStats[pStatsInfo->qcuIndex][pStatsInfo->descRate].RSSIPerExtAntMin[ii] = pStatsInfo->extAntStrength[ii][pStatsInfo->descRate].rxMinSigStrength;
                    pLibDev->rx.rxStats[pStatsInfo->qcuIndex][0].RSSIPerExtAntAvg[ii] = pStatsInfo->extAntStrength[ii][0].rxAvrgSignalStrength;
                    pLibDev->rx.rxStats[pStatsInfo->qcuIndex][0].RSSIPerExtAntMax[ii] = pStatsInfo->extAntStrength[ii][0].rxMaxSigStrength;
                    pLibDev->rx.rxStats[pStatsInfo->qcuIndex][0].RSSIPerExtAntMin[ii] = pStatsInfo->extAntStrength[ii][0].rxMinSigStrength;
                }
            }
        }
    } else {
//printf("SNOOP: Packet NOK status2 = %x\n", pStatsInfo->status2);
        if (pStatsInfo->status2 & DESC_CRC_ERR) {
            pLibDev->rx.rxStats[pStatsInfo->qcuIndex][0].crcPackets++;
            pLibDev->rx.rxStats[pStatsInfo->qcuIndex][pStatsInfo->descRate].crcPackets++;
        } else if (pStatsInfo->status2 & pLibDev->decryptErrMsk) {
            pLibDev->rx.rxStats[pStatsInfo->qcuIndex][0].decrypErrors++;
            pLibDev->rx.rxStats[pStatsInfo->qcuIndex][pStatsInfo->descRate].decrypErrors++;
        }
    }

    pLibDev->rx.haveStats = 1;
    return;
}

/**************************************************************************
* mdkExtractRemoteStats() - Extract and save statistics sent from remote stn
*
*
* RETURNS: TRUE if the last stats packet or error - otherwise FALSE
*/
A_BOOL mdkExtractRemoteStats
(
 A_UINT32 devNum,
 RX_STATS_TEMP_INFO *pStatsInfo
)
{
    LIB_DEV_INFO *pLibDev = gLibInfo.pLibDevArray[devNum];
    A_UINT32     addressStats;          //physical address of where stats are in packet
    A_UINT32     locBin = 0;
#ifdef DEBUG_MEMORY
printf("SNOOP::mdkExtractRemoteStats enter\n");
#endif

    if ((pStatsInfo->mdkPktType < MDK_TX_STATS_PKT) || (pStatsInfo->mdkPktType > MDK_TXRX_STATS_PKT)) {
        mError(devNum, EINVAL, "Device Number %d:mdkExtractRemoteStats: Illegal mdk packet type for stats extraction\n", devNum);
        return TRUE;
    }

    pLibDev->remoteStats = pStatsInfo->mdkPktType;

    addressStats = pStatsInfo->bufferAddr + sizeof(WLAN_DATA_MAC_HEADER3) + sizeof(MDK_PACKET_HEADER);
    if ((pStatsInfo->mdkPktType == MDK_TX_STATS_PKT) ||
        (pStatsInfo->mdkPktType == MDK_TXRX_STATS_PKT))
    {
        //figure out which bin to put these remote stats in
        pLibDev->devMap.OSmemRead(devNum, addressStats,
                    (A_UCHAR *)&locBin, sizeof(locBin));

#ifdef ARCH_BIG_ENDIAN
        locBin = ltob_l(locBin);
#endif
#ifdef DEBUG_MEMORY
printf("SNOOP::mdkExtractRemoteStats::locBin=%d\n", locBin);
#endif

        if(locBin > STATS_BINS - 1) {
            mError(devNum, EINVAL, "Device Number %d:mdkExtractRemoteStats: TXSTATS:Illegal stats bin value received: %d\n", devNum, locBin);
            return TRUE;
        }

        addressStats += sizeof(locBin);

        //copy over stats info from packet
        pLibDev->devMap.OSmemRead(devNum, addressStats,
                    (A_UCHAR *)&pLibDev->txRemoteStats[locBin], sizeof(TX_STATS_STRUCT));
#ifdef ARCH_BIG_ENDIAN
        swapBlock_l((void *)(&pLibDev->txRemoteStats[locBin]),sizeof(TX_STATS_STRUCT));
#endif

        //increment our address value so that would point to RX stats if any
        addressStats += sizeof(TX_STATS_STRUCT);
    }

    if ((pStatsInfo->mdkPktType == MDK_RX_STATS_PKT) ||
        (pStatsInfo->mdkPktType == MDK_TXRX_STATS_PKT)) {

        pLibDev->devMap.OSmemRead(devNum, addressStats,
                    (A_UCHAR *)&locBin, sizeof(locBin));
#ifdef ARCH_BIG_ENDIAN
        locBin = ltob_l(locBin);
#endif

        //if this is venice, then need to put the stats in the correct CCK bins
        if (((pLibDev->swDevID & 0xff) >= 0x0013) && (pLibDev->mode == MODE_11B)){
            if((locBin > 0) && (locBin < 9)) {
                if(locBin == 1){
                    locBin += 8;
                }
                else {
                    locBin += 7;
                }
            }
        }

        if (locBin > STATS_BINS - 1) {
            mError(devNum, EINVAL, "Device Number %d:mdkExtractRemoteStats: RXSTATS:Illegal stats bin value received: %x\n", devNum, locBin);
            return TRUE;
        }

        addressStats += sizeof(locBin);

        //copy over stats info from packet
        pLibDev->devMap.OSmemRead(devNum, addressStats,
                    (A_UCHAR *)&pLibDev->rxRemoteStats[0][locBin], sizeof(RX_STATS_STRUCT));

#ifdef ARCH_BIG_ENDIAN
        swapBlock_l((void *)(&pLibDev->rxRemoteStats[0][locBin]),sizeof(RX_STATS_STRUCT));
#endif
    }
#ifdef DEBUG_MEMORY
printf("SNOOP::return from mdkExtractRemoteStats:locBin=%d\n", locBin);
#endif
    return (A_UCHAR)(locBin ? FALSE : TRUE );
}

/**************************************************************************
* countDuplicatePackets() - Count duplicate packets
*
* Uses the address and sequence number already extracted for the packet
* and determines if this is a duplicate packet.  Updates statistics if
* it is a duplicate packet
*
* RETURNS:
*/
void mdkCountDuplicatePackets
(
 A_UINT32   devNum,
 RX_STATS_TEMP_INFO *pStatsInfo,
 A_BOOL     *pIsDuplicate
)
{
    LIB_DEV_INFO *pLibDev = gLibInfo.pLibDevArray[devNum];
    *pIsDuplicate = FALSE;

    // don't need to do any address checking, we did that already
    if (pStatsInfo->retry) {
        if((WLAN_GET_SEQNUM(pStatsInfo->seqCurrentPacket) == pStatsInfo->lastRxSeqNo) &&
           (WLAN_GET_FRAGNUM(pStatsInfo->seqCurrentPacket) == pStatsInfo->lastRxFragNo))
        {
            *pIsDuplicate = TRUE;
            pLibDev->rx.rxStats[pStatsInfo->qcuIndex][0].singleDups++;
            pLibDev->rx.rxStats[pStatsInfo->qcuIndex][pStatsInfo->descRate].singleDups++;

            if(WLAN_GET_SEQNUM(pStatsInfo->seqCurrentPacket) == pStatsInfo->oneBeforeLastRxSeqNo) {
                pLibDev->rx.rxStats[pStatsInfo->qcuIndex][0].multipleDups++;
                pLibDev->rx.rxStats[pStatsInfo->qcuIndex][pStatsInfo->descRate].singleDups++;
            }
            pStatsInfo->oneBeforeLastRxSeqNo = pStatsInfo->lastRxSeqNo;
        }
    }

    pStatsInfo->lastRxSeqNo = WLAN_GET_SEQNUM(pStatsInfo->seqCurrentPacket);
    pStatsInfo->lastRxFragNo = WLAN_GET_FRAGNUM(pStatsInfo->seqCurrentPacket);

    return;
}

/**************************************************************************
* mdkGetSignalStrengthStats() - Update signal strength stats
*
* Given the value of signal strength from the descriptor, update the
* relevant signal strength statistics
*
* RETURNS:
*/
void mdkGetSignalStrengthStats
(
 SIG_STRENGTH_STATS *pStats,
 A_INT8         signalStrength
)
{
    //workaround -128 RSSI
    if(signalStrength == -128) {
        signalStrength = 0;
    }
    pStats->rxAccumSignalStrength += signalStrength;
    pStats->rxNumSigStrengthSamples++;
    pStats->rxAvrgSignalStrength = (A_INT8)
    (pStats->rxAccumSignalStrength/(A_INT32)pStats->rxNumSigStrengthSamples);

    /* max and min */
    if (signalStrength > pStats->rxMaxSigStrength) {
        pStats->rxMaxSigStrength = signalStrength;
    }

    if (signalStrength < pStats->rxMinSigStrength) {
        pStats->rxMinSigStrength = signalStrength;
    }
    return;
}

/**************************************************************************
* extractPPM() - Pull PPM value from the packet body
*
*/
void extractPPM
(
 A_UINT32 devNum,
 RX_STATS_TEMP_INFO *pStatsInfo
)
{
    LIB_DEV_INFO *pLibDev = gLibInfo.pLibDevArray[devNum];
    A_UINT32 pktData;
    A_INT32 ppm;
    double f_ppm;
    double scale_factor;
    A_BOOL        falconTrue = FALSE;
    A_UINT32      ppm_data_padding = PPM_DATA_SIZE;
    A_UINT32      ppm_byte_offset = 0;

    falconTrue = isFalcon(devNum) || isDragon(devNum);
    if (falconTrue) {
        ppm_byte_offset = 4;
        if (pLibDev->libCfgParams.chainSelect == 2) {
            ppm_data_padding = PPM_DATA_SIZE_FALCON_DUAL_CHAIN;
        } else {
            ppm_data_padding = PPM_DATA_SIZE_FALCON_SINGLE_CHAIN;
        }
    }

    // Only pull PPM from good packets
    if (pStatsInfo->status2 & DESC_FRM_RCV_OK) {
        if (isOwl(pLibDev->swDevID)) {
            /* Pull PPM from registers */
            ppm = REGR(devNum, 0x9CF4) & 0xFFF;
            // Sign extend ppm
            if ((ppm >> 11) == 1) {
                ppm = ppm | 0xfffff000;
            }
        } else {
            //get to start of packet body
            memcpy((A_UCHAR*)&pktData, &(pStatsInfo->ppm_data[0]), sizeof(pktData));
#ifdef ARCH_BIG_ENDIAN
            pktData = ltob_l(pktData);
#endif
            ppm = pktData & 0xffff;

            // Sign extend ppm
            if ((ppm >> 15) == 1) {
                ppm = ppm | 0xffff0000;
            }
        }
        // scale ppm to parts per million
        scale_factor = pLibDev->freqForResetDevice * 3.2768 / 1000;
        //scale ppm appropriately for turbo, half and quarter modes
        if (pLibDev->turbo == TURBO_ENABLE) {
            scale_factor = scale_factor / 2;
        } else if (pLibDev->turbo == HALF_SPEED_MODE) {
            scale_factor = scale_factor * 2;
        } else if (pLibDev->turbo == QUARTER_SPEED_MODE) {
            scale_factor = scale_factor * 4;
        }
        f_ppm = ppm/scale_factor;
        ppm = (A_INT32)(ppm /scale_factor);

        pStatsInfo->ppmAccum[0] += f_ppm;
        pStatsInfo->ppmSamples[0]++;
        pLibDev->rx.rxStats[pStatsInfo->qcuIndex][0].ppmAvg = (A_INT32)(pStatsInfo->ppmAccum[0] /
            pStatsInfo->ppmSamples[0]);

        /* max and min */
        if(ppm > pLibDev->rx.rxStats[pStatsInfo->qcuIndex][0].ppmMax) {
            pLibDev->rx.rxStats[pStatsInfo->qcuIndex][0].ppmMax = ppm;
        }
        if (ppm < pLibDev->rx.rxStats[pStatsInfo->qcuIndex][0].ppmMin) {
            pLibDev->rx.rxStats[pStatsInfo->qcuIndex][0].ppmMin = ppm;
        }

        pStatsInfo->ppmAccum[pStatsInfo->descRate] += f_ppm;
        pStatsInfo->ppmSamples[pStatsInfo->descRate]++;
        pLibDev->rx.rxStats[pStatsInfo->qcuIndex][pStatsInfo->descRate].ppmAvg = (A_INT32)(pStatsInfo->ppmAccum[pStatsInfo->descRate] /
            pStatsInfo->ppmSamples[pStatsInfo->descRate]);

        /* max and min */
        if (ppm > pLibDev->rx.rxStats[pStatsInfo->qcuIndex][pStatsInfo->descRate].ppmMax) {
            pLibDev->rx.rxStats[pStatsInfo->qcuIndex][pStatsInfo->descRate].ppmMax = ppm;
        }
        if (ppm < pLibDev->rx.rxStats[pStatsInfo->qcuIndex][pStatsInfo->descRate].ppmMin) {
            pLibDev->rx.rxStats[pStatsInfo->qcuIndex][pStatsInfo->descRate].ppmMin = ppm;
        }
        return;
    }
}

/**************************************************************************
* comparePktData() - Do a bit comparison on pkt data to find bit errors
*
* Note compare only compares the packet body.  It does not compare
* the header, since most of the header had to be correct to get to here
* ie all 3 addresses would already by correct
*
* RETURNS:
*/
void comparePktData
(
 A_UINT32   devNum,
 RX_STATS_TEMP_INFO *pStatsInfo
)
{
    A_UINT32 pktCompareAddr;
    A_UINT32 pktBodySize, initSize;
    A_UINT32 pktData;
    A_UINT32 compareWord;
    LIB_DEV_INFO *pLibDev = gLibInfo.pLibDevArray[devNum];
    A_UINT32 *pTempPtr;
    A_UINT32 mask;
    A_UINT32 mismatchBits;
    A_UINT32 *tmp_pktData;
    A_UCHAR  *intermediatePtr;
    A_BOOL   falconTrue = FALSE;
    A_UINT32 ppm_data_padding = PPM_DATA_SIZE;

    falconTrue = isFalcon(devNum) || isDragon(devNum);
    if (isOwl(pLibDev->swDevID)) {
        ppm_data_padding = 0;
    } else if (falconTrue) {
        if (pLibDev->libCfgParams.chainSelect == 2) {
            ppm_data_padding = PPM_DATA_SIZE_FALCON_DUAL_CHAIN;
        } else {
            ppm_data_padding = PPM_DATA_SIZE_FALCON_SINGLE_CHAIN;
        }
    }

    /* check to see if more bit is set in descriptor, compare does not
     * support this at the moment
     */
    if (pStatsInfo->status1 & DESC_MORE) {
        mError(devNum, EINVAL,
            "Device Number %d:comparePktData: Packet is contained in more than one descriptor, compare does not support this\n", devNum);
        return;
    }

    //get to start of packet body
    pktCompareAddr = pStatsInfo->bufferAddr + sizeof(WLAN_DATA_MAC_HEADER3) +
                     (pLibDev->wepEnable ? WEP_IV_FIELD : 0)
                     + sizeof(MDK_PACKET_HEADER);


    //calculate pkt body size
    pktBodySize = (pStatsInfo->status1 & DESC_DATA_LENGTH_FIELDS) - sizeof(WLAN_DATA_MAC_HEADER3)
                    -  (pLibDev->wepEnable ? WEP_IV_FIELD : 0)
                    - sizeof(MDK_PACKET_HEADER) - FCS_FIELD;
    initSize = pktBodySize;

    if(pLibDev->rx.enablePPM) {
        pktBodySize -= ppm_data_padding;
    }

    pTempPtr = (A_UINT32 *)pStatsInfo->pCompareData;
    //temp work around for predator, skip the first 4 bytes of buffer
//  pTempPtr++;
//  pktBodySize -=4;
    tmp_pktDataPtr = (A_UCHAR *) malloc(pktBodySize * sizeof(A_UCHAR));
    if (tmp_pktDataPtr) {
        if(pktBodySize > 2000) {
            pLibDev->devMap.OSmemRead(devNum, pktCompareAddr, (A_UCHAR *)tmp_pktDataPtr, 2000);
            intermediatePtr = (A_UCHAR *)tmp_pktDataPtr + 2000;
            pLibDev->devMap.OSmemRead(devNum, pktCompareAddr + 2000, intermediatePtr, pktBodySize - 2000);
        } else {
            pLibDev->devMap.OSmemRead(devNum, pktCompareAddr, (A_UCHAR *)tmp_pktDataPtr, pktBodySize);
        }
    } else {
        printf("Unable to allocate memory for reading pkt data\n");
        return;
    }

    tmp_pktData = (A_UINT32 *)tmp_pktDataPtr;
    while (pktBodySize) {
        //pLibDev->devMap.OSmemRead(devNum, pktCompareAddr, (A_UCHAR *)&pktData, sizeof(pktData));
        pktData = (A_UINT32)(*tmp_pktData);
        tmp_pktData++;
        compareWord = *pTempPtr;
        if (pktBodySize < 4) {
            //mask off bits won't compare
            mask = 0xffffffff >> ((4 - pktBodySize) * 8);
            pktData &= mask;
            compareWord &= mask;
            pktBodySize = 0;
        } else {
            pktBodySize -= 4;
        }

        mismatchBits = compareWord ^ pktData;
        if (mismatchBits) {
            /* there is a compare mismatch so count number of bits mismatch
             * and increment correct stats counter based on frame OK or not
             */
            if (pStatsInfo->status2 & DESC_FRM_RCV_OK) {
                pLibDev->rx.rxStats[pStatsInfo->qcuIndex][0].bitErrorCompares += countBits(mismatchBits);
                pLibDev->rx.rxStats[pStatsInfo->qcuIndex][pStatsInfo->descRate].bitErrorCompares += countBits(mismatchBits);
//              printf("snoop: error at %d/%d : mismatch bits = %d [0x%x==0x%x]\n", pktBodySize, initSize, countBits(mismatchBits), compareWord, pktData);
            } else {
                pLibDev->rx.rxStats[pStatsInfo->qcuIndex][0].bitMiscompares += countBits(mismatchBits);
                pLibDev->rx.rxStats[pStatsInfo->qcuIndex][pStatsInfo->descRate].bitMiscompares += countBits(mismatchBits);
            }
        }
        pTempPtr ++;
        pktCompareAddr += sizeof(pktCompareAddr);
    }
    free(tmp_pktDataPtr);

    return;
}

/**************************************************************************
* countBits() - Count number of bits set in 32 bit value
*
* Uses a byte wide lookup table
*
* RETURNS: Number of bits set
*/
A_UINT32 countBits
(
 A_UINT32 mismatchBits
)
{
    A_UINT32 count;

    count = bitCount[mismatchBits >> 24] + bitCount[(mismatchBits >> 16) & 0xff]
            + bitCount[(mismatchBits >> 8) & 0xff] + bitCount[mismatchBits & 0xff];

    return(count);
}

/**************************************************************************
* fillCompareBuffer() - Fill buffer with repeating input pattern
*
*
* RETURNS:
*/
void fillCompareBuffer
(
 A_UCHAR *pBuffer,
 A_UINT32 compareBufferSize,
 A_UCHAR *pDataPattern,
 A_UINT32 dataPatternLength
)
{
    A_UINT32 amountToWrite;

    //fill the buffer with the pattern
    while (compareBufferSize) {
        if (compareBufferSize > dataPatternLength) {
            amountToWrite = dataPatternLength;
            compareBufferSize -= dataPatternLength;
        } else {
            amountToWrite = compareBufferSize;
            compareBufferSize = 0;
        }
        memcpy(pBuffer, pDataPattern, amountToWrite);
        pBuffer += amountToWrite;
    }
}

/****************************************************************************
* setQueue() - set the Transmit queue number
*
*/

MANLIB_API void setQueue
(
    A_UINT32 devNum,
    A_UINT32 qcuNumber
)
{
    LIB_DEV_INFO *pLibDev = gLibInfo.pLibDevArray[devNum];

    if (checkDevNum(devNum) == FALSE) {
        mError(devNum, EINVAL, "Device Number %d:setQueue\n", devNum);
        return;
    }
    ar5kInitData[pLibDev->ar5kInitIndex].pMacAPI->setQueue(devNum, qcuNumber);
}

/****************************************************************************
* mapQueue() - map the Transmit QCU to a DCU
*
*/

MANLIB_API void mapQueue
(
    A_UINT32 devNum,
    A_UINT32 qcuNumber,
    A_UINT32 dcuNumber

)
{
    LIB_DEV_INFO *pLibDev = gLibInfo.pLibDevArray[devNum];

    if (checkDevNum(devNum) == FALSE) {
        mError(devNum, EINVAL, "Device Number %d:setQueue\n", devNum);
        return;
    }
    ar5kInitData[pLibDev->ar5kInitIndex].pMacAPI->mapQueue(devNum, qcuNumber,dcuNumber);
}

/****************************************************************************
* clearKeyCache() - Clear the Key Cache Table
*
*/

MANLIB_API void clearKeyCache
(
    A_UINT32 devNum
)
{
    LIB_DEV_INFO *pLibDev = gLibInfo.pLibDevArray[devNum];

    if (checkDevNum(devNum) == FALSE) {
        mError(devNum, EINVAL, "Device Number %d:setQueue\n", devNum);
        return;
    }
    ar5kInitData[pLibDev->ar5kInitIndex].pMacAPI->clearKeyCache(devNum);
}

/**************************************************************************
* mdkProcessMultiAntSigStrength() - Process receive descriptor statistics
*                                   for multi-antenna related MIMO stats
*
* Extract and save required statistics.
*
* RETURNS:
*/
void mdkProcessMultiAntSigStrength
(
 A_UINT32 devNum,
 RX_STATS_TEMP_INFO *pStatsInfo
)
{
    A_UINT32      ii, jj, kk;
    A_INT8        rssi;
    MULT_ANT_SIG_STRENGTH_STATS *pStatsMult;

    for (jj = 0; jj < 2; jj++) {
        /* Process into the desc Rate bin and the all rates bin (0) */
        kk = (jj>0) ? pStatsInfo->descRate : 0 ;
        pStatsMult = &(pStatsInfo->multAntSigStrength[kk]);

        for (ii = 0; ii < 4; ii++) {
            /* Process each of the 4 antenna signals */
            rssi = (A_INT8) (( (pStatsInfo->status3) >> (8*ii) ) & SIG_STRENGTH_MASK);

            // skip rssi = -128, they're invalid
            if (rssi > -128) {
                pStatsMult->rxAccumSignalStrengthAnt[ii] += rssi;
                pStatsMult->rxNumSigStrengthSamplesAnt[ii]++;
                pStatsMult->rxAvrgSignalStrengthAnt[ii] = (A_INT8) (pStatsMult->rxAccumSignalStrengthAnt[ii] /
                                                                 (A_INT32)(pStatsMult->rxNumSigStrengthSamplesAnt[ii]) );

                if (rssi > pStatsMult->rxMaxSigStrengthAnt[ii]) {
                    pStatsMult->rxMaxSigStrengthAnt[ii] = rssi;
                }

                if (rssi < pStatsMult->rxMinSigStrengthAnt[ii]) {
                    pStatsMult->rxMinSigStrengthAnt[ii] = rssi;
                }
            }
        }

        // Chain0AntSel
        pStatsInfo->Chain0AntSel[kk].Count[( pStatsInfo->status2 >> CHAIN_0_ANT_SEL_S) & 0x1];
        pStatsInfo->Chain0AntSel[kk].totalNum++;

        // Chain1AntSel
        pStatsInfo->Chain1AntSel[kk].Count[( pStatsInfo->status2 >> CHAIN_1_ANT_SEL_S) & 0x1];
        pStatsInfo->Chain1AntSel[kk].totalNum++;

        // Chain0AntReq
        pStatsInfo->Chain0AntReq[kk].Count[( pStatsInfo->status2 >> CHAIN_0_ANT_REQ_S) & 0x1];
        pStatsInfo->Chain0AntReq[kk].totalNum++;

        // Chain1AntReq
        pStatsInfo->Chain1AntReq[kk].Count[( pStatsInfo->status2 >> CHAIN_1_ANT_REQ_S) & 0x1];
        pStatsInfo->Chain1AntReq[kk].totalNum++;

        // Chain_Strong
        pStatsInfo->ChainStrong[kk].Count[( pStatsInfo->status2 >> CHAIN_STRONG_S) & 0x1];
        pStatsInfo->ChainStrong[kk].totalNum++;
    }
    devNum = 0; // quiet ye compiler
}

void fillRxDescAndFrame(A_UINT32 devNum, RX_STATS_TEMP_INFO *statsInfo) {

    LIB_DEV_INFO *pLibDev = gLibInfo.pLibDevArray[devNum];
    A_UINT32     frame_info_len;
    A_UCHAR      tmpFrameInfo[MAX_FRAME_INFO_SIZE];
    A_UINT32     ppm_data_padding = PPM_DATA_SIZE;
    A_UINT32     status1_offset, status2_offset, status3_offset; // legacy status1 - datalen/rate,
                                                                 // legacy status2 - done/error/timestamp
                                                                 // new with falcon status3 - rssi for 4 ant
	A_UINT32    timeStamp_offset = 0;

    if (isOwl(pLibDev->swDevID)) {
        ppm_data_padding = 0;
        status1_offset = FIRST_OWL_RX_STATUS_WORD; /* RXS5 */
        status2_offset = SECOND_OWL_RX_STATUS_WORD; /* ...,frame_rx_ok[1],done[0]*/ /* RXS12 */
        status3_offset = OWL_ANT_RSSI_RX_STATUS_WORD; /* RXS8 */
		timeStamp_offset = OWL_RX_TIME_STAMP_WORD; /* RXS6 */
    } else if (isFalcon(devNum) || isDragon(devNum)) {
        status1_offset = FIRST_FALCON_RX_STATUS_WORD;
        status2_offset = SECOND_FALCON_RX_STATUS_WORD;
        status3_offset = FALCON_ANT_RSSI_RX_STATUS_WORD;
        if (pLibDev->libCfgParams.chainSelect == 2) {
            ppm_data_padding = PPM_DATA_SIZE_FALCON_DUAL_CHAIN;
        } else {
            ppm_data_padding = PPM_DATA_SIZE_FALCON_SINGLE_CHAIN;
        }
    } else {
        status1_offset = FIRST_STATUS_WORD;
        status2_offset = SECOND_STATUS_WORD;
        status3_offset = status2_offset; // to avoid illegal value. should never be used besides multAnt chips
    }

    pLibDev->devMap.OSmemRead(devNum, statsInfo->descToRead,
                (A_UCHAR *)&(statsInfo->desc), sizeof(statsInfo->desc));

    /*
    {
    A_UINT32 *pTempPtr, jj;

    pTempPtr = (A_UINT32 *)(&(statsInfo->desc)) + FIRST_OWL_RX_STATUS_WORD/4;
    printf("\n");
    for(jj = 0; jj < 9; jj++) {
	    printf("%08x ", pTempPtr[jj]);
    }
    printf("\n");
    }
    */

    statsInfo->bufferAddr = (statsInfo->desc).bufferPhysPtr;
    statsInfo->status1 = *( (A_UINT32 *)((A_UCHAR *)(&(statsInfo->desc)) + status1_offset) );
    statsInfo->status2 = *( (A_UINT32 *)((A_UCHAR *)(&(statsInfo->desc)) + status2_offset) );
    statsInfo->status3 = *( (A_UINT32 *)((A_UCHAR *)(&(statsInfo->desc)) + status3_offset) );
    if (isOwl(pLibDev->swDevID)) {
        statsInfo->status4 = *( (A_UINT32 *)((A_UCHAR *)(&(statsInfo->desc)) + OWL_RX_PRIM_CHAIN_STATUS_WORD) );
    }
    statsInfo->totalBytes = (statsInfo->status1 & 0xfff);
	statsInfo->rxTimeStamp = *( (A_UINT32 *)((A_UCHAR *)(&(statsInfo->desc)) + timeStamp_offset) );

    if (statsInfo->status2 & DESC_DONE) {
		if(pLibDev->yesAgg == TRUE) {
          frame_info_len = sizeof(WLAN_QOS_DATA_MAC_HEADER3) + 2 + sizeof(MDK_PACKET_HEADER);
		} else {
          frame_info_len = sizeof(WLAN_DATA_MAC_HEADER3) + sizeof(MDK_PACKET_HEADER);
		}
        statsInfo->ppm_data_len = 0;
        if (pLibDev->rx.enablePPM) {
            frame_info_len += ppm_data_padding;
            statsInfo->ppm_data_len = ppm_data_padding;
        }

        pLibDev->devMap.OSmemRead(devNum, statsInfo->bufferAddr,
                    (A_UCHAR *)&(tmpFrameInfo), frame_info_len);

        //printf("SNOOP::fillRxDescAndFrame:enablePPM=%d:frame_info_len=%d\n", pLibDev->rx.enablePPM, frame_info_len);
        if (pLibDev->rx.enablePPM && !isOwl(pLibDev->swDevID)) {
            memcpy( &(statsInfo->ppm_data[0]), &(tmpFrameInfo[0]), ppm_data_padding);
            memcpy( &(statsInfo->frame_info[0]), &(tmpFrameInfo[0])+ppm_data_padding, (frame_info_len - ppm_data_padding));
        } else {
            memcpy( &(statsInfo->frame_info[0]), &(tmpFrameInfo[0]), frame_info_len);
        }
        statsInfo->frame_info_len = frame_info_len;

// Capture EVM stats if OWL 2.0 plus
		if(isOwl_2_Plus(pLibDev->macRev)){
			short ht40;
			A_UINT32 evm0, evm1, evm2;
			ht40 = (short)(*( (A_UINT32 *)((A_UCHAR *)(&(statsInfo->desc)) + 0x1c)));
			ht40 = (short)((ht40 >> 1) & 0x1);

			// check if packet received is of type ht40
			if(ht40 == 1) {
				evm0 = *( (A_UINT32 *)((A_UCHAR *)(&(statsInfo->desc)) + 0x24));
				evm1 = *( (A_UINT32 *)((A_UCHAR *)(&(statsInfo->desc)) + 0x28));
				evm2 =  *( (A_UINT32 *)((A_UCHAR *)(&(statsInfo->desc)) + 0x2c));
				statsInfo->evm_stream0 = (((evm0 >> 24) & 0xff) + ((evm0 >> 16) & 0xff)
					+ ((evm0 >> 8) & 0xff) + (evm0 & 0xff) + ((evm1 >> 8) & 0xff) + (evm1 & 0xff))/6;

				statsInfo->evm_stream1 = (((evm1 >> 24) & 0xff) + ((evm1 >> 16) & 0xff)
					+ ((evm2 >> 24) & 0xff) + ((evm2 >> 16) & 0xff) + ((evm2 >> 8) & 0xff) + (evm2 & 0xff))/6;
			} else {
				evm0 = *( (A_UINT32 *)((A_UCHAR *)(&(statsInfo->desc)) + 0x24));
				evm1 = *( (A_UINT32 *)((A_UCHAR *)(&(statsInfo->desc)) + 0x28));
				statsInfo->evm_stream0 = (((evm0 >> 24) & 0xff) + ((evm0 >> 16) & 0xff)
					+ ((evm0 >> 8) & 0xff) + (evm0 & 0xff))/4;
				statsInfo->evm_stream1 = (((evm1 >> 24) & 0xff) + ((evm1 >> 16) & 0xff)
					+ ((evm1 >> 8) & 0xff) + (evm1 & 0xff))/4;

//				printf("EVM: %d,  %d\n", statsInfo->evm_stream0, statsInfo->evm_stream1);

			}

		}

	}
}

#if defined(__ATH_DJGPPDOS__)
/**************************************************************************
* milliTime - return time in milliSeconds
*
* This routine calls a OS specific routine for gettting the time
*
* RETURNS: time in milliSeconds
*/

A_UINT32 milliTime
(
    void
)
{
    A_UINT32 timeMilliSeconds;
    A_UINT64 tempTime = 0;

    tempTime = uclock() * 1000;
    timeMilliSeconds = (tempTime)/UCLOCKS_PER_SEC;
    return timeMilliSeconds;

}
#endif
