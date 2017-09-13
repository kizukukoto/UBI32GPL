/*
 *  Copyright © 2001 Atheros Communications, Inc.,  All Rights Reserved.
 */
/* mdata(5)210.h - Type definitions needed for data transfer functions */

/* Copyright 2000, T-Span Systems Corporation */
/* Copyright 2000, Atheros Communications Inc. */

#ifndef	_MDATA210_H
#define	_MDATA210_H

#ident  "ACI $Id: //depot/sw/src/dk/mdk/devlib/ar5210/mData210.h#14 $, $Header: //depot/sw/src/dk/mdk/devlib/ar5210/mData210.h#14 $"

void macAPIInitAr5210(A_UINT32 devNum);
void setRetryLimitAr5210(A_UINT32 devNum, A_UINT16 queueIndex);
A_UINT32 setupAntennaAr5210(A_UINT32 devNum, A_UINT32 antenna, A_UINT32* antModePtr);
A_UINT32 sendTxEndPacketAr5210(A_UINT32 devNum, A_UINT16 queueIndex);

void setDescriptorAr5210(A_UINT32 devNum, MDK_ATHEROS_DESC*  localDescPtr, 
					   A_UINT32 pktSize, A_UINT32 antMode,
					   A_UINT32 descNum, A_UINT32 rateIndex, A_UINT32 broadcast);
void setContDescriptorAr5210(A_UINT32 devNum, MDK_ATHEROS_DESC*  localDescPtr, 
					   A_UINT32 pktSize, A_UINT32 antMode,  A_UCHAR  dataRate);

void txBeginConfigAr5210(A_UINT32 devNum, A_BOOL enableInterrupt);
void txBeginContDataAr5210(A_UINT32 devNum, A_UINT16 queueIndex);
void txBeginContFramedDataAr5210(A_UINT32 devNum, A_UINT16 queueIndex, A_UINT32 ifswait, A_BOOL);
void beginSendStatsPktAr5210(A_UINT32 DevNum, A_UINT32 DescAddress);
void writeRxDescriptorAr5210(A_UINT32 devNum, A_UINT32 rxDescAddress);
void rxBeginConfigAr5210(A_UINT32 devNum);
void rxCleanupConfigAr5210(A_UINT32 devNum);
void txCleanupConfigAr5210(A_UINT32 devNum);
void setPPM5210( A_UINT32 devNum, A_UINT32 enablePPM);
A_UINT32 isTxdescEvent5210(A_UINT32 eventISRValue);
A_UINT32 isRxdescEvent5210(A_UINT32 eventISRValue);
A_UINT32 isTxComplete5210(A_UINT32 devNum);
void enableRx5210(A_UINT32 devNum);
void disableRx5210(A_UINT32 devNum);
void setStatsPktDescAr5210
(
 A_UINT32 devNum,
 MDK_ATHEROS_DESC*  localDescPtr,
 A_UINT32 pktSize,
 A_UINT32 rateValue
);

A_UINT32 txGetDescRateAr5210
(
 A_UINT32 devNum,
 A_UINT32 descAddr,
 A_BOOL   *ht40,
 A_BOOL   *shortGi
);

void txEndContFramedDataAr5210
(
 A_UINT32 devNum,
 A_UINT16 queueIndex
);

void setQueueAr5210
(
	A_UINT32 devNum,
	A_UINT32 qcuNumber
);

void mapQueueAr5210
(
	A_UINT32 devNum,
	A_UINT32 qcuNumber,
	A_UINT32 dcuNumber
);

void clearKeyCacheAr5210
(
	A_UINT32 devNum
);

void AGCDeafAr5210
(
 A_UINT32 devNum
);

void AGCUnDeafAr5210
(
  A_UINT32 devNum
);

#endif	/* _MDATA210_H */

