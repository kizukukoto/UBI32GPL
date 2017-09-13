/* mld.h - macros and definitions for sim environment hardware access */

/* Copyright (c) 2001 Atheros Communications, Inc., All Rights Reserved */

#ident  "ACI $Id: //depot/sw/src/dk/mdk/devmld/mld_linux.h#12 $, $Header: //depot/sw/src/dk/mdk/devmld/mld_linux.h#12 $"

#ifndef __INCmldh
#define __INCmldh
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* includes */
#include "wlantype.h"

#define DRV_MAJOR_VERSION	1
#define DRV_MINOR_VERSION	2

#define MAX_BARS    6

/* defines */
#define INTERRUPT_F2    1
#define TIMEOUT         4
#define ISR_INTERRUPT   0x10
#define DEFAULT_TIMEOUT 0xff

#define A_MEM_ZERO(addr, len) memset(addr, 0, len)

#define F2_VENDOR_ID			0x168C		/* vendor ID for our device */
#define MAX_REG_OFFSET			0xfffc	    /* maximum platform register offset */
#define MAX_CFG_OFFSET          256         /* maximum locations for PCI config space per device */
#define MAX_MEMREAD_BYTES       2048        /* maximum of 2k location per OSmemRead action */

/* PCI Config space mapping */
#define F2_PCI_CMD				0x04		/* address of F2 PCI config command reg */
#define F2_PCI_CACHELINESIZE    0x0C        /* address of F2 PCI cache line size value */
#define F2_PCI_LATENCYTIMER     0x0D        /* address of F2 PCI Latency Timer value */
#define F2_PCI_BAR				0x10		/* address of F2 PCI config BAR register */
#define F2_PCI_BAR0_REG         0x10
#define F2_PCI_INTLINE          0x3C        /* address of F2 PCI Interrupt Line reg */
/* PCI Config space bitmaps */
#define MEM_ACCESS_ENABLE		0x002       /* bit mask to enable mem access for PCI */
#define MASTER_ENABLE           0x004       /* bit mask to enable bus mastering for PCI */
#define MEM_WRITE_INVALIDATE    0x010       /* bit mask to enable write and invalidate combos for PCI */
#define SYSTEMERROR_ENABLE      0x100		/* bit mask to enable system error */

#define BUFF_BLOCK_SIZE			0x100  /* size of a buffer block */
		

typedef struct eHandle {
    A_UINT16 eventID;
    A_UINT16 f2Handle;
} EVT_HANDLE;

typedef struct eventStruct {
    EVT_HANDLE          eventHandle;
    A_UINT32            type;
    A_UINT32            persistent;
    A_UINT32            param1;
    A_UINT32            param2;
    A_UINT32            param3;
    A_UINT32            result[6];
} EVENT_STRUCT;


#define WLAN_MAX_DEV	8		/* Number of maximum supported devices */

/* holds all the dk specific information within DEV_INFO structure */
typedef struct dkDevInfo {
	A_UINT16	devIndex;	/* used to track which F2 within system this is */
	A_UINT16	f2Mapped;		    /* true if the f2 registers are mapped */
	A_UINT16	devMapped;		    /* true if the f2 registers are mapped */
	A_UINT32 	f2MapAddress;
	A_UINT32	regVirAddr;
	A_UINT32	regMapRange;
	A_UINT32	memPhyAddr;
	A_UINT32	memVirAddr;
	A_UINT32	memSize;
	A_BOOL		haveEvent;
    A_UINT32    aregPhyAddr[MAX_BARS];
    A_UINT32    aregVirAddr[MAX_BARS];
    A_UINT32    aregRange[MAX_BARS];
    A_UINT16    bar_select;
    A_UINT16    numBars;
} DK_DEV_INFO;

/* WLAN_DRIVER_INFO structure will hold the driver global information.
 *
 */
typedef struct mdk_wlanDrvInfo {
	A_UINT32           devCount;                     /* No. of currently connected devices */
	struct mdk_wlanDevInfo *pDevInfoArray[WLAN_MAX_DEV]; /* Array of devinfo pointers */
} MDK_WLAN_DRV_INFO;

/*
 * MDK_WLAN_DEV_INFO structure will hold all kinds of device related information.
 * It will hold OS specific information about the device and a device number.
 * Most of the code doesn't need to know what's inside that structure.
 * The fields are very likely to change.
 */

typedef	struct	mdk_wlanDevInfo
{
	DK_DEV_INFO *   	pdkInfo;            /* pointer to structure containing info for dk */
	A_UINT32			handle;
} MDK_WLAN_DEV_INFO;

// SOCK_INFO contains the socket information for commuincating with AP

typedef struct artSockInfo {
    A_CHAR 	 hostname[128];
    A_INT32  port_num;
    A_UINT32 ip_addr;
    A_INT32  sockfd;
} ART_SOCK_INFO;

/////////////////////////////////////
// function declarations

A_UINT32 hwMemRead32(A_UINT16 devIndex, A_UINT32 memAddress);
void hwMemWrite32(A_UINT16 devIndex, A_UINT32 memAddress, A_UINT32 writeValue);
A_UINT32 hwCfgRead32(A_UINT16 devIndex, A_UINT32 address);
void hwCfgWrite32(A_UINT16 devIndex, A_UINT32 address, A_UINT32 writeValue);
A_INT16 hwMemWriteBlock(A_UINT16 devIndex,A_UCHAR *pBuffer, A_UINT32 length, A_UINT32 *pPhysAddr);
A_INT16 hwMemReadBlock(A_UINT16 devIndex,A_UCHAR *pBuffer, A_UINT32 physAddr, A_UINT32 length);
A_INT16 hwCreateEvent(A_UINT16 devIndex, A_UINT32 type, A_UINT32 persistent, A_UINT32 param1, A_UINT32 param2,
	A_UINT32 param3, EVT_HANDLE eventHandle);
A_STATUS envInit(A_BOOL debugMode,A_BOOL openDriver);
void envCleanup(A_BOOL closeDriver);
A_STATUS deviceInit(A_UINT16 devIndex);
void deviceCleanup(A_UINT16 devIndex);
A_UINT16 getNextEvent(A_UINT16 devIndex, EVENT_STRUCT *pEvent);

#define Sleep(x) milliSleep(x)
extern void milliSleep(A_UINT32 millitime); 
extern A_UINT32	milliTime();

A_UINT16 uilog(char *, A_BOOL);
A_UINT16 uiWriteToLog(char *string);
void uilogClose(void);
void dk_quiet(A_UINT16 Mode);
A_INT32 uiPrintf(const char * format, ...);
A_INT32 q_uiPrintf(const char * format, ...);
void configPrint(A_BOOL flag);

#ifndef A_ASSERT
  #include <assert.h>
  #define A_ASSERT assert
#endif
#define A_MALLOC(a)	(malloc(a))
#define A_FREE(a)	(free(a))

// Global Externs
extern MDK_WLAN_DRV_INFO	globDrvInfo;	            /* Global driver data structure */

#ifdef __cplusplus
}
#endif /* __cplusplus */
		
#endif /* __INCmldh */
