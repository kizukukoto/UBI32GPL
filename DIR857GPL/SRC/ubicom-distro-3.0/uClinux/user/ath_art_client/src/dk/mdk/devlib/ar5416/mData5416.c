/*
 *  Copyright © 2005 Atheros Communications, Inc.,  All Rights Reserved.
 *
 */

#ifdef VXWORKS
#include "vxworks.h"
#endif

#ifdef __ATH_DJGPPDOS__
#define __int64 long long
typedef unsigned long DWORD;
#define Sleep   delay
#endif  // #ifdef __ATH_DJGPPDOS__

#include <errno.h>
#include <assert.h>

#ifndef __ATH_DJGPPDOS__
#include "mData513.h"
#include "mIds.h"
#ifdef Linux
#include "../athreg.h"
#include "../manlib.h"
#include "../mEeprom.h"
#else
#include "..\athreg.h"
#include "..\manlib.h"
#include "..\mEeprom.h"
#endif
#endif

#ifdef Linux
#include "../mConfig.h"
#else
#include "..\mConfig.h"
#endif

#include "ar5416reg.h"

static void enable5416QueueClocks(
    A_UINT32 devNum,
    A_UINT32 qcuIndex,
    A_UINT32 dcuIndex
);
/*
#if (!defined(WIN32) || defined(_DEBUG)) 
A_UINT32 numBits(A_UINT32 data) {
    A_UINT32 i, result = 0;
    for (i = 0; i < sizeof(A_UINT32) * 8; i++) {
        if ((data >> i) & 0x1) {
            result++;
        }
    }
    return result;
}
#endif
*/
void macAPIInitAr5416
(
 A_UINT32 devNum
)
{
    LIB_DEV_INFO *pLibDev = gLibInfo.pLibDevArray[devNum];
    if (IS_MAC_5416_2_0_UP(pLibDev->macRev)) {
        pLibDev->txDescStatus1 = FIRST_5416_2_TX_STATUS_WORD;
        pLibDev->txDescStatus2 = SECOND_5416_2_TX_STATUS_WORD;
    } else {
        pLibDev->txDescStatus1 = FIRST_5416_TX_STATUS_WORD;
        pLibDev->txDescStatus2 = SECOND_5416_TX_STATUS_WORD;
    }
    pLibDev->decryptErrMsk = VENICE_DESC_DECRYPT_ERROR;
    pLibDev->bitsToRxSigStrength = OWL_BITS_TO_RX_SIG_STRENGTH;
    pLibDev->rxDataRateMsk = 0xff;

    return;
}

A_UINT32 setupAntennaAr5416
(
 A_UINT32 devNum,
 A_UINT32 antenna,
 A_UINT32* antModePtr   // retVal used by setDescr, createDescr etc...
)
{
    /* Nothing to be done for now... use .cfg switch table */
    devNum = 0; // quiet compiler
    antenna = 0; // quiet compiler
    antModePtr = 0; // quiet compiler
    return 1;
}


void writeRxDescriptorAr5416
(
 A_UINT32 devNum,
 A_UINT32 rxDescAddress
)
{
    REGW(devNum, F2_RXDP, rxDescAddress);
}

void setDescriptorAr5416
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
    A_UINT16     queueIndex = pLibDev->selQueueIndex;
    A_UINT32     frameLen = 0;
    MDK_5416_DESC    *local5416DescPtr = (MDK_5416_DESC *)localDescPtr;
    MDK_5416_2_DESC  *local5416v2DescPtr = (MDK_5416_2_DESC *)localDescPtr;
    A_UINT32     numAttempts;
    A_UINT32     dest_idx = (broadcast ? FALCON_BROADCAST_DEST_INDEX : FALCON_UNICAST_DEST_INDEX);
    A_UINT32     ht40, short_gi, dup, ext_only;
    A_UINT32     insert_timestamp = 1;

    antMode = 0; // to quiet warnings. not needed for falcon

    //check for this being an xr rate
    if (rateIndex >= LOWEST_XR_RATE_INDEX && rateIndex <= HIGHEST_XR_RATE_INDEX) {
        assert(1); // XR not supported by 5416
    }

    //create and write the control words
    frameLen = (pktSize + FCS_FIELD) + ((pLibDev->wepEnable && (descNum < pLibDev->tx[queueIndex].numDesc)) ? WEP_ICV_FIELD : 0);

    dup = !IS_HT40_RATE_INDEX(rateIndex) && pLibDev->libCfgParams.ht40_enable && (pLibDev->libCfgParams.extended_channel_op & EXT_CHANNEL_DUP_MASK);
    ext_only = !IS_HT40_RATE_INDEX(rateIndex) && pLibDev->libCfgParams.ht40_enable && (pLibDev->libCfgParams.extended_channel_op & EXT_CHANNEL_ONLY_MASK);
    assert(dup + ext_only <= 1);

    //for now only setup one group of retries
    //add 1 to retries to account for 1st attempt (but if retries is 15, don't add at moment)
    numAttempts = pLibDev->tx[queueIndex].retryValue;
    if (numAttempts < 0xf) {
        numAttempts++;
    }
    // data rate checks
    assert(rateIndex < numRateCodes);
//    assert((numBits(pLibDev->libCfgParams.tx_chain_mask) > 1) || !IS_2STREAM_RATE_INDEX(rateIndex));
    short_gi = pLibDev->libCfgParams.short_gi_enable && IS_HT40_RATE_INDEX(rateIndex);
    ht40 = pLibDev->libCfgParams.ht40_enable && (IS_HT40_RATE_INDEX(rateIndex) || dup);
    assert(pLibDev->libCfgParams.tx_chain_mask); // Needs to be at least 1

    if (IS_MAC_5416_2_0_UP(pLibDev->macRev)) {
        local5416v2DescPtr->hwControl[0] = frameLen | 0x01000000; //clear dest mask
        if (descNum == pLibDev->tx[queueIndex].numDesc - 1) {
            local5416v2DescPtr->hwControl[0] |= DESC_TX_INTER_REQ;
        }
        local5416v2DescPtr->hwControl[1] = pktSize | (broadcast ? (1 << BITS_TO_VENICE_NOACK) : 0) | (dest_idx << 13);
        local5416v2DescPtr->hwControl[1] |= (dup << 28) | (ext_only << 27);
        local5416v2DescPtr->hwControl[1] |= (insert_timestamp << 25) ; // needed to get reliable timestamps. PD 5/17/06
        local5416v2DescPtr->hwControl[2] = numAttempts << BITS_TO_DATA_TRIES0;
        local5416v2DescPtr->hwControl[3] = rateValues[rateIndex] << OWL_BITS_TO_TX_DATA_RATE0;
        local5416v2DescPtr->hwControl[4] = 0; // pktDur0, pktDur1 = 0
        local5416v2DescPtr->hwControl[5] = 0; // pktDur2, pktDur3 = 0
        local5416v2DescPtr->hwControl[6] = 0; // agg, encrypt
        
        if(pLibDev->swDevID == SW_DEVICE_ID_OWL){
            local5416v2DescPtr->hwControl[7] = ((pLibDev->libCfgParams.tx_chain_mask | 0x1) & 0x7) << 2 | (short_gi << 1) | ht40; // 20_40, GI, chain_sel
        } else {
            local5416v2DescPtr->hwControl[7] = ((pLibDev->libCfgParams.tx_chain_mask) & 0x7) << 2 | (short_gi << 1) | ht40; // 20_40, GI, chain_sel
        }

        local5416v2DescPtr->hwControl[8] = 0;
        local5416v2DescPtr->hwControl[9] = 0;
        local5416v2DescPtr->hwControl[10] = 0;
        local5416v2DescPtr->hwControl[11] = 0;
    } else {
        local5416DescPtr->hwControl[0] = frameLen | 0x01000000; //clear dest mask
        if (descNum == pLibDev->tx[queueIndex].numDesc - 1) {
            local5416DescPtr->hwControl[0] |= DESC_TX_INTER_REQ;
        }
        local5416DescPtr->hwControl[1] = pktSize | (broadcast ? (1 << BITS_TO_VENICE_NOACK) : 0) | (dest_idx << 13);
        local5416DescPtr->hwControl[1] |= (dup << 28) | (ext_only << 27);
        local5416DescPtr->hwControl[2] = numAttempts << BITS_TO_DATA_TRIES0;
        local5416DescPtr->hwControl[3] = rateValues[rateIndex] << OWL_BITS_TO_TX_DATA_RATE0;
        local5416DescPtr->hwControl[4] = 0; // pktDur0, pktDur1 = 0
        local5416DescPtr->hwControl[5] = 0; // pktDur2, pktDur3 = 0
        local5416DescPtr->hwControl[6] = 0; // agg, encrypt
        if(pLibDev->swDevID == SW_DEVICE_ID_OWL){
            local5416DescPtr->hwControl[7] = ((pLibDev->libCfgParams.tx_chain_mask | 0x1) & 0x7) << 2 | (short_gi << 1) | ht40; // 20_40, GI, chain_sel
        } else {
            local5416DescPtr->hwControl[7] = ((pLibDev->libCfgParams.tx_chain_mask) & 0x7) << 2 | (short_gi << 1) | ht40; // 20_40, GI, chain_sel
        }
    }
}

/**************************************************************************
* txBeginConfig - program register ready for tx begin
*
*/
void txBeginConfigAr5416
(
 A_UINT32 devNum,
 A_BOOL   enableInterrupt
)
{
    LIB_DEV_INFO *pLibDev = gLibInfo.pLibDevArray[devNum];
    A_UINT32 enableFlag = 0;
    A_UINT32 i = 0;
    A_UINT32 qcu = 0;
    A_UINT16 activeQueues[MAX_TX_QUEUE] = {0};  // Index of the active queue
    A_UINT16 activeQueueCount = 0;  // active queues count
    A_UINT32 imrS0,imrS0desc;

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

        if (enableInterrupt) {
            imrS0desc = imrS0desc | qcu;
//          REGW(devNum, F2_IMR_S0, (F2_IMR_S0_QCU_TXDESC_M & (qcu << F2_IMR_S0_QCU_TXDESC_S)));
        }

        //enable the clocks
        enable5416QueueClocks(devNum, activeQueues[i], pLibDev->tx[activeQueues[i]].dcuIndex);
    }
    // Program the IMR_SO Register
    imrS0desc = imrS0desc << F2_IMR_S0_QCU_TXDESC_S;
    imrS0 = (REGR(devNum,F2_IMR_S0) & F2_IMR_S0_QCU_TXDESC_M) | imrS0desc;
    REGW(devNum, F2_IMR_S0, imrS0);

    // QUEUE SETUP
    //enable tx DESC interrupt
    if (enableInterrupt) {
        REGW(devNum, F2_IMR, REGR(devNum, F2_IMR) | F2_IMR_TXDESC);

        //clear ISR before enable
        REGR(devNum, F2_ISR_RAC);
        REGW(devNum, F2_IER, F2_IER_ENABLE);
    }

    //write TXDP
    for( i = 0; i < MAX_TX_QUEUE; i++ )
    {
        if ( pLibDev->tx[i].txEnable )
        {
            REGW(devNum,  F2_Q0_TXDP + (4 * i), pLibDev->tx[i].descAddress);
            enableFlag |= F2_QCU_0 << i;
        }
    }

    //improve performance
    REGW(devNum, 0x1230, 0x10);

    //enable unicast reception - needed to receive acks
    REGW(devNum, F2_RX_FILTER, REGR(devNum, F2_RX_FILTER) | F2_RX_UCAST);
    //enable receive
    REGW(devNum, F2_DIAG_SW, REGR(devNum, F2_DIAG_SW) & ~F2_DIAG_RX_DIS);

    REGW(devNum, F2_CR, F2_CR_RXE);

	// set preserve seqNum bit if aggregation is requested
	if(pLibDev->yesAgg == 1) {
		REGW(devNum, 0x8004, 0x20000000 | REGR(devNum, 0x8004));
	}

    //write TX enable bit
    REGW(devNum, F2_Q_TXE, REGR(devNum, F2_Q_TXE) | enableFlag );

    return;
}

void setContDescriptorAr5416(
 A_UINT32 devNum,
 MDK_ATHEROS_DESC*  localDescPtr,
 A_UINT32 pktSize,
 A_UINT32 antMode,
 A_UCHAR  dataRate
)
{
    LIB_DEV_INFO *pLibDev = gLibInfo.pLibDevArray[devNum];
    MDK_5416_DESC    *local5416DescPtr = (MDK_5416_DESC *)localDescPtr;
    MDK_5416_2_DESC  *local5416v2DescPtr = (MDK_5416_2_DESC *)localDescPtr;
    A_UINT32 ht40, short_gi, dup, ext_only, stbc;

    dup = !IS_HT40_RATE_INDEX(dataRate) && pLibDev->libCfgParams.ht40_enable && (pLibDev->libCfgParams.extended_channel_op & EXT_CHANNEL_DUP_MASK);
    ext_only = !IS_HT40_RATE_INDEX(dataRate) && pLibDev->libCfgParams.ht40_enable && (pLibDev->libCfgParams.extended_channel_op & EXT_CHANNEL_ONLY_MASK);
    assert(dup + ext_only <= 1);
    // data rate checks
    assert(dataRate < numRateCodes);
//    assert((numBits(pLibDev->libCfgParams.tx_chain_mask) > 1) || !IS_2STREAM_RATE_INDEX(dataRate));
    short_gi = pLibDev->libCfgParams.short_gi_enable && IS_HT40_RATE_INDEX(dataRate);
    ht40 = pLibDev->libCfgParams.ht40_enable && (IS_HT40_RATE_INDEX(dataRate) || dup);
    stbc = pLibDev->libCfgParams.stbc_enable;
    assert(pLibDev->libCfgParams.tx_chain_mask); // Needs to be at least 1

    if (IS_MAC_5416_2_0_UP(pLibDev->macRev)) {
        local5416v2DescPtr->hwControl[0] = pktSize + FCS_FIELD;
        local5416v2DescPtr->hwControl[1] = pktSize | FALCON_UNICAST_DEST_INDEX;
        local5416v2DescPtr->hwControl[1] |= (dup << 28) | (ext_only << 27);
        local5416v2DescPtr->hwControl[2] = 0xf << BITS_TO_DATA_TRIES0;
        local5416v2DescPtr->hwControl[3] = rateValues[dataRate] << OWL_BITS_TO_TX_DATA_RATE0;
        local5416v2DescPtr->hwControl[4] = 0; // pktDur0, pktDur1 = 0
        local5416v2DescPtr->hwControl[5] = 0; // pktDur2, pktDur3 = 0
        local5416v2DescPtr->hwControl[6] = 0; // agg, encrypt
        if(isMerlin(pLibDev->swDevID)) {
            local5416v2DescPtr->hwControl[7] = ((stbc << 28) |
                                                ((pLibDev->libCfgParams.tx_chain_mask) & 0x7) << 2) |
                                                (short_gi << 1) |
                                                ht40; // 20_40, GI, chain_sel, stbc mode
        } else {
            if(pLibDev->swDevID == SW_DEVICE_ID_OWL){
                local5416v2DescPtr->hwControl[7] = ((pLibDev->libCfgParams.tx_chain_mask | 0x1) & 0x7) << 2 | (short_gi << 1) | ht40; // 20_40, GI, chain_sel
            } else {
                local5416v2DescPtr->hwControl[7] = ((pLibDev->libCfgParams.tx_chain_mask) & 0x7) << 2 | (short_gi << 1) | ht40; // 20_40, GI, chain_sel
            }
        }
        local5416v2DescPtr->hwControl[8] = 0;
        local5416v2DescPtr->hwControl[9] = 0;
        local5416v2DescPtr->hwControl[10] = 0;
        local5416v2DescPtr->hwControl[11] = 0;
    } else {
        local5416DescPtr->hwControl[0] = pktSize + FCS_FIELD;
        local5416DescPtr->hwControl[1] = pktSize | FALCON_UNICAST_DEST_INDEX;
        local5416DescPtr->hwControl[1] |= (dup << 28) | (ext_only << 27);

        //set max attempts for 1 exchange for better performance
        local5416DescPtr->hwControl[2] = 0xf << BITS_TO_DATA_TRIES0;
        local5416DescPtr->hwControl[3] = rateValues[dataRate] << OWL_BITS_TO_TX_DATA_RATE0;
        local5416DescPtr->hwControl[4] = 0; // pktDur0, pktDur1 = 0
        local5416DescPtr->hwControl[5] = 0; // pktDur2, pktDur3 = 0
        local5416DescPtr->hwControl[6] = 0; // agg, encrypt
        if(pLibDev->swDevID == SW_DEVICE_ID_OWL){
            local5416DescPtr->hwControl[7] = ((pLibDev->libCfgParams.tx_chain_mask | 0x1) & 0x7) << 2 | (short_gi << 1) | ht40; // 20_40, GI, chain_sel    
        } else {
            local5416DescPtr->hwControl[7] = ((pLibDev->libCfgParams.tx_chain_mask) & 0x7) << 2 | (short_gi << 1) | ht40; // 20_40, GI, chain_sel
        }

    }
//#ifdef _IQV
//	local5416DescPtr->hwControl[2] = 0;	// for 0 retries.
//#endif // _IQV
    antMode = 0;    //this is not used quieting warnings
}

A_UINT32 txGetDescRateAr5416
(
 A_UINT32 devNum,
 A_UINT32 descAddr,
 A_BOOL   *ht40,
 A_BOOL   *shortGi
)
{
    LIB_DEV_INFO *pLibDev = gLibInfo.pLibDevArray[devNum];
    A_UINT32 descRate, ht40Word;

    pLibDev->devMap.OSmemRead(devNum, descAddr + FOURTH_CONTROL_WORD,
                        (A_UCHAR *)&(descRate), sizeof(descRate));

    pLibDev->devMap.OSmemRead(devNum, descAddr + 36,
							(A_UCHAR *)&(ht40Word), sizeof(ht40Word));
    *ht40 = (ht40Word & 0x1) && 1;
    *shortGi = ((ht40Word >> 1) & 0x1) && 1;

    descRate = (descRate >> OWL_BITS_TO_TX_DATA_RATE0) & 0xFF;
    return(descRate);
}

void enable5416QueueClocks
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

void txBeginContDataAr5416
(
 A_UINT32 devNum,
 A_UINT16 queueIndex
)
{
    LIB_DEV_INFO *pLibDev = gLibInfo.pLibDevArray[devNum];
    A_UINT16    activeQueues[MAX_TX_QUEUE] = {0};   // Index of the active queue
    A_UINT16    activeQueueCount = 0;   // active queues count
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

        //enable the clocks
        enable5416QueueClocks(devNum, activeQueues[i], pLibDev->tx[activeQueues[i]].dcuIndex);
    }

    //write TXDP
    for( i = 0; i < MAX_TX_QUEUE; i++ )
    {
        if ( pLibDev->tx[i].txEnable )
        {
            REGW(devNum,  F2_Q0_TXDP + (4 * i), pLibDev->tx[i].descAddress);
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

    queueIndex = 0; //this is not used quieting warnings
}

void txBeginContFramedDataAr5416
(
 A_UINT32 devNum,
 A_UINT16 queueIndex,
 A_UINT32 ifswait,
 A_BOOL   retries
)
{
    LIB_DEV_INFO *pLibDev = gLibInfo.pLibDevArray[devNum];
    A_UINT16    activeQueues[MAX_TX_QUEUE] = {0};   // Index of the active queue
    A_UINT16    activeQueueCount = 0;   // active queues count
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
        enable5416QueueClocks(devNum, activeQueues[i], pLibDev->tx[activeQueues[i]].dcuIndex);
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
            REGW(devNum,  F2_Q0_TXDP + (4 * i), pLibDev->tx[i].descAddress);
            enableFlag |= F2_QCU_0 << i;
        }
    }

    //Increase the interframe spacing to improve the performance
    if( pLibDev->libCfgParams.ht40_enable ) {
        REGW(devNum, F2_D_GBL_IFS_SIFS, 0x800); //sifs, 0x64
    }
    else {
        REGW(devNum, F2_D_GBL_IFS_SIFS, 0x400); //sifs, 0x64
    }

    REGW(devNum, F2_D_GBL_IFS_EIFS, 100); //eifs
    REGW(devNum, F2_D_FPCTL, 0x10); //enable prefetch
    REGW(devNum, F2_TIME_OUT, 0x2);  //ACK timeout

    //write TX enable bit, clear any disable bits set first
    REGW(devNum, F2_Q_TXD, 0);

    REGW(devNum, F2_Q_TXE, REGR(devNum, F2_Q_TXE) | enableFlag );
    REGW(devNum, F2_CR, F2_CR_RXE);

    queueIndex = 0; //this is not used quieting warnings
}

void setDescriptorEndPacketAr5416
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
//    LIB_DEV_INFO *pLibDev = gLibInfo.pLibDevArray[devNum];
//    A_UINT32    numAttempts;
//    A_UINT16    queueIndex = pLibDev->selQueueIndex;

//  send end packet at rate 6
	rateIndex = 32;
//	setDescriptorAr5416(devNum, localDescPtr, pktSize, antMode, descNum, rateIndex, broadcast);
    setDescriptorAr5416(devNum, localDescPtr, pktSize, antMode, descNum, 0, broadcast);

/*
    // Added for 2nd set of tries if the 1st one fails.
	// Here 1st one is done at rate 6 and 2nd one is at ht20 MSC 0.
    numAttempts = pLibDev->tx[queueIndex].retryValue;
    if (numAttempts < 0xf) {
        numAttempts++;
    }

    localDescPtr->hwControl[2] = localDescPtr->hwControl[2] | (numAttempts << BITS_TO_DATA_TRIES1);
    localDescPtr->hwControl[3] = localDescPtr->hwControl[3] | (rateValues[32] << OWL_BITS_TO_TX_DATA_RATE1);
*/
}

void rxBeginConfigAr5416
(
 A_UINT32 devNum
)
{
    A_UINT32    regValue = 0;
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
    REGW(devNum, F2_RXDP, pLibDev->rx.descAddress);

#ifdef _DEBUG
    if(REGR(devNum, F2_RXDP) != (pLibDev->rx.descAddress)) {
        mError(devNum, EIO, "F2_RXDP (0x%x) does not equal pLibDev->rx.descAddress (0x%x)\n", REGR(devNum, F2_RXDP), pLibDev->rx.descAddress);
    }
#endif

    //enable unicast and broadcast reception
    if(pLibDev->rx.rxMode == RX_FIXED_NUMBER) {
        REGW(devNum, F2_RX_FILTER, REGR(devNum, F2_RX_FILTER) | F2_RX_BEACON );
    }
    else {
        REGW(devNum, F2_RX_FILTER, REGR(devNum, F2_RX_FILTER) | F2_RX_UCAST | F2_RX_BCAST );
//	printf("TEST:: Setting up promiscuous mode\n");
//		_IQV to disable MAC address in Per.
//      REGW(devNum, F2_RX_FILTER, REGR(devNum, F2_RX_FILTER) | F2_RX_UCAST | F2_RX_BCAST |F2_RX_PROM );
    }

    //enable receive
    REGW(devNum, F2_DIAG_SW, REGR(devNum, F2_DIAG_SW) & ~F2_DIAG_RX_DIS);

    //write RX enable bit
    REGW(devNum, F2_CR, REGR(devNum, F2_CR) | F2_CR_RXE);

    return;
}

void setPPM5416
(
 A_UINT32 devNum,
 A_UINT32 enablePPM
)
{
    LIB_DEV_INFO *pLibDev = gLibInfo.pLibDevArray[devNum];

    if(enablePPM) {
        REGW(devNum, PHY_CHAN_INFO, REGR(devNum, PHY_CHAN_INFO) | 1);
        pLibDev->rx.enablePPM = 1;
    } else {
//		i = REGR(devNum, PHY_CHAN_INFO);
//		j=0;
//		while ((j<5) && ((i&1) != 0)){
//			printf("j=%d, reg=0x%x\n",j,i);
//			i = REGR(devNum, PHY_CHAN_INFO);
//			j++;
//		}
        // ChanInfo is self clearing for 5416
        assert((REGR(devNum, PHY_CHAN_INFO) & 1) == 0);
        pLibDev->rx.enablePPM = 0;
    }
}

void setStatsPktDescAr5416
(
 A_UINT32 devNum,
 MDK_ATHEROS_DESC*  localDescPtr,
 A_UINT32 pktSize,
 A_UINT32 rateValue
)
{
    LIB_DEV_INFO *pLibDev = gLibInfo.pLibDevArray[devNum];
    A_UINT16    queueIndex = pLibDev->selQueueIndex;
	A_UINT16 tempNumAttempts;

	//save off the library copy of num attempts so can set to max for descriptor setup
	tempNumAttempts = (A_UINT16) pLibDev->tx[queueIndex].retryValue;
	pLibDev->tx[queueIndex].retryValue = 0xf;

	//send these at HT20, MCS0
	rateValue = 32;
	setDescriptorAr5416(devNum, localDescPtr, pktSize, 0, 0, rateValue, FALSE);
	//restore original num attempts
	pLibDev->tx[queueIndex].retryValue = tempNumAttempts;


    return;
}

void beginSendStatsPktAr5416
(
 A_UINT32 devNum,
 A_UINT32 DescAddress
)
{
    REGW(devNum, F2_D0_RETRY_LIMIT, (REGR(devNum, F2_D0_RETRY_LIMIT) | MAX_RETRIES));

    REGW(devNum, F2_D0_QCUMASK , REGR(devNum, F2_D0_QCUMASK) | F2_QCU_0 );

    //enable the clocks
    enable5416QueueClocks(devNum, 0, 0);

    //clear the waitforPoll bit, incase also enabled for receive
    REGW(devNum, F2_Q0_TXDP, DescAddress);
    REGW(devNum, F2_Q_TXE, REGR(devNum, F2_Q_TXE) | F2_QCU_0);
}

/**************************************************************************
* rxCleanupConfig - program register ready for rx begin
*
*/
void rxCleanupConfigAr5416
(
 A_UINT32 devNum
)
{
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

A_UINT32 sendTxEndPacketAr5416
(
 A_UINT32 devNum,
 A_UINT16 queueIndex
)
{
    LIB_DEV_INFO *pLibDev = gLibInfo.pLibDevArray[devNum];
    A_UINT32 nRet = 1;
    A_UINT32 i = 0;
    A_UINT32 index = pLibDev->tx[queueIndex].dcuIndex;  // DCU 0...10 (11)
    A_UINT32    status1 = 0;
    A_UINT32    status2 = 0;
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
//    writeField(devNum, "mc_sifs_dcu", tempValue);
    tempValue = getFieldForMode(devNum, "mc_ack_timeout", pLibDev->mode, pLibDev->turbo);
//    writeField(devNum, "mc_ack_timeout", tempValue);

    //write TXDP
    REGW(devNum, F2_Q0_TXDP + ( 4 * queueIndex), pLibDev->tx[queueIndex].endPktDesc);

    //write TX enable bit
    REGW(devNum, F2_Q_TXE, REGR(devNum, F2_Q_TXE) | ( F2_QCU_0 << queueIndex ) );

    //wait for this packet to complete, by polling the done bit
    for(i = 0; i < MDK_PKT_TIMEOUT; i++) {
        pLibDev->devMap.OSmemRead(devNum, pLibDev->tx[queueIndex].endPktDesc + pLibDev->txDescStatus2,
                (A_UCHAR *)&status2, sizeof(status2));
        if(status2 & DESC_DONE) {
            pLibDev->devMap.OSmemRead(devNum, pLibDev->tx[queueIndex].endPktDesc + pLibDev->txDescStatus1,
                (A_UCHAR *)&status1, sizeof(status1));
            // If not ok and not sending broadcast frames (i.e. no response required)
            if( (!(status1 & DESC_FRM_XMIT_OK)) && (pLibDev->tx[queueIndex].broadcast == 0) ) {
#ifdef _DEBUG_MEM
                printf("End Desc contents \n");
                memDisplay(devNum, pLibDev->tx[queueIndex].endPktDesc, sizeof(MDK_ATHEROS_DESC)/sizeof(A_UINT32));
#endif
                mError(devNum, EIO, "sendTxEndPacket5416: end packet not successfully sent, status = 0x%04X\n",
                            status1);
                nRet = 0;
                return nRet;
            }
            break;
        }
        mSleep(1);
    }

    if (i == (MDK_PKT_TIMEOUT * 5)) {
        mError(devNum, EIO, "sendTxEndPacket5416: timed out waiting for end packet done, QCU (0-15) = %d\n",
                            queueIndex);
        nRet = 0;
    }
    return nRet;
}
