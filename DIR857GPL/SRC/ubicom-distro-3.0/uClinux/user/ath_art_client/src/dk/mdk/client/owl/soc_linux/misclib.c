#include "wlantype.h"
#include "stdio.h"
#include "common_hw.h"
#include "dk_common.h"
#include "misclib.h"
#include "endian_func.h"

#if defined(THIN_CLIENT_BUILD)
#else // THIN_CLIENT_BUILD
#include "athreg.h"
#include "manlib.h"
#endif // THIN_CLIENT_BUILD

#define CODE_START_ADDR	0xa0300000
#define CODE_END_ADDR	0xa0400000
#define LOAD_BLOCK_SIZE	256
#define RETVAL_ADDR	0xa03ffffc

#ifdef INCLUDE_SCREENING_TESTS
#include "intLib.h"
#include "ar531xreg.h"
#include "ae531xreg.h"
#include "ar5211reg.h"
#include "cacheLib.h" 
#if AR5311
#include "rtPhy.h"
#endif
#if AR5312
#include "mvPhy.h"
#endif


// MACROS for screening Test
#define SDRAM_TEST 1
#define FLASH_TEST 2
#define PROC_SPEED_TEST 3
#define REG_TEST 4
#define ETHERNET_TEST 5

#define ETH1_MAC_BASE_ADDR (AR531X_ENET1+AE_MAC_OFFSET)
#define ETH1_DMA_BASE_ADDR (AR531X_ENET1+AE_DMA_OFFSET)
#define ETH1_PHY_ADDR 2
#define TX_FRAME_SIZE 0x100
#define RX_BUF_SIZE 0x400
#define NO_MAC_BB_REG_TEST 3
#define NO_ETH1_MAC_REG_TEST 1
#define NO_ETH1_DMA_REG_TEST 2
 
A_UINT32 dataPattern[12] = {	0x00000000,0x55555555,0x33333333,0x0F0F0F0F,
			    	0x00FF00FF,0x0000FFFF,0xFFFF0000,0xFF00FF00,
			   	0xF0F0F0F0,0xCCCCCCCC,0xAAAAAAAA,0xFFFFFFFF};

struct regStruct {
	A_UINT32 regOffset;	
	A_UINT32 regMask;
};

extern A_UINT32 reverseBits
(
 	A_UINT32 val, 
 	int bit_count
);

#endif // INCLUDE_SCREENING_TESTS


/*extern STATUS sysNvRamSet
(
	char *str, 
	int len, 
	int off
);
*/

#ifdef DEBUG
extern int bdShow
(
	struct ar531x_boarddata *bd
);
#endif


/**************************************************************************
* loadAndRunCode - load the code into memory if the flag load is 0
*		 - run the code and wait for the code to return if the load flag is 1
*
* The Address should be between CODE_START_ADDR to CODE_END_ADDR
*
* RETURNS: 
	for load - 0 on success, -1 on error
	for run  - the C code return value on success, -1 on error
*/

A_INT32 loadAndRunCode
(
	A_UINT32 loadFlag,
	A_UINT32 pPhyAddr,
	A_UINT32 length,
 	A_UCHAR  *pBuffer
)
{
	void (*funcptr)();
	A_INT32 *retval_ptr;

	if (loadFlag == 1) 
	{
		// only 256 bytes per load 
		// sanity check
		if (length > LOAD_BLOCK_SIZE) 
		{
			uiPrintf("Error in loadandruncode :: Length %x : Should be less than %x \n",length,LOAD_BLOCK_SIZE);
			return -1;
		}
		if ((pPhyAddr < CODE_START_ADDR) || (pPhyAddr >= CODE_END_ADDR)) 
		{
			uiPrintf("Error in loadandruncode :: Invalid address %x : Should be between %x and %x \n",pPhyAddr,CODE_START_ADDR,CODE_END_ADDR);
			return -1;
		}
		q_uiPrintf("Loading code @%x \n",pPhyAddr);
		memcpy((void *)pPhyAddr,(void *)pBuffer, length);
		return 0;
	}

	if (loadFlag == 0)
	{
		if ((pPhyAddr < CODE_START_ADDR) || (pPhyAddr >= CODE_END_ADDR)) 
		{
			uiPrintf("Error in loadandruncode :: Invalid address %x : Should be between %x and %x \n",pPhyAddr,CODE_START_ADDR,CODE_END_ADDR);
			return -1;
		}
		// sanity check
		if (length != 4) 
		{
			uiPrintf("Error in loadandruncode :: Length %x : Should be 4 \n",length);
			return -1;
		}
		q_uiPrintf("Running code @%x \n",pPhyAddr);
		funcptr = (void (*)())pPhyAddr;
		(*funcptr)();

		retval_ptr = (A_INT32 *)RETVAL_ADDR;
		uiPrintf("Return Value from C code = %d \n",*retval_ptr);
		return *retval_ptr;
	}
	

    	return(-1);
}

void writeNewProdData
(
	A_UINT32 devNum,
	A_UINT32 argList[6],
	A_UINT32 numArgs
)
{
        
//	char *bootLine = DEFAULT_BOOT_LINE " s=factory";
	struct ar531x_boarddata sysBoardData;
	A_UINT32 i, j ;
	A_UCHAR *tmpBdData;

	if (numArgs < 2) {
		uiPrintf("ERROR: writeNewProdData : at least 2 arguments expected : radio0_mask and radio1_mask\n");
		return;
	}

	tmpBdData = (A_UCHAR *)A_MALLOC(sizeof(sysBoardData));
	if (tmpBdData == NULL) {
		uiPrintf("ERROR: writeNewProdData : could not allocate memory for tmpBoardData\n");
		return;
	}

	for (j=0; j<sizeof(sysBoardData); j++) {
		tmpBdData[j] = sysFlashConfigRead(devNum, FLC_BOARDDATA, j);		 
	}

	memcpy((void *)&(sysBoardData), (void *)tmpBdData, sizeof(sysBoardData));	

	if(sysBoardData.rev >= 4) {
		if ((argList[0] & 0x1) > 0) {
			sysBoardData.config |= BD_WLAN0_2G_EN;
		}

		if ((argList[0] & 0x2) > 0) {
			sysBoardData.config |= BD_WLAN0_5G_EN;
		}

		if ((argList[1] & 0x1) > 0) {
			sysBoardData.config |= BD_WLAN1_2G_EN;
		}

		if ((argList[1] & 0x2) > 0) {
			sysBoardData.config |= BD_WLAN1_5G_EN;
		}
	}
	
	sysBoardData.cksum = sysBoardDataChecksum(&sysBoardData);
	sysFlashConfigWrite(devNum, FLC_BOARDDATA, 0, (char *)&sysBoardData, sizeof(sysBoardData));
#ifdef CHK_BRD_DATA	
/***************************************************************************************************************************/
	int x;
	A_UCHAR tmp;
	
	for (x=0; x<sizeof(sysBoardData); x++) {
		tmp = sysFlashConfigRead(devNum, FLC_BOARDDATA, x);
		printf("Board Data %x \n", tmp);		
	}
/***************************************************************************************************************************/
#endif
	
#ifdef DEBUG
	bdShow(&sysBoardData);
#endif

	A_FREE(tmpBdData);
    
}
/******************************************************************************/
/* Added  From vxworks/target/config/ar531x/sysLib.c*/
/******************************************************************************/
UINT16
sysBoardDataChecksum(struct ar531x_boarddata *bd)
{
    UINT16 cksum = 0;
    UINT16 save;
    int len, i;
    A_UINT8 *p;

    switch (bd->rev) {
    case 1:
        return 0;               /* old ROMs have checksum of 0 */
    case 2:
        len = SIZEOF_BD_REV2;
        break;
    case 3:
        len = sizeof(struct ar531x_boarddata);
        break;
    case 4:
        len = sizeof(struct ar531x_boarddata);
        break;
    default:
        printf("unknown boarddata rev!\n");
        return 0;
    }

    save      = bd->cksum;
    bd->cksum = 0;
    p         = (A_UINT8 *)bd;
    for (i = 0; i < len; i++) {
        cksum += p[i];
    }
    bd->cksum = save;

    return cksum;
}
/******************************************************************************/
// ff:ff:ff:ff:ff:ff as enet1Mac address indicates it is not used
void writeProdData
(
	A_UINT32 devNum,
	A_UCHAR wlan0Mac[6],
	A_UCHAR wlan1Mac[6],
	A_UCHAR enet0Mac[6],
	A_UCHAR enet1Mac[6]
)
{
        
	char *bootLine = DEFAULT_BOOT_LINE " s=factory";
	struct ar531x_boarddata sysBoardData;
	A_UINT32 i;
	A_UINT32 enet0enable;	
	A_UINT32 enet1enable;	
	A_UINT32 wlan0enable = 0;
	A_UINT32 wlan1enable = 0;
	A_UCHAR  *validWlanMac;

	// Write the factory default bootline
//	sysNvRamSet(bootLine, strlen(bootLine)+1, 0);

	// Write the Board Configuration Data
	sysBoardData.magic = BD_MAGIC;
    	sysBoardData.rev   = BD_REV;
    	strcpy(sysBoardData.boardName, "Atheros AR5001AP default");
    	sysBoardData.major = 1;
    	sysBoardData.minor = 0;
    	sysBoardData.config =	BD_UART0 |        			
        						BD_SYSLED |
								BD_RSTFACTORY;

	sysBoardData.resetConfigGpio = 6;
    	sysBoardData.sysLedGpio = 7;

	enet1enable = 0;
	enet0enable = 0;
	for (i=0;i<6;i++) {
 		sysBoardData.wlan0Mac[i] = wlan0Mac[i];
 		sysBoardData.wlan1Mac[i] = wlan1Mac[i];
		sysBoardData.enet0Mac[i] = enet0Mac[i];
   		sysBoardData.enet1Mac[i] = enet1Mac[i];
		if (enet0Mac[i] != 0xff) enet0enable = 1;
		if (enet1Mac[i] != 0xff) enet1enable = 1;
		if (wlan0Mac[i] != 0xff) wlan0enable = 1;
		if (wlan1Mac[i] != 0xff) wlan1enable = 1;
	}

	if (enet0enable) sysBoardData.config |= BD_ENET0;
	if (enet1enable) sysBoardData.config |= BD_ENET1;
	if (wlan0enable) sysBoardData.config |= BD_WLAN0;
	if (wlan1enable) sysBoardData.config |= BD_WLAN1;

    	sysBoardData.pciId = 0x013;

	sysBoardData.cksum = sysBoardDataChecksum(&sysBoardData);

	sysFlashConfigWrite(devNum, FLC_BOARDDATA, 0, (char *)&sysBoardData, sizeof(sysBoardData));
#ifdef CHK_BRD_DATA	
/***************************************************************************************************************************/
printf("Board Data mag %x rev  %x name %s Size of board data %x \n ",sysBoardData.magic,sysBoardData.rev,sysBoardData.boardName,sizeof(sysBoardData));
	int x;
	A_UCHAR tmp;
	
	for (x=0; x<sizeof(sysBoardData); x++) {
		tmp = sysFlashConfigRead(devNum, FLC_BOARDDATA, x);
		printf("Board Data %x \n", tmp);		
	}
/***************************************************************************************************************************/
#endif	
	// write to the eeprom flash area
	if (wlan0enable) {
		validWlanMac = wlan0Mac;
	} else {
		validWlanMac = wlan1Mac;
	}
#if defined(THIN_CLIENT_BUILD)
#else // THIN_CLIENT_BUILD
    eepromWrite(devNum, 0x1d, (A_UINT16)((validWlanMac[4] << 8) | validWlanMac[5]));
    eepromWrite(devNum, 0x1e, (A_UINT16)((validWlanMac[2] << 8) | validWlanMac[3]));
    eepromWrite(devNum, 0x1f, (A_UINT16)((validWlanMac[0] << 8) | validWlanMac[1]));

	// cardbus tuples
    eepromWrite(devNum, 0xa5, swap_s((A_UINT16)((validWlanMac[4] << 8) | validWlanMac[5])));
    eepromWrite(devNum, 0xa6, swap_s((A_UINT16)((validWlanMac[2] << 8) | validWlanMac[3])));
    eepromWrite(devNum, 0xa7, swap_s((A_UINT16)((validWlanMac[0] << 8) | validWlanMac[1])));
#endif

#ifdef DEBUG
	bdShow(&sysBoardData);
#endif

}

A_INT32 freedomGetMacAddr
(
	A_UINT32 devNum,
	A_UINT16 wmac,
	A_UINT16 instNo,
	A_UINT8 *macAddr
)
{
         
	char *p;
	int i;
	struct ar531x_boarddata sysBoardData;

	if ((!wmac) && (instNo >=2)) {
		uiPrintf("Error:: Only two ethernet card present \n");
		return -1;
	}

	p = (char *)&sysBoardData;
	for (i=0; i < sizeof(sysBoardData); i++) {
		p[i] = sysFlashConfigRead(devNum, FLC_BOARDDATA, i);
	}

	if (wmac) {
	    if (instNo == 0) {
		memcpy((void *)macAddr,(void *)sysBoardData.wlan0Mac, 6);
	    }
	    else {
		memcpy((void *)macAddr,(void *)sysBoardData.wlan1Mac, 6);
	    }
	} else {
		if (instNo == 0) {
			memcpy((void *)macAddr,(void *)sysBoardData.enet0Mac, 6);
		} else {
			memcpy((void *)macAddr,(void *)sysBoardData.enet1Mac, 6);
		}
	} 
#ifdef CHK_BRD_DATA	
	printf("freedomGetMacAddr: %x:%x:%x:%x:%x:%x \n",macAddr[0],macAddr[1],macAddr[2],macAddr[3],macAddr[4],macAddr[5]);
#endif	
	
	return 0;
     
}

//similar to ftpdownload, except this will be called from ART/MDK 
//rather than use the cli interface for prompts
#define FTP_BUFSIZE	1024
int ftpDownloadFile
(
 char *hostname,
 char *user,
 char *passwd,
 char *remotefile,
 char *localfile
)
{
        /*
	int fd, errfd, rc, wrc, wfd, cnt, eof;
//	struct cli_s *pMyCli = cliPtrGet();
	char buf[FTP_BUFSIZE];
	int bcnt, error;
	int download = 1;  //hardcode download = 1
	
	printf("SNOOP: h = %s\n u = %s\n p = %s\n r = %s\n l = %s\n",
		hostname, user, passwd, remotefile, localfile);

	if (download)
		{
			wfd = open(localfile, O_CREAT|O_WRONLY, 0600);
			error = wfd < 0 ? 1 : 0;
		}
	else
		{
			fd = open(localfile, O_RDONLY, 0600);
			error = fd < 0 ? 1 : 0;
		}
	if (error)
		{
			uiPrintf("can't open %s\n", localfile);
			return 0;
		}

	if (download)
		uiPrintf("Getting %s@%s:%s -> %s\n",
				 user, hostname, remotefile, localfile);
	else
		uiPrintf("Putting %s -> %s@%s:%s\n",
				 localfile, user, hostname, remotefile);

	rc = ftpXfer(hostname, user, passwd, "",
				 download ? "RETR %s" : "STOR %s",
				 "", remotefile,
	             &errfd,
				 download ? &fd : &wfd);


	if (rc == ERROR)
		{
			uiPrintf("ftp connection failed\n");
			close(download ? wfd : fd);
			return 0;
		}

	bcnt = cnt = 0;
	while (1)
		{
			rc = read(fd, buf, FTP_BUFSIZE);
			if (rc == 0) break;
			if (rc < 0)
				{
					uiPrintf("\n%s read failed, rc: %d\n", download ? "remote" : "local", rc);
					break;
				}
			bcnt += rc;
		
			wrc = write(wfd, buf, rc);
			if (wrc != rc)
				{
					uiPrintf("\n%s write failed (%d != %d)\n", download ? "local" : "remote", wrc, rc);
					break;
				}


			uiPrintf("#");
			if (cnt++ == 64)
				{
					cnt = 0;
					uiPrintf("\n");
				}
		}
	uiPrintf("\ndone\n");
	uiPrintf("%d bytes\n", bcnt);

	close(fd);
	close(wfd);

	ftpReplyGet (errfd, TRUE);
	ftpCommand (errfd, "QUIT", 0, 0, 0, 0, 0, 0);

	close(errfd);
	return 1;
    */
}


#ifdef INCLUDE_SCREENING_TESTS

// Sdram controller configuration programmed in romInit.S
// B = 1 (4 Bank device), T = 0 (x16 or x32 memory device), F = 0(not 256 Mb device)
// Refer to the ARM sdram controller specification for more details
// The mapping between the physical address used in the program and the RAS
// address generated by the sdram controller is 
// Sdram Ras Address: 14  13  12  11  10  9  8  7  6  5  4  3  2  1  0
// Physical Address : 11* 10  11  23  22  21 20 19 18 17 16 15 14 13 12

static int sdramTest()
{
	A_UINT32 *memAddr;
	A_UINT32 *dataAddr;
	A_UINT32 temp;
	A_UINT32 intLevel;
	A_UINT32 i;
	A_UINT32 j;
	

	printf("Sdram test ");

	// Allocate 1 MB memory
	memAddr = (A_UINT32 *)A_MALLOC(1048 * 1024); 
	if (memAddr == NULL) {
		uiPrintf("Failed:: Cannot Allocate physical memory to do the test \n");
		return -1;
	}
	

	printf(".");

	// Data Pattern Test
	for (i=0;i<512;i++) {
		if (!(i % 32)) printf(".");
		dataAddr = (A_UINT32 *)(memAddr + i);
		for (j=0;j<12;j++) {
			*dataAddr = dataPattern[j];
			if (*dataAddr != dataPattern[j]) {
				uiPrintf("Failed:: Wrote %x, Read Back = %x \n",dataPattern[j],*memAddr);
				A_FREE(memAddr);  // free memory
				return -1;
			}
		}
	}

	// Address bit test
	// Need to lock the interrupt as the address space used may collide with
	// other processes. Save the value before it is modified and restore back.
	intLevel = intLock();

	// Flip the bits 10 to 22 one by one 
	for (i=10;i<23;i++) {
		if (!(i % 2)) printf(".");
		dataAddr = (A_UINT32 *)(0xA07FFFFC & ~(1 << i));
		temp = *dataAddr;
		*dataAddr = 0xDEADDEAD;
		if (*dataAddr != 0xDEADDEAD) {
			uiPrintf("Failed:: Wrote 0xDEADEAD, Read Back = %x \n",*dataAddr);
			*dataAddr = temp;
			break;
		}
		*dataAddr = temp;
	}

	intUnlock(intLevel);	

	// Free the memory allocated	
	A_FREE(memAddr);

	if (i != 23) return -1;

	printf("Passed \n");
	return 0;
}


// Flash test writes to the flash data patterns and reads it back
// The flash space allocated for storing the radio configuration data is used for
// this purpose.

static int flashTest()
{
	A_INT32 i;
	A_UINT16 flashVal;
	A_UINT16 temp;
	
	printf("Flash test ");

	for (i=0;i<8;i+=2) {
		uiPrintf(".");
        	temp = (sysFlashConfigRead(0, FLC_RADIOCFG, i) << 8) | 
			       sysFlashConfigRead(0, FLC_RADIOCFG, (i + 1));
		
		flashVal = 0xAAAA;
		sysFlashConfigWrite(0, FLC_RADIOCFG, i, (A_INT8 *)&flashVal, 2);	

        	flashVal = (sysFlashConfigRead(0, FLC_RADIOCFG, i) << 8) | 
			       sysFlashConfigRead(0, FLC_RADIOCFG, (i + 1));
				
		if (flashVal != 0xAAAA) {
			uiPrintf("Failed:: Wrote 0xDEAD, Read Back = %x \n",flashVal);
			sysFlashConfigWrite(0, FLC_RADIOCFG, i, (A_INT8 *)&temp, 2);	
			break;
		}

		flashVal = 0x5555;
		sysFlashConfigWrite(0, FLC_RADIOCFG, i, (A_INT8 *)&flashVal, 2);	

        	flashVal = (sysFlashConfigRead(0, FLC_RADIOCFG, i) << 8) | 
			       sysFlashConfigRead(0, FLC_RADIOCFG, (i + 1));
		
		if (flashVal != 0x5555) {
			uiPrintf("Failed:: Wrote 0xDEAD, Read Back = %x \n",flashVal);
			sysFlashConfigWrite(0, FLC_RADIOCFG, i, (A_INT8 *)&temp, 2);	
			break;
		}

		sysFlashConfigWrite(0, FLC_RADIOCFG, i, (A_INT8 *)&temp, 2);	
	}
	
	printf("Passed \n");
	return 0;
}

// This test uses the hit and invalidate cache instruction. 
// Store the following cache tag at index 0 by referring to the Address
// Address	index 		way 		tag
// 0x80000000 	 0 		 00 		0x7f00f2
// 0x80001000	 0		 01		0x5f00f2
// 0x80002000	 0		 02		0x3f00f2
// 0x80003000	 0 		 03		0x1f00f2
// A hit and invalidate instruction on address 0x807f0000 should have a cache hit
// on index 0,way 00 and should invaludate the cache line. Read the cache tag and 
// check whether the cache line is invalidated. Repeat the same for addresses 
// 0x805f0000,0x803f0000, 0x801f0000

static int procSpeedTest()
{
	A_UINT32 intLevel;
	A_UINT32 result;

	printf("Processor speed test ");

	// Need to lock the interrupt as the address space used may collide with
	// other processes. 
	result = 1;
	intLevel = intLock();

	__asm__(
		"li $8,0x7f00f2 \n\t"	// store the tag in TagLo register 
		"mtc0 $8,$28 \n\t"	// PhyAddr = 0x7f0000

		"li $4,0x80000000 \n\t"	// load the tag into the cache line 
		"cache 0x8,0x0($4) \n\t"// index and way is calculated using the address

		"li $8,0x5f00f2 \n\t"
		"mtc0 $8,$28 \n\t"

		"li $4,0x80001000 \n\t"
		"cache 0x8,0x0($4) \n\t"

		"li $8,0x3f00f2 \n\t"
		"mtc0 $8,$28 \n\t"

		"li $4,0x80002000 \n\t"
		"cache 0x8,0x0($4) \n\t"

		"li $8,0x1f00f2 \n\t"
		"mtc0 $8,$28 \n\t"

		"li $4,0x80003000 \n\t"
		"cache 0x8,0x0($4) \n\t"

		"li $4, 0x807f0000 \n\t"   // Hit and invalidate the address 0x7f0000
		"cache 0x10, 0x0($4) \n\t" 

		"li $4,0x80000000 \n\t"	// Read the tag
		"cache 0x4,0x0($4) \n\t"// index and way is calculated using the address

		"mfc0 $8,$28 \n\t"
		"bne $8,0x7f0000,1f \n\t"

		"li $4, 0x805f0000 \n\t"   // Hit and invalidate the address 0x5f0000
		"cache 0x10, 0x0($4) \n\t" 

		"li $4,0x80001000 \n\t"	// Read the tag
		"cache 0x4,0x0($4) \n\t"// index and way is calculated using the address

		"mfc0 $8,$28 \n\t"
		"bne $8,0x5f0000,1f \n\t"

		"li $4, 0x803f0000 \n\t"   // Hit and invalidate the address 0x3f0000
		"cache 0x10, 0x0($4) \n\t" 

		"li $4,0x80002000 \n\t"	// Read the tag
		"cache 0x4,0x0($4) \n\t"// index and way is calculated using the address

		"mfc0 $8,$28 \n\t"
		"bne $8,0x3f0000,1f \n\t"

		"li $4, 0x801f0000 \n\t"   // Hit and invalidate the address 0x1f0000
		"cache 0x10, 0x0($4) \n\t" 

		"li $4,0x80003000 \n\t"	// Read the tag
		"cache 0x4,0x0($4) \n\t"// index and way is calculated using the address

		"mfc0 $8,$28 \n\t"
		"bne $8,0x1f0000,1f \n\t"

		"j 2f \n\t"

		"1: sw $0,%0 \n\t"
		
		"2: \n\t"

		: "=m" (result) 
		: 
		);

	intUnlock(intLevel);

	if (result == 0) {
		uiPrintf("Hit and Invalidate Failed \n");
		return -1;
	}
	
	printf("Passed \n");
	return 0;
}

static void wmac0RegWrite
(
	A_UINT32 reg,
	A_UINT32 data
)
{
	sysRegWrite(AR531X_WLAN0+reg,data);
}

static A_UINT32 wmac0RegRead
(
	A_UINT32 reg
)
{
	return sysRegRead(AR531X_WLAN0+reg);
}

static void wmac1RegWrite
(
	A_UINT32 reg,
	A_UINT32 data
)
{
	sysRegWrite(AR531X_WLAN1+reg,data);
}

static A_UINT32 wmac1RegRead
(
	A_UINT32 reg
)
{
	return sysRegRead(AR531X_WLAN1+reg);
}


static void eth1MacRegWrite
(
	A_UINT32 reg,
	A_UINT32 data
)
{
	sysRegWrite(ETH1_MAC_BASE_ADDR+reg,data);
}

static A_UINT32 eth1MacRegRead
(
	A_UINT32 reg
)
{
	return sysRegRead(ETH1_MAC_BASE_ADDR+reg);
}

static void eth1DmaRegWrite
(
	A_UINT32 reg,
	A_UINT32 data
)
{
	sysRegWrite(ETH1_DMA_BASE_ADDR+reg,data);
}

static A_UINT32 eth1DmaRegRead
(
	A_UINT32 reg
)
{
	return sysRegRead(ETH1_DMA_BASE_ADDR+reg);
}


static int wmac0RegTest()
{
	struct regStruct mac_bb_Reg[NO_MAC_BB_REG_TEST] = {
		{F2_RXDP,0xFFFFFFFC},
		{F2_IMR,0x0F9FFFFF},
		{0x9820,0xFFFFFFFF}
	};
	A_UINT32 i;
	A_UINT32 j;
	A_UINT32 regVal;
	A_UINT32 regValBackup;
	A_UINT32 regOffset;
	A_UINT32 rdData,wrData;

	// cold reset the wmac
	sysRegWrite(AR531X_RESET, sysRegRead(AR531X_RESET) | RESET_WLAN0);
	sysUDelay(10000);

	sysRegWrite(AR531X_RESET, sysRegRead(AR531X_RESET) & ~(RESET_WLAN0 | RESET_WARM_WLAN0_MAC | RESET_WARM_WLAN0_BB));
	sysUDelay(10000);
	
	// MAC and BB Register Test
	for (i=0;i<NO_MAC_BB_REG_TEST;i++) {
		regOffset=mac_bb_Reg[i].regOffset;
		regValBackup = wmac0RegRead(regOffset);
		for (j=0;j<32;j++) {
			regVal = (1 << j) & mac_bb_Reg[i].regMask;
			wmac0RegWrite(regOffset,regVal);
			
			if ((wmac0RegRead(regOffset) & mac_bb_Reg[i].regMask)!=regVal) {
				printf("Error: Register value doesnt match the value writeen \n");
				break;
			}
		}
		wmac0RegWrite(regOffset,regValBackup);
		if (j != 32) break;	
	}
	if (i != NO_MAC_BB_REG_TEST) {
		return -1;
	}

	wmac0RegWrite(0x9800, 0x47);

	//Analog Register Test
	//Disable AGC to A2 traffic
	wmac0RegWrite(0x9808,wmac0RegRead(0x9808) | 0x08000000);
	for (i=0; i < 0x40; i++) {
		wrData = i & 0x3f;

		// ------ DAC 1 Write -------
		wmac0RegWrite(PHY_BASE+(0x35<<2), (reverseBits(wrData, 6) << 16) 
				| (reverseBits(wrData, 6) << 8) | 0x24);      
		wmac0RegWrite(PHY_BASE+(0x34<<2), 0x00120017);      
	    
		for (j=0; j<18; j++) {
			wmac0RegWrite(PHY_BASE+(0x20<<2), 0x00010000);
		}
    
		rdData = reverseBits((wmac0RegRead(PHY_BASE+(256<<2)) >> 26) & 0x3f, 6);
		
		if (rdData != wrData) {
			printf("Error: Analog register value doesnt match the value writeen \n");
			break;
		}
    
		wmac0RegWrite(PHY_BASE+(0x34<<2), 0x00110017);      
    
		for (j=0; j<18; j++) {
			wmac0RegWrite(PHY_BASE+(0x20<<2), 0x00010000);
		}
    
		rdData = reverseBits((wmac0RegRead(PHY_BASE+(256<<2)) >> 26) & 0x3f, 6);
    
		if (rdData != wrData) {
			printf("Error: Analog register value doesnt match the value writeen \n");
			break;
		}
	}
	wmac0RegWrite(PHY_BASE+(0x34<<2), 0x14);

	//Enable AGC to A2 traffic
	wmac0RegWrite(0x9808,wmac0RegRead(0x9808) & (~0x08000000));

	if (i != 0x40) {
		return -1;
	}
	return 0;
}

static int wmac1RegTest()
{
	struct regStruct mac_bb_Reg[NO_MAC_BB_REG_TEST] = {
		{F2_RXDP,0xFFFFFFFC},
		{F2_IMR,0x0F9FFFFF},
		{0x9820,0xFFFFFFFF}
	};
	A_UINT32 i;
	A_UINT32 j;
	A_UINT32 regVal;
	A_UINT32 regValBackup;
	A_UINT32 regOffset;
	A_UINT32 rdData,wrData;

// reseting the wmac1 also resets the uart commented for now
#if 0
	// cold reset the wmac
	sysRegWrite(AR531X_RESET, sysRegRead(AR531X_RESET) | RESET_WLAN1);
	sysUDelay(10000);
#endif

	sysRegWrite(AR531X_RESET, sysRegRead(AR531X_RESET) & ~(RESET_WLAN1 | RESET_WARM_WLAN1_MAC| RESET_WARM_WLAN1_BB));
	sysUDelay(10000);
	
	// MAC and BB Register Test
	for (i=0;i<NO_MAC_BB_REG_TEST;i++) {
		regOffset=mac_bb_Reg[i].regOffset;
		regValBackup = wmac1RegRead(regOffset);
		for (j=0;j<32;j++) {
			regVal = (1 << j) & mac_bb_Reg[i].regMask;
			wmac1RegWrite(regOffset,regVal);
			
			if ((wmac1RegRead(regOffset) & mac_bb_Reg[i].regMask)!=regVal) {
				printf("Error: Register value doesnt match the value writeen \n");
				break;
			}
		}
		wmac1RegWrite(regOffset,regValBackup);
		if (j != 32) break;	
	}
	if (i != NO_MAC_BB_REG_TEST) {
		return -1;
	}

	wmac1RegWrite(0x9800, 0x47);

	//Analog Register Test
	//Disable AGC to A2 traffic
	wmac1RegWrite(0x9808,wmac1RegRead(0x9808) | 0x08000000);
	for (i=0; i < 0x40; i++) {
		wrData = i & 0x3f;

		// ------ DAC 1 Write -------
		wmac1RegWrite(PHY_BASE+(0x35<<2), (reverseBits(wrData, 6) << 16) 
				| (reverseBits(wrData, 6) << 8) | 0x24);      
		wmac1RegWrite(PHY_BASE+(0x34<<2), 0x00120017);      
	    
		for (j=0; j<18; j++) {
			wmac1RegWrite(PHY_BASE+(0x20<<2), 0x00010000);
		}
    
		rdData = reverseBits((wmac1RegRead(PHY_BASE+(256<<2)) >> 26) & 0x3f, 6);
		
		if (rdData != wrData) {
			printf("Error: Analog register value doesnt match the value writeen \n");
			break;
		}
    
		wmac1RegWrite(PHY_BASE+(0x34<<2), 0x00110017);      
    
		for (j=0; j<18; j++) {
			wmac1RegWrite(PHY_BASE+(0x20<<2), 0x00010000);
		}
    
		rdData = reverseBits((wmac1RegRead(PHY_BASE+(256<<2)) >> 26) & 0x3f, 6);
    
		if (rdData != wrData) {
			printf("Error: Analog register value doesnt match the value writeen \n");
			break;
		}
	}
	wmac1RegWrite(PHY_BASE+(0x34<<2), 0x14);

	//Enable AGC to A2 traffic
	wmac1RegWrite(0x9808,wmac1RegRead(0x9808) & (~0x08000000));

	if (i != 0x40) {
		return -1;
	}
	return 0;
}


static int eth1RegTest()
{
	struct regStruct eth1_Mac_Reg[NO_ETH1_MAC_REG_TEST] = {
		{MacAddrLow,0xFFFFFFFF},
	};
	struct regStruct eth1_Dma_Reg[NO_ETH1_DMA_REG_TEST] = {
		{DmaRxBaseAddr,0xFFFFFFFC},
		{DmaInterrupt,0x1C5EF}
	};
	A_UINT32 i;
	A_UINT32 j;
	A_UINT32 regVal;
	A_UINT32 regValBackup;
	A_UINT32 regOffset;

  	// Ensure DMA interface 
    	sysRegWrite(AR531X_ENABLE, sysRegRead(AR531X_ENABLE) | ENABLE_ENET1);

	// reset the device
    	sysRegWrite(AR531X_RESET, sysRegRead(AR531X_RESET) | (RESET_ENET1|RESET_EPHY1));
	sysUDelay(10000);
	sysRegWrite(AR531X_RESET, sysRegRead(AR531X_RESET) & (~(RESET_ENET1|RESET_EPHY1)));
	sysUDelay(10000);
		
	for (i=0;i<NO_ETH1_MAC_REG_TEST;i++) {
		regOffset=eth1_Mac_Reg[i].regOffset;
		regValBackup = eth1MacRegRead(regOffset);
		for (j=0;j<32;j++) {
			regVal = (1 << j) & eth1_Mac_Reg[i].regMask;
			eth1MacRegWrite(regOffset,regVal);

			if ((eth1MacRegRead(regOffset) & eth1_Mac_Reg[i].regMask)!=regVal) {
				printf("Error: Register value doesnt match the value writeen \n");
				printf("regoffset = %x Value = %x Expected = %x \n",regOffset, eth1MacRegRead(regOffset) & eth1_Mac_Reg[i].regMask,regVal);
				
				break;
			}
		}
		eth1MacRegWrite(regOffset,regValBackup);
		if (j != 32) break;	
	}
	if (i != NO_ETH1_MAC_REG_TEST) {
		return -1;
	}


	for (i=0;i<NO_ETH1_DMA_REG_TEST;i++) {
		regOffset=eth1_Dma_Reg[i].regOffset;
		regValBackup = eth1DmaRegRead(regOffset);
		for (j=0;j<32;j++) {
			regVal = (1 << j) & eth1_Dma_Reg[i].regMask;
			eth1DmaRegWrite(regOffset,regVal);
			if ((eth1DmaRegRead(regOffset) & eth1_Dma_Reg[i].regMask)!=regVal) {
				printf("Error: Register value doesnt match the value writeen \n");
				printf("regoffset = %x Value = %x Expected = %x \n",regOffset, eth1DmaRegRead(regOffset) & eth1_Mac_Reg[i].regMask,regVal);
				break;
			}
		}
		eth1DmaRegWrite(regOffset,regValBackup);
		if (j != 32) break;	
	}
	if (i != NO_ETH1_DMA_REG_TEST) {
		return -1;
	}


	return 0;
}



static int regTest()
{
	printf("Register test ");


	if (wmac0RegTest() == -1) {
		printf("Wmac 0 reg test failed \n");
		return -1;
	}
	if (wmac1RegTest() == -1) {
		printf("Wmac 1 reg test failed \n");
		return -1;
	}
	if (eth1RegTest() == -1) {
		printf("Eth1 reg test failed \n");
		return -1;
	}

	printf("Passed \n");
	
	return 0;
}

static void eth1MiiRegWrite
(
	A_UINT32 reg,
	A_UINT16 data
)
{
	A_UINT32 addr;

	eth1MacRegWrite(MacMiiData,data);
	addr = (ETH1_PHY_ADDR << MiiDevShift) & MiiDevMask |
	       (reg << MiiRegShift) & MiiRegMask |
	       MiiWrite;

	eth1MacRegWrite(MacMiiAddr,addr);
	
	do {
	} while((eth1MacRegRead(MacMiiAddr) & MiiBusy) == MiiBusy);
}

static A_UINT16 eth1MiiRegRead
(
	A_UINT32 reg
)
{
	A_UINT32 addr;
	A_UINT16 data;

	addr = (ETH1_PHY_ADDR << MiiDevShift) & MiiDevMask |
	       (reg << MiiRegShift) & MiiRegMask;

	eth1MacRegWrite(MacMiiAddr,addr);
	
	do {
	} while((eth1MacRegRead(MacMiiAddr) & MiiBusy) == MiiBusy);

	data = eth1MacRegRead(MacMiiData) & 0xFFFF;

	return data;
}


static int ethernetTest()
{
	A_UINT32 regVal;
	A_UINT32 count;
	A_UINT32 *txDesc;
	A_UINT32 *rxDesc;
	A_UINT8 *txFrame;
	A_UINT8 *rxFrame;
	A_INT32 i;

	printf("Ethernet test ");

	txDesc = (A_UINT32 *)cacheDmaMalloc(4 * sizeof(A_UINT32));
	rxDesc = (A_UINT32 *)cacheDmaMalloc(4 * sizeof(A_UINT32));
	if ((txDesc == NULL) || (rxDesc == NULL)) {
		uiPrintf("Failed:: Cannot Allocate physical memory to do the test \n");
		return -1;
	} 
	txFrame = (A_UINT8 *)cacheDmaMalloc(TX_FRAME_SIZE);
	rxFrame = (A_UINT8 *)cacheDmaMalloc(RX_BUF_SIZE);
	if ((txFrame == NULL) || (rxFrame == NULL)) {
		uiPrintf("Failed:: Cannot Allocate physical memory to do the test \n");
		return -1;
	} 

  	// Ensure DMA interface 
    	sysRegWrite(AR531X_ENABLE, sysRegRead(AR531X_ENABLE) | ENABLE_ENET1);

	// reset the device
    	sysRegWrite(AR531X_RESET, sysRegRead(AR531X_RESET) | (RESET_ENET1|RESET_EPHY1));
	sysUDelay(10000);
//	sysRegWrite(AR531X_RESET, sysRegRead(AR531X_RESET) & (~(RESET_ENET1|RESET_EPHY1)));
	sysRegWrite(AR531X_RESET, sysRegRead(AR531X_RESET) & (~(RESET_EPHY1)));
	sysUDelay(10000);
	sysRegWrite(AR531X_RESET, sysRegRead(AR531X_RESET) & (~(RESET_ENET1)));
	sysUDelay(10000);

  
	// external loopback test

	// Write into PHY BMCR: 
#ifdef AR5311
	eth1MiiRegWrite(GEN_ctl,PHY_SW_RST);
	sysUDelay(100);
	eth1MiiRegWrite(GEN_ctl, DUPLEX);
#endif

#ifdef AR5312
	eth1MiiRegWrite(MV_PHY_CONTROL,MV_CTRL_SOFTWARE_RESET);
	sysUDelay(100);
#endif
	// Dma Reset
	eth1DmaRegWrite(DmaBusMode,DmaResetOn);
	sysUDelay(10000);
	eth1DmaRegWrite(DmaBusMode,DmaResetOn);
	sysUDelay(10000);

	//  Program the DMA and MAC register
	eth1DmaRegWrite(DmaBusMode,DmaBurstLength4 | DmaLittleEndianDesc | DmaBigEndianData | DmaResetOff);
	eth1DmaRegWrite(DmaBusMode, DmaBusModeInit);
	eth1DmaRegWrite(DmaInterrupt, DmaIntDisable);

	eth1MacRegWrite(MacControl,	MacFilterOn | MacLittleEndian | MacLoopbackInt | MacFullDuplex | 
				MacPromiscuousModeOn | MacTxEnable |
				MacRxEnable);

	txDesc[0]=0x80000000;
	txDesc[1]=(1 << 30) | (1 << 29) | (1 << 25) | TX_FRAME_SIZE;
	txDesc[2]=(A_UINT32)txFrame;
	txDesc[3]=0x00000000;

	rxDesc[0]=0x80000000;
	rxDesc[1]= (1 << 25) | RX_BUF_SIZE; // end of ring & Rx buffer size
	rxDesc[2]=(A_UINT32)rxFrame;
	rxDesc[3]=0x00000000;

	txFrame[0]=0xff;
	txFrame[1]=0xff;
	txFrame[2]=0xff;
	txFrame[3]=0xff;
	txFrame[4]=0xff;
	txFrame[5]=0xff;
	txFrame[6]=0x22;
	txFrame[7]=0x33;
	txFrame[8]=0x44;
	txFrame[9]=0x55;
	txFrame[10]=0x66;
	txFrame[11]=0x77;
	txFrame[12]=TX_FRAME_SIZE & 0xff;
	txFrame[13]=(TX_FRAME_SIZE >> 8) & 0xff;

	for (i=14;i<TX_FRAME_SIZE;i++) {
		txFrame[i] = 0xa5;
	}

	for (i=0;i<RX_BUF_SIZE;i++) {
		rxFrame[i] = 0xff;
	}

	// Write the descriptor address
	eth1DmaRegWrite(DmaTxBaseAddr,(A_UINT32)txDesc);
	eth1DmaRegWrite(DmaRxBaseAddr,(A_UINT32)rxDesc);

	// Start Receieve and Transmit
	eth1DmaRegWrite(DmaControl, DmaRxStart | DmaTxStart);

	while (rxDesc[0] == 0x80000000);

	for (i=0;i<TX_FRAME_SIZE;i++) {
		if (rxFrame[i] != txFrame[i]) break;
	}

	cacheDmaFree(txDesc);
	cacheDmaFree(rxDesc);
	cacheDmaFree(txFrame);
	cacheDmaFree(rxFrame);

	if (i != TX_FRAME_SIZE) {
		printf("Error: Tx and Rx frame doesnt match \n");		
		return -1;
	}

	printf("Passed \n");
	return 0;
}

A_INT32 runScreeningTest
(
	A_UINT32 testId
)
{
	int ret;

	switch (testId) 
	{
		case SDRAM_TEST:
			ret=sdramTest();
			break;
		case FLASH_TEST:
			ret=flashTest();
			break;
		case PROC_SPEED_TEST:
			ret=procSpeedTest();
			break;
		case REG_TEST:
			ret=regTest();
			break;
		case ETHERNET_TEST:
			ret=ethernetTest();
			break;
		default:
			ret=-1;
			break;
	}
	
	return ret;
}

#endif // INCLUDE_SCREENING_TESTS

