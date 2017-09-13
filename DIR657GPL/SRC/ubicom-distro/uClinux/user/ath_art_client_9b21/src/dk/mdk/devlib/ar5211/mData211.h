/*
 *  Copyright © 2001 Atheros Communications, Inc.,  All Rights Reserved.
 */
/* mdata(5)211.h - Type definitions needed for data transfer functions */

/* Copyright 2000, T-Span Systems Corporation */
/* Copyright 2000, Atheros Communications Inc. */

#ifndef	_MDATA211_H
#define	_MDATA211_H

#ident  "ACI $Id: //depot/sw/src/dk/mdk/devlib/ar5211/mData211.h#16 $, $Header: //depot/sw/src/dk/mdk/devlib/ar5211/mData211.h#16 $"
#ifdef Linux
#include "../mdata.h"
#else
#include "..\mdata.h"
#endif

void macAPIInitAr5211(A_UINT32 devNum);
void setRetryLimitAr5211(A_UINT32 devNum, A_UINT16 queueIndex);
void setRetryLimitAllAr5211(A_UINT32 devNum, A_UINT16 reserved);
A_UINT32 setupAntennaAr5211(A_UINT32 devNum, A_UINT32 antenna, A_UINT32* antModePtr);

A_UINT32 sendTxEndPacketAr5211(A_UINT32 devNum, A_UINT16 queueIndex);

void setDescriptorAr5211(A_UINT32 devNum, MDK_ATHEROS_DESC*  localDescPtr, 
					   A_UINT32 pktSize, A_UINT32 antMode,
					   A_UINT32 descNum, A_UINT32 rateIndex, A_UINT32 broadcast);
void setContDescriptorAr5211(A_UINT32 devNum, MDK_ATHEROS_DESC*  localDescPtr, 
					   A_UINT32 pktSize, A_UINT32 antMode,  A_UCHAR  dataRate);
void txBeginConfigAr5211(A_UINT32 devNum, A_BOOL enableInterrupt);
void txBeginContDataAr5211(A_UINT32 devNum, A_UINT16 queueIndex);
void txBeginContFramedDataAr5211(A_UINT32 devNum, A_UINT16 queueIndex, A_UINT32 ifswait, A_BOOL);
void beginSendStatsPktAr5211(A_UINT32 DevNum, A_UINT32 DescAddress);

void writeRxDescriptorAr5211(A_UINT32 devNum, A_UINT32 rxDescAddress);
void rxBeginConfigAr5211(A_UINT32 devNum );
void rxCleanupConfigAr5211(A_UINT32 devNum);
void txCleanupConfigAr5211(A_UINT32 devNum);
void setPPM5211( A_UINT32 devNum, A_UINT32 enablePPM);
A_UINT32 isTxdescEvent5211(A_UINT32 eventISRValue);
A_UINT32 isRxdescEvent5211(A_UINT32 eventISRValue);
A_UINT32 isTxComplete5211(A_UINT32 devNum);
void enableRx5211(A_UINT32 devNum);
void disableRx5211(A_UINT32 devNum);
void enable5211QueueClocks(A_UINT32 devNum, A_UINT32 qcuIndex, A_UINT32 dcuIndex);
void disable5211QueueClocks(A_UINT32 devNum);
void oahuWorkAround(A_UINT32 devNum);
void setStatsPktDescAr5211
(
 A_UINT32 devNum,
 MDK_ATHEROS_DESC*  localDescPtr,
 A_UINT32 pktSize,
 A_UINT32 rateValue
);

A_UINT32 txGetDescRateAr5211
(
 A_UINT32 devNum,
 A_UINT32 descAddr,
 A_BOOL   *ht40,
 A_BOOL   *shortGi
);


void txEndContFramedDataAr5211
(
 A_UINT32 devNum,
 A_UINT16 queueIndex
);

void setQueueAr5211
(
	A_UINT32 devNum,
	A_UINT32 qcuNumber
);

void mapQueueAr5211
(
	A_UINT32 devNum,
	A_UINT32 qcuNumber,
	A_UINT32 dcuNumber
);

void clearKeyCacheAr5211
(
	A_UINT32 devNum
);

void AGCDeafAr5211
(
 A_UINT32 devNum
);

void AGCUnDeafAr5211
(
  A_UINT32 devNum
);

#endif	/* _MDATA211_H */
