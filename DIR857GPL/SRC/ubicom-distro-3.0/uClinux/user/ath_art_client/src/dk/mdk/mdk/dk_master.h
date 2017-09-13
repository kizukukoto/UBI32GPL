/* dk_master.h - contians definitions for dk_master */

/* Copyright (c) 2000 Atheros Communications, Inc., All Rights Reserved */

#ident	"ACI $Id: //depot/sw/src/dk3.0/master/dk_master.h#1 $, $Header: //depot/sw/src/dk3.0/master/dk_master.h#1 $"

/*
modification history
--------------------
00a    10oct00	  fjc	 Created.
*/

/*
DESCRIPTION
Contains the definitions for the dk_master command handling
*/

#ifndef __INCdk_masterh
#define __INCdk_masterh

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "perlarry.h"
#include "event.h"
#include "dk_common.h"
#include "common_hw.h"
#include "dkPerl.h"

A_BOOL devNumValid(A_UINT16 handle);
A_UINT32 initializeMLIB(A_UINT16 devNum, MDK_WLAN_DEV_INFO *pdevInfo, A_UINT16 remoteLib);
void removeCurrentInst(void);


/* struct to be able to create a linked list of unique pipes that dk_perl has opened */
typedef struct dkPerlPipe
{
    struct dkPerlPipe	*pNext;
    OS_SOCK_INFO	*pOSSock;	    /* contains socket creation related information */
    A_CHAR		machineName[256];   /* the name of the server (provided by user) that are connected to */
} DK_PERL_PIPE;

/* keep track of information needed for each F2 being used */
typedef struct f2InstInfo
{
    struct f2InstInfo	*pNext;		/* pointer to next instance in list */
    A_UINT16		f2Handle;	/* the handle assigned and given to use by dk_perl for an F2 */
    A_UINT16		whichF2;	/* which F2 within remote system */
	A_UINT32			devNum;		/* devnum returned by the library call */
    A_UINT16		nextEventID;	/* next event ID that can be assigned for this F2 */
	A_UINT16	    goLocal;	    /* Set when the F2 is local */
	A_UINT16	    remote_exec;	    /* Set when the library is present in remote*/
	A_BOOL		    thin_client;	    /* Set when the remote value is USB*/
    DK_PERL_PIPE	*pPipe;		/* contains pipe creation related information */
	time_t				rxStatsTime;	/* time stamp for when started gathering rx stats */
	A_CHAR				rxStatsFilename[256]; /* string containing filename for stats logging files */
	FILE				*pRxStatsFile;	/* pointer to file to hold rx stats info */
	FILE				*pRxStnStats[MAX_DK_STA_NUM];
										/* file pointers for per station rx stats */
	FILE				*pTxStatsFile;	/* pointer to transmit stats file */
	A_BOOL		eepFileLoaded;
	A_BOOL		configFileSet;
	A_CHAR      *pCfgFilename;
	A_BOOL		eePromLoad;
	A_UINT16 beanie2928Mode ;
	A_UINT16 refClk         ;
	A_BOOL   enableXR       ;
	A_BOOL   loadEar        ;
	A_UINT16 eepStartLocation;
	A_BOOL   artAniEnable   ;
	A_BOOL   artAniReuse    ;
	A_BOOL   chainSelect    ;
	A_BOOL   printPciWrites;		//debug mode to enable dumping of pci writes
	A_UINT32 ht40_enable;
    A_UINT32 halFriendly;
	A_UINT32 short_gi_enable;
	A_UINT32 tx_chain_mask;
	A_UINT32 rx_chain_mask;
	A_UINT32 rateMaskMcs40;
	A_UINT32 rateMaskMcs20;
	A_UINT32 extended_channel_op;
	A_UINT32 extended_channel_sep;
    A_UINT32 cal_data_in_eeprom;
} F2_INST_INFO;

/* structure for information need to track globally in dk_perl */
typedef struct dkPerlGlobal
{
    F2_INST_INFO    *pF2Instances;	/* pointer to the F2 instance structure linked list */
    F2_INST_INFO    *pCurrentF2;	/* pointer to the currently selected F2 */
    DK_PERL_PIPE    *pUniquePipes;	/* pointer to list of unique pipes opened */
    A_UINT16	    nextHandle;		/* number of the next handle to be assigned */
    A_UINT8	    commandBuf[4000];	/* holds the command/cmd replies sent over channel */
    EVENT_QUEUE	    *timerQ;		/* Queue for holding active timer events */
	EVENT_ARRAY eventArray;
} DK_PERL_GLOBAL;

#ifndef MDK_C
A_STATUS dkPerlInit(void);
void	 dkPerlCleanup(void);
#endif

#ifdef __cplusplus
}
#endif

#endif // __INCdk_masterh

