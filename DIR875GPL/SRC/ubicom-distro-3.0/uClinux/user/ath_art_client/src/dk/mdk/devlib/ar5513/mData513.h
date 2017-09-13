/*
 *  Copyright © 2003 Atheros Communications, Inc.,  All Rights Reserved.
 */
/* mdata(5)513.h - Type definitions needed for falcon data transfer functions */


#ifndef	_MDATA513_H
#define	_MDATA513_H

#ident  "ACI $Id: //depot/sw/src/dk/mdk/devlib/ar5513/mData513.h#2 $, $Header: //depot/sw/src/dk/mdk/devlib/ar5513/mData513.h#2 $"
#ifdef Linux
#include "../mdata.h"
#else
#include "..\mdata.h"
#endif


void macAPIInitAr5513
(
 A_UINT32 devNum
);

A_UINT32 setupAntennaAr5513
(
 A_UINT32 devNum, 
 A_UINT32 antenna, 
 A_UINT32* antModePtr 
);


void writeRxDescriptorAr5513
( 
 A_UINT32 devNum, 
 A_UINT32 rxDescAddress
);

void setDescriptorAr5513
(
 A_UINT32 devNum,
 MDK_ATHEROS_DESC*  localDescPtr,
 A_UINT32 pktSize,
 A_UINT32 antMode,
 A_UINT32 descNum,
 A_UINT32 rateIndex,
 A_UINT32 broadcast
);

void txBeginConfigAr5513
(
 A_UINT32 devNum,
 A_BOOL	  enableInterrupt
);

void setContDescriptorAr5513
(
 A_UINT32 devNum,
 MDK_ATHEROS_DESC*  localDescPtr,
 A_UINT32 pktSize,
 A_UINT32 antMode,
 A_UCHAR  dataRate
);

void enable5513QueueClocks(A_UINT32 devNum, A_UINT32 qcuIndex, A_UINT32 dcuIndex);

void oahuWorkAround(A_UINT32 devNum);

void txBeginContDataAr5513
(
 A_UINT32 devNum,
 A_UINT16 queueIndex
);

void txBeginContFramedDataAr5513
(
 A_UINT32 devNum,
 A_UINT16 queueIndex,
 A_UINT32 ifswait,
 A_BOOL	  retries
);

void setDescriptorEndPacketAr5513
(
 A_UINT32 devNum,
 MDK_ATHEROS_DESC*  localDescPtr,
 A_UINT32 pktSize,
 A_UINT32 antMode,
 A_UINT32 descNum,
 A_UINT32 rateIndex,
 A_UINT32 broadcast
);

void rxBeginConfigAr5513
(
 A_UINT32 devNum
);

void setPPM5513
(
 A_UINT32 devNum,
 A_UINT32 enablePPM
);

void setStatsPktDescAr5513
(
 A_UINT32 devNum,
 MDK_ATHEROS_DESC*  localDescPtr,
 A_UINT32 pktSize,
 A_UINT32 rateValue
);

void beginSendStatsPktAr5513
(
 A_UINT32 devNum, 
 A_UINT32 DescAddress
);

void rxCleanupConfigAr5513
(
 A_UINT32 devNum
);

void rxBeginConfigAr5513
(
 A_UINT32 devNum
);

A_UINT32 sendTxEndPacketAr5513
(
 A_UINT32 devNum, 
 A_UINT16 queueIndex
);

#endif	/* _MDATA513_H */
