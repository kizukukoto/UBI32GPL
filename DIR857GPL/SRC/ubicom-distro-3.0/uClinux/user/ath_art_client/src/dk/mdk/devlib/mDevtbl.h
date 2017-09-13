/*
 *  Copyright © 2001 Atheros Communications, Inc.,  All Rights Reserved.
 */
/* mDevtbl.h - Type definitions needed for configuration table */

/* Copyright 2000, Atheros Communications Inc. */

#ifndef	__INCmdevtblh
#define	__INCmdevtblh

#ident  "ACI $Id: //depot/sw/src/dk/mdk/devlib/mDevtbl.h#23 $, $Header: //depot/sw/src/dk/mdk/devlib/mDevtbl.h#23 $"

#include "mdata.h"

//define to indicate don't match subsytemID 
#define DONT_MATCH		0xff

//includes mac specific function changes between devices
typedef struct macAPITable
{
	void		(*macAPIInit)(A_UINT32);
	A_UINT32	(*eepromRead)(A_UINT32, A_UINT32);
	void		(*eepromWrite)(A_UINT32, A_UINT32, A_UINT32);

	void		(*hwReset)(A_UINT32, A_UINT32);
	void 		(*pllProgram)(A_UINT32,A_UINT32);

	void		(*setRetryLimit)(A_UINT32, A_UINT16);
	A_UINT32	(*setupAntenna)(A_UINT32, A_UINT32, A_UINT32*);
	A_UINT32	(*sendTxEndPacket)(A_UINT32, A_UINT16);
	void		(*setDescriptor)(A_UINT32, MDK_ATHEROS_DESC*, 
								 A_UINT32, A_UINT32,
								 A_UINT32, A_UINT32, A_UINT32);
	void		(*setStatsPktDesc)(A_UINT32, MDK_ATHEROS_DESC*, 
								 A_UINT32, A_UINT32);
	void		(*setContDescriptor)(A_UINT32, MDK_ATHEROS_DESC*, 
								 A_UINT32, A_UINT32, A_UCHAR);
	void		(*txBeginConfig)(A_UINT32, A_BOOL);
	void		(*txBeginContData)(A_UINT32, A_UINT16);
	void		(*txBeginContFramedData)(A_UINT32, A_UINT16, A_UINT32, A_BOOL);
	void        (*txEndContFramedData)(A_UINT32, A_UINT16);
	void		(*beginSendStatsPkt)(A_UINT32, A_UINT32);
	void		(*writeRxDescriptor)(A_UINT32, A_UINT32);
	void		(*rxBeginConfig)(A_UINT32);
	void		(*rxCleanupConfig)(A_UINT32);
	void		(*txCleanupConfig)(A_UINT32);
	A_UINT32	(*txGetDescRate)(A_UINT32, A_UINT32, A_BOOL *, A_BOOL *);

	void		(*setPPM)(A_UINT32, A_UINT32);
	A_UINT32	(*isTxdescEvent)(A_UINT32);
	A_UINT32	(*isRxdescEvent)(A_UINT32);
	A_UINT32	(*isTxComplete)(A_UINT32);
	void		(*enableRx)(A_UINT32);
	void		(*disableRx)(A_UINT32);
	void 		(*setQueue)(A_UINT32,A_UINT32);
	void 		(*mapQueue)(A_UINT32,A_UINT32,A_UINT32);
	void 		(*clearKeyCache)(A_UINT32);
	void 		(*AGCDeaf)(A_UINT32);
	void 		(*AGCUnDeaf)(A_UINT32);

} MAC_API_TABLE;

//includes baseband and rf specific function changes between devices
typedef struct rfAPITable
{
	void			(*setPower)(A_UINT32, A_UINT32, A_UINT32, A_UCHAR *);
	void			(*setSinglePower)(A_UINT32, A_UCHAR );
	A_BOOL			(*setChannel)(A_UINT32, A_UINT32);
} RF_API_TABLE;

//includes baseband and rf specific function changes between devices
typedef struct aniAPITable
{
	A_BOOL			(*configArtAniLadder)(A_UINT32, A_UINT32);
	A_BOOL			(*enableArtAni)(A_UINT32, A_UINT32);
	A_BOOL			(*disableArtAni)(A_UINT32, A_UINT32);
	A_BOOL			(*setArtAniLevel)(A_UINT32, A_UINT32, A_UINT32);
	A_BOOL			(*setArtAniLevelMax)(A_UINT32, A_UINT32);
	A_BOOL			(*setArtAniLevelMin)(A_UINT32, A_UINT32);
	A_BOOL			(*incrementArtAniLevel)(A_UINT32, A_UINT32);
	A_BOOL			(*decrementArtAniLevel)(A_UINT32, A_UINT32);
	A_UINT32		(*getArtAniLevel)(A_UINT32, A_UINT32);
	A_BOOL			(*measArtAniFalseDetects)(A_UINT32, A_UINT32);	
	A_BOOL			(*isArtAniOptimized)(A_UINT32, A_UINT32);
	A_UINT32		(*getArtAniFalseDetects)(A_UINT32, A_UINT32);
	A_BOOL			(*setArtAniFalseDetectInterval)(A_UINT32, A_UINT32);
	A_BOOL			(*programCurrArtAniLevel)(A_UINT32, A_UINT32);
} ART_ANI_API_TABLE;

typedef struct _analogRev {
	A_UCHAR		productID;
	A_UCHAR		revID;
} ANALOG_REV;

typedef struct DeviceInitData
{
	A_UINT16		    deviceID;
    ANALOG_REV          *pAnalogRevs;
	A_UINT16			numAnalogRevs;
	A_UINT32			*pMacRevs;
	A_UINT16			numMacRevs;
	A_UINT16			swDeviceID;
	ATHEROS_REG_FILE	*pInitRegs;
    A_UINT16            sizeArray;
	MAC_API_TABLE		*pMacAPI;
	RF_API_TABLE		*pRfAPI;
	ART_ANI_API_TABLE   *pArtAniAPI;
	A_UINT16			cfgVersion;
	MODE_INFO			*pModeValues;
	A_UINT16			sizeModeArray;
	A_CHAR				*pCfgFileVersion;
} DEVICE_INIT_DATA;


extern DEVICE_INIT_DATA ar5kInitData[];
extern A_UINT32 numDeviceIDs;




#endif
