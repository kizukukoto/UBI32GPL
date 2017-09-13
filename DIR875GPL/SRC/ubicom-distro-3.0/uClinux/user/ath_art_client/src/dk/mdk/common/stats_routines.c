#ifdef __ATH_DJGPPDOS__
#include <unistd.h>
#ifndef EILSEQ  
    #define EILSEQ EIO
#endif  // EILSEQ

 #define __int64    long long
 #define HANDLE long
 typedef unsigned long DWORD;
 #define Sleep  delay
 #include <bios.h>
 #include <dir.h>
#endif  // #ifdef __ATH_DJGPPDOS__

#ifdef ECOS
#include "dk_common.h"
#endif

#include "wlantype.h"
#include "athreg.h"
#include "manlib.h"
#include "mdata.h"
#include <string.h>
#include <stdlib.h>
#if defined (THIN_CLIENT_BUILD) || defined (WIN_CLIENT_BUILD)
#include "hwext.h"
#else
#include "errno.h"
#include "common_defs.h"
#include "mDevtbl.h"
#endif

#include "mConfig.h"

#ifdef	VER1
#define TIME_STAMP_OFFSET	48
#else
#define TIME_STAMP_OFFSET	64
#endif


//extern A_UINT32 debug_pkt_num;
//static FILE *fpDebug;

void getSignalStrengthStats
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
    if(signalStrength > pStats->rxMaxSigStrength)
        pStats->rxMaxSigStrength = signalStrength;
    
    if (signalStrength < pStats->rxMinSigStrength)
        pStats->rxMinSigStrength = signalStrength;
    return;
}


void fillRateThroughput
(
 TX_STATS_STRUCT *txStats,
 A_UINT32 descRate, 
 A_UINT32 dataBodyLen
)
{
    A_UINT32 descStartTime;
    A_UINT32 descEndTime;
    A_UINT32 bitsReceived;
    float newTxTime = 0;
    
    descStartTime = txStats[descRate].startTime;
    descEndTime = txStats[descRate].endTime;
    if(descStartTime < descEndTime) {
        bitsReceived = (txStats[descRate].goodPackets - txStats[descRate].firstPktGood)  \
            * (dataBodyLen + sizeof(MDK_PACKET_HEADER)) * 8;
        newTxTime = (float)((descEndTime - descStartTime) * 1024)/1000;  //puts the time in millisecs
        txStats[descRate].newThroughput = (A_UINT32)((float)bitsReceived / newTxTime);
    }
}

void extractTxStats
(
 TX_STATS_TEMP_INFO *pStatsInfo,
 TX_STATS_STRUCT *txStats,
 A_BOOL yesAgg,
 A_UINT32 aggSize
)
{
	A_UINT32 i;
    A_UINT16 numRetries;
    //fprintf(fpDebug, "fr_xmit_ok: 0x%0x, ba_status: 0x%0x, ba_0_31: 0x%0x\n", (pStatsInfo->status1&0x1), pStatsInfo->baStatus, pStatsInfo->blockAck0_31);

    // first check for good packets
    if (pStatsInfo->status1 & DESC_FRM_XMIT_OK) 
    {
	  // Check if this was a aggregated frame
	  if(yesAgg == TRUE)
	  {
		// Count good packets only if block ack status is 1
		if(pStatsInfo->baStatus != 0) {
		  // look at the blk ack bit map for good packet count
		  if(aggSize>32){
			// go through bits of ba_0_31
			for(i=0; i<32; i++)
			{
				if(((pStatsInfo->blockAck0_31 >> i) & 1) == 1) {
					// This is a good packet
			        txStats[0].goodPackets ++;
			        txStats[pStatsInfo->descRate].goodPackets ++;
				}
			}
			// go through bits of ba_32_63
			for(;i<aggSize; i++)
			{
				if(((pStatsInfo->blockAck32_63 >> (i-32))& 1) == 1) {
					// This is a good packet
			        txStats[0].goodPackets ++;
			        txStats[pStatsInfo->descRate].goodPackets ++;
				}
			}
		  } else {
   		    // go through bits of ba_0_31
		    for(i=0; i<aggSize; i++)
			{
				if(((pStatsInfo->blockAck0_31 >> i) & 1) == 1) {
					// This is a good packet
			        txStats[0].goodPackets ++;
			        txStats[pStatsInfo->descRate].goodPackets ++;
				}
			}

		  }
		}   // end of baStatus check
	  } else {
		// this was a non-agg frame
        txStats[0].goodPackets ++;
        txStats[pStatsInfo->descRate].goodPackets ++;
	  }

      /* do any signal strength processig */
      /* API UP for Owl */
      getSignalStrengthStats(&(pStatsInfo->sigStrength[0]), 
          (A_INT8)((pStatsInfo->status2 >> BITS_TO_TX_SIG_STRENGTH) & SIG_STRENGTH_MASK));
      txStats[0].ackSigStrengthAvg = pStatsInfo->sigStrength[0].rxAvrgSignalStrength;
      txStats[0].ackSigStrengthMax = pStatsInfo->sigStrength[0].rxMaxSigStrength;
      txStats[0].ackSigStrengthMin = pStatsInfo->sigStrength[0].rxMinSigStrength;

      getSignalStrengthStats(&(pStatsInfo->sigStrength[pStatsInfo->descRate]), 
          (A_INT8)((pStatsInfo->status2 >> BITS_TO_TX_SIG_STRENGTH) & SIG_STRENGTH_MASK));
      txStats[pStatsInfo->descRate].ackSigStrengthAvg = pStatsInfo->sigStrength[pStatsInfo->descRate].rxAvrgSignalStrength;
      txStats[pStatsInfo->descRate].ackSigStrengthMax = pStatsInfo->sigStrength[pStatsInfo->descRate].rxMaxSigStrength;
      txStats[pStatsInfo->descRate].ackSigStrengthMin = pStatsInfo->sigStrength[pStatsInfo->descRate].rxMinSigStrength;
    }
    else
    {
        /* processess error statistics */
      if (pStatsInfo->status1 & DESC_FIFO_UNDERRUN)
      {
          txStats[0].underruns ++;
          txStats[pStatsInfo->descRate].underruns ++;
      }
      else if (pStatsInfo->status1 & DESC_EXCESS_RETIES)
      {
          txStats[0].excessiveRetries ++;
          txStats[pStatsInfo->descRate].excessiveRetries ++;
      }
    }

    /* count the number of retries */
    numRetries = (A_UINT16)((pStatsInfo->status1 >> BITS_TO_SHORT_RETRIES) & RETRIES_MASK);
    if (numRetries == 1) {
        txStats[0].shortRetry1++;
        txStats[pStatsInfo->descRate].shortRetry1++;
    }
    else if (numRetries == 2) {
        txStats[0].shortRetry2++;
        txStats[pStatsInfo->descRate].shortRetry2++;
    }
    else if (numRetries == 3) {
        txStats[0].shortRetry3++;
        txStats[pStatsInfo->descRate].shortRetry3++;
    }
    else if (numRetries == 4) {
        txStats[0].shortRetry4++;
        txStats[pStatsInfo->descRate].shortRetry4++;
    }
    else if (numRetries == 5) {
        txStats[0].shortRetry5++;
        txStats[pStatsInfo->descRate].shortRetry5++;
    }
    else if ((numRetries >= 6) && (numRetries <= 10)) {
        txStats[0].shortRetry6to10++;
        txStats[pStatsInfo->descRate].shortRetry6to10++;
    }
    else if ((numRetries >=11) && (numRetries <= 15)) {
        txStats[0].shortRetry11to15++;
        txStats[pStatsInfo->descRate].shortRetry11to15++;
    }
    else;

    numRetries = (A_UINT16)((pStatsInfo->status1 >> BITS_TO_LONG_RETRIES) & RETRIES_MASK);
    if (numRetries == 1) {
        txStats[0].longRetry1++;
        txStats[pStatsInfo->descRate].longRetry1++;
    }
    else if (numRetries == 2) {
        txStats[0].longRetry2++;
        txStats[pStatsInfo->descRate].longRetry2++;
    }
    else if (numRetries == 3) {
        txStats[0].longRetry3++;
        txStats[pStatsInfo->descRate].longRetry3++;
    }
    else if (numRetries == 4) {
        txStats[0].longRetry4++;
        txStats[pStatsInfo->descRate].longRetry4++;
    }
    else if (numRetries == 5) {
        txStats[0].longRetry5++;
        txStats[pStatsInfo->descRate].longRetry5++;
    }
    else if ((numRetries >= 6) && (numRetries <= 10)) {
        txStats[0].longRetry6to10++;
        txStats[pStatsInfo->descRate].longRetry6to10++;
    }
    else if ((numRetries >=11) && (numRetries <= 15)) {
        txStats[0].longRetry11to15++;
        txStats[pStatsInfo->descRate].longRetry11to15++;
    }
    else;

    return;
}


MANLIB_API void fillTxStats
(
 A_UINT32 devNumIndex, 
 A_UINT32 descAddress,
 A_UINT32 numDesc,
 A_UINT32 dataBodyLen,
 A_UINT32 txTime,
 TX_STATS_STRUCT *txStats
)
{
    TX_STATS_TEMP_INFO  statsInfo;
    A_UINT32 i, statsLoop;
    A_UINT32 bitsReceived;
    A_UINT32 descStartTime = 0;
    A_UINT32 descTempTime = 0;
    A_UINT32 descMidTime = 0;
    A_UINT32 descEndTime = 0;
    float    newTxTime = 0;
    A_UINT16 firstPktGood = 0;
    A_UINT32 lastDescRate = 0;
    A_UINT32 timeStamp;
    A_BOOL   ht40, shortGi;
	A_UINT32 aggSize;
	A_UINT32 numAggFrames = 0;
	A_UINT32 aggFrameCounter = 0;


 // fpDebug = fopen("txDesc.txt", "w");

#if defined(AR5523) || defined(AR6000)
    A_UINT32 tempRate;
#endif

//	FILE *fp1;
//    fp1 = fopen("log1.txt", "a+");

#if defined(THIN_CLIENT_BUILD)
#else
    LIB_DEV_INFO *pLibDev = gLibInfo.pLibDevArray[devNumIndex];
#endif

#ifndef MALLOC_ABSENT
    memset(txStats, 0, sizeof(TX_STATS_STRUCT) * STATS_BINS);
    // traverse the descriptor list and get the stats from each one
    memset(&statsInfo, 0, sizeof(TX_STATS_TEMP_INFO));
#else
    A_MEMSET(txStats, 0, sizeof(TX_STATS_STRUCT) * STATS_BINS);
    // traverse the descriptor list and get the stats from each one
    A_MEMSET(&statsInfo, 0, sizeof(TX_STATS_TEMP_INFO));
#endif

    for(statsLoop = 0; statsLoop < STATS_BINS; statsLoop++) {
        statsInfo.sigStrength[statsLoop].rxMinSigStrength = 127;
    }

	if(pLibDev->yesAgg == 1)
		numAggFrames = numDesc/pLibDev->aggSize;

    statsInfo.descToRead = descAddress;
    for (i = 0; i < numDesc; i++) {
		// In aggregation mode stats collection will be done at the end of aggregation block on in last packet
		// memDisplay(devNumIndex, statsInfo.descToRead, 24);
		
	/*	fprintf(fpDebug, "pkt num %d\n", debug_pkt_num);
		debug_pkt_num++;
		{
		  	A_UINT32 iIndex, memValue;
    			for(iIndex=0; iIndex<24; iIndex++) {
    				pLibDev->devMap.OSmemRead(devNumIndex, statsInfo.descToRead+(iIndex*4), (A_UCHAR *)&memValue, sizeof(memValue));
    				fprintf(fpDebug, "W %d\t0x%08x\n", iIndex, memValue);
  			}
		}
	*/	
		if((pLibDev->yesAgg == TRUE))
		{
			// If it is not on aggregation boundary don't collect data
			if(((i+1)%pLibDev->aggSize != 0)  && (i!= (numDesc-1)))
			{
		        statsInfo.descToRead += sizeof(MDK_ATHEROS_DESC);       
				continue;
			}
		}

#if defined (THIN_CLIENT_BUILD) || defined (WIN_CLIENT_BUILD)

#if defined(AR5523)
        tempRate = (hwMemRead32(devNumIndex, (statsInfo.descToRead + FOURTH_CONTROL_WORD)) >> BITS_TO_TX_DATA_RATE0) & VENICE_DATA_RATE_MASK;
        statsInfo.descRate = descRate2bin(tempRate);
        statsInfo.status1 = hwMemRead32(devNumIndex, statsInfo.descToRead + FIRST_VENICE_STATUS_WORD);
        statsInfo.status2 = hwMemRead32(devNumIndex, statsInfo.descToRead + SECOND_VENICE_STATUS_WORD);
#endif

#if defined(AR6000)
        tempRate = (hwMemRead32(devNumIndex, (statsInfo.descToRead + FOURTH_CONTROL_WORD)) >> BITS_TO_TX_DATA_RATE0) & VENICE_DATA_RATE_MASK;
        statsInfo.descRate = descRate2bin(tempRate);
        statsInfo.status1 = hwMemRead32(devNumIndex, statsInfo.descToRead + FIRST_FALCON_TX_STATUS_WORD);
        statsInfo.status2 = hwMemRead32(devNumIndex, statsInfo.descToRead + SECOND_FALCON_TX_STATUS_WORD);
#endif

#else
        statsInfo.descRate = ar5kInitData[pLibDev->ar5kInitIndex].pMacAPI->txGetDescRate(devNumIndex, statsInfo.descToRead, &ht40, &shortGi);
        statsInfo.descRate = descRate2RateIndex(statsInfo.descRate, ht40);
        pLibDev->devMap.OSmemRead(devNumIndex, statsInfo.descToRead + pLibDev->txDescStatus1, 
                    (A_UCHAR *)&(statsInfo.status1), sizeof(statsInfo.status1));
        pLibDev->devMap.OSmemRead(devNumIndex, statsInfo.descToRead + pLibDev->txDescStatus2, 
                        (A_UCHAR *)&(statsInfo.status2), sizeof(statsInfo.status2));

        /* Hack to push RSSI into status word 2 TODO */
        if (isOwl(pLibDev->swDevID)) {
            A_UINT32 rssi;
            pLibDev->devMap.OSmemRead(devNumIndex, statsInfo.descToRead + OWL_ANT_RSSI_TX_STATUS_WORD, 
                (A_UCHAR *)&(rssi), sizeof(rssi));
            statsInfo.status2 = (statsInfo.status2 & 0xFFE01FFF) | ((rssi >> 11) & 0x001FE000);
        }
#endif
  	    aggSize = pLibDev->aggSize;
        if (statsInfo.status2 & DESC_DONE) {
			// check if the frame is agg frame
			if(pLibDev->yesAgg == 1) {
			  // Read ba_status and block ack bit maps
			  pLibDev->devMap.OSmemRead(devNumIndex, statsInfo.descToRead + OWL_BA_STATUS, 
                    (A_UCHAR *)&(statsInfo.baStatus), sizeof(statsInfo.baStatus));
			  pLibDev->devMap.OSmemRead(devNumIndex, statsInfo.descToRead + OWL_BLK_ACK_0_31, 
                    (A_UCHAR *)&(statsInfo.blockAck0_31), sizeof(statsInfo.blockAck0_31));
			  pLibDev->devMap.OSmemRead(devNumIndex, statsInfo.descToRead + OWL_BLK_ACK_32_63, 
                    (A_UCHAR *)&(statsInfo.blockAck32_63), sizeof(statsInfo.blockAck32_63));
			  statsInfo.baStatus = (statsInfo.baStatus >> 30 ) &0x1;

			  // if(((statsInfo.status1 & 0x1) == 0) || (statsInfo.baStatus == 0))
			  //  fprintf(fp1, "status1: %x status2: %x ba_status: %x, back1: %x\n", statsInfo.status1, statsInfo.status2,  statsInfo.baStatus, statsInfo.blockAck0_31);

			  // If this is last agg frame and total number of packets is not multiple 
			  // of arrSize. Last agg frame size will be total_desc modulo aggSize
			  if((i==(numDesc-1)) && (numDesc%pLibDev->aggSize != 0))
 				aggSize = numDesc%pLibDev->aggSize;
			}

  		    extractTxStats(&statsInfo, txStats, pLibDev->yesAgg, aggSize);
		
			// Check if aggregated frame or not
			if(pLibDev->yesAgg == 1) {
				// Check if this is first agg frame
			  if(aggFrameCounter == 0) {
                pLibDev->devMap.OSmemRead(devNumIndex, statsInfo.descToRead + TIME_STAMP_OFFSET, 
                        (A_UCHAR *)&(timeStamp), sizeof(timeStamp));

                descStartTime = timeStamp;
                descMidTime = descStartTime;
                if (statsInfo.status1 & DESC_FRM_XMIT_OK) {
                    firstPktGood = 1; //need to remove the 1st packet from new throughput calculation
                    txStats[statsInfo.descRate].firstPktGood = 1;
                }
                //update the rate start time
                txStats[statsInfo.descRate].startTime = descStartTime;
                txStats[statsInfo.descRate].endTime = descStartTime;

			  } else if (aggFrameCounter == (numAggFrames - 1)){
                pLibDev->devMap.OSmemRead(devNumIndex, statsInfo.descToRead + TIME_STAMP_OFFSET, 
                       (A_UCHAR *)&(timeStamp), sizeof(timeStamp));
                descEndTime = timeStamp;
                //update the end time for the current rate
                txStats[statsInfo.descRate].endTime = descEndTime;
			  }
			  aggFrameCounter++; 
			} else {
			  // Not an agg frame
              if (i == 0) {
                if (isOwl(pLibDev->swDevID)) {
                    pLibDev->devMap.OSmemRead(devNumIndex, statsInfo.descToRead + TIME_STAMP_OFFSET, 
                        (A_UCHAR *)&(timeStamp), sizeof(timeStamp));
				} else {
                    timeStamp = (statsInfo.status1 >> 16) & 0xffff;
                }
                descStartTime = timeStamp;
                descMidTime = descStartTime;
                if (statsInfo.status1 & DESC_FRM_XMIT_OK) {
                    firstPktGood = 1; //need to remove the 1st packet from new throughput calculation
                    txStats[statsInfo.descRate].firstPktGood = 1;
                }

                //update the rate start time
                txStats[statsInfo.descRate].startTime = descStartTime;
                txStats[statsInfo.descRate].endTime = descStartTime;
			  } else if ( i == numDesc - 1) {
                if (isOwl(pLibDev->swDevID)) {
                    pLibDev->devMap.OSmemRead(devNumIndex, statsInfo.descToRead + TIME_STAMP_OFFSET, 
                        (A_UCHAR *)&(timeStamp), sizeof(timeStamp));
                } else {
                    timeStamp = (statsInfo.status1 >> 16) & 0xffff;
                }
                descEndTime += timeStamp;
                if (descEndTime < descMidTime) {
                    descEndTime += 0x10000;
                }
                //update the end time for the current rate
                txStats[statsInfo.descRate].endTime = descEndTime;
                fillRateThroughput(txStats, lastDescRate, dataBodyLen);
			  } else {
                if (isOwl(pLibDev->swDevID)) {
                    pLibDev->devMap.OSmemRead(devNumIndex, statsInfo.descToRead + TIME_STAMP_OFFSET, 
                        (A_UCHAR *)&(timeStamp), sizeof(timeStamp));
                } else {
                    timeStamp = (statsInfo.status1 >> 16) & 0xffff;
                }
                descTempTime = timeStamp;
                if (descTempTime < descMidTime) {
                    descMidTime = 0x10000 + descTempTime;
                }
                else {
                    descMidTime = descTempTime;
                }
                txStats[statsInfo.descRate].endTime = descTempTime;
                if(lastDescRate != statsInfo.descRate) {
                    //this is the next rate so update the time
                    txStats[statsInfo.descRate].startTime = descTempTime;

                    //update the throughput on the last rate
                    if(statsInfo.status1 & DESC_FRM_XMIT_OK) {
                        //need to remove the 1st packet from new throughput calculation
                        txStats[statsInfo.descRate].firstPktGood = 1;
                    }

                    fillRateThroughput(txStats, lastDescRate, dataBodyLen);
                }
			  }
			} // end of if non-agg
        } else {
            //assume that if find a descriptor that is not done, then
            //none of the rest of the descriptors will be done, since assume
            //tx has completed by time get to here
#if defined (THIN_CLIENT_BUILD) 
            uiPrintf("Device Number %d:txGetStats: found a descriptor that is not done,  \
                stopped gathering stats %08lx %08lx\n", devNumIndex, statsInfo.status1, statsInfo.status2);
#else
#if defined (WIN_CLIENT_BUILD)
            printf("Device Number %d:txGetStats: found a descriptor that is not done,  \
                stopped gathering stats %08lx %08lx\n", devNumIndex, statsInfo.status1, statsInfo.status2);
#else 
            mError(devNumIndex, EIO, 
                "Device Number %d:txGetStats: found a descriptor that is not done, stopped gathering stats %08lx %08lx\n", devNumIndex,
                statsInfo.status1, statsInfo.status2);
#endif
#endif
            break;
        }

        lastDescRate = statsInfo.descRate;
        statsInfo.descToRead += sizeof(MDK_ATHEROS_DESC);       
    } // end of numDesc loop

    if (txTime == 0) {
//      mError(devNumIndex, EINVAL, "Device Number %d:transmit time is zero, not calculating throughput\n", devNumIndex);
    } else {
        //calculate the throughput
        bitsReceived = txStats[0].goodPackets * (dataBodyLen + sizeof(MDK_PACKET_HEADER)) * 8;
        txStats[0].throughput = bitsReceived / txTime;
    }

//	printf("Good packet count: %d, bitsReceived: %d, txTime: %d, tput: %d\n", txStats[0].goodPackets, bitsReceived, txTime, txStats[0].throughput);
	if(pLibDev->yesAgg == 1)
		numAggFrames = numDesc/pLibDev->aggSize;

    if(descStartTime < descEndTime) {
    	if(pLibDev->yesAgg == 1)
            bitsReceived = (txStats[0].goodPackets - pLibDev->aggSize) * (dataBodyLen + sizeof(MDK_PACKET_HEADER)) * 8;
        else 
            bitsReceived = (txStats[0].goodPackets - firstPktGood) * (dataBodyLen + sizeof(MDK_PACKET_HEADER)) * 8;

        if (isOwl(pLibDev->swDevID)) {
            newTxTime = (float)(descEndTime - descStartTime)/1000;  //puts the time in millisecs
        } else {
            newTxTime = (float)((descEndTime - descStartTime) * 1024)/1000;  //puts the time in millisecs
        }
        txStats[0].newThroughput = (A_UINT32)((float)bitsReceived / newTxTime);
        txStats[statsInfo.descRate].newThroughput = (A_UINT32)((float)bitsReceived / newTxTime);
    }
//	printf("Good pkt: %d, bitsReceived: %d, newTxTime: %f, tput: %d\n", txStats[0].goodPackets, bitsReceived, newTxTime, txStats[0].newThroughput);

//	fclose(fp1);

 // fclose(fpDebug);

    return;
}


