
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>
#include <assert.h>
#include <time.h>

#ifdef LINUX
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <stdarg.h>
#include <signal.h>
#endif

#ifdef ANWI
#include <windows.h>
#include <malloc.h>
#include <process.h>
#include <winioctl.h>
#ifndef LEGACY
#include <initguid.h>
#include <setupapi.h>
#endif

#endif

#ifdef __ATH_DJGPPDOS__
#include <dos.h>
#include <io.h>
//#include <limits.h>
//#include <stdio.h>
//#include <stdlib.h>
//#include <string.h>
#include <stdarg.h>
#include <unistd.h>

#define __int64	long long
#define HANDLE long
typedef unsigned long DWORD;
#define Sleep	delay
#endif	// #ifdef __ATH_DJGPPDOS__

#include "wlantype.h"
#include "dw16550reg.h"

#include "athreg.h"

#include "common_hw.h"
#include "dk_mem.h"

#ifdef SIM
#include "sim.h"
#endif
#include "dk_common.h"

#ifdef OWL_AP
#include "endian_func.h"
#endif

/////////////////////////////////////////////////////////////////////////////
//GLOBAL VARIABLES

DRV_VERSION_INFO driverVer;

static FILE     *logFile;  /* file handle of logfile */    
static FILE     *yieldLogFile;  /* file handle of yieldLogfile */    
static A_BOOL   logging;   /* set to 1 if a log file open */
static A_BOOL   enablePrint = 1;
static A_BOOL   yieldLogging = 0;   /* set to 1 if a log file open */
static A_BOOL   yieldEnablePrint = 1;
static A_UINT16 quietMode; /* set to 1 for quiet mode, 0 for not */
static A_UINT16 yieldQuietMode; /* set to 1 for quiet mode, 0 for not */
static A_BOOL	driverOpened;

A_UCHAR			*pbuffMapBytes=NULL;
A_UINT16		*pnumBuffs=NULL;

MDK_WLAN_DRV_INFO	globDrvInfo;				/* Global driver info */

/////////////////////////////////////////////////////////////////////////////
// FORWARD DECLARATIONS
void        deviceCleanup(A_UINT16 devIndex);
DLL_EXPORT void dk_quiet ( A_UINT16 Mode);

// Extern declarations
extern A_UINT16    driverOpen();
extern A_STATUS connectSigHandler();
extern void close_device(MDK_WLAN_DEV_INFO *pdevInfo);
extern void close_driver();
extern A_STATUS get_device_client_info(MDK_WLAN_DEV_INFO *pdevInfo, PDRV_VERSION_INFO pdrv, PCLI_INFO cliInfo);


/**************************************************************************
* envInit - performs any intialization needed by the environment
*
* For the windows NT hardware environment, need to open the driver and
* perform any other initialization required by it
*
* RETURNS: A_OK if it works, A_ERROR if not
*/
#ifdef ART_BUILD
A_STATUS envInit ( A_BOOL debugMode, A_BOOL openDriver)
#else
A_STATUS envInit ( A_BOOL debugMode) 
#endif
{
    A_UINT16 i;
#ifdef ART_BUILD
#else
//Define openDriver as it would not be defined for MDK
   A_BOOL openDriver=1;
#endif
	
    // quiet our optional uiPrintfs...
    dk_quiet((A_UINT16)(debugMode ? 0 : 1));

#if defined(LINUX ) && !defined(SOC_LINUX)
   ansi_init();
#endif

    // open a handle to the driver 
	// need not open the driver if art is controlling a remote client
	driverOpened = 0;
	if (openDriver) {
		if (!driverOpen()) {
			return A_ERROR;
		}
		driverOpened = 1;
	}
	
    globDrvInfo.devCount = 0;

    // set all the devInfo pointers to null
    for(i = 0; i < WLAN_MAX_DEV; i++)
        globDrvInfo.pDevInfoArray[i] = NULL;

    return connectSigHandler();
}


void envCleanup
    (
    A_BOOL closeDriver
    )
{
    A_UINT16 i;

    //uiPrintf("SNOOP:::envCleanup called \n");
#ifdef SIM
	signal(SIGINT, SIG_DFL);
#endif

    // cleanup all the devInfo structures
    for ( i = 0; i < WLAN_MAX_DEV; i++ ) {
        if ( globDrvInfo.pDevInfoArray[i] ) {
            deviceCleanup(i);
         }
    }
 
    globDrvInfo.devCount = 0;

    // close the handle to the driver
    if ((closeDriver) && (driverOpened)) {
            close_driver();
		driverOpened = 0;
    }
}

#ifndef SOC_LINUX
void hwInit ( MDK_WLAN_DEV_INFO *pdevInfo, A_UINT32 resetMask)
{
    A_UINT32    reg=0, i;
    A_UINT32    devmapRegAddress = pdevInfo->pdkInfo->f2MapAddress;
    A_UINT16 devIndex = (A_UINT16)pdevInfo->pdkInfo->devIndex;
    A_UINT16 hwDevId;

    hwDevId = (A_UINT16)( (hwCfgRead32(devIndex, 0) >> 16) & 0xffff);

	/* Bring out of sleep mode */
	hwMemWrite32(devIndex, F2_SCR + devmapRegAddress, F2_SCR_SLE_FWAKE);
	mSleep(10);

	reg = 0;
    if (hwDevId && 0xff) {
	  if (resetMask & MAC_RESET) {
		reg = reg | F2_RC_MAC | F2_RC_BB | F2_RC_RESV0; 
	  }
	  if (resetMask & BB_RESET) {
		reg = reg | F2_RC_RESV1; 
	  }
	  if (resetMask & BUS_RESET) {
		reg = reg | F2_RC_PCI; 
      }
    }
    else {
	  if (resetMask & MAC_RESET) {
		reg = reg | F2_RC_MAC; 
	  }
	  if (resetMask & BB_RESET) {
		reg = reg | F2_RC_BB; 
	  }
	  if (resetMask & BUS_RESET) {
		reg = reg | F2_RC_PCI; 
	  }
    }

	hwMemRead32(devIndex, F2_RXDP+devmapRegAddress);  // To clear any pending writes, as per doc/mac_registers.txt
	hwMemWrite32(devIndex, F2_RC+devmapRegAddress, reg);
	mSleep(1);
	//workaround for hainan 1.0
	if((hwMemRead32(devIndex, F2_SREV+devmapRegAddress) & F2_SREV_ID_M)== 0x55) {
		for (i = 0; i < 20; i++) {
			hwMemRead32(devIndex, 0x4020);
		}
	}
//	mSleep(10);


	if(hwMemRead32(devIndex, F2_RC+devmapRegAddress) == 0) {
		uiPrintf("hwInit: Device did not enter Reset \n");
		return;
	}

	/* Bring out of sleep mode again */
   	hwMemWrite32(devIndex, F2_SCR+devmapRegAddress, F2_SCR_SLE_FWAKE);
	mSleep(10);

	/* Clear the reset */
	hwMemWrite32(devIndex, F2_RC+devmapRegAddress, 0);
	mSleep(10);

}
#endif


/**************************************************************************
* deviceInit - performs any initialization needed for a device
*
* Perform the initialization needed for a device.  This includes creating a 
* devInfo structure and initializing its contents
*
* RETURNS: A_OK if successful, A_ERROR if not
*/
A_STATUS deviceInit(
    A_UINT16 devIndex, /* index of globalDrvInfo which to add device to */
    A_UINT16 device_fn,
    DK_DEV_INFO *pdkInfo)
{
    MDK_WLAN_DEV_INFO *pdevInfo;
    CLI_INFO cliInfo;
    A_UINT32 iIndex;
	A_UINT32 NumBuffBlocks;
	A_UINT32 NumBuffMapBytes;

    /* check to see if we already have a devInfo structure created for this device */
    if (globDrvInfo.pDevInfoArray[devIndex]) {
        uiPrintf("Error : Device already in use \n");
        return A_ERROR;
	}

      pdevInfo = (MDK_WLAN_DEV_INFO *) A_MALLOC(sizeof(MDK_WLAN_DEV_INFO));
      if(!pdevInfo) {                                            
        uiPrintf("Error: Unable to allocate MDK_WLAN_DEV_INFO struct!\n");
        return(A_ERROR);
      }
#ifdef WIN32
      pdevInfo->hDevice = NULL;
#else
      pdevInfo->hDevice = -1;
#endif

      pdevInfo->pdkInfo = (DK_DEV_INFO *) A_MALLOC(sizeof(DK_DEV_INFO));
      if(!pdevInfo->pdkInfo) {
        A_FREE(pdevInfo);
        uiPrintf("Error: Unable to allocate DK_DEV_INFO struct!\n");
        return A_ERROR;
      }
      //uiPrintf("pdevInfo->pdkInfo=%x:\npdevInfo=%x:\n", pdevInfo->pdkInfo, pdevInfo);
    if (!pdkInfo) {
	  /* zero out the dkInfo struct */
	  A_MEM_ZERO(pdevInfo->pdkInfo, sizeof(DK_DEV_INFO));

	  pdevInfo->pdkInfo->devIndex = devIndex;
	  pdevInfo->pdkInfo->device_fn = device_fn;

	  cliInfo.numBars = 1;    //apparently anwi driver does not update this for eagle
	                          //make sure it is initialized to a good default  
      if (get_device_client_info(pdevInfo, &driverVer, &cliInfo) == A_ERROR) {
         return A_ERROR;
      }

	  pdevInfo->pdkInfo->f2Mapped = 1;
      if (driverVer.minorVersion >= 2) {
         pdevInfo->pdkInfo->f2MapAddress = cliInfo.aregPhyAddr[0];
         pdevInfo->pdkInfo->regMapRange = cliInfo.aregRange[0];
         pdevInfo->pdkInfo->regVirAddr = cliInfo.aregVirAddr[0];
         for(iIndex=0; iIndex<cliInfo.numBars; iIndex++) {
            pdevInfo->pdkInfo->aregPhyAddr[iIndex] = cliInfo.aregPhyAddr[iIndex];
            pdevInfo->pdkInfo->aregVirAddr[iIndex] = cliInfo.aregVirAddr[iIndex];
            pdevInfo->pdkInfo->aregRange[iIndex] = cliInfo.aregRange[iIndex];
            pdevInfo->pdkInfo->res_type[iIndex] = cliInfo.res_type[iIndex];
         }
      }
      else {
         pdevInfo->pdkInfo->aregPhyAddr[0] = cliInfo.regPhyAddr;
         pdevInfo->pdkInfo->aregVirAddr[0] = cliInfo.regVirAddr;
         pdevInfo->pdkInfo->aregRange[0] = cliInfo.regRange;
         pdevInfo->pdkInfo->f2MapAddress = cliInfo.regPhyAddr;
         pdevInfo->pdkInfo->regMapRange = cliInfo.regRange;
         pdevInfo->pdkInfo->regVirAddr = cliInfo.regVirAddr;
      }
      pdevInfo->pdkInfo->numBars = (A_UINT16)cliInfo.numBars;

	  pdevInfo->pdkInfo->memPhyAddr = cliInfo.memPhyAddr;
	  pdevInfo->pdkInfo->memVirAddr = cliInfo.memVirAddr;
	  pdevInfo->pdkInfo->memSize = cliInfo.memSize;
	  pdevInfo->pdkInfo->bar_select = 0;
    }
    else {
      memcpy(pdevInfo->pdkInfo, pdkInfo, sizeof(DK_DEV_INFO));
	  uiPrintf("Client Version = %d.%d Build %d\n", ((pdevInfo->pdkInfo->version&0xfff0)>> 4), (pdevInfo->pdkInfo->version & 0xf), (pdevInfo->pdkInfo->version >> 16));
    }
    
      q_uiPrintf("Num bars = %d\n", pdevInfo->pdkInfo->numBars);
      q_uiPrintf("+ f2MapAddress = %08x\n", (A_UINT32)(pdevInfo->pdkInfo->f2MapAddress));
		iIndex = 0;
      for(iIndex=0; iIndex<pdevInfo->pdkInfo->numBars; iIndex++) {
        q_uiPrintf("+   aregPhyAddr[%d]= %08x\n", iIndex, (A_UINT32)(pdevInfo->pdkInfo->aregPhyAddr[iIndex]));
        q_uiPrintf("+   aregVirAddr[%d]= %08x\n", iIndex, (A_UINT32)(pdevInfo->pdkInfo->aregVirAddr[iIndex]));
        q_uiPrintf("+   aregRange[%d]= %08x\n", iIndex, (A_UINT32)(pdevInfo->pdkInfo->aregRange[iIndex]));
      }
      q_uiPrintf("+ Allocated memory in the driver.\n");
      q_uiPrintf("+ VirtAddress = %08x\n", (A_UINT32)(pdevInfo->pdkInfo->memVirAddr));
      q_uiPrintf("+ PhysAddress = %08x\n", (A_UINT32)(pdevInfo->pdkInfo->memPhyAddr));
      q_uiPrintf("+ Size = %08x\n", (A_UINT32)(pdevInfo->pdkInfo->memSize));


#ifdef ART_BUILD
	NumBuffBlocks = 0;  // to quiet compiler warnings
	NumBuffMapBytes = 0;  // to quiet compiler warnings
#else
	NumBuffBlocks	= pdevInfo->pdkInfo->memSize / BUFF_BLOCK_SIZE;
	NumBuffMapBytes = NumBuffBlocks / 8;

	/* initialize the map bytes for tracking memory management: calloc will init to 0 as well*/
#ifdef SIM
    if (pbuffMapBytes == NULL)
         pbuffMapBytes = (A_UCHAR *) calloc(NumBuffMapBytes, sizeof(A_UCHAR));
    pdevInfo->pbuffMapBytes = pbuffMapBytes;
#else
    pdevInfo->pbuffMapBytes = (A_UCHAR *) calloc(NumBuffMapBytes, sizeof(A_UCHAR));
#endif
    if(!pdevInfo->pbuffMapBytes) {
        close_device(pdevInfo);
		A_FREE(pdevInfo->pdkInfo);
		A_FREE(pdevInfo);
        uiPrintf("Error: Unable to allocate buffMapBytes struct!\n");
        return A_ERROR;
    }

#ifdef SIM
    if (pnumBuffs == NULL)
        pnumBuffs = (A_UINT16 *) calloc(NumBuffBlocks, sizeof(A_UINT16));
    pdevInfo->pnumBuffs = pnumBuffs;
#else
    pdevInfo->pnumBuffs = (A_UINT16 *) calloc(NumBuffBlocks, sizeof(A_UINT16));
#endif
    if(!pdevInfo->pnumBuffs) {
		A_FREE(pdevInfo->pbuffMapBytes);
        close_device(pdevInfo);
		A_FREE(pdevInfo->pdkInfo);
		A_FREE(pdevInfo);
        uiPrintf("Error: Unable to allocate numBuffs struct!\n");
        return A_ERROR;
    }

#endif

    globDrvInfo.pDevInfoArray[devIndex] = pdevInfo;
    globDrvInfo.devCount++;

#ifdef OWL_PB42
if(!isHowlAP(devIndex)){
    A_UINT32  regValue;
    if (!pdkInfo)  {  // pdkInfo will be non-zero only for thin/usb clients
  	  // Setup memory window, bus mastering, & SERR
      regValue = hwCfgRead32(devIndex, F2_PCI_CMD);
      regValue |= (MEM_ACCESS_ENABLE | MASTER_ENABLE | SYSTEMERROR_ENABLE); 
	  regValue &= ~MEM_WRITE_INVALIDATE; // Disable write & invalidate for our device
      hwCfgWrite32(devIndex, F2_PCI_CMD, regValue);

      regValue = hwCfgRead32(devIndex, F2_PCI_CACHELINESIZE);
      regValue = (regValue & 0xffff) | (0x40 << 8) | 0x08;
	  hwCfgWrite32(devIndex, F2_PCI_CACHELINESIZE, regValue);

#ifdef SOC_LINUX
     //hwInit(pdevInfo, BUS_RESET | BB_RESET | MAC_RESET);
     hwInit(pdevInfo, 0);
#endif
	}
 }
#endif
            
	/*
	  // Test memaccesses
	  uiPrintf("Testing Reg accesses\n");
	  hwMemWrite32(devIndex, F2_RXDP+pdevInfo->pdkInfo->f2MapAddress, 0xbabecafc);  
	  uiPrintf("RXDP Content exp=0xbabecafc:act=%x\n", hwMemRead32(devIndex, F2_RXDP+pdevInfo->pdkInfo->f2MapAddress));
	  uiPrintf("Testing Memory accesses\n");
	  hwMemWrite32(devIndex, pdevInfo->pdkInfo->memPhyAddr + 0x100, 0xdeadbeef);  
	  uiPrintf("Memory  Content exp=0xdeadbeef:act=%x\n", hwMemRead32(devIndex, pdevInfo->pdkInfo->memPhyAddr + 0x100));
	  */

	  

    return A_OK;
}
/**************************************************************************
* deviceCleanup - performs any memory cleanup needed for a device
*
* Perform any cleanup needed for a device.  This includes deleting any 
* memory allocated by a device, and unregistering the card with the driver
*
* RETURNS: 1 if successful, 0 if not
*/
void deviceCleanup
    (
    A_UINT16 devIndex
    )
{
	MDK_WLAN_DEV_INFO    *pdevInfo;

//printf("SNOOP::deviceCleanup::devIndex=%d\n", devIndex);
	pdevInfo = globDrvInfo.pDevInfoArray[devIndex];
	

#ifdef SOC_LINUX
     hwClose(pdevInfo);
#endif

    close_device(pdevInfo);

    A_FREE(pdevInfo->pdkInfo);
    A_FREE(pdevInfo);

	globDrvInfo.pDevInfoArray[devIndex] = NULL;
    globDrvInfo.devCount--;
//	printf("SNOOP::exit deviceCleanup::devIndex=%d:devCount=%d\n", devIndex, globDrvInfo.devCount);

}


/**************************************************************************
* checkRegSpace - Check to see if an address sits in the setup register space
* 
* This internal routine checks to see if an address lies in the register space
*
* RETURNS: A_OK to signify a valid address or A_ENOENT
*/
A_STATUS checkRegSpace
	(
	MDK_WLAN_DEV_INFO *pdevInfo,
	A_UINT32      address
	)
{
        A_UINT32 iIndex;

        if (driverVer.minorVersion >= 2) {
            for(iIndex=0; iIndex<pdevInfo->pdkInfo->numBars; iIndex++) {
               if((address >= pdevInfo->pdkInfo->aregPhyAddr[iIndex]) && 
                   (address < pdevInfo->pdkInfo->aregPhyAddr[iIndex] + pdevInfo->pdkInfo->aregRange[iIndex])) 
                   return A_OK;
             }
         }
         else {
            if((address >= pdevInfo->pdkInfo->aregPhyAddr[0]) && (address < pdevInfo->pdkInfo->aregPhyAddr[0] + pdevInfo->pdkInfo->regMapRange)) 
            return A_OK;
         }
         return A_ENOENT;
	
}
 
/**************************************************************************
* checkMemSpace - Check to see if an address sits in the setup physical memory space
* 
* This internal routine checks to see if an address lies in the physical memory space
*
* RETURNS: A_OK to signify a valid address or A_ENOENT
*/
A_STATUS checkMemSpace
	(
	MDK_WLAN_DEV_INFO *pdevInfo,
	A_UINT32      address
	)
{
	if((address >= (A_UINT32)pdevInfo->pdkInfo->memPhyAddr) &&
            (address < (A_UINT32)((A_UCHAR *)(pdevInfo->pdkInfo->memPhyAddr) + pdevInfo->pdkInfo->memSize)))
		return A_OK;
	else 
		return A_ENOENT;
}


/**************************************************************************
* checkIOSpace - Check to see if an address sits in the IO register space
* 
* This internal routine checks to see if an address lies in the IO register space
*
* RETURNS: A_OK to signify a valid address or A_ENOENT
*/
A_STATUS checkIOSpace
	(
	MDK_WLAN_DEV_INFO *pdevInfo,
	A_UINT32      address
	)
{
	A_UINT32 i;
		
	for (i=0;i<pdevInfo->pdkInfo->numBars;i++) {
			if (pdevInfo->pdkInfo->res_type[i] == RES_IO) { //check if its IO space
				if ((address >= pdevInfo->pdkInfo->aregPhyAddr[i]) && (address < 
				 		(pdevInfo->pdkInfo->aregPhyAddr[i] + pdevInfo->pdkInfo->aregRange[i]))) {
						return A_OK;
				}
			}
	}
		
	return A_ENOENT;
}

/**************************************************************************
* hwMemRead8 - read an 8 bit value
*
* This routine will call into the simulation environment to activate an
* 8 bit PCI memory read cycle, value read is returned to caller
*
* RETURNS: the value read
*/
A_UINT8 hwMemRead8
(
	A_UINT16 devIndex,
    A_UINT32 address                    /* the address to read from */
)
{
#ifdef SIM
	A_UINT8 value;
#endif
	A_UINT8 *pMem;
    MDK_WLAN_DEV_INFO *pdevInfo;

	pdevInfo = globDrvInfo.pDevInfoArray[devIndex];
    
	// check within the register regions 
	if (A_OK == checkRegSpace(pdevInfo, address))
	{
#ifdef SIM
        PCIsimMemReadB(address, &value);
        return value;
#else
		pMem = (A_UINT8 *) (pdevInfo->pdkInfo->aregVirAddr[pdevInfo->pdkInfo->bar_select] + (address - pdevInfo->pdkInfo->aregPhyAddr[pdevInfo->pdkInfo->bar_select]));
		return (*pMem);
#endif
	}

	//check within memory allocation
	if(A_OK == checkMemSpace(pdevInfo, address))
	{
		pMem = (A_UINT8 *) (pdevInfo->pdkInfo->memVirAddr + 
						  (address - pdevInfo->pdkInfo->memPhyAddr));
		return(*pMem);
	}
 	uiPrintf("ERROR: hwMemRead8 could not access hardware address: %08lx\n", address);
    return (0xdb);
    
}

/**************************************************************************
* hwMemRead16 - read a 16 bit value
*
* This routine will call into the simulation environment to activate a
* 16 bit PCI memory read cycle, value read is returned to caller
*
* RETURNS: the value read
*/
A_UINT16 hwMemRead16
(
	A_UINT16 devIndex,
    A_UINT32 address                    /* the address to read from */
)
{
#ifdef SIM
	A_UINT8 value;
#endif
	A_UINT16 *pMem;
    MDK_WLAN_DEV_INFO *pdevInfo;

	pdevInfo = globDrvInfo.pDevInfoArray[devIndex];
    
	// check within the register regions 
	if (A_OK == checkRegSpace(pdevInfo, address))
	{
#ifdef SIM
        PCIsimMemReadW(address, &value);
        return value;
#else
		pMem = (A_UINT16 *) (pdevInfo->pdkInfo->aregVirAddr[pdevInfo->pdkInfo->bar_select] + (address - pdevInfo->pdkInfo->aregPhyAddr[pdevInfo->pdkInfo->bar_select]));
		return (*pMem);
#endif
	}

	//check within memory allocation
	if(A_OK == checkMemSpace(pdevInfo, address))
	{
		pMem = (A_UINT16 *) (pdevInfo->pdkInfo->memVirAddr + 
						  (address - pdevInfo->pdkInfo->memPhyAddr));
		return(*pMem);
	}

 	uiPrintf("ERROR: hwMemRead16 could not access hardware address: %08lx\n", address);
    return (0xdead);
}

/**************************************************************************_UINT32 temp = 0;
#ifdef SIM
        A_UINT8 value;
#endif
    A_UINT32 *pMem;
        MDK_WLAN_DEV_INFO    *pdevInfo;

        pdevInfo = globDrvInfo.pDevInfoArray[devIndex];

        if (A_OK == checkIOSpace(pdevInfo, address))
        {
            return(hwIORead(devIndex, address));
    }


    // check within the register regions
        if (A_OK == checkRegSpace(pdevInfo, address))
        {
#ifdef SIM
        PCIsimMemReadDW(address, &value);
        return value;
#else
                q_uiPrintf("hwMRd:bs=%d:rva=%x:rpa=%x:addr=%x:\n", pdevInfo->pdkInfo->bar_select, pdevInfo->pdkInfo->aregVirAddr[pdevInfo->pdkInfo->bar_select],  pdevInfo->pdkInfo->aregPhyAddr[pdevInfo->pdkInfo->bar_select], address );

        pMem = (A_UINT32 *) (pdevInfo->pdkInfo->aregVirAddr[pdevInfo->pdkInfo->bar_select] + (address - pdevInfo->pdkInfo->aregPhyAddr[pdevInfo->pdkInfo->bar_select]));

                q_uiPrintf("Reg addr %x \n", pMem);
                q_uiPrintf("Reg Val %x read\n", *(volatile int *)pMem);
                temp = (*(volatile int *)pMem);
#if defined(OWL_AP) && (!defined(OWL_PB42))
                temp = btol_l(temp);
#endif
                return (temp);
                //return(0xbabe);
#endif
        }

        //check within memory allocation
        //if(A_OK == checkMemSpace(pdevInfo, address))
        //{
                uiPrintf("mva=%x:mpa=%x:addr=%x:\n", pdevInfo->pdkInfo->memVirAddr,  pdevInfo->pdkInfo->memPhyAddr, address);
                pMem = (A_UINT32 *) (pdevInfo->pdkInfo->memVirAddr + (address - pdevInfo->pdkInfo->memPhyAddr));
                //pMem= (A_UINT32 *)address;
                 temp = btol_l(*pMem);
                //q_uiPrintf("Mem Val %x read\n", *pMem);
                return(temp);
        //}

        uiPrintf("ERROR: hwMemRead32 could not access hardware address: %08lx\n", address);
    return (0xdeadbeef);
}
* hwMemRead32 - read an 32 bit value
*
* This routine will call into the simulation environment to activate a
* 32 bit PCI memory read cycle, value read is returned to caller
*
* RETURNS: the value read
*/
A_UINT32 hwMemRead32
(
	A_UINT16 devIndex,
    A_UINT32 address                    /* the address to read from */
)

{
    A_UINT32 temp = 0;
#ifdef SIM
	A_UINT8 value;
#endif
    A_UINT32 *pMem;
	MDK_WLAN_DEV_INFO    *pdevInfo;
	pdevInfo = globDrvInfo.pDevInfoArray[devIndex];
	if (A_OK == checkIOSpace(pdevInfo, address))
	{
           return(hwIORead(devIndex, address)); 
    }


    // check within the register regions 
	if (A_OK == checkRegSpace(pdevInfo, address))
	{
#ifdef SIM
        PCIsimMemReadDW(address, &value);
        return value;
#else
		q_uiPrintf("hwMRd:bs=%d:rva=%x:rpa=%x:addr=%x:\n", pdevInfo->pdkInfo->bar_select, pdevInfo->pdkInfo->aregVirAddr[pdevInfo->pdkInfo->bar_select],  pdevInfo->pdkInfo->aregPhyAddr[pdevInfo->pdkInfo->bar_select], address );
#ifdef __UBICOM32__
		temp = apRegRead32(devIndex, address);
#else // __UBICOM32__
        pMem = (A_UINT32 *) (pdevInfo->pdkInfo->aregVirAddr[pdevInfo->pdkInfo->bar_select] + (address - pdevInfo->pdkInfo->aregPhyAddr[pdevInfo->pdkInfo->bar_select]));

		q_uiPrintf("Reg addr %x \n", pMem);
		q_uiPrintf("Reg Val %x read\n", *(volatile int *)pMem);
		temp = (*(volatile int *)pMem);
#endif // __UBICOM32__
#if defined(OWL_AP) && (!defined(OWL_PB42))
		#ifdef ARCH_BIG_ENDIAN
                temp = btol_l(temp);
		#else 
		//temp = btol_l(temp); //RSP-ENDIAN it's essential not to swap here
		#endif
#endif
                return (temp);
		//return(0xbabe);
#endif
	}

#ifdef OWL_PB42
        if (isHowlAP(devIndex) || isFlashCalData())
        {
                          //q_uiPrintf("mva=%x:mpa=%x:addr=%x:\n", pdevInfo->pdkInfo->memVirAddr,  pdevInfo->pdkInfo->memPhyAddr, address);
                q_uiPrintf("mva=%x:mpa=%x:addr=%x:\n", pdevInfo->pdkInfo->memVirAddr,  pdevInfo->pdkInfo->memPhyAddr, address);
                pMem = (A_UINT32 *) (pdevInfo->pdkInfo->memVirAddr +
                                                  (address - pdevInfo->pdkInfo->memPhyAddr));
                q_uiPrintf("Mem Val %x read\n", *pMem);
                temp = btol_l(*pMem);
                return(temp);

        }
#endif

	//check within memory allocation
	if(A_OK == checkMemSpace(pdevInfo, address))
	{
		//q_uiPrintf("mva=%x:mpa=%x:addr=%x:\n", pdevInfo->pdkInfo->memVirAddr,  pdevInfo->pdkInfo->memPhyAddr, address);
		q_uiPrintf("mva=%x:mpa=%x:addr=%x:\n", pdevInfo->pdkInfo->memVirAddr,  pdevInfo->pdkInfo->memPhyAddr, address);
		pMem = (A_UINT32 *) (pdevInfo->pdkInfo->memVirAddr + 
						  (address - pdevInfo->pdkInfo->memPhyAddr));
		q_uiPrintf("Mem Val %x read\n", *pMem);
		return(*pMem);
	}
	
 	uiPrintf("ERROR: hwMemRead32 could not access hardware address: %08lx\n", address);
    return (0xdeadbeef);
}



/**************************************************************************
* hwMemWrite8 - write an 8 bit value
*
* This routine will call into the simulation environment to activate an
* 8 bit PCI memory write cycle
*
* RETURNS: N/A
*/
void hwMemWrite8
(
	A_UINT16 devIndex,
    A_UINT32 address,                    /* the address to write */
    A_UINT8  value                        /* value to write */
)
{
    A_UINT8 *pMem;
    MDK_WLAN_DEV_INFO *pdevInfo;

	pdevInfo = globDrvInfo.pDevInfoArray[devIndex];
   
	// check within the register regions 
	if (A_OK == checkRegSpace(pdevInfo, address))
	{
#ifdef SIM
        PCIsimMemWriteB(address, value);
#else
		pMem = (A_UINT8 *) (pdevInfo->pdkInfo->aregVirAddr[pdevInfo->pdkInfo->bar_select] + (address - pdevInfo->pdkInfo->aregPhyAddr[pdevInfo->pdkInfo->bar_select]));
		*pMem = value;
#endif
		return;
	}
	// check within our malloc area
	if(A_OK == checkMemSpace(pdevInfo, address))
	{
		pMem = (A_UINT8 *) (pdevInfo->pdkInfo->memVirAddr + 
					  (address - pdevInfo->pdkInfo->memPhyAddr));
		*pMem = value;
		return;
	}
   	uiPrintf("ERROR: hwMemWrite8 could not access hardware address: %08lx\n", address);

}

/**************************************************************************
* hwMemWrite16 - write a 16 bit value
*
* This routine will call into the simulation environment to activate a
* 16 bit PCI memory write cycle
*
* RETURNS: N/A
*/
void hwMemWrite16
(
	A_UINT16 devIndex,
    A_UINT32 address,                    /* the address to write */
    A_UINT16  value                        /* value to write */
)
{
    A_UINT16 *pMem;
    MDK_WLAN_DEV_INFO *pdevInfo;

	pdevInfo = globDrvInfo.pDevInfoArray[devIndex];

	// check within the register regions 
	if (A_OK == checkRegSpace(pdevInfo, address))
	{
#ifdef SIM
        PCIsimMemWriteW(address, value);
#else
		pMem = (A_UINT16 *) (pdevInfo->pdkInfo->aregVirAddr[pdevInfo->pdkInfo->bar_select] + (address - pdevInfo->pdkInfo->aregPhyAddr[pdevInfo->pdkInfo->bar_select]));
		*pMem = value;
#endif
		return;
	}

	// check within our malloc area
	if(A_OK == checkMemSpace(pdevInfo, address))
	{
		pMem = (A_UINT16 *) (pdevInfo->pdkInfo->memVirAddr + 
					  (address - pdevInfo->pdkInfo->memPhyAddr));
		*pMem = value;
		return;
	}

   	uiPrintf("ERROR: hwMemWrite16 could not access hardware address: %08lx\n", address);
}

/**************************************************************************
* hwMemWrite32 - write a 32 bit value
*
* This routine will call into the simulation environment to activate a
* 32 bit PCI memory write cycle
*
* RETURNS: N/A
*/
void hwMemWrite32
(
	A_UINT16 devIndex,
    A_UINT32 address,                    /* the address to write */
    A_UINT32  value                        /* value to write */
)


{
    A_UINT32 *pMem;
	MDK_WLAN_DEV_INFO    *pdevInfo;
	pdevInfo = globDrvInfo.pDevInfoArray[devIndex];
	if (A_OK == checkIOSpace(pdevInfo, address))
	{
            hwIOWrite(devIndex, address, value); 
            return;
    }
	// check within the register regions 
	if (A_OK == checkRegSpace(pdevInfo, address))
	{
#ifdef SIM
        PCIsimMemWriteDW(address, value);
#else
#if defined(OWL_AP) && (!defined(OWL_PB42))
		#ifdef ARCH_BIG_ENDIAN
                value = btol_l(value);
		#else 
                // value = btol_l(value); //RSP-ENDIAN it's essential not to swap here
		#endif
#endif
		q_uiPrintf("hwMWr:bs=%d:rva=%x:rpa=%x:addr=%x:value=%x\n", pdevInfo->pdkInfo->bar_select, pdevInfo->pdkInfo->aregVirAddr[pdevInfo->pdkInfo->bar_select],  pdevInfo->pdkInfo->aregPhyAddr[pdevInfo->pdkInfo->bar_select], address , value );
#ifdef __UBICOM32__ 
        apRegWrite32(devIndex, address, value);
#else // __UBICOM32__
        pMem = (A_UINT32 *) (pdevInfo->pdkInfo->aregVirAddr[pdevInfo->pdkInfo->bar_select] + (address - pdevInfo->pdkInfo->aregPhyAddr[pdevInfo->pdkInfo->bar_select]));
		*pMem = value;
#endif // __UBICOM32__
		q_uiPrintf("Reg Val %x written at %x\n", value, pMem);
#endif
		return;
	}

	// check within our malloc area
#ifdef OWL_PB42
        if(isHowlAP(devIndex) || isFlashCalData())
         {
                pMem = (A_UINT32 *) (pdevInfo->pdkInfo->memVirAddr +
                                          (address - pdevInfo->pdkInfo->memPhyAddr));
                q_uiPrintf("mva=%x:mpa=%x:addr=%x:value=%x\n", pdevInfo->pdkInfo->memVirAddr,  pdevInfo->pdkInfo->memPhyAddr, address , value );
                value = btol_l(value);
                *pMem = value;
                q_uiPrintf("Mem Val %x written\n", value);
                return;
        }else{
#endif
	  if(A_OK == checkMemSpace(pdevInfo, address))
	   {
		pMem = (A_UINT32 *) (pdevInfo->pdkInfo->memVirAddr + 
					  (address - pdevInfo->pdkInfo->memPhyAddr));
		q_uiPrintf("mva=%x:mpa=%x:addr=%x:value=%x\n", pdevInfo->pdkInfo->memVirAddr,  pdevInfo->pdkInfo->memPhyAddr, address , value );
		*pMem = value;
		q_uiPrintf("Mem Val %x written\n", value);
		return;
	 }
#ifdef OWL_PB42
	}
#endif

  	uiPrintf("ERROR: hwMemWrite32 could not access hardware address: %08lx\n", address);
}







#ifndef __ATH_DJGPPDOS__
/**************************************************************************
* uiPrintf - print to the perl console
*
* This routine is the equivalent of printf.  It is used such that logging
* capabilities can be added.
*
* RETURNS: same as printf.  Number of characters printed
*/
A_INT32 uiPrintf
    (
    const char * format,
    ...
    )
{
    va_list argList;
    A_INT32     retval = 0;
    A_CHAR    buffer[1024];

    /*if have logging turned on then can also write to a file if needed */

    /* get the arguement list */
    va_start(argList, format);

    /* using vprintf to perform the printing it is the same is printf, only
     * it takes a va_list or arguments
     */
    retval = vprintf(format, argList);
    fflush(stdout);

    if (logging) {
        vsprintf(buffer, format, argList);
        fputs(buffer, logFile);
		fflush(logFile);
    }

    va_end(argList);    /* cleanup arg list */

    return(retval);
}

A_INT32 q_uiPrintf
    (
    const char * format,
    ...
    )
{
    va_list argList;
    A_INT32    retval = 0;
    A_CHAR    buffer[256];
    if ( !quietMode ) {
        va_start(argList, format);

        retval = vprintf(format, argList);
        fflush(stdout);

        if ( logging ) {
            vsprintf(buffer, format, argList);
            fputs(buffer, logFile);
        }

        va_end(argList);    // clean up arg list
    }

    return(retval);
}
#endif

/**************************************************************************
* statsPrintf - print to the perl console and to stats file
*
* This routine is the equivalent of printf.  In addition it will write
* to the file in stats format (ie with , delimits).  If the pointer to 
* the file is null, just print to console
*
* RETURNS: same as printf.  Number of characters printed
*/
A_INT16 statsPrintf
    (
    FILE *pFile,
	const char * format,
    ...
    )
{
    va_list argList;
    int     retval = 0;
    char    buffer[256];

    /*if have logging turned on then can also write to a file if needed */

    /* get the arguement list */
    va_start(argList, format);

    /* using vprintf to perform the printing it is the same is printf, only
     * it takes a va_list or arguments
     */
    retval = vprintf(format, argList);
    fflush(stdout);

    if (logging) {
        vsprintf(buffer, format, argList);
        fputs(buffer, logFile);
    }

    if (pFile)
	{
        vsprintf(buffer, format, argList);
        fputs(buffer, pFile);	
		fputc(',', pFile);
	    fflush(pFile);
	}

	va_end(argList);    /* cleanup arg list */

    return(retval);
}


#ifndef __ATH_DJGPPDOS__
/**************************************************************************
* uilog - turn on logging
*
* A user interface command which turns on logging to a fill, all of the
* information printed on screen
*
* RETURNS: 1 if file opened, 0 if not
*/
A_UINT16 uilog
    (
    char *filename,        /* name of file to log to */
	A_BOOL append
    )
{
    /* open file for writing */
    if (append) {
		logFile = fopen(filename, "a+");
	}
	else {
		logFile = fopen(filename, "w");
	}
    if (logFile == NULL) {
        uiPrintf("Unable to open file %s\n", filename);
        return(0);
    }

    /* set flag to say logging enabled */
    logging = 1;

#ifndef MDK_AP
	//turn on logging in the library
	enableLogging(filename);
#endif
    return(1);
}
#endif
/**************************************************************************
* uiWriteToLog - write a string to the log file
*
* A user interface command which writes a string to the log file
*
* RETURNS: 1 if sucessful, 0 if not
*/
A_UINT16 uiWriteToLog
(
	char *string
)
{
	if(logFile == NULL) {
		uiPrintf("Error, logfile not valid, unable to write to file\n");
		return 0;
	}

	/* write string to file */
	fprintf(logFile, string);
	return 1;
}

void configPrint
(
     A_BOOL flag
)
{
    enablePrint = flag;
}


#ifndef __ATH_DJGPPDOS__
/**************************************************************************
* uilogClose - close the logging file
*
* A user interface command which closes an already open log file
*
* RETURNS: 1 if file opened, 0 if not
*/
void uilogClose(void)
{
    if ( logging ) {
        fclose(logFile);
        logging = 0;
    }

    return;
}

/**************************************************************************
* quiet - set quiet mode on or off
*
* A user interface command which turns quiet mode on or off
*
* RETURNS: N/A
*/
DLL_EXPORT void dk_quiet
    (
    A_UINT16 Mode        // 0 for off, 1 for on
    )
{
    quietMode = Mode;
    return;
}
#endif

/**************************************************************************
* uiOpenYieldLog - open the yield log file.
*
* A user interface command which turns on logging to the yield log file
*
* RETURNS: 1 if file opened, 0 if not
*/
A_UINT16 uiOpenYieldLog
(
    char *filename,        /* name of file to log to */
	A_BOOL append
)
{
    /* open file for writing */
    if (append) {
		yieldLogFile = fopen(filename, "a+");
	}
	else {
		yieldLogFile = fopen(filename, "w");
	}
    if (yieldLogFile == NULL) {
        uiPrintf("Unable to open yield log file %s\n", filename);
        return(0);
    } else {
		uiPrintf("Opened file %s for yieldLog\n", filename);
	}

    /* set flag to say yield logging enabled */
    yieldLogging = 1;

    return(1);
}

/**************************************************************************
* uiYieldLog - write a string to the yield log file
*
* A user interface command which writes a string to the log file
*
* RETURNS: 1 if sucessful, 0 if not
*/
A_UINT16 uiYieldLog
(
	char *string
)
{
	if (yieldLogging > 0) {
		if(yieldLogFile == NULL) {
			uiPrintf("Error, yield logfile not valid, unable to write to file\n");
			return 0;
		}

	  /* write string to file */
	  fprintf(yieldLogFile, string);

	  fflush(yieldLogFile);
	}
	return 1;
}

/**************************************************************************
* uiCloseYieldLog - close the yield logging file
*
* A user interface command which closes an already open log file
*
* RETURNS: void
*/
void uiCloseYieldLog(void)
{
    if ( yieldLogging) {
        if (yieldLogFile != NULL) 
			fclose(yieldLogFile);
        yieldLogging = 0;
    }

    return;
}


A_UINT16 hwGetBarSelect(A_UINT16 devIndex) {
    return (A_UINT16) globDrvInfo.pDevInfoArray[devIndex]->pdkInfo->bar_select;
}

A_UINT16 hwSetBarSelect(A_UINT16 devIndex, A_UINT16 bs) {
    if (driverVer.minorVersion >= 2) {
        if (bs < globDrvInfo.pDevInfoArray[devIndex]->pdkInfo->numBars) {
           globDrvInfo.pDevInfoArray[devIndex]->pdkInfo->bar_select = bs;
           return bs;
       }
    }
    globDrvInfo.pDevInfoArray[devIndex]->pdkInfo->bar_select = 0;
    return 0;
}


/**************************************************************************
* hwMemWriteBlock -  Write a block of memory within the simulation environment
*
* Write a block of memory within the simulation environment
*
*
* RETURNS: 0 on success, -1 on error
*/
A_INT16 hwMemWriteBlock
(
	A_UINT16 devIndex,
    A_UCHAR    *pBuffer,
    A_UINT32 length,
    A_UINT32 *pPhysAddr
)
{
    A_UCHAR *pMem;                /* virtual pointer to area to be written */
    A_UINT16 i;
    A_UINT32 startPhysAddr;        /* physical address of start of device memory block,
                                   for easier readability */
    MDK_WLAN_DEV_INFO *pdevInfo;

	pdevInfo = globDrvInfo.pDevInfoArray[devIndex];

    if(*pPhysAddr == 0)
    {
        return(-1);
    }

    /* first need to check that the phys address is within the allocated memory block.
       Need to make sure that the begin size and endsize match.  Will check all the 
       devices.  Only checking the memory block, will not allow registers to be accessed
       this way
     */
    
    /* want to scan all of the F2's to see if this is an allowable address for any of them
       if it is then allow the access.  This stays compatable with existing lab scripts */
    for (i = 0; i < WLAN_MAX_DEV; i++) {
        if(pdevInfo = globDrvInfo.pDevInfoArray[i])	{
			//check start and end addresswithin memory allocation
			startPhysAddr = pdevInfo->pdkInfo->memPhyAddr;
			if((*pPhysAddr >= startPhysAddr) &&
				(*pPhysAddr <= (startPhysAddr + pdevInfo->pdkInfo->memSize)) &&
				((*pPhysAddr + length) >= startPhysAddr) &&
				((*pPhysAddr + length) <= (startPhysAddr + pdevInfo->pdkInfo->memSize))
				) { 
				/* address is within range, so can do the write */
            
				/* get the virtual pointer to start and read */
				pMem = (A_UINT8 *) (pdevInfo->pdkInfo->memVirAddr + (*pPhysAddr - pdevInfo->pdkInfo->memPhyAddr));
                memcpy(pMem, pBuffer, length);
				return(0);
            }
			// check within the register regions
            startPhysAddr = pdevInfo->pdkInfo->aregPhyAddr[pdevInfo->pdkInfo->bar_select];
			if ((*pPhysAddr >= startPhysAddr) &&
                (*pPhysAddr < startPhysAddr + pdevInfo->pdkInfo->aregRange[pdevInfo->pdkInfo->bar_select]) &&
                ((*pPhysAddr + length) >= startPhysAddr) &&
                ((*pPhysAddr + length) <= (startPhysAddr + pdevInfo->pdkInfo->aregRange[pdevInfo->pdkInfo->bar_select]))) {
				pMem = (A_UINT8 *) (pdevInfo->pdkInfo->aregVirAddr[pdevInfo->pdkInfo->bar_select] + (*pPhysAddr - pdevInfo->pdkInfo->aregPhyAddr[pdevInfo->pdkInfo->bar_select]));
                memcpy(pMem, pBuffer, length);
                return(0);
			}
        }
	}
    /* if got to here, then address is bad */
    uiPrintf("Warning: Address is not within legal memory range, nothing written\n");
    return(-1);
    }

/**************************************************************************
* hwMemReadBlock - Read a block of memory within the simulation environment
*
* Read a block of memory within the simulation environment
*
*
* RETURNS: 0 on success, -1 on error
*/
A_INT16 hwMemReadBlock
(
 	A_UINT16 devIndex,
    A_UCHAR    *pBuffer,
    A_UINT32 physAddr,
    A_UINT32 length
)
{
    A_UCHAR *pMem;                /* virtual pointer to area to be written */
    A_UINT16 i;
    A_UINT32 startPhysAddr;        /* physical address of start of device memory block,
                                   for easier readability */
    MDK_WLAN_DEV_INFO *pdevInfo;

	pdevInfo = globDrvInfo.pDevInfoArray[devIndex];



    /* first need to check that the phys address is within the allocated memory block.
       Need to make sure that the begin size and endsize match.  Will check all the 
       devices.  Only checking the memory block, will not allow registers to be accessed
       this way
     */
    for (i = 0; i < WLAN_MAX_DEV; i++) {
        if(pdevInfo = globDrvInfo.pDevInfoArray[i]) {
			//check start and end addresswithin memory allocation
			startPhysAddr = pdevInfo->pdkInfo->memPhyAddr;
			if((physAddr >= startPhysAddr) &&
				(physAddr <= (startPhysAddr + pdevInfo->pdkInfo->memSize)) &&
				((physAddr + length) >= startPhysAddr) &&
				((physAddr + length) <= (startPhysAddr + pdevInfo->pdkInfo->memSize))
				) { 
				/* address is within range, so can do the read */
				/* get the virtual pointer to start and read */
				pMem = (A_UINT8 *) (pdevInfo->pdkInfo->memVirAddr + (physAddr - pdevInfo->pdkInfo->memPhyAddr));
				memcpy(pBuffer, pMem, length);
				return(0);
			}
			startPhysAddr = pdevInfo->pdkInfo->aregPhyAddr[pdevInfo->pdkInfo->bar_select];
			if ((physAddr >= startPhysAddr) &&
                (physAddr < startPhysAddr + pdevInfo->pdkInfo->aregRange[pdevInfo->pdkInfo->bar_select]) &&
                ((physAddr + length) >= startPhysAddr) &&
                ((physAddr + length) <= (startPhysAddr + pdevInfo->pdkInfo->aregRange[pdevInfo->pdkInfo->bar_select]))) {
				pMem = (A_UINT8 *) (pdevInfo->pdkInfo->aregVirAddr[pdevInfo->pdkInfo->bar_select] + (physAddr - pdevInfo->pdkInfo->aregPhyAddr[pdevInfo->pdkInfo->bar_select]));
     		// check within the register regions
                memcpy(pBuffer, pMem, length);
                return(0);
            }
        }
    }
    /* if got to here, then address is bad */
    uiPrintf("Warning: Address (%x) is not within legal memory range, nothing read\n", physAddr);
    return(-1);
}

//#ifdef ART_BUILD
#if 1
//#else

/**************************************************************************
* hwGetPhysMem - get a block of physically contiguous memory
*
* This routine gets physically contiguous driver-level memory
*
* RETURNS: The physical address of the memory allocated
*/
/* #####Note may replace this with the function later that returns */
/* both physical and virtual memory of buffer. */
void *hwGetPhysMem
(
	A_UINT16 devIndex,
	A_UINT32 memSize,		             /* number of bytes to allocate */
	A_UINT32 *physAddress
)
{
    A_UCHAR *virtAddress;
    A_UINT32 offset;        /* offset from start of where descriptor is */
    A_UINT16 numBlocks;     /* num blocks need to allocate */
    A_UINT16 index;         /* index of first block to allocate */
    MDK_WLAN_DEV_INFO *pdevInfo;

	pdevInfo = globDrvInfo.pDevInfoArray[devIndex];

	if((memSize/BUFF_BLOCK_SIZE + 1) >= (1 << 8 * sizeof(A_UINT16))) {
 		uiPrintf("WARNING: Physical memory of size %ld out of range - returning NULL address\n", memSize); 
        return(NULL);
	}

    /* calculate how many blocks of memory are needed and round up result */
    numBlocks = (A_UINT16) (memSize/BUFF_BLOCK_SIZE + ((memSize % BUFF_BLOCK_SIZE) || 0));
    
    if(memGetIndexForBlock2(pdevInfo, pdevInfo->pbuffMapBytes, numBlocks, &index) != A_OK) {
		uiPrintf("WARNING: Failed to allocate physical memory of size %ld - returning NULL address\n", memSize); 
        return(NULL);
	}

    /* got an index, now calculate addresses */
    offset = index * BUFF_BLOCK_SIZE;
    virtAddress = (A_UCHAR *)(pdevInfo->pdkInfo->memVirAddr + offset);
    *physAddress = (A_UINT32)(pdevInfo->pdkInfo->memPhyAddr + offset);

//  /* zero out memory */ -- Need to handle for predator also, so comment for now
//	memset(virtAddress, 0, memSize);
    return(virtAddress);    
}


/**************************************************************************
* hwFreeAll - Environment specific code for Command to free all the 
*             currently allocated memory
*
* This routine calls to the hardware abstraction layer, to free all of the
* currently allocated memory.  This will include all descriptors and packet
* data as well as any memory allocated with the alloc command.
*
*
* RETURNS: N/A
*/
void hwFreeAll
(
	A_UINT16 devIndex
)
{
	A_UINT32        NumBuffBlocks;
	A_UINT32        NumBuffMapBytes;
    MDK_WLAN_DEV_INFO *pdevInfo;

	pdevInfo = globDrvInfo.pDevInfoArray[devIndex];

    if(pdevInfo) {
		NumBuffBlocks	= pdevInfo->pdkInfo->memSize / BUFF_BLOCK_SIZE;
		NumBuffMapBytes = NumBuffBlocks / 8;

		/* clear the memory allocated by clearing all the map bytes */
		memset( pdevInfo->pbuffMapBytes, 0, NumBuffMapBytes * sizeof(A_UCHAR) );
		memset( pdevInfo->pnumBuffs, 0, NumBuffBlocks * sizeof(A_UINT16) );
	}

    return;
}


/**************************************************************************
* hwEnableFeature - Handle feature enable within windows environment
*
* Enable ISR features within windows environment
*
*
* RETURNS: 0 on success, -1 on error
*/

A_INT16 hwEnableFeature
(
 	A_UINT16 devIndex,
	PIPE_CMD *pCmd
)
{
	printf("hwEnableFeature not implemented\n");
	return(0);
}

/**************************************************************************
* hwDisableFeature - Handle feature disable within windows environment
*
* Disble ISR features within windows environment
*
*
* RETURNS: 0 on success, -1 on error
*/

A_INT16 hwDisableFeature
(
	A_UINT16 devIndex,
	PIPE_CMD *pCmd
)
{
	printf("hwDiableFeature not implemented\n");
	return(0);
}

/**************************************************************************
* hwGetStats - Get stats
*
* call into kernel plugin to get the stats copied into user supplied 
* buffer
*
*
* RETURNS: 0 on success, -1 on error
*/

A_INT16 hwGetStats
(
	A_UINT16 devIndex,
	A_UINT32 clearOnRead,
	A_UCHAR  *pBuffer,
	A_BOOL	 rxStats
)
{
	printf("hwGetStats not implemented\n");
	return(0);
}

/**************************************************************************
* hwGetSingleStat - Get single stat
*
* call into kernel plugin to get the stats copied into user supplied 
* buffer
*
*
* RETURNS: 0 on success, -1 on error
*/

A_INT16 hwGetSingleStat
(
	A_UINT16 devIndex,
	A_UINT32 statID,
	A_UINT32 clearOnRead,
	A_UCHAR  *pBuffer,
	A_BOOL	 rxStats
)
{
	printf("hwGetSingleStat not implemented\n");
	return(0);
}

/**************************************************************************
* hwRemapHardware - Remap the hardware to a new address
*
* Remap the hardware to a new address
*
*
* RETURNS: 0 on success, -1 on error
*/
A_INT16 hwRemapHardware
(
 	A_UINT16 devIndex,
    A_UINT32 mapAddress
)
{
	printf("hwReMapHardware not implemented\n");
    return(0);
}


/**************************************************************************
* hwTramWriteBlock -  Write trace ram
*
* Write a block of trace ram
*
*
* RETURNS: 0 on success, -1 on error
*/
A_INT16 hwTramWriteBlock
(
	A_UINT16 devIndex,
	A_UCHAR    *pBuffer,
	A_UINT32 length,
	A_UINT32 physAddr
)
{
	A_UINT32 numDWords = length / 4;
	A_UINT32 *pData = (A_UINT32 *)pBuffer;
	A_UINT32 i;
    MDK_WLAN_DEV_INFO *pdevInfo;

	pdevInfo = globDrvInfo.pDevInfoArray[devIndex];

	
	for(i = 0; i < numDWords; i++) {
		//write the address
		hwMemWrite32(devIndex,pdevInfo->pdkInfo->aregPhyAddr[0] + 0x18, physAddr);

		//write the value
		hwMemWrite32(devIndex,pdevInfo->pdkInfo->aregPhyAddr[0] + 0x1c, *pData);

		pData++;
		physAddr++;
	}
	return 0;
}

/**************************************************************************
* hwTramReadBlock - Read a block of trace ram
*
* Read a block of traceram
*
*
* RETURNS: 0 on success, -1 on error
*/
A_INT16 hwTramReadBlock
(
	A_UINT16 devIndex,
	A_UCHAR    *pBuffer,
	A_UINT32 physAddr,
	A_UINT32 length
)
{
	A_UINT32 numDWords = length / 4;
	A_UINT32 *pData = (A_UINT32 *)pBuffer;
	A_UINT32 i;
    MDK_WLAN_DEV_INFO *pdevInfo;

	pdevInfo = globDrvInfo.pDevInfoArray[devIndex];
	
	for(i = 0; i < numDWords; i++) {
		//write the address
		hwMemWrite32(devIndex, pdevInfo->pdkInfo->aregPhyAddr[0] + 0x18, physAddr);

		//read the value
		*pData = hwMemRead32(devIndex, pdevInfo->pdkInfo->aregPhyAddr[0] + 0x1c);

		pData++;
		physAddr++;
	}
	return 0;

}

#endif  // ART_BUILD


/**************************************************************************
* freeDevInfo - frees memory held by devInfo
*
* RETURNS: N/A
*/
void freeDevInfo
(
	MDK_WLAN_DEV_INFO *pdevInfo
)
{
    A_FREE(pdevInfo->pdkInfo);
    A_FREE(pdevInfo);
    
    return;
}



