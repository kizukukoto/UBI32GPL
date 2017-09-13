/*
 *  Copyright © 2005 Atheros Communications, Inc.,  All Rights Reserved.
 */
/* mdata5416.h - Type definitions needed for data transfer functions */

#ifndef	_MDATA5416_H
#define	_MDATA5416_H

#ident  "ACI $Id: //depot/sw/src/dk/mdk/devlib/ar5416/mData5416.h#5 $, $Header: //depot/sw/src/dk/mdk/devlib/ar5416/mData5416.h#5 $"

#ifdef Linux
#include "../mdata.h"
#else
#include "..\mdata.h"
#endif

void macAPIInitAr5416
(
 A_UINT32 devNum
);

A_UINT32 setupAntennaAr5416
(
 A_UINT32 devNum, 
 A_UINT32 antenna, 
 A_UINT32* antModePtr 
);

A_UINT32 sendTxEndPacketAr5416
(
 A_UINT32 devNum, 
 A_UINT16 queueIndex
);

void setDescriptorAr5416
(
 A_UINT32 devNum,
 MDK_ATHEROS_DESC*  localDescPtr,
 A_UINT32 pktSize,
 A_UINT32 antMode,
 A_UINT32 descNum,
 A_UINT32 rateIndex,
 A_UINT32 broadcast
);

void setStatsPktDescAr5416
(
 A_UINT32 devNum,
 MDK_ATHEROS_DESC*  localDescPtr,
 A_UINT32 pktSize,
 A_UINT32 rateValue
);

void setContDescriptorAr5416
(
 A_UINT32 devNum,
 MDK_ATHEROS_DESC*  localDescPtr,
 A_UINT32 pktSize,
 A_UINT32 antMode,
 A_UCHAR  dataRate
);

A_UINT32 txGetDescRateAr5416
(
 A_UINT32 devNum,
 A_UINT32 descAddr,
 A_BOOL   *ht40,
 A_BOOL   *shortGi
);

void txBeginConfigAr5416
(
 A_UINT32 devNum,
 A_BOOL	  enableInterrupt
);

void txBeginContDataAr5416
(
 A_UINT32 devNum,
 A_UINT16 queueIndex
);

void txBeginContFramedDataAr5416
(
 A_UINT32 devNum,
 A_UINT16 queueIndex,
 A_UINT32 ifswait,
 A_BOOL	  retries
);

void beginSendStatsPktAr5416
(
 A_UINT32 devNum, 
 A_UINT32 DescAddress
);

void writeRxDescriptorAr5416
( 
 A_UINT32 devNum, 
 A_UINT32 rxDescAddress
);

void rxBeginConfigAr5416
(
 A_UINT32 devNum
);

void rxCleanupConfigAr5416
(
 A_UINT32 devNum
);

void setPPM5416
(
 A_UINT32 devNum,
 A_UINT32 enablePPM
);

void setDescriptorEndPacketAr5416
(
 A_UINT32 devNum,
 MDK_ATHEROS_DESC*  localDescPtr,
 A_UINT32 pktSize,
 A_UINT32 antMode,
 A_UINT32 descNum,
 A_UINT32 rateIndex,
 A_UINT32 broadcast
);

#endif	/* _MDATA5416_H */
