/* dk_client.c - contians functions related to dk_client side of application */

/* Copyright (c) 2000 Atheros Communications, Inc., All Rights Reserved */

/* $Id: $ */

/*
DESCRIPTION
Contains functions related to dk_client side of application.  Including the main
function, the stuff for the communications channel etc.
*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#include "wlantype.h"
#include "dk_cmds.h"
#include "dk_client.h"
#include "dk_ver.h"


#if defined(PHOENIX)
#include "rtc_reg.h"
#include "phs_lib_includes.h"
#include "phs_defs.h"
#endif

#if defined (DOS_CLIENT)
#include "common_hwext.h"
#endif

#if defined(VXWORKS) || defined(ECOS)
#include "hw.h"
#include "hwext.h"
#endif

#if defined(PREDATOR) || defined(PHOENIX) || defined(DRAGON)
#include "flash_ops.h"
#endif

#if defined(VXWORKS) || defined(MDK_AP)
#ifdef AR5315
#include "ar531xPlusreg.h"
#endif
#ifdef AR5312
#include "ar531xreg.h"
#endif
#else

//#ifdef PREDATOR
//#include "ar5211reg.h"
//#else
//#include "ar5210reg.h"
//#endif

#if (!defined(ECOS) && !defined(SENAO_AP))
#include "common_hw.h"
#endif

#endif
#include "perlarry.h"

#if defined(THIN_CLIENT_BUILD)
#else // THIN_CLIENT_BUILD
#include "athreg.h"
#include "manlib.h"
#include "devlibif.h"
#endif // THIN_CLIENT_BUILD

#include "misclibif.h"
#include "stats_routines.h"

#ifdef LINUX
#include "termios.h"
#endif

#ifdef ENDIAN_SWAP
#include "endian_func.h"
#endif

#include "dk_common.h"

//  VxWorks 'C' library doesnt have stricmp or strcasecmp function.
#ifdef VXWORKS
#define stricmp (strcmp)
#endif

// thread info

#ifdef ECOS
#define MAX_THREADS	       1
#define MAX_CLIENTS_PER_THREAD 1
#else
#define MAX_THREADS	       2
#define MAX_CLIENTS_PER_THREAD 2
#endif

#define INVALID_ENTRY	       -1

#ifdef PREDATOR
extern A_UINT32 cal_offset;
#endif

#ifdef PHOENIX
struct phsChannelToggle pPCT;
#endif
A_UINT32 sent_bytes=0, received_bytes=0;
#ifndef MALLOC_ABSENT
#else
#include "global_vars.h"
#endif

#if defined(AR6000)
	    A_UINT16 pTempBuffer[1024];
#endif

#ifdef OWL_AP
#define FLC_RADIOCFG 2
#endif

struct dkThreadInfo {
	A_INT32  inUse;
	A_UINT32 threadId;
	OS_SOCK_INFO  *pOSSock;
	PIPE_CMD *PipeBuffer_p;
	CMD_REPLY *ReplyCmd_p;
	A_UINT32  devIndex[MAX_CLIENTS_PER_THREAD];
	A_INT32  devNum[MAX_CLIENTS_PER_THREAD];
	A_UINT32  numClients;
	A_UINT32  currentDevIndex;
};
struct dkThreadInfo dkThreadArray[MAX_THREADS];
int numThreads;
static A_UINT32 sem;

// FORWARD DECLARATIONS
extern void connectThread ( void *param);
extern A_STATUS commandDispatch ( struct dkThreadInfo *dkThread);
extern void putThreadEntry(struct dkThreadInfo *dkThread);
extern void initThreadArray(void);
struct dkThreadInfo *getThreadEntry(void);
static A_INT32  addClientToThread ( struct dkThreadInfo *dkThread, A_UINT16  devIndex, A_UINT32  devNum);
extern A_UINT32 get_chip_id(A_UINT32 devIndex);
static void removeClientFromThread
(
	struct dkThreadInfo *dkThread,
	A_UINT16  devIndex
);


/**************************************************************************
* main - main function
*
* RETURNS: N/A
*/
A_INT32 mdk_main(A_INT32 debugMode, A_UINT16 port)
{
	OS_SOCK_INFO *pOSSock;  	/* Pointer to socket info data */
   	A_STATUS        status;
	OS_SOCK_INFO *pOSClientSock;

	struct dkThreadInfo *dkThread;
	A_UINT32 threadId, reg;
	A_INT32 i, comport_established=0;
 
    	// display our startup message
	port = SOCK_PORT_NUM;
#ifdef SERIAL_COMN
	port = COM1_PORT_NUM;
#endif
#ifdef USB_COMN
    port = USB_PORT_NUM;
#endif
#ifdef MBOX_COMN
    port = MBOX_PORT_NUM;
#endif

#ifdef ART
   	uiPrintf("Starting ART Client... \n");
#else
   	uiPrintf("Starting MDK Client... \n");
#endif
   	uiPrintf("Waiting for connection from Host ... !!!!\n");
        //reading the CHIP_REV_ID;

#ifdef PHOENIX
   	diag_printf("Waiting for connection from Host (Pheonix.....)\n");
/*
	apRegWrite32(0xffffd10c, 0x7);
	apRegWrite32(0xffffd024, 0x1c0000);
   	diag_printf("Waiting for connection from Host \n");
	asm volatile (
	  "mcr p15, 0, r0, c7, c0, 4\n"
	);
   	diag_printf(".\n");
*/
#endif


#ifdef PREDATOR
	reg = sysRegRead(AR5523_RESET);
	reg &= ~RESET_GPIO;
	sysRegWrite(AR5523_RESET, reg);
    uDelay(100);
	sysRegWrite(AR5523_GPIO_BASE_ADDRESS+0x8, 0x0);
	sysRegWrite(AR5523_GPIO_BASE_ADDRESS+0x0, 0xf);
#endif

	status = A_OK;

    	// perform environment initialization
    	if ( A_ERROR == envInit((A_BOOL)debugMode) ) {
        	uiPrintf("Error: Unable to initialize the environment... Closing down!\n");
        	return 0;
	}

	initThreadArray();

#ifndef DOS_CLIENT
    /* Create the socket for dk_master to connect to.  */
    pOSSock = osSockListen(0, port);
	if (!pOSSock) {
        uiPrintf("Error: Something went wrong with socket creation...  Closing down!\n");
	   	envCleanup(TRUE);
     	return 0;
   	}

	sem = semInit();
	if (!sem) {
		uiPrintf("Error: semInit failed... Closing down! \n");
		osSockClose(pOSSock); 
	   	envCleanup(TRUE);
     	return 0;
	}
#endif
	q_uiPrintf("port = %d\n", port);

	while (1) {
	    if (port == SOCK_PORT_NUM) {
#ifndef DOS_CLIENT
		   pOSClientSock = osSockAccept(pOSSock);
#else 
			pOSClientSock = osSockListen(0, port);
#endif
		   if (!pOSClientSock) {
       			uiPrintf("Error: Something went wrong with socket creation...  Closing down!\n");
	 		   break;
     		}
		   uiPrintf("Socket connection to master established.  Waiting for commands....\n");
		   uiPrintf("Connection established with %s : Port %d \n",pOSClientSock->hostname,pOSClientSock->port_num);
		}
		else {
		   pOSClientSock = pOSSock;
		   if (comport_established) {
			milliSleep(100); // this will loop infinitely
			continue;
		   }
		   else comport_established = 1;
		}
		   semLock(sem);
		   dkThread = getThreadEntry();
		   if (!dkThread) {
			   uiPrintf("Error: Max Thread Limit Exceeded \n");
			   osSockClose(pOSClientSock);
			   break;
		   }
		   semUnLock(sem);

		   dkThread->pOSSock = pOSClientSock;
    		/* create the thread that will wait for commands */
     		status = osThreadCreate(connectThread, (void *)dkThread, MDK_CLIENT,MDK_CLIENT_PRIO, &threadId);
#if defined(ECOS)
				putThreadEntry(dkThread);
		   		comport_established = 0;
#endif
    		if ( A_FAILED(status) ) {
				putThreadEntry(dkThread);
				osSockClose(pOSClientSock);
				uiPrintf("Error: Unable to start connectThread()... Closing down!\n");
				break;
       		}
		dkThread->threadId = threadId;
	}

	semLock(sem);
	for (i=0;i<MAX_THREADS;i++) {
		if (dkThreadArray[i].inUse) {
			osThreadKill(dkThreadArray[i].threadId);
#ifndef MALLOC_ABSENT
			A_FREE(dkThreadArray[i].ReplyCmd_p);
			A_FREE(dkThreadArray[i].PipeBuffer_p);
#endif
			osSockClose(dkThreadArray[i].pOSSock);
			putThreadEntry(dkThread);
		}
	}

#ifndef DOS_CLIENT
	semUnLock(sem);

	semClose(sem);
	//  close the master socket	
 	osSockClose(pOSSock); 
#endif
#if defined(THIN_CLIENT_BUILD)
#else // THIN_CLIENT_BUILD
	// cleanup the library
	m_devlibCleanup();
	// clean up anything that was allocated and close the driver		
	envCleanup(TRUE);
#endif // THIN_CLIENT_BUILD
    	
     	return 0;
}


void printVersion(void)
{
    	uiPrintf(MDK_CLIENT_VER1);
    	uiPrintf(MDK_CLIENT_VER2);
}


void connectThread (void *pparam)
{
  	A_INT32	    numBytes;
   	OS_SOCK_INFO *pOSSock;
  	A_UINT16	cmdLen;
	A_STATUS	status;
  	A_BOOL	    bGoodRead;
	PIPE_CMD *PipeBuffer_p = NULL;
	CMD_REPLY *ReplyCmd_p = NULL;
	struct dkThreadInfo *dkThread;
	A_INT32 i;

	printVersion();

	dkThread = (struct dkThreadInfo *)pparam;
  	pOSSock = (OS_SOCK_INFO *)dkThread->pOSSock;


	semLock(sem);
#ifndef MALLOC_ABSENT
	PipeBuffer_p = (PIPE_CMD *)A_MALLOC(sizeof(PIPE_CMD));
	if ( NULL == PipeBuffer_p ) {
 	    	uiPrintf("Error: Unable to malloc Receive Pipe Buffer struct... Closing down!\n");
	    	osSockClose(pOSSock); 
		putThreadEntry(dkThread);
		semUnLock(sem);
	    	return;
    	}

	ReplyCmd_p = (CMD_REPLY *)A_MALLOC(sizeof(CMD_REPLY));
 	if ( NULL == ReplyCmd_p ) {
  	   	uiPrintf("Error: Unable to malloc Reply Pipe Buffer struct... Closing down!\n");
		A_FREE(PipeBuffer_p);
		osSockClose(pOSSock); 
		putThreadEntry(dkThread);
		semUnLock(sem);
	   	return;
    	}
#else

	PipeBuffer_p = &gPipeBuffer;
	ReplyCmd_p = &gReplyCmd;
	
#endif

	dkThread->ReplyCmd_p = ReplyCmd_p;
	dkThread->PipeBuffer_p = PipeBuffer_p;
	semUnLock(sem);
	
  	while ( 1 ) {
    		bGoodRead = FALSE;
		numBytes = 0;

 	
    		if ( osSockRead(pOSSock, (A_UINT8 *)(&cmdLen), sizeof(cmdLen)) ) {
#ifdef ENDIAN_SWAP
	 		cmdLen = ltob_s(cmdLen);
#endif
	 		if ((cmdLen == 0) || (cmdLen > sizeof(PIPE_CMD))) {
	   			uiPrintf("Error: Pipe write size too large or zero length:cmdLen=%d\n", cmdLen);
	   			uiPrintf("Error: >>>>>>> Pipe write size too large or zero length:cmdLen=%d\n", cmdLen);
	   			pOSSock->sockDisconnect = 1;
	   			break;
	 		}

			PipeBuffer_p->cmdLen = cmdLen;
//uiPrintf("RCL:%d:", cmdLen);
#ifdef _DEBUG_RD
            q_uiPrintf("cmdLen=%d\n", cmdLen);
#endif

			// Received the length field already. 
			// Read the command structure of the specified length.
			numBytes = osSockRead(pOSSock, (((A_UINT8 *)PipeBuffer_p)+sizeof(PipeBuffer_p->cmdLen)), cmdLen);
	 		if ( numBytes == cmdLen) {
	   			bGoodRead = TRUE;
			}
		}

    		if ( !bGoodRead ) {
	 		pOSSock->sockDisconnect = 1;
	 		break;
   	 	}
#ifdef USB_COMN
uiPrintf("DC:%d::", dkThread->PipeBuffer_p->cmdID);
#else
            q_uiPrintf("Dispatching command %d at port_num=%d\n", dkThread->PipeBuffer_p->cmdID, pOSSock->port_num);
#endif
//uiPrintf("DC:%d::", dkThread->PipeBuffer_p->cmdID);

    		status = commandDispatch(dkThread);
    		if ( A_FAILED(status) ) {
	 		uiPrintf("Error: Problem in commandDispatch()!\n");
	 		pOSSock->sockDisconnect = 1;
    	 		break;
    		}
		

    		if ( pOSSock->sockClose || pOSSock->sockDisconnect ) {
#ifndef __UBICOM32__
					if (pOSSock->port_num != SOCK_PORT_NUM) {
							pOSSock->sockClose = 0;
#if !defined(PHOENIX)
							pOSSock->sockDisconnect = 0;
#endif
#ifdef LINUX
							tcflush(pOSSock->sockfd, TCIOFLUSH);
#endif
							for (i=0;i<MAX_CLIENTS_PER_THREAD;i++) {
								if (dkThread->devIndex[i] != (A_UINT32)INVALID_ENTRY) {	
#if defined(THIN_CLIENT_BUILD)
#else // THIN_CLIENT_BUILD
									closeMLIB(dkThread->devNum[i]);	
#endif // THIN_CLIENT_BUILD
							        dkThread->devNum[i] = INVALID_ENTRY;
								deviceCleanup((A_UINT16)dkThread->devIndex[i]);
        							dkThread->devIndex[i] = (A_UINT32)INVALID_ENTRY;
			                        		dkThread->numClients--;
          						}
							}
#if !defined(PHOENIX_UART_WAKEUP_FEATURE)
								pOSSock->sockDisconnect = 0; // add this in the next line, later
#else
							if (pOSSock->sockDisconnect) {
								pOSSock->sockDisconnect = 0;
							   /**** ENABLE UART Wakeup and Sleep */
							   /* Begin */
							   sysRegWrite(UART0_WAKEUP_ADDRESS, UART0_WAKEUP_ENABLE_SET(1));
							   // Read the sleep registers sysRegRead(
						   	asm volatile (
	  									"mcr p15, 0, r0, c7, c0, 4\n"
							   );
						   
							   /* End */
							}
#endif
							continue;
					}
					else
#endif	// __UBICOM32__
	 					break;
   	 		}
  	}


	semLock(sem);
#ifndef MALLOC_ABSENT
	A_FREE(ReplyCmd_p);
	A_FREE(PipeBuffer_p);
#endif
	
#if defined(THIN_CLIENT_BUILD)
#else // THIN_CLIENT_BUILD
	for (i=0;i<MAX_CLIENTS_PER_THREAD;i++) {
		if (dkThread->devNum[i] != INVALID_ENTRY) {
			closeMLIB(dkThread->devNum[i]);	
		}

		if (dkThread->devIndex[i] != (A_UINT32)INVALID_ENTRY) {	
			deviceCleanup((A_UINT16)dkThread->devIndex[i]);
          	}
	}
#endif // THIN_CLIENT_BUILD

	osSockClose(pOSSock); 
	putThreadEntry(dkThread);
	semUnLock(sem);

  	return;
}


/**************************************************************************
* processInitF2Cmd - Process INIT_F2_CMD command
*
* RETURNS: 0 processed OK, an Error code if it is not
*/
A_UINT32 processInitF2Cmd
(
	struct dkThreadInfo *dkThread
)
{
	PIPE_CMD      *pCmd;
	CMD_REPLY     *ReplyCmd_p;
	A_UCHAR     *replyBuf;
	MDK_WLAN_DEV_INFO    *pdevInfo;
	A_UINT32 devNum;
	A_UINT32 devIndex;
	A_UINT16 device_fn;

	device_fn = WMAC_FUNCTION;
	pCmd = dkThread->PipeBuffer_p;
	ReplyCmd_p = dkThread->ReplyCmd_p;
	replyBuf = (A_UCHAR *)(ReplyCmd_p->cmdBytes);
 
	/* check that the size is appropriate before we access */
	if ( pCmd->cmdLen != sizeof(pCmd->CMD_U.INIT_F2_CMD)+sizeof(pCmd->cmdID)+sizeof(pCmd->devNum) ) {
		uiPrintf("Error, command Len is not equal to size of INIT_F2_CMD:Exp=%d:Recd=%d\n", sizeof(pCmd->CMD_U.INIT_F2_CMD)+sizeof(pCmd->cmdID)+sizeof(pCmd->devNum), pCmd->cmdLen);
		return(COMMS_ERR_BAD_LENGTH);
	}	

#ifdef ENDIAN_SWAP
     pCmd->CMD_U.INIT_F2_CMD.whichF2 = ltob_l(pCmd->CMD_U.INIT_F2_CMD.whichF2);
#endif

	devIndex =  pCmd->CMD_U.INIT_F2_CMD.whichF2;
//#if defined(AR6000)
//	devIndex = devIndex - SDIO_FN_DEV_START_NUM;
//#endif
	uiPrintf("F2 to init = %d\n", devIndex);
    
	/* validate the F2 required; see that it is a valid index */
	if (devIndex > WLAN_MAX_DEV ) {
		/* don't have a large enough array to accommodate this device */
		uiPrintf("Error: devInfo array not large enough, only support %d devices\n", WLAN_MAX_DEV);
		return(COMMS_ERR_INDEX_TOO_LARGE);
	}

	if (devIndex == 0) {
		uiPrintf("Error: Cannot have a zero index \n");
		return(COMMS_ERR_DEV_INIT_FAIL);
	}

	semLock(sem);
	devIndex = devIndex - 1;
#if defined(VXWORKS) || defined(ECOS)
	if (A_FAILED(deviceInit((A_UINT16)devIndex, device_fn)) ) {
#else
	if (A_FAILED(deviceInit((A_UINT16)devIndex, device_fn, NULL)) ) {
#endif
		uiPrintf("Error: Failed call to deviceInit() \n");
		semUnLock(sem);
		return(COMMS_ERR_DEV_INIT_FAIL);
	}

	pdevInfo = globDrvInfo.pDevInfoArray[devIndex];
#if defined(THIN_CLIENT_BUILD)
    devNum = pCmd->devNum;
#else
	devNum = initializeMLIB(pdevInfo);
#endif
	if (devNum == -1) {
		uiPrintf("Cannot initalize MLIB \n");
		semUnLock(sem);
		return(COMMS_ERR_DEV_INIT_FAIL);
	}
	if (addClientToThread(dkThread,(A_UINT16)devIndex,devNum) < 0) {
		uiPrintf("Cannot add thread to client \n");
		semUnLock(sem);
		return(COMMS_ERR_DEV_INIT_FAIL);
	}
    	// make this the current device
   	dkThread->currentDevIndex = (A_UINT16)devIndex;
	semUnLock(sem);

#if defined(THIN_CLIENT_BUILD)
	*((A_UINT32 *)replyBuf) = (devNum | 0x10000000);
#else
	*((A_UINT32 *)replyBuf) = devNum;
#endif

#if defined(THIN_CLIENT_BUILD)
#ifndef MALLOC_ABSENT
    memcpy(replyBuf+4, pdevInfo->pdkInfo, sizeof(DK_DEV_INFO));
#else
    A_MEMCPY(replyBuf+4, pdevInfo->pdkInfo, sizeof(DK_DEV_INFO));
#endif
#ifdef ENDIAN_SWAP
    swapBlock_l(replyBuf+4, sizeof(DK_DEV_INFO));
#endif
#endif
#ifdef ENDIAN_SWAP
        *((A_UINT32 *)replyBuf) = btol_l(*((A_UINT32 *)replyBuf));
#endif

	return (0);
}


/**************************************************************************
* processRegReadCmd - Process REG_READ_CMD command
*
* RETURNS: 0 processed OK, an Error code if it is not
*/
A_UINT32 processRegReadCmd
(
	struct dkThreadInfo *dkThread
)
{
	PIPE_CMD      *pCmd;
	CMD_REPLY     *ReplyCmd_p;
	A_UCHAR     *replyBuf;
	MDK_WLAN_DEV_INFO    *pdevInfo;
	A_UINT32 rAddr;

	pdevInfo = globDrvInfo.pDevInfoArray[dkThread->currentDevIndex];
	pCmd = dkThread->PipeBuffer_p;
	ReplyCmd_p = dkThread->ReplyCmd_p;
	replyBuf = (A_UCHAR *)(ReplyCmd_p->cmdBytes);
    
    /* check that the size is appropriate before we access */
    if( pCmd->cmdLen != sizeof(pCmd->CMD_U.REG_READ_CMD)+sizeof(pCmd->cmdID) + sizeof(pCmd->devNum) )
        {
        uiPrintf("Error, command Len is not equal to size of REG_READ_CMD\n");
        return(COMMS_ERR_BAD_LENGTH);
        }

#ifdef ENDIAN_SWAP
	pCmd->CMD_U.REG_READ_CMD.readAddr = ltob_l(pCmd->CMD_U.REG_READ_CMD.readAddr);
#endif

#ifndef PHOENIX
	/* check that the register is within the range of registers */
    	if (pCmd->CMD_U.REG_READ_CMD.readAddr > MAX_REG_OFFSET) {
		uiPrintf("Error:  Not a valid register offset\n");
		return(COMMS_ERR_BAD_OFFSET);
        }
#endif


 
      /* read the register */
#ifdef JUNGO
	*((A_UINT32 *)replyBuf) = hwMemRead32(dkThread->currentDevIndex,pdevInfo->pdkInfo->pSharedInfo->devMapAddress + pCmd->CMD_U.REG_READ_CMD.readAddr); 
#else
#ifdef PREDATOR
	//*((A_UINT32 *)replyBuf) = sysRegRead(pdevInfo->pdkInfo->f2MapAddress + pCmd->CMD_U.REG_READ_CMD.readAddr);
	rAddr = pdevInfo->pdkInfo->f2MapAddress + pCmd->CMD_U.REG_READ_CMD.readAddr;
	//uiPrintf("A:%x:", rAddr);
	*((A_UINT32 *)replyBuf) = sysRegRead(rAddr);
#else
	*((A_UINT32 *)replyBuf) = hwMemRead32((A_UINT16)dkThread->currentDevIndex,pdevInfo->pdkInfo->f2MapAddress + pCmd->CMD_U.REG_READ_CMD.readAddr);
#endif
#endif

#ifdef ENDIAN_SWAP
    *((A_UINT32 *)replyBuf) = btol_l(*((A_UINT32 *)replyBuf));
#endif
    
    return(0);
    }

/**************************************************************************
* processRegWriteCmd - Process REG_WRITE_CMD command
*
* RETURNS: 0 processed OK, an Error code if it is not
*/
A_UINT32 processRegWriteCmd
(
	struct dkThreadInfo *dkThread
)
{
	PIPE_CMD      *pCmd;
	CMD_REPLY     *ReplyCmd_p;
	A_UCHAR     *replyBuf;
	MDK_WLAN_DEV_INFO    *pdevInfo;
	A_UINT32 wAddr;

	pCmd = dkThread->PipeBuffer_p;
	ReplyCmd_p = dkThread->ReplyCmd_p;
	replyBuf = (A_UCHAR *)(ReplyCmd_p->cmdBytes);
	pdevInfo = globDrvInfo.pDevInfoArray[dkThread->currentDevIndex];
    
    /* check that the size is appropriate before we access */
    if( pCmd->cmdLen != sizeof(pCmd->CMD_U.REG_WRITE_CMD)+sizeof(pCmd->cmdID) + sizeof(pCmd->devNum) )
        {
        uiPrintf("Error, command Len is not equal to size of REG_WRITE_CMD\n");
        return(COMMS_ERR_BAD_LENGTH);
        }

#ifdef ENDIAN_SWAP
    pCmd->CMD_U.REG_WRITE_CMD.writeAddr = ltob_l(pCmd->CMD_U.REG_WRITE_CMD.writeAddr);
    pCmd->CMD_U.REG_WRITE_CMD.regValue  = ltob_l(pCmd->CMD_U.REG_WRITE_CMD.regValue);
#endif

#ifndef PHOENIX
	/* check that the register is within the range of registers */
    	if (pCmd->CMD_U.REG_WRITE_CMD.writeAddr > MAX_REG_OFFSET) {
		uiPrintf("Error:  Not a valid register offset\n");
		return(COMMS_ERR_BAD_OFFSET);
        }
#endif

    /* write the register */
#ifdef JUNGO
	hwMemWrite32(dkThread->currentDevIndex,pdevInfo->pdkInfo->pSharedInfo->devMapAddress + pCmd->CMD_U.REG_WRITE_CMD.writeAddr, pCmd->CMD_U.REG_WRITE_CMD.regValue); 
#else
#ifdef PREDATOR
	wAddr = pdevInfo->pdkInfo->f2MapAddress + pCmd->CMD_U.REG_WRITE_CMD.writeAddr;
	//uiPrintf("B:%x:", wAddr);
	sysRegWrite(wAddr, pCmd->CMD_U.REG_WRITE_CMD.regValue);
#else
	hwMemWrite32((A_UINT16)dkThread->currentDevIndex,pdevInfo->pdkInfo->f2MapAddress + pCmd->CMD_U.REG_WRITE_CMD.writeAddr, pCmd->CMD_U.REG_WRITE_CMD.regValue);
#endif
#endif
  
    return(0);
}

/**************************************************************************
* processRtcRegReadCmd - Process REG_READ_CMD command
*
* RETURNS: 0 processed OK, an Error code if it is not
*/
A_UINT32 processRtcRegReadCmd
(
        struct dkThreadInfo *dkThread
)
{
#ifdef OWL_PB42
    PIPE_CMD      *pCmd;
    CMD_REPLY     *ReplyCmd_p;
    A_UCHAR     *replyBuf;
    pCmd = dkThread->PipeBuffer_p;
    ReplyCmd_p = dkThread->ReplyCmd_p;
    replyBuf = (A_UCHAR *)(ReplyCmd_p->cmdBytes);
    printf("in RTC READ\n");

    if( pCmd->cmdLen != sizeof(pCmd->CMD_U.RTC_REG_READ_CMD)+sizeof(pCmd->cmdID) + sizeof(pCmd->devNum) )
    {
        uiPrintf("Error, command Len is not equal to size of CFG_READ_CMD\n");
        return(COMMS_ERR_BAD_LENGTH);
    }

#ifdef ENDIAN_SWAP
	pCmd->CMD_U.REG_READ_CMD.readAddr = ltob_l(pCmd->CMD_U.REG_READ_CMD.readAddr);
#endif
#ifndef VXWORKS
    *((A_UINT32 *)replyBuf) = (A_UINT32)hwRtcRegRead(dkThread->currentDevIndex, pCmd->CMD_U.RTC_REG_READ_CMD.readAddr);
#endif

#ifdef ENDIAN_SWAP
    *((A_UINT32 *)replyBuf) = btol_l(*((A_UINT32 *)replyBuf));
#endif

#endif  // OWL_PB42 


    return(0);
}

/**************************************************************************
* processRtcRegWriteCmd - Process REG_WRITE_CMD command
*
* RETURNS: 0 processed OK, an Error code if it is not
*/
A_UINT32 processRtcRegWriteCmd
(
        struct dkThreadInfo *dkThread
)


{
#ifdef OWL_PB42
        PIPE_CMD      *pCmd;
        CMD_REPLY     *ReplyCmd_p;
        A_UCHAR     *replyBuf;

        pCmd = dkThread->PipeBuffer_p;
        ReplyCmd_p = dkThread->ReplyCmd_p;
        replyBuf = (A_UCHAR *)(ReplyCmd_p->cmdBytes);
        printf("in RTC WRITE\n");
        /* check that the size is appropriate before we access */
        if( pCmd->cmdLen != sizeof(pCmd->CMD_U.RTC_REG_WRITE_CMD)+sizeof(pCmd->cmdID) + sizeof(pCmd->devNum)) {
            uiPrintf("Error, command Len is not equal to size of CFG_WRITE_CMD\n");
            return(COMMS_ERR_BAD_LENGTH);
        }

#ifdef ENDIAN_SWAP
        pCmd->CMD_U.RTC_REG_WRITE_CMD.writeAddr = ltob_l(pCmd->CMD_U.REG_WRITE_CMD.writeAddr);
        pCmd->CMD_U.RTC_REG_WRITE_CMD.regValue  = ltob_l(pCmd->CMD_U.REG_WRITE_CMD.regValue);
#endif
#ifndef VXWORKS
    /* write the register */
        hwRtcRegWrite(dkThread->currentDevIndex, pCmd->CMD_U.RTC_REG_WRITE_CMD.writeAddr, pCmd->CMD_U.RTC_REG_WRITE_CMD.regValue);
#endif
#endif  // OWL_AP

    return(0);
    }


/**************************************************************************
* processCfgReadCmd - Process CFG_READ_CMD command
*
* RETURNS: 0 processed OK, an Error code if it is not
*/
A_UINT32 processCfgReadCmd
(
	struct dkThreadInfo *dkThread
)
{
	PIPE_CMD      *pCmd;
	CMD_REPLY     *ReplyCmd_p;
	A_UCHAR     *replyBuf;

	pCmd = dkThread->PipeBuffer_p;
	ReplyCmd_p = dkThread->ReplyCmd_p;
	replyBuf = (A_UCHAR *)(ReplyCmd_p->cmdBytes);
    /* check that the size is appropriate before we access */
    if( pCmd->cmdLen != sizeof(pCmd->CMD_U.CFG_READ_CMD)+sizeof(pCmd->cmdID) + sizeof(pCmd->devNum) )
        {
        uiPrintf("Error, command Len is not equal to size of CFG_READ_CMD\n");
        return(COMMS_ERR_BAD_LENGTH);
        }

#ifdef ENDIAN_SWAP
	pCmd->CMD_U.CFG_READ_CMD.cfgReadAddr = ltob_l(pCmd->CMD_U.CFG_READ_CMD.cfgReadAddr);        
	pCmd->CMD_U.CFG_READ_CMD.readSize = ltob_l(pCmd->CMD_U.CFG_READ_CMD.readSize);
#endif

    /* read the register */
    switch(pCmd->CMD_U.CFG_READ_CMD.readSize)
        {
#if !defined(ART) && !defined(THIN_CLIENT_BUILD)
        case 8:
            *((A_UINT32 *)replyBuf) = (A_UINT32)hwCfgRead8((A_UINT16)dkThread->currentDevIndex, pCmd->CMD_U.CFG_READ_CMD.cfgReadAddr); 
            break;

        case 16:
            *((A_UINT32 *)replyBuf) = (A_UINT32)hwCfgRead16((A_UINT16)dkThread->currentDevIndex, pCmd->CMD_U.CFG_READ_CMD.cfgReadAddr); 
            break;
#endif
        case 32:
            *((A_UINT32 *)replyBuf) = hwCfgRead32((A_UINT16)dkThread->currentDevIndex, pCmd->CMD_U.CFG_READ_CMD.cfgReadAddr); 
            break;

        default:
            return(COMMS_ERR_BAD_CFG_SIZE);

        }
  
#ifdef ENDIAN_SWAP
    *((A_UINT32 *)replyBuf) = btol_l(*((A_UINT32 *)replyBuf));
#endif



    
    return(0);
}

/**************************************************************************
* processCfgWriteCmd - Process CFG_WRITE_CMD command
*
* RETURNS: 0 processed OK, an Error code if it is not
*/
A_UINT32 processCfgWriteCmd
(
	struct dkThreadInfo *dkThread
)
{

#ifndef PREDATOR
	PIPE_CMD      *pCmd;
	CMD_REPLY     *ReplyCmd_p;
	A_UCHAR     *replyBuf;

	pCmd = dkThread->PipeBuffer_p;
	ReplyCmd_p = dkThread->ReplyCmd_p;
	replyBuf = (A_UCHAR *)(ReplyCmd_p->cmdBytes);
    
    /* check that the size is appropriate before we access */
    if( pCmd->cmdLen != sizeof(pCmd->CMD_U.CFG_WRITE_CMD)+sizeof(pCmd->cmdID) + sizeof(pCmd->devNum))
        {
        uiPrintf("Error, command Len is not equal to size of CFG_WRITE_CMD\n");
        return(COMMS_ERR_BAD_LENGTH);
        }

#ifdef ENDIAN_SWAP
	pCmd->CMD_U.CFG_WRITE_CMD.writeSize = ltob_l(pCmd->CMD_U.CFG_WRITE_CMD.writeSize);
	pCmd->CMD_U.CFG_WRITE_CMD.cfgWriteAddr = ltob_l(pCmd->CMD_U.CFG_WRITE_CMD.cfgWriteAddr);
	pCmd->CMD_U.CFG_WRITE_CMD.cfgValue = ltob_l(pCmd->CMD_U.CFG_WRITE_CMD.cfgValue);
#endif

    /* write the register */
    switch(pCmd->CMD_U.CFG_WRITE_CMD.writeSize)
        {
#if !defined(ART) && !defined(THIN_CLIENT_BUILD)
        case 8:
            hwCfgWrite8((A_UINT16)dkThread->currentDevIndex, pCmd->CMD_U.CFG_WRITE_CMD.cfgWriteAddr, (A_UINT8)pCmd->CMD_U.CFG_WRITE_CMD.cfgValue); 
            break;

        case 16:
            hwCfgWrite16((A_UINT16)dkThread->currentDevIndex, pCmd->CMD_U.CFG_WRITE_CMD.cfgWriteAddr, (A_UINT16)pCmd->CMD_U.CFG_WRITE_CMD.cfgValue); 
            break;
#endif
        case 32:
            hwCfgWrite32((A_UINT16)dkThread->currentDevIndex, pCmd->CMD_U.CFG_WRITE_CMD.cfgWriteAddr, pCmd->CMD_U.CFG_WRITE_CMD.cfgValue); 
            break;

        default:
            return(COMMS_ERR_BAD_CFG_SIZE);

        }
#endif
    
    return(0);
    }


/**************************************************************************
* processMemReadBlockCmd - Process MEM_READ_BLOCK_CMD command
*
* RETURNS: 0 processed OK, an Error code if it is not
*/
A_UINT32 processMemReadBlockCmd
(
	struct dkThreadInfo *dkThread
)
{
	PIPE_CMD      *pCmd;
	CMD_REPLY     *ReplyCmd_p;
	A_UCHAR     *replyBuf;

	pCmd = dkThread->PipeBuffer_p;
	ReplyCmd_p = dkThread->ReplyCmd_p;
	replyBuf = (A_UCHAR *)(ReplyCmd_p->cmdBytes);

    /* check that the size is appropriate before we access */
    if( pCmd->cmdLen != sizeof(pCmd->CMD_U.MEM_READ_BLOCK_CMD)+sizeof(pCmd->cmdID) + sizeof(pCmd->devNum))
        {
        uiPrintf("Error, command Len is not equal to size of MEM_READ_BLOCK_CMD\n");
        return(COMMS_ERR_BAD_LENGTH);
        }

#ifdef ENDIAN_SWAP
	pCmd->CMD_U.MEM_READ_BLOCK_CMD.physAddr = ltob_l(pCmd->CMD_U.MEM_READ_BLOCK_CMD.physAddr);		
	pCmd->CMD_U.MEM_READ_BLOCK_CMD.length = ltob_l(pCmd->CMD_U.MEM_READ_BLOCK_CMD.length);
#endif

    /* read the memory */
    if(hwMemReadBlock((A_UINT16)dkThread->currentDevIndex, replyBuf, pCmd->CMD_U.MEM_READ_BLOCK_CMD.physAddr, 
				  pCmd->CMD_U.MEM_READ_BLOCK_CMD.length) == -1)
        {
        uiPrintf("Failed call to hwMemReadBlock()\n");
        return(COMMS_ERR_READ_BLOCK_FAIL);
        }

#ifdef PREDATOR // Just to avoid mac endian register programming
// Currently this functions reads the byte array starting from the
// phyaddr, therefore endianness free


// Swap from big endian to little endian.
#ifdef ENDIAN_SWAP
{
    int noWords;
    int i;
    A_UINT32 *wordPtr;

    noWords =   pCmd->CMD_U.MEM_READ_BLOCK_CMD.length/sizeof(A_UINT32);
    wordPtr = (A_UINT32 *)replyBuf;
    for (i=0;i<noWords;i++)
    {
    	*(wordPtr + i) = ltob_l(*(wordPtr + i));
    }
}
#endif

#endif

    return(0);
    }

/**************************************************************************
* processMemWriteBlockCmd - Process MEM_WRITE_BLOCK_CMD command
*
* RETURNS: 0 processed OK, an Error code if it is not
*/
A_UINT32 processMemWriteBlockCmd
(
	struct dkThreadInfo *dkThread
)
{
	PIPE_CMD      *pCmd;
	CMD_REPLY     *ReplyCmd_p;
	A_UCHAR     *replyBuf;

	pCmd = dkThread->PipeBuffer_p;
	ReplyCmd_p = dkThread->ReplyCmd_p;
	replyBuf = (A_UCHAR *)(ReplyCmd_p->cmdBytes);

#ifdef ENDIAN_SWAP
     pCmd->CMD_U.MEM_WRITE_BLOCK_CMD.physAddr = ltob_l(pCmd->CMD_U.MEM_WRITE_BLOCK_CMD.physAddr);
     pCmd->CMD_U.MEM_WRITE_BLOCK_CMD.length = ltob_l(pCmd->CMD_U.MEM_WRITE_BLOCK_CMD.length);
#endif


	/* check the size , but need to access the structure to do it */
    if( pCmd->cmdLen != sizeof(pCmd->cmdID) + sizeof(pCmd->devNum) + sizeof(pCmd->CMD_U.MEM_WRITE_BLOCK_CMD.physAddr) +
		sizeof(pCmd->CMD_U.MEM_WRITE_BLOCK_CMD.length) + pCmd->CMD_U.MEM_WRITE_BLOCK_CMD.length ) 
        {
        uiPrintf("Error: command Len is not equal to size of expected bytes for MEM_WRITE_BLOCK_CMD\n");
		uiPrintf("     : got %d, expected %d\n", pCmd->cmdLen, sizeof(pCmd->cmdID) +
			sizeof(pCmd->CMD_U.MEM_WRITE_BLOCK_CMD.physAddr) +
			sizeof(pCmd->CMD_U.MEM_WRITE_BLOCK_CMD.length) + pCmd->CMD_U.MEM_WRITE_BLOCK_CMD.length);
        return(COMMS_ERR_BAD_LENGTH);
        }

#ifdef PREDATOR // Just to avoid mac endian register programming
// Currently this functions writes the byte array starting from the
// phyaddr, therefore endianness free


// swap from little endian to big endian
#ifdef ENDIAN_SWAP
{
    int noWords;
    int i;
    A_UINT32 *wordPtr;

    noWords =   pCmd->CMD_U.MEM_WRITE_BLOCK_CMD.length/sizeof(A_UINT32);
    wordPtr = (A_UINT32 *)pCmd->CMD_U.MEM_WRITE_BLOCK_CMD.bytes;
    for (i=0;i<noWords;i++)
    {
    	*(wordPtr + i) = ltob_l(*(wordPtr + i));
    }
}
#endif

#endif

 
    /* write the memory */
    if(hwMemWriteBlock((A_UINT16)dkThread->currentDevIndex,pCmd->CMD_U.MEM_WRITE_BLOCK_CMD.bytes, 
				   pCmd->CMD_U.MEM_WRITE_BLOCK_CMD.length, 
				   &(pCmd->CMD_U.MEM_WRITE_BLOCK_CMD.physAddr)) == -1)
        {
        uiPrintf("Failed call to hwMemWriteBlock()\n");
        return(COMMS_ERR_WRITE_BLOCK_FAIL);
        }
    /* copy the address into the return buffer */
    *(A_UINT32 *)replyBuf = pCmd->CMD_U.MEM_WRITE_BLOCK_CMD.physAddr;

#ifdef ENDIAN_SWAP
    *((A_UINT32 *)replyBuf) = btol_l(*((A_UINT32 *)replyBuf));
#endif

    return(0);
}


/**************************************************************************
* processCreateEventCmd - Process CREATE_EVENT_CMD command
*
* RETURNS: 0 processed OK, an Error code if it is not
*/
A_UINT32 processCreateEventCmd
(
	struct dkThreadInfo *dkThread
)
{
	PIPE_CMD      *pCmd;
	CMD_REPLY     *ReplyCmd_p;
	A_UCHAR     *replyBuf;

	pCmd = dkThread->PipeBuffer_p;
	ReplyCmd_p = dkThread->ReplyCmd_p;
	replyBuf = (A_UCHAR *)(ReplyCmd_p->cmdBytes);

    /* check that the size is appropriate before we access */
    if( pCmd->cmdLen != sizeof(pCmd->CMD_U.CREATE_EVENT_CMD)+sizeof(pCmd->cmdID) + sizeof(pCmd->devNum)) 
        {
        uiPrintf("Error, command Len is not equal to size of CREATE_EVENT_CMD\n");
        return(COMMS_ERR_BAD_LENGTH);
        }

#ifdef ENDIAN_SWAP
	pCmd->CMD_U.CREATE_EVENT_CMD.type = ltob_l(pCmd->CMD_U.CREATE_EVENT_CMD.type);
	pCmd->CMD_U.CREATE_EVENT_CMD.persistent = ltob_l(pCmd->CMD_U.CREATE_EVENT_CMD.persistent);
	pCmd->CMD_U.CREATE_EVENT_CMD.param1 = ltob_l(pCmd->CMD_U.CREATE_EVENT_CMD.param1);
	pCmd->CMD_U.CREATE_EVENT_CMD.param2 = ltob_l(pCmd->CMD_U.CREATE_EVENT_CMD.param2);
	pCmd->CMD_U.CREATE_EVENT_CMD.param3 = ltob_l(pCmd->CMD_U.CREATE_EVENT_CMD.param3);
	pCmd->CMD_U.CREATE_EVENT_CMD.eventHandle.eventID = ltob_s(pCmd->CMD_U.CREATE_EVENT_CMD.eventHandle.eventID);
	pCmd->CMD_U.CREATE_EVENT_CMD.eventHandle.f2Handle = ltob_s(pCmd->CMD_U.CREATE_EVENT_CMD.eventHandle.f2Handle);	
#endif        

	  if(hwCreateEvent((A_UINT16)dkThread->currentDevIndex,pCmd) == -1)
        {
        uiPrintf("Error:  unable to Create event\n");
        return(COMMS_ERR_EVENT_CREATE_FAIL);
        }
    return(0);
    }

/**************************************************************************
* processGetEventCmd - Process GET_EVENT_CMD command
*
* Get the first event from the triggered Q
*
* RETURNS: 0 processed OK, an Error code if it is not
*/
A_UINT32 processGetEventCmd
(
	struct dkThreadInfo *dkThread
)
{
	PIPE_CMD      *pCmd;
	CMD_REPLY     *ReplyCmd_p;
	A_UCHAR     *replyBuf;
	MDK_WLAN_DEV_INFO    *pdevInfo;
	EVENT_STRUCT  *evt;


	pCmd = dkThread->PipeBuffer_p;
	ReplyCmd_p = dkThread->ReplyCmd_p;
	replyBuf = (A_UCHAR *)(ReplyCmd_p->cmdBytes);
	pdevInfo = globDrvInfo.pDevInfoArray[dkThread->currentDevIndex];

    // check that the size is appropriate before we access
    if( pCmd->cmdLen != (sizeof(pCmd->cmdID) + sizeof(pCmd->devNum))) {
        uiPrintf("Error, command Len is not correct for GET_EVENT_CMD\n");
        return(COMMS_ERR_BAD_LENGTH);
    }

#ifdef JUNGO
    // Check if the Q has an event.  Set type to 0 if no events...
    if(!(pdevInfo->pdkInfo->pSharedInfo->anyEvents)) {
		((EVENT_STRUCT *)replyBuf)->type = 0;
    } else {
		//call into the kernel plugin to get the event, event will get
		//copied straight into replyBuf
		if(hwGetNextEvent(dkThread->currentDevIndex,(void *)replyBuf) == -1) {
			uiPrintf("Error: Unable to get event from kernel plugin\n");
			return(COMMS_ERR_NO_EVENTS);
		}
	}
#else
	((EVENT_STRUCT *)replyBuf)->type = 0;
	if (hwGetNextEvent((A_UINT16)dkThread->currentDevIndex,(void *)replyBuf) == -1) {
			uiPrintf("Error: Unable to get event from kernel plugin\n");
			return(COMMS_ERR_NO_EVENTS);
	}
#endif

#ifdef ENDIAN_SWAP
    	((EVENT_STRUCT *)replyBuf)->type  = btol_l(((EVENT_STRUCT *)replyBuf)->type);
	((EVENT_STRUCT *)replyBuf)->result  = btol_l(((EVENT_STRUCT *)replyBuf)->result);
	((EVENT_STRUCT *)replyBuf)->eventHandle.eventID  = btol_s(((EVENT_STRUCT *)replyBuf)->eventHandle.eventID);
	((EVENT_STRUCT *)replyBuf)->eventHandle.f2Handle = btol_s(((EVENT_STRUCT *)replyBuf)->eventHandle.f2Handle);;
#endif

evt = (EVENT_STRUCT *)replyBuf;

    return(0);
}

A_UINT32 processCloseDeviceCmd
(
	struct dkThreadInfo *dkThread
)
{
	PIPE_CMD      *pCmd;
	CMD_REPLY     *ReplyCmd_p;
	A_UCHAR     *replyBuf;
	A_UINT16     devIndex;

	pCmd = dkThread->PipeBuffer_p;
	ReplyCmd_p = dkThread->ReplyCmd_p;
	replyBuf = (A_UCHAR *)(ReplyCmd_p->cmdBytes);

	/* check that the size is appropriate before we access */
     	if( pCmd->cmdLen != sizeof(pCmd->cmdID) + sizeof(pCmd->devNum) ) 
        {
        	uiPrintf("Error, command Len is not equal to size of CLOSE_DEVICE CMD\n");
        	return(COMMS_ERR_BAD_LENGTH);
        }

#if defined(THIN_CLIENT_BUILD)
	devIndex = dkThread->currentDevIndex;

    // Close low level structs for this device
	if (devIndex != (A_UINT16)-1) {
  		deviceCleanup(devIndex);
	}
#else
	devIndex = m_closeDevice(pCmd->devNum);
#endif

	
	semLock(sem);
	removeClientFromThread(dkThread, devIndex);
	semUnLock(sem);
    return 0;

}

/**************************************************************************
* eepromRead - Read from given EEPROM offset and return the 16 bit value
*
* RETURNS: 16 bit value from given offset (in a 32-bit value)
*/
A_UINT32 processEepromReadCmd
(
	struct dkThreadInfo *dkThread
)
{
	PIPE_CMD      *pCmd;
	CMD_REPLY     *ReplyCmd_p;
	A_UCHAR     *replyBuf;
#if defined(THIN_CLIENT_BUILD)
    A_UINT32    eepromValue, eepromOffset;
#endif

	pCmd = dkThread->PipeBuffer_p;
	ReplyCmd_p = dkThread->ReplyCmd_p;
	replyBuf = (A_UCHAR *)(ReplyCmd_p->cmdBytes);

	/* check that the size is appropriate before we access */
     	if( pCmd->cmdLen != sizeof(pCmd->CMD_U.EEPROM_READ_CMD)+sizeof(pCmd->cmdID) + sizeof(pCmd->devNum)) 
        {
        	uiPrintf("Error, command Len is not equal to size of EEPROM_READ_CMD\n");
        	return(COMMS_ERR_BAD_LENGTH);
        }

	#ifdef ENDIAN_SWAP
     		pCmd->CMD_U.EEPROM_READ_CMD.offset = ltob_l(pCmd->CMD_U.EEPROM_READ_CMD.offset);
	#endif

#if defined(THIN_CLIENT_BUILD)
#if defined(SPIRIT_AP) || defined(FREEDOM_AP) // || defined(PREDATOR)
    //  16 bit addressing 
    eepromOffset = pCmd->CMD_U.EEPROM_READ_CMD.offset << 1;
#ifdef ARCH_BIG_ENDIAN
    eepromValue = (sysFlashConfigRead(FLC_RADIOCFG, eepromOffset) << 8) | sysFlashConfigRead(FLC_RADIOCFG, (eepromOffset + 1));
#else 
	#ifdef RSPBYTESWAP
	    eepromValue = (sysFlashConfigRead(FLC_RADIOCFG, eepromOffset+1) << 8) | sysFlashConfigRead(FLC_RADIOCFG, (eepromOffset)); 
	#else
	    eepromValue = (sysFlashConfigRead(FLC_RADIOCFG, eepromOffset) << 8) | sysFlashConfigRead(FLC_RADIOCFG, (eepromOffset + 1)); 
	#endif
#endif

    *((A_UINT32 *)replyBuf) = eepromValue;
#else

#if defined(PREDATOR)
	if ( pCmd->CMD_U.EEPROM_READ_CMD.offset & 0x10000000)  {
    	eepromOffset = (pCmd->CMD_U.EEPROM_READ_CMD.offset & 0x7fffffff) + AR5523_FLASH_ADDRESS;
		eepromValue = apRegRead32(eepromOffset);
       	*((A_UINT32 *)replyBuf) = eepromValue;
	}
	else  {
    	eepromOffset = (pCmd->CMD_U.EEPROM_READ_CMD.offset<<1) + AR5523_FLASH_ADDRESS + cal_offset;
		eepromValue = apRegRead32(eepromOffset);
		if (eepromOffset & 0x2)
       		*((A_UINT32 *)replyBuf) = ((eepromValue >> 16) & 0xffff);
		else
       		*((A_UINT32 *)replyBuf) = (eepromValue & 0xffff);
	}
#endif

#if defined(PHOENIX)
	if ( pCmd->CMD_U.EEPROM_READ_CMD.offset & 0x10000000)  {
    	eepromOffset = (pCmd->CMD_U.EEPROM_READ_CMD.offset & 0x7fffffff);
		eepromValue = sysFlashConfigRead(FLC_BOOTLINE, eepromOffset);
       	*((A_UINT32 *)replyBuf) = eepromValue;
	}
	else  {
    	eepromOffset = (pCmd->CMD_U.EEPROM_READ_CMD.offset<<1);
//		eepromValue = sysFlashConfigRead(FLC_RADIOCFG, eepromOffset);
        eepromValue = sysFlashConfigRead(FLC_RADIOCFG, eepromOffset)  | (sysFlashConfigRead(FLC_RADIOCFG, (eepromOffset + 1)) << 8);
	/*	if (eepromOffset & 0x2)
       		*((A_UINT32 *)replyBuf) = ((eepromValue >> 16) & 0xffff);
		else
       		*((A_UINT32 *)replyBuf) = (eepromValue & 0xffff);
    */
        *((A_UINT32 *)replyBuf) = eepromValue & 0xffff;
	}
#endif
    //eepromValue = sysFlashRead(eepromOffset); // We call apRegRead32 as
							// we get an exception for the ecos generated code 
#if defined(AR6000)
    	eepromOffset = (pCmd->CMD_U.EEPROM_READ_CMD.offset<<1) ;
		//eepromValue = apRegRead32(eepromOffset);
		eepromValue = sysFlashRead(eepromOffset);
//A_PRINTF("off/val=%x/%x\n", eepromOffset, eepromValue);
       		*((A_UINT32 *)replyBuf) = (eepromValue & 0xffff);
/*
		if (eepromOffset & 0x2)
       		*((A_UINT32 *)replyBuf) = ((eepromValue >> 16) & 0xffff);
		else
       		*((A_UINT32 *)replyBuf) = (eepromValue & 0xffff);
*/

#endif

#endif
#else
    /* read the eeprom */
    *((A_UINT32 *)replyBuf) = m_eepromRead(pCmd->devNum,pCmd->CMD_U.EEPROM_READ_CMD.offset); 
#endif

#ifdef ENDIAN_SWAP
    *((A_UINT32 *)replyBuf) = btol_l(*((A_UINT32 *)replyBuf));
#endif
	
	return 0;

}

/**************************************************************************
* eepromWrite - Write to given EEPROM offset
*
*/

A_UINT32 processEepromWriteCmd
(
	struct dkThreadInfo *dkThread
)
{
	PIPE_CMD      *pCmd;
	CMD_REPLY     *ReplyCmd_p;
	A_UCHAR     *replyBuf;
    A_UINT32	 status=0;
#if defined(THIN_CLIENT_BUILD)
    A_UINT32    eepromValue, eepromOffset;
    A_UINT8 *pVal;
    A_UINT16 *pVal16;
#endif

	pCmd = dkThread->PipeBuffer_p;
	ReplyCmd_p = dkThread->ReplyCmd_p;
	replyBuf = (A_UCHAR *)(ReplyCmd_p->cmdBytes);

	/* check that the size is appropriate before we access */
     	if( pCmd->cmdLen != sizeof(pCmd->CMD_U.EEPROM_WRITE_CMD)+sizeof(pCmd->cmdID) + sizeof(pCmd->devNum)) 
        {
        	uiPrintf("Error, command Len is not equal to size of EEPROM_WRITE_CMD\n");
        	return(COMMS_ERR_BAD_LENGTH);
        }

	#ifdef ENDIAN_SWAP
     		pCmd->CMD_U.EEPROM_WRITE_CMD.offset = ltob_l(pCmd->CMD_U.EEPROM_WRITE_CMD.offset);
     		pCmd->CMD_U.EEPROM_WRITE_CMD.value  = ltob_l(pCmd->CMD_U.EEPROM_WRITE_CMD.value);
	#endif


#if defined(THIN_CLIENT_BUILD)
        pVal = (A_UINT8 *)&(pCmd->CMD_U.EEPROM_WRITE_CMD.value);

#if defined(SPIRIT_AP) || defined(FREEDOM_AP) // || defined(PREDATOR)
        sysFlashConfigWrite(FLC_RADIOCFG, pCmd->CMD_U.EEPROM_WRITE_CMD.offset << 1, (pVal+2) , 2);
#else

//		uiPrintf("********** SNOOP:pVal=%x:*pVal=%d:value=%x\n", pVal, *((A_UINT32*)pVal), pCmd->CMD_U.EEPROM_WRITE_CMD.value);

#if defined (PHOENIX)
		if ( pCmd->CMD_U.EEPROM_WRITE_CMD.offset & 0x10000000) 
           status = sysFlashConfigWrite(FLC_BOOTLINE, pCmd->CMD_U.EEPROM_WRITE_CMD.offset << 1, (pVal+2) , 2);
		else
           status = sysFlashConfigWrite(FLC_RADIOCFG, pCmd->CMD_U.EEPROM_WRITE_CMD.offset<<1, (pVal) , 2);
#endif
#if defined (PREDATOR)
           status = sysFlashConfigWrite(pCmd->CMD_U.EEPROM_WRITE_CMD.offset<<1, pVal+2 , 2);
#endif
#if defined (AR6000)
           status = sysFlashConfigWrite(pCmd->CMD_U.EEPROM_WRITE_CMD.offset<<1, pVal , 2);
#endif

#endif  //!SPIRIT_AP

#else // THIN_CLIENT_BUILD

    	/* write the eeprom */
    	m_eepromWrite(pCmd->devNum,pCmd->CMD_U.EEPROM_WRITE_CMD.offset,
		      pCmd->CMD_U.EEPROM_WRITE_CMD.value); 

#endif // THIN_CLIENT_BUILD

    	return status;

}

A_UINT32 processHwReset( struct dkThreadInfo *dkThread) {
	PIPE_CMD      *pCmd;
	CMD_REPLY     *ReplyCmd_p;
	A_UCHAR     *replyBuf;
	MDK_WLAN_DEV_INFO    *pdevInfo;
	A_UINT16 expCmdLen;

	pCmd = dkThread->PipeBuffer_p;
	ReplyCmd_p = dkThread->ReplyCmd_p;
	replyBuf = (A_UCHAR *)(ReplyCmd_p->cmdBytes);
    
	expCmdLen = sizeof(pCmd->CMD_U.HW_RESET_CMD)+sizeof(pCmd->cmdID) + sizeof(pCmd->devNum);
    /* check that the size is appropriate before we access */
    if( pCmd->cmdLen != expCmdLen)
        {
        uiPrintf("Error, command Len is not equal to size of HW_RESET_CMD\n");
        return(COMMS_ERR_BAD_LENGTH);
        }

	pdevInfo = globDrvInfo.pDevInfoArray[dkThread->currentDevIndex];
#if defined(FREEDOM_AP) || defined(PREDATOR) || defined(SENAO_AP)
    hwInit(pdevInfo, pCmd->CMD_U.HW_RESET_CMD.resetMask);
#else
#ifndef VXWORKS
    hwInit(pdevInfo, pCmd->CMD_U.HW_RESET_CMD.resetMask);
#endif
#endif

#if defined(FREEDOM_AP) 

#ifdef SOC_LINUX

#if defined(AR5312)
	*((A_UINT32 *)replyBuf) = (apRegRead32(dkThread->currentDevIndex, AR531X_REV) >> REV_WMAC_MIN_S)&0xff ;
#endif
#if defined(AR5315)
	*((A_UINT32 *)replyBuf) =  (apRegRead32(dkThread->devIndex, AR531XPLUS_WLAN0)+F2_SREV) & F2_SREV_ID_M ;
#endif

#else

#if defined(AR5312)
	*((A_UINT32 *)replyBuf) = (sysRegRead(AR531X_REV) >> REV_WMAC_MIN_S)&0xff ;
#endif
#if defined(AR5315)
	*((A_UINT32 *)replyBuf) =  (sysRegRead(AR531XPLUS_WLAN0)+F2_SREV) & F2_SREV_ID_M ;
#endif
#endif

#else
#if defined(PHOENIX)
	*((A_UINT32 *)replyBuf) = CHIP_REV_ID_GET(hwMemRead32((A_UINT16)dkThread->currentDevIndex,pdevInfo->pdkInfo->f2MapAddress + CHIP_REV_ADDRESS));
#else
	*((A_UINT32 *)replyBuf) = hwMemRead32((A_UINT16)dkThread->currentDevIndex,pdevInfo->pdkInfo->f2MapAddress + F2_SREV) & F2_SREV_ID_M;
#endif
#endif

#ifdef ENDIAN_SWAP
    *((A_UINT32 *)replyBuf) = btol_l(*((A_UINT32 *)replyBuf));
#endif

    return(0);
    }

A_UINT32 processPllProgram( struct dkThreadInfo *dkThread) {
	PIPE_CMD      *pCmd;
	CMD_REPLY     *ReplyCmd_p;
	A_UCHAR     *replyBuf;
	MDK_WLAN_DEV_INFO    *pdevInfo;
	A_INT8 mode, turbo;

	pCmd = dkThread->PipeBuffer_p;
	ReplyCmd_p = dkThread->ReplyCmd_p;
	replyBuf = (A_UCHAR *)(ReplyCmd_p->cmdBytes);
    
    /* check that the size is appropriate before we access */
    if( pCmd->cmdLen != sizeof(pCmd->CMD_U.PLL_PROGRAM_CMD)+sizeof(pCmd->cmdID) + sizeof(pCmd->devNum) )
        {
        uiPrintf("Error, command Len is not equal to size of PLL_PROGRAM_CMD\n");
        return(COMMS_ERR_BAD_LENGTH);
        }

	pdevInfo = globDrvInfo.pDevInfoArray[dkThread->currentDevIndex];
   //uiPrintf("mode=%d:addr=%x\n", pCmd->CMD_U.PLL_PROGRAM_CMD.mode, pdevInfo->pdkInfo->f2MapAddress );
#if defined(FREEDOM_AP) 
	hwMemWrite32(dkThread->currentDevIndex,pdevInfo->pdkInfo->f2MapAddress + 0xa200, 0);
	if(pCmd->CMD_U.PLL_PROGRAM_CMD.mode == MODE_11B)   {
	    hwMemWrite32(dkThread->currentDevIndex,pdevInfo->pdkInfo->f2MapAddress + 0x987c, 0x4b);
	} else {
	    hwMemWrite32(dkThread->currentDevIndex,pdevInfo->pdkInfo->f2MapAddress + 0x987c, 0x05);
	}
	milliSleep(2);
    hwInit(pdevInfo, MAC_RESET|BB_RESET);
#endif

#if defined(PREDATOR)

	mode = pCmd->CMD_U.PLL_PROGRAM_CMD.mode;
	turbo = pCmd->CMD_U.PLL_PROGRAM_CMD.turbo;

	switch(mode) {
		case MODE_11B:	
	      hwMemWrite32(dkThread->currentDevIndex,pdevInfo->pdkInfo->f2MapAddress + 0x987c, 0xd6);
		break;

		case MODE_11G:	
			switch(turbo) {
				case HALF_SPEED_MODE:
	      		  hwMemWrite32(dkThread->currentDevIndex,pdevInfo->pdkInfo->f2MapAddress + 0x987c, 0x1eb);
				break;
				case QUARTER_SPEED_MODE:
	      		  hwMemWrite32(dkThread->currentDevIndex,pdevInfo->pdkInfo->f2MapAddress + 0x987c, 0x2eb);
				break;
				case TURBO_ENABLE:
	      		  hwMemWrite32(dkThread->currentDevIndex,pdevInfo->pdkInfo->f2MapAddress + 0x987c, 0xea);
				break;
				default:
	      		  hwMemWrite32(dkThread->currentDevIndex,pdevInfo->pdkInfo->f2MapAddress + 0x987c, 0xeb);
			    break;
			}
		break;

		case MODE_11A:
		case MODE_11O:
			switch(turbo) {
				case HALF_SPEED_MODE:
	      		  hwMemWrite32(dkThread->currentDevIndex,pdevInfo->pdkInfo->f2MapAddress + 0x987c, 0x1ea);
				break;
				case QUARTER_SPEED_MODE:
	      		  hwMemWrite32(dkThread->currentDevIndex,pdevInfo->pdkInfo->f2MapAddress + 0x987c, 0x2ea);
				break;
				default:
	      		  hwMemWrite32(dkThread->currentDevIndex,pdevInfo->pdkInfo->f2MapAddress + 0x987c, 0xea);
				break;
			}
		break;
    }

	milliSleep(2);
    hwInit(pdevInfo, MAC_RESET|BB_RESET);
#endif

#if defined(AR6000)
	mode = pCmd->CMD_U.PLL_PROGRAM_CMD.mode;
	turbo = pCmd->CMD_U.PLL_PROGRAM_CMD.turbo;
    switch(mode) {
		case MODE_11G:	
		case MODE_11B:	
		case MODE_11O:
            A_WLAN_BAND_SET(A_BAND_24GHZ);
            break;
		case MODE_11A:
            A_WLAN_BAND_SET(A_BAND_5GHZ);
            break;
    }
#endif

    return(0);
}

A_UINT32 processPciWriteCmd
(
	struct dkThreadInfo *dkThread
)
{
	PIPE_CMD      *pCmd;
	CMD_REPLY     *ReplyCmd_p;
	A_UCHAR     *replyBuf;
	PCI_VALUES 	*pPciValues;
	A_UINT32 i;
	MDK_WLAN_DEV_INFO    *pdevInfo;
	A_UINT32 dataSize;

	pdevInfo = globDrvInfo.pDevInfoArray[dkThread->currentDevIndex];
	pCmd = dkThread->PipeBuffer_p;
	ReplyCmd_p = dkThread->ReplyCmd_p;
	replyBuf = (A_UCHAR *)(ReplyCmd_p->cmdBytes);

	pPciValues = pCmd->CMD_U.PCI_WRITE_CMD.pPciValues;

	#ifdef ENDIAN_SWAP
     	pCmd->CMD_U.PCI_WRITE_CMD.length = ltob_l(pCmd->CMD_U.PCI_WRITE_CMD.length); 
	    for(i=0; i<pCmd->CMD_U.PCI_WRITE_CMD.length; i++) {
			pPciValues[i].offset = ltob_l(pPciValues[i].offset);
			pPciValues[i].baseValue = ltob_l(pPciValues[i].baseValue);
		}
	#endif

	dataSize = sizeof(pCmd->CMD_U.PCI_WRITE_CMD) - ((MAX_PCI_ENTRIES-pCmd->CMD_U.PCI_WRITE_CMD.length) * sizeof(PCI_VALUES));
	//dataSize = pCmd->CMD_U.PCI_WRITE_CMD.length * sizeof(PCI_VALUES);

    if( pCmd->cmdLen != (sizeof(pCmd->cmdID) + dataSize + sizeof(pCmd->devNum)))
    {
       uiPrintf("Error: command Len is not equal to size of expected bytes for PCI_WRITE_CMD\n");
	   uiPrintf(": got %d, expected %d\n", pCmd->cmdLen, (sizeof(pCmd->cmdID) + dataSize + sizeof(pCmd->devNum)));
       return(COMMS_ERR_BAD_LENGTH);
    }

	for(i=0; i<pCmd->CMD_U.PCI_WRITE_CMD.length; i++) {
	  hwMemWrite32((A_UINT16)dkThread->currentDevIndex,pdevInfo->pdkInfo->f2MapAddress + pPciValues[i].offset, pPciValues[i].baseValue);
	}
  return (0);
}

A_UINT32 processCalCheckCmd( struct dkThreadInfo *dkThread) {
	PIPE_CMD      *pCmd;
	CMD_REPLY     *ReplyCmd_p;
	A_UCHAR     *replyBuf;
	A_UINT32 i;
	A_UINT32 enableCal, regValue;
	MDK_WLAN_DEV_INFO    *pdevInfo;

	pdevInfo = globDrvInfo.pDevInfoArray[dkThread->currentDevIndex];

	pCmd = dkThread->PipeBuffer_p;
	ReplyCmd_p = dkThread->ReplyCmd_p;
	replyBuf = (A_UCHAR *)(ReplyCmd_p->cmdBytes);
    
    /* check that the size is appropriate before we access */
    if( pCmd->cmdLen != sizeof(pCmd->CMD_U.CAL_CHECK_CMD)+sizeof(pCmd->cmdID) + sizeof(pCmd->devNum) )
        {
        uiPrintf("Error, command Len is not equal to size of CAL_CHECK_CMD\n");
        return(COMMS_ERR_BAD_LENGTH);
        }

#ifdef ENDIAN_SWAP
		pCmd->CMD_U.CAL_CHECK_CMD.timeout = ltob_l(pCmd->CMD_U.CAL_CHECK_CMD.timeout);
		pCmd->CMD_U.CAL_CHECK_CMD.enableCal = ltob_l(pCmd->CMD_U.CAL_CHECK_CMD.enableCal);
#endif
		enableCal = pCmd->CMD_U.CAL_CHECK_CMD.enableCal;

			regValue = hwMemRead32((A_UINT16)dkThread->currentDevIndex,pdevInfo->pdkInfo->f2MapAddress + PHY_AGC_CONTROL);
		  for (i = 0; i < pCmd->CMD_U.CAL_CHECK_CMD.timeout; i++)
		  {
			regValue = hwMemRead32((A_UINT16)dkThread->currentDevIndex,pdevInfo->pdkInfo->f2MapAddress + PHY_AGC_CONTROL);
			if ((regValue & enableCal) == 0 )
			{
				break;
			}
			milliSleep(1);
		  }


		*((A_UINT32 *)replyBuf)  = i;
#ifdef ENDIAN_SWAP
        *((A_UINT32 *)replyBuf) = btol_l(*((A_UINT32 *)replyBuf));
#endif

  return (0);

}

#if  defined(PREDATOR) || defined(AR6000)
A_UINT32 processEepromReadBlockCmd
(
	struct dkThreadInfo *dkThread
)
{
	PIPE_CMD      *pCmd;
	CMD_REPLY     *ReplyCmd_p;
	A_UCHAR     *replyBuf;
	PDWORDBUFFER pDwordBuffer = NULL;
	A_UINT32 *pDword;
	A_UINT32 i;
    A_UINT32    eepromValue, eepromOffset, j, eepOffset;

    A_PRINTF("!!!\n");
	pCmd = dkThread->PipeBuffer_p;
	ReplyCmd_p = dkThread->ReplyCmd_p;
	replyBuf = (A_UCHAR *)(ReplyCmd_p->cmdBytes);

	/* check that the size is appropriate before we access */
     	if( pCmd->cmdLen != sizeof(pCmd->CMD_U.EEPROM_READ_BLOCK_CMD)+sizeof(pCmd->cmdID) + sizeof(pCmd->devNum)) 
        {
        	uiPrintf("Error, command Len is not equal to size of EEPROM_READ_BLOCK_CMD\n");
        	return(COMMS_ERR_BAD_LENGTH);
        }

	#ifdef ENDIAN_SWAP
     		pCmd->CMD_U.EEPROM_READ_BLOCK_CMD.startOffset = ltob_l(pCmd->CMD_U.EEPROM_READ_BLOCK_CMD.startOffset);
     		pCmd->CMD_U.EEPROM_READ_BLOCK_CMD.length = ltob_l(pCmd->CMD_U.EEPROM_READ_BLOCK_CMD.length);
	#endif

	pDword = (A_UINT32 *)replyBuf;	
	eepOffset = AR5523_FLASH_ADDRESS + cal_offset;
  A_PRINTF("RD Block len = %d\n", pCmd->CMD_U.EEPROM_READ_BLOCK_CMD.length); 
	for  (j=0;j<pCmd->CMD_U.EEPROM_READ_BLOCK_CMD.length;j++)	{
            //eepromValue = sysFlashRead(eepromOffse
#ifdef AR6000
	    eepromOffset = ((pCmd->CMD_U.EEPROM_READ_BLOCK_CMD.startOffset+j)<<1);
	    eepromValue = sysFlashRead(eepromOffset);
        pDword[j] = (A_UINT32) (eepromValue & 0xffff);
#else
	    eepromOffset = ((pCmd->CMD_U.EEPROM_READ_BLOCK_CMD.startOffset+j)<<1)+eepOffset;
	    eepromValue = apRegRead32(eepromOffset);
	        if (eepromOffset & 0x2)
              pDword[j] = (A_UINT32) ( (eepromValue>>16) & 0xffff);
		    else
              pDword[j] = (A_UINT32) (eepromValue & 0xffff);
#endif
	#ifdef ENDIAN_SWAP
	  	  		pDword[j] = btol_l(pDword[j]);
	#endif
    }

	return 0;
}
#else
/**************************************************************************
* eepromRead - Read from given EEPROM offset and return the 16 bit value
*/

A_UINT32 processEepromReadBlockCmd
(
	struct dkThreadInfo *dkThread
)
{
	PIPE_CMD      *pCmd;
	CMD_REPLY     *ReplyCmd_p;
	A_UCHAR     *replyBuf;
	PDWORDBUFFER pDwordBuffer = NULL;
	A_UINT32 *pDword;
	A_UINT32 i;
#if defined(THIN_CLIENT_BUILD)
    A_UINT32    eepromValue, eepromOffset, j;
#endif

	pCmd = dkThread->PipeBuffer_p;
	ReplyCmd_p = dkThread->ReplyCmd_p;
	replyBuf = (A_UCHAR *)(ReplyCmd_p->cmdBytes);

	/* check that the size is appropriate before we access */
     	if( pCmd->cmdLen != sizeof(pCmd->CMD_U.EEPROM_READ_BLOCK_CMD)+sizeof(pCmd->cmdID) + sizeof(pCmd->devNum)) 
        {
        	uiPrintf("Error, command Len is not equal to size of EEPROM_READ_BLOCK_CMD\n");
        	return(COMMS_ERR_BAD_LENGTH);
        }

	#ifdef ENDIAN_SWAP
     		pCmd->CMD_U.EEPROM_READ_BLOCK_CMD.startOffset = ltob_l(pCmd->CMD_U.EEPROM_READ_BLOCK_CMD.startOffset);
     		pCmd->CMD_U.EEPROM_READ_BLOCK_CMD.length = ltob_l(pCmd->CMD_U.EEPROM_READ_BLOCK_CMD.length);
	#endif

#if defined(THIN_CLIENT_BUILD)
	pDwordBuffer = (PDWORDBUFFER) A_MALLOC(sizeof(DWORDBUFFER));
    	if (!pDwordBuffer) {
        	return(COMMS_ERR_ALLOC_FAIL);
    	}

	pDwordBuffer->pDword = (A_UINT32 *)A_MALLOC( pCmd->CMD_U.EEPROM_READ_BLOCK_CMD.length* 4);
	if(!pDwordBuffer->pDword) {
		A_FREE(pDwordBuffer);
 		return(COMMS_ERR_ALLOC_FAIL);
    }
#if defined(SPIRIT_AP) || defined(FREEDOM_AP) // || defined(PREDATOR)
	for  (j=0;j<pCmd->CMD_U.EEPROM_READ_BLOCK_CMD.length;j++)	{
	    eepromOffset = (pCmd->CMD_U.EEPROM_READ_BLOCK_CMD.startOffset+j) << 1;
        pDwordBuffer->pDword[j] = (A_UINT32)((sysFlashConfigRead(FLC_RADIOCFG, eepromOffset) << 8) | (sysFlashConfigRead(FLC_RADIOCFG, (eepromOffset + 1))));
    }
#else // !SPIRIT_AP || !FREEDOM_AP

#if defined(PHOENIX)
	for  (j=0;j<pCmd->CMD_U.EEPROM_READ_BLOCK_CMD.length;j++)	{
    	eepromOffset = (pCmd->CMD_U.EEPROM_READ_CMD.offset<<1);
        eepromValue = sysFlashConfigRead(FLC_RADIOCFG, eepromOffset)  | (sysFlashConfigRead(FLC_RADIOCFG, (eepromOffset + 1)) << 8);
    }
#else
	for  (j=0;j<pCmd->CMD_U.EEPROM_READ_BLOCK_CMD.length;j++)	{
	    if ( pCmd->CMD_U.EEPROM_READ_BLOCK_CMD.startOffset & 0x10000000)  {
    	    eepromOffset = (pCmd->CMD_U.EEPROM_READ_BLOCK_CMD.startOffset & 0x7fffffff) + AR5523_FLASH_ADDRESS;
		    eepromValue = apRegRead32(eepromOffset);
            pDwordBuffer->pDword[j] = (A_UINT32) eepromValue;
	    }
		else {
	        eepromOffset = ( ((pCmd->CMD_U.EEPROM_READ_BLOCK_CMD.startOffset+j)<<1)+ AR5523_FLASH_ADDRESS + cal_offset);
            //eepromValue = sysFlashRead(eepromOffset);
	        eepromValue = apRegRead32(eepromOffset);
	        if (eepromOffset & 0x2)
              pDwordBuffer->pDword[j] = (A_UINT32) ( (eepromValue>>16) & 0xffff);
		    else
              pDwordBuffer->pDword[j] = (A_UINT32) (eepromValue & 0xffff);
		}
    }
#endif

#endif // !SPIRIT_AP || !FREEDOM_AP
#else // THIN_CLIENT_BUILD
	pDwordBuffer = m_eepromReadBlock(pCmd->devNum,
					 pCmd->CMD_U.EEPROM_READ_BLOCK_CMD.startOffset,
					 pCmd->CMD_U.EEPROM_READ_BLOCK_CMD.length);
#endif // THIN_CLIENT_BUILD

	pDword = (A_UINT32 *)replyBuf;	

	if (pDwordBuffer != NULL)
	{
		for (i=0;i<pCmd->CMD_U.EEPROM_READ_BLOCK_CMD.length;i++)
		{
			#ifdef ENDIAN_SWAP
	  	  		pDword[i] = btol_l(pDwordBuffer->pDword[i]);
			#else
				pDword[i] = pDwordBuffer->pDword[i];
			#endif
		}
		A_FREE(pDwordBuffer->pDword);
		A_FREE(pDwordBuffer);
	}
	return 0;
}
#endif

#ifdef AR6000
/***************************************************************************/

A_UINT32 processEepromWriteBlockCmd
(
	struct dkThreadInfo *dkThread
)
{
	PIPE_CMD      *pCmd;
	CMD_REPLY     *ReplyCmd_p;
	A_UCHAR     *replyBuf;
	A_UINT32 i;
    A_INT32	 status=0;
	pCmd = dkThread->PipeBuffer_p;
	ReplyCmd_p = dkThread->ReplyCmd_p;
	replyBuf = (A_UCHAR *)(ReplyCmd_p->cmdBytes);

    // Delete these when eeprom routines in place
	#ifdef ENDIAN_SWAP
     		pCmd->CMD_U.EEPROM_WRITE_BLOCK_CMD.length = ltob_l(pCmd->CMD_U.EEPROM_WRITE_BLOCK_CMD.length); 
	#endif


    	if( pCmd->cmdLen != (sizeof(pCmd->cmdID) + 
		       sizeof(pCmd->CMD_U.EEPROM_WRITE_BLOCK_CMD)-
		       ((MAX_BLOCK_DWORDS - pCmd->CMD_U.EEPROM_WRITE_BLOCK_CMD.length) * 4)) + sizeof(pCmd->devNum))
        {
        	uiPrintf("Error: command Len is not equal to size of expected bytes for EEPROM_WRITE_BLOCK_CMD\n");
			uiPrintf(": got %d, expected %d\n", pCmd->cmdLen, (sizeof(pCmd->cmdID) + sizeof(pCmd->CMD_U.EEPROM_WRITE_BLOCK_CMD)
				- ((MAX_BLOCK_DWORDS - pCmd->CMD_U.EEPROM_WRITE_BLOCK_CMD.length) * 4)));
        	return(COMMS_ERR_BAD_LENGTH);
        }


	#ifdef ENDIAN_SWAP
     		pCmd->CMD_U.EEPROM_WRITE_BLOCK_CMD.startOffset = ltob_l(pCmd->CMD_U.EEPROM_WRITE_BLOCK_CMD.startOffset);
	

	#endif

    {
	    A_UINT8 *pVal;
	    A_UINT16 *pTempPtr;
        A_UINT32 length;
        A_UINT32 startOffset, offset;
        length=(A_UINT16)pCmd->CMD_U.EEPROM_WRITE_BLOCK_CMD.length;

	    //copy the data into 16bit data buffer
	    pTempPtr = pTempBuffer;
A_PRINTF("Copy eeprom data (%d) to %x\n", length, pTempPtr);
	    for(i = 0; i < length; i++) {
		#ifdef ENDIAN_SWAP
			*pTempPtr = ltob_l(pCmd->CMD_U.EEPROM_WRITE_BLOCK_CMD.eepromValue[i]); 
		#else
			*pTempPtr = pCmd->CMD_U.EEPROM_WRITE_BLOCK_CMD.eepromValue[i];
		#endif
		    pTempPtr++;
	    }

	    pVal = (A_UINT8 *)pTempBuffer;
A_PRINTF("Start eeprom write \n");
    // 16 bit addressing
        startOffset = pCmd->CMD_U.EEPROM_WRITE_BLOCK_CMD.startOffset;
         offset = pCmd->CMD_U.EEPROM_WRITE_BLOCK_CMD.startOffset << 1;
#ifdef ARCH_BIG_ENDIAN
         status=sysFlashConfigWrite(offset, pVal, (length * sizeof(A_UINT16)));
#else 
         //status=sysFlashConfigWrite(offset, pVal, (length * sizeof(A_UINT16)));
#endif
A_PRINTF("eeprom write done\n");
  }

   return 0;

}

#else
/***************************************************************************/

A_UINT32 processEepromWriteBlockCmd
(
	struct dkThreadInfo *dkThread
)
{
	PIPE_CMD      *pCmd;
	CMD_REPLY     *ReplyCmd_p;
	A_UCHAR     *replyBuf;
	PDWORDBUFFER pDwordBuffer;
	A_UINT32 i;
    A_INT32	 status=0;

	pCmd = dkThread->PipeBuffer_p;
	ReplyCmd_p = dkThread->ReplyCmd_p;
	replyBuf = (A_UCHAR *)(ReplyCmd_p->cmdBytes);
	#ifdef ENDIAN_SWAP
     		pCmd->CMD_U.EEPROM_WRITE_BLOCK_CMD.length = ltob_l(pCmd->CMD_U.EEPROM_WRITE_BLOCK_CMD.length); 
	#endif


    	if( pCmd->cmdLen != (sizeof(pCmd->cmdID) + 
		       sizeof(pCmd->CMD_U.EEPROM_WRITE_BLOCK_CMD)-
		       ((MAX_BLOCK_DWORDS - pCmd->CMD_U.EEPROM_WRITE_BLOCK_CMD.length) * 4)) + sizeof(pCmd->devNum))
        {
        	uiPrintf("Error: command Len is not equal to size of expected bytes for EEPROM_WRITE_BLOCK_CMD\n");
			uiPrintf(": got %d, expected %d\n", pCmd->cmdLen, (sizeof(pCmd->cmdID) + sizeof(pCmd->CMD_U.EEPROM_WRITE_BLOCK_CMD)
				- ((MAX_BLOCK_DWORDS - pCmd->CMD_U.EEPROM_WRITE_BLOCK_CMD.length) * 4)));
        	return(COMMS_ERR_BAD_LENGTH);
        }


	#ifdef ENDIAN_SWAP
     		pCmd->CMD_U.EEPROM_WRITE_BLOCK_CMD.startOffset = ltob_l(pCmd->CMD_U.EEPROM_WRITE_BLOCK_CMD.startOffset);
	

	#endif

	pDwordBuffer = (PDWORDBUFFER)A_MALLOC(sizeof(DWORDBUFFER));
	if (pDwordBuffer == NULL)
	{
		uiPrintf("Error, unable to allocate memory for mac PDWORDBUFFER struct \n");
		return (COMMS_ERR_ALLOC_FAIL);	
	}
	pDwordBuffer->Length = (A_UINT16)pCmd->CMD_U.EEPROM_WRITE_BLOCK_CMD.length * 4;

	pDwordBuffer->pDword = (A_UINT32 *)A_MALLOC(pDwordBuffer->Length);
	if (pDwordBuffer->pDword == NULL)
	{
		A_FREE(pDwordBuffer);
		uiPrintf("Error, unable to allocate memory for mac array \n");
		return (COMMS_ERR_ALLOC_FAIL);	
	}
	

	for (i=0;i<pCmd->CMD_U.EEPROM_WRITE_BLOCK_CMD.length;i++)
	{
		#ifdef ENDIAN_SWAP
			pDwordBuffer->pDword[i] = ltob_l(pCmd->CMD_U.EEPROM_WRITE_BLOCK_CMD.eepromValue[i]); 
		#else
			pDwordBuffer->pDword[i] = pCmd->CMD_U.EEPROM_WRITE_BLOCK_CMD.eepromValue[i];
		#endif
	}

#if defined(THIN_CLIENT_BUILD)
    {
	    A_UINT8 *pVal;
	    A_UINT16 *pTempBuffer = NULL;
	    A_UINT16 *pTempPtr;
        A_UINT32 length;
        A_UINT32 startOffset, offset;
        length=(A_UINT16)pCmd->CMD_U.EEPROM_WRITE_BLOCK_CMD.length;

	    //copy the data into 16bit data buffer
	    pTempBuffer = (A_UINT16 *)malloc(length * sizeof(A_UINT16));
	    pTempPtr = pTempBuffer;
	    for(i = 0; i < length; i++) {
		    *pTempPtr = pDwordBuffer->pDword[i] & 0xffff; 
		    pTempPtr++;
	    }

	    pVal = (A_UINT8 *)pTempBuffer;
    // 16 bit addressing
        startOffset = pCmd->CMD_U.EEPROM_WRITE_BLOCK_CMD.startOffset;
#if defined(SPIRIT_AP) || defined(FREEDOM_AP) // || defined(PREDATOR)
	    startOffset = startOffset << 1;	
            q_uiPrintf("Write to eeprom @ %x : %x \n",startOffset, *pVal); 
        // write only the lower 2 bytes
        sysFlashConfigWrite(FLC_RADIOCFG, startOffset, pVal , length*sizeof(A_UINT16));
#else // SPIRIT_AP || FREEDOM_AP

#if defined(PHOENIX)
	  if (startOffset & 0x10000000) {
		 pVal = &(pDwordBuffer->pDword[0]);
         status=sysFlashConfigWrite(FLC_BOOTLINE, offset, pVal, length*2);
	  }
	  else {
         offset = pCmd->CMD_U.EEPROM_WRITE_BLOCK_CMD.startOffset << 1;
         status=sysFlashConfigWrite(FLC_RADIOCFG, offset, pVal, (length * sizeof(A_UINT16)));
	  }
#endif
#if defined(PREDATOR)
	  if (startOffset & 0x10000000) {
         offset = pCmd->CMD_U.EEPROM_WRITE_BLOCK_CMD.startOffset << 1 | 0x10000000;
	  }
	  else {
         offset = pCmd->CMD_U.EEPROM_WRITE_BLOCK_CMD.startOffset << 1;
	  }
         status=sysFlashConfigWrite(offset, pVal, (length * sizeof(A_UINT16)));
#endif

#endif // !SPIRIT_AP || !FREEDOM_AP
        	free(pTempBuffer);
  }
#else // THIN_CLIENT_BUILD
    	/* write the eeprom */
    	m_eepromWriteBlock(pCmd->devNum,
			   pCmd->CMD_U.EEPROM_WRITE_BLOCK_CMD.startOffset,
			   pDwordBuffer); 
#endif

	A_FREE(pDwordBuffer->pDword);
	A_FREE(pDwordBuffer);

    	return 0;

}
#endif

/***************************************************************************/

A_UINT32 processEepromWriteByteBasedBlockCmd
(
	struct dkThreadInfo *dkThread
)
{
	PIPE_CMD      *pCmd;
	CMD_REPLY     *ReplyCmd_p;
	A_UCHAR       *replyBuf;
	A_UINT32      i;
        A_INT32	      status=0;
	A_UINT8       *pVal;
        A_UINT32      offset, length;

	pCmd = dkThread->PipeBuffer_p;
	ReplyCmd_p = dkThread->ReplyCmd_p;
	replyBuf = (A_UCHAR *)(ReplyCmd_p->cmdBytes);
	#ifdef ENDIAN_SWAP
     		pCmd->CMD_U.EEPROM_WRITE_BYTEBASED_BLOCK_CMD.length = ltob_l(pCmd->CMD_U.EEPROM_WRITE_BYTEBASED_BLOCK_CMD.length); 
	#endif

        length = pCmd->CMD_U.EEPROM_WRITE_BYTEBASED_BLOCK_CMD.length; 

    	if( pCmd->cmdLen != (sizeof(pCmd->cmdID) + 
		       sizeof(pCmd->CMD_U.EEPROM_WRITE_BYTEBASED_BLOCK_CMD)-
		       (MAX_BLOCK_BYTES - pCmd->CMD_U.EEPROM_WRITE_BYTEBASED_BLOCK_CMD.length) ) + sizeof(pCmd->devNum))
        {
        	uiPrintf("Error: command Len is not equal to size of expected bytes for EEPROM_WRITE_BYTEBASED_BLOCK_CMD\n");
		uiPrintf(": got %d, expected %d\n", pCmd->cmdLen, 
                     (sizeof(pCmd->cmdID) + sizeof(pCmd->devNum) + 
                     sizeof(pCmd->CMD_U.EEPROM_WRITE_BYTEBASED_BLOCK_CMD)
		     - (MAX_BLOCK_BYTES - pCmd->CMD_U.EEPROM_WRITE_BYTEBASED_BLOCK_CMD.length) ));
        	return(COMMS_ERR_BAD_LENGTH);
        }

	#ifdef ENDIAN_SWAP
     		pCmd->CMD_U.EEPROM_WRITE_BYTEBASED_BLOCK_CMD.startOffset = ltob_l(pCmd->CMD_U.EEPROM_WRITE_BYTEBASED_BLOCK_CMD.startOffset);
	#endif

        offset = pCmd->CMD_U.EEPROM_WRITE_BYTEBASED_BLOCK_CMD.startOffset;

        //copy the data into 8bit data buffer
        pVal = (A_UINT8 *)malloc(length * sizeof(A_UINT8));
        for(i = 0; i < length; i++) {
	    *(pVal + i) = 
             *(((A_UINT8 *)pCmd->CMD_U.EEPROM_WRITE_BLOCK_CMD.eepromValue) + i);
        }

    	/* write the eeprom */


#ifndef WIN_CLIENT_BUILD
#ifdef OWL_PB42
	if(isHowlAP(pCmd->devNum) || isFlashCalData()){
        	status=sysFlashConfigWrite(pCmd->devNum, FLC_RADIOCFG, offset, pVal, length);
	}
//    	printf("PB42 MAC address write\n");
#else 
        status=sysFlashConfigWrite(pCmd->devNum, FLC_RADIOCFG, offset, pVal, length);
#endif
#endif

    	return 0;

}


A_UINT32 processGetEndianModeCmd
(
	struct dkThreadInfo *dkThread
)
{
	PIPE_CMD      *pCmd;
	CMD_REPLY     *ReplyCmd_p;
	A_UCHAR      *replyBuf;

	pCmd = dkThread->PipeBuffer_p;
	ReplyCmd_p = dkThread->ReplyCmd_p;
	replyBuf = (A_UCHAR *)(ReplyCmd_p->cmdBytes);

	/* check that the size is appropriate before we access */
     	if( pCmd->cmdLen != sizeof(pCmd->CMD_U.GET_ENDIAN_MODE_CMD)+sizeof(pCmd->cmdID) + sizeof(pCmd->devNum)) 
        {
        	uiPrintf("Error, command Len is not equal to size of GET_ENDIAN_MODE_CMD\n");
        	return(COMMS_ERR_BAD_LENGTH);
        }
#ifdef OWL_PB42
    if(isHowlAP(pCmd->devNum) || isFlashCalData()){
       *((A_UINT32 *)replyBuf) = 3;// for Howl Client and target (Mp and Flash resp.) are BIG endian
    }else{
#endif 

    // Bit 0 is target endianess and bit 1 is client endianess. 
    // Target means eeprom/flash and client means processor.
#if defined(OWL_AP) && !defined(OWL_OB42)
#ifdef ARCH_BIG_ENDIAN
     *((A_UINT32 *)replyBuf) = 3;  // Bit 0 is target endianess and bit 1 is client endianess
#else 
      *((A_UINT32 *)replyBuf) = 0;  // Bit 0 is target endianess and bit 1 is client endianess (RSP-ENDIAN, essential for checksum)
#endif
#else
	#if defined(OWL_OB42)
      *((A_UINT32 *)replyBuf) = 2;   // PB42 or OB42 
	#else
      *((A_UINT32 *)replyBuf) = 0;   // Windows cleint with MB72/82 etc. 
	#endif	
#endif

#ifdef OWL_PB42
	}//isHowlAP
#endif

#ifdef ENDIAN_SWAP
    *((A_UINT32 *)replyBuf) = btol_l(*((A_UINT32 *)replyBuf));
#endif
	
	return 0;

}


/**************************************************************************
* processApRegReadCmd - Process AP_REG_READ_CMD command
*
* RETURNS: 0 processed OK, an Error code if it is not
*/
A_UINT32 processApRegReadCmd
(
	struct dkThreadInfo *dkThread
)
{
	PIPE_CMD      *pCmd;
	CMD_REPLY     *ReplyCmd_p;
	A_UCHAR     *replyBuf;

	pCmd = dkThread->PipeBuffer_p;
	ReplyCmd_p = dkThread->ReplyCmd_p;
	replyBuf = (A_UCHAR *)(ReplyCmd_p->cmdBytes);
    
	/* check that the size is appropriate before we access */
	if( pCmd->cmdLen != sizeof(pCmd->CMD_U.AP_REG_READ_CMD)+sizeof(pCmd->cmdID)+sizeof(pCmd->devNum) ) {
        	uiPrintf("Error, command Len is not equal to size of AP_REG_READ_CMD\n");
        	return(COMMS_ERR_BAD_LENGTH);
        }

#ifdef ENDIAN_SWAP
	pCmd->CMD_U.REG_READ_CMD.readAddr = ltob_l(pCmd->CMD_U.AP_REG_READ_CMD.readAddr);
#endif
    
	// read the register 
#if  defined(MDK_AP) || defined(THIN_CLIENT_BUILD)
#ifdef SOC_LINUX
	*((A_UINT32 *)replyBuf) = apRegRead32(dkThread->currentDevIndex, pCmd->CMD_U.AP_REG_READ_CMD.readAddr);
#else
	*((A_UINT32 *)replyBuf) = apRegRead32(pCmd->CMD_U.AP_REG_READ_CMD.readAddr);
#endif
#else
	*((A_UINT32 *)replyBuf) = 0;
#endif
   

#ifdef ENDIAN_SWAP
	*((A_UINT32 *)replyBuf) = btol_l(*((A_UINT32 *)replyBuf));
#endif
    
	return(0);
}

/**************************************************************************
* processApRegWriteCmd - Process AP_REG_WRITE_CMD command
*
* RETURNS: 0 processed OK, an Error code if it is not
*/
A_UINT32 processApRegWriteCmd
(
	struct dkThreadInfo *dkThread
)
{
	PIPE_CMD      *pCmd;
	CMD_REPLY     *ReplyCmd_p;
	A_UCHAR     *replyBuf;

	pCmd = dkThread->PipeBuffer_p;
	ReplyCmd_p = dkThread->ReplyCmd_p;
	replyBuf = (A_UCHAR *)(ReplyCmd_p->cmdBytes);
    
	/* check that the size is appropriate before we access */
	if( pCmd->cmdLen != sizeof(pCmd->CMD_U.AP_REG_WRITE_CMD)+sizeof(pCmd->cmdID) +sizeof(pCmd->devNum)) {
		uiPrintf("Error, command Len is not equal to size of AP_REG_WRITE_CMD\n");
		return(COMMS_ERR_BAD_LENGTH);
	}

#ifdef ENDIAN_SWAP
	pCmd->CMD_U.AP_REG_WRITE_CMD.writeAddr = ltob_l(pCmd->CMD_U.AP_REG_WRITE_CMD.writeAddr);
	pCmd->CMD_U.AP_REG_WRITE_CMD.regValue  = ltob_l(pCmd->CMD_U.AP_REG_WRITE_CMD.regValue);
#endif

	// write the register
#if  defined(MDK_AP) || defined(THIN_CLIENT_BUILD)
#ifdef SOC_LINUX
	apRegWrite32(dkThread->currentDevIndex, pCmd->CMD_U.AP_REG_WRITE_CMD.writeAddr, pCmd->CMD_U.AP_REG_WRITE_CMD.regValue);
#else
	apRegWrite32(pCmd->CMD_U.AP_REG_WRITE_CMD.writeAddr, pCmd->CMD_U.AP_REG_WRITE_CMD.regValue);
#endif
#endif
  
	return(0);
}

A_UINT32 processWriteProdDataCmd
(
	struct dkThreadInfo *dkThread
)
{
	PIPE_CMD      *pCmd;
	CMD_REPLY     *ReplyCmd_p;
	A_UCHAR     *replyBuf;
	A_UCHAR *wlan0Mac;
    A_UINT16 val;

	pCmd = dkThread->PipeBuffer_p;
	ReplyCmd_p = dkThread->ReplyCmd_p;
	replyBuf = (A_UCHAR *)(ReplyCmd_p->cmdBytes);

	/* check that the size is appropriate before we access */
     	if( pCmd->cmdLen != sizeof(pCmd->CMD_U.WRITE_PROD_DATA_CMD)+sizeof(pCmd->cmdID) +sizeof(pCmd->devNum)) 
        {
        	uiPrintf("Error, command Len is not equal to size of WRITE_PROD_DATA_CMD\n");
        	return(COMMS_ERR_BAD_LENGTH);
        }


#if defined(SPIRIT_AP) || defined(FREEDOM_AP) || defined(OWL_PB42) // || defined(PREDATOR)
	 m_writeProdData
	(
		pCmd->devNum,
		pCmd->CMD_U.WRITE_PROD_DATA_CMD.wlan0Mac,
		pCmd->CMD_U.WRITE_PROD_DATA_CMD.wlan1Mac,
		pCmd->CMD_U.WRITE_PROD_DATA_CMD.enet0Mac, 
		pCmd->CMD_U.WRITE_PROD_DATA_CMD.enet1Mac
	);
#endif

#if defined(PREDATOR) 
	wlan0Mac = pCmd->CMD_U.WRITE_PROD_DATA_CMD.wlan0Mac;
	val = (wlan0Mac[4] << 8) |wlan0Mac[5];
    sysFlashConfigWrite(0x1d<<1, (A_UINT8 *)&val, 2);
	val = (wlan0Mac[2] << 8) |wlan0Mac[3];
    sysFlashConfigWrite(0x1e<<1, (A_UINT8 *)&val, 2);
	val = (wlan0Mac[0] << 8) |wlan0Mac[1];
    sysFlashConfigWrite(0x1f<<1, (A_UINT8 *)&val, 2);
#endif

	return 0;
}

A_UINT32 processFillTxStatsCmd
(
	struct dkThreadInfo *dkThread
)
{

	PIPE_CMD      *pCmd;
	CMD_REPLY     *ReplyCmd_p;
	A_UCHAR     *replyBuf;
    TX_STATS_STRUCT txStats[STATS_BINS];

#ifndef __ATH_DJGPPDOS__
	pCmd = dkThread->PipeBuffer_p;
	ReplyCmd_p = dkThread->ReplyCmd_p;
	replyBuf = (A_UCHAR *)(ReplyCmd_p->cmdBytes);

	#ifdef ENDIAN_SWAP
     		pCmd->CMD_U.FILL_TX_STATS_CMD.descAddress = ltob_l(pCmd->CMD_U.FILL_TX_STATS_CMD.descAddress);
     		pCmd->CMD_U.FILL_TX_STATS_CMD.numDesc = ltob_l(pCmd->CMD_U.FILL_TX_STATS_CMD.numDesc);
     		pCmd->CMD_U.FILL_TX_STATS_CMD.dataBodyLen = ltob_l(pCmd->CMD_U.FILL_TX_STATS_CMD.dataBodyLen);
     		pCmd->CMD_U.FILL_TX_STATS_CMD.txTime = ltob_l(pCmd->CMD_U.FILL_TX_STATS_CMD.txTime);
 	#endif

#ifndef PHOENIX
    fillTxStats (dkThread->currentDevIndex, pCmd->CMD_U.FILL_TX_STATS_CMD.descAddress, \
			pCmd->CMD_U.FILL_TX_STATS_CMD.numDesc, pCmd->CMD_U.FILL_TX_STATS_CMD.dataBodyLen, \
			pCmd->CMD_U.FILL_TX_STATS_CMD.txTime, txStats);

#ifndef MALLOC_ABSENT
    memcpy(replyBuf, txStats, sizeof(TX_STATS_STRUCT)*STATS_BINS);
#else
    A_MEMCPY(replyBuf, txStats, sizeof(TX_STATS_STRUCT)*STATS_BINS);
#endif
#ifdef ENDIAN_SWAP
    swapBlock_l(replyBuf, sizeof(TX_STATS_STRUCT)*STATS_BINS);
#endif
#endif
#else //__ATH_DJGPPDOS__
	uiPrintf("create descriptors command not supported in DOS client\n");
#endif //__ATH_DJGPPDOS__

	return(0);

}

A_UINT32 processCreateDescCmd
(
	struct dkThreadInfo *dkThread
)
{

	PIPE_CMD      *pCmd;
	CMD_REPLY     *ReplyCmd_p;
	A_UCHAR       *replyBuf;
	A_UINT32      numDescWords;

#ifndef __ATH_DJGPPDOS__
	pCmd = dkThread->PipeBuffer_p;
	ReplyCmd_p = dkThread->ReplyCmd_p;
	replyBuf = (A_UCHAR *)(ReplyCmd_p->cmdBytes);

	#ifdef ENDIAN_SWAP
     		pCmd->CMD_U.CREATE_DESC_CMD.descBaseAddress = ltob_l(pCmd->CMD_U.CREATE_DESC_CMD.descBaseAddress);
     		pCmd->CMD_U.CREATE_DESC_CMD.descInfo = ltob_l(pCmd->CMD_U.CREATE_DESC_CMD.descInfo);
     		pCmd->CMD_U.CREATE_DESC_CMD.bufAddrIncrement = ltob_l(pCmd->CMD_U.CREATE_DESC_CMD.bufAddrIncrement);
     		pCmd->CMD_U.CREATE_DESC_CMD.descOp = ltob_l(pCmd->CMD_U.CREATE_DESC_CMD.descOp);
			numDescWords = (pCmd->CMD_U.CREATE_DESC_CMD.descInfo & DESC_INFO_NUM_DESC_WORDS_MASK) >> DESC_INFO_NUM_DESC_WORDS_BIT_START;
    		swapBlock_l(pCmd->CMD_U.CREATE_DESC_CMD.descWords, (numDescWords * sizeof(A_UINT32)));
 	#endif

#if !defined(PHOENIX) && !defined(SOC_AP)
uiPrintf("dba=%x:dI=%d:dO=%d:devIndex=%d\n",  pCmd->CMD_U.CREATE_DESC_CMD.descBaseAddress, \
			pCmd->CMD_U.CREATE_DESC_CMD.descInfo, pCmd->CMD_U.CREATE_DESC_CMD.descOp, dkThread->currentDevIndex);

    createDescriptors (dkThread->currentDevIndex, pCmd->CMD_U.CREATE_DESC_CMD.descBaseAddress, \
			pCmd->CMD_U.CREATE_DESC_CMD.descInfo, pCmd->CMD_U.CREATE_DESC_CMD.bufAddrIncrement, \
     		pCmd->CMD_U.CREATE_DESC_CMD.descOp, \
			pCmd->CMD_U.CREATE_DESC_CMD.descWords);

#endif
#else //__ATH_DJGPPDOS__
	uiPrintf("create descriptors command not supported in DOS client\n");
#endif //__ATH_DJGPPDOS__
	return(0);

}

A_UINT32 processLoadAndProgramCodeCmd
(
	struct dkThreadInfo *dkThread
)
{
	PIPE_CMD      *pCmd;
	CMD_REPLY     *ReplyCmd_p;
	A_UCHAR     *replyBuf;

	pCmd = dkThread->PipeBuffer_p;
	ReplyCmd_p = dkThread->ReplyCmd_p;
	replyBuf = (A_UCHAR *)(ReplyCmd_p->cmdBytes);

	/* check that the size is appropriate before we access */
     	if( pCmd->cmdLen != sizeof(pCmd->CMD_U.LOAD_AND_PROGRAM_CODE_CMD)+sizeof(pCmd->cmdID) +sizeof(pCmd->devNum)) 
        {
        	uiPrintf("Error, command Len is not equal to size of LOAD_AND_PROGRAM_CODE \n");
        	return(COMMS_ERR_BAD_LENGTH);
        }

	#ifdef ENDIAN_SWAP
     		pCmd->CMD_U.LOAD_AND_PROGRAM_CODE_CMD.loadFlag = ltob_l(pCmd->CMD_U.LOAD_AND_PROGRAM_CODE_CMD.loadFlag);
     		pCmd->CMD_U.LOAD_AND_PROGRAM_CODE_CMD.csAddr = ltob_l(pCmd->CMD_U.LOAD_AND_PROGRAM_CODE_CMD.csAddr);
     		pCmd->CMD_U.LOAD_AND_PROGRAM_CODE_CMD.pPhyAddr = ltob_l(pCmd->CMD_U.LOAD_AND_PROGRAM_CODE_CMD.pPhyAddr);
     		pCmd->CMD_U.LOAD_AND_PROGRAM_CODE_CMD.length = ltob_l(pCmd->CMD_U.LOAD_AND_PROGRAM_CODE_CMD.length);
 	#endif

#ifdef PREDATOR
	*(A_UINT32 *)replyBuf = m_loadAndProgramCode(pCmd->CMD_U.LOAD_AND_PROGRAM_CODE_CMD.loadFlag, 
								   	pCmd->CMD_U.LOAD_AND_PROGRAM_CODE_CMD.csAddr, 
     		       			       	pCmd->CMD_U.LOAD_AND_PROGRAM_CODE_CMD.pPhyAddr,
     		       			       	pCmd->CMD_U.LOAD_AND_PROGRAM_CODE_CMD.length,
		       			       		pCmd->CMD_U.LOAD_AND_PROGRAM_CODE_CMD.pBuffer, dkThread->currentDevIndex);
#endif

	#ifdef ENDIAN_SWAP
    		*((A_UINT32 *)replyBuf) = btol_l(*((A_UINT32 *)replyBuf));
	#endif
    	return 0;
}


#ifdef PHOENIX

A_UINT32 processPhsChannelToggleCmd
(
	struct dkThreadInfo *dkThread
)
{
	PIPE_CMD      *pCmd;
	CMD_REPLY     *ReplyCmd_p;
	A_UCHAR     *replyBuf;

	pCmd = dkThread->PipeBuffer_p;
	ReplyCmd_p = dkThread->ReplyCmd_p;
	replyBuf = (A_UCHAR *)(ReplyCmd_p->cmdBytes);
    
	/* check that the size is appropriate before we access */
	if( pCmd->cmdLen != sizeof(pCmd->CMD_U.PHS_CHANNEL_TOGGLE_CMD)+sizeof(pCmd->cmdID) +sizeof(pCmd->devNum)) {
		uiPrintf("Error, command Len is not equal to size of PHS_CHANNEL_TOGGLE_CMD\n");
		return(COMMS_ERR_BAD_LENGTH);
	}

#ifdef ENDIAN_SWAP
		pCmd->CMD_U.PHS_CHANNEL_TOGGLE_CMD.ch_int_cal = ltob_l(pCmd->CMD_U.PHS_CHANNEL_TOGGLE_CMD.ch_int_cal);
		pCmd->CMD_U.PHS_CHANNEL_TOGGLE_CMD.ch_frac_cal = ltob_l(pCmd->CMD_U.PHS_CHANNEL_TOGGLE_CMD.ch_frac_cal );
		pCmd->CMD_U.PHS_CHANNEL_TOGGLE_CMD.ch1_int = ltob_l(pCmd->CMD_U.PHS_CHANNEL_TOGGLE_CMD.ch1_int );
		pCmd->CMD_U.PHS_CHANNEL_TOGGLE_CMD.ch1_frac = ltob_l(pCmd->CMD_U.PHS_CHANNEL_TOGGLE_CMD.ch1_frac) ;
		pCmd->CMD_U.PHS_CHANNEL_TOGGLE_CMD.ch2_int = ltob_l(pCmd->CMD_U.PHS_CHANNEL_TOGGLE_CMD.ch2_int );
		pCmd->CMD_U.PHS_CHANNEL_TOGGLE_CMD.ch2_frac = ltob_l(pCmd->CMD_U.PHS_CHANNEL_TOGGLE_CMD.ch2_frac );
		pCmd->CMD_U.PHS_CHANNEL_TOGGLE_CMD.delay_us = ltob_l(pCmd->CMD_U.PHS_CHANNEL_TOGGLE_CMD.delay_us );
#endif

		pPCT.devIndex = dkThread->currentDevIndex;
		pPCT.ch_int_cal = pCmd->CMD_U.PHS_CHANNEL_TOGGLE_CMD.ch_int_cal;
		pPCT.ch_frac_cal = pCmd->CMD_U.PHS_CHANNEL_TOGGLE_CMD.ch_frac_cal;
		pPCT.ch1_int = pCmd->CMD_U.PHS_CHANNEL_TOGGLE_CMD.ch1_int;
		pPCT.ch1_frac = pCmd->CMD_U.PHS_CHANNEL_TOGGLE_CMD.ch1_frac;
		pPCT.ch2_int = pCmd->CMD_U.PHS_CHANNEL_TOGGLE_CMD.ch2_int;
		pPCT.ch2_frac = pCmd->CMD_U.PHS_CHANNEL_TOGGLE_CMD.ch2_frac;
		pPCT.delay_us = pCmd->CMD_U.PHS_CHANNEL_TOGGLE_CMD.delay_us;

		phs_chan_toggle(&pPCT);

	return(0);
}

A_UINT32 processStopPhsChannelToggleCmd
(
	struct dkThreadInfo *dkThread
)
{
	PIPE_CMD      *pCmd;
	CMD_REPLY     *ReplyCmd_p;
	A_UCHAR     *replyBuf;

	pCmd = dkThread->PipeBuffer_p;
	ReplyCmd_p = dkThread->ReplyCmd_p;
	replyBuf = (A_UCHAR *)(ReplyCmd_p->cmdBytes);
    
	/* check that the size is appropriate before we access */
	if( pCmd->cmdLen != sizeof(pCmd->cmdID) +sizeof(pCmd->devNum)) {
		uiPrintf("Error, command Len is not equal to size of STOP_PHS_CHANNEL_TOGGLE_CMD\n");
		return(COMMS_ERR_BAD_LENGTH);
	}

	stop_phs_chan_toggle(dkThread->currentDevIndex);

	return(0);
}

A_UINT32 processGenericFnCallCmd
(
	struct dkThreadInfo *dkThread
)
{
	PIPE_CMD      *pCmd;
	CMD_REPLY     *ReplyCmd_p;
	A_UCHAR     *replyBuf;
    A_INT32	 cmp_status=0;

	pCmd = dkThread->PipeBuffer_p;
	ReplyCmd_p = dkThread->ReplyCmd_p;
	replyBuf = (A_UCHAR *)(ReplyCmd_p->cmdBytes);
    
	/* check that the size is appropriate before we access */
	if( pCmd->cmdLen != (sizeof(pCmd->CMD_U.PHS_CHANNEL_TOGGLE_CMD)+sizeof(pCmd->cmdID) +sizeof(pCmd->devNum))) {
		uiPrintf("Error, command Len is not equal to size of STOP_PHS_CHANNEL_TOGGLE_CMD\n");
		return(COMMS_ERR_BAD_LENGTH);
	}

#ifdef ENDIAN_SWAP
		pCmd->CMD_U.GENERIC_FN_CALL_CMD.stGFnCallCmd.fnId = ltob_l(pCmd->CMD_U.GENERIC_FN_CALL_CMD.stGFnCallCmd.fnId);
		pCmd->CMD_U.GENERIC_FN_CALL_CMD.stGFnCallCmd.arg1 = ltob_l(pCmd->CMD_U.GENERIC_FN_CALL_CMD.stGFnCallCmd.arg1);
		pCmd->CMD_U.GENERIC_FN_CALL_CMD.stGFnCallCmd.arg2 = ltob_l(pCmd->CMD_U.GENERIC_FN_CALL_CMD.stGFnCallCmd.arg2);
		pCmd->CMD_U.GENERIC_FN_CALL_CMD.stGFnCallCmd.arg3 = ltob_l(pCmd->CMD_U.GENERIC_FN_CALL_CMD.stGFnCallCmd.arg3);
		pCmd->CMD_U.GENERIC_FN_CALL_CMD.stGFnCallCmd.arg4 = ltob_l(pCmd->CMD_U.GENERIC_FN_CALL_CMD.stGFnCallCmd.arg4);
		pCmd->CMD_U.GENERIC_FN_CALL_CMD.stGFnCallCmd.arg5 = ltob_l(pCmd->CMD_U.GENERIC_FN_CALL_CMD.stGFnCallCmd.arg5);
		pCmd->CMD_U.GENERIC_FN_CALL_CMD.stGFnCallCmd.arg6 = ltob_l(pCmd->CMD_U.GENERIC_FN_CALL_CMD.stGFnCallCmd.arg6);
		pCmd->CMD_U.GENERIC_FN_CALL_CMD.stGFnCallCmd.arg7 = ltob_l(pCmd->CMD_U.GENERIC_FN_CALL_CMD.stGFnCallCmd.arg7);
		pCmd->CMD_U.GENERIC_FN_CALL_CMD.stGFnCallCmd.arg8 = ltob_l(pCmd->CMD_U.GENERIC_FN_CALL_CMD.stGFnCallCmd.arg8);
#endif

	switch(pCmd->CMD_U.GENERIC_FN_CALL_CMD.stGFnCallCmd.fnId) {
		case RADIO_INIT_FN_ID:
			radio_init();
			break;
		case SETUP_MODEM_FN_ID:
			setup_modem();
			break;
		case SETUP_TDMA_FN_ID:
			setup_tdma();
			break;
		case SETUP_AGC_FN_ID:
			setup_agc();
			break;
		case SETUP_LS_MATRIX_FN_ID:
			setup_ls_matrix();
			break;
		case POLL_DMA_COMPARE_FN_ID:
			cmp_status = poll_dma_compare(dkThread->currentDevIndex, \
				pCmd->CMD_U.GENERIC_FN_CALL_CMD.stGFnCallCmd.arg1, \
				pCmd->CMD_U.GENERIC_FN_CALL_CMD.stGFnCallCmd.arg2, \
				pCmd->CMD_U.GENERIC_FN_CALL_CMD.stGFnCallCmd.arg3,  \
				pCmd->CMD_U.GENERIC_FN_CALL_CMD.stGFnCallCmd.arg4\
				);
			break;
	}


*((A_UINT32 *)replyBuf) = cmp_status;

#ifdef ENDIAN_SWAP
        *((A_UINT32 *)replyBuf) = btol_l(*((A_UINT32 *)replyBuf));
#endif

return(0);
}


#endif

A_UINT32 processSendFrameCmd
(
	struct dkThreadInfo *dkThread
)
{
	PIPE_CMD      *pCmd;
	CMD_REPLY     *ReplyCmd_p;
	A_UCHAR     *replyBuf;

	pCmd = dkThread->PipeBuffer_p;
	ReplyCmd_p = dkThread->ReplyCmd_p;
	replyBuf = (A_UCHAR *)(ReplyCmd_p->cmdBytes);
    
	/* check that the size is appropriate before we access */
	if( pCmd->cmdLen != sizeof(pCmd->CMD_U.SEND_FRAME_CMD)+sizeof(pCmd->cmdID) +sizeof(pCmd->devNum)) {
		uiPrintf("Error, command Len is not equal to size of SEND_FRAME_CMD\n");
		return(COMMS_ERR_BAD_LENGTH);
	}

#ifdef ENDIAN_SWAP
		pCmd->CMD_U.SEND_FRAME_CMD.tx_desc_ptr = ltob_l(pCmd->CMD_U.SEND_FRAME_CMD.tx_desc_ptr);
		pCmd->CMD_U.SEND_FRAME_CMD.tx_buf_ptr = ltob_l(pCmd->CMD_U.SEND_FRAME_CMD.tx_buf_ptr);
		pCmd->CMD_U.SEND_FRAME_CMD.rx_desc_ptr = ltob_l(pCmd->CMD_U.SEND_FRAME_CMD.rx_desc_ptr);
		pCmd->CMD_U.SEND_FRAME_CMD.rx_buf_ptr = ltob_l(pCmd->CMD_U.SEND_FRAME_CMD.rx_buf_ptr);
		pCmd->CMD_U.SEND_FRAME_CMD.rate_code = ltob_l(pCmd->CMD_U.SEND_FRAME_CMD.rate_code);
		{
    		int noWords;
    		int i;
    		A_UINT32 *wordPtr;

    		noWords =   MAX_LB_FRAME_LEN/sizeof(A_UINT32);
    		wordPtr = (A_UINT32 *)pCmd->CMD_U.SEND_FRAME_CMD.pBuffer;
    		for (i=0;i<noWords;i++)
    		{
    			*(wordPtr + i) = ltob_l(*(wordPtr + i));
    		}
		}
#endif

#if !defined(PHOENIX) && !defined(PREDATOR)
#if defined(THIN_CLIENT_BUILD)
#ifndef AR6000
	*(A_UINT32 *)replyBuf = m_send_frame_and_recv(dkThread->currentDevIndex, pCmd->CMD_U.SEND_FRAME_CMD.pBuffer, \
								pCmd->CMD_U.SEND_FRAME_CMD.tx_desc_ptr, \
								pCmd->CMD_U.SEND_FRAME_CMD.tx_buf_ptr, \
								pCmd->CMD_U.SEND_FRAME_CMD.rx_desc_ptr, \
								pCmd->CMD_U.SEND_FRAME_CMD.rx_buf_ptr,
								pCmd->CMD_U.SEND_FRAME_CMD.rate_code
											);
#endif
#endif
#endif

#ifdef ENDIAN_SWAP
        *((A_UINT32 *)replyBuf) = btol_l(*((A_UINT32 *)replyBuf));
#endif
	return(0);
}

A_UINT32 processRecvFrameCmd
(
	struct dkThreadInfo *dkThread
)
{
	PIPE_CMD      *pCmd;
	CMD_REPLY     *ReplyCmd_p;
	A_UCHAR     *replyBuf;

	pCmd = dkThread->PipeBuffer_p;
	ReplyCmd_p = dkThread->ReplyCmd_p;
	replyBuf = (A_UCHAR *)(ReplyCmd_p->cmdBytes);
    
	/* check that the size is appropriate before we access */
	if( pCmd->cmdLen != sizeof(pCmd->CMD_U.RECV_FRAME_CMD)+sizeof(pCmd->cmdID) +sizeof(pCmd->devNum)) {
		uiPrintf("Error, command Len is not equal to size of RECV_FRAME_CMD\n");
		return(COMMS_ERR_BAD_LENGTH);
	}

#ifdef ENDIAN_SWAP
		pCmd->CMD_U.RECV_FRAME_CMD.tx_desc_ptr = ltob_l(pCmd->CMD_U.RECV_FRAME_CMD.tx_desc_ptr);
		pCmd->CMD_U.RECV_FRAME_CMD.tx_buf_ptr = ltob_l(pCmd->CMD_U.RECV_FRAME_CMD.tx_buf_ptr);
		pCmd->CMD_U.RECV_FRAME_CMD.rx_desc_ptr = ltob_l(pCmd->CMD_U.RECV_FRAME_CMD.rx_desc_ptr);
		pCmd->CMD_U.RECV_FRAME_CMD.rx_buf_ptr = ltob_l(pCmd->CMD_U.RECV_FRAME_CMD.rx_buf_ptr);
		pCmd->CMD_U.RECV_FRAME_CMD.rate_code = ltob_l(pCmd->CMD_U.RECV_FRAME_CMD.rate_code);
#endif

#if !defined(PHOENIX) && !defined(PREDATOR)
#if defined(THIN_CLIENT_BUILD)
#ifndef AR6000
	*(A_UINT32 *)replyBuf = m_recv_frame_and_xmit(dkThread->currentDevIndex, 
								pCmd->CMD_U.RECV_FRAME_CMD.tx_desc_ptr, \
								pCmd->CMD_U.RECV_FRAME_CMD.tx_buf_ptr, \
								pCmd->CMD_U.RECV_FRAME_CMD.rx_desc_ptr, \
								pCmd->CMD_U.RECV_FRAME_CMD.rx_buf_ptr,
								pCmd->CMD_U.RECV_FRAME_CMD.rate_code
											);
#endif
#endif
#endif

#ifdef ENDIAN_SWAP
        *((A_UINT32 *)replyBuf) = btol_l(*((A_UINT32 *)replyBuf));
#endif
	return(0);
}

#if defined(THIN_CLIENT_BUILD)
// These following commands will not be part of thin client build
#else

A_UINT32 processLoadAndRunCodeCmd
(
	struct dkThreadInfo *dkThread
)
{
	PIPE_CMD      *pCmd;
	CMD_REPLY     *ReplyCmd_p;
	A_UCHAR     *replyBuf;

#ifndef __ATH_DJGPPDOS__
	pCmd = dkThread->PipeBuffer_p;
	ReplyCmd_p = dkThread->ReplyCmd_p;
	replyBuf = (A_UCHAR *)(ReplyCmd_p->cmdBytes);

	/* check that the size is appropriate before we access */
     	if( pCmd->cmdLen != sizeof(pCmd->CMD_U.LOAD_AND_RUN_CODE_CMD)+sizeof(pCmd->cmdID) +sizeof(pCmd->devNum)) 
        {
        	uiPrintf("Error, command Len is not equal to size of LOAD_AND_RUN_CODE \n");
        	return(COMMS_ERR_BAD_LENGTH);
        }

	#ifdef ENDIAN_SWAP
     		pCmd->CMD_U.LOAD_AND_RUN_CODE_CMD.loadFlag = ltob_l(pCmd->CMD_U.LOAD_AND_RUN_CODE_CMD.loadFlag);
     		pCmd->CMD_U.LOAD_AND_RUN_CODE_CMD.pPhyAddr = ltob_l(pCmd->CMD_U.LOAD_AND_RUN_CODE_CMD.pPhyAddr);
     		pCmd->CMD_U.LOAD_AND_RUN_CODE_CMD.length = ltob_l(pCmd->CMD_U.LOAD_AND_RUN_CODE_CMD.length);
 	#endif

#ifndef PHOENIX
	*(A_UINT32 *)replyBuf = m_loadAndRunCode(pCmd->CMD_U.LOAD_AND_RUN_CODE_CMD.loadFlag,
     		       			       pCmd->CMD_U.LOAD_AND_RUN_CODE_CMD.pPhyAddr,
     		       			       pCmd->CMD_U.LOAD_AND_RUN_CODE_CMD.length,
		       			       pCmd->CMD_U.LOAD_AND_RUN_CODE_CMD.pBuffer, dkThread->currentDevIndex);
#endif

	#ifdef ENDIAN_SWAP
    		*((A_UINT32 *)replyBuf) = btol_l(*((A_UINT32 *)replyBuf));
	#endif
#else //__ATH_DJGPPDOS__
	uiPrintf("Load and run command not supported in DOS client\n");
#endif //__ATH_DJGPPDOS__

    	return 0;
}

/**************************************************************************
* processTramReadBlockCmd - Process TRAM_READ_BLOCK_CMD command
*
* RETURNS: 0 processed OK, an Error code if it is not
*/
A_UINT32 processTramReadBlockCmd
(
	struct dkThreadInfo *dkThread
)
{
	PIPE_CMD      *pCmd;
	CMD_REPLY     *ReplyCmd_p;
	A_UCHAR     *replyBuf;

	pCmd = dkThread->PipeBuffer_p;
	ReplyCmd_p = dkThread->ReplyCmd_p;
	replyBuf = (A_UCHAR *)(ReplyCmd_p->cmdBytes);


#ifndef __ATH_DJGPPDOS__
    /* check that the size is appropriate before we access */
    if( pCmd->cmdLen != sizeof(pCmd->CMD_U.MEM_READ_BLOCK_CMD)+sizeof(pCmd->cmdID) +sizeof(pCmd->devNum) )
        {
        uiPrintf("Error, command Len is not equal to size of MEM_READ_BLOCK_CMD\n");
        return(COMMS_ERR_BAD_LENGTH);
        }

#ifdef ENDIAN_SWAP
	pCmd->CMD_U.MEM_READ_BLOCK_CMD.physAddr = ltob_l(pCmd->CMD_U.MEM_READ_BLOCK_CMD.physAddr);		
	pCmd->CMD_U.MEM_READ_BLOCK_CMD.length = ltob_l(pCmd->CMD_U.MEM_READ_BLOCK_CMD.length);
#endif

    
    /* read the memory */
    if(hwTramReadBlock((A_UINT16)dkThread->currentDevIndex,replyBuf, pCmd->CMD_U.MEM_READ_BLOCK_CMD.physAddr, 
				  pCmd->CMD_U.MEM_READ_BLOCK_CMD.length) == -1)
        {
        uiPrintf("Failed call to hwTramReadBlock():devIndex=%d\n", dkThread->currentDevIndex );
        return(COMMS_ERR_READ_BLOCK_FAIL);
        }

// Swap from big endian to little endian.
#ifdef ENDIAN_SWAP
{
    int noWords;
    int i;
    A_UINT32 *wordPtr;

    noWords =   pCmd->CMD_U.MEM_READ_BLOCK_CMD.length/sizeof(A_UINT32);
    wordPtr = (A_UINT32 *)replyBuf;
    for (i=0;i<noWords;i++)
    {
    	*(wordPtr + i) = btol_l(*(wordPtr + i));
    }
}
#endif
#else 
	uiPrintf("Command not supported in DOS client\n");
#endif //__ATH_DJGPPDOS__


    return(0);
    }

/**************************************************************************
* processTramWriteBlockCmd - Process TRAM_WRITE_BLOCK_CMD command
*
* RETURNS: 0 processed OK, an Error code if it is not
*/
A_UINT32 processTramWriteBlockCmd
(
	struct dkThreadInfo *dkThread
)
{
	PIPE_CMD      *pCmd;
	CMD_REPLY     *ReplyCmd_p;
	A_UCHAR     *replyBuf;

	pCmd = dkThread->PipeBuffer_p;
	ReplyCmd_p = dkThread->ReplyCmd_p;
	replyBuf = (A_UCHAR *)(ReplyCmd_p->cmdBytes);

#ifndef __ATH_DJGPPDOS__

#ifdef ENDIAN_SWAP
     pCmd->CMD_U.MEM_WRITE_BLOCK_CMD.physAddr = ltob_l(pCmd->CMD_U.MEM_WRITE_BLOCK_CMD.physAddr);
     pCmd->CMD_U.MEM_WRITE_BLOCK_CMD.length = ltob_l(pCmd->CMD_U.MEM_WRITE_BLOCK_CMD.length);
#endif


	/* check the size , but need to access the structure to do it */
    if( pCmd->cmdLen != sizeof(pCmd->cmdID) + sizeof(pCmd->CMD_U.MEM_WRITE_BLOCK_CMD.physAddr) +
		sizeof(pCmd->CMD_U.MEM_WRITE_BLOCK_CMD.length) + pCmd->CMD_U.MEM_WRITE_BLOCK_CMD.length +sizeof(pCmd->devNum)) 
        {
        uiPrintf("Error: command Len is not equal to size of expected bytes for MEM_WRITE_BLOCK_CMD\n");
		uiPrintf("     : got %d, expected %d\n", pCmd->cmdLen, sizeof(pCmd->cmdID) +
			sizeof(pCmd->CMD_U.MEM_WRITE_BLOCK_CMD.physAddr) +
			sizeof(pCmd->CMD_U.MEM_WRITE_BLOCK_CMD.length) + pCmd->CMD_U.MEM_WRITE_BLOCK_CMD.length);
        return(COMMS_ERR_BAD_LENGTH);
        }

// swap from little endian to big endian
#ifdef ENDIAN_SWAP
{
    int noWords;
    int i;
    A_UINT32 *wordPtr;

    noWords =   pCmd->CMD_U.MEM_WRITE_BLOCK_CMD.length/sizeof(A_UINT32);
    wordPtr = (A_UINT32 *)pCmd->CMD_U.MEM_WRITE_BLOCK_CMD.bytes;
    for (i=0;i<noWords;i++)
    {
    	*(wordPtr + i) = btol_l(*(wordPtr + i));
    }
}
#endif
 
    /* write the memory */
    if(hwTramWriteBlock((A_UINT16)dkThread->currentDevIndex,pCmd->CMD_U.MEM_WRITE_BLOCK_CMD.bytes, 
				   pCmd->CMD_U.MEM_WRITE_BLOCK_CMD.length, 
				   pCmd->CMD_U.MEM_WRITE_BLOCK_CMD.physAddr) == -1)
        {
        uiPrintf("Failed call to hwTramWriteBlock()\n");
        return(COMMS_ERR_WRITE_BLOCK_FAIL);
        }

#else
	uiPrintf("Command not supported in DOS client\n");
#endif //__ATH_DJGPPDOS__
    return(0);
}

/**************************************************************************
* processremapHwCmd - Process REMAP_HW_CMD command
*
* RETURNS: 0 processed OK, an Error code if it is not
*/
A_UINT32 processRemapHwCmd
(
	struct dkThreadInfo *dkThread
)
{
	PIPE_CMD      *pCmd;
	CMD_REPLY     *ReplyCmd_p;
	A_UCHAR     *replyBuf;

#ifndef __ATH_DJGPPDOS
	pCmd = dkThread->PipeBuffer_p;
	ReplyCmd_p = dkThread->ReplyCmd_p;
	replyBuf = (A_UCHAR *)(ReplyCmd_p->cmdBytes);

    /* check that the size is appropriate before we access */
    if( pCmd->cmdLen != sizeof(pCmd->CMD_U.REMAP_HW_CMD)+sizeof(pCmd->cmdID) +sizeof(pCmd->devNum)) 
        {
        uiPrintf("Error, command Len is not equal to size of REMAP_HW_CMD\n");
        return(COMMS_ERR_BAD_LENGTH);
        }

#ifdef ENDIAN_SWAP
	pCmd->CMD_U.REMAP_HW_CMD.mapAddress = ltob_l(pCmd->CMD_U.REMAP_HW_CMD.mapAddress);
#endif

    if(hwRemapHardware((A_UINT16)dkThread->currentDevIndex,pCmd->CMD_U.REMAP_HW_CMD.mapAddress) == -1)
        {
        uiPrintf("Error:  unable to remap hardware\n");
        return(COMMS_ERR_REMAP_FAIL);
        }

#else //__ATH_DJGPPDOS__
	uiPrintf("create descriptors command not supported in DOS client\n");
#endif //__ATH_DJGPPDOS__
    return(0);
    }

/**************************************************************************
* processSelectHwCmd - Process SELECT_HW_CMD command
*
* RETURNS: 0 processed OK, an Error code if it is not
*/
A_UINT32 processSelectHwCmd
(
	struct dkThreadInfo *dkThread
)
{
	PIPE_CMD      *pCmd;
	CMD_REPLY     *ReplyCmd_p;
	A_UCHAR     *replyBuf;

	pCmd = dkThread->PipeBuffer_p;
	ReplyCmd_p = dkThread->ReplyCmd_p;
	replyBuf = (A_UCHAR *)(ReplyCmd_p->cmdBytes);

    /* check that the size is appropriate before we access */
    if( pCmd->cmdLen != sizeof(pCmd->CMD_U.SELECT_HW_CMD)+sizeof(pCmd->cmdID) +sizeof(pCmd->devNum))
        {
        uiPrintf("Error, command Len is not equal to size of SELECT_HW_CMD\n");
        return(COMMS_ERR_BAD_LENGTH);
        }

    
#ifdef ENDIAN_SWAP    
    pCmd->CMD_U.SELECT_HW_CMD.whichF2 = ltob_l(pCmd->CMD_U.SELECT_HW_CMD.whichF2);
#endif

    /* do a sanity check to see that we have a devInfo structure for this F2 */
    if(globDrvInfo.pDevInfoArray[pCmd->CMD_U.SELECT_HW_CMD.whichF2 - 1] == NULL)
        {
        /* for some reason don't seem to have a structure for this, return error */
        return(COMMS_ERR_NO_F2_TO_SELECT);
        }

    /* make this new F2 the current F2 */
    dkThread->currentDevIndex = (A_UINT16)pCmd->CMD_U.SELECT_HW_CMD.whichF2 - 1;

    return(0);
    }
/**************************************************************************
* processMemReadCmd - Process MEM_READ_CMD command
*
* RETURNS: 0 processed OK, an Error code if it is not
*/
A_UINT32 processMemReadCmd
(
	struct dkThreadInfo *dkThread
)
{
	PIPE_CMD      *pCmd;
	CMD_REPLY     *ReplyCmd_p;
	A_UCHAR     *replyBuf;

	pCmd = dkThread->PipeBuffer_p;
	ReplyCmd_p = dkThread->ReplyCmd_p;
	replyBuf = (A_UCHAR *)(ReplyCmd_p->cmdBytes);


    /* check that the size is appropriate before we access */
	 if( pCmd->cmdLen != sizeof(pCmd->CMD_U.MEM_READ_CMD)+sizeof(pCmd->cmdID) + sizeof(pCmd->devNum))
        {
        uiPrintf("Error, command Len is not equal to size of MEM_READ_CMD\n");
        return(COMMS_ERR_BAD_LENGTH);
        }
#ifdef ENDIAN_SWAP    
	pCmd->CMD_U.MEM_READ_CMD.readSize = ltob_l(pCmd->CMD_U.MEM_READ_CMD.readSize);
	pCmd->CMD_U.MEM_READ_CMD.readAddr = ltob_l(pCmd->CMD_U.MEM_READ_CMD.readAddr);
#endif

    /* read the memory */
    switch(pCmd->CMD_U.MEM_READ_CMD.readSize)
        {
#if !defined(ART) && !defined(THIN_CLIENT_BUILD)
        case 8:
            *((A_UINT32 *)replyBuf) = (A_UINT32)hwMemRead8((A_UINT16)dkThread->currentDevIndex,pCmd->CMD_U.MEM_READ_CMD.readAddr); 
            break;

        case 16:
            *((A_UINT32 *)replyBuf) = (A_UINT32)hwMemRead16((A_UINT16)dkThread->currentDevIndex,pCmd->CMD_U.MEM_READ_CMD.readAddr); 
            break;
#endif
        case 32:
            *((A_UINT32 *)replyBuf) = hwMemRead32((A_UINT16)dkThread->currentDevIndex,pCmd->CMD_U.MEM_READ_CMD.readAddr); 
            break;
    
        default:
            return(COMMS_ERR_BAD_MEM_SIZE);

        }

    return(0);
    }

/**************************************************************************
* processMemWriteCmd - Process MEM_WRITE_CMD command
*
* RETURNS: 0 processed OK, an Error code if it is not
*/
A_UINT32 processMemWriteCmd
(
	struct dkThreadInfo *dkThread
)
{
	PIPE_CMD      *pCmd;
	CMD_REPLY     *ReplyCmd_p;
	A_UCHAR     *replyBuf;

	pCmd = dkThread->PipeBuffer_p;
	ReplyCmd_p = dkThread->ReplyCmd_p;
	replyBuf = (A_UCHAR *)(ReplyCmd_p->cmdBytes);

    /* check that the size is appropriate before we access */	 
    if( pCmd->cmdLen != sizeof(pCmd->CMD_U.MEM_WRITE_CMD)+sizeof(pCmd->cmdID) + sizeof(pCmd->devNum))
    {
        uiPrintf("Error, command Len is not equal to size of MEM_WRITE_CMD\n");
        return(COMMS_ERR_BAD_LENGTH);
    }
	
#ifdef ENDIAN_SWAP
	pCmd->CMD_U.MEM_WRITE_CMD.writeSize = ltob_l(pCmd->CMD_U.MEM_WRITE_CMD.writeSize);
	pCmd->CMD_U.MEM_WRITE_CMD.writeAddr = ltob_l(pCmd->CMD_U.MEM_WRITE_CMD.writeAddr);
#endif
  
    /* write the register */
    switch(pCmd->CMD_U.MEM_WRITE_CMD.writeSize)
        {
#if !defined(ART) && !defined(THIN_CLIENT_BUILD)
        case 8:
            hwMemWrite8((A_UINT16)dkThread->currentDevIndex,pCmd->CMD_U.MEM_WRITE_CMD.writeAddr, (A_UINT8)pCmd->CMD_U.MEM_WRITE_CMD.writeValue); 
            break;

        case 16:
            hwMemWrite16((A_UINT16)dkThread->currentDevIndex,pCmd->CMD_U.MEM_WRITE_CMD.writeAddr, (A_UINT16)pCmd->CMD_U.MEM_WRITE_CMD.writeValue); 
            break;
#endif
        case 32:
            hwMemWrite32((A_UINT16)dkThread->currentDevIndex,pCmd->CMD_U.MEM_WRITE_CMD.writeAddr, pCmd->CMD_U.MEM_WRITE_CMD.writeValue); 
            break;

        default:
            return(COMMS_ERR_BAD_MEM_SIZE);

        }
    
    return(0);
    }
/**************************************************************************
* processMemAllocCmd - Process NEN_ALLOC_CMD command
*
* RETURNS: 0 processed OK, an Error code if it is not
*/
A_UINT32 processMemAllocCmd
(
	struct dkThreadInfo *dkThread
)
{
	PIPE_CMD      *pCmd;
	CMD_REPLY     *ReplyCmd_p;
	A_UCHAR     *replyBuf;
	A_UINT32       *pVirtAddress;        /* virtual address of mem block */

	pCmd = dkThread->PipeBuffer_p;
	ReplyCmd_p = dkThread->ReplyCmd_p;
	replyBuf = (A_UCHAR *)(ReplyCmd_p->cmdBytes);

    /* check that the size is appropriate before we access */
    if( pCmd->cmdLen != sizeof(pCmd->CMD_U.MEM_ALLOC_CMD)+sizeof(pCmd->cmdID) +sizeof(pCmd->devNum))
        {
        uiPrintf("Error, command Len is not equal to size of MEM_ALLOC_CMD\n");
        return(COMMS_ERR_BAD_LENGTH);
        }

#ifdef ENDIAN_SWAP
	pCmd->CMD_U.MEM_ALLOC_CMD.allocSize = ltob_l(pCmd->CMD_U.MEM_ALLOC_CMD.allocSize);
	pCmd->CMD_U.MEM_ALLOC_CMD.physAddr = ltob_l(pCmd->CMD_U.MEM_ALLOC_CMD.physAddr);
#endif

    /* do the allocation */
    pVirtAddress = (A_UINT32 *)hwGetPhysMem((A_UINT16)dkThread->currentDevIndex, pCmd->CMD_U.MEM_ALLOC_CMD.allocSize, 
								    &(pCmd->CMD_U.MEM_ALLOC_CMD.physAddr));

    if (pVirtAddress == NULL)
        {
        return(COMMS_ERR_ALLOC_FAIL);
        } 
    
    /* copy the address into the return buffer */
    *(A_UINT32 *)replyBuf = pCmd->CMD_U.MEM_ALLOC_CMD.physAddr;

#ifdef ENDIAN_SWAP
    *((A_UINT32 *)replyBuf) = btol_l(*((A_UINT32 *)replyBuf));
#endif

    return(0);
    }

/**************************************************************************
* processMemFreeCmd - Process MEM_FREE_CMD command
*
* 
*
* RETURNS: 0 processed OK, an Error code if it is not
*/
A_UINT32 processMemFreeCmd
(
	struct dkThreadInfo *dkThread
)
{
	PIPE_CMD      *pCmd;
	CMD_REPLY     *ReplyCmd_p;
	A_UCHAR     *replyBuf;

	pCmd = dkThread->PipeBuffer_p;
	ReplyCmd_p = dkThread->ReplyCmd_p;
	replyBuf = (A_UCHAR *)(ReplyCmd_p->cmdBytes);

    /* check that the size is appropriate before we access */
    if( pCmd->cmdLen != sizeof(pCmd->cmdID) +sizeof(pCmd->devNum))
        {
        uiPrintf("Error, command Len is not equal to size of MEM_FREE_CMD\n");
        return(COMMS_ERR_BAD_LENGTH);
        }

    /* do the free */
    hwFreeAll((A_UINT16)dkThread->currentDevIndex);
    
    return(0);
    }

/**************************************************************************
* resetDevice - reset the device and initialize all registers
*
*/

A_UINT32 processResetDeviceCmd
(
	struct dkThreadInfo *dkThread
)
{
	PIPE_CMD      *pCmd;
	CMD_REPLY     *ReplyCmd_p;
	A_UCHAR     *replyBuf;
	PDATABUFFER mac;
	PDATABUFFER bss;

	pCmd = dkThread->PipeBuffer_p;
	ReplyCmd_p = dkThread->ReplyCmd_p;
	replyBuf = (A_UCHAR *)(ReplyCmd_p->cmdBytes);

	/* check that the size is appropriate before we access */
     	if( pCmd->cmdLen != sizeof(pCmd->CMD_U.RESET_DEVICE_CMD)+sizeof(pCmd->cmdID) + sizeof(pCmd->devNum) ) 
        {
        	uiPrintf("Error, command Len is not equal to size of RESET_DEVICE_CMD\n");
        	return(COMMS_ERR_BAD_LENGTH);
        }

	#ifdef ENDIAN_SWAP
     		pCmd->CMD_U.RESET_DEVICE_CMD.freq = ltob_l(pCmd->CMD_U.RESET_DEVICE_CMD.freq);
     		pCmd->CMD_U.RESET_DEVICE_CMD.turbo  = ltob_l(pCmd->CMD_U.RESET_DEVICE_CMD.turbo);
	#endif

	mac = (PDATABUFFER)A_MALLOC(sizeof(DATABUFFER));
	if (mac == NULL)
	{
		uiPrintf("Error, unable to allocate memory for mac PDATABUFFER struct \n");
		return (COMMS_ERR_ALLOC_FAIL);	
	}
	mac->Length = 6;
	mac->pData = (A_UCHAR *)A_MALLOC(mac->Length);
	if (mac->pData == NULL)
	{
		A_FREE(mac);
		uiPrintf("Error, unable to allocate memory for mac array \n");
		return (COMMS_ERR_ALLOC_FAIL);	
	}
	memcpy(mac->pData,pCmd->CMD_U.RESET_DEVICE_CMD.mac,mac->Length);

	bss = (PDATABUFFER)A_MALLOC(sizeof(DATABUFFER));
	if (bss == NULL)
	{
		A_FREE(mac->pData);
		A_FREE(mac);
		uiPrintf("Error, unable to allocate memory for bss PDATABUFFER struct \n");
		return (COMMS_ERR_ALLOC_FAIL);	
	}
	bss->Length = 6;
	bss->pData = (A_UCHAR *)A_MALLOC(bss->Length);
	if (bss->pData == NULL)
	{
		A_FREE(mac->pData);
		A_FREE(mac);
		A_FREE(bss);
		uiPrintf("Error, unable to allocate memory for bss array \n");
		return (COMMS_ERR_ALLOC_FAIL);	
	}
	memcpy(bss->pData,pCmd->CMD_U.RESET_DEVICE_CMD.bss,bss->Length);

	    /* read the eeprom */
    	m_resetDevice(pCmd->devNum,
			mac,	
			bss,
			pCmd->CMD_U.RESET_DEVICE_CMD.freq,
			pCmd->CMD_U.RESET_DEVICE_CMD.turbo);

	A_FREE(mac->pData);
	A_FREE(mac);
	A_FREE(bss->pData);
	A_FREE(bss);

    	return 0;
}

/**************************************************************************
* processIsrFeatureEnableCmd - Process ISR_FEATURE_ENABLE_CMD command
*
* RETURNS: 0 processed OK, an Error code if it is not
*/
A_UINT32 processIsrFeatureEnableCmd
(
	struct dkThreadInfo *dkThread
)
{
#ifdef __ATH_DJGPPDOS__
	PIPE_CMD      *pCmd;
	CMD_REPLY     *ReplyCmd_p;
	A_UCHAR     *replyBuf;
	pCmd = dkThread->PipeBuffer_p;
	ReplyCmd_p = dkThread->ReplyCmd_p;
	replyBuf = (A_UCHAR *)(ReplyCmd_p->cmdBytes);

    /* check that the size is appropriate before we access */
	if( pCmd->cmdLen != sizeof(pCmd->CMD_U.ISR_FEATURE_ENABLE_CMD)+sizeof(pCmd->cmdID) +sizeof(pCmd->devNum)) 
        {
        uiPrintf("Error, command Len is not equal to size of ISR_FEATURE_ENABLE_CMD\n");
        return(COMMS_ERR_BAD_LENGTH);
        }

	if(hwEnableFeature(dkThread->currentDevIndex,pCmd) == -1)
		{
		uiPrintf("Error:  unable to Enable feature\n");
		return(COMMS_ERR_EVENT_ENABLE_FEATURE_FAIL);
	    }

#else //__ATH_DJGPPDOS__
	uiPrintf("create descriptors command not supported in DOS client\n");
#endif //__ATH_DJGPPDOS__
	return(0);
	}


/**************************************************************************
* processIsrFeatureDisableCmd - Process ISR_FEATURE_ENABLE_CMD command
*
* RETURNS: 0 processed OK, an Error code if it is not
*/
A_UINT32 processIsrFeatureDisableCmd
(
	struct dkThreadInfo *dkThread
)
{
#ifdef __ATH_DJGPPDOS__
	PIPE_CMD      *pCmd;
	A_UCHAR     *replyBuf;
	CMD_REPLY     *ReplyCmd_p;

    pCmd = dkThread->PipeBuffer_p;
	ReplyCmd_p = dkThread->ReplyCmd_p;
	replyBuf = (A_UCHAR *)(ReplyCmd_p->cmdBytes);

    /* check that the size is appropriate before we access */
	if( pCmd->cmdLen != sizeof(pCmd->CMD_U.ISR_FEATURE_DISABLE_CMD)+sizeof(pCmd->cmdID) +sizeof(pCmd->devNum)) 
        {
        uiPrintf("Error, command Len is not equal to size of ISR_FEATURE_DISABLE_CMD\n");
        return(COMMS_ERR_BAD_LENGTH);
        }

	if(hwDisableFeature(dkThread->currentDevIndex,pCmd) == -1)
		{
		uiPrintf("Error:  unable to disable feature\n");
		return(COMMS_ERR_EVENT_DISABLE_FEATURE_FAIL);
	    }

#else //__ATH_DJGPPDOS__
	uiPrintf("create descriptors command not supported in DOS client\n");
#endif //__ATH_DJGPPDOS__
	return(0);
	}

/**************************************************************************
* processIsrGetRxStatsCmd - Process ISR_GET_RX_STATS_CMD command
*
* RETURNS: 0 processed OK, an Error code if it is not
*/
A_UINT32 processIsrGetRxStatsCmd
(
	struct dkThreadInfo *dkThread
)
{
	PIPE_CMD      *pCmd;
	CMD_REPLY     *ReplyCmd_p;
	A_UCHAR     *replyBuf;

	pCmd = dkThread->PipeBuffer_p;
	ReplyCmd_p = dkThread->ReplyCmd_p;
	replyBuf = (A_UCHAR *)(ReplyCmd_p->cmdBytes);

    /* check that the size is appropriate before we access */
	if( pCmd->cmdLen != sizeof(pCmd->CMD_U.ISR_GET_STATS_CMD)+sizeof(pCmd->cmdID) +sizeof(pCmd->devNum)) 
        {
        uiPrintf("Error, command Len is not equal to size of ISR_GET_STATS_CMD\n");
        return(COMMS_ERR_BAD_LENGTH);
        }

	hwGetStats((A_UINT16)dkThread->currentDevIndex,pCmd->CMD_U.ISR_GET_STATS_CMD.clearOnRead, replyBuf, TRUE);
	return(0);
	}

/**************************************************************************
* processIsrGettxStatsCmd - Process ISR_GET_TX_STATS_CMD command
*
* RETURNS: 0 processed OK, an Error code if it is not
*/
A_UINT32 processIsrGetTxStatsCmd
(
	struct dkThreadInfo *dkThread
)
{
	PIPE_CMD      *pCmd;
	CMD_REPLY     *ReplyCmd_p;
	A_UCHAR     *replyBuf;

	pCmd = dkThread->PipeBuffer_p;
	ReplyCmd_p = dkThread->ReplyCmd_p;
	replyBuf = (A_UCHAR *)(ReplyCmd_p->cmdBytes);

	/* check that the size is appropriate before we access */
    if( pCmd->cmdLen != sizeof(pCmd->CMD_U.ISR_GET_STATS_CMD)+sizeof(pCmd->cmdID) +sizeof(pCmd->devNum)) 
        {
        uiPrintf("Error, command Len is not equal to size of ISR_GET_STATS_CMD\n");
        return(COMMS_ERR_BAD_LENGTH);
        }

	hwGetStats((A_UINT16)dkThread->currentDevIndex,pCmd->CMD_U.ISR_GET_STATS_CMD.clearOnRead, replyBuf, FALSE);
	return(0);
	}

/**************************************************************************
* processIsrSingleRxStatCmd - Process ISR_SINGLE_RX_STAT_CMD command
*
* RETURNS: 0 processed OK, an Error code if it is not
*/
A_UINT32 processIsrSingleRxStatCmd
(
	struct dkThreadInfo *dkThread
)
{
	PIPE_CMD      *pCmd;
	CMD_REPLY     *ReplyCmd_p;
	A_UCHAR     *replyBuf;

	pCmd = dkThread->PipeBuffer_p;
	ReplyCmd_p = dkThread->ReplyCmd_p;
	replyBuf = (A_UCHAR *)(ReplyCmd_p->cmdBytes);

	/* check that the size is appropriate before we access */
    if( pCmd->cmdLen != sizeof(pCmd->CMD_U.ISR_SINGLE_STAT_CMD)+sizeof(pCmd->cmdID) +sizeof(pCmd->devNum)) 
        {
        uiPrintf("Error, command Len is not equal to size of ISR_SINGLE_STAT_CMD\n");
        return(COMMS_ERR_BAD_LENGTH);
        }

	hwGetSingleStat((A_UINT16)dkThread->currentDevIndex,pCmd->CMD_U.ISR_SINGLE_STAT_CMD.statID, 
					pCmd->CMD_U.ISR_SINGLE_STAT_CMD.clearOnRead, replyBuf, TRUE);
	return(0);
}

/**************************************************************************
* processIsrSingleTxStatCmd - Process ISR_SINGLE_TX_STAT_CMD command
*
* 
*
* RETURNS: 0 processed OK, an Error code if it is not
*/
A_UINT32 processIsrSingleTxStatCmd
(
	struct dkThreadInfo *dkThread
)
{
	PIPE_CMD      *pCmd;
	CMD_REPLY     *ReplyCmd_p;
	A_UCHAR     *replyBuf;

	pCmd = dkThread->PipeBuffer_p;
	ReplyCmd_p = dkThread->ReplyCmd_p;
	replyBuf = (A_UCHAR *)(ReplyCmd_p->cmdBytes);

	/* check that the size is appropriate before we access */
     	if( pCmd->cmdLen != sizeof(pCmd->CMD_U.ISR_SINGLE_STAT_CMD)+sizeof(pCmd->cmdID) +sizeof(pCmd->devNum)) 
        {
        uiPrintf("Error, command Len is not equal to size of ISR_SINGLE_STAT_CMD\n");
        return(COMMS_ERR_BAD_LENGTH);
        }

	hwGetSingleStat((A_UINT16)dkThread->currentDevIndex,pCmd->CMD_U.ISR_SINGLE_STAT_CMD.statID, 
					pCmd->CMD_U.ISR_SINGLE_STAT_CMD.clearOnRead, replyBuf, FALSE);
	return(0);
}



/**************************************************************************
*/

A_UINT32 processCheckRegsCmd
(
	struct dkThreadInfo *dkThread
)
{
	PIPE_CMD      *pCmd;
	CMD_REPLY     *ReplyCmd_p;
	A_UCHAR     *replyBuf;

	pCmd = dkThread->PipeBuffer_p;
	ReplyCmd_p = dkThread->ReplyCmd_p;
	replyBuf = (A_UCHAR *)(ReplyCmd_p->cmdBytes);

	/* check that the size is appropriate before we access */
     	if( pCmd->cmdLen != sizeof(pCmd->cmdID) +sizeof(pCmd->devNum)) 
        {
        	uiPrintf("Error, command Len is not equal to size of CHECK REGS Cmd\n");
        	return(COMMS_ERR_BAD_LENGTH);
        }

	    /* read the eeprom */
    	*((A_UINT32 *)replyBuf) = m_checkRegs(pCmd->devNum);

#ifdef ENDIAN_SWAP
    	*((A_UINT32 *)replyBuf) = btol_l(*((A_UINT32 *)replyBuf));
#endif
    	return 0;

}

/**************************************************************************
*/

A_UINT32 processChangeChannelCmd
(
	struct dkThreadInfo *dkThread
)
{
	PIPE_CMD      *pCmd;
	CMD_REPLY     *ReplyCmd_p;
	A_UCHAR     *replyBuf;

	pCmd = dkThread->PipeBuffer_p;
	ReplyCmd_p = dkThread->ReplyCmd_p;
	replyBuf = (A_UCHAR *)(ReplyCmd_p->cmdBytes);

	/* check that the size is appropriate before we access */
     	if( pCmd->cmdLen != sizeof(pCmd->CMD_U.SET_ONE_CMD)+sizeof(pCmd->cmdID) +sizeof(pCmd->devNum)) 
        {
        	uiPrintf("Error, command Len is not equal to size of SET_ONE_CMD\n");
        	return(COMMS_ERR_BAD_LENGTH);
        }

	#ifdef ENDIAN_SWAP
     		pCmd->CMD_U.SET_ONE_CMD.param = ltob_l(pCmd->CMD_U.SET_ONE_CMD.param);
 	#endif

	
 	m_changeChannel(pCmd->devNum,pCmd->CMD_U.SET_ONE_CMD.param);

    	return 0;
}

/**************************************************************************
*/

A_UINT32 processCheckPromCmd
(
	struct dkThreadInfo *dkThread
)
{
	PIPE_CMD      *pCmd;
	CMD_REPLY     *ReplyCmd_p;
	A_UCHAR     *replyBuf;

	pCmd = dkThread->PipeBuffer_p;
	ReplyCmd_p = dkThread->ReplyCmd_p;
	replyBuf = (A_UCHAR *)(ReplyCmd_p->cmdBytes);

	/* check that the size is appropriate before we access */
     	if( pCmd->cmdLen != sizeof(pCmd->CMD_U.SET_ONE_CMD)+sizeof(pCmd->cmdID) +sizeof(pCmd->devNum)) 
        {
        	uiPrintf("Error, command Len is not equal to size of SET_ONE CMD\n");
        	return(COMMS_ERR_BAD_LENGTH);
        }

	#ifdef ENDIAN_SWAP
     		pCmd->CMD_U.SET_ONE_CMD.param = ltob_l(pCmd->CMD_U.SET_ONE_CMD.param);
 	#endif

	
 	*((A_UINT32 *)replyBuf) = m_checkProm(pCmd->devNum,pCmd->CMD_U.SET_ONE_CMD.param);

#ifdef ENDIAN_SWAP
    	*((A_UINT32 *)replyBuf) = btol_l(*((A_UINT32 *)replyBuf));
#endif


    	return 0;
}

/**************************************************************************
*/

A_UINT32 processRereadPromCmd
(
	struct dkThreadInfo *dkThread
)
{
	PIPE_CMD      *pCmd;
	CMD_REPLY     *ReplyCmd_p;
	A_UCHAR     *replyBuf;

	pCmd = dkThread->PipeBuffer_p;
	ReplyCmd_p = dkThread->ReplyCmd_p;
	replyBuf = (A_UCHAR *)(ReplyCmd_p->cmdBytes);

	/* check that the size is appropriate before we access */
     	if( pCmd->cmdLen != sizeof(pCmd->cmdID) +sizeof(pCmd->devNum)) 
        {
        	uiPrintf("Error, command Len is not equal to size of REREAD_PROM_CMD\n");
        	return(COMMS_ERR_BAD_LENGTH);
        }

	m_rereadProm(pCmd->devNum);

    	return 0;

}

/**************************************************************************
*/

A_UINT32 processTxDataSetupCmd
(
	struct dkThreadInfo *dkThread
)
{
	PIPE_CMD      *pCmd;
	CMD_REPLY     *ReplyCmd_p;
	A_UCHAR     *replyBuf;
	PDATABUFFER dest;
 	PDATABUFFER dataPattern;

	pCmd = dkThread->PipeBuffer_p;
	ReplyCmd_p = dkThread->ReplyCmd_p;
	replyBuf = (A_UCHAR *)(ReplyCmd_p->cmdBytes);

	#ifdef ENDIAN_SWAP
     		pCmd->CMD_U.TX_DATA_SETUP_CMD.dataPatternLength = ltob_l(pCmd->CMD_U.TX_DATA_SETUP_CMD.dataPatternLength);
 	#endif


	/* check that the size is appropriate before we access */
     	if( pCmd->cmdLen != (sizeof(pCmd->CMD_U.TX_DATA_SETUP_CMD)+
      		       sizeof(pCmd->cmdID)-
		       MAX_BLOCK_BYTES+
	       	       pCmd->CMD_U.TX_DATA_SETUP_CMD.dataPatternLength+sizeof(pCmd->devNum)))

        {
        	uiPrintf("Error, command Len is not equal to size of TX_DATA_SETUP_CMD\n");
        	return(COMMS_ERR_BAD_LENGTH);
        }

	#ifdef ENDIAN_SWAP
     		pCmd->CMD_U.TX_DATA_SETUP_CMD.rateMask = ltob_l(pCmd->CMD_U.TX_DATA_SETUP_CMD.rateMask);
     		pCmd->CMD_U.TX_DATA_SETUP_CMD.numDescPerRate = ltob_l(pCmd->CMD_U.TX_DATA_SETUP_CMD.numDescPerRate);
     		pCmd->CMD_U.TX_DATA_SETUP_CMD.dataBodyLength = ltob_l(pCmd->CMD_U.TX_DATA_SETUP_CMD.dataBodyLength);
     		pCmd->CMD_U.TX_DATA_SETUP_CMD.retries = ltob_l(pCmd->CMD_U.TX_DATA_SETUP_CMD.retries);
     		pCmd->CMD_U.TX_DATA_SETUP_CMD.antenna = ltob_l(pCmd->CMD_U.TX_DATA_SETUP_CMD.antenna);
     		pCmd->CMD_U.TX_DATA_SETUP_CMD.broadcast = ltob_l(pCmd->CMD_U.TX_DATA_SETUP_CMD.broadcast);
 	#endif

    /* 
      uiPrintf("SNOOP::processTxDataSetup:rateMask=%x:numDescPerRate=%d:dataBodyLength=%d:retries=%d:antenna=%d:broadcast=%x:\n",
     		pCmd->CMD_U.TX_DATA_SETUP_CMD.rateMask,
     		pCmd->CMD_U.TX_DATA_SETUP_CMD.numDescPerRate,
     		pCmd->CMD_U.TX_DATA_SETUP_CMD.dataBodyLength,
     		pCmd->CMD_U.TX_DATA_SETUP_CMD.retries,
     		pCmd->CMD_U.TX_DATA_SETUP_CMD.antenna,
     		pCmd->CMD_U.TX_DATA_SETUP_CMD.broadcast);
	*/
			
	dest = (PDATABUFFER)A_MALLOC(sizeof(DATABUFFER));
	if (dest == NULL)
	{
		uiPrintf("Error, unable to allocate memory for dest DATABUFFER struct \n");
		return (COMMS_ERR_ALLOC_FAIL);	
	}
	dest->Length = 6;
	dest->pData = (A_UCHAR *)A_MALLOC(dest->Length);
	if (dest->pData == NULL)
	{
		A_FREE(dest);
		uiPrintf("Error, unable to allocate memory for dest array \n");
		return (COMMS_ERR_ALLOC_FAIL);	
	}
	memcpy(dest->pData,pCmd->CMD_U.TX_DATA_SETUP_CMD.dest,dest->Length);

	dataPattern = (PDATABUFFER)A_MALLOC(sizeof(DATABUFFER));
	if (dataPattern == NULL)
	{
		A_FREE(dest->pData);
		A_FREE(dest);
		uiPrintf("Error, unable to allocate memory for dataPattern DATABUFFER struct \n");
		return (COMMS_ERR_ALLOC_FAIL);	
	}
	dataPattern->Length = (A_UINT16)pCmd->CMD_U.TX_DATA_SETUP_CMD.dataPatternLength;
	dataPattern->pData = (A_UCHAR *)A_MALLOC(dataPattern->Length);
	if (dataPattern->pData == NULL)
	{
		A_FREE(dest->pData);
		A_FREE(dest);
		A_FREE(dataPattern);
		uiPrintf("Error, unable to allocate memory for dataPattern array \n");
		return (COMMS_ERR_ALLOC_FAIL);	
	}
	memcpy(dataPattern->pData,pCmd->CMD_U.TX_DATA_SETUP_CMD.dataPattern,dataPattern->Length);

	m_txDataSetup(pCmd->devNum,
		      pCmd->CMD_U.TX_DATA_SETUP_CMD.rateMask,
		      dest,
		      pCmd->CMD_U.TX_DATA_SETUP_CMD.numDescPerRate,
		      pCmd->CMD_U.TX_DATA_SETUP_CMD.dataBodyLength,
		      dataPattern,
		      pCmd->CMD_U.TX_DATA_SETUP_CMD.retries,
		      pCmd->CMD_U.TX_DATA_SETUP_CMD.antenna,
		      pCmd->CMD_U.TX_DATA_SETUP_CMD.broadcast);


	A_FREE(dest->pData);
	A_FREE(dest);
	A_FREE(dataPattern->pData);
	A_FREE(dataPattern);
	
	
    	return 0;
}


/**************************************************************************
/* This is command response of processTxDataAggSetupCmd in dk_client
*/

A_UINT32 processTxDataAggSetupCmd
(
	struct dkThreadInfo *dkThread
)
{
	PIPE_CMD      *pCmd;
	CMD_REPLY     *ReplyCmd_p;
	A_UCHAR     *replyBuf;
	PDATABUFFER dest;
 	PDATABUFFER dataPattern;

	pCmd = dkThread->PipeBuffer_p;
	ReplyCmd_p = dkThread->ReplyCmd_p;
	replyBuf = (A_UCHAR *)(ReplyCmd_p->cmdBytes);

	#ifdef ENDIAN_SWAP
     		pCmd->CMD_U.TX_DATA_AGG_SETUP_CMD.dataPatternLength = ltob_l(pCmd->CMD_U.TX_DATA_AGG_SETUP_CMD.dataPatternLength);
 	#endif


     	if( pCmd->cmdLen != (sizeof(pCmd->CMD_U.TX_DATA_AGG_SETUP_CMD)+
      		       sizeof(pCmd->cmdID)-
		       MAX_BLOCK_BYTES+
	       	       pCmd->CMD_U.TX_DATA_AGG_SETUP_CMD.dataPatternLength+sizeof(pCmd->devNum)))

        {
        	uiPrintf("Error, command Len is not equal to size of TX_DATA_AGG_SETUP_CMD\n");
        	return(COMMS_ERR_BAD_LENGTH);
        }

	#ifdef ENDIAN_SWAP
     		pCmd->CMD_U.TX_DATA_AGG_SETUP_CMD.rateMask = ltob_l(pCmd->CMD_U.TX_DATA_AGG_SETUP_CMD.rateMask);
     		pCmd->CMD_U.TX_DATA_AGG_SETUP_CMD.numDescPerRate = ltob_l(pCmd->CMD_U.TX_DATA_AGG_SETUP_CMD.numDescPerRate);
     		pCmd->CMD_U.TX_DATA_AGG_SETUP_CMD.dataBodyLength = ltob_l(pCmd->CMD_U.TX_DATA_AGG_SETUP_CMD.dataBodyLength);
     		pCmd->CMD_U.TX_DATA_AGG_SETUP_CMD.retries = ltob_l(pCmd->CMD_U.TX_DATA_AGG_SETUP_CMD.retries);
     		pCmd->CMD_U.TX_DATA_AGG_SETUP_CMD.antenna = ltob_l(pCmd->CMD_U.TX_DATA_AGG_SETUP_CMD.antenna);
     		pCmd->CMD_U.TX_DATA_AGG_SETUP_CMD.broadcast = ltob_l(pCmd->CMD_U.TX_DATA_AGG_SETUP_CMD.broadcast);
     		pCmd->CMD_U.TX_DATA_AGG_SETUP_CMD.aggSize = ltob_l(pCmd->CMD_U.TX_DATA_AGG_SETUP_CMD.aggSize);
 	#endif

			
	dest = (PDATABUFFER)A_MALLOC(sizeof(DATABUFFER));
	if (dest == NULL)
	{
		uiPrintf("Error, unable to allocate memory for dest DATABUFFER struct \n");
		return (COMMS_ERR_ALLOC_FAIL);	
	}
	dest->Length = 6;
	dest->pData = (A_UCHAR *)A_MALLOC(dest->Length);
	if (dest->pData == NULL)
	{
		A_FREE(dest);
		uiPrintf("Error, unable to allocate memory for dest array \n");
		return (COMMS_ERR_ALLOC_FAIL);	
	}
	memcpy(dest->pData,pCmd->CMD_U.TX_DATA_AGG_SETUP_CMD.dest,dest->Length);

	dataPattern = (PDATABUFFER)A_MALLOC(sizeof(DATABUFFER));
	if (dataPattern == NULL)
	{
		A_FREE(dest->pData);
		A_FREE(dest);
		uiPrintf("Error, unable to allocate memory for dataPattern DATABUFFER struct \n");
		return (COMMS_ERR_ALLOC_FAIL);	
	}
	dataPattern->Length = (A_UINT16)pCmd->CMD_U.TX_DATA_AGG_SETUP_CMD.dataPatternLength;
	dataPattern->pData = (A_UCHAR *)A_MALLOC(dataPattern->Length);
	if (dataPattern->pData == NULL)
	{
		A_FREE(dest->pData);
		A_FREE(dest);
		A_FREE(dataPattern);
		uiPrintf("Error, unable to allocate memory for dataPattern array \n");
		return (COMMS_ERR_ALLOC_FAIL);	
	}
	memcpy(dataPattern->pData,pCmd->CMD_U.TX_DATA_AGG_SETUP_CMD.dataPattern,dataPattern->Length);

	m_txDataAggSetup(pCmd->devNum,
		      pCmd->CMD_U.TX_DATA_AGG_SETUP_CMD.rateMask,
		      dest,
		      pCmd->CMD_U.TX_DATA_AGG_SETUP_CMD.numDescPerRate,
		      pCmd->CMD_U.TX_DATA_AGG_SETUP_CMD.dataBodyLength,
		      dataPattern,
		      pCmd->CMD_U.TX_DATA_AGG_SETUP_CMD.retries,
		      pCmd->CMD_U.TX_DATA_AGG_SETUP_CMD.antenna,
		      pCmd->CMD_U.TX_DATA_AGG_SETUP_CMD.broadcast,
			  pCmd->CMD_U.TX_DATA_AGG_SETUP_CMD.aggSize);


	A_FREE(dest->pData);
	A_FREE(dest);
	A_FREE(dataPattern->pData);
	A_FREE(dataPattern);
    return 0;
}

/**************************************************************************
*/

A_UINT32 processTxDataBeginCmd
(
	struct dkThreadInfo *dkThread
)
{
	PIPE_CMD      *pCmd;
	CMD_REPLY     *ReplyCmd_p;
	A_UCHAR     *replyBuf;

	pCmd = dkThread->PipeBuffer_p;
	ReplyCmd_p = dkThread->ReplyCmd_p;
	replyBuf = (A_UCHAR *)(ReplyCmd_p->cmdBytes);

	/* check that the size is appropriate before we access */
     	if( pCmd->cmdLen != sizeof(pCmd->CMD_U.TX_DATA_BEGIN_CMD)+sizeof(pCmd->cmdID) +sizeof(pCmd->devNum)) 
        {
        	uiPrintf("Error, command Len is not equal to size of TX_DATA_BEGIN_CMD\n");
        	return(COMMS_ERR_BAD_LENGTH);
        }

	#ifdef ENDIAN_SWAP
     		pCmd->CMD_U.TX_DATA_BEGIN_CMD.timeout = ltob_l(pCmd->CMD_U.TX_DATA_BEGIN_CMD.timeout);
     		pCmd->CMD_U.TX_DATA_BEGIN_CMD.remoteStats= ltob_l(pCmd->CMD_U.TX_DATA_BEGIN_CMD.remoteStats);
 	#endif

	m_txDataBegin(pCmd->devNum,
		      pCmd->CMD_U.TX_DATA_BEGIN_CMD.timeout,
		      pCmd->CMD_U.TX_DATA_BEGIN_CMD.remoteStats);
	
    	return 0;
}

/**************************************************************************
*/

A_UINT32 processTxDataCompleteCmd
(
	struct dkThreadInfo *dkThread
)
{
	PIPE_CMD      *pCmd;
	CMD_REPLY     *ReplyCmd_p;
	A_UCHAR     *replyBuf;

	pCmd = dkThread->PipeBuffer_p;
	ReplyCmd_p = dkThread->ReplyCmd_p;
	replyBuf = (A_UCHAR *)(ReplyCmd_p->cmdBytes);

	/* check that the size is appropriate before we access */
     	if( pCmd->cmdLen != sizeof(pCmd->CMD_U.TX_DATA_BEGIN_CMD)+sizeof(pCmd->cmdID) +sizeof(pCmd->devNum)) 
        {
        	uiPrintf("Error, command Len is not equal to size of TX_DATA_COM LETE_CMD\n");
        	return(COMMS_ERR_BAD_LENGTH);
        }

	#ifdef ENDIAN_SWAP
     		pCmd->CMD_U.TX_DATA_BEGIN_CMD.timeout = ltob_l(pCmd->CMD_U.TX_DATA_BEGIN_CMD.timeout);
     		pCmd->CMD_U.TX_DATA_BEGIN_CMD.remoteStats= ltob_l(pCmd->CMD_U.TX_DATA_BEGIN_CMD.remoteStats);
 	#endif

	//Calling direct without going through devlibif
	txDataComplete(pCmd->devNum,
		      pCmd->CMD_U.TX_DATA_BEGIN_CMD.timeout,
		      pCmd->CMD_U.TX_DATA_BEGIN_CMD.remoteStats);
	
    	return 0;
}

/**************************************************************************
*/

A_UINT32 processRxDataSetupCmd
(
	struct dkThreadInfo *dkThread
)
{
	PIPE_CMD      *pCmd;
	CMD_REPLY     *ReplyCmd_p;
	A_UCHAR     *replyBuf;

	pCmd = dkThread->PipeBuffer_p;
	ReplyCmd_p = dkThread->ReplyCmd_p;
	replyBuf = (A_UCHAR *)(ReplyCmd_p->cmdBytes);

	/* check that the size is appropriate before we access */
     	if( pCmd->cmdLen != sizeof(pCmd->CMD_U.RX_DATA_SETUP_CMD)+sizeof(pCmd->cmdID) +sizeof(pCmd->devNum)) 
        {
        	uiPrintf("Error, command Len is not equal to size of RX_DATA_SETUP_CMD\n");
        	return(COMMS_ERR_BAD_LENGTH);
        }

	#ifdef ENDIAN_SWAP
     		pCmd->CMD_U.RX_DATA_SETUP_CMD.numDesc = ltob_l(pCmd->CMD_U.RX_DATA_SETUP_CMD.numDesc);
     		pCmd->CMD_U.RX_DATA_SETUP_CMD.dataBodyLength = ltob_l(pCmd->CMD_U.RX_DATA_SETUP_CMD.dataBodyLength);
     		pCmd->CMD_U.RX_DATA_SETUP_CMD.enablePPM = ltob_l(pCmd->CMD_U.RX_DATA_SETUP_CMD.enablePPM);
 	#endif

	m_rxDataSetup(pCmd->devNum,
		      pCmd->CMD_U.RX_DATA_SETUP_CMD.numDesc,
		      pCmd->CMD_U.RX_DATA_SETUP_CMD.dataBodyLength,
		      pCmd->CMD_U.RX_DATA_SETUP_CMD.enablePPM);
	
    	return 0;
}


/**************************************************************************
*/

A_UINT32 processRxDataAggSetupCmd
(
	struct dkThreadInfo *dkThread
)
{
	PIPE_CMD      *pCmd;
	CMD_REPLY     *ReplyCmd_p;
	A_UCHAR     *replyBuf;

	pCmd = dkThread->PipeBuffer_p;
	ReplyCmd_p = dkThread->ReplyCmd_p;
	replyBuf = (A_UCHAR *)(ReplyCmd_p->cmdBytes);

     	if( pCmd->cmdLen != sizeof(pCmd->CMD_U.RX_DATA_AGG_SETUP_CMD)+sizeof(pCmd->cmdID) +sizeof(pCmd->devNum)) 
        {
        	uiPrintf("Error, command Len is not equal to size of RX_DATA_AGG_SETUP_CMD\n");
        	return(COMMS_ERR_BAD_LENGTH);
        }

	#ifdef ENDIAN_SWAP
     		pCmd->CMD_U.RX_DATA_AGG_SETUP_CMD.numDesc = ltob_l(pCmd->CMD_U.RX_DATA_AGG_SETUP_CMD.numDesc);
     		pCmd->CMD_U.RX_DATA_AGG_SETUP_CMD.dataBodyLength = ltob_l(pCmd->CMD_U.RX_DATA_AGG_SETUP_CMD.dataBodyLength);
     		pCmd->CMD_U.RX_DATA_AGG_SETUP_CMD.enablePPM = ltob_l(pCmd->CMD_U.RX_DATA_AGG_SETUP_CMD.enablePPM);
     		pCmd->CMD_U.RX_DATA_AGG_SETUP_CMD.aggSize = ltob_l(pCmd->CMD_U.RX_DATA_AGG_SETUP_CMD.aggSize);
 	#endif

	m_rxDataAggSetup(pCmd->devNum,
		      pCmd->CMD_U.RX_DATA_AGG_SETUP_CMD.numDesc,
		      pCmd->CMD_U.RX_DATA_AGG_SETUP_CMD.dataBodyLength,
		      pCmd->CMD_U.RX_DATA_AGG_SETUP_CMD.enablePPM,
			  pCmd->CMD_U.RX_DATA_AGG_SETUP_CMD.aggSize);
	
    	return 0;
}

/**************************************************************************
*/

A_UINT32 processRxDataBeginCmd
(
	struct dkThreadInfo *dkThread
)
{
	PIPE_CMD      *pCmd;
	CMD_REPLY     *ReplyCmd_p;
	A_UCHAR     *replyBuf;
	PDATABUFFER dataPattern;

	pCmd = dkThread->PipeBuffer_p;
	ReplyCmd_p = dkThread->ReplyCmd_p;
	replyBuf = (A_UCHAR *)(ReplyCmd_p->cmdBytes);

	#ifdef ENDIAN_SWAP
     		pCmd->CMD_U.RX_DATA_BEGIN_CMD.dataPatternLength = ltob_l(pCmd->CMD_U.RX_DATA_BEGIN_CMD.dataPatternLength);
	#endif

	/* check that the size is appropriate before we access */
	if( pCmd->cmdLen != (sizeof(pCmd->CMD_U.RX_DATA_BEGIN_CMD)+
		      sizeof(pCmd->cmdID)-
		      MAX_BLOCK_BYTES +
		      pCmd->CMD_U.RX_DATA_BEGIN_CMD.dataPatternLength+sizeof(pCmd->devNum))) 
	
        {
        	uiPrintf("Error, command Len is not equal to size of RX_DATA_BEGIN_CMD\n");
        	return(COMMS_ERR_BAD_LENGTH);
        }

	#ifdef ENDIAN_SWAP
     		pCmd->CMD_U.RX_DATA_BEGIN_CMD.waitTime = ltob_l(pCmd->CMD_U.RX_DATA_BEGIN_CMD.waitTime);
     		pCmd->CMD_U.RX_DATA_BEGIN_CMD.timeout = ltob_l(pCmd->CMD_U.RX_DATA_BEGIN_CMD.timeout);
     		pCmd->CMD_U.RX_DATA_BEGIN_CMD.remoteStats = ltob_l(pCmd->CMD_U.RX_DATA_BEGIN_CMD.remoteStats);
     		pCmd->CMD_U.RX_DATA_BEGIN_CMD.enableCompare = ltob_l(pCmd->CMD_U.RX_DATA_BEGIN_CMD.enableCompare);
 	#endif

	dataPattern = (PDATABUFFER)A_MALLOC(sizeof(DATABUFFER));
	if (dataPattern == NULL)
	{
		uiPrintf("Error, unable to allocate memory for dataPattern DATABUFFER struct \n");
		return (COMMS_ERR_ALLOC_FAIL);	
	}
	dataPattern->Length = (A_UINT16)pCmd->CMD_U.RX_DATA_BEGIN_CMD.dataPatternLength;
	dataPattern->pData = (A_UCHAR *)A_MALLOC(dataPattern->Length);
	if (dataPattern->pData == NULL)
	{
		A_FREE(dataPattern);
		uiPrintf("Error, unable to allocate memory for dataPattern array \n");
		return (COMMS_ERR_ALLOC_FAIL);	
	}
	memcpy(dataPattern->pData,pCmd->CMD_U.RX_DATA_BEGIN_CMD.dataPattern,dataPattern->Length);

	m_rxDataBegin(pCmd->devNum,
		      pCmd->CMD_U.RX_DATA_BEGIN_CMD.waitTime,
		      pCmd->CMD_U.RX_DATA_BEGIN_CMD.timeout,
		      pCmd->CMD_U.RX_DATA_BEGIN_CMD.remoteStats,
		      pCmd->CMD_U.RX_DATA_BEGIN_CMD.enableCompare,
		      dataPattern);

	A_FREE(dataPattern->pData);
	A_FREE(dataPattern);
	
    	return 0;
}

/**************************************************************************
*/

A_UINT32 processRxDataCompleteCmd
(
	struct dkThreadInfo *dkThread
)
{
	PIPE_CMD      *pCmd;
	CMD_REPLY     *ReplyCmd_p;
	A_UCHAR     *replyBuf;

	pCmd = dkThread->PipeBuffer_p;
	ReplyCmd_p = dkThread->ReplyCmd_p;
	replyBuf = (A_UCHAR *)(ReplyCmd_p->cmdBytes);

	#ifdef ENDIAN_SWAP
     		pCmd->CMD_U.RX_DATA_BEGIN_CMD.dataPatternLength = ltob_l(pCmd->CMD_U.RX_DATA_BEGIN_CMD.dataPatternLength);
	#endif

	/* check that the size is appropriate before we access */
	if( pCmd->cmdLen != (sizeof(pCmd->CMD_U.RX_DATA_BEGIN_CMD)+
		      sizeof(pCmd->cmdID)-
		      MAX_BLOCK_BYTES +
		      pCmd->CMD_U.RX_DATA_BEGIN_CMD.dataPatternLength+sizeof(pCmd->devNum))) 
	
        {
        	uiPrintf("Error, command Len is not equal to size of RX_DATA_COMPLETE_CMD\n");
        	return(COMMS_ERR_BAD_LENGTH);
        }

	#ifdef ENDIAN_SWAP
     		pCmd->CMD_U.RX_DATA_BEGIN_CMD.waitTime = ltob_l(pCmd->CMD_U.RX_DATA_BEGIN_CMD.waitTime);
     		pCmd->CMD_U.RX_DATA_BEGIN_CMD.timeout = ltob_l(pCmd->CMD_U.RX_DATA_BEGIN_CMD.timeout);
     		pCmd->CMD_U.RX_DATA_BEGIN_CMD.remoteStats = ltob_l(pCmd->CMD_U.RX_DATA_BEGIN_CMD.remoteStats);
     		pCmd->CMD_U.RX_DATA_BEGIN_CMD.enableCompare = ltob_l(pCmd->CMD_U.RX_DATA_BEGIN_CMD.enableCompare);
 	#endif


	//call library direct
	rxDataComplete(pCmd->devNum,
		      pCmd->CMD_U.RX_DATA_BEGIN_CMD.waitTime,
		      pCmd->CMD_U.RX_DATA_BEGIN_CMD.timeout,
		      pCmd->CMD_U.RX_DATA_BEGIN_CMD.remoteStats,
		      pCmd->CMD_U.RX_DATA_BEGIN_CMD.enableCompare,
		      pCmd->CMD_U.RX_DATA_BEGIN_CMD.dataPattern,
			  pCmd->CMD_U.RX_DATA_BEGIN_CMD.dataPatternLength);

   	return 0;
}

A_UINT32 processTxDataStartCmd
(
	struct dkThreadInfo *dkThread
)
{
	PIPE_CMD      *pCmd;
	CMD_REPLY     *ReplyCmd_p;
	A_UCHAR     *replyBuf;

	pCmd = dkThread->PipeBuffer_p;
	ReplyCmd_p = dkThread->ReplyCmd_p;
	replyBuf = (A_UCHAR *)(ReplyCmd_p->cmdBytes);

	/* check that the size is appropriate before we access */
     	if( pCmd->cmdLen != sizeof(pCmd->cmdID) +sizeof(pCmd->devNum)) 
        {
        	uiPrintf("Error, command Len is not equal to size of TX_DATA_START cmd\n");
        	return(COMMS_ERR_BAD_LENGTH);
        }

    	m_txDataStart(pCmd->devNum);

    	return 0;

}

A_UINT32 processRxDataStartCmd
(
	struct dkThreadInfo *dkThread
)
{
	PIPE_CMD      *pCmd;
	CMD_REPLY     *ReplyCmd_p;
	A_UCHAR     *replyBuf;

	pCmd = dkThread->PipeBuffer_p;
	ReplyCmd_p = dkThread->ReplyCmd_p;
	replyBuf = (A_UCHAR *)(ReplyCmd_p->cmdBytes);

	/* check that the size is appropriate before we access */
     	if( pCmd->cmdLen != sizeof(pCmd->cmdID) +sizeof(pCmd->devNum)) 
        {
        	uiPrintf("Error, command Len is not equal to size of RX_DATA_START cmd\n");
        	return(COMMS_ERR_BAD_LENGTH);
        }

    	m_rxDataStart(pCmd->devNum);

    	return 0;

}

/**************************************************************************
*/

A_UINT32 processTxRxDataBeginCmd
(
	struct dkThreadInfo *dkThread
)
{
	PIPE_CMD      *pCmd;
	CMD_REPLY     *ReplyCmd_p;
	A_UCHAR     *replyBuf;
	PDATABUFFER dataPattern;

	pCmd = dkThread->PipeBuffer_p;
	ReplyCmd_p = dkThread->ReplyCmd_p;
	replyBuf = (A_UCHAR *)(ReplyCmd_p->cmdBytes);

	/* check that the size is appropriate before we access */
     	if( pCmd->cmdLen != sizeof(pCmd->CMD_U.RX_DATA_BEGIN_CMD)+sizeof(pCmd->cmdID) +sizeof(pCmd->devNum)) 
        {
        	uiPrintf("Error, command Len is not equal to size of RX_DATA_BEGIN_CMD\n");
        	return(COMMS_ERR_BAD_LENGTH);
        }

	#ifdef ENDIAN_SWAP
     		pCmd->CMD_U.RX_DATA_BEGIN_CMD.waitTime = ltob_l(pCmd->CMD_U.RX_DATA_BEGIN_CMD.waitTime);
     		pCmd->CMD_U.RX_DATA_BEGIN_CMD.timeout = ltob_l(pCmd->CMD_U.RX_DATA_BEGIN_CMD.timeout);
     		pCmd->CMD_U.RX_DATA_BEGIN_CMD.remoteStats = ltob_l(pCmd->CMD_U.RX_DATA_BEGIN_CMD.remoteStats);
     		pCmd->CMD_U.RX_DATA_BEGIN_CMD.enableCompare = ltob_l(pCmd->CMD_U.RX_DATA_BEGIN_CMD.enableCompare);
     		pCmd->CMD_U.RX_DATA_BEGIN_CMD.dataPatternLength = ltob_l(pCmd->CMD_U.RX_DATA_BEGIN_CMD.dataPatternLength);
 	#endif

	dataPattern = (PDATABUFFER)A_MALLOC(sizeof(DATABUFFER));
	if (dataPattern == NULL)
	{
		uiPrintf("Error, unable to allocate memory for dataPattern PDATABUFFER struct \n");
		return (COMMS_ERR_ALLOC_FAIL);	
	}
	dataPattern->Length = (A_UINT16)pCmd->CMD_U.RX_DATA_BEGIN_CMD.dataPatternLength;
	dataPattern->pData = (A_UCHAR *)A_MALLOC(dataPattern->Length);
	if (dataPattern->pData == NULL)
	{
		A_FREE(dataPattern);
		uiPrintf("Error, unable to allocate memory for dataPattern array \n");
		return (COMMS_ERR_ALLOC_FAIL);	
	}
	memcpy(dataPattern->pData,pCmd->CMD_U.RX_DATA_BEGIN_CMD.dataPattern,dataPattern->Length);

	m_txrxDataBegin(pCmd->devNum,
		      pCmd->CMD_U.RX_DATA_BEGIN_CMD.waitTime,
		      pCmd->CMD_U.RX_DATA_BEGIN_CMD.timeout,
		      pCmd->CMD_U.RX_DATA_BEGIN_CMD.remoteStats,
		      pCmd->CMD_U.RX_DATA_BEGIN_CMD.enableCompare,
		      dataPattern);

	A_FREE(dataPattern->pData);
	A_FREE(dataPattern);

    	return 0;
}

A_UINT32 processCleanupTxRxMemoryCmd
(
	struct dkThreadInfo *dkThread
)
{
	PIPE_CMD      *pCmd;
	CMD_REPLY     *ReplyCmd_p;
	A_UCHAR     *replyBuf;

	pCmd = dkThread->PipeBuffer_p;
	ReplyCmd_p = dkThread->ReplyCmd_p;
	replyBuf = (A_UCHAR *)(ReplyCmd_p->cmdBytes);

	/* check that the size is appropriate before we access */
     	if( pCmd->cmdLen != sizeof(pCmd->CMD_U.SET_ONE_CMD)+sizeof(pCmd->cmdID) +sizeof(pCmd->devNum)) 
        {
        	uiPrintf("Error, command Len is not equal to size of SET_ONE_CMD \n");
        	return(COMMS_ERR_BAD_LENGTH);
        }

	#ifdef ENDIAN_SWAP
     		pCmd->CMD_U.SET_ONE_CMD.param = ltob_l(pCmd->CMD_U.SET_ONE_CMD.param);
 	#endif

	m_cleanupTxRxMemory(pCmd->devNum,
				pCmd->CMD_U.SET_ONE_CMD.param);

    	return 0;
}



/**************************************************************************
*/

A_UINT32 processTxGetStatsCmd
(
	struct dkThreadInfo *dkThread
)
{
	PIPE_CMD      *pCmd;
	CMD_REPLY     *ReplyCmd_p;
	A_UCHAR     *replyBuf;
	PDWORDBUFFER pStruct;

	pCmd = dkThread->PipeBuffer_p;
	ReplyCmd_p = dkThread->ReplyCmd_p;
	replyBuf = (A_UCHAR *)(ReplyCmd_p->cmdBytes);

	/* check that the size is appropriate before we access */
     	if( pCmd->cmdLen != sizeof(pCmd->CMD_U.GET_STATS_CMD)+sizeof(pCmd->cmdID) +sizeof(pCmd->devNum)) 
        {
        	uiPrintf("Error, command Len is not equal to size of GET_STATS_CMD\n");
        	return(COMMS_ERR_BAD_LENGTH);
        }

	#ifdef ENDIAN_SWAP
     		pCmd->CMD_U.GET_STATS_CMD.rateInMb = ltob_l(pCmd->CMD_U.GET_STATS_CMD.rateInMb);
     		pCmd->CMD_U.GET_STATS_CMD.remote = ltob_l(pCmd->CMD_U.GET_STATS_CMD.remote);
 	#endif

	pStruct = m_txGetStats(pCmd->devNum,
		     		 pCmd->CMD_U.GET_STATS_CMD.rateInMb,
		     		 pCmd->CMD_U.GET_STATS_CMD.remote);

	if (pStruct != NULL)
	{
		memcpy((A_UCHAR *)replyBuf,(A_UCHAR *)pStruct->pDword, sizeof(TX_STATS_STRUCT));
		A_FREE(pStruct->pDword);
		A_FREE(pStruct);
	}

#ifdef ENDIAN_SWAP
// assumption all struct variables are 4 byte words
	{
		int i;

		for (i=0;i<(sizeof(TX_STATS_STRUCT));i+=4)
		{
	  	  	*((A_UINT32 *)(replyBuf+i)) = btol_l(*((A_UINT32 *)(replyBuf+i)));
		}
	}
#endif
		     
    	return 0;
}

/**************************************************************************
*/

A_UINT32 processRxGetStatsCmd
(
	struct dkThreadInfo *dkThread
)
{
	PIPE_CMD      *pCmd;
	CMD_REPLY     *ReplyCmd_p;
	A_UCHAR     *replyBuf;
	PDWORDBUFFER pStruct;

	pCmd = dkThread->PipeBuffer_p;
	ReplyCmd_p = dkThread->ReplyCmd_p;
	replyBuf = (A_UCHAR *)(ReplyCmd_p->cmdBytes);

	/* check that the size is appropriate before we access */
     	if( pCmd->cmdLen != sizeof(pCmd->CMD_U.GET_STATS_CMD)+sizeof(pCmd->cmdID) +sizeof(pCmd->devNum)) 
        {
        	uiPrintf("Error, command Len is not equal to size of GET_STATS_CMD\n");
        	return(COMMS_ERR_BAD_LENGTH);
        }

	#ifdef ENDIAN_SWAP
     		pCmd->CMD_U.GET_STATS_CMD.rateInMb = ltob_l(pCmd->CMD_U.GET_STATS_CMD.rateInMb);
     		pCmd->CMD_U.GET_STATS_CMD.remote = ltob_l(pCmd->CMD_U.GET_STATS_CMD.remote);
 	#endif

	pStruct = m_rxGetStats(pCmd->devNum,
		     		 pCmd->CMD_U.GET_STATS_CMD.rateInMb,
		     		 pCmd->CMD_U.GET_STATS_CMD.remote);

	if (pStruct != NULL)
	{
		memcpy((A_UCHAR *)replyBuf,(A_UCHAR *)pStruct->pDword, sizeof(RX_STATS_STRUCT));
		A_FREE(pStruct->pDword);
		A_FREE(pStruct);
	}

#ifdef ENDIAN_SWAP
// assumption all struct variables are 4 byte words
	{
		int i;

		for (i=0;i<(sizeof(RX_STATS_STRUCT));i+=4)
		{
	  	  	*((A_UINT32 *)(replyBuf+i)) = btol_l(*((A_UINT32 *)(replyBuf+i)));
		}
	}
#endif

		     
    	return 0;
}

A_UINT32 processRxStatsSnapshotCmd
(
	struct dkThreadInfo *dkThread
)
{
	PIPE_CMD      *pCmd;
	CMD_REPLY     *ReplyCmd_p;
	A_UCHAR     *replyBuf;
	RX_STATS_SNAPSHOT statsStruct;
	A_BOOL returnFlag;

	pCmd = dkThread->PipeBuffer_p;
	ReplyCmd_p = dkThread->ReplyCmd_p;
	replyBuf = (A_UCHAR *)(ReplyCmd_p->cmdBytes);

	/* check that the size is appropriate before we access */
    if( pCmd->cmdLen != sizeof(pCmd->cmdID) +sizeof(pCmd->devNum)) 
    {
        uiPrintf("Error, command Len is not equal to size of RX_STATS_SNAPSHOT_CMD\n");
        return(COMMS_ERR_BAD_LENGTH);
    }

	returnFlag = rxLastDescStatsSnapshot(pCmd->devNum, &statsStruct);

	//put the reply flag in the reply buffer first
	*replyBuf = returnFlag;
	replyBuf+=4;

	//copy the stats into the reply buffer
	memcpy(replyBuf, &statsStruct, sizeof(RX_STATS_SNAPSHOT));

#ifdef ENDIAN_SWAP
// assumption all struct variables are 4 byte words
	{
		int i;

		for (i=0;i<(sizeof(RX_STATS_SNAPSHOT));i+=4)
		{
	  	  	*((A_UINT32 *)(replyBuf+i)) = btol_l(*((A_UINT32 *)(replyBuf+i)));
		}
	}
#endif
		     
    return 0;
}

A_UINT32 processGetCtlPowerInfoCmd
(
	struct dkThreadInfo *dkThread
)
{
	PIPE_CMD      *pCmd;
	CMD_REPLY     *ReplyCmd_p;
	A_UCHAR     *replyBuf;
	CTL_POWER_INFO ctlInfo;

	pCmd = dkThread->PipeBuffer_p;
	ReplyCmd_p = dkThread->ReplyCmd_p;
	replyBuf = (A_UCHAR *)(ReplyCmd_p->cmdBytes);

	/* check that the size is appropriate before we access */
    if( pCmd->cmdLen != sizeof(pCmd->cmdID) +sizeof(pCmd->devNum)) 
    {
        uiPrintf("Error, command Len is not equal to size of GET_CTL_INFO_CMD\n");
        return(COMMS_ERR_BAD_LENGTH);
    }

	getCtlPowerInfo(pCmd->devNum, &ctlInfo);

	//copy the stats into the reply buffer
	memcpy(replyBuf, &ctlInfo, sizeof(CTL_POWER_INFO));

#ifdef ENDIAN_SWAP
// assumption all struct variables are 4 byte words
	{
		int i;

		for (i=0;i<(sizeof(CTL_POWER_INFO));i+=4)
		{
	  	  	*((A_UINT32 *)(replyBuf+i)) = btol_l(*((A_UINT32 *)(replyBuf+i)));
		}
	}
#endif
		     
    return 0;
}



/**************************************************************************
*/

A_UINT32 processRxGetDataCmd
(
	struct dkThreadInfo *dkThread
)
{
	PIPE_CMD      *pCmd;
	CMD_REPLY     *ReplyCmd_p;
	A_UCHAR     *replyBuf;
	PDATABUFFER pStruct;

	pCmd = dkThread->PipeBuffer_p;
	ReplyCmd_p = dkThread->ReplyCmd_p;
	replyBuf = (A_UCHAR *)(ReplyCmd_p->cmdBytes);

	/* check that the size is appropriate before we access */
     	if( pCmd->cmdLen != sizeof(pCmd->CMD_U.RX_GET_DATA_CMD)+sizeof(pCmd->cmdID) +sizeof(pCmd->devNum)) 
        {
        	uiPrintf("Error, command Len is not equal to size of RX_GET_DATA_CMD\n");
        	return(COMMS_ERR_BAD_LENGTH);
        }

	#ifdef ENDIAN_SWAP
     		pCmd->CMD_U.RX_GET_DATA_CMD.bufferNum = ltob_l(pCmd->CMD_U.RX_GET_DATA_CMD.bufferNum);
     		pCmd->CMD_U.RX_GET_DATA_CMD.sizeBuffer = ltob_l(pCmd->CMD_U.RX_GET_DATA_CMD.sizeBuffer);
 	#endif

	pStruct = m_rxGetData(pCmd->devNum,
		     	      pCmd->CMD_U.RX_GET_DATA_CMD.bufferNum,
		     	      (A_UINT16)pCmd->CMD_U.RX_GET_DATA_CMD.sizeBuffer);

	if (pStruct != NULL)
	{
		memcpy((A_UCHAR *)replyBuf,(A_UCHAR *)pStruct->pData,pCmd->CMD_U.RX_GET_DATA_CMD.sizeBuffer);
		A_FREE(pStruct->pData);
		A_FREE(pStruct);
	}
		     
    	return 0;
}

/**************************************************************************
*/

A_UINT32 processTxContBeginCmd
(
	struct dkThreadInfo *dkThread
)
{
	PIPE_CMD      *pCmd;
	CMD_REPLY     *ReplyCmd_p;
	A_UCHAR     *replyBuf;

	pCmd = dkThread->PipeBuffer_p;
	ReplyCmd_p = dkThread->ReplyCmd_p;
	replyBuf = (A_UCHAR *)(ReplyCmd_p->cmdBytes);

	/* check that the size is appropriate before we access */
     	if( pCmd->cmdLen != sizeof(pCmd->CMD_U.TX_CONT_BEGIN_CMD)+sizeof(pCmd->cmdID) +sizeof(pCmd->devNum)) 
        {
        	uiPrintf("Error, command Len is not equal to size of TX_CONT_BEGIN_CMD\n");
        	return(COMMS_ERR_BAD_LENGTH);
        }

	#ifdef ENDIAN_SWAP
     		pCmd->CMD_U.TX_CONT_BEGIN_CMD.type = ltob_l(pCmd->CMD_U.TX_CONT_BEGIN_CMD.type);
     		pCmd->CMD_U.TX_CONT_BEGIN_CMD.typeOption1 = ltob_l(pCmd->CMD_U.TX_CONT_BEGIN_CMD.typeOption1);
     		pCmd->CMD_U.TX_CONT_BEGIN_CMD.typeOption2 = ltob_l(pCmd->CMD_U.TX_CONT_BEGIN_CMD.typeOption2);
     		pCmd->CMD_U.TX_CONT_BEGIN_CMD.antenna = ltob_l(pCmd->CMD_U.TX_CONT_BEGIN_CMD.antenna);
 	#endif

	m_txContBegin(pCmd->devNum,
     		      pCmd->CMD_U.TX_CONT_BEGIN_CMD.type,
     		      pCmd->CMD_U.TX_CONT_BEGIN_CMD.typeOption1,
     		      pCmd->CMD_U.TX_CONT_BEGIN_CMD.typeOption2,
     		      pCmd->CMD_U.TX_CONT_BEGIN_CMD.antenna);
		     
    	return 0;
}

A_UINT32 processTxContFrameBeginCmd
(
	struct dkThreadInfo *dkThread
)
{
	PIPE_CMD      *pCmd;
	CMD_REPLY     *ReplyCmd_p;
	A_UCHAR     *replyBuf;

	pCmd = dkThread->PipeBuffer_p;
	ReplyCmd_p = dkThread->ReplyCmd_p;
	replyBuf = (A_UCHAR *)(ReplyCmd_p->cmdBytes);

	/* check that the size is appropriate before we access */
     	if( pCmd->cmdLen != sizeof(pCmd->CMD_U.TX_CONT_FRAME_BEGIN_CMD)+sizeof(pCmd->cmdID) +sizeof(pCmd->devNum)) 
        {
        	uiPrintf("Error, command Len is not equal to size of TX_CONT_FRAME_BEGIN_CMD\n");
        	return(COMMS_ERR_BAD_LENGTH);
        }

	#ifdef ENDIAN_SWAP
     		pCmd->CMD_U.TX_CONT_FRAME_BEGIN_CMD.length = ltob_l(pCmd->CMD_U.TX_CONT_FRAME_BEGIN_CMD.length);
     		pCmd->CMD_U.TX_CONT_FRAME_BEGIN_CMD.ifswait = ltob_l(pCmd->CMD_U.TX_CONT_FRAME_BEGIN_CMD.ifswait);
     		pCmd->CMD_U.TX_CONT_FRAME_BEGIN_CMD.typeOption1 = ltob_l(pCmd->CMD_U.TX_CONT_FRAME_BEGIN_CMD.typeOption1);
     		pCmd->CMD_U.TX_CONT_FRAME_BEGIN_CMD.typeOption2 = ltob_l(pCmd->CMD_U.TX_CONT_FRAME_BEGIN_CMD.typeOption2);
     		pCmd->CMD_U.TX_CONT_FRAME_BEGIN_CMD.antenna = ltob_l(pCmd->CMD_U.TX_CONT_FRAME_BEGIN_CMD.antenna);
     		pCmd->CMD_U.TX_CONT_FRAME_BEGIN_CMD.numDescriptors = ltob_l(pCmd->CMD_U.TX_CONT_FRAME_BEGIN_CMD.numDescriptors);
 	#endif

	m_txContFrameBegin(pCmd->devNum,
     		      pCmd->CMD_U.TX_CONT_FRAME_BEGIN_CMD.length,
     		      pCmd->CMD_U.TX_CONT_FRAME_BEGIN_CMD.ifswait,
     		      pCmd->CMD_U.TX_CONT_FRAME_BEGIN_CMD.typeOption1,
     		      pCmd->CMD_U.TX_CONT_FRAME_BEGIN_CMD.typeOption2,
     		      pCmd->CMD_U.TX_CONT_FRAME_BEGIN_CMD.antenna,
		      pCmd->CMD_U.TX_CONT_FRAME_BEGIN_CMD.performStabilizePower,
		      pCmd->CMD_U.TX_CONT_FRAME_BEGIN_CMD.numDescriptors,
			  pCmd->CMD_U.TX_CONT_FRAME_BEGIN_CMD.dest);
		     
    	return 0;
}

/**************************************************************************
*/

A_UINT32 processTxContEndCmd
(
	struct dkThreadInfo *dkThread
)
{
	PIPE_CMD      *pCmd;
	CMD_REPLY     *ReplyCmd_p;
	A_UCHAR     *replyBuf;

	pCmd = dkThread->PipeBuffer_p;
	ReplyCmd_p = dkThread->ReplyCmd_p;
	replyBuf = (A_UCHAR *)(ReplyCmd_p->cmdBytes);

	/* check that the size is appropriate before we access */
     	if( pCmd->cmdLen != sizeof(pCmd->cmdID) +sizeof(pCmd->devNum)) 
        {
        	uiPrintf("Error, command Len is not equal to size of TX_CONT_END\n");
        	return(COMMS_ERR_BAD_LENGTH);
        }

	m_txContEnd(pCmd->devNum);

    	return 0;

}

/**************************************************************************
*/

A_UINT32 processSetAntennaCmd
(
	struct dkThreadInfo *dkThread
)
{
	PIPE_CMD      *pCmd;
	CMD_REPLY     *ReplyCmd_p;
	A_UCHAR     *replyBuf;

	pCmd = dkThread->PipeBuffer_p;
	ReplyCmd_p = dkThread->ReplyCmd_p;
	replyBuf = (A_UCHAR *)(ReplyCmd_p->cmdBytes);

	/* check that the size is appropriate before we access */
     	if( pCmd->cmdLen != sizeof(pCmd->CMD_U.SET_ONE_CMD)+sizeof(pCmd->cmdID) +sizeof(pCmd->devNum)) 
        {
        	uiPrintf("Error, command Len is not equal to size of SET_ONE_CMD\n");
        	return(COMMS_ERR_BAD_LENGTH);
        }

	#ifdef ENDIAN_SWAP
     		pCmd->CMD_U.SET_ONE_CMD.param = ltob_l(pCmd->CMD_U.SET_ONE_CMD.param);
 	#endif

	
 	m_setAntenna(pCmd->devNum,pCmd->CMD_U.SET_ONE_CMD.param);

    	return 0;
}

/**************************************************************************
*/

A_UINT32 processSetPowerScaleCmd
(
	struct dkThreadInfo *dkThread
)
{
	PIPE_CMD      *pCmd;
	CMD_REPLY     *ReplyCmd_p;
	A_UCHAR     *replyBuf;

	pCmd = dkThread->PipeBuffer_p;
	ReplyCmd_p = dkThread->ReplyCmd_p;
	replyBuf = (A_UCHAR *)(ReplyCmd_p->cmdBytes);

	/* check that the size is appropriate before we access */
     	if( pCmd->cmdLen != sizeof(pCmd->CMD_U.SET_ONE_CMD)+sizeof(pCmd->cmdID) +sizeof(pCmd->devNum)) 
        {
        	uiPrintf("Error, command Len is not equal to size of SET_ONE_CMD\n");
        	return(COMMS_ERR_BAD_LENGTH);
        }

	#ifdef ENDIAN_SWAP
     		pCmd->CMD_U.SET_ONE_CMD.param = ltob_l(pCmd->CMD_U.SET_ONE_CMD.param);
 	#endif

	
 	m_setPowerScale(pCmd->devNum,pCmd->CMD_U.SET_ONE_CMD.param);

    	return 0;
}

/**************************************************************************
*/

A_UINT32 processSetTransmitPowerCmd
(
	struct dkThreadInfo *dkThread
)
{
	PIPE_CMD      *pCmd;
	CMD_REPLY     *ReplyCmd_p;
	A_UCHAR     *replyBuf;
	PDATABUFFER txPowerArray;

	pCmd = dkThread->PipeBuffer_p;
	ReplyCmd_p = dkThread->ReplyCmd_p;
	replyBuf = (A_UCHAR *)(ReplyCmd_p->cmdBytes);

	/* check that the size is appropriate before we access */
     	if( pCmd->cmdLen != sizeof(pCmd->CMD_U.SET_TRANSMIT_POWER_CMD)+sizeof(pCmd->cmdID) +sizeof(pCmd->devNum)) 
        {
        	uiPrintf("Error, command Len is not equal to size of SET_TRANSMIT_POWER_CMD\n");
        	return(COMMS_ERR_BAD_LENGTH);
        }

	txPowerArray = (PDATABUFFER)A_MALLOC(sizeof(DATABUFFER));
	if (txPowerArray == NULL)
	{
		uiPrintf("Error, unable to allocate memory for txPowerArray DATABUFFER struct \n");
		return (COMMS_ERR_ALLOC_FAIL);	
	}
	txPowerArray->Length = 17;
	txPowerArray->pData = (A_UCHAR *)A_MALLOC(txPowerArray->Length);
	if (txPowerArray->pData == NULL)
	{
		A_FREE(txPowerArray);
		uiPrintf("Error, unable to allocate memory for txPowerArray array \n");
		return (COMMS_ERR_ALLOC_FAIL);	
	}
	memcpy(txPowerArray->pData,pCmd->CMD_U.SET_TRANSMIT_POWER_CMD.txPowerArray,txPowerArray->Length);
	
 	m_setTransmitPower(pCmd->devNum,
			   txPowerArray);

	A_FREE(txPowerArray->pData);
	A_FREE(txPowerArray);

    	return 0;
}

/**************************************************************************
*/

A_UINT32 processSetSingleTransmitPowerCmd
(
	struct dkThreadInfo *dkThread
)
{
	PIPE_CMD      *pCmd;
	CMD_REPLY     *ReplyCmd_p;
	A_UCHAR     *replyBuf;

	pCmd = dkThread->PipeBuffer_p;
	ReplyCmd_p = dkThread->ReplyCmd_p;
	replyBuf = (A_UCHAR *)(ReplyCmd_p->cmdBytes);

	/* check that the size is appropriate before we access */
     	if( pCmd->cmdLen != sizeof(pCmd->CMD_U.SET_ONE_CMD)+sizeof(pCmd->cmdID) +sizeof(pCmd->devNum)) 
        {
        	uiPrintf("Error, command Len is not equal to size of SET_ONE_CMD \n");
        	return(COMMS_ERR_BAD_LENGTH);
        }

	#ifdef ENDIAN_SWAP
     		pCmd->CMD_U.SET_ONE_CMD.param = ltob_l(pCmd->CMD_U.SET_ONE_CMD.param);
 	#endif

	
 	m_setSingleTransmitPower(pCmd->devNum,(A_UCHAR)pCmd->CMD_U.SET_ONE_CMD.param);

    	return 0;
}

A_UINT32 processSpecifySubsystemCmd
(
	struct dkThreadInfo *dkThread
)
{
	PIPE_CMD      *pCmd;
	CMD_REPLY     *ReplyCmd_p;
	A_UCHAR     *replyBuf;

	pCmd = dkThread->PipeBuffer_p;
	ReplyCmd_p = dkThread->ReplyCmd_p;
	replyBuf = (A_UCHAR *)(ReplyCmd_p->cmdBytes);

	/* check that the size is appropriate before we access */
     	if( pCmd->cmdLen != sizeof(pCmd->CMD_U.SET_ONE_CMD)+sizeof(pCmd->cmdID) +sizeof(pCmd->devNum)) 
        {
        	uiPrintf("Error, command Len is not equal to size of SPECIFY_SUBSYSTEM_ID\n");
        	return(COMMS_ERR_BAD_LENGTH);
        }

	#ifdef ENDIAN_SWAP
     		pCmd->CMD_U.SET_ONE_CMD.param = ltob_l(pCmd->CMD_U.SET_ONE_CMD.param);
 	#endif

	
 	m_specifySubSystemID(pCmd->devNum,(A_UCHAR)pCmd->CMD_U.SET_ONE_CMD.param);

    return 0;
}

/**************************************************************************
*/

A_UINT32 processDevSleepCmd
(
	struct dkThreadInfo *dkThread
)
{
	PIPE_CMD      *pCmd;
	CMD_REPLY     *ReplyCmd_p;
	A_UCHAR     *replyBuf;

	pCmd = dkThread->PipeBuffer_p;
	ReplyCmd_p = dkThread->ReplyCmd_p;
	replyBuf = (A_UCHAR *)(ReplyCmd_p->cmdBytes);

	/* check that the size is appropriate before we access */
     	if( pCmd->cmdLen != sizeof(pCmd->cmdID) +sizeof(pCmd->devNum)) 
        {
        	uiPrintf("Error, command Len is not equal to size of DEV_SLEEP_CMD\n");
        	return(COMMS_ERR_BAD_LENGTH);
        }

	m_devSleep(pCmd->devNum);

    	return 0;

}
/**************************************************************************
*/


A_UINT32 processChangeFieldCmd
(
	struct dkThreadInfo *dkThread
)
{
	PIPE_CMD      *pCmd;
	CMD_REPLY     *ReplyCmd_p;
	A_UCHAR     *replyBuf;

	pCmd = dkThread->PipeBuffer_p;
	ReplyCmd_p = dkThread->ReplyCmd_p;
	replyBuf = (A_UCHAR *)(ReplyCmd_p->cmdBytes);

	/* check that the size is appropriate before we access */
     	if( pCmd->cmdLen != sizeof(pCmd->CMD_U.CHANGE_FIELD_CMD)+sizeof(pCmd->cmdID) +sizeof(pCmd->devNum)) 
        {
        	uiPrintf("Error, command Len is not equal to size of CHANGE_FIELD_CMD\n");
        	return(COMMS_ERR_BAD_LENGTH);
        }

	#ifdef ENDIAN_SWAP
     		pCmd->CMD_U.CHANGE_FIELD_CMD.newValue = ltob_l(pCmd->CMD_U.CHANGE_FIELD_CMD.newValue);
 	#endif

	m_changeField(pCmd->devNum,
		      pCmd->CMD_U.CHANGE_FIELD_CMD.fieldName,
		      pCmd->CMD_U.CHANGE_FIELD_CMD.newValue);

    	return 0;

}


A_UINT32 processChangeMultiFieldsAllModes
(
	struct dkThreadInfo *dkThread
)
{
	PIPE_CMD      *pCmd;
	CMD_REPLY     *ReplyCmd_p;
	A_UCHAR     *replyBuf;

	pCmd = dkThread->PipeBuffer_p;
	ReplyCmd_p = dkThread->ReplyCmd_p;
	replyBuf = (A_UCHAR *)(ReplyCmd_p->cmdBytes);


	#ifdef ENDIAN_SWAP
     		pCmd->CMD_U.CHANGE_MULTI_FIELDS_ALL_MODES_CMD.numFields = ltob_l(pCmd->CMD_U.CHANGE_MULTI_FIELDS_ALL_MODES_CMD.numFields);
 	#endif

	changeMultipleFieldsAllModes(pCmd->devNum,
		      pCmd->CMD_U.CHANGE_MULTI_FIELDS_ALL_MODES_CMD.FieldsToChange,
		      pCmd->CMD_U.CHANGE_MULTI_FIELDS_ALL_MODES_CMD.numFields);

    	return 0;

}

A_UINT32 processChangeMultiFields
(
	struct dkThreadInfo *dkThread
)
{
	PIPE_CMD      *pCmd;
	CMD_REPLY     *ReplyCmd_p;
	A_UCHAR     *replyBuf;

	pCmd = dkThread->PipeBuffer_p;
	ReplyCmd_p = dkThread->ReplyCmd_p;
	replyBuf = (A_UCHAR *)(ReplyCmd_p->cmdBytes);

	#ifdef ENDIAN_SWAP
     		pCmd->CMD_U.CHANGE_MULTI_FIELDS_CMD.numFields = ltob_l(pCmd->CMD_U.CHANGE_MULTI_FIELDS_CMD.numFields);
 	#endif

	changeMultipleFields(pCmd->devNum,
		      pCmd->CMD_U.CHANGE_MULTI_FIELDS_CMD.FieldsToChange,
		      pCmd->CMD_U.CHANGE_MULTI_FIELDS_CMD.numFields);
	
    return 0;

}


A_UINT32 processChangeAddacField
(
	struct dkThreadInfo *dkThread
)
{
	PIPE_CMD      *pCmd;
	CMD_REPLY     *ReplyCmd_p;
	A_UCHAR       *replyBuf;

	pCmd = dkThread->PipeBuffer_p;
	ReplyCmd_p = dkThread->ReplyCmd_p;
	replyBuf = (A_UCHAR *)(ReplyCmd_p->cmdBytes);

	changeAddacField(pCmd->devNum, &(pCmd->CMD_U.CHANGE_ADDAC_FIELD_CMD.FieldToChange));
	
    return 0;

}

A_UINT32 processSaveXpaBiasLvlFreq
(
	struct dkThreadInfo *dkThread
)
{
	PIPE_CMD      *pCmd;
	CMD_REPLY     *ReplyCmd_p;
	A_UCHAR       *replyBuf;

	pCmd = dkThread->PipeBuffer_p;
	ReplyCmd_p = dkThread->ReplyCmd_p;
	replyBuf = (A_UCHAR *)(ReplyCmd_p->cmdBytes);

	saveXpaBiasLvlFreq(pCmd->devNum, &(pCmd->CMD_U.SAVE_XPA_BIAS_FREQ_CMD.FieldToChange), pCmd->CMD_U.SAVE_XPA_BIAS_FREQ_CMD.biasLevel);
	
    return 0;

}


A_UINT32 processEnableWepCmd
(
	struct dkThreadInfo *dkThread
)
{
	PIPE_CMD      *pCmd;
	CMD_REPLY     *ReplyCmd_p;
	A_UCHAR     *replyBuf;

	pCmd = dkThread->PipeBuffer_p;
	ReplyCmd_p = dkThread->ReplyCmd_p;
	replyBuf = (A_UCHAR *)(ReplyCmd_p->cmdBytes);

	/* check that the size is appropriate before we access */
     	if( pCmd->cmdLen != sizeof(pCmd->CMD_U.ENABLE_WEP_CMD)+sizeof(pCmd->cmdID) +sizeof(pCmd->devNum)) 
        {
        	uiPrintf("Error, command Len is not equal to size of ENABLE_WEP_CMD\n");
        	return(COMMS_ERR_BAD_LENGTH);
        }

	m_enableWep(pCmd->devNum,
		      pCmd->CMD_U.ENABLE_WEP_CMD.key);


    	return 0;


}
	
A_UINT32 processEnablePAPreDistCmd
(
	struct dkThreadInfo *dkThread
)
{
	PIPE_CMD      *pCmd;
	CMD_REPLY     *ReplyCmd_p;
	A_UCHAR     *replyBuf;

	pCmd = dkThread->PipeBuffer_p;
	ReplyCmd_p = dkThread->ReplyCmd_p;
	replyBuf = (A_UCHAR *)(ReplyCmd_p->cmdBytes);

	/* check that the size is appropriate before we access */
     	if( pCmd->cmdLen != sizeof(pCmd->CMD_U.ENABLE_PA_PRE_DIST_CMD)+sizeof(pCmd->cmdID) +sizeof(pCmd->devNum)) 
        {
        	uiPrintf("Error, command Len is not equal to size of ENABLE_PA_PRE_DIST_CMD\n");
        	return(COMMS_ERR_BAD_LENGTH);
        }

	#ifdef ENDIAN_SWAP
		pCmd->CMD_U.ENABLE_PA_PRE_DIST_CMD.rate = ltob_s(pCmd->CMD_U.ENABLE_PA_PRE_DIST_CMD.rate);
		pCmd->CMD_U.ENABLE_PA_PRE_DIST_CMD.power = ltob_l(pCmd->CMD_U.ENABLE_PA_PRE_DIST_CMD.power);
 	#endif

	m_enablePAPreDist(pCmd->devNum,
			  pCmd->CMD_U.ENABLE_PA_PRE_DIST_CMD.rate,
			  pCmd->CMD_U.ENABLE_PA_PRE_DIST_CMD.power);

    	return 0;

}


A_UINT32 processDumpRegsCmd
(
	struct dkThreadInfo *dkThread
)
{
	PIPE_CMD      *pCmd;
	CMD_REPLY     *ReplyCmd_p;
	A_UCHAR     *replyBuf;

	pCmd = dkThread->PipeBuffer_p;
	ReplyCmd_p = dkThread->ReplyCmd_p;
	replyBuf = (A_UCHAR *)(ReplyCmd_p->cmdBytes);

	/* check that the size is appropriate before we access */
     	if( pCmd->cmdLen != sizeof(pCmd->cmdID) +sizeof(pCmd->devNum)) 
        {
        	uiPrintf("Error, command Len is not equal to size of DUMP_REGS cmd\n");
        	return(COMMS_ERR_BAD_LENGTH);
        }


	m_dumpRegs(pCmd->devNum);

    	return 0;

}


A_UINT32 processDumpPciWritesCmd
(
	struct dkThreadInfo *dkThread
)
{
	PIPE_CMD      *pCmd;
	CMD_REPLY     *ReplyCmd_p;
	A_UCHAR     *replyBuf;

	pCmd = dkThread->PipeBuffer_p;
	ReplyCmd_p = dkThread->ReplyCmd_p;
	replyBuf = (A_UCHAR *)(ReplyCmd_p->cmdBytes);

	/* check that the size is appropriate before we access */
     	if( pCmd->cmdLen != sizeof(pCmd->cmdID) +sizeof(pCmd->devNum)) 
        {
        	uiPrintf("Error, command Len is not equal to size of DUMP_PCI_WRITES cmd\n");
        	return(COMMS_ERR_BAD_LENGTH);
        }

	m_dumpPciWrites(pCmd->devNum);

    	return 0;
}



A_UINT32 processTestLibCmd
(
	struct dkThreadInfo *dkThread
)
{
	PIPE_CMD      *pCmd;
	CMD_REPLY     *ReplyCmd_p;
	A_UCHAR     *replyBuf;

	pCmd = dkThread->PipeBuffer_p;
	ReplyCmd_p = dkThread->ReplyCmd_p;
	replyBuf = (A_UCHAR *)(ReplyCmd_p->cmdBytes);

	/* check that the size is appropriate before we access */
     	if( pCmd->cmdLen != sizeof(pCmd->CMD_U.SET_ONE_CMD)+sizeof(pCmd->cmdID) +sizeof(pCmd->devNum)) 
        {
        	uiPrintf("Error, command Len is not equal to size of SET_ONE_CMD\n");
        	return(COMMS_ERR_BAD_LENGTH);
        }

	#ifdef ENDIAN_SWAP
     		pCmd->CMD_U.SET_ONE_CMD.param = ltob_l(pCmd->CMD_U.SET_ONE_CMD.param);
 	#endif

	
 	*((A_UINT32 *)replyBuf) = (A_UINT32) m_testLib(pCmd->devNum,pCmd->CMD_U.SET_ONE_CMD.param);

#ifdef ENDIAN_SWAP
    	*((A_UINT32 *)replyBuf) = btol_l(*((A_UINT32 *)replyBuf));
#endif

    	return 0;
}


A_UINT32 processDisplayFieldValuesCmd
(
	struct dkThreadInfo *dkThread
)
{
	PIPE_CMD      *pCmd;
	CMD_REPLY     *ReplyCmd_p;
	A_UCHAR     *replyBuf;
	A_UINT32 baseValue=0;
	A_UINT32 turboValue=0;

	pCmd = dkThread->PipeBuffer_p;
	ReplyCmd_p = dkThread->ReplyCmd_p;
	replyBuf = (A_UCHAR *)(ReplyCmd_p->cmdBytes);

	/* check that the size is appropriate before we access */
     	if( pCmd->cmdLen != sizeof(pCmd->CMD_U.DISPLAY_FIELD_VALUES_CMD)+sizeof(pCmd->cmdID) +sizeof(pCmd->devNum)) 
        {
        	uiPrintf("Error, command Len is not equal to size of DISPLAY_FIELD_VALUES \n");
        	return(COMMS_ERR_BAD_LENGTH);
        }

	m_displayFieldValues(pCmd->devNum,
		             pCmd->CMD_U.DISPLAY_FIELD_VALUES_CMD.fieldName,
			     &baseValue,
			     &turboValue);

    	*(A_UINT32 *)replyBuf = baseValue;
    	*(A_UINT32 *)(replyBuf+4) = turboValue;

	#ifdef ENDIAN_SWAP
    		*((A_UINT32 *)replyBuf) = btol_l(*((A_UINT32 *)replyBuf));
		*((A_UINT32 *)(replyBuf+4)) = btol_l(*((A_UINT32 *)(replyBuf+4)));
	#endif

    	return 0;
}

A_UINT32 processGetFieldForModeCmd
(
	struct dkThreadInfo *dkThread
)
{
	PIPE_CMD      *pCmd;
	CMD_REPLY     *ReplyCmd_p;
	A_UCHAR     *replyBuf;
	A_INT32 returnValue=0;

	pCmd = dkThread->PipeBuffer_p;
	ReplyCmd_p = dkThread->ReplyCmd_p;
	replyBuf = (A_UCHAR *)(ReplyCmd_p->cmdBytes);


	/* check that the size is appropriate before we access */
     	if( pCmd->cmdLen != sizeof(pCmd->CMD_U.GET_FIELD_FOR_MODE_CMD)+sizeof(pCmd->cmdID) +sizeof(pCmd->devNum)) 
        {
        	uiPrintf("Error, command Len is not equal to size of GET_FIELD_FOR_MODE \n");
        	return(COMMS_ERR_BAD_LENGTH);
        }

	#ifdef ENDIAN_SWAP
     		pCmd->CMD_U.GET_FIELD_FOR_MODE_CMD.turbo = ltob_l(pCmd->CMD_U.GET_FIELD_FOR_MODE_CMD.turbo);
     		pCmd->CMD_U.GET_FIELD_FOR_MODE_CMD.mode  = ltob_l(pCmd->CMD_U.GET_FIELD_FOR_MODE_CMD.mode);
 	#endif

	returnValue = m_getFieldForMode(pCmd->devNum,
		        pCmd->CMD_U.GET_FIELD_FOR_MODE_CMD.fieldName,
		        pCmd->CMD_U.GET_FIELD_FOR_MODE_CMD.mode,
				pCmd->CMD_U.GET_FIELD_FOR_MODE_CMD.turbo);


    	*(A_INT32 *)replyBuf = returnValue;

	#ifdef ENDIAN_SWAP
    	*((A_UINT32 *)replyBuf) = btol_l(*((A_UINT32 *)replyBuf));
	#endif

    	return 0;
}


A_UINT32 processGetMaxPowerCmd
(
	struct dkThreadInfo *dkThread
)
{
	PIPE_CMD      *pCmd;
	CMD_REPLY     *ReplyCmd_p;
	A_UCHAR     *replyBuf;
	A_UINT16 returnValue=0;

	pCmd = dkThread->PipeBuffer_p;
	ReplyCmd_p = dkThread->ReplyCmd_p;
	replyBuf = (A_UCHAR *)(ReplyCmd_p->cmdBytes);


	/* check that the size is appropriate before we access */
    if( pCmd->cmdLen != sizeof(pCmd->CMD_U.GET_MAX_POWER_CMD)+sizeof(pCmd->cmdID) +sizeof(pCmd->devNum)) 
    {
        uiPrintf("Error, command Len is not equal to size of GET_MAX_POWER_CMD \n");
        return(COMMS_ERR_BAD_LENGTH);
    }

	#ifdef ENDIAN_SWAP
     	pCmd->CMD_U.GET_MAX_POWER_CMD.freq = ltob_l(pCmd->CMD_U.GET_MAX_POWER_CMD.freq);
     	pCmd->CMD_U.GET_MAX_POWER_CMD.rate  = ltob_l(pCmd->CMD_U.GET_MAX_POWER_CMD.rate);
 	#endif

	returnValue = getMaxPowerForRate(pCmd->devNum,
		        pCmd->CMD_U.GET_MAX_POWER_CMD.freq,
		        pCmd->CMD_U.GET_MAX_POWER_CMD.rate);


    *(A_UINT16 *)replyBuf = returnValue;

	#ifdef ENDIAN_SWAP
    	*((A_UINT16 *)replyBuf) = btol_s(*((A_UINT16 *)replyBuf));
	#endif

    return 0;
}

A_UINT32 processGetXpdgainCmd
(
	struct dkThreadInfo *dkThread
)
{
	PIPE_CMD      *pCmd;
	CMD_REPLY     *ReplyCmd_p;
	A_UCHAR     *replyBuf;
	A_UINT16 returnValue=0;

	pCmd = dkThread->PipeBuffer_p;
	ReplyCmd_p = dkThread->ReplyCmd_p;
	replyBuf = (A_UCHAR *)(ReplyCmd_p->cmdBytes);


	/* check that the size is appropriate before we access */
    if( pCmd->cmdLen != sizeof(pCmd->CMD_U.GET_XPDGAIN_CMD)+sizeof(pCmd->cmdID) +sizeof(pCmd->devNum)) 
    {
        uiPrintf("Error, command Len is not equal to size of GET_XPDGAIN_CMD \n");
        return(COMMS_ERR_BAD_LENGTH);
    }

	#ifdef ENDIAN_SWAP
     	pCmd->CMD_U.GET_XPDGAIN_CMD.power = ltob_l(pCmd->CMD_U.GET_XPDGAIN_CMD.power);
 	#endif

	returnValue = getXpdgainForPower(pCmd->devNum,
		        pCmd->CMD_U.GET_XPDGAIN_CMD.power);


    *(A_UINT16 *)replyBuf = returnValue;

	#ifdef ENDIAN_SWAP
    	*((A_UINT16 *)replyBuf) = btol_s(*((A_UINT16 *)replyBuf));
	#endif

    return 0;
}

A_UINT32 processGetPcdacForPowerCmd
(
	struct dkThreadInfo *dkThread
)
{
	PIPE_CMD      *pCmd;
	CMD_REPLY     *ReplyCmd_p;
	A_UCHAR     *replyBuf;
	A_UINT16 returnValue=0;

	pCmd = dkThread->PipeBuffer_p;
	ReplyCmd_p = dkThread->ReplyCmd_p;
	replyBuf = (A_UCHAR *)(ReplyCmd_p->cmdBytes);


	/* check that the size is appropriate before we access */
    if( pCmd->cmdLen != sizeof(pCmd->CMD_U.GET_PCDAC_FOR_POWER_CMD)+sizeof(pCmd->cmdID) +sizeof(pCmd->devNum)) 
    {
        uiPrintf("Error, command Len is not equal to size of GET_PCDAC_FOR_POWER_CMD \n");
        return(COMMS_ERR_BAD_LENGTH);
    }

	#ifdef ENDIAN_SWAP
     	pCmd->CMD_U.GET_PCDAC_FOR_POWER_CMD.freq = ltob_l(pCmd->CMD_U.GET_PCDAC_FOR_POWER_CMD.freq);
     	pCmd->CMD_U.GET_PCDAC_FOR_POWER_CMD.power  = ltob_l(pCmd->CMD_U.GET_PCDAC_FOR_POWER_CMD.power);
 	#endif

	returnValue = getPcdacForPower(pCmd->devNum,
		        pCmd->CMD_U.GET_PCDAC_FOR_POWER_CMD.freq,
		        pCmd->CMD_U.GET_PCDAC_FOR_POWER_CMD.power);


    *(A_UINT16 *)replyBuf = returnValue;

	#ifdef ENDIAN_SWAP
    	*((A_UINT16 *)replyBuf) = btol_s(*((A_UINT16 *)replyBuf));
	#endif

    return 0;
}

A_UINT32 processGetPowerIndexCmd
(
	struct dkThreadInfo *dkThread
)
{
	PIPE_CMD      *pCmd;
	CMD_REPLY     *ReplyCmd_p;
	A_UCHAR		  *replyBuf;
	A_UINT16	   returnValue = 0;

	pCmd = dkThread->PipeBuffer_p;
	ReplyCmd_p = dkThread->ReplyCmd_p;
	replyBuf = (A_UCHAR *)(ReplyCmd_p->cmdBytes);


	/* check that the size is appropriate before we access */
    if( pCmd->cmdLen != sizeof(pCmd->CMD_U.GET_POWER_INDEX_CMD)+sizeof(pCmd->cmdID) +sizeof(pCmd->devNum)) 
    {
        uiPrintf("Error, command Len is not equal to size of GET_POWER_INDEX_CMD \n");
        return(COMMS_ERR_BAD_LENGTH);
    }

	#ifdef ENDIAN_SWAP
     	pCmd->CMD_U.GET_POWER_INDEX_CMD.power  = ltob_l(pCmd->CMD_U.GET_POWER_INDEX_CMD.power);
 	#endif

	returnValue = getPowerIndex(pCmd->devNum,
								   pCmd->CMD_U.GET_POWER_INDEX_CMD.power);


    *(A_UINT16 *)replyBuf = returnValue;

	#ifdef ENDIAN_SWAP
    	*((A_UINT16 *)replyBuf) = btol_s(*((A_UINT16 *)replyBuf));
	#endif

    return 0;
}

A_UINT32 processGetFieldValueCmd
(
	struct dkThreadInfo *dkThread
)
{
	PIPE_CMD      *pCmd;
	CMD_REPLY     *ReplyCmd_p;
	A_UCHAR     *replyBuf;
	A_UINT32 baseValue=0;
	A_UINT32 turboValue=0;

	pCmd = dkThread->PipeBuffer_p;
	ReplyCmd_p = dkThread->ReplyCmd_p;
	replyBuf = (A_UCHAR *)(ReplyCmd_p->cmdBytes);


	/* check that the size is appropriate before we access */
     	if( pCmd->cmdLen != sizeof(pCmd->CMD_U.GET_FIELD_VALUE_CMD)+sizeof(pCmd->cmdID) +sizeof(pCmd->devNum)) 
        {
        	uiPrintf("Error, command Len is not equal to size of GET_FIELD_VALUE \n");
        	return(COMMS_ERR_BAD_LENGTH);
        }

	#ifdef ENDIAN_SWAP
     		pCmd->CMD_U.GET_FIELD_VALUE_CMD.turbo = ltob_l(pCmd->CMD_U.GET_FIELD_VALUE_CMD.turbo);
 	#endif

	m_getFieldValue(pCmd->devNum,
		        pCmd->CMD_U.GET_FIELD_VALUE_CMD.fieldName,
		        pCmd->CMD_U.GET_FIELD_VALUE_CMD.turbo,
			&baseValue,
			&turboValue);


    	*(A_UINT32 *)replyBuf = baseValue;
    	*(A_UINT32 *)(replyBuf+4) = turboValue;

	#ifdef ENDIAN_SWAP
    		*((A_UINT32 *)replyBuf) = btol_l(*((A_UINT32 *)replyBuf));
		*((A_UINT32 *)(replyBuf+4)) = btol_l(*((A_UINT32 *)(replyBuf+4)));
	#endif

    	return 0;
}

A_UINT32 processGetArtAniLevel
(
	struct dkThreadInfo *dkThread
)
{
	PIPE_CMD		*pCmd;
	CMD_REPLY		*ReplyCmd_p;
	A_UCHAR			*replyBuf;
	A_UINT32		returnValue = 0;

	pCmd = dkThread->PipeBuffer_p;
	ReplyCmd_p = dkThread->ReplyCmd_p;
	replyBuf = (A_UCHAR *)(ReplyCmd_p->cmdBytes);


	/* check that the size is appropriate before we access */
    if( pCmd->cmdLen != sizeof(pCmd->CMD_U.GET_ART_ANI_LEVEL_CMD)+sizeof(pCmd->cmdID) +sizeof(pCmd->devNum)) 
    {
        uiPrintf("Error, command Len is not equal to size of GET_ART_ANI_LEVEL_CMD \n");
        return(COMMS_ERR_BAD_LENGTH);
    }

	#ifdef ENDIAN_SWAP
     		pCmd->CMD_U.GET_ART_ANI_LEVEL_CMD.artAniType = ltob_l(pCmd->CMD_U.GET_ART_ANI_LEVEL_CMD.artAniType);
 	#endif

	returnValue = getArtAniLevel(pCmd->devNum,
					pCmd->CMD_U.GET_ART_ANI_LEVEL_CMD.artAniType);


   	*(A_UINT32 *)replyBuf = returnValue;

	#ifdef ENDIAN_SWAP
   		*((A_UINT32 *)replyBuf) = btol_l(*((A_UINT32 *)replyBuf));
	#endif

   	return 0;

}

A_UINT32 processSetArtAniLevel
(
	struct dkThreadInfo *dkThread
)
{
	PIPE_CMD		*pCmd;
	CMD_REPLY		*ReplyCmd_p;
	A_UCHAR			*replyBuf;

	pCmd = dkThread->PipeBuffer_p;
	ReplyCmd_p = dkThread->ReplyCmd_p;
	replyBuf = (A_UCHAR *)(ReplyCmd_p->cmdBytes);


	/* check that the size is appropriate before we access */
    if( pCmd->cmdLen != sizeof(pCmd->CMD_U.SET_ART_ANI_LEVEL_CMD)+sizeof(pCmd->cmdID) +sizeof(pCmd->devNum)) 
    {
        uiPrintf("Error, command Len is not equal to size of SET_ART_ANI_LEVEL_CMD \n");
        return(COMMS_ERR_BAD_LENGTH);
    }

	#ifdef ENDIAN_SWAP
     		pCmd->CMD_U.SET_ART_ANI_LEVEL_CMD.artAniType = ltob_l(pCmd->CMD_U.SET_ART_ANI_LEVEL_CMD.artAniType);
			pCmd->CMD_U.SET_ART_ANI_LEVEL_CMD.artAniLevel = ltob_l(pCmd->CMD_U.SET_ART_ANI_LEVEL_CMD.artAniLevel);
 	#endif

	setArtAniLevel(pCmd->devNum,
						pCmd->CMD_U.SET_ART_ANI_LEVEL_CMD.artAniType,
						pCmd->CMD_U.SET_ART_ANI_LEVEL_CMD.artAniLevel);


   	*(A_UINT32 *)replyBuf = 0;

	#ifdef ENDIAN_SWAP
   		*((A_UINT32 *)replyBuf) = btol_l(*((A_UINT32 *)replyBuf));
	#endif

   	return 0;

}


A_UINT32 processReadFieldCmd
(
	struct dkThreadInfo *dkThread
)
{
	PIPE_CMD      *pCmd;
	CMD_REPLY     *ReplyCmd_p;
	A_UCHAR     *replyBuf;
	A_UINT32 unsignedValue;
	A_INT32 signedValue;
	A_BOOL   signedFlag;

	pCmd = dkThread->PipeBuffer_p;
	ReplyCmd_p = dkThread->ReplyCmd_p;
	replyBuf = (A_UCHAR *)(ReplyCmd_p->cmdBytes);


	/* check that the size is appropriate before we access */
     	if( pCmd->cmdLen != sizeof(pCmd->CMD_U.READ_FIELD_CMD)+sizeof(pCmd->cmdID) +sizeof(pCmd->devNum)) 
        {
        	uiPrintf("Error, command Len is not equal to size of READ_FIELD \n");
        	return(COMMS_ERR_BAD_LENGTH);
        }

	#ifdef ENDIAN_SWAP
     		pCmd->CMD_U.READ_FIELD_CMD.printValue = ltob_l(pCmd->CMD_U.READ_FIELD_CMD.printValue);
 	#endif

	m_readField(pCmd->devNum,
		        pCmd->CMD_U.READ_FIELD_CMD.fieldName,
			pCmd->CMD_U.READ_FIELD_CMD.printValue,
			&unsignedValue,
			&signedValue,
			&signedFlag);


    	*(A_UINT32 *)replyBuf = unsignedValue;
    	*(A_UINT32 *)(replyBuf+4) = signedValue;
	*(A_BOOL *)(replyBuf+8) = signedFlag;

	#ifdef ENDIAN_SWAP
    		*((A_UINT32 *)replyBuf) = btol_l(*((A_UINT32 *)replyBuf));
		*((A_UINT32 *)(replyBuf+4)) = btol_l(*((A_UINT32 *)(replyBuf+4)));
	#endif

    	return 0;

}


A_UINT32 processWriteFieldCmd
(
	struct dkThreadInfo *dkThread
)
{
	PIPE_CMD      *pCmd;
	CMD_REPLY     *ReplyCmd_p;
	A_UCHAR     *replyBuf;

	pCmd = dkThread->PipeBuffer_p;
	ReplyCmd_p = dkThread->ReplyCmd_p;
	replyBuf = (A_UCHAR *)(ReplyCmd_p->cmdBytes);


	/* check that the size is appropriate before we access */
     	if( pCmd->cmdLen != sizeof(pCmd->CMD_U.WRITE_FIELD_CMD)+sizeof(pCmd->cmdID) +sizeof(pCmd->devNum)) 
        {
        	uiPrintf("Error, command Len is not equal to size of WRITE_FIELD \n");
        	return(COMMS_ERR_BAD_LENGTH);
        }

	#ifdef ENDIAN_SWAP
     		pCmd->CMD_U.WRITE_FIELD_CMD.newValue = ltob_l(pCmd->CMD_U.WRITE_FIELD_CMD.newValue);
 	#endif


	m_writeField(pCmd->devNum,
		     pCmd->CMD_U.WRITE_FIELD_CMD.fieldName,
		     pCmd->CMD_U.WRITE_FIELD_CMD.newValue);

			

    	return 0;
}

A_UINT32 processGetMacAddr
(
	struct dkThreadInfo *dkThread
)
{
	PIPE_CMD      *pCmd;
	CMD_REPLY     *ReplyCmd_p;
	A_UCHAR     *replyBuf;

	pCmd = dkThread->PipeBuffer_p;
	ReplyCmd_p = dkThread->ReplyCmd_p;
	replyBuf = (A_UCHAR *)(ReplyCmd_p->cmdBytes);

	/* check that the size is appropriate before we access */
     	if( pCmd->cmdLen != sizeof(pCmd->CMD_U.GET_MAC_ADDR_CMD)+sizeof(pCmd->cmdID) +sizeof(pCmd->devNum)) 
        {
        	uiPrintf("Error, command Len is not equal to size of GET_FIELD_FOR_MODE \n");
        	return(COMMS_ERR_BAD_LENGTH);
        }

	#ifdef ENDIAN_SWAP
     		pCmd->CMD_U.GET_MAC_ADDR_CMD.wmac = ltob_s(pCmd->CMD_U.GET_MAC_ADDR_CMD.wmac);
     		pCmd->CMD_U.GET_MAC_ADDR_CMD.instNo  = ltob_s(pCmd->CMD_U.GET_MAC_ADDR_CMD.instNo);
 	#endif

	m_getMacAddr(pCmd->devNum,
		      pCmd->CMD_U.GET_MAC_ADDR_CMD.wmac,
		      pCmd->CMD_U.GET_MAC_ADDR_CMD.instNo,
		      (A_UINT8 *)replyBuf);

	return 0;
}


A_UINT32 processSetResetParamsCmd
(
	struct dkThreadInfo *dkThread
)
{
	PIPE_CMD      *pCmd;
	CMD_REPLY     *ReplyCmd_p;
	A_UCHAR     *replyBuf;

	pCmd = dkThread->PipeBuffer_p;
	ReplyCmd_p = dkThread->ReplyCmd_p;
	replyBuf = (A_UCHAR *)(ReplyCmd_p->cmdBytes);

	/* check that the size is appropriate before we access */
     	if( pCmd->cmdLen != sizeof(pCmd->CMD_U.SET_RESET_PARAMS_CMD)+sizeof(pCmd->cmdID) +sizeof(pCmd->devNum)) 
        {
        	uiPrintf("Error, command Len is not equal to size of SET_RESET_PARAMS \n");
        	return(COMMS_ERR_BAD_LENGTH);
        }

	#ifdef ENDIAN_SWAP
     		pCmd->CMD_U.SET_RESET_PARAMS_CMD.eePromLoad = ltob_l(pCmd->CMD_U.SET_RESET_PARAMS_CMD.eePromLoad);
     		pCmd->CMD_U.SET_RESET_PARAMS_CMD.forceCfgLoad = ltob_l(pCmd->CMD_U.SET_RESET_PARAMS_CMD.forceCfgLoad);
     		pCmd->CMD_U.SET_RESET_PARAMS_CMD.mode = ltob_l(pCmd->CMD_U.SET_RESET_PARAMS_CMD.mode); 	
     		pCmd->CMD_U.SET_RESET_PARAMS_CMD.use_init = ltob_l(pCmd->CMD_U.SET_RESET_PARAMS_CMD.use_init); 	
	#endif

	m_setResetParams(pCmd->devNum,
		     	pCmd->CMD_U.SET_RESET_PARAMS_CMD.fileName,
		     	pCmd->CMD_U.SET_RESET_PARAMS_CMD.eePromLoad,
				pCmd->CMD_U.SET_RESET_PARAMS_CMD.forceCfgLoad,
				pCmd->CMD_U.SET_RESET_PARAMS_CMD.mode,
				pCmd->CMD_U.SET_RESET_PARAMS_CMD.use_init);
			

    	return 0;
}




A_UINT32 processForceSinglePCDACTableCmd
(
	struct dkThreadInfo *dkThread
)
{
	PIPE_CMD      *pCmd;
	CMD_REPLY     *ReplyCmd_p;
	A_UCHAR     *replyBuf;

	pCmd = dkThread->PipeBuffer_p;
	ReplyCmd_p = dkThread->ReplyCmd_p;
	replyBuf = (A_UCHAR *)(ReplyCmd_p->cmdBytes);

	/* check that the size is appropriate before we access */
     	if( pCmd->cmdLen != sizeof(pCmd->CMD_U.FORCE_SINGLE_PCDAC_TABLE_CMD.pcdac)+sizeof(pCmd->cmdID) +sizeof(pCmd->devNum)) 
        {
        	uiPrintf("Error, command Len is not equal to size of FORCE_SINGLE_PCDAC_TABLE \n");
        	return(COMMS_ERR_BAD_LENGTH);
        }

	#ifdef ENDIAN_SWAP
     		pCmd->CMD_U.FORCE_SINGLE_PCDAC_TABLE_CMD.pcdac = ltob_s(pCmd->CMD_U.FORCE_SINGLE_PCDAC_TABLE_CMD.pcdac);
 	#endif

	m_forceSinglePCDACTable(pCmd->devNum,pCmd->CMD_U.FORCE_SINGLE_PCDAC_TABLE_CMD.pcdac);

    	return 0;
}

A_UINT32 processForceSinglePCDACTableGriffinCmd
(
	struct dkThreadInfo *dkThread
)
{
	PIPE_CMD      *pCmd;
	CMD_REPLY     *ReplyCmd_p;
	A_UCHAR     *replyBuf;

	pCmd = dkThread->PipeBuffer_p;
	ReplyCmd_p = dkThread->ReplyCmd_p;
	replyBuf = (A_UCHAR *)(ReplyCmd_p->cmdBytes);

	/* check that the size is appropriate before we access */
     	if( pCmd->cmdLen != sizeof(pCmd->CMD_U.FORCE_SINGLE_PCDAC_TABLE_GRIFFIN_CMD)+sizeof(pCmd->cmdID) +sizeof(pCmd->devNum)) 
        {
        	uiPrintf("Error, command Len is not equal to size of FORCE_SINGLE_PCDAC_TABLE_GRIFFIN \n");
        	return(COMMS_ERR_BAD_LENGTH);
        }

	#ifdef ENDIAN_SWAP
     		pCmd->CMD_U.FORCE_SINGLE_PCDAC_TABLE_GRIFFIN_CMD.pcdac = ltob_s(pCmd->CMD_U.FORCE_SINGLE_PCDAC_TABLE_GRIFFIN_CMD.pcdac);
			pCmd->CMD_U.FORCE_SINGLE_PCDAC_TABLE_GRIFFIN_CMD.offset = ltob_s(pCmd->CMD_U.FORCE_SINGLE_PCDAC_TABLE_GRIFFIN_CMD.offset);

 	#endif

	ForceSinglePCDACTableGriffin(pCmd->devNum,pCmd->CMD_U.FORCE_SINGLE_PCDAC_TABLE_GRIFFIN_CMD.pcdac,
		                         pCmd->CMD_U.FORCE_SINGLE_PCDAC_TABLE_GRIFFIN_CMD.offset);

    	return 0;
}


A_UINT32 processForcePCDACTableCmd
(
	struct dkThreadInfo *dkThread
)
{
	PIPE_CMD      *pCmd;
	CMD_REPLY     *ReplyCmd_p;
	A_UCHAR     *replyBuf;
#ifdef ENDIAN_SWAP
	A_UINT16  i;
#endif

	pCmd = dkThread->PipeBuffer_p;
	ReplyCmd_p = dkThread->ReplyCmd_p;
	replyBuf = (A_UCHAR *)(ReplyCmd_p->cmdBytes);


	/* check that the size is appropriate before we access */
    if( pCmd->cmdLen != sizeof(pCmd->CMD_U.FORCE_PCDAC_TABLE_CMD)+sizeof(pCmd->cmdID) +sizeof(pCmd->devNum)) 
    {
        uiPrintf("Error, command Len is not equal to size of FORCE_PCDAC_TABLE \n");
        return(COMMS_ERR_BAD_LENGTH);
    }

	#ifdef ENDIAN_SWAP
		for(i = 0; i < MAX_PCDACS; i++) {
			pCmd->CMD_U.FORCE_PCDAC_TABLE_CMD.pcdac[i] = ltob_s(pCmd->CMD_U.FORCE_PCDAC_TABLE_CMD.pcdac[i]);
		}
 	#endif

	m_forcePCDACTable(pCmd->devNum,pCmd->CMD_U.FORCE_PCDAC_TABLE_CMD.pcdac);
    return 0;
}

A_UINT32 processPAOffsetCalCmd
(
	struct dkThreadInfo *dkThread
)
{
	PIPE_CMD      *pCmd;
	CMD_REPLY     *ReplyCmd_p;
	A_UCHAR     *replyBuf;
#ifdef ENDIAN_SWAP
	A_UINT16  i;
#endif

	pCmd = dkThread->PipeBuffer_p;
	ReplyCmd_p = dkThread->ReplyCmd_p;
	replyBuf = (A_UCHAR *)(ReplyCmd_p->cmdBytes);


	/* check that the size is appropriate before we access */
    if( pCmd->cmdLen != sizeof(pCmd->CMD_U.PA_OFFSET_CAL_CMD_ID)+sizeof(pCmd->cmdID) +sizeof(pCmd->devNum)) 
    {
        uiPrintf("Error, command Len is not equal to size of PA_OFFSET_CAL \n");
        return(COMMS_ERR_BAD_LENGTH);
    }

//	#ifdef ENDIAN_SWAP
//		for(i = 0; i < MAX_PCDACS; i++) {
//			pCmd->CMD_U.FORCE_PCDAC_TABLE_CMD.pcdac[i] = ltob_s(pCmd->CMD_U.FORCE_PCDAC_TABLE_CMD.pcdac[i]);
//		}
//	#endif

	m_PAOffsetCal(pCmd->devNum);
    return 0;
}

/**************************************************************************
*/

A_UINT32 processForcePowerTxMaxCmd
(
	struct dkThreadInfo *dkThread
)
{
	PIPE_CMD      *pCmd;
	CMD_REPLY     *ReplyCmd_p;
	A_UCHAR     *replyBuf;
#ifdef ENDAIN_SWAP
	A_UINT32 i;
#endif

	pCmd = dkThread->PipeBuffer_p;
	ReplyCmd_p = dkThread->ReplyCmd_p;
	replyBuf = (A_UCHAR *)(ReplyCmd_p->cmdBytes);


	#ifdef ENDIAN_SWAP
     		pCmd->CMD_U.FORCE_POWER_TX_MAX_CMD.length = ltob_l(pCmd->CMD_U.FORCE_POWER_TX_MAX_CMD.length); 
	#endif

    	if( pCmd->cmdLen != (sizeof(pCmd->cmdID) + 
		       sizeof(pCmd->CMD_U.FORCE_POWER_TX_MAX_CMD)-
		       ((MAX_BLOCK_SWORDS - pCmd->CMD_U.FORCE_POWER_TX_MAX_CMD.length) * 2)+sizeof(pCmd->devNum)))

        {
        	uiPrintf("Error: command Len is not equal to size of expected bytes for FORCE_POWER_TX_MAX_CMD\n");
		uiPrintf(": got %d, expected %d\n", pCmd->cmdLen, (sizeof(pCmd->cmdID) + sizeof(pCmd->CMD_U.FORCE_POWER_TX_MAX_CMD)
				- ((MAX_BLOCK_SWORDS - pCmd->CMD_U.FORCE_POWER_TX_MAX_CMD.length) * 2)));
        	return(COMMS_ERR_BAD_LENGTH);
        }


	#ifdef ENDIAN_SWAP
	{
		A_UINT32 i;

 	
		for (i=0;i<pCmd->CMD_U.FORCE_POWER_TX_MAX_CMD.length;i++)
		{
			pCmd->CMD_U.FORCE_POWER_TX_MAX_CMD.ratesPower[i] = ltob_s(pCmd->CMD_U.FORCE_POWER_TX_MAX_CMD.ratesPower[i]); 
		}
	}
	#endif

    	m_forcePowerTxMax(pCmd->devNum,
			  pCmd->CMD_U.FORCE_POWER_TX_MAX_CMD.length,
			  pCmd->CMD_U.FORCE_POWER_TX_MAX_CMD.ratesPower); 

    	return 0;
}

/**************************************************************************
*/


A_UINT32 processForceSinglePowerTxMaxCmd
(
	struct dkThreadInfo *dkThread
)
{
	PIPE_CMD      *pCmd;
	CMD_REPLY     *ReplyCmd_p;
	A_UCHAR     *replyBuf;

	pCmd = dkThread->PipeBuffer_p;
	ReplyCmd_p = dkThread->ReplyCmd_p;
	replyBuf = (A_UCHAR *)(ReplyCmd_p->cmdBytes);


	/* check that the size is appropriate before we access */
     	if( pCmd->cmdLen != sizeof(pCmd->CMD_U.SET_ONE_CMD)+sizeof(pCmd->cmdID) + sizeof(pCmd->devNum)) 
        {
        	uiPrintf("Error, command Len is not equal to size of FORCE_SINGLE_POWER_TX_MAX_CMD\n");
        	return(COMMS_ERR_BAD_LENGTH);
        }

	#ifdef ENDIAN_SWAP
     		pCmd->CMD_U.SET_ONE_CMD.param = ltob_l(pCmd->CMD_U.SET_ONE_CMD.param);
 	#endif

    	m_forceSinglePowerTxMax(pCmd->devNum,
			  (A_UINT16)pCmd->CMD_U.SET_ONE_CMD.param); 



    	return 0;
}

/**************************************************************************
*/


A_UINT32 processFalseDetectBackoffValsCmd
(
	struct dkThreadInfo *dkThread
)
{
	PIPE_CMD      *pCmd;
	CMD_REPLY     *ReplyCmd_p;
	A_UCHAR     *replyBuf;

	pCmd = dkThread->PipeBuffer_p;
	ReplyCmd_p = dkThread->ReplyCmd_p;
	replyBuf = (A_UCHAR *)(ReplyCmd_p->cmdBytes);

	/* check that the size is appropriate before we access */
    if( pCmd->cmdLen != sizeof(pCmd->CMD_U.FALSE_DETECT_BACKOFF_VALS_CMD)+sizeof(pCmd->cmdID) +sizeof(pCmd->devNum)) 
    {
        uiPrintf("Error, command Len is not equal to size of FALSE_DETECT_BACKOFF_VALS_CMD\n");
        return(COMMS_ERR_BAD_LENGTH);
    }

#ifdef ENDIAN_SWAP
	{
		A_UINT32 i;

		for(i = 0; i < 3; i++) {
			pCmd->CMD_U.FALSE_DETECT_BACKOFF_VALS_CMD.backoffValues[i] = 
				ltob_l(pCmd->CMD_U.FALSE_DETECT_BACKOFF_VALS_CMD.backoffValues[i]);
		}
	}
 	#endif

   	supplyFalseDetectbackoff(pCmd->devNum,
		  pCmd->CMD_U.FALSE_DETECT_BACKOFF_VALS_CMD.backoffValues); 

	return 0;
}

/**************************************************************************
*/

A_UINT32 processEnableHwCalCmd
(
	struct dkThreadInfo *dkThread
)
{
	PIPE_CMD      *pCmd;
	CMD_REPLY     *ReplyCmd_p;
	A_UCHAR     *replyBuf;

	pCmd = dkThread->PipeBuffer_p;
	ReplyCmd_p = dkThread->ReplyCmd_p;
	replyBuf = (A_UCHAR *)(ReplyCmd_p->cmdBytes);

    if( pCmd->cmdLen != sizeof(pCmd->cmdID) +sizeof(pCmd->devNum)) 
    {
        uiPrintf("Error, command Len is not equal to size of ENABLE_HW_CAL_CMD\n");
        return(COMMS_ERR_BAD_LENGTH);
    }

	m_changeCal(pCmd->devNum);
	return 0;

}

/**************************************************************************
*/

A_UINT32 processGetEepromStructCmd
(
	struct dkThreadInfo *dkThread
)
{
	PIPE_CMD      *pCmd;
	CMD_REPLY     *ReplyCmd_p;
	A_UCHAR     *replyBuf;
	A_UINT32 sizeStruct;
	A_UINT32 i;
#ifdef OWL_AP
	A_UINT8 *pReturnStruct;
	A_UINT8 *pWord;
#else
	A_INT16 *pReturnStruct;
	A_UINT16 *pWord;
#endif

	pCmd = dkThread->PipeBuffer_p;
	ReplyCmd_p = dkThread->ReplyCmd_p;
	replyBuf = (A_UCHAR *)(ReplyCmd_p->cmdBytes);

	/* check that the size is appropriate before we access */
     	if( pCmd->cmdLen != sizeof(pCmd->CMD_U.GET_EEPROM_STRUCT_CMD)+sizeof(pCmd->cmdID) +sizeof(pCmd->devNum)) 
        {
        	uiPrintf("Error, command Len is not equal to size of GET_EEPROM_STRUCT_CMD\n");
        	return(COMMS_ERR_BAD_LENGTH);
        }

	#ifdef ENDIAN_SWAP
     		pCmd->CMD_U.GET_EEPROM_STRUCT_CMD.eepStructFlag = ltob_s(pCmd->CMD_U.GET_EEPROM_STRUCT_CMD.eepStructFlag);
	#endif

#ifdef OWL_AP
	m_getEepromStruct(pCmd->devNum,
			   (A_UINT16)pCmd->CMD_U.GET_EEPROM_STRUCT_CMD.eepStructFlag,
			   (void **)&pReturnStruct,
			   &sizeStruct);		 

	*(A_UINT32 *)replyBuf = (A_UINT32)-1;

	#ifdef ENDIAN_SWAP
 		*((A_UINT32 *)replyBuf) = btol_l(sizeStruct);
	#else
		*((A_UINT32 *)replyBuf)=sizeStruct;
	#endif

	pWord = (A_UINT8 *)(replyBuf+4);

	for (i=0;i<sizeStruct;i++)
	{
		pWord[i] = pReturnStruct[i];
	}
#else
	 m_getEepromStruct(pCmd->devNum,
			   (A_UINT16)pCmd->CMD_U.GET_EEPROM_STRUCT_CMD.eepStructFlag,
			   (void **)&pReturnStruct,
			   &sizeStruct);		 

	*(A_UINT32 *)replyBuf = (A_UINT32)-1;

	#ifdef ENDIAN_SWAP
 		*((A_UINT32 *)replyBuf) = btol_l(sizeStruct);
	#else
		*((A_UINT32 *)replyBuf)=sizeStruct;
	#endif

	pWord = (A_UINT16 *)(replyBuf+4);
	sizeStruct = sizeStruct/2;

	for (i=0;i<sizeStruct;i++)
	{
		#ifdef ENDIAN_SWAP
			pWord[i] = btol_s(pReturnStruct[i]);
		#else
			pWord[i] = pReturnStruct[i];	  	  		
		#endif
	}
#endif

	return 0;
}

A_UINT32 processGetDeviceInfoCmd
(
	struct dkThreadInfo *dkThread
)
{
	PIPE_CMD      *pCmd;
	CMD_REPLY     *ReplyCmd_p;
	A_UCHAR     *replyBuf;
	SUB_DEV_INFO *pSubDevInfo;

	pCmd = dkThread->PipeBuffer_p;
	ReplyCmd_p = dkThread->ReplyCmd_p;
	replyBuf = (A_UCHAR *)(ReplyCmd_p->cmdBytes);

	/* check that the size is appropriate before we access */
     	if( pCmd->cmdLen != sizeof(pCmd->cmdID) +sizeof(pCmd->devNum)) 
        {
        	uiPrintf("Error, command Len is not equal to size of GET_DEVICE_INFO cmd\n");
        	return(COMMS_ERR_BAD_LENGTH);
        }

	pSubDevInfo = (SUB_DEV_INFO *)replyBuf;

	m_getDeviceInfo(pCmd->devNum,pSubDevInfo);

	#ifdef ENDIAN_SWAP
         	pSubDevInfo->aRevID = btol_l(pSubDevInfo->aRevID);   
		pSubDevInfo->hwDevID = btol_l(pSubDevInfo->hwDevID);   
		pSubDevInfo->swDevID = btol_l(pSubDevInfo->swDevID);   
		pSubDevInfo->bbRevID = btol_l(pSubDevInfo->bbRevID);   
		pSubDevInfo->macRev = btol_l(pSubDevInfo->macRev);   
		pSubDevInfo->subSystemID = btol_l(pSubDevInfo->subSystemID);
		pSubDevInfo->defaultConfig =  btol_l(pSubDevInfo->defaultConfig);
	#endif

	return 0;
}


A_UINT32 processWriteNewProdDataCmd
(
	struct dkThreadInfo *dkThread
)
{
	PIPE_CMD      *pCmd;
	CMD_REPLY     *ReplyCmd_p;
	A_UCHAR       *replyBuf;
#ifdef ENDIAN_SWAP
	A_UINT32      ii;
#endif

	pCmd = dkThread->PipeBuffer_p;
	ReplyCmd_p = dkThread->ReplyCmd_p;
	replyBuf = (A_UCHAR *)(ReplyCmd_p->cmdBytes);

	/* check that the size is appropriate before we access */
     	if( pCmd->cmdLen != sizeof(pCmd->CMD_U.WRITE_NEW_PROD_DATA_CMD)+sizeof(pCmd->cmdID) +sizeof(pCmd->devNum)) 
        {
        	uiPrintf("Error, command Len is not equal to size of WRITE_NEW_PROD_DATA_CMD\n");
        	return(COMMS_ERR_BAD_LENGTH);
        }

	#ifdef ENDIAN_SWAP
			for (ii=0; ii<pCmd->CMD_U.WRITE_NEW_PROD_DATA_CMD.numArgs; ii++) {
				pCmd->CMD_U.WRITE_NEW_PROD_DATA_CMD.argList[ii] = ltob_l(pCmd->CMD_U.WRITE_NEW_PROD_DATA_CMD.argList[ii]);
			}
	#endif

#if defined(SPIRIT_AP) || defined(FREEDOM_AP) // || defined(PREDATOR)
	 m_writeNewProdData
	(
		pCmd->devNum,		
		pCmd->CMD_U.WRITE_NEW_PROD_DATA_CMD.argList,
		pCmd->CMD_U.WRITE_NEW_PROD_DATA_CMD.numArgs		
	);
#endif

#if defined(PREDATOR) 
#endif

	return 0;
}


A_UINT32 processFtpDownloadFileCmd
(
	struct dkThreadInfo *dkThread
)
{
	PIPE_CMD      *pCmd;
	CMD_REPLY     *ReplyCmd_p;
	A_UCHAR     *replyBuf;

#ifndef __ATH_DJGPPDOS__
	pCmd = dkThread->PipeBuffer_p;
	ReplyCmd_p = dkThread->ReplyCmd_p;
	replyBuf = (A_UCHAR *)(ReplyCmd_p->cmdBytes);

	/* check that the size is appropriate before we access */
    if( pCmd->cmdLen != sizeof(pCmd->CMD_U.FTP_DOWNLOAD_FILE_CMD)+sizeof(pCmd->cmdID) +sizeof(pCmd->devNum)) 
    {
      	uiPrintf("Error, command Len is not equal to size of FTP_DOWNLOAD_FILE_CMD\n");
       	return(COMMS_ERR_BAD_LENGTH);
    }

	if(!m_ftpDownloadFile
	(
		pCmd->CMD_U.FTP_DOWNLOAD_FILE_CMD.hostname,
		pCmd->CMD_U.FTP_DOWNLOAD_FILE_CMD.user,
		pCmd->CMD_U.FTP_DOWNLOAD_FILE_CMD.passwd,
		pCmd->CMD_U.FTP_DOWNLOAD_FILE_CMD.remotefile, 
		pCmd->CMD_U.FTP_DOWNLOAD_FILE_CMD.localfile
	)) {
		return(COMMS_ERROR_FTPDOWNLOAD_FAIL);
	}

#else 
	uiPrintf("ftpDownload command not supported in dos client\n");
#endif //__ATH_DJGPPDOS__

	return 0;
}

	
A_UINT32 processSetQueueCmd
(
	struct dkThreadInfo *dkThread
)
{
	PIPE_CMD      *pCmd;
	CMD_REPLY     *ReplyCmd_p;
	A_UCHAR     *replyBuf;

	pCmd = dkThread->PipeBuffer_p;
	ReplyCmd_p = dkThread->ReplyCmd_p;
	replyBuf = (A_UCHAR *)(ReplyCmd_p->cmdBytes);

	/* check that the size is appropriate before we access */
     	if( pCmd->cmdLen != sizeof(pCmd->CMD_U.SET_ONE_CMD)+sizeof(pCmd->cmdID) +sizeof(pCmd->devNum)) 
        {
        	uiPrintf("Error, command Len is not equal to size of SET_ONE_CMD\n");
        	return(COMMS_ERR_BAD_LENGTH);
        }

	#ifdef ENDIAN_SWAP
     		pCmd->CMD_U.SET_ONE_CMD.param = ltob_l(pCmd->CMD_U.SET_ONE_CMD.param);
 	#endif


	m_setQueue(pCmd->devNum,
		   pCmd->CMD_U.SET_ONE_CMD.param);


    	return 0;
}

A_UINT32 processMapQueueCmd
(
	struct dkThreadInfo *dkThread
)
{
	PIPE_CMD      *pCmd;
	CMD_REPLY     *ReplyCmd_p;
	A_UCHAR     *replyBuf;

	pCmd = dkThread->PipeBuffer_p;
	ReplyCmd_p = dkThread->ReplyCmd_p;
	replyBuf = (A_UCHAR *)(ReplyCmd_p->cmdBytes);

	/* check that the size is appropriate before we access */
     	if( pCmd->cmdLen != sizeof(pCmd->CMD_U.MAP_QUEUE_CMD)+sizeof(pCmd->cmdID) +sizeof(pCmd->devNum)) 
        {
        	uiPrintf("Error, command Len is not equal to size of MAP_QUEUE \n");
        	return(COMMS_ERR_BAD_LENGTH);
        }

	#ifdef ENDIAN_SWAP
     		pCmd->CMD_U.MAP_QUEUE_CMD.qcuNumber = ltob_l(pCmd->CMD_U.MAP_QUEUE_CMD.qcuNumber);
     		pCmd->CMD_U.MAP_QUEUE_CMD.dcuNumber = ltob_l(pCmd->CMD_U.MAP_QUEUE_CMD.dcuNumber);
 	#endif

	m_mapQueue(pCmd->devNum,
		   pCmd->CMD_U.MAP_QUEUE_CMD.qcuNumber,
		   pCmd->CMD_U.MAP_QUEUE_CMD.dcuNumber);


    	return 0;
}

A_UINT32 processClearKeyCacheCmd
(
	struct dkThreadInfo *dkThread
)
{
	PIPE_CMD      *pCmd;
	CMD_REPLY     *ReplyCmd_p;
	A_UCHAR     *replyBuf;

	pCmd = dkThread->PipeBuffer_p;
	ReplyCmd_p = dkThread->ReplyCmd_p;
	replyBuf = (A_UCHAR *)(ReplyCmd_p->cmdBytes);

	/* check that the size is appropriate before we access */
     	if( pCmd->cmdLen != sizeof(pCmd->cmdID) +sizeof(pCmd->devNum)) 
        {
        	uiPrintf("Error, command Len is not equal to size of CLEAR_KEY_CACHE cmd\n");
        	return(COMMS_ERR_BAD_LENGTH);
        }

	m_clearKeyCache(pCmd->devNum);

    	return 0;
}

A_UINT32 processRunScreeningTestCmd
(
	struct dkThreadInfo *dkThread
)
{
	PIPE_CMD      *pCmd;
	CMD_REPLY     *ReplyCmd_p;
	A_UCHAR     *replyBuf;

#ifndef __ATH_DJGPPDOS__
	pCmd = dkThread->PipeBuffer_p;
	ReplyCmd_p = dkThread->ReplyCmd_p;
	replyBuf = (A_UCHAR *)(ReplyCmd_p->cmdBytes);

	/* check that the size is appropriate before we access */
     	if( pCmd->cmdLen != sizeof(pCmd->CMD_U.RUN_SCREENING_TEST_CMD)+sizeof(pCmd->cmdID) +sizeof(pCmd->devNum)) 
        {
        	uiPrintf("Error, command Len is not equal to size of RUN_SCREENING_TEST_CMD\n");
        	return(COMMS_ERR_BAD_LENGTH);
        }

	#ifdef ENDIAN_SWAP
		pCmd->CMD_U.RUN_SCREENING_TEST_CMD.testId = ltob_l(pCmd->CMD_U.RUN_SCREENING_TEST_CMD.testId);
 	#endif

	*((A_INT32 *)replyBuf) = m_runScreeningTest(pCmd->CMD_U.RUN_SCREENING_TEST_CMD.testId);

	#ifdef ENDIAN_SWAP
    		*((A_INT32 *)replyBuf) = (A_INT32)(btol_l(*((A_UINT32 *)replyBuf)));
	#endif
#else
	uiPrintf("Run screening command not supported in dos client\n");
#endif //__ATH_DJGPPDOS__


    	return 0;
}


/**************************************************************************
* processMaskTriggerSweepCmd - Process MASK_TRIGGER_SWEEP_CMD command
*
* RETURNS: 0 processed OK, an Error code if it is not
*/
A_UINT32 processMaskTriggerSweepCmd
(
	struct dkThreadInfo *dkThread
)
{
	PIPE_CMD      *pCmd;
	CMD_REPLY     *ReplyCmd_p;
	A_UCHAR     *replyBuf;
	A_UINT32 num_fail_pts;
	double   *retlist;
	int noWords;
	int i;
	A_UCHAR *wordPtr;
	char str[32];

#ifndef __ATH_DJGPPDOS__
	pCmd = dkThread->PipeBuffer_p;
	ReplyCmd_p = dkThread->ReplyCmd_p;
	replyBuf = (A_UCHAR *)(ReplyCmd_p->cmdBytes);


	retlist = (double *) malloc(pCmd->CMD_U.MASK_TRIGGER_SWEEP_CMD.ret_length * sizeof(double));
	if (retlist == NULL) {
		uiPrintf("processMaskTriggerSweepCmd: could not allocate memory to receive mask retlist of doubles\n");
		return(COMMS_ERR_ALLOC_FAIL);
	}

    /* check that the size is appropriate before we access */
    if( pCmd->cmdLen != sizeof(pCmd->CMD_U.MASK_TRIGGER_SWEEP_CMD)+sizeof(pCmd->cmdID)+sizeof(pCmd->devNum) )
        {
        uiPrintf("Error, command Len is not equal to size of MASK_TRIGGER_SWEEP_CMD\n");
        return(COMMS_ERR_BAD_LENGTH);
        }

#ifdef ENDIAN_SWAP
	pCmd->CMD_U.MASK_TRIGGER_SWEEP_CMD.channel = ltob_l(pCmd->CMD_U.MASK_TRIGGER_SWEEP_CMD.channel);		
	pCmd->CMD_U.MASK_TRIGGER_SWEEP_CMD.mode = ltob_l(pCmd->CMD_U.MASK_TRIGGER_SWEEP_CMD.mode);		
	pCmd->CMD_U.MASK_TRIGGER_SWEEP_CMD.averages = ltob_l(pCmd->CMD_U.MASK_TRIGGER_SWEEP_CMD.averages);		
	pCmd->CMD_U.MASK_TRIGGER_SWEEP_CMD.path_loss = ltob_l(pCmd->CMD_U.MASK_TRIGGER_SWEEP_CMD.path_loss);		
	pCmd->CMD_U.MASK_TRIGGER_SWEEP_CMD.ret_length = ltob_l(pCmd->CMD_U.MASK_TRIGGER_SWEEP_CMD.ret_length);		
#endif

    
    /* call the function */
    num_fail_pts = trigger_sweep(pCmd->devNum, 
								 pCmd->CMD_U.MASK_TRIGGER_SWEEP_CMD.channel, 
								 pCmd->CMD_U.MASK_TRIGGER_SWEEP_CMD.mode, 
								 pCmd->CMD_U.MASK_TRIGGER_SWEEP_CMD.averages, 
								 pCmd->CMD_U.MASK_TRIGGER_SWEEP_CMD.path_loss, 
								 pCmd->CMD_U.MASK_TRIGGER_SWEEP_CMD.return_spectrum, 
								 (double *)(retlist));
 


    noWords =   pCmd->CMD_U.MASK_TRIGGER_SWEEP_CMD.ret_length;
	wordPtr = (A_UCHAR *)(replyBuf+sizeof(A_UINT32));
    
    for (i=0;i<noWords;i++)
    {
		sprintf(&(str[0]), "%g", retlist[i]);
		memcpy(wordPtr + i*32*sizeof(A_UCHAR), &(str[0]), 32);    	
    }
	
	free(retlist);
// Swap from big endian to little endian.
#ifdef ENDIAN_SWAP
{
	num_fail_pts = btol_l(num_fail_pts);
}
#endif

	memcpy(replyBuf, &num_fail_pts, sizeof(A_UINT32));
#else
	uiPrintf("Trigger sweep command not supported in DOS client\n");
#endif //__ATH_DJGPPDOS__
    return(0);
}


/**************************************************************************
* processMaskDetectSignalCmd - Process MASK_DETECT_SIGNAL_CMD command
*
* RETURNS: 0 processed OK, an Error code if it is not
*/
A_UINT32 processMaskDetectSignalCmd
(
	struct dkThreadInfo *dkThread
)
{
	PIPE_CMD      *pCmd;
	CMD_REPLY     *ReplyCmd_p;
	A_UCHAR     *replyBuf;
	double rssi;
	char rssi_str[32];

#ifndef __ATH_DJGPPDOS__
	pCmd = dkThread->PipeBuffer_p;
	ReplyCmd_p = dkThread->ReplyCmd_p;
	replyBuf = (A_UCHAR *)(ReplyCmd_p->cmdBytes);

    /* check that the size is appropriate before we access */
    if( pCmd->cmdLen != sizeof(pCmd->CMD_U.MASK_DETECT_SIGNAL_CMD)+sizeof(pCmd->cmdID)+sizeof(pCmd->devNum) )
        {
        uiPrintf("Error, command Len is not equal to size of MASK_DETECT_SIGNAL_CMD\n");
        return(COMMS_ERR_BAD_LENGTH);
        }

#ifdef ENDIAN_SWAP
	pCmd->CMD_U.MASK_DETECT_SIGNAL_CMD.desc_cnt = ltob_l(pCmd->CMD_U.MASK_DETECT_SIGNAL_CMD.desc_cnt);		
	pCmd->CMD_U.MASK_DETECT_SIGNAL_CMD.adc_des_size = ltob_l(pCmd->CMD_U.MASK_DETECT_SIGNAL_CMD.adc_des_size);		
	pCmd->CMD_U.MASK_DETECT_SIGNAL_CMD.mode = ltob_l(pCmd->CMD_U.MASK_DETECT_SIGNAL_CMD.mode);		
	pCmd->CMD_U.MASK_DETECT_SIGNAL_CMD.ret_length = ltob_l(pCmd->CMD_U.MASK_DETECT_SIGNAL_CMD.ret_length);		
#endif

    
    /* call the function */
    rssi = detect_signal(pCmd->CMD_U.MASK_DETECT_SIGNAL_CMD.desc_cnt, 
						 pCmd->CMD_U.MASK_DETECT_SIGNAL_CMD.adc_des_size, 
						 pCmd->CMD_U.MASK_DETECT_SIGNAL_CMD.mode, 
						 (A_UINT32 *)(replyBuf+32*sizeof(A_UCHAR)));
						 //(A_UINT32 *)(replyBuf+sizeof(double)));
		
	
// Swap from big endian to little endian.
#ifdef ENDIAN_SWAP
{
    int noWords;
    int i;
    A_UINT32 *wordPtr;

    noWords =   pCmd->CMD_U.MASK_DETECT_SIGNAL_CMD.ret_length;
	wordPtr = (A_UINT32 *)(replyBuf+32*sizeof(A_UCHAR));
    //wordPtr = (A_UINT32 *)(replyBuf+sizeof(double));
    for (i=0;i<noWords;i++)
    {
    	*(wordPtr + i) = btol_l(*(wordPtr + i));
    }

}
#endif

	sprintf(rssi_str, "%g", rssi);
	memcpy(replyBuf, &(rssi_str[0]), 32*sizeof(A_UCHAR));

#else
	uiPrintf("Detect signal command not implemented\n");
#endif //__ATH_DJGPPDOS__
    return(0);
}

/**************************************************************************
* processMaskConfigCaptureCmd - Process MASK_CONFIG_CAPTURE_CMD command
*
* RETURNS: 0 processed OK, an Error code if it is not
*/
A_UINT32 processMaskConfigCaptureCmd
(
	struct dkThreadInfo *dkThread
)
{	
	PIPE_CMD      *pCmd;
	CMD_REPLY     *ReplyCmd_p;
	A_UCHAR     *replyBuf;

#ifndef __ATH_DJGPPDOS__
	pCmd = dkThread->PipeBuffer_p;
	ReplyCmd_p = dkThread->ReplyCmd_p;
	replyBuf = (A_UCHAR *)(ReplyCmd_p->cmdBytes);
	
    /* check that the size is appropriate before we access */
    if( pCmd->cmdLen != sizeof(pCmd->CMD_U.MASK_CONFIG_CAPTURE_CMD)+sizeof(pCmd->cmdID)+sizeof(pCmd->devNum) )
        {
        uiPrintf("Error, command Len is not equal to size of MASK_CONFIG_CAPTURE_CMD\n");
        return(COMMS_ERR_BAD_LENGTH);
        }

#ifdef ENDIAN_SWAP
{
    int noWords;
    int i;
    A_UINT32 *wordPtr;

    noWords =  5;
    wordPtr = pCmd->CMD_U.MASK_CONFIG_CAPTURE_CMD.gain;
    for (i=0;i<noWords;i++)
    {
    	*(wordPtr + i) = ltob_l(*(wordPtr + i));
    }
	
	pCmd->CMD_U.MASK_CONFIG_CAPTURE_CMD.dut_dev = ltob_l(pCmd->CMD_U.MASK_CONFIG_CAPTURE_CMD.dut_dev);		
	pCmd->CMD_U.MASK_CONFIG_CAPTURE_CMD.channel = ltob_l(pCmd->CMD_U.MASK_CONFIG_CAPTURE_CMD.channel);		
	pCmd->CMD_U.MASK_CONFIG_CAPTURE_CMD.mode = ltob_l(pCmd->CMD_U.MASK_CONFIG_CAPTURE_CMD.mode);		
	pCmd->CMD_U.MASK_CONFIG_CAPTURE_CMD.turbo = ltob_l(pCmd->CMD_U.MASK_CONFIG_CAPTURE_CMD.turbo);		
}
#endif

    
    /* call the function */
    config_capture (pCmd->CMD_U.MASK_CONFIG_CAPTURE_CMD.dut_dev, 
					pCmd->CMD_U.MASK_CONFIG_CAPTURE_CMD.RX_ID, 
					pCmd->CMD_U.MASK_CONFIG_CAPTURE_CMD.BSS_ID, 
					pCmd->CMD_U.MASK_CONFIG_CAPTURE_CMD.channel, 
					pCmd->CMD_U.MASK_CONFIG_CAPTURE_CMD.turbo, 
					pCmd->CMD_U.MASK_CONFIG_CAPTURE_CMD.gain, 
					pCmd->CMD_U.MASK_CONFIG_CAPTURE_CMD.mode) ;
	
// Swap from big endian to little endian.
#ifdef ENDIAN_SWAP
{
}
#endif

#else
	uiPrintf("ConfigCapture command not implemented\n");
#endif //__ATH_DJGPPDOS__
    return(0);
}

/**************************************************************************
* processMaskSetDevNumsCmd - Process MASK_SET_DEV_NUMS_CMD command
*
* RETURNS: 0 processed OK, an Error code if it is not
*/
A_UINT32 processMaskSetDevNumsCmd
(
	struct dkThreadInfo *dkThread
)
{
	PIPE_CMD      *pCmd;
	CMD_REPLY     *ReplyCmd_p;
	A_UCHAR     *replyBuf;

#ifndef __ATH_DJGPPDOS__
	pCmd = dkThread->PipeBuffer_p;
	ReplyCmd_p = dkThread->ReplyCmd_p;
	replyBuf = (A_UCHAR *)(ReplyCmd_p->cmdBytes);
	
    /* check that the size is appropriate before we access */
    if( pCmd->cmdLen != sizeof(pCmd->CMD_U.SET_ONE_CMD)+sizeof(pCmd->cmdID)+sizeof(pCmd->devNum) )
        {
        uiPrintf("Error, command Len is not equal to size of SET_ONE_CMD\n");
        return(COMMS_ERR_BAD_LENGTH);
        }

#ifdef ENDIAN_SWAP
	pCmd->CMD_U.SET_ONE_CMD.param = ltob_l(pCmd->CMD_U.SET_ONE_CMD.param);		
#endif

    
    /* call the function */
    set_dev_nums(pCmd->devNum, pCmd->CMD_U.SET_ONE_CMD.param) ;
	
// Swap from big endian to little endian.
#ifdef ENDIAN_SWAP
{
}
#endif

#else
	uiPrintf("Set dev nums command not implemented\n");
#endif //__ATH_DJGPPDOS__
    return(0);
}

/**************************************************************************
* processMaskForceMinCCAPowerCmd - Process MASK_FORCE_MIN_CCAPWR_CMD command
*
* RETURNS: 0 processed OK, an Error code if it is not
*/
A_UINT32 processMaskForceMinCCAPowerCmd
(
	struct dkThreadInfo *dkThread
)
{
	PIPE_CMD      *pCmd;
	CMD_REPLY     *ReplyCmd_p;
	A_UCHAR     *replyBuf;

#ifndef __ATH_DJGPPDOS__
	pCmd = dkThread->PipeBuffer_p;
	ReplyCmd_p = dkThread->ReplyCmd_p;
	replyBuf = (A_UCHAR *)(ReplyCmd_p->cmdBytes);
	
    /* check that the size is appropriate before we access */
    if( pCmd->cmdLen != sizeof(pCmd->cmdID)+sizeof(pCmd->devNum) + sizeof(pCmd->CMD_U.MASK_FORCE_MIN_CCAPWR_CMD) )
        {
        uiPrintf("Error, command Len is not equal to size of MASK_FORCE_MIN_CCAPWR cmd\n");
        return(COMMS_ERR_BAD_LENGTH);
        }

    /* call the function */
    force_minccapwr(pCmd->CMD_U.MASK_FORCE_MIN_CCAPWR_CMD.maxccapwr) ;
	
// Swap from big endian to little endian.
#ifdef ENDIAN_SWAP
{
}
#endif
#else
	uiPrintf("Force min cca power command not implemented\n");
#endif //__ATH_DJGPPDOS__

    return(0);
}


/**************************************************************************
* processSetLibConfigCmd - Process SET_LIB_CONFIG_CMD command
*
* RETURNS: 0 processed OK, an Error code if it is not
*/
A_UINT32 processSetLibConfigCmd
(
	struct dkThreadInfo *dkThread
)
{
	PIPE_CMD      *pCmd;
	CMD_REPLY     *ReplyCmd_p;

	pCmd = dkThread->PipeBuffer_p;
	ReplyCmd_p = dkThread->ReplyCmd_p;
    
	/* check that the size is appropriate before we access */
	if( pCmd->cmdLen != sizeof(pCmd->CMD_U.SET_LIB_CONFIG_CMD)+sizeof(pCmd->cmdID) +sizeof(pCmd->devNum)) {
		uiPrintf("Error, command Len is not equal to size of SET_LIB_CONFIG_CMD\n");
		return(COMMS_ERR_BAD_LENGTH);
	}

#ifdef ENDIAN_SWAP
	{
	// assumption all struct variables are 4 byte words
	
    	A_UINT32      *tempPtr;
		int i;

		tempPtr = (A_UINT32 *)(&(pCmd->CMD_U.SET_LIB_CONFIG_CMD.libParams));

		for (i=0; i < (sizeof(LIB_PARAMS)/sizeof(A_UINT32));i++)
		{
	  	  	*(tempPtr+i) = ltob_l(*(tempPtr+i));
		}
	
#ifdef ARCH_BIG_ENDIAN
//	pCmd->CMD_U.SET_LIB_CONFIG_CMD.libParams.refClock  = ltob_l(pCmd->CMD_U.SET_LIB_CONFIG_CMD.libParams.refClock);
//	pCmd->CMD_U.SET_LIB_CONFIG_CMD.libParams.beanie2928Mode  = ltob_l(pCmd->CMD_U.SET_LIB_CONFIG_CMD.libParams.beanie2928Mode);
//	pCmd->CMD_U.SET_LIB_CONFIG_CMD.libParams.enableXR  = ltob_l(pCmd->CMD_U.SET_LIB_CONFIG_CMD.libParams.enableXR);
//	pCmd->CMD_U.SET_LIB_CONFIG_CMD.libParams.loadEar  = ltob_l(pCmd->CMD_U.SET_LIB_CONFIG_CMD.libParams.loadEar);
//	pCmd->CMD_U.SET_LIB_CONFIG_CMD.libParams.applyCtlLimit  = ltob_l(pCmd->CMD_U.SET_LIB_CONFIG_CMD.libParams.applyCtlLimit);
#else 
	pCmd->CMD_U.SET_LIB_CONFIG_CMD.libParams.refClock  = ltob_l(pCmd->CMD_U.SET_LIB_CONFIG_CMD.libParams.refClock);
	pCmd->CMD_U.SET_LIB_CONFIG_CMD.libParams.beanie2928Mode  = ltob_l(pCmd->CMD_U.SET_LIB_CONFIG_CMD.libParams.beanie2928Mode);
	pCmd->CMD_U.SET_LIB_CONFIG_CMD.libParams.enableXR  = ltob_l(pCmd->CMD_U.SET_LIB_CONFIG_CMD.libParams.enableXR);
	pCmd->CMD_U.SET_LIB_CONFIG_CMD.libParams.loadEar  = ltob_l(pCmd->CMD_U.SET_LIB_CONFIG_CMD.libParams.loadEar);
	pCmd->CMD_U.SET_LIB_CONFIG_CMD.libParams.applyCtlLimit  = ltob_l(pCmd->CMD_U.SET_LIB_CONFIG_CMD.libParams.applyCtlLimit);
#endif
	}
#endif

	configureLibParams(pCmd->devNum, &(pCmd->CMD_U.SET_LIB_CONFIG_CMD.libParams), 1);
	changePciWritesFlag(pCmd->devNum, pCmd->CMD_U.SET_LIB_CONFIG_CMD.libParams.printPciWrites);
	return(0);
}


/**************************************************************************
* processSelectDevNumCmd - Process SELECT_DEV_NUM_CMD command
*
* RETURNS: 0 processed OK, an Error code if it is not
*/
A_UINT32 processSelectDevNumCmd
(
	struct dkThreadInfo *dkThread
)
{
	PIPE_CMD      *pCmd;
	CMD_REPLY     *ReplyCmd_p;
	A_UINT16       i;

	pCmd = dkThread->PipeBuffer_p;
	ReplyCmd_p = dkThread->ReplyCmd_p;
    
	/* check that the size is appropriate before we access */
	if( pCmd->cmdLen != sizeof(pCmd->cmdID) +sizeof(pCmd->devNum)) {
		uiPrintf("Error, command Len is not equal to size of SELECT_DEV_NUM_CMD\n");
		return(COMMS_ERR_BAD_LENGTH);
	}

	for (i = 0; i < MAX_CLIENTS_PER_THREAD; i++) {
		if(pCmd->devNum == dkThread->devNum[i]) {
			break;
		}
	}
	
	if(i != MAX_CLIENTS_PER_THREAD) {
		//make the devIndex for this command the current devIndex
		dkThread->currentDevIndex = dkThread->devIndex[i];
	}
	else {
		uiPrintf("Error, Invalid devNum in SELECT_DEV_NUM_CMD\n");
		return(COMMS_ERR_BAD_LENGTH);
	}
  
	return(0);
}

/**************************************************************************
* processGetMaxLinPwrCmd - Process SELECT_DEV_NUM_CMD command
*
* RETURNS: A_INT32 maxlinPwr*2
*/
A_INT32 processGetMaxLinPwrCmd
(
	struct dkThreadInfo *dkThread
)
{
	PIPE_CMD      *pCmd;
	CMD_REPLY     *ReplyCmd_p;
	A_UCHAR     *replyBuf;
	A_INT32      retVal;

	pCmd = dkThread->PipeBuffer_p;
	ReplyCmd_p = dkThread->ReplyCmd_p;
	replyBuf = (A_UCHAR *)(ReplyCmd_p->cmdBytes);


    /* check that the size is appropriate before we access */
    if( pCmd->cmdLen != sizeof(pCmd->cmdID) +sizeof(pCmd->devNum))
        {
        uiPrintf("Error, command Len is not equal to size of GET_MAX_LIN_PWR_CMD\n");
        return(COMMS_ERR_BAD_LENGTH);
        }

    /* call the function */
	retVal = getMaxLinPowerx4(pCmd->devNum);
 

// Swap from big endian to little endian.
#ifdef ENDIAN_SWAP
{
	retVal = btol_l(retVal);
}
#endif

	memcpy(replyBuf, &retVal, sizeof(A_INT32));

    return(0);
}


/***************************************************************************
* Function Name: 
* Description: Perform IQ calibration of the device
* Return: None.
*/
A_UINT32 processIqCal
(
	struct dkThreadInfo *dkThread
)
{
	PIPE_CMD      *pCmd;
	CMD_REPLY     *ReplyCmd_p;
	A_UCHAR     *replyBuf;
    IQ_FACTOR   iq_coeff;

	pCmd = dkThread->PipeBuffer_p;
	ReplyCmd_p = dkThread->ReplyCmd_p;
	replyBuf = (A_UCHAR *)(ReplyCmd_p->cmdBytes);

    /* check that the size is appropriate before we access */
    if( pCmd->cmdLen != sizeof(pCmd->CMD_U.IQ_CAL_CMD) + sizeof(pCmd->cmdID) +sizeof(pCmd->devNum)){
        uiPrintf("Error, command Len is not equal to size of IQ_CAL_CMD\n");
        return(COMMS_ERR_BAD_LENGTH);
    }

	m_iq_calibration(pCmd->devNum, &iq_coeff);
	memcpy(replyBuf, &iq_coeff, sizeof(IQ_FACTOR));

#ifdef ENDIAN_SWAP
// assumption all struct variables are 4 byte words
	{
		int i;

		for (i=0;i<(sizeof(IQ_FACTOR));i+=4)
		{
	  	  	*((A_UINT32 *)(replyBuf+i)) = btol_l(*((A_UINT32 *)(replyBuf+i)));
		}
	}
#endif

    return 0;
}


/**************************************************************************
*/

A_UINT32 processTempCompCmd
(
	struct dkThreadInfo *dkThread
)
{
	PIPE_CMD      *pCmd;
	CMD_REPLY     *ReplyCmd_p;
	A_UCHAR     *replyBuf;

	pCmd = dkThread->PipeBuffer_p;
	ReplyCmd_p = dkThread->ReplyCmd_p;
	replyBuf = (A_UCHAR *)(ReplyCmd_p->cmdBytes);

	/* check that the size is appropriate before we access */
     	if( pCmd->cmdLen != sizeof(pCmd->cmdID) +sizeof(pCmd->devNum)) 
        {
        	uiPrintf("Error, command Len is not equal to size of TEMPCOMP\n");
        	return(COMMS_ERR_BAD_LENGTH);
        }

	m_TempComp(pCmd->devNum);

    	return 0;

}


#endif // THIN_CLIENT_BUILD

/**************************************************************************
* commandDispatch - Process commands received from dk_perl
*
* Use the command ID to process the commands received from dk_perl and call
* the correct command related code
*
* RETURNS: A_OK or A_ERROR
*/

A_STATUS commandDispatch
(
	struct dkThreadInfo *dkThread
)
{
    A_UINT32    sizeReplyBuf = 0;
    A_BOOL      goodWrite    = FALSE;
    A_INT32 	errNo;
#if defined(THIN_CLIENT_BUILD)
#else  // THIN_CLIENT_BUILD
    A_CHAR	errStr[SIZE_ERROR_BUFFER];
#endif // THIN_CLIENT_BUILD
    OS_SOCK_INFO  *pOSSock;
    PIPE_CMD      *pCmd;
    CMD_REPLY     *ReplyCmd_p;
    A_UINT32	 status=0;
	A_UINT32     i;

	pOSSock = dkThread->pOSSock;   
	ReplyCmd_p = dkThread->ReplyCmd_p;
	pCmd = dkThread->PipeBuffer_p;


    if (pCmd->cmdID != INIT_F2_CMD_ID && pCmd->cmdID != DISCONNECT_PIPE_CMD_ID &&
		pCmd->cmdID != ENABLE_HW_CAL_CMD && pCmd->cmdID != SELECT_HW_CMD_ID) {
      for (i = 0; i < MAX_CLIENTS_PER_THREAD; i++) {
		if(pCmd->devNum == dkThread->devNum[i]) {
			break ;
		}
	  }
	
	  if((i != MAX_CLIENTS_PER_THREAD)) {
		//make the devIndex for this command the current devIndex
		dkThread->currentDevIndex = dkThread->devIndex[i];
	  }
	  else {
		uiPrintf("Error, Invalid devNum (%d) in Command=%d\n", pCmd->devNum, pCmd->cmdID);
		status = COMMS_ERR_BAD_LENGTH;
	  }

#ifdef USB_COMN
	//uiPrintf("C:%d::", pCmd->cmdID);
#else
	  q_uiPrintf("Command id %d for devIndex %d(devNum=%d) at port_num=%d\n",pCmd->cmdID, dkThread->currentDevIndex, pCmd->devNum, pOSSock->port_num);
#endif

    }
    if (status!=COMMS_ERR_BAD_LENGTH) {
    switch ( pCmd->cmdID ) {

	case INIT_F2_CMD_ID:
		status = processInitF2Cmd(dkThread);
#if defined(THIN_CLIENT_BUILD) 
		sizeReplyBuf = 4 + sizeof(DK_DEV_INFO);
#else // THIN_CLIENT_BUILD
		sizeReplyBuf = 4;
#endif // THIN_CLIENT_BUILD
		break;

	case REG_READ_CMD_ID:
		status = processRegReadCmd(dkThread);
		sizeReplyBuf = 4;
		break;

	case REG_WRITE_CMD_ID:
		status = processRegWriteCmd(dkThread);
		sizeReplyBuf = 0;
		break;

	case CFG_READ_CMD_ID:
		status = processCfgReadCmd(dkThread);
		sizeReplyBuf = 4;
		break;

	case CFG_WRITE_CMD_ID:
#ifdef PHOENIX
	asm volatile (
	  "mcr p15, 0, r0, c7, c0, 4\n"
	);
		status = A_OK;
#else
		status = processCfgWriteCmd(dkThread);
#endif
		sizeReplyBuf = 0;
		break;
        case RTC_REG_WRITE_CMD_ID:
                status = processRtcRegWriteCmd(dkThread);
                sizeReplyBuf = 0;
                break;

        case RTC_REG_READ_CMD_ID:
                status = processRtcRegReadCmd(dkThread);
                sizeReplyBuf = 4;
                break;

	case MEM_READ_BLOCK_CMD_ID:
		status = processMemReadBlockCmd(dkThread);
		sizeReplyBuf = pCmd->CMD_U.MEM_READ_BLOCK_CMD.length;
		break;

	case MEM_WRITE_BLOCK_CMD_ID:
		status = processMemWriteBlockCmd(dkThread);
		sizeReplyBuf = 4;
		break;

	case CREATE_EVENT_CMD_ID:
		status = processCreateEventCmd(dkThread);
		sizeReplyBuf = 0;
		break;

	case GET_EVENT_CMD_ID:
		status = processGetEventCmd(dkThread);
		sizeReplyBuf = sizeof(EVENT_STRUCT);
		break;

	case M_CLOSE_DEVICE_CMD_ID:
		status = processCloseDeviceCmd(dkThread);
		sizeReplyBuf = 0;
		break;

	case M_HW_RESET_CMD_ID:
		status = processHwReset(dkThread);
		sizeReplyBuf = 4;
		break;
	case M_PLL_PROGRAM_CMD_ID:
		status = processPllProgram(dkThread);
		sizeReplyBuf = 0;
		break;

	case M_PCI_WRITE_CMD_ID:
		status = processPciWriteCmd(dkThread);
		sizeReplyBuf = 0;
		break;

	case M_CAL_CHECK_CMD_ID:
		status = processCalCheckCmd(dkThread);
		sizeReplyBuf = 4;
		break;

	case DISCONNECT_PIPE_CMD_ID:
		status = 0;
		pOSSock->sockDisconnect = 1;
		break;

	case CLOSE_PIPE_CMD_ID:
		status = 0;
		pOSSock->sockClose = 1;
		break;

	case M_EEPROM_READ_CMD_ID:
		status = COMMS_ERR_BAD_LENGTH;
		status = processEepromReadCmd(dkThread);
		sizeReplyBuf = 4;
		break;

	case M_EEPROM_WRITE_CMD_ID:
		status = COMMS_ERR_BAD_LENGTH;
		status = processEepromWriteCmd(dkThread);
		sizeReplyBuf = 0;
		break;

	case M_EEPROM_READ_BLOCK_CMD_ID:
		status = COMMS_ERR_BAD_LENGTH;
		status = processEepromReadBlockCmd(dkThread);
		sizeReplyBuf = pCmd->CMD_U.EEPROM_READ_BLOCK_CMD.length * 4;
		break;

	case M_EEPROM_WRITE_BLOCK_CMD_ID:
		status = COMMS_ERR_BAD_LENGTH;
		status = processEepromWriteBlockCmd(dkThread);
		sizeReplyBuf = 0;
		break;

        case M_EEPROM_WRITE_BYTEBASED_BLOCK_CMD_ID:
                status = COMMS_ERR_BAD_LENGTH;
                status = processEepromWriteByteBasedBlockCmd(dkThread);
                sizeReplyBuf = 0;
                break;

        case GET_ENDIAN_MODE_CMD_ID:
                status = COMMS_ERR_BAD_LENGTH;
                status = processGetEndianModeCmd(dkThread);
                sizeReplyBuf = 4;
                break;

    case AP_REG_READ_CMD_ID:
        status = processApRegReadCmd(dkThread);
        sizeReplyBuf = 4;
        break;

    case AP_REG_WRITE_CMD_ID:
        status = processApRegWriteCmd(dkThread);
        sizeReplyBuf = 0;
        break;

	case M_WRITE_PROD_DATA_CMD_ID:
		status = processWriteProdDataCmd(dkThread);
		sizeReplyBuf = 0;
		break;

	case M_FILL_TX_STATS_CMD_ID:
		status = COMMS_ERR_BAD_LENGTH;
		status = processFillTxStatsCmd(dkThread);
		sizeReplyBuf = sizeof(TX_STATS_STRUCT) * STATS_BINS;
		break;

	case M_CREATE_DESC_CMD_ID:
		status = COMMS_ERR_BAD_LENGTH;
		status = processCreateDescCmd(dkThread);
		sizeReplyBuf = 0;
		break;

#if defined(PHOENIX)
    case PHS_CHANNEL_TOGGLE_CMD_ID:
        status = processPhsChannelToggleCmd(dkThread);
        sizeReplyBuf = 0;
        break;
    case STOP_PHS_CHANNEL_TOGGLE_CMD_ID:
        status = processStopPhsChannelToggleCmd(dkThread);
        sizeReplyBuf = 0;
        break;
    case GENERIC_FN_CALL_CMD_ID:
        status = processGenericFnCallCmd(dkThread);
        sizeReplyBuf = 4;
        break;
#endif

    case SEND_FRAME_CMD_ID:
        status = processSendFrameCmd(dkThread);
        sizeReplyBuf = 4;
        break;

    case RECV_FRAME_CMD_ID:
        status = processRecvFrameCmd(dkThread);
        sizeReplyBuf = 4;
        break;

	case LOAD_AND_PROGRAM_CODE_CMD_ID:
		status = processLoadAndProgramCodeCmd(dkThread);
		sizeReplyBuf = 4;
		break;

#if defined(THIN_CLIENT_BUILD)
#else  // THIN_CLIENT_BUILD
	case LOAD_AND_RUN_CODE_CMD_ID:
		status = processLoadAndRunCodeCmd(dkThread);
		sizeReplyBuf = 4;
		break;

	case M_WRITE_NEW_PROD_DATA_CMD_ID:
		status = processWriteNewProdDataCmd(dkThread);
		sizeReplyBuf = 0;
		break;

	case MEM_READ_CMD_ID:
		status = processMemReadCmd(dkThread);
		sizeReplyBuf = 4;
		break;

	case MEM_WRITE_CMD_ID:
		status = processMemWriteCmd(dkThread);
		sizeReplyBuf = 0;
		break;

	case SELECT_HW_CMD_ID:
		status = processSelectHwCmd(dkThread);
		sizeReplyBuf = 0;
		break;

	case TRAM_READ_BLOCK_CMD_ID:
		status = processTramReadBlockCmd(dkThread);
		sizeReplyBuf = pCmd->CMD_U.MEM_READ_BLOCK_CMD.length;
		break;

	case TRAM_WRITE_BLOCK_CMD_ID:
		status = processTramWriteBlockCmd(dkThread);
		sizeReplyBuf = 0;
		break;

	case REMAP_HW_CMD_ID:
		status = processRemapHwCmd(dkThread);
		sizeReplyBuf = 0;
		break;

	case MEM_ALLOC_CMD_ID:
		status = processMemAllocCmd(dkThread);
		sizeReplyBuf = 4;
		break;

	case MEM_FREE_CMD_ID:
		status = processMemFreeCmd(dkThread);
		sizeReplyBuf = 0;
		break;

	case ISR_FEATURE_ENABLE_CMD_ID:
		status = processIsrFeatureEnableCmd(dkThread);
		sizeReplyBuf = 0;
		break;

	case ISR_FEATURE_DISABLE_CMD_ID:
		status = processIsrFeatureDisableCmd(dkThread);
		sizeReplyBuf = 0;
		break;

	case ISR_GET_RX_STATS_CMD_ID:
		status = processIsrGetRxStatsCmd(dkThread);
		sizeReplyBuf = *((A_UINT16 *)(ReplyCmd_p->cmdBytes)) + 2;  /* num of stats bytes is in first 2 bytes of buf */
		break;

	case ISR_GET_TX_STATS_CMD_ID:
		status = processIsrGetTxStatsCmd(dkThread);
		sizeReplyBuf = *((A_UINT16 *)(ReplyCmd_p->cmdBytes)) + 2;  /* num of stats bytes is in first 2 bytes of buf */
		break;

	case ISR_SINGLE_RX_STAT_CMD_ID:
		status = processIsrSingleRxStatCmd(dkThread);
		sizeReplyBuf = 4;
		break;

	case ISR_SINGLE_TX_STAT_CMD_ID:
		status = processIsrSingleTxStatCmd(dkThread);
		sizeReplyBuf = 4;
		break;

	case M_RESET_DEVICE_CMD_ID:
		status = processResetDeviceCmd(dkThread);
		sizeReplyBuf = 0;
		break;

	case M_CHECK_REGS_CMD_ID:
		status = processCheckRegsCmd(dkThread);
		sizeReplyBuf = 4;
		break;

	case M_CHANGE_CHANNEL_CMD_ID:
		status = processChangeChannelCmd(dkThread);
		sizeReplyBuf = 0;
		break;

	case M_CHECK_PROM_CMD_ID:
		status = processCheckPromCmd(dkThread);
		sizeReplyBuf = 4;
		break;

	case M_REREAD_PROM_CMD_ID:
		status = processRereadPromCmd(dkThread);
		sizeReplyBuf = 0;
		break;

	case M_TX_DATA_SETUP_CMD_ID:
		status = processTxDataSetupCmd(dkThread);
		sizeReplyBuf = 0;
		break;

	case M_TX_DATA_AGG_SETUP_CMD_ID:
		status = processTxDataAggSetupCmd(dkThread);
		sizeReplyBuf = 0;
		break;

	case M_TX_DATA_START_CMD_ID:
		status = processTxDataStartCmd(dkThread);
		sizeReplyBuf = 0;
		break;

	case M_TX_DATA_BEGIN_CMD_ID:
		status = processTxDataBeginCmd(dkThread);
		sizeReplyBuf = 0;
		break;

	case M_TX_DATA_COMPLETE_CMD_ID:
		status = processTxDataCompleteCmd(dkThread);
		sizeReplyBuf = 0;
		break;

	case M_RX_DATA_SETUP_CMD_ID:
		status = processRxDataSetupCmd(dkThread);
		sizeReplyBuf = 0;
		break;

	case M_RX_DATA_AGG_SETUP_CMD_ID:
		status = processRxDataAggSetupCmd(dkThread);
		sizeReplyBuf = 0;
		break;

	case M_RX_DATA_BEGIN_CMD_ID:
		status = processRxDataBeginCmd(dkThread);
		sizeReplyBuf = 0;
		break;

	case M_RX_DATA_START_CMD_ID:
		status = processRxDataStartCmd(dkThread);
		sizeReplyBuf = 0;
		break;

	case M_RX_DATA_COMPLETE_CMD_ID:
		status = processRxDataCompleteCmd(dkThread);
		sizeReplyBuf = 0;
		break;

	case M_TXRX_DATA_BEGIN_CMD_ID:
		status = processTxRxDataBeginCmd(dkThread);
		sizeReplyBuf = 0;
		break;

	case M_CLEANUP_TXRX_MEMORY_CMD_ID:
		status = processCleanupTxRxMemoryCmd(dkThread);
		sizeReplyBuf = 0;
		break;

	case M_TX_GET_STATS_CMD_ID:
		status = processTxGetStatsCmd(dkThread);
		sizeReplyBuf = sizeof(TX_STATS_STRUCT);
		break;

	case M_RX_STATS_SNAPSHOT_CMD_ID:
		status = processRxStatsSnapshotCmd(dkThread);
		sizeReplyBuf = sizeof(RX_STATS_SNAPSHOT) + sizeof(A_UINT32); //send back status
		break;

	case M_RX_GET_STATS_CMD_ID:
		status = processRxGetStatsCmd(dkThread);
		sizeReplyBuf = sizeof(RX_STATS_STRUCT);
		break;

	case M_GET_CTL_INFO_CMD_ID:
		status = processGetCtlPowerInfoCmd(dkThread);
		sizeReplyBuf = sizeof(CTL_POWER_INFO);
		break;

	case M_RX_GET_DATA_CMD_ID:
		status = processRxGetDataCmd(dkThread);
		sizeReplyBuf = pCmd->CMD_U.RX_GET_DATA_CMD.sizeBuffer;
		break;

	case M_TX_CONT_BEGIN_CMD_ID:
		status = processTxContBeginCmd(dkThread);
		sizeReplyBuf = 0;
		break;

	case M_TX_CONT_FRAME_BEGIN_CMD_ID:
		status = processTxContFrameBeginCmd(dkThread);
		sizeReplyBuf = 0;
		break;

	case M_TX_CONT_END_CMD_ID:
		status = processTxContEndCmd(dkThread);
		sizeReplyBuf = 0;
		break;

	case M_SET_ANTENNA_CMD_ID:
		status = processSetAntennaCmd(dkThread);
		sizeReplyBuf = 0;
		break;

	case M_SET_POWER_SCALE_CMD_ID:
		status = processSetPowerScaleCmd(dkThread);
		sizeReplyBuf = 0;
		break;

	case M_SET_TRANSMIT_POWER_CMD_ID:
		status = processSetTransmitPowerCmd(dkThread);
		sizeReplyBuf = 0;
		break;

	case M_SET_SINGLE_TRANSMIT_POWER_CMD_ID:
		status = processSetSingleTransmitPowerCmd(dkThread);
		sizeReplyBuf = 0;
		break;

	case M_GET_MAX_POWER_CMD_ID:
		status = processGetMaxPowerCmd(dkThread);
		sizeReplyBuf = 2;
		break;

	case M_GET_XPDGAIN_CMD_ID:
		status = processGetXpdgainCmd(dkThread);
		sizeReplyBuf = 2;
		break;

	case M_GET_PCDAC_FOR_POWER_CMD_ID:
		status = processGetPcdacForPowerCmd(dkThread);
		sizeReplyBuf = 2;
		break;

	case M_GET_POWER_INDEX_CMD_ID:
		status = processGetPowerIndexCmd(dkThread);
		sizeReplyBuf = 2;
		break;

	case M_DEV_SLEEP_CMD_ID:
		status = processDevSleepCmd(dkThread);
		sizeReplyBuf = 0;
		break;

	case M_CHANGE_FIELD_CMD_ID:
		status = processChangeFieldCmd(dkThread);
		sizeReplyBuf = 0;
		break;

	case M_ENABLE_WEP_CMD_ID:
		status = processEnableWepCmd(dkThread);
		sizeReplyBuf = 0;
		break;

	case M_ENABLE_PA_PRE_DIST_CMD_ID:
		status = processEnablePAPreDistCmd(dkThread);
		sizeReplyBuf = 0;
		break;
	
	case M_DUMP_REGS_CMD_ID:
		status = processDumpRegsCmd(dkThread);
		sizeReplyBuf = 0;
		break;

	case M_DUMP_PCI_WRITES_CMD_ID:
		status = processDumpPciWritesCmd(dkThread);
		sizeReplyBuf = 0;
		break;

	case M_TEST_LIB_CMD_ID:
		status = processTestLibCmd(dkThread);
		sizeReplyBuf = 4;
		break;

	case M_SPECIFY_SUBSYSTEM_CMD_ID:
		status = processSpecifySubsystemCmd(dkThread);
		sizeReplyBuf = 0;
		break;

	case M_DISPLAY_FIELD_VALUES_CMD_ID:
		status = processDisplayFieldValuesCmd(dkThread);
		sizeReplyBuf = 8;
		break;

	case M_GET_FIELD_VALUE_CMD_ID:
		status = processGetFieldValueCmd(dkThread);
		sizeReplyBuf = 8;
		break;

	case M_GET_FIELD_FOR_MODE_CMD_ID:
		status = processGetFieldForModeCmd(dkThread);
		sizeReplyBuf = 4;
		break;

	case M_READ_FIELD_CMD_ID:
		status = processReadFieldCmd(dkThread);
		sizeReplyBuf = 12;
		break;

	case M_WRITE_FIELD_CMD_ID:
		status = processWriteFieldCmd(dkThread);
		sizeReplyBuf = 0;
		break;

	case M_SET_RESET_PARAMS_CMD_ID:
		status = processSetResetParamsCmd(dkThread);
		sizeReplyBuf = 0;
		break;

	case M_CHANGE_MULTIPLE_FIELDS_ALL_MODES_CMD_ID:
		status = processChangeMultiFieldsAllModes(dkThread);
		sizeReplyBuf = 0;
		break;
	
	case M_CHANGE_MULTIPLE_FIELDS_CMD_ID:
		status = processChangeMultiFields(dkThread);
		sizeReplyBuf = 0;
		break;

	case M_CHANGE_ADDAC_FIELD_CMD_ID:
		status = processChangeAddacField(dkThread);
		sizeReplyBuf = 0;
		break;

	case M_SAVE_XPA_BIAS_FREQ_CMD_ID:
		status = processSaveXpaBiasLvlFreq(dkThread);
		sizeReplyBuf = 0;
		break;

	case M_FORCE_SINGLE_PCDAC_TABLE_CMD_ID:
		status = processForceSinglePCDACTableCmd(dkThread);
		sizeReplyBuf = 0;
		break;

	case M_FORCE_SINGLE_PCDAC_TABLE_GRIFFIN_CMD_ID:
		status = processForceSinglePCDACTableGriffinCmd(dkThread);
		sizeReplyBuf = 0;
		break;

	case M_FORCE_PCDAC_TABLE_CMD_ID:
		status = processForcePCDACTableCmd(dkThread);
		sizeReplyBuf = 0;
		break;

	case M_FALSE_DETECT_BACKOFF_VALS_CMD_ID:
		status = processFalseDetectBackoffValsCmd(dkThread);
		sizeReplyBuf = 0;
		break;

	case M_FORCE_POWER_TX_MAX_CMD_ID:
		status = processForcePowerTxMaxCmd(dkThread);
		sizeReplyBuf = 0;
		break;

	case M_FORCE_SINGLE_POWER_TX_MAX_CMD_ID:
		status = processForceSinglePowerTxMaxCmd(dkThread);
		sizeReplyBuf = 0;
		break;

	case M_GET_MAC_ADDR_CMD_ID :
		status = processGetMacAddr(dkThread);
		sizeReplyBuf = 6;
		break;

	case M_GET_EEPROM_STRUCT_CMD_ID:
		status = processGetEepromStructCmd(dkThread);

	#ifdef ENDIAN_SWAP
		// need to swap the endianness again
		sizeReplyBuf = ltob_l(*((A_INT32 *)ReplyCmd_p->cmdBytes));
	#else
		sizeReplyBuf = *((A_INT32 *)ReplyCmd_p->cmdBytes);
	#endif
		sizeReplyBuf += 4;
		break;

	case M_GET_DEVICE_INFO_CMD_ID:
		status = processGetDeviceInfoCmd(dkThread);
		sizeReplyBuf = sizeof(SUB_DEV_INFO);
		break;


	case M_FTP_DOWNLOAD_FILE_CMD_ID:
		status = processFtpDownloadFileCmd(dkThread);
		sizeReplyBuf = 0;
		break;

	case M_SET_QUEUE_CMD_ID:
		status = processSetQueueCmd(dkThread);
		sizeReplyBuf = 0;
		break;

	case M_MAP_QUEUE_CMD_ID:
		status = processMapQueueCmd(dkThread);
		sizeReplyBuf = 0;
		break;

	case M_CLEAR_KEY_CACHE_CMD_ID:
		status = processClearKeyCacheCmd(dkThread);
		sizeReplyBuf = 0;
		break;

	case RUN_SCREENING_TEST_CMD_ID:
		status = processRunScreeningTestCmd(dkThread);
		sizeReplyBuf = 4;
		break;

	case ENABLE_HW_CAL_CMD:
		status = processEnableHwCalCmd(dkThread);
		sizeReplyBuf = 0;
		break;

	case M_MASK_TRIGGER_SWEEP_CMD_ID:
		status = processMaskTriggerSweepCmd(dkThread);
		//sizeReplyBuf = pCmd->CMD_U.MASK_TRIGGER_SWEEP_CMD.ret_length*sizeof(double) + sizeof(A_UINT32);
		// list of doubles expressed as 32 char strings due to endian swap
		sizeReplyBuf = pCmd->CMD_U.MASK_TRIGGER_SWEEP_CMD.ret_length*32*sizeof(A_UCHAR) + sizeof(A_UINT32);
		break;

	case M_MASK_DETECT_SIGNAL_CMD_ID:
		status = processMaskDetectSignalCmd(dkThread);
		//sizeReplyBuf = pCmd->CMD_U.MASK_DETECT_SIGNAL_CMD.ret_length*sizeof(A_UINT32) + sizeof(double);
		// double expressed as 32 char string due to endian swap
		sizeReplyBuf = pCmd->CMD_U.MASK_DETECT_SIGNAL_CMD.ret_length*sizeof(A_UINT32) + 32*sizeof(A_UCHAR);
		break;

	case M_MASK_CONFIG_CAPTURE_CMD_ID:
		status = processMaskConfigCaptureCmd(dkThread);
		sizeReplyBuf = 0;
		break;

	case M_MASK_SET_DEV_NUMS_CMD_ID:
		status = processMaskSetDevNumsCmd(dkThread);
		sizeReplyBuf = 0;
		break;

	case M_MASK_FORCE_MIN_CCAPWR_CMD_ID:
		status = processMaskForceMinCCAPowerCmd(dkThread);
		sizeReplyBuf = 0;
		break;

	case M_SET_LIB_CONFIG_CMD_ID:
		status = processSetLibConfigCmd(dkThread);
		sizeReplyBuf = 0;
		break;

	case M_SELECT_DEV_NUM_CMD_ID:
		status = processSelectDevNumCmd(dkThread);
		sizeReplyBuf = 0;
		break;

	case M_GET_MAX_LIN_PWR_CMD_ID:
		status = processGetMaxLinPwrCmd(dkThread);
		sizeReplyBuf = 4;
		break;

	case IQ_CAL_CMD_ID:
		status = processIqCal(dkThread);
		sizeReplyBuf = sizeof(IQ_FACTOR);
		break;
    case M_PA_OFFSET_CAL_CMD_ID:
		status = processPAOffsetCalCmd(dkThread);
		sizeReplyBuf = 0;
		break;
		
	case M_TEMPCOMP_CMD_ID:
		status = processTempCompCmd(dkThread);
		sizeReplyBuf = 0;
		break;
        

#endif // THIN_CLIENT_BUILD

    default:
         uiPrintf("Error: Illegal commandID!\n");
         //return A_ERROR;
	     status = 1;
    	}
    } // end of if(status!=COMMS_ERR_BAD_LENGTH...

	ReplyCmd_p->status = status << COMMS_ERR_SHIFT;

	// set the reply buf size to 0 in case of error
	if ((ReplyCmd_p->status && COMMS_ERR_MASK) != CMD_OK) sizeReplyBuf = 0;

#if defined(THIN_CLIENT_BUILD)
#else // THIN_CLIENT_BUILD
	// Return MDK_ERROR if mdkErrno is set and the command status is OK
	//first get the devNum for the currentDevIndex
	for (i = 0; i < MAX_CLIENTS_PER_THREAD; i++) {
		if(dkThread->currentDevIndex == dkThread->devIndex[i]) {
			break;
		}
	}

	if(i != MAX_CLIENTS_PER_THREAD) {
		errNo = getMdkErrNo(dkThread->devNum[i]);
		if (((ReplyCmd_p->status && COMMS_ERR_MASK) == CMD_OK) && 
			(errNo != 0))
		{
			ReplyCmd_p->status = (errNo << COMMS_ERR_INFO_SHIFT) |
						 (COMMS_ERR_MDK_ERROR << COMMS_ERR_SHIFT);

			// copy the error string in the return buffer
			getMdkErrStr(dkThread->devNum[i], errStr);
			strncpy((A_CHAR *)ReplyCmd_p->cmdBytes,errStr,SIZE_ERROR_BUFFER);
			sizeReplyBuf = SIZE_ERROR_BUFFER;
//			printf("SNOOP: error handling, errno = %d, errStr = %s, size = %d\n", errNo, errStr, sizeReplyBuf); 
		}
	}
#endif // THIN_CLIENT_BUILD


    if ( !pOSSock->sockDisconnect && !pOSSock->sockClose ) {
        // create the reply to the command
	   ReplyCmd_p->replyCmdId = pCmd->cmdID;

        // expand the size of the command reply to include the commandId and status
	// dont include the len field in calculation
        sizeReplyBuf += (sizeof(ReplyCmd_p->replyCmdId) + sizeof(ReplyCmd_p->status));
	
	// put the size in the cmdLen field
	ReplyCmd_p->replyCmdLen = sizeReplyBuf;


#ifdef ENDIAN_SWAP
	ReplyCmd_p->replyCmdLen = btol_l(ReplyCmd_p->replyCmdLen);
	ReplyCmd_p->replyCmdId = btol_l(ReplyCmd_p->replyCmdId);
	ReplyCmd_p->status = btol_l(ReplyCmd_p->status);
#endif

	// send the reply
	if (osSockWrite(pOSSock, (A_UINT8*)ReplyCmd_p, (sizeReplyBuf+sizeof(ReplyCmd_p->replyCmdLen)))) {
                goodWrite = TRUE;
	}

    //uiPrintf("WD:\n");
        if ( !goodWrite ) {
            uiPrintf("Error: Unable to write reply to master!!\n");
            return A_ERROR;
        }
    }
    
    return A_OK;
}


void initThreadInfo(struct dkThreadInfo *dkThread) 
{
	int i;

	dkThread->inUse = 0;
	dkThread->threadId = 0;
	dkThread->pOSSock = NULL;
	dkThread->PipeBuffer_p = NULL;
	dkThread->ReplyCmd_p = NULL;
	dkThread->numClients = 0;
	dkThread->currentDevIndex = (A_UINT32)INVALID_ENTRY;

	for (i=0;i<MAX_CLIENTS_PER_THREAD;i++) {
		dkThread->devIndex[i] = (A_UINT32)INVALID_ENTRY;
		dkThread->devNum[i] = INVALID_ENTRY;
	}
	
	return;	
}

void initThreadArray()
{
	A_INT32 i;
	
	numThreads = 0;
	for (i=0;i<MAX_THREADS;i++) {
		initThreadInfo(&dkThreadArray[i]);
				
	}
	
	return;
}


struct dkThreadInfo *getThreadEntry()
{
	A_INT32 i;

	if (numThreads < MAX_THREADS) {
		for (i=0;i<MAX_THREADS;i++) {
			if (dkThreadArray[i].inUse == 0) {
				dkThreadArray[i].inUse = 1;
				numThreads++;
				return (&dkThreadArray[i]);
			}
		}
	}
	
	return NULL;
}

void putThreadEntry(struct dkThreadInfo *dkThread)
{
	numThreads--;
	initThreadInfo(dkThread);
}

A_INT32  addClientToThread
(
	struct dkThreadInfo *dkThread,
	A_UINT16 devIndex,
	A_UINT32 devNum
)
{
	A_INT32 i;

	if (dkThread->numClients < MAX_CLIENTS_PER_THREAD) {
		for (i=0;i<MAX_CLIENTS_PER_THREAD;i++) {
			if (dkThread->devIndex[i] == (A_UINT32)INVALID_ENTRY) {
				dkThread->devIndex[i]=(A_UINT32)devIndex;
				dkThread->devNum[i]=(A_INT32) devNum;
				dkThread->numClients++;
				return i;
			}
		}
	}

	return -1;
}

void removeClientFromThread
(
	struct dkThreadInfo *dkThread,
	A_UINT16 devIndex
)
{
	A_INT32 i;

	for (i=0;i<MAX_CLIENTS_PER_THREAD;i++) {
		if (dkThread->devIndex[i] == devIndex) {
			dkThread->devIndex[i]=(A_UINT32)INVALID_ENTRY;
			dkThread->devNum[i]=INVALID_ENTRY;
			dkThread->numClients--;
		}
	}
}
