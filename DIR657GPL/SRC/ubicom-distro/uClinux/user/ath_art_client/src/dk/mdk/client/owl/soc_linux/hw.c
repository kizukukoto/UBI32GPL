/* hw.c - contain the access hrdware routines for SPIRIT  */

/* Copyright (c) 2001 Atheros Communications, Inc., All Rights Reserved */
#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>

#include "dk_mem.h"
#include "dk_client.h"
#include "dk_common.h"
#include "dk_ioctl.h"

#include "ar5211reg.h"
#include "common_hw.h"
#include "ar5416reg.h"
#include "mConfig.h"

//flash read varibles..

//flash block size is defined as 64 k
#define blk_size (64 << 10)

//// FORWARD DECLARATIONS
void hwInit(MDK_WLAN_DEV_INFO *pdevInfo, A_UINT32 resetMask);

//void deviceCleanup(A_UINT16 devIndex);

#ifndef __UBICOM32__
// global variables
MDK_WLAN_DRV_INFO	globDrvInfo;				
#endif
//#define  HWMEMRDWR

void hwInit
(
	MDK_WLAN_DEV_INFO *pdevInfo, A_UINT32 resetMask
)
{
	A_UINT32  reg, reg_pci, l_resetMask=0, pci_resetMask=0;
	 A_UINT16 devIndex = (A_UINT16)pdevInfo->pdkInfo->devIndex;
#if 0
        if (resetMask == 0) {
         l_resetMask = RESET_WARM_WLAN0_MAC | RESET_WARM_WLAN0_BB;
          pci_resetMask = F2_RC_BB | F2_RC_MAC;
				          }
        if (resetMask & MAC_RESET) {
          l_resetMask = RESET_WARM_WLAN0_MAC ;
          pci_resetMask = F2_RC_MAC ;
          }
        if (resetMask & BB_RESET) {
         l_resetMask |= RESET_WARM_WLAN0_BB;
         pci_resetMask |= F2_RC_BB ;
      }
     pci_resetMask |= F2_RC_PCI ;
     reg = apRegRead32(devIndex,AR531XPLUS_RESET);
    apRegWrite32(devIndex,AR531XPLUS_RESET, reg | l_resetMask);
     sysUDelay(10);
    reg = apRegRead32(devIndex,AR531XPLUS_RESET);
       apRegWrite32(devIndex,AR531XPLUS_RESET, reg & ~l_resetMask);
    sysUDelay(10);

    reg_pci = apRegRead32(devIndex, AR531XPLUS_PCI + F2_RC );
    reg_pci |= pci_resetMask;
    apRegWrite32(devIndex, AR531XPLUS_PCI + F2_RC, reg_pci);
    sysUDelay(10);
    reg_pci = apRegRead32(devIndex, AR531XPLUS_PCI + F2_RC );
    reg_pci &= ~pci_resetMask;
    apRegWrite32( devIndex,AR531XPLUS_PCI + F2_RC, reg_pci);
    sysUDelay(10);
   apRegWrite32(devIndex,AR531XPLUS_PCI+F2_SCR, F2_SCR_SLE_FWAKE );

    apRegWrite32(devIndex,AR531XPLUS_PCI + F2_SFR, F2_SFR_WAKE);

    while(apRegRead32(devIndex,AR531XPLUS_PCI + F2_PCICFG) \
		    & F2_PCICFG_SPWR_DN) {
    }
    reg = apRegRead32(devIndex,AR531XPLUS_AHB_ARB_CTL);

    reg |= ARB_WLAN;

    apRegWrite32(devIndex,AR531XPLUS_AHB_ARB_CTL, reg);

#endif

}
	

//#define AR5313_MAJOR_REV0	5
void hwClose
(
	MDK_WLAN_DEV_INFO *pdevInfo
)
{
	A_UINT32      reg, l_resetMask=0;
	A_UINT32 majorVer;
	 A_UINT16 devIndex = (A_UINT16)pdevInfo->pdkInfo->devIndex;

#if 0	 
         apRegWrite32(devIndex,(AR531XPLUS_WLAN0 + F2_IER), F2_IER_DISABLE);
         l_resetMask = RESET_WARM_WLAN0_MAC | RESET_WARM_WLAN0_BB;
         reg = apRegRead32(devIndex,AR531XPLUS_RESET);
         apRegWrite32(devIndex,AR531XPLUS_RESET, reg|l_resetMask);
#endif
}

A_UINT32 validDevAddress(A_UINT32 address) 
{
#if 0
        address = address & 0xbfff0000;
       if ((address == AR531XPLUS_ENET0) ||
           (address == AR531XPLUS_WLAN0) ||
           (address ==  AR531XPLUS_DSLBASE) ||
           (address ==  AR531XPLUS_PCI) ||
           (address ==  AR531XPLUS_SPI)) {
             return 1;
        }
        return 0;
#endif
    return 1;
	
}

/**************************************************************************
* apRegRead32 - read an 32 bit value.
*
* This routine will read the register pointed by the address 
* and return the value. This function is used only in AP, to read non-wmac
* registers. The address should be a 32-bit absolute address
*
* RETURNS: the value read
*/

A_UINT32 apRegRead32
(
	A_UINT16 devIndex,
    	A_UINT32 address                  
)
{
    	A_UINT32 value=0;
	 struct cfg_op co;
	  MDK_WLAN_DEV_INFO    *pdevInfo;
	  pdevInfo = globDrvInfo.pDevInfoArray[devIndex];
	  address = address & 0xFFFFFFFC;
	  if (validDevAddress(address)) {
	  // make an ioctl  call
	   co.offset=address;
	    co.size = 4;
            co.value = 0;
              if (ioctl(pdevInfo->hDevice,DK_IOCTL_SYS_REG_READ_32,&co) < 0) {
                   uiPrintf("Error: Sys reg Read to %x failed \n", address);
                   value = 0xdeadbeef;
             } else {
			 value = co.value;
             }
#ifdef DEBUG
           q_uiPrintf("Register at address %08lx: %08lx\n", address, value);
#endif
        } else {
                uiPrintf("ERROR: apRegRead32 could not access hardware address: 0x%08lx\n", address);
        }

        return value;
					      
}

/**************************************************************************
* apRegWrite32 - write a 32 bit value
*
* This routine will write the value into the register pointed 
* by the address/ This function is used only in AP, to write non-wmac
* registers. The address should be a 32-bit absolute address
*
*
* RETURNS: N/A
*/
void apRegWrite32
(
		A_UINT16 devIndex,
    	A_UINT32 address,         
    	A_UINT32  value           
)
{
       MDK_WLAN_DEV_INFO    *pdevInfo;
       struct cfg_op co;

        pdevInfo = globDrvInfo.pDevInfoArray[devIndex];

	address = address & 0xFFFFFFFC;

	if (validDevAddress(address)) {
         // make an ioctl  call
	co.offset=address;
	co.size = 4;
	co.value = value;
	if (ioctl(pdevInfo->hDevice,DK_IOCTL_SYS_REG_WRITE_32,&co) < 0) {
		uiPrintf("Error: Sys reg write to %x failed \n", address);
	}

#ifdef DEBUG
	q_uiPrintf("Register write @ address %08lx: %08lx\n", address, value);
#endif

	}  else {
		uiPrintf("ERROR: apRegWrite32 could not access hardware address: 0x%08lx\n", address);
	}
	       
	return;
			
}

A_UINT8 sysFlashConfigRead(A_UINT32 devNum, int fcl , int offset) {
    	A_UINT8  value=0;
	 struct  flash_op fop;
	  MDK_WLAN_DEV_INFO    *pdevInfo;
	   // pdevInfo = globDrvInfo.pDevInfoArray[devIndex];
	    pdevInfo = globDrvInfo.pDevInfoArray[0];
	  fop.fcl=fcl;
	  fop.offset = offset;
	  fop.len= 1;
	  fop.value=0;

#ifdef OWL_PB42
       if(isHowlAP(0)){

        if(fcl==2){
           offset=offset+0x1000;
         }

        int fd, i, j, count;
        unsigned char *fn,buf[10];
        unsigned int  val;
                fn = "/dev/mtdblock4";

        if ((fd = open(fn, O_RDWR)) < 0) {
                perror("Could not open flash");
                exit(1);
        }

        lseek(fd, -blk_size+offset, SEEK_END);
        if(read(fd, &value,1)<0){
	        perror("read");
        }

        close(fd);

       } else if(isFlashCalData()) {

         LIB_DEV_INFO  *pLibDev = gLibInfo.pLibDevArray[devNum];
	     offset += pLibDev->devMap.devIndex*16*1024; // Base of caldata (16K bytes each block)
         if(fcl==2){
           offset=offset+0x1000;
         }

		 int fd;
         unsigned char *fn;
         fn = "/dev/caldata";
         if ((fd = open(fn, O_RDWR)) < 0) {
            perror("Could not open flash");
            exit(1);
         }
         lseek(fd, -blk_size+offset, SEEK_END);
         if(read(fd, &value,1)<0){
	        perror("read");
         }
         close(fd);

       } else {
              // This block supports accessing falsh through art kernel module
              if (ioctl(pdevInfo->hDevice,DK_IOCTL_FLASH_READ,&fop) < 0) {
                   uiPrintf("Error: sysFlashConfigRead to  failed \n");
                   value = 0xda;
             } else {
			   value = fop.value;
//			   *retlen=fop.retlen;
	         }
       }

#endif

	  return value;
         
}
A_UINT8  flash_read(A_UINT32 devNum, A_UINT32 fcl ,A_UINT8 *buf,A_UINT32 len, A_UINT32 offset) {
        A_UINT8  value=0;
         struct  flash_op fop;
          MDK_WLAN_DEV_INFO    *pdevInfo;
           // pdevInfo = globDrvInfo.pDevInfoArray[devIndex];
            pdevInfo = globDrvInfo.pDevInfoArray[0];
          fop.fcl=fcl;
          fop.offset = offset;
          fop.len= 1;
          fop.value=0;

       if(isHowlAP(0)){
         if(fcl==2){
            offset=offset+0x1000;
         }

        int fd, i, j, count;
        unsigned char *fn;
        unsigned int  val;
                fn = "/dev/mtdblock4";

        if ((fd = open(fn, O_RDWR)) < 0) {
                perror("Could not open flash");
                exit(1);
        }

        lseek(fd, -blk_size+offset, SEEK_END);
        if(read(fd, buf,len*2)<0){
        	perror("read");
	    }
        close(fd);
      } else if (isFlashCalData()){
        LIB_DEV_INFO  *pLibDev = gLibInfo.pLibDevArray[devNum];
	    offset += pLibDev->devMap.devIndex*16*1024; // Base of caldata (16K bytes each block)
	
        if(fcl==2){
            offset=offset+0x1000;
        }

        int fd;
        unsigned char *fn;
        fn = "/dev/caldata";

        if ((fd = open(fn, O_RDWR)) < 0) {
                perror("Could not open flash");
                exit(1);
        }

        lseek(fd, -blk_size+offset, SEEK_END);
        if(read(fd, buf,len*2)<0){
        	perror("read");
	    }
        close(fd);

      } else {
			printf ("Error: flash access flash_read()\n");
	  }


      return 0;
}

void sysFlashConfigWrite(A_UINT32 devNum, int fcl, int offset, A_UINT8 *data, int len) {
    	A_UINT8 value=0;
	 struct  flash_op_wr fop;
	  MDK_WLAN_DEV_INFO    *pdevInfo;
	   // pdevInfo = globDrvInfo.pDevInfoArray[devIndex];
	  pdevInfo = globDrvInfo.pDevInfoArray[0];
	  fop.fcl=fcl;
	  fop.offset = offset;
	  fop.len=len;
	  fop.pvalue=data;

#ifdef OWL_PB42
       if(isHowlAP(0)){
         if(fcl==2){
               offset=offset+0x1000;
         }

        int fd, i, j, count;
        unsigned char *fn;
        fn = "/dev/mtdblock4";

        if ((fd = open(fn, O_RDWR)) < 0) {
                perror("Could not open flash");
                exit(1);
        }

        lseek(fd, -blk_size+offset, SEEK_END);
        if (write(fd, data, len) != len) {
        	perror("\nwrite");
	    }

        close(fd);

      } else if(isFlashCalData()){
         LIB_DEV_INFO  *pLibDev = gLibInfo.pLibDevArray[devNum];
	     offset += pLibDev->devMap.devIndex*16*1024; // Base of caldata (16K bytes each block)
         if(fcl==2){
               offset=offset+0x1000;
         }

        int fd;
        unsigned char *fn;
        fn = "/dev/caldata";

        if ((fd = open(fn, O_RDWR)) < 0) {
                perror("Could not open flash");
                exit(1);
        }

        lseek(fd, -blk_size+offset, SEEK_END);
        if (write(fd, data, len) != len) {
        	perror("\nwrite");
	    }

        close(fd);

     } else{
          if (ioctl(pdevInfo->hDevice,DK_IOCTL_FLASH_WRITE,&fop) < 0) {
             printf("Error: sysFlashConfigWrite to  failed \n");
//                   value = 0xdeadbeef;
//		   data=0xdeadbeef;                   
             } else {
			data  = fop.pvalue;
//			* retlen=fop.retlen;
		  }
     }
#endif
	  
}

/************************************************************************/
// Function Name: writeProdDataWlanMac()
// Input:   Device Number
//			WLAN MAC address of size 6 char
// Output:  None
// Description: This fucntion writes WLAN mac address for PB42.
/************************************************************************/
void writeProdDataMacAddr(A_UINT32 devNum, A_UCHAR *mac0Addr, A_UCHAR *mac1Addr)
{
	int i;
	struct flash_mac_addr flashMac;
	MDK_WLAN_DEV_INFO    *pdevInfo;
	pdevInfo = globDrvInfo.pDevInfoArray[0];
    flashMac.len = 12;
    flashMac.pAddr0 = mac0Addr;
    flashMac.pAddr1 = mac1Addr;

    if (ioctl(pdevInfo->hDevice,DK_IOCTL_MAC_WRITE,&flashMac) < 0) {
		printf("ERROR: writeProdDataWlanMac failed\n");
    }

}


void taskLock() {

}

void taskUnlock() {

}





