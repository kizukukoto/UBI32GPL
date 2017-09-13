/* mConfig.c - contians configuration and setup functions for manlib */
/* Copyright (c) 2001 Atheros Communications, Inc., All Rights Reserved */

#ident  "ACI $Id: //depot/sw/src/dk/mdk/devlib/mConfig.c#389 $, $Header: //depot/sw/src/dk/mdk/devlib/mConfig.c#389 $"

/*
Revsision history
--------------------
1.0       Created.
*/

#ifdef VXWORKS
    #include "vxworks.h"
#endif

#ifdef __ATH_DJGPPDOS__
    #include <unistd.h>
    #ifndef EILSEQ
        #define EILSEQ EIO
    #endif  // EILSEQ

    #define __int64 long long
    #define HANDLE long
    typedef unsigned long DWORD;
    #define Sleep   delay
#endif  // #ifdef __ATH_DJGPPDOS__

#include "wlantype.h"
#include "ar5210reg.h"
#include "ar2413reg.h"

#if defined(SPIRIT_AP) || defined(FREEDOM_AP)
    #ifdef COBRA_AP
        #include "ar531xPlusreg.h"
    #else
        #include "ar531xreg.h"
    #endif
    
    #ifdef FALCON_ART_CLIENT
        #include "ar5513reg.h"
    #endif
#endif

#include "athreg.h"
#include "manlib.h"
#include "mdata.h"
#include "mEeprom.h"
#include "mConfig.h"
#include "mDevtbl.h"
#ifdef LINUX
    #include "../mdk/dk_ver.h"
#else
    #include "..\mdk\dk_ver.h"
#endif
#include "mCfg210.h"
#include "mCfg211.h"
#include "mData211.h"
#include "mEEP210.h"
#include "mEEP211.h"
#ifndef __ATH_DJGPPDOS__
    #include "mCfg513.h"
#endif
#include "mCfg5416.h"
#include "mEep5416.h"
#ifdef LINUX
    #include "mEep6000.h"
#endif
#include "artEar.h"
#include "art_ani.h"
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <math.h>
#include "mIds.h"
#include "rate_constants.h"

//#ifdef LINUX
//#include "linuxdrv.h"
//#else
//#include "ntdrv.h"
//#endif
#if !defined(VXWORKS) && !defined(LINUX)
#include <io.h>
#endif

#if (!CUSTOMER_REL) && (INCLUDE_DRIVER_CODE)
#if (MDK_AP)
int m_ar5212Reset(LIB_DEV_INFO *pLibDev, A_UCHAR *mac, A_UCHAR *bss, A_UINT32 freq, A_UINT32 turbo) {
        return -1;
}
#else
  #include "m_ar5212.h"
#endif
#endif //CUSTOMER_REL

// ARCH_BIG_ENDIAN is defined some where. Cannot determine where it is defined
// Undefined LINUX for now
//#ifdef LINUX
//#undef ARCH_BIG_ENDIAN
//#endif

// Mapping _access to access
#if defined (LINUX) || defined (__ATH_DJGPPDOS__ )
#define _access(fileName,mode) access((const char *)(fileName),(int)(mode))
#endif

#ifdef ARCH_BIG_ENDIAN
#include "endian_func.h"
#endif

//###########Temp place for ar5001 definitions
#define F2_EEPROM_ADDR      0x6000  // EEPROM address register (10 bit)
#define F2_EEPROM_DATA      0x6004  // EEPROM data register (16 bit)

#define F2_EEPROM_CMD       0x6008  // EEPROM command register
#define F2_EEPROM_CMD_READ   0x00000001
#define F2_EEPROM_CMD_WRITE  0x00000002
#define F2_EEPROM_CMD_RESET  0x00000004

#define F2_EEPROM_STS       0x600c  // EEPROM status register
#define F2_EEPROM_STS_READ_ERROR     0x00000001
#define F2_EEPROM_STS_READ_COMPLETE  0x00000002
#define F2_EEPROM_STS_WRITE_ERROR    0x00000004
#define F2_EEPROM_STS_WRITE_COMPLETE 0x00000008

#define MERLIN_MAC_REV_MASK 0xfffc0f00
#define MERLIN_1207      0x80000
#define MERLIN_0108      0x80100
#define MERLIN_0208		 0x80200

#define SOWL_MAC_ID         0x00040000
#define HOWL_MAC_ID         0x00000140
#define NEW_MAC_ID_VER_MASK 0xfffc0000
#ifdef OWL_PB42
extern A_UINT32 get_chip_id(A_UINT32 devIndex);
#define EEPROM_MEM_OFFSET  0x2000
#define EEPROM_STATUS_READ 0x407c
#define EEPROM_BUSY_M      0x10000
#endif
LIB_INFO gLibInfo = {0, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};
MANLIB_API A_UINT32 enableCal = DO_NF_CAL | DO_OFSET_CAL;

#define PHY_ADC_PARALLEL_CNTL (0x982c) /* adc parallel control register */
#define PWD_ADC_CL_MASK       (0xffff7fff) /* mask: clears bb_off_pwdAdc bit */

/* Rx gain type values */
#define RX_GAIN_ORIG           (2)
#define RX_GAIN_13dB_BACKOFF   (1)
#define RX_GAIN_23dB_BACKOFF   (0)

A_UINT32 lib_memory_range;
A_UINT32 cache_line_size;
static A_UINT16 cal_data_in_eeprom  = 1;
static A_UINT32 fastClkApply        = 0;
static A_INT32  tempMDKErrno        = 0;
static A_UINT32 fastClkFieldFound   = 0;
static A_UINT32 fastClkReg          = 0;

static char tempMDKErrStr[SIZE_ERROR_BUFFER];
#ifndef MDK_AP
static FILE     *logFileHandle;
static A_BOOL   logging = 0;
#endif

A_UINT32 gFreqMHz; /* global used when setting the pll in merlin */

PCI_VALUES pciValues[MAX_PCI_ENTRIES];

//MANLIB_API A_UINT32 checkSumLength=0x400; //default to 16k
//MANLIB_API A_UINT32 eepromSize=0;

// Local Functions
static void    initializeTransmitPower(A_UINT32 devNum, A_UINT32 freq,
    A_INT32  override, A_UCHAR  *pwrSettings);
static A_BOOL setChannel(A_UINT32 devNum, A_UINT32 freq);
static void setSpurMitigation(A_UINT32 devNum, A_UINT32 freq);
static void setSpurMitigationOwl(A_UINT32 devNum, A_UINT32 freq);
static void setSpurMitigationMerlin(A_UINT32 devNum, A_UINT32 freq);
static void testEEProm(A_UINT32 devNum);
static void enable_beamforming(A_UINT32 devNum);
static void init_beamforming_weights(A_UINT32 devNum);
#ifdef HEADER_LOAD_SCHEME
static A_BOOL setupEEPromHeaderMap(A_UINT32 devNum);
#endif //HEADER_LOAD_SCHEME
void Set11nMac2040( A_UINT32 devNum );

static PWR_CTL_PARAMS  pwrParams = { {FALSE, FALSE, FALSE},
                                    {0, 0, 0},
                                    {0, 0, 0},
                                    {0, 0, 0},
                                    {0, 0, 0},
                                    {0, 0, 0},
                                    {0, 0, 0},
                                    {0, 0, 0},
                                    {0, 0, 0},
                                    {0, 0, 0} };

A_UINT32 hwReset (A_UINT32 devNum, A_UINT32 resetMask) {
    LIB_DEV_INFO *pLibDev = gLibInfo.pLibDevArray[devNum];

    //printf("SNOOP:::hwReset passing to %s\n", (pLibDev->devMap.remoteLib?"remote":"local"));

    if (pLibDev->devMap.remoteLib)
       return (pLibDev->devMap.r_hwReset(devNum, resetMask));
    else {
       ar5kInitData[pLibDev->ar5kInitIndex].pMacAPI->hwReset(devNum, resetMask);
       return(0xDEADBEEF); // it appears hwReset is overloaded to return macRev ONLY for remoteLib.
                          // return a bogus value in other case to suppress warnings. PD 1/04
    }
}

void pllProgram (A_UINT32 devNum, A_UINT32 turbo) {

    LIB_DEV_INFO *pLibDev = gLibInfo.pLibDevArray[devNum];

    if (pLibDev->devMap.remoteLib) {
#ifndef _IQV
        if((ar5kInitData[pLibDev->ar5kInitIndex].deviceID & 0xf000 == 0xa000) && pLibDev->mode == MODE_11B)
           changeField(devNum,"bb_tx_frame_to_xpab_on", 0x7);
#else
        if(((ar5kInitData[pLibDev->ar5kInitIndex].deviceID & 0xf000) == 0xa000) && pLibDev->mode == MODE_11B)
           changeField(devNum,"bb_tx_frame_to_xpab_on", 0x7);
#endif	// _IQV
        pLibDev->devMap.r_pllProgram(devNum, turbo, pLibDev->mode);
    } else {
        ar5kInitData[pLibDev->ar5kInitIndex].pMacAPI->pllProgram(devNum, turbo);
    }
}

MANLIB_API A_INT32 getNextDevNum() {
    A_UINT32 i;
    for(i = 0; i < LIB_MAX_DEV; i++) {
        if(gLibInfo.pLibDevArray[i] == NULL) {
            return i;
        }
    }
    return -1;
}

/**************************************************************************
* initializeDevice - Setup all structures associated with this device
*
*/
MANLIB_API A_UINT32 initializeDevice
(
 struct DeviceMap map
)
{
    A_UINT32     i, devNum = LIB_MAX_DEV;
    LIB_DEV_INFO *pLibDev;

    if (map.remoteLib) { // Thin client
        lib_memory_range = 100 * 1024; // 100kB
        cache_line_size = 0x10;
    }
    else { // library present locally
        lib_memory_range = 1048576; // 1mB
        cache_line_size = 0x20;
    }

    //printf("SNOOP:: intializeDevice called\n");
    if(gLibInfo.devCount+1 > LIB_MAX_DEV) {
        mError(0xdead, EINVAL, "initializeDevice: Cannot initialize more than %d devices\n", LIB_MAX_DEV);
        return 0xdead;
    }

    // Device sanity checks
    if(map.DEV_CFG_RANGE != LIB_CFG_RANGE) {
        mError(0xdead, EINVAL, "initializeDevice: DEV_CFG_RANGE should be %d, was given %d\n", LIB_CFG_RANGE, map.DEV_CFG_RANGE);
        return 0xdead;
    }
    // Atleast 1 MB of memory is required

    if(map.DEV_MEMORY_RANGE < lib_memory_range) {
        mError(0xdead, EINVAL, "initializeDevice: DEV_MEMORY_RANGE should be more than %d, was given %d\n", lib_memory_range, map.DEV_MEMORY_RANGE);
        return 0xdead;
    }

    if(map.DEV_REG_RANGE != LIB_REG_RANGE) {
        mError(0xdead, EINVAL, "initializeDevice: DEV_REG_RANGE should be %d, was given %d\n", LIB_REG_RANGE, map.DEV_REG_RANGE);
        return 0xdead;
    }

    //check for the allocated memory being cache line aligned.
        if (map.DEV_MEMORY_ADDRESS & (cache_line_size - 1)) {
            mError(0xdead, EINVAL,
                "initializeDevice: DEV_MEMORY_ADDRESS should be cache line aligned (0x%x), was given 0x%08lx\n",
                cache_line_size, map.DEV_MEMORY_ADDRESS);
            return 0xdead;
        }

        for(i = 0; i < LIB_MAX_DEV; i++) {
        if(gLibInfo.pLibDevArray[i] == NULL) {
            devNum = i;
            break;
        }
    }

    assert(i < LIB_MAX_DEV);

    // Allocate the structures for the new device
    gLibInfo.pLibDevArray[devNum] = (LIB_DEV_INFO *)malloc(sizeof(LIB_DEV_INFO));
        if  (gLibInfo.pLibDevArray[devNum] == NULL) {
       mError(devNum, ENOMEM, "initializeDevice: Failed to allocate the library device information structure\n");
       return 0xdead;
    }

    // Zero out all structures
    pLibDev = gLibInfo.pLibDevArray[devNum];
    memset(pLibDev, 0, sizeof(LIB_DEV_INFO));

    pLibDev->devNum = devNum;
    pLibDev->ar5kInitIndex = UNKNOWN_INIT_INDEX;
    gLibInfo.devCount++;
    memcpy(&(pLibDev->devMap), &map, sizeof(struct DeviceMap));

    /* Initialize the map bytes for tracking memory management: calloc will init to 0 as well*/
    pLibDev->mem.allocMapSize = (pLibDev->devMap.DEV_MEMORY_RANGE / BUFF_BLOCK_SIZE) / 8;
    pLibDev->mem.pAllocMap = (A_UCHAR *) calloc(pLibDev->mem.allocMapSize, sizeof(A_UCHAR));
    if(pLibDev->mem.pAllocMap == NULL) {
        mError(devNum, ENOMEM, "initializeDevice: Failed to allocate the allocation map\n");
        free(gLibInfo.pLibDevArray[devNum]);
        return 0xdead;
    }

    pLibDev->mem.pIndexBlocks = (A_UINT16 *) calloc((pLibDev->mem.allocMapSize * 8), sizeof(A_UINT16));
    if(pLibDev->mem.pIndexBlocks == NULL) {
        mError(devNum, ENOMEM, "initializeDevice: Failed to allocate the indexBlock map\n");
        free(gLibInfo.pLibDevArray[devNum]->mem.pAllocMap);
        free(gLibInfo.pLibDevArray[devNum]);
        return 0xdead;
    }
    pLibDev->mem.usingExternalMemory = FALSE;

    pLibDev->regFileRead = 0;
    pLibDev->regFilename[0] = '\0'; // empty string

    pLibDev->mdkErrno = 0;
    pLibDev->mdkErrStr[0] = '\0';
    pLibDev->libCfgParams.refClock = REF_CLK_DYNAMIC;
    pLibDev->devState = INIT_STATE;

    pLibDev->eepromStartLoc = 0x00; // default. set to 0x400 for the 2nd eeprom_block if present
    pLibDev->maxLinPwrx4    = 0;    // default init to 0 dBm

    pLibDev->libCfgParams.chainSelect = 0x1;
    pLibDev->libCfgParams.flashCalSectorErased = FALSE;
    pLibDev->libCfgParams.useShadowEeprom = FALSE;
    pLibDev->libCfgParams.useEepromNotFlash = FALSE; // AR5005 configured to use flash by default
    pLibDev->libCfgParams.tx_chain_mask = 0x1;
    pLibDev->libCfgParams.rx_chain_mask = 0x1;
    pLibDev->libCfgParams.chain_mask_updated_from_eeprom = FALSE;
    pLibDev->libCfgParams.extended_channel_op = 0;
    pLibDev->libCfgParams.extended_channel_sep = 20;
    pLibDev->libCfgParams.short_gi_enable = FALSE;
    pLibDev->libCfgParams.ht40_enable = FALSE;
    pLibDev->libCfgParams.enablePdadcLogging = FALSE;

    pLibDev->blockFlashWriteOn = FALSE;
    pLibDev->noEndPacket = FALSE;
	pLibDev->yesAgg = FALSE;

#ifndef __ATH_DJGPPDOS__
    // enable ART ANI and reuse by default
    configArtAniSetup(devNum, ART_ANI_ENABLED, ART_ANI_REUSE_ON);
#endif

    return devNum;
}

/**************************************************************************
* useMDKMemory - Repoints the libraries memory map at the one used by
*                MDK so they can share the map
*
*/

MANLIB_API void useMDKMemory
(
 A_UINT32 devNum,
 A_UCHAR *pExtAllocMap,
 A_UINT16 *pExtIndexBlocks
)
{
    LIB_DEV_INFO *pLibDev = gLibInfo.pLibDevArray[devNum];

    if(gLibInfo.pLibDevArray[devNum] == NULL) {
        mError(devNum, EINVAL, "Device Number %d:useMDKMemory\n", devNum);
        return;
    }
    if (gLibInfo.pLibDevArray[devNum]->devState < INIT_STATE) {
        mError(devNum, EILSEQ, "Device Number %d:useMDKMemory: initializeDevice must be called first\n", devNum);
        return;
    }
    if((pExtAllocMap == NULL) || (pExtIndexBlocks == NULL)) {
        mError(devNum, EINVAL, "Device Number %d:useMDKMemory: provided memory pointers are NULL\n", devNum);
        return;
    }
    // Free the current Memory Map
    free(gLibInfo.pLibDevArray[devNum]->mem.pIndexBlocks);
    free(gLibInfo.pLibDevArray[devNum]->mem.pAllocMap);
    gLibInfo.pLibDevArray[devNum]->mem.pIndexBlocks = NULL;
    gLibInfo.pLibDevArray[devNum]->mem.pAllocMap = NULL;

    // Assign to the MDK Memory Map
    pLibDev->mem.pAllocMap = pExtAllocMap;
    pLibDev->mem.pIndexBlocks = pExtIndexBlocks;
    pLibDev->mem.usingExternalMemory = TRUE;
}

MANLIB_API A_UINT16 getDevIndex
(
    A_UINT32 devNum
)
{
    if (gLibInfo.pLibDevArray[devNum] == NULL) {
        mError(devNum, EINVAL, "Device Number %d:getDevIndex \n", devNum);
        return (A_UINT16)-1;
    }

    return gLibInfo.pLibDevArray[devNum]->devMap.devIndex;
}
/**************************************************************************
* closeDevice - Free all structures associated with this device
*
*/
MANLIB_API void closeDevice
(
    A_UINT32 devNum
)
{
//      FIELD_VALUES *pFieldValues;

    if(gLibInfo.pLibDevArray[devNum] == NULL) {
        mError(devNum, EINVAL, "Device Number %d:closeDevice\n", devNum);
        return;
    }
    if(gLibInfo.pLibDevArray[devNum]->mem.usingExternalMemory == FALSE) {
        if(gLibInfo.pLibDevArray[devNum]->mem.pIndexBlocks) {
            free(gLibInfo.pLibDevArray[devNum]->mem.pIndexBlocks);
            gLibInfo.pLibDevArray[devNum]->mem.pIndexBlocks = NULL;
        }
        if (gLibInfo.pLibDevArray[devNum]->mem.pAllocMap) {
            free(gLibInfo.pLibDevArray[devNum]->mem.pAllocMap);
            gLibInfo.pLibDevArray[devNum]->mem.pAllocMap = NULL;
        }
    }

    if(gLibInfo.pLibDevArray[devNum]->eepData.infoValid) {
        freeEepStructs(devNum);
        gLibInfo.pLibDevArray[devNum]->eepData.infoValid = FALSE;
    }


    if (gLibInfo.pLibDevArray[devNum]->regArray) {
        free(gLibInfo.pLibDevArray[devNum]->regArray);
        gLibInfo.pLibDevArray[devNum]->regArray = NULL;
    }
    if (gLibInfo.pLibDevArray[devNum]->pModeArray) {
        free(gLibInfo.pLibDevArray[devNum]->pModeArray);
        gLibInfo.pLibDevArray[devNum]->pModeArray  = NULL;
    }

    if (gLibInfo.pLibDevArray[devNum]->pciValuesArray) {
        free(gLibInfo.pLibDevArray[devNum]->pciValuesArray);
        gLibInfo.pLibDevArray[devNum]->pciValuesArray = NULL;
    }

    if(gLibInfo.pLibDevArray[devNum]->pEarHead) {
        ar5212EarFree(gLibInfo.pLibDevArray[devNum]->pEarHead);
    }

    free(gLibInfo.pLibDevArray[devNum]);
    gLibInfo.pLibDevArray[devNum] = NULL;
    gLibInfo.devCount--;
}

/**************************************************************************
* specifySubSystemID - Tell library what subsystemID to try if can't find
*                      a match
*
*
*
*/
MANLIB_API void specifySubSystemID
(
 A_UINT32 devNum,
 A_UINT32   subsystemID
)
{
    mError(devNum, EIO, "Device Number %d:specifySubSystemID is an obsolete library command\n", devNum);

    devNum = 0;         //quiet compiler warning.
    subsystemID = 0;
    return;
}

/**************************************************************************
* setResetParams - Setup params prior to calling resetDevice
*
* These are new params added for second generation products
*
*/
MANLIB_API void setResetParams
(
 A_UINT32 devNum,
 A_CHAR     *pFilename,
 A_BOOL     eePromLoad,
 A_BOOL     forceCfgLoad,
 A_UCHAR    mode,
 A_UINT16     initCodeFlag
)
{
    LIB_DEV_INFO *pLibDev = gLibInfo.pLibDevArray[devNum];
    A_UINT16      i;

    if (checkDevNum(devNum) == FALSE)  {
        mError(devNum, EINVAL, "Device Number %d:setResetParams\n", devNum);
        return;
    }

    pLibDev->eePromLoad = eePromLoad;
//  pLibDev->eePromHeaderLoad = eePromHeaderLoad;
    pLibDev->mode = mode;
    pLibDev->use_init = initCodeFlag;

    if(eePromLoad == 0) {
        //force the eeprom header to re-apply, next time we load eeprom
        for (i = 0; i < 4; i++) {
            pLibDev->eepromHeaderApplied[i] = FALSE;
        }
    }

    if( forceCfgLoad ) {
        //force the pci values to be regenerated
        pLibDev->regFileRead = 0;
        //printf("SNOOP::: regFileRead to zero\n");

        //force the eeprom header to re-apply
        for( i = 0; i < 4; i++ ) {
            pLibDev->eepromHeaderApplied[i] = FALSE;
        }
    }

    if(( pFilename == '\0' ) || ( pFilename == NULL ) || (pFilename[0] == (A_CHAR)NULL)) {
        return;
   }


#ifndef MDK_AP
    //do a check for file exist and is readable
    if(_access(pFilename, 04) != 0) {
        mError(devNum, EINVAL, "Device Number %d:setRegFilename: either file [%s] does not exist or is not readable\n", devNum, pFilename);
        return;
    }

    //if there is a config file change then reset the config file reading
    if(strcmp(pLibDev->regFilename, pFilename) != 0) {
        pLibDev->regFileRead = 0;
//        printf("SNOOP::: regFileRead to zero\n");
    }

    strcpy(pLibDev->regFilename, pFilename);
#endif
}

/** 
 * - Function Name: UpdateLibParams
 * - Description  : updates lib params with values coming from 
 * - Arguments
 *     - 
 * - Returns      :                                                                
 *******************************************************************************/
void UpdateLibParams( A_UINT32 devNum, LIB_PARAMS* pLibParamsInfo ) {
    LIB_DEV_INFO *pLibDev = gLibInfo.pLibDevArray[devNum];

    pLibDev->libCfgParams.printPciWrites = pLibParamsInfo->printPciWrites;
    pLibDev->libCfgParams.beanie2928Mode = pLibParamsInfo->beanie2928Mode;
    pLibDev->libCfgParams.refClock       = pLibParamsInfo->refClock;
    pLibDev->libCfgParams.enableXR       = pLibParamsInfo->enableXR;
    pLibDev->libCfgParams.loadEar        = pLibParamsInfo->loadEar;
    pLibDev->libCfgParams.eepStartLocation = pLibParamsInfo->eepStartLocation;
    pLibDev->libCfgParams.artAniEnable     = pLibParamsInfo->artAniEnable;
    pLibDev->libCfgParams.artAniReuse      = pLibParamsInfo->artAniReuse;
    pLibDev->libCfgParams.chainSelect      = pLibParamsInfo->chainSelect;
    pLibDev->libCfgParams.ht40_enable      = pLibParamsInfo->ht40_enable;
    pLibDev->libCfgParams.tx_chain_mask    = pLibParamsInfo->tx_chain_mask;
    pLibDev->libCfgParams.rx_chain_mask    = pLibParamsInfo->rx_chain_mask;
    pLibDev->libCfgParams.short_gi_enable  = pLibParamsInfo->short_gi_enable;
    pLibDev->libCfgParams.rateMaskMcs20    = pLibParamsInfo->rateMaskMcs20;
    pLibDev->libCfgParams.rateMaskMcs40    = pLibParamsInfo->rateMaskMcs40;
    pLibDev->libCfgParams.extended_channel_op  = pLibParamsInfo->extended_channel_op;
    pLibDev->libCfgParams.extended_channel_sep = pLibParamsInfo->extended_channel_sep;
    pLibDev->libCfgParams.pdadcDelta           = pLibParamsInfo->pdadcDelta;
    pLibDev->libCfgParams.verifyPdadcTable     = pLibParamsInfo->verifyPdadcTable;
    pLibDev->libCfgParams.enablePdadcLogging   = pLibParamsInfo->enablePdadcLogging;
    pLibDev->libCfgParams.cal_data_in_eeprom   = pLibParamsInfo->cal_data_in_eeprom;
    pLibDev->libCfgParams.applyPLLOverride     = pLibParamsInfo->applyPLLOverride;
}
/* UpdateLibParams */

/**************************************************************************
* configureLibParams - Allow certain library params to be configured.
*                      Similar to setResetParams.  This function will
*                      use a strcuture to pass params as this will be device
*                      specific params that will grow through time.
*
*/
MANLIB_API void configureLibParams
(
 A_UINT32 devNum,
 LIB_PARAMS *pLibParamsInfo,
 A_UINT32 cpFlag /* added to avoid bug */
)
{
    LIB_DEV_INFO *pLibDev = gLibInfo.pLibDevArray[devNum];
    if (pLibDev->libCfgParams.loadEar != pLibParamsInfo->loadEar) {
        pLibDev->regFileRead = 0;
    }
#ifndef __ATH_DJGPPDOS__
if (!isFalcon(devNum)) {
    if (pLibDev->artAniSetup.Enabled != pLibParamsInfo->artAniEnable) {
        if (pLibParamsInfo->artAniEnable == ART_ANI_ENABLED) {
            enableArtAniSetup(devNum);
        } else {
            disableArtAniSetup(devNum);
        }
    }
}
    pLibDev->artAniSetup.Reuse = pLibParamsInfo->artAniReuse;
    pLibDev->artAniSetup.currNILevel = pLibParamsInfo->artAniNILevel;
    pLibDev->artAniSetup.currBILevel = pLibParamsInfo->artAniBILevel;
    pLibDev->artAniSetup.currSILevel = pLibParamsInfo->artAniSILevel;
#else
    pLibParamsInfo->artAniEnable = ART_ANI_DISABLED;
#endif

    if(isMerlin(pLibDev->swDevID)) {
        pLibDev->femBandSel = pLibParamsInfo->femBandSel;
    }

    // chainmask from eeprom gets precedence
    if ((pLibDev->libCfgParams.chain_mask_updated_from_eeprom) && isFalcon(devNum)) {
        pLibParamsInfo->tx_chain_mask = pLibDev->libCfgParams.tx_chain_mask;
        pLibParamsInfo->rx_chain_mask = pLibDev->libCfgParams.rx_chain_mask;
    }
    if (pLibDev->libCfgParams.useEepromNotFlash > 0) {
        pLibParamsInfo->useEepromNotFlash = pLibDev->libCfgParams.useEepromNotFlash;
    }

if (cpFlag)    {
    /* BAA: this is a problem. The perl script environment from which this  
     *      routine is also called is not aware of the new items that have been  
     *      added to pLibDev->libCfgParams from within the host, hence, a memcpy 
     *      here overwrites those params with values that are unknown/wrong.
     *      Only allow this memcpy() to be used if configureLibParams() is called
     *      from art_configureLibParams().
     */
    memcpy(&(pLibDev->libCfgParams), pLibParamsInfo, sizeof(LIB_PARAMS));
} else {
    /* BAA: if it gets here, it its because configureLibParams() was called from
     * m_bindDkParamsToDevnum() (perl environment), hence, update only those
     * params set up in m_bindDkParamsToDevnum()
     */
    UpdateLibParams(devNum,pLibParamsInfo);
}

    // Save cal_data_in_eeprom flag as static variable which will be used by isFlashCalData() of AP
    cal_data_in_eeprom = (A_UINT16) pLibParamsInfo->cal_data_in_eeprom;
    
    return;
}

/**************************************************************************
* getDeviceInfo - get a subset of the device info for display
    *
*/
MANLIB_API void getDeviceInfo(
 A_UINT32 devNum,
 SUB_DEV_INFO *pInfoStruct      //pointer to caller assigned struct
)
{
    LIB_DEV_INFO *pLibDev = gLibInfo.pLibDevArray[devNum];
    A_UINT32    strLength;

    if(pLibDev->ar5kInitIndex == UNKNOWN_INIT_INDEX) {
        mError(devNum, EIO, "Device Number %d:getDeviceInfo fail, run resetDevice first\n", devNum);
        return;
    }
    pInfoStruct->aRevID = pLibDev->aRevID;
    pInfoStruct->hwDevID = pLibDev->hwDevID;
    pInfoStruct->swDevID = ar5kInitData[pLibDev->ar5kInitIndex].swDeviceID;
    pInfoStruct->bbRevID = pLibDev->bbRevID;
    pInfoStruct->macRev = pLibDev->macRev;
    pInfoStruct->subSystemID = pLibDev->subSystemID;
    strLength = strlen(pLibDev->regFilename);
    if (strLength) {
        strcpy(pInfoStruct->regFilename, pLibDev->regFilename);
        pInfoStruct->defaultConfig = 0;
    }
    else {
        //copy in the default
        if (ar5kInitData[pLibDev->ar5kInitIndex].pCfgFileVersion) {
            strcpy(pInfoStruct->regFilename, ar5kInitData[pLibDev->ar5kInitIndex].pCfgFileVersion);
        } else {
            pInfoStruct->regFilename[0]='\0';
        }
        pInfoStruct->defaultConfig = 1;
    }
    sprintf(pInfoStruct->libRevStr, DEVLIB_VER1);

}

static A_BOOL analogRevMatch
(
    A_UINT32 index,
    A_UINT32 aRevID
)
{
    A_UINT16 i;
    A_UCHAR  aProduct  = (A_UCHAR)(aRevID & 0xf);
    A_UCHAR  aRevision = (A_UCHAR)((aRevID >> 4) & 0xf);

    if(ar5kInitData[index].pAnalogRevs == NULL) {
        return(TRUE);
    }

    for (i = 0; i < ar5kInitData[index].numAnalogRevs; i++) {
        if((aProduct == ar5kInitData[index].pAnalogRevs[i].productID) &&
            (aRevision == ar5kInitData[index].pAnalogRevs[i].revID)) {
            return (TRUE);
        }
    }
    return(FALSE);
}

static A_BOOL macRevMatch
(
    A_UINT32 index,
    A_UINT32 macRev
)
{
    A_UINT16 i;

    if(ar5kInitData[index].pMacRevs == NULL) {
        return (TRUE);
    }

    for(i = 0; i < ar5kInitData[index].numMacRevs; i++) {
        if(macRev == ar5kInitData[index].pMacRevs[i]) {
            return(TRUE);
        }
    }
    return(FALSE);
}

A_INT32 findDevTableEntry(A_UINT32 devNum)
{
    A_UINT32 i;
    LIB_DEV_INFO *pLibDev;
    static int printOnce = 0;

    pLibDev = gLibInfo.pLibDevArray[devNum];

    //FJC: 07/24/03 this function can now be called from outside resetDevice
    //so it needs to read the hw info incase it has not been read.
    //have copied this from resetDevice rather than move it to minimize
    //inpact.  Should readly optimize this so don't do the same stuff twice.
    pLibDev->hwDevID = (pLibDev->devMap.OScfgRead(devNum, 0) >> 16) & 0xffff;
//printf(" pLibDev->hwDevID is 0x%x\n", pLibDev->hwDevID);
	//Also read the subsystemID from the oscfg instead of a changing EEPROM interface
    pLibDev->subSystemID = (pLibDev->devMap.OScfgRead(devNum, 0x2C) >> 16) & 0xffff;
//printf(" pLibDev->subSystemID is 0x%x\n", pLibDev->subSystemID);
     /* Reset the device. */
     /* Masking the board specific part of the device id */
	switch (pLibDev->hwDevID & 0xff)   {
        case 0x0007:
#ifndef __ATH_DJGPPDOS__
            if (pLibDev->devMap.remoteLib)
            {
                pLibDev->macRev = hwReset(devNum, BUS_RESET | BB_RESET | MAC_RESET);
            }
            else
            {
                hwResetAr5210(devNum, BUS_RESET | BB_RESET | MAC_RESET);
#if defined(SPIRIT_AP) || defined(FREEDOM_AP)
#if !defined(COBRA_AP)
                pLibDev->macRev = (sysRegRead(AR531X_REV) >> REV_WMAC_MIN_S) & 0xff;
#endif
#else
                pLibDev->macRev = REGR(devNum, F2_SREV) & F2_SREV_ID_M;
#endif
            }
#endif
            break;
        case 0x0011:
        case 0x0012:
        case 0x0013:
        case 0x0014:
        case 0x0015:
        case 0x0016:
        case 0x0017:
        case 0x001a:
        case 0x001b:
//        case 0x001c:
        case 0x001f:
        case 0x00b0:
        case 0xa014:
        case 0xa016:
        case 0x0019:
        case (DEVICE_ID_COBRA&0xff):
            if (pLibDev->devMap.remoteLib)
                pLibDev->macRev = hwReset(devNum, BUS_RESET | BB_RESET | MAC_RESET);
            else
            {
                hwResetAr5211(devNum, BUS_RESET | BB_RESET | MAC_RESET);
#if defined(SPIRIT_AP) || defined(FREEDOM_AP)
#if defined(COBRA_AP)
            if((pLibDev->hwDevID & 0xff) == (DEVICE_ID_COBRA&0xff))
                pLibDev->macRev = (sysRegRead(AR531XPLUS_PCI+F2_SREV) & F2_SREV_ID_M);
            else {
                pLibDev->macRev = REGR(devNum, F2_SREV) & F2_SREV_ID_M;
            }
#else
    #ifdef AR5513
        pLibDev->macRev = (sysRegRead(AR5513_SREV) >> REV_MIN_S) & 0xff;
    #else
                pLibDev->macRev = (sysRegRead(AR531X_REV) >> REV_WMAC_MIN_S) & 0xff;
   #endif
#endif
#else
#ifdef FALCON_ART_CLIENT
//                pLibDev->macRev = (sysRegRead(AR5513_SREV) >> REV_MIN_S) & 0xff;
         pLibDev->macRev = sysRegRead(0x10100000 + F2_SREV) & F2_SREV_ID_M;
#else
                pLibDev->macRev = REGR(devNum, F2_SREV) & F2_SREV_ID_M;
#endif
#endif
                if (isGriffin_1_0(pLibDev->macRev) || isGriffin_1_1(pLibDev->macRev)) {
                    // to override corrupted 0x9808 in griffin 1.0 and 1.1. 2.0 should be fixed.
//                  printf("SNOOP: setting reg 0x9808 to 0 for griffin 1.0 and 1.1 \n");
                    REGW(devNum, 0x9808, 0);
                }
            }
            break;
        case (0x1d):

			if(pLibDev->hwDevID == 0xff1d) {
				// this is owl based device
              if (pLibDev->devMap.remoteLib) {
                pLibDev->macRev = hwReset(devNum, BUS_RESET | BB_RESET | MAC_RESET);
			  } else {
                  hwResetAr5416(devNum, BUS_RESET | BB_RESET | MAC_RESET);
                  pLibDev->macRev = REGR(devNum, F2_SREV) & F2_SREV_ID_M;

				  // Check if this is of 32bit macRev type. Old chips had 8bit and new chips have 32 bit macRev
				  if(pLibDev->macRev == 0xFF) {
					pLibDev->macRev = REGR(devNum, F2_SREV);
				  }
			  }

			} else {
  			  // This is nala based device
              if (pLibDev->devMap.remoteLib)
			    pLibDev->macRev = hwReset(devNum, BUS_RESET | BB_RESET | MAC_RESET);
              else
			  {
		        hwResetAr5211(devNum, BUS_RESET | BB_RESET | MAC_RESET);
#if defined(SPIRIT_AP) || defined(FREEDOM_AP)
#if defined(COBRA_AP)
		        if((pLibDev->hwDevID & 0xff) == (DEVICE_ID_COBRA&0xff))
		          pLibDev->macRev = (sysRegRead(AR531XPLUS_PCI+F2_SREV) & F2_SREV_ID_M);
		        else {
		          pLibDev->macRev = REGR(devNum, F2_SREV) & F2_SREV_ID_M;
				}
#else
                pLibDev->macRev = (sysRegRead(AR531X_REV) >> REV_WMAC_MIN_S) & 0xff;
#endif
#else
                pLibDev->macRev = REGR(devNum, F2_SREV) & F2_SREV_ID_M;
#endif
			    if (isGriffin_1_0(pLibDev->macRev) || isGriffin_1_1(pLibDev->macRev)) {
					// to override corrupted 0x9808 in griffin 1.0 and 1.1. 2.0 should be fixed.
//					printf("SNOOP: setting reg 0x9808 to 0 for griffin 1.0 and 1.1 \n");
				  REGW(devNum, 0x9808, 0);
				}
			  }
			}
			break;

		case (0x1c):
        case SW_DEVICE_ID_OWL:
        case SW_DEVICE_ID_HOWL: // Howl DevId is a027. This is ANDed with 0x00ff to obtain 0027 which is same as SOWL
        case SW_DEVICE_ID_OWL_PCIE:
        case SW_DEVICE_ID_SOWL:
        case SW_DEVICE_ID_SOWL_PCIE:
        case SW_DEVICE_ID_MERLIN:
        case SW_DEVICE_ID_MERLIN_PCIE:
            if (pLibDev->devMap.remoteLib) {
                pLibDev->macRev = hwReset(devNum, BUS_RESET | BB_RESET | MAC_RESET);
            } else {
                hwResetAr5416(devNum, BUS_RESET | BB_RESET | MAC_RESET);

#ifdef OWL_PB42
                if(isHowlAP(devNum)){
                  pLibDev->macRev=REGR(devNum,0x0600);
                }else{
#endif
                  pLibDev->macRev = REGR(devNum, F2_SREV) & F2_SREV_ID_M;
#ifdef OWL_PB42
                }
#endif
				// Check if this is of 32bit macRev type. Old chips had 8bit and new chips have 32 bit macRev
				if(pLibDev->macRev == 0xFF) {
					pLibDev->macRev = REGR(devNum, F2_SREV);
				}
            }
            break;
    }

    // Needed for venice FPGA
    if ((pLibDev->hwDevID == 0xf013) || (pLibDev->hwDevID == 0xfb13)
        || (pLibDev->hwDevID == 0xf016) || (pLibDev->hwDevID == 0xff16)) {
        REGW(devNum, 0x4014, 0x3);
        REGW(devNum, 0x4018, 0x0);
        REGW(devNum, 0xd87c, 0x16);
        mSleep(100);
    }

    // set it to ofdm mode for reading the analog rev id for 11b fpga
    if (pLibDev->hwDevID == 0xf11b) {
        REGW(devNum, 0xa200, 0);
        REGW(devNum, 0x987c, 0x19) ;
        mSleep(20);
    }

    //read the baseband revision
    pLibDev->bbRevID = REGR(devNum, PHY_CHIP_ID);
    //printf("bb rev id = %x\n", pLibDev->bbRevID);
    //Needed so that analog revID write will work.
    if((pLibDev->hwDevID & 0xff) >= 0x0012) {
        REGW(devNum, 0x9800, 0x07);
    }
    else {
        REGW(devNum, 0x9800, 0x47);
    }

    //read the analog revIDs
    REGW(devNum, (PHY_BASE+(0x34<<2)), 0x00001c16);
    for (i=0; i<8; i++) {
       REGW(devNum, (PHY_BASE+(0x20<<2)), 0x00010000);
    }
    pLibDev->aRevID = (REGR(devNum, PHY_BASE + (256<<2)) >> 24) & 0xff;
    pLibDev->aRevID = reverseBits(pLibDev->aRevID, 8);

    if( (0x0012 == pLibDev->hwDevID) || (0xff12 == pLibDev->hwDevID)
        ||((pLibDev->hwDevID == 0x0013) && ((pLibDev->aRevID & 0xF) != 0x5)) || // exclude griffin
#ifndef _IQV
        (pLibDev->hwDevID == 0xff13) && (pLibDev->aRevID & 0xf < 3) ) {
#else
       (pLibDev->hwDevID == 0xff13) && ( (pLibDev->aRevID & 0xf) < 3)) {
#endif // _IQV	
        // Only the 2 mac of freedom has A/B
#if defined(FREEDOM_AP)&&!defined(COBRA_AP)
        if( pLibDev->devMap.DEV_REG_ADDRESS == AR531X_WLAN1 )
#endif
        {
            //read the beanie rev ID
            REGW(devNum, 0x9800, 0x00004007);
            mSleep(2);
            REGW(devNum, (PHY_BASE+(0x34<<2)), 0x00001c16);
            for( i=0; i<8; i++ ) {
                REGW(devNum, (PHY_BASE+(0x20<<2)), 0x00010000);
            }
            pLibDev->aBeanieRevID = (REGR(devNum, PHY_BASE + (256<<2)) >> 24) & 0xff;
            pLibDev->aBeanieRevID = reverseBits(pLibDev->aBeanieRevID, 8);
            REGW(devNum, 0x9800, 0x07);
        }
    }

    /* identify the chip */
    for (i = 0; i < numDeviceIDs; i++)  {
        /* for single chips (like merlin) use only the mac rev */
        if( (pLibDev->hwDevID & 0xff) == SW_DEVICE_ID_MERLIN ||
            (pLibDev->hwDevID & 0xff) == SW_DEVICE_ID_MERLIN_PCIE )
        {
            if( ((pLibDev->hwDevID == ar5kInitData[i].deviceID) ||
                 (ar5kInitData[i].deviceID == DONT_MATCH) )     &&
                  macRevMatch(i, pLibDev->macRev) )
            {
                 break;
            }
        }
        else
        {
            if( ((pLibDev->hwDevID == ar5kInitData[i].deviceID) ||
                 (ar5kInitData[i].deviceID == DONT_MATCH) ) &&
                analogRevMatch(i, pLibDev->aRevID) &&
                macRevMatch(i, pLibDev->macRev) )
            {
                break;
            }
        }
    }

    if(i == numDeviceIDs) {
        mError(devNum, EIO, "Device Number %d:unable to find device init details for deviceID 0x%04lx, analog rev 0x%02lx, mac rev 0x%04lx, bb rev 0x%04lx\n", devNum,
        pLibDev->hwDevID, pLibDev->aRevID, pLibDev->macRev, pLibDev->bbRevID);
        printf("Device Number %d:unable to find device init details for deviceID 0x%04lx, analog rev 0x%02lx, mac rev 0x%04lx, bb rev 0x%04lx\n", devNum,
        pLibDev->hwDevID, pLibDev->aRevID, pLibDev->macRev, pLibDev->bbRevID);
        return -1;
    }

    //read the subsystemID, this has been moved from above, so can read
    //this from eeprom
    //force the eeprom size incase blank card
    if(ar5kInitData[i].swDeviceID >= 0x0012 && !isOwl(ar5kInitData[i].swDeviceID)) {
        REGW(devNum, 0x6010, REGR(devNum, 0x6010) | 0x3);
    }
    if (pLibDev->devMap.remoteLib) {
        pLibDev->subSystemID =  pLibDev->devMap.r_eepromRead(devNum, 0x7);
    }
    else {
        // pLibDev->subSystemID = ar5kInitData[i].pMacAPI->eepromRead(devNum, 0x7);
    }

#ifndef NO_LIB_PRINT
    if (!pLibDev->regFileRead && !printOnce ) {
        printf(DEVLIB_VER1);
        printf("\n\n                     ===============================================\n");
        printf("                     |               ");
        switch(pLibDev->subSystemID) {
            case 0x1021  :  printf("AR5001a_cb                    "); break;
            case 0x1022  :  printf("AR5001x_cb                    "); break;
            case 0x2022  :  printf("AR5001x_mb                    "); break;
            case 0x2023  :  printf("AR5001a_mb                    "); break;
            case 0xa021  :  printf("AR5001ap_ap                   "); break;

            case 0x1025  :  printf("AR5001g_cb21g                 "); break;
            case 0x1026  :  printf("AR5001x2_cb22ag               "); break;
            case 0x2025  :  printf("AR5001g_mb21g                 "); break;
            case 0x2026  :  printf("AR5001x2_mb22ag               "); break;

            case 0x2030 :  printf("AR5002g_mb31g (de-stuffed)    "); break;
            case 0x2031 :  printf("AR5002x_mb32ag                "); break;
            case 0x2027 :  printf("AR5001g_mb22g (de-stuffed)    "); break;
            case 0x2029 :  printf("AR5001x_mb22ag (single-sided) "); break;
            case 0x2024 :  printf("AR5001x_mb23j                 "); break;
            case 0x1030 :  printf("AR5002g_cb31g (de-stuffed)    "); break;
            case 0x1031 :  printf("AR5002x_cb32ag                "); break;
            case 0x1027 :  printf("AR5001g_cb22g (de-stuffed)    "); break;
            case 0x1029 :  printf("AR5001x_cb22ag (single-sided) "); break;
            case 0xa032 :  printf("AR5002a_ap30                  "); break;
            case 0xa034 :  printf("AR5002a_ap30  (040)           "); break;
            case 0xa041 :  printf("AR5004ap_ap41g (g)            "); break;
            case 0xa043 :  printf("AR5004ap_ap43g (g)            "); break;
            case 0xa048 :  printf("AR5004ap_ap48ag (a/g)         "); break;
            case 0x1041 :  printf("AR5004g_cb41g (g)             "); break;
            case 0x1042 :  printf("AR5004x_cb42ag (a/g)          "); break;
            case 0x1043 :  printf("AR5004g_cb43g (g)             "); break;
            case 0x2041 :  printf("AR5004g_mb41g (g)             "); break;
            case 0x2042 :  printf("AR5004x_mb42ag (a/g)          "); break;
            case 0x2043 :  printf("AR5004g_mb43g (g)             "); break;
            case 0x2044 :  printf("AR5004x_mb44ag (a/g)          "); break;
            case 0x1051 :  printf("AR5005gs_cb51g (g super)      "); break;
            case 0x1052 :  printf("AR5005g_cb51g (g)             "); break;
            case 0x2051 :  printf("AR5005gs_mb51g (g super)      "); break;
            case 0x2052 :  printf("AR5005g_mb51g (g)             "); break;
            case 0x1062 :  printf("AR5006xs_cb62ag (a/g super)   "); break;
            case 0x1063 :  printf("AR5006x_cb62ag (a/g)          "); break;
            case 0x2062 :  printf("AR5006xs_mb62ag (a/g super)   "); break;
            case 0x2063 :  printf("AR5006x_mb62g (a/g)           "); break;
            case 0x1071 :  printf("AR5008ng_cb71 (2ghz 11n)      "); break;
			case 0x1072 :  printf("AR5008nx_cb72 (5/2ghz 11n)    "); break;
			case 0xa071 :  printf("AR5008ng_ap71 (2ghz 11n)      "); break;
			case 0xa072 :  printf("AR5008nx_ap72 (5/2ghz 11n)    "); break;
			case 0x2071 :  printf("AR5008ng_mb71 (2ghz 11n)      "); break;
			case 0x2072 :  printf("AR5008nx_MB72 (5/2ghz 11n)    "); break;
			case 0x2081 :  printf("AR5008nx_MB81 (2ghz 11n)    "); break;
			case 0x2082 :  printf("AR5008nx_MB82 (5/2ghz 11n)    "); break;
			case 0x3072 :  printf("AR5008nx_xb72 (5/2ghz 11n)    "); break;
            case 0x3092 :  printf("ar9280nx_hb92 (5/2ghz 11n)    "); break;
            case 0x3091 :  printf("ar9280ng_hb91 (2ghz 11n)      "); break;
            case 0x2091 :  printf("ar9280nx_mb91 (2ghz 11n)    "); break;
            case 0x2092 :  printf("ar9280nx_mb92 (5/2ghz 11n)    "); break;
            case 0x3094 :  printf("ar9280nx_xb92 (5/2ghz 11n)    "); break;
            case UB51_SUBSYSTEM_ID:  printf("AR5005ug_UB51 (g)   "); break;
            case UB52_SUBSYSTEM_ID:  printf("AR5005ux_UB52 (a/g) "); break;
            case AP51_FULL_SUBSYSTEM_ID:  printf("AR5006apgs_AP51 (g) "); break;
            case AP51_LITE_SUBSYSTEM_ID:  printf("AR5006apg_AP51 (g)  "); break;

            default: printf("UNKNOWN TYPE"); break;
        }
        printf("|\n");
        printf("                     ===============================================\n\n");
        printf("Devices detected:\n");
        printf("   PCI deviceID     : 0x%04lx\t", pLibDev->hwDevID);
        printf("Sub systemID     : 0x%04lx\n", pLibDev->subSystemID);
        printf("   MAC revisionID   : 0x%lx\t", pLibDev->macRev);
        printf("BB  revisionID   : 0x%02lx\n", pLibDev->bbRevID & 0xff);
        printf("   RF  productID    : 0x%01lx\t", pLibDev->aRevID & 0xf);
        printf("RF  revisionID   : 0x%01lx\n", (pLibDev->aRevID >> 4) & 0xf);
        if((0x0012 == ar5kInitData[i].swDeviceID) || (ar5kInitData[i].swDeviceID == 0x0013) ) {
            printf("   Beanie productID : 0x%01lx\t", pLibDev->aBeanieRevID & 0xf);
            printf("Beanie revisionID: 0x%01lx\n\n", (pLibDev->aBeanieRevID >> 4) & 0xf);
        } else {
            printf("\n");
        }
        printOnce = 1;
    }
#endif //NO_LIB_PRINT

    return i;
}

/**
 * - Function Name: loadCfgData
 * - Description  : loads the ini and mod files into local arrays
 * - Arguments
 *     -      
 * - Returns      : 
 ******************************************************************************/
A_BOOL loadCfgData(A_UINT32 devNum, A_UINT32 freq) {

LIB_DEV_INFO *pLibDev = gLibInfo.pLibDevArray[devNum];
A_UINT32 devTableIndex;
A_BOOL   earHere = FALSE;
A_UINT32 jj;

    /* check if loading has already taken place */
    if (!pLibDev->regFileRead) {
        devTableIndex = findDevTableEntry(devNum);
        //devTableIndex = 0;

        if (devTableIndex == -1) {
            mError(devNum, EIO, "Invalid Device table number %d:\n", devTableIndex);
            return 0;
        }

        pLibDev->ar5kInitIndex = devTableIndex;
        //try to read the eeprom to see if there is an ear
        if (pLibDev->eePromLoad) { /* go through here when loading the eeprom from the main menu */
            if(setupEEPromMap(devNum)) {
                if((((pLibDev->eepData.version >> 12) & 0xF) >= 4) &&
                   (((pLibDev->eepData.version >> 12) & 0xF) <= 5))
                {
                    earHere = ar5212IsEarEngaged(devNum, pLibDev->pEarHead, freq);
                }
                //If we are loading the EAR, then move onto the next structure
                //so we point to the "frozen" config file contents, then load ear on that
                if(earHere && (pLibDev->libCfgParams.loadEar)) {
                    //  printf("earHere\n");
                    if(((ar5kInitData[devTableIndex].swDeviceID & 0xff) == 0x0016) ||
                       ((ar5kInitData[devTableIndex].swDeviceID & 0xff) == 0x0017) ||
                       ((ar5kInitData[devTableIndex].swDeviceID & 0xff) == 0x0018))
                    {
                        //      printf("ar5kInitData[devTableIndex].swDeviceID & 0xff) == 0x0016) || ((ar5kInitData[devTableIndex].swDeviceID & 0xff)\n");
                        devTableIndex++;
                    }
                }
            }
        }

        //set devTableIndex again incase it moved
        pLibDev->ar5kInitIndex = devTableIndex;
        pLibDev->sizeRegArray = ar5kInitData[devTableIndex].sizeArray;

        if(pLibDev->regArray) {
            free(pLibDev->regArray);
            pLibDev->regArray = NULL;
        }

        /* allocate memory for the ini register info */
        pLibDev->regArray = (ATHEROS_REG_FILE *)malloc(sizeof(ATHEROS_REG_FILE) *  pLibDev->sizeRegArray);
        if (!pLibDev->regArray) {
            mError(devNum, ENOMEM, "Device Number %d:resetDevice Failed to allocate memory for regArray \n", devNum);
            return 0;
        }

        /* copy from ini reg file into the devlib */
        memcpy(pLibDev->regArray, ar5kInitData[devTableIndex].pInitRegs, (sizeof(ATHEROS_REG_FILE) *  pLibDev->sizeRegArray));

        pLibDev->sizeModeArray = ar5kInitData[devTableIndex].sizeModeArray;

        if (pLibDev->pModeArray) {
            free(pLibDev->pModeArray);
            pLibDev->pModeArray = NULL;
        }

        /* allocate memory for the modal register info only */
        pLibDev->pModeArray = (MODE_INFO *)malloc(sizeof(MODE_INFO)*pLibDev->sizeModeArray);
        if (!pLibDev->pModeArray) {
            mError(devNum, ENOMEM, "Device Number %d:resetDevice Failed to allocate memory for modeArray \n", devNum);
            return 0;
        }

        /* copy from mod reg file into the devlib */
        memcpy(pLibDev->pModeArray, ar5kInitData[devTableIndex].pModeValues,sizeof(MODE_INFO)*pLibDev->sizeModeArray);

        pLibDev->swDevID = ar5kInitData[devTableIndex].swDeviceID;

              //printf(">>>>>>pLibDev->regFilename = %s\n",pLibDev->regFilename);
        /* see if there is a need to parse the cfg file at runtime, this will
         * happen if (pLibDev->regFilename != NULL) 
         */
        if( !parseAtherosRegFile(pLibDev, pLibDev->regFilename) ) {
            printf(" After parseAtherosRegFile\n");
            if( pLibDev->mdkErrno ) {
                printf("SNOOP: ERROR : %s\n", pLibDev->mdkErrStr);
                pLibDev->mdkErrno = 0;
            }
            mError(devNum, EIO, "Device Number %d:resetDevice: Unable to open atheros register file : %s\n", devNum, pLibDev->regFilename);
            return(0);
        }

        /* create and array from the ini file that is actually writeable: 
         * address to write to, value to write
         */
        if( !createPciRegWrites(devNum) ) {
            //  printf("Not of createPciRegWrites \n");
            mError(devNum, EIO, "Device Number %d:resetDevice: Unable to convert atheros register file to pci write values\n", devNum);
            return(0);
        }
        pLibDev->regFileRead = 1;

        //initialize the API
        ar5kInitData[pLibDev->ar5kInitIndex].pMacAPI->macAPIInit(devNum);

        if( pLibDev->devMap.remoteLib ) {
        }
        else {
            //zero out the key cache
            for( jj = 0x8800; jj < 0x97ff; jj+=4 ) {
                REGW(devNum, jj, 0);
            }
        }
    } /* if (!pLibDev->regFileRead) */
//printf("Load cfgdata end\n");
    return 1;
}

//++JC++
void sleep_cal( clock_t wait ) {
   clock_t goal;
   goal = wait + clock();
   while( goal > clock() );
}

/**************************************************************************
* griffin_cl_cal - Do carrier leak cal for griffin
***************************************************************************/
void griffin_cl_cal(A_UINT32 devNum)
{

   A_INT32 add_I_first[9], add_Q_first[9], add_I_second[9], add_Q_second[9];
   A_INT32 add_I_avg[9], add_Q_avg[9];
   A_INT32 add_I_diff, add_Q_diff, kk;
   A_UINT32 offset, data, cl_cal_done, i;
   A_UINT32 cl_cal_reg[9] = {0xa35c,0xa360,0xa364,0xa368,0xa36c,0xa370,0xa374,0xa378,0xa37c};


   data = REGR(devNum,0xa358);
   data = data | 0x3;
   REGW(devNum,0xa358, data);

   // Get first set of I and Q data
   data = REGR(devNum,0x9860);
   data = data | 0x1;
   REGW(devNum,0x9860, data);

   sleep_cal (10);

   // Check for cal done
   for (i = 0; i < 1000; i++) {
      if ((REGR(devNum, PHY_AGC_CONTROL) & (enableCal)) == 0 ) {
         break;
      }
      mSleep(1);
   }
   if(i >= 1000) {
      printf("griffin_cl_cal : Device Number %d didn't complete cal but keep going anyway\n", devNum);
   }

    for (kk=0; kk<9; kk++) {
    offset = cl_cal_reg[kk];
        data = REGR(devNum, offset);

        add_I_first[kk] = (data >> 16) & 0x7ff;
        add_Q_first[kk] = (data >>  5) & 0x7ff;

        if (((add_I_first[kk]>>10) & 0x1) == 1) {
            add_I_first[kk] = add_I_first[kk] | 0xfffff800;
        }
        if (((add_Q_first[kk]>>10) & 0x1) == 1) {
            add_Q_first[kk] = add_Q_first[kk] | 0xfffff800;
        }

        //printf ("Debug: 0x%04x: I_first:%d Q_first:%d\n", offset, add_I_first[kk], add_Q_first[kk]);
    }
    // Finished getting the first set of data

   // Get second set of data - in the while loop till the second set is obtained
   cl_cal_done = 0;
   while (cl_cal_done == 0) {

      data = REGR(devNum,0x9860);
      data = data | 0x1;
      REGW(devNum,0x9860, data);

      // Check for cal done
      for (i = 0; i < 1000; i++) {
         if ((REGR(devNum, PHY_AGC_CONTROL) & (enableCal)) == 0 ) {
            break;
         }
         mSleep(1);
      }
      if(i >= 1000) {
         printf("Device Number %d:Didn't complete cal but keep going anyway\n", devNum);
      }

      offset = cl_cal_reg[0];
      data = REGR(devNum, offset);
      add_I_second[0] = (data >> 16) & 0x7ff;
      add_Q_second[0] = (data >>  5) & 0x7ff;

      if (((add_I_second[0]>>10) & 0x1) == 1) {
          add_I_second[0] = add_I_second[0] | 0xfffff800;
      }
      if (((add_Q_second[0]>>10) & 0x1) == 1) {
          add_Q_second[0] = add_Q_second[0] | 0xfffff800;
      }

      // Get the difference between the first set and the current set
      add_I_diff = abs(add_I_first[0] - add_I_second[0]);
      add_Q_diff = abs(add_Q_first[0] - add_Q_second[0]);

      // Recognize the current set as the second set if diff > 8
      if ((add_I_diff > 8) || (add_Q_diff > 8)) {
          for (kk=0; kk<9; kk++) {             // Get the full second set
          offset = cl_cal_reg[kk];
              data = REGR(devNum, offset);

              add_I_second[kk] = (data >> 16) & 0x7ff;
              add_Q_second[kk] = (data >>  5) & 0x7ff;

              if (((add_I_second[kk]>>10) & 0x1) == 1) {
                  add_I_second[kk] = add_I_second[kk] | 0xfffff800;
              }
              if (((add_Q_second[kk]>>10) & 0x1) == 1) {
                  add_Q_second[kk] = add_Q_second[kk] | 0xfffff800;
              }
          }
          cl_cal_done = 1;
      }
   }

   // Form the average of the two values
   for (kk=0; kk<9; kk++) {
      add_I_avg[kk] = (add_I_first[kk] + add_I_second[kk])/2;
      add_Q_avg[kk] = (add_Q_first[kk] + add_Q_second[kk])/2;

      add_I_avg[kk] = add_I_avg[kk] & 0x7FF;  // To take care of signed values
      add_Q_avg[kk] = add_Q_avg[kk] & 0x7FF;  // To take care of signed values
   }

   // Write back the average values into the registers
   for (kk=0; kk<9; kk++) {
      offset = cl_cal_reg[kk];
      data = REGR(devNum, offset);
      data = data & 0xF800001F;               // Clear the bits where the avg is to be written
      data = data | (add_I_avg[kk] << 16) | (add_Q_avg[kk] << 5);     // Or in the avg data
      REGW(devNum,cl_cal_reg[kk], data);
   }

   // Enable Carrier Leak Correction
   data = REGR(devNum,0xa358);
   data = data | 0x40000000;
   REGW(devNum,0xa358, data);
   //printf("Done Carrier Leak cal workaround\n\n");

}
//++JC++


#define H_GPIO_OE_BITS   (0x404c) /* gpio output enable reg */
#define H_GPIO_IN_OUT    (0x4048) /* gpio output reg */
#define H_GP_OUTPT_MUX1  (0x4060) /* gpio output mux reg for gpios 0,1,2,3,4,5 */
#define H_GP_OUTPT_MUX2  (0x4064) /* gpio output mux reg for gpios 6,7,8,9 */
#define H_GP_INPT_EN_VAL (0x4054) /* gpio input enable & value register */
#define JTAG_DIS_SHIFT   (17)     /* jtag disable bit shift */
/**
 * - Function Name: ar5416SetGpio
 * - Description  : sets a GPIO to the value set in the GPIO output register 
 *                  (0x4048)
 * - Arguments
 *     - devNum: device number
 *     - uiGpio: gpio number
 *     - uiVal : gpio value: 0 or 1
 * - Returns   : none
 *******************************************************************************/
void ar5416SetGpio( A_UINT32 devNum, A_UINT32 uiGpio, A_UINT32 uiVal ) {
    A_UINT32 regval;

    /* for gpio4 or lower, the "jtag_disable" bit needs to be written in 
     * order for the pins to operate as gpio's 
     */
    if(uiGpio < 5) {
        regval = REGR(devNum,H_GP_INPT_EN_VAL); /* get the current value */
        regval |= 0x01<<JTAG_DIS_SHIFT; /* set the "jtag_disable bit to "1" */
        REGW(devNum, H_GP_INPT_EN_VAL, regval);
    }

    /* map the GPIO ouptut enable register: always drive output */
    regval = REGR(devNum,H_GPIO_OE_BITS); /* get the current value */
    regval &= ~(0x3<<(uiGpio*2));         /* clear the appropriate bits */
    regval |= 0x3<<(uiGpio*2);            /* enable the appropriate gpio */
    REGW(devNum, H_GPIO_OE_BITS, regval); /* write the new value */

    /* select fixed value: set output mux so that gpio output is set by gpio output reg */
    if(uiGpio < 6 ) {
        regval = REGR(devNum,H_GP_OUTPT_MUX1); /* get the current value */
        regval &= ~(0x1f<<(uiGpio*5));         /* clear the appropriate bits */
        REGW(devNum, H_GP_OUTPT_MUX1, regval); /* write the new value */
    }
    else {
        regval = REGR(devNum,H_GP_OUTPT_MUX2); /* get the current value */
        regval &= ~(0x1f<<((uiGpio-5)*5));     /* clear the appropriate bits */
        REGW(devNum, H_GP_OUTPT_MUX2, regval); /* write the new value */
    }

    /* do a read/modify/write on the gpio output reg */
    regval = REGR(devNum,H_GPIO_IN_OUT); /* get the current value */
    regval &= ~(1<<uiGpio) ;              /* clear the appropriate bit */
    regval |= (uiVal&1)<<uiGpio;
    REGW(devNum, H_GPIO_IN_OUT, regval);
}
/* ar5416SetGpio */

/** 
 * - Function Name: ar5416ReadGpio
 * - Description  : reads the current value of a gpio
 * - Arguments
 *     - devnum: device number
 *     - uiGpio: gpio number to read
 * - Returns      : 1 if gpio is set, else 0                                 
 *******************************************************************************/
A_UINT32 ar5416ReadGpio( A_UINT32 devNum, A_UINT32 uiGpio ) {
    A_UINT32 regval = 0;
    regval = REGR(devNum,H_GPIO_IN_OUT); /* get the current value */
    regval &= (1<<(10+uiGpio)) ;         /* pick the appropriate bit */
    regval = (regval>0)? 1:0;
    return (regval);
}
/* ar5416ReadGpio */

/** 
 * - Function Name: ReadNF
 * - Description  : reads/displays the noise floor after calibration 
 * - Arguments
 *     - 
 * - Returns      : 1: noise floor > -100 (bad), else 0 (normal)
 *******************************************************************************/
A_UINT32 ReadNF( A_UINT32 devNum ) {
    LIB_DEV_INFO *pLibDev = gLibInfo.pLibDevArray[devNum];
    A_UINT32 temp1, temp2, temp3, temp4;
    A_UINT32 i;
    A_UINT32 numChains = 0;
    A_UINT32 retVal    = 0;
	A_INT32	 nf_ctl, nf_ext, nf_threshold;

	nf_ctl = 0;
	nf_ext = 0;
	nf_threshold = -100;

    /* count the # of rx chains */
    if((pLibDev->libCfgParams.rx_chain_mask & 0x07) == 0x07) {
        numChains = 3;
    }
    else if ((pLibDev->libCfgParams.rx_chain_mask & 0x03) == 0x03) {
        numChains = 2;
    }
    else if(((pLibDev->libCfgParams.rx_chain_mask & 0x01) == 0x01) ||
			((pLibDev->libCfgParams.rx_chain_mask & 0x02) == 0x02)) {
        numChains = 1;
    }
//    printf("numChains: %d, 0x%x\n", numChains, pLibDev->libCfgParams.rx_chain_mask);

    /* read the NF cal results */
    for (i = 0; i < numChains; i++) {
        temp1 = REGR(devNum, (0x9800 + (i * 0x1000) + (25<<2)));
        temp2 = (temp1 >> 19) & 0x1ff;

        if(isMerlin(pLibDev->swDevID)) { // bit location is changed in Merlin
            temp2 = (temp1 >> 20) & 0x1ff;
        }

        if ((temp2 & 0x100) != 0x0) {
            nf_ctl = 0 - ((temp2 ^ 0x1ff) + 1);
        }
        temp4 = 0;
        if (isOwl(pLibDev->swDevID)) {
            temp3 = REGR(devNum, (0x9800 + (i * 0x1000) + (111<<2)));
            temp4 = (temp3 >> 23) & 0x1ff;

            if(isMerlin(pLibDev->swDevID)) { // bit location is changed in Merlin
               temp4 = (temp3 >> 16) & 0x1ff;
            }

            if ((temp4 & 0x100) != 0x0) {
                nf_ext = 0 - ((temp4 ^ 0x1ff) + 1);
            }
        }
//        printf("NF for Chain%d = ctl %d, ext %d \n", i, nf_ctl, nf_ext);
        if(((pLibDev->libCfgParams.ht40_enable == 0) && (nf_ctl > nf_threshold)) ||
			((pLibDev->libCfgParams.ht40_enable == 1) && ((nf_ctl > nf_threshold) || (nf_ext > nf_threshold)))) {
            retVal = 1;
        }
    }
    return (retVal);
}
/* ReadNF */

/** 
 * - Function Name: calculateDeltaCoeffs
 * - Description  : Calculates the delta slope coefficients. These need to be 
 *                  recalculated when the frequency is changed or when going
 *                  from static 20MHz configuration (HT20 or legacy) to dynamic
 *                  HT20/40 mode. Changing from legacy to HT20 does not require
 *                  an update as lon as teh center frequency is unchanged. No
 *                  need to update delta slope coefficients for tx power level
 *                  changes, or for rate change, or for going from tx to rx or
 *                  rx to tx.
 * - Arguments
 *     - 
 * - Returns      :                                                                
 *******************************************************************************/
void calculateDeltaCoeffs( A_UINT32 devNum, double fclk, A_UINT32 freq) {
    LIB_DEV_INFO *pLibDev = gLibInfo.pLibDevArray[devNum];
    double coeff, coeff_shgi;
    A_UINT32 coeff_exp_shgi, coeff_man_shgi;
    A_UINT32 coeffExp,coeffMan;
    A_UINT32 deltaSlopeCoeffMan, deltaSlopeCoeffExp;

    coeff = (2.5 * fclk) / ((double)freq);
#ifndef _IQV
    coeffExp = 14 -  (int)(floor(log10(coeff)/log10(2)));
    coeffMan = (int)(floor((coeff*(pow(2,coeffExp))) + 0.5));
#else       // to avoid compiler error at LP
    coeffExp = 14 -  (int)(floor(log10((double)coeff)/log10(2.0)));
    coeffMan = (int)(floor((coeff*(pow(2.0,(double)coeffExp))) + 0.5));
#endif
    deltaSlopeCoeffExp = coeffExp - 16;
    deltaSlopeCoeffMan = coeffMan;
    REGW(devNum, 0x9814, (REGR(devNum,0x9814) & 0x00001fff) |
                         (deltaSlopeCoeffExp << 13) |
                         (deltaSlopeCoeffMan << 17));
//            printf("Delta Coeff Exp = %d   Delta Coeff Man = %d\n", deltaSlopeCoeffExp, deltaSlopeCoeffMan);
    if( isOwl(pLibDev->swDevID) ) {

        coeff_shgi = (0.9 * 2.5 * fclk) / freq;
#ifndef _IQV
        coeff_exp_shgi  = (A_UINT32) (14 - floor(log10(coeff_shgi)/log10(2)));
        coeff_man_shgi = (A_UINT32) floor((coeff_shgi * pow(2, coeff_exp_shgi)) + 0.5);
#else
        coeff_exp_shgi  = (A_UINT32) (14 - floor(log10((double)coeff_shgi)/log10(2.0)));
        coeff_man_shgi = (A_UINT32) floor((coeff_shgi * pow(2.0, (double)coeff_exp_shgi)) + 0.5);
#endif
        coeff_exp_shgi -= 16;
        //printf("Delta Coeff Exp Short GI = %d   Delta Coeff Man Short GI = %d\n", coeff_exp_shgi, coeff_man_shgi);
        REGW(devNum, 0x99D0, (coeff_man_shgi << 4) | coeff_exp_shgi); /* reg# 116 */
    }
}
/* calculateDeltaCoeffs */

/** 
 * - Function Name: writeTestAddr
 * - Description  :
 * - Arguments
 *     - 
 * - Returns      :                                                                
 *******************************************************************************/
void writeTestAddr( A_UINT32 devNum, A_UCHAR *mac, A_UCHAR *bss) {
    A_UINT32 temp1, temp2;
    LIB_DEV_INFO *pLibDev = gLibInfo.pLibDevArray[devNum];

    memcpy(pLibDev->macAddr.octets, mac, WLAN_MAC_ADDR_SIZE);
#ifndef ARCH_BIG_ENDIAN
    REGW(devNum, F2_STA_ID0, pLibDev->macAddr.st.word);
    temp1 = REGR(devNum, F2_STA_ID1);
    temp2 = (temp1 & 0xffff0000) | F2_STA_ID1_AD_HOC | pLibDev->macAddr.st.half | F2_STA_ID1_DESC_ANT;
#ifdef _IQV
    // set sequence number fixed, disable the sequence number auto-increase function
#define F2_STA_ID1_FIXED_SEQ  0x20000000
    temp2 = temp2 | F2_STA_ID1_FIXED_SEQ;
#endif	//  _IQV 		
    REGW(devNum, F2_STA_ID1, temp2);
#else
    {
        A_UINT32 addr;

        addr = swap_l(pLibDev->macAddr.st.word);
        REGW(devNum, F2_STA_ID0, addr);
        addr = (A_UINT32)swap_s(pLibDev->macAddr.st.half);
        REGW(devNum, F2_STA_ID1, (REGR(devNum, F2_STA_ID1) & 0xffff0000) | F2_STA_ID1_AD_HOC |
             addr | F2_STA_ID1_DESC_ANT);
    }
#endif

    // then our BSSID
    memcpy(pLibDev->bssAddr.octets, bss, WLAN_MAC_ADDR_SIZE);
#ifndef ARCH_BIG_ENDIAN
    REGW(devNum, F2_BSS_ID0, pLibDev->bssAddr.st.word);
    REGW(devNum, F2_BSS_ID1, pLibDev->bssAddr.st.half);
#else
    {
        A_UINT32 addr;

        addr = swap_l(pLibDev->bssAddr.st.word);
        REGW(devNum, F2_BSS_ID0, addr);
        addr = (A_UINT32)swap_s(pLibDev->bssAddr.st.half);
        REGW(devNum, F2_BSS_ID1, addr);
    }
#endif

    REGW(devNum, F2_BCR, F2_BCR_STAMODE);
    REGR(devNum, F2_BSR); // cleared on read
}
/* writeTestAddr */

/** 
 * - Function Name: EnableRxFilterCal
 * - Description  : Enables the Rx filter calibration. The filter cal does not 
 *                  vary with channel frequency or rate.  It doesn't vary when
 *                  going from HT20 to HT40 or viceversa.
 * - Arguments
 *     - 
 * - Returns      :                                                                
 *******************************************************************************/
void EnableRxFilterCal( A_UINT32 devNum ) {

    /* first, clear the bb_off_pwdAdc bit */
    REGW(devNum, PHY_ADC_PARALLEL_CNTL,
         REGR(devNum, PHY_ADC_PARALLEL_CNTL) & PWD_ADC_CL_MASK);

    /* then enable the calibration */
    REGW(devNum, PHY_AGC_CONTROL,
         REGR(devNum, PHY_AGC_CONTROL) | (0x1<<16));
}
/* EnableRxFilterCal */

/** 
 * - Function Name: DisableRxFilterCal
 * - Description  :
 * - Arguments
 *     - 
 * - Returns      :                                                                
 *******************************************************************************/
void DisableRxFilterCal( A_UINT32 devNum ) {

    /* first, set the bb_off_pwdAdc bit */
    REGW(devNum, PHY_ADC_PARALLEL_CNTL,
         REGR(devNum, PHY_ADC_PARALLEL_CNTL) | ~PWD_ADC_CL_MASK);

    /* then disable the calibration */
    REGW(devNum, PHY_AGC_CONTROL,
         REGR(devNum, PHY_AGC_CONTROL) & ~(0x1<<16));
}
/* DisableRxFilterCal */

/** 
 * - Function Name: RunDcOffsetCal
 * - Description  : Runs the DC offset calibration. This calibration should be 
 *                  performed when the frequency changes.
 * - Arguments
 *     - 
 * - Returns      :                                                                
 *******************************************************************************/
void RunDcOffsetCal( A_UINT32 devNum ) {
    LIB_DEV_INFO *pLibDev = gLibInfo.pLibDevArray[devNum];
    A_UINT32  timeout = 8;
    A_UINT32 i;

    REGW(devNum, PHY_AGC_CONTROL,
         REGR(devNum, PHY_AGC_CONTROL) | DO_OFSET_CAL);

    for( i = 0; i < timeout; i++ ) {
        if( (REGR(devNum, PHY_AGC_CONTROL) & DO_OFSET_CAL) == 0 ) {
            break;
        }
        mSleep(1);
    }

    if( i >= timeout ) {
#ifndef CUSTOMER_REL
        printf("RunDcOffsetCal : Device Number %d:Didn't complete DC offset cal after timeout(%d) on channel %d but keep going anyway\n", devNum, timeout, pLibDev->freqForResetDevice);
#endif
    }
}
/* RunDcOffsetCal */

/** 
 * - Function Name: RunNFCal
 * - Description  : Performs the noise floor calibration. To be done when the 
 *                  frequency is changed or when going from a 20 MHz mode (HT20
 *                  or legacy) to a dynamic HT20/40 mode and viceversa.
 * - Arguments
 *     - 
 * - Returns      : 
 *******************************************************************************/
void RunNFCal( A_UINT32 devNum ) {
    LIB_DEV_INFO *pLibDev = gLibInfo.pLibDevArray[devNum];
    A_UINT32  timeout = 8;
    A_UINT32 i;

    REGW(devNum, PHY_AGC_CONTROL,
         REGR(devNum, PHY_AGC_CONTROL) | DO_NF_CAL);
    // noise cal takes a long time to complete in simulation
    // need not wait for the noise cal to complete

    for( i = 0; i < timeout; i++ ) {
        if( (REGR(devNum, PHY_AGC_CONTROL) & DO_NF_CAL) == 0 ) {
            break;
        }
        mSleep(1);
    }

    if( i >= timeout ) {
#ifndef CUSTOMER_REL
        printf("resetDevice : Device Number %d:Didn't complete NF cal after timeout(%d) on channel %d but keep going anyway\n", devNum, timeout, pLibDev->freqForResetDevice);
#endif
    }
}
/* RunNFCal */

/** 
 * - Function Name: RunNFCalForced
 * - Description  :
 * - Arguments
 *     - 
 * - Returns      :                                                                
 *******************************************************************************/
void RunNFCalForced( A_UINT32 devNum, A_INT32 nf_force ) {
    LIB_DEV_INFO* pLibDev = gLibInfo.pLibDevArray[devNum];
    A_UINT32 temp1;
    A_UINT32 i;
    A_UINT32 timeout;

    for( i = 0; i < 2; i ++ ) {
        temp1 = REGR(devNum, 0x9864 + (i * 0x1000));
        temp1 &= 0xFFFFFE00;
        REGW(devNum, 0x9864 + (i * 0x1000), temp1 | ((nf_force << 1) & 0x1ff));
        temp1 = REGR(devNum, 0x99bc + (i * 0x1000));
        temp1 &= 0xFFFFFE00;
        REGW(devNum, 0x99bc + (i * 0x1000), temp1 | ((nf_force << 1) & 0x1ff));

    }

    temp1 = REGR(devNum, PHY_AGC_CONTROL);
    temp1 = temp1 & 0xFFFD7ffD;
    REGW(devNum, PHY_AGC_CONTROL, temp1 | (0x0 << 17) | (0x0 << 15) | DO_NF_CAL);
    // Do remote check
    if( pLibDev->devMap.remoteLib ) {
        timeout = 32000;
        i = pLibDev->devMap.r_calCheck(devNum, DO_NF_CAL, timeout);
    }
    else {
        timeout = 8;
        for( i = 0; i < timeout; i++ ) {
            if( (REGR(devNum, PHY_AGC_CONTROL) & DO_NF_CAL) == 0 ) {
                break;
            }
            mSleep(1);
        }
    }
    if( i >= timeout ) {
#ifndef CUSTOMER_REL 
        printf("RunNFCalForced : Device Number %d:Didn't complete NF cal after timeout(%d) on channel %d but keep going anyway\n", devNum, timeout, pLibDev->freqForResetDevice);
#endif
    }

    /* Restore maxCCAPower register parameter again so    */
    /* that we're not capped by the median we just        */
    /* loaded.  This will be initial (and max) value      */
    /* of next noise floor calibration the baseband does. */

    for( i = 0; i < 2; i ++ ) {
        temp1 = REGR(devNum, 0x9860 + (i * 0x1000));
        temp1 &= 0xFFFFFE00;
        REGW(devNum, 0x9864 + (i * 0x1000), temp1);
        temp1 = REGR(devNum, 0x99bc + (i * 0x1000));
        temp1 &= 0xFFFFFE00;
        REGW(devNum, 0x99bc + (i * 0x1000), temp1);

    }
    temp1 = REGR(devNum, PHY_AGC_CONTROL);
    temp1 = temp1 & 0xfffD7fff;
    REGW(devNum, PHY_AGC_CONTROL, temp1 | (0x0 << 17) | (0x1 << 15));
}
/* RunNFCalForced */

/** 
 * - Function Name: RunOtherCal
 * - Description  :
 * - Arguments
 *     - 
 * - Returns      :                                                                
 *******************************************************************************/
void RunOtherCal( A_UINT32 devNum ) {
    LIB_DEV_INFO* pLibDev = gLibInfo.pLibDevArray[devNum];
    A_UINT32 timeout;
    A_UINT32 i;

    REGW(devNum, PHY_AGC_CONTROL,
         REGR(devNum, PHY_AGC_CONTROL) | enableCal);
    // noise cal takes a long time to complete in simulation
    // need not wait for the noise cal to complete

    // Do remote check
    if( pLibDev->devMap.remoteLib ) {
        timeout = 32000;
        i = pLibDev->devMap.r_calCheck(devNum, enableCal, timeout);
    }
    else {
        timeout = 8;
        for( i = 0; i < timeout; i++ ) {
            if( (REGR(devNum, PHY_AGC_CONTROL) & (enableCal)) == 0 ) {
                break;
            }
            mSleep(1);
        }
    }
    if( i >= timeout ) {
#ifndef CUSTOMER_REL
        printf("RunOtherCal : Device Number %d:Didn't complete cal after timeout(%d) on channel %d but keep going anyway\n", devNum, timeout, pLibDev->freqForResetDevice);
#endif
    }
}
/* RunOtherCal */

/** 
 * - Function Name: RunCals
 * - Description  : Gathers all the calibrations in one place: Rx Filter cal, 
 *                  Noise Floor cal and DC Offset cal.
 * - Arguments
 *     - 
 * - Returns      :                                                                
 *******************************************************************************/
void RunCals( A_UINT32 devNum, A_UINT32 freq ) {
    LIB_DEV_INFO *pLibDev = gLibInfo.pLibDevArray[devNum];
    A_INT32 nf_force;
    A_UINT32 num_nf_cal;
    A_UINT32 kk;
    A_UINT32 freqChange;
    A_UINT32 mode_2040;
    static A_UINT32 prevFreq = 99;
    static A_UINT32 prevMode2040 = 99;

    /* determine if frequency or 20/40 mode has changed */
    freqChange = (freq != prevFreq) ? 1 : 0;
    mode_2040 = (pLibDev->libCfgParams.ht40_enable != prevMode2040) ? 1 : 0;

    if(isMerlin(pLibDev->swDevID)) 
    {
        if((freqChange || mode_2040) && 
           (enableCal == (DO_NF_CAL | DO_OFSET_CAL))) {

            /* at 5G run NF cal twice */
            num_nf_cal = (freq > 4000) ? 2 : 1;

            for( kk = 0; kk < num_nf_cal; kk++ ) {
                EnableRxFilterCal(devNum);  /* enable Rx filter calibration */
                RunDcOffsetCal(devNum);     /* perform DC offset calibration */
                DisableRxFilterCal(devNum); /* disable Rx filter calibration */
                RunNFCal(devNum);           /* perform noise floor calibration */

                /* read/display the NF after calibration */
                if( num_nf_cal == 2 ) {
                    if( !ReadNF(devNum) ) {
                        break;           
                    }
                    else {
                        /* if NF cal fails, force the noise floor */
                        /* Force NF to -110dB on 5G & 2G channels */
                        nf_force = (freq > 4000) ? -110 : -110; 

                        if( kk == (num_nf_cal - 1) ) {
                            RunNFCalForced(devNum, nf_force);
                        }
                    }
                }
            }
        }
        /* update flags */
        prevFreq = freq;
        prevMode2040 = pLibDev->libCfgParams.ht40_enable;
    }
    else { // Non-Merlin
        RunOtherCal(devNum);
    }
}
/* RunCals() */

/** 
 * - Function Name: FaskClkCheck
 * - Description  : Check the frequency of operation and returns the 
 *                  proper value for the variable fastClkApply.
 * - Arguments
 *     - freq: current frequency of operation
 * - Returns      : 1 if fast clock applies, otherwise 0        
 *******************************************************************************/
A_UINT32 FaskClkCheck( A_UINT32 devNum, A_UINT32 freq ) {
    LIB_DEV_INFO *pLibDev = gLibInfo.pLibDevArray[devNum];

    /* among 5G frequencies, check for 5MHz separation ones */
    if( freq > 4000 ) {
        /* check for fast clock condition */
        if( pLibDev->libCfgParams.fastClk5g ) {
            return (1);
        }
        else if( ((freq % 20) == 0) || ((freq % 10) == 0) ) {
            return (0);
        }
        else { /* apply to 5MHz channels only */
            return (1);
        }
    }
    else {
        return (0); /* a 2G frequency */
    }
    //printf("> fastClkApply: %d, freq: %d\n", fastClkApply, freq);
}
/* FaskClkCheck */

/** 
 * - Function Name: UpdateModeRegs
 * - Description  : Update the mode registers. Deal with the fast clock and the
 *                  Rx gain table options during the updating process.  this
 *                  process updates the default values of the reg offsets in
 *                  the pciValuesArray[]. The default reg offset values will be
 *                  updated unless the new values are not applicable.
 * - Arguments
 *     - 
 * - Returns      :                                                                
 *******************************************************************************/
void UpdateModeRegs( A_UINT32 devNum ) {
    LIB_DEV_INFO *pLibDev = gLibInfo.pLibDevArray[devNum];
    A_UINT32  i;
    A_UINT32  *pValue = NULL;

    /* Handle mode switching, get a ptr to the top of the modal array */
    switch (pLibDev->mode) {
    case MODE_11A:  //11a
        if (isOwl(pLibDev->swDevID) && pLibDev->libCfgParams.ht40_enable) 
        {
            pValue = &(pLibDev->pModeArray->value11aTurbo);  //11a ht40
        } else if(pLibDev->turbo != TURBO_ENABLE) {    //11a base
            pValue = &(pLibDev->pModeArray->value11a);
        } else {
            pValue = &(pLibDev->pModeArray->value11aTurbo);
        }
        break;

    case MODE_11G: //11g
        if (isOwl(pLibDev->swDevID) && pLibDev->libCfgParams.ht40_enable) {
            pValue = &(pLibDev->pModeArray->value11b); // ht40 params live in the 11b column
        } else if ((pLibDev->turbo != TURBO_ENABLE) || (ar5kInitData[pLibDev->ar5kInitIndex].cfgVersion < 3)) { //11g base
            pValue = &(pLibDev->pModeArray->value11g);
        } else {
            pValue = &(pLibDev->pModeArray->value11gTurbo);
        }
        break;

    case MODE_11B: //11b
        pValue = &(pLibDev->pModeArray->value11b);
        break;
    } //end switch

    /* update the registers */
    for(i = 0; i < pLibDev->sizeModeArray; i++) {
            if( isMerlin(pLibDev->swDevID) ) {
                /* do not update fast clk fields when not applicable */
                if( !fastClkApply ) {
                    if( strncmp(pLibDev->pModeArray[i].fieldName,"fas", 3) == 0 ) {
                        pValue = (A_UINT32 *)((A_CHAR *)pValue + sizeof(MODE_INFO));
                        continue; /* skip updating */
                    }
                }

                /* skip original Rx gain table values if not applicable */
                if( pLibDev->libCfgParams.rxGainType != RX_GAIN_ORIG ) {
                    if( strncmp(pLibDev->pModeArray[i].fieldName,"ori", 3) == 0 ) {
                        pValue = (A_UINT32 *)((A_CHAR *)pValue + sizeof(MODE_INFO));
                        continue; /* skip updating */
                    }
                } 

                /* skip 13dB backoff Rx gain table values if not applicable */
                if( pLibDev->libCfgParams.rxGainType != RX_GAIN_13dB_BACKOFF ) {
                    if( strncmp(pLibDev->pModeArray[i].fieldName,"bac", 3) == 0 ) {
                        pValue = (A_UINT32 *)((A_CHAR *)pValue + sizeof(MODE_INFO));
                        continue; /* skip updating */
                    }
                }
            }

            /* update the pci reg array, but do not write into chip */
            updateField(devNum, 
                        &pLibDev->regArray[pLibDev->pModeArray[i].indexToMainArray],
                        *pValue, 
                        0);
        //increment value pointer by size of an array element
        pValue = (A_UINT32 *)((A_CHAR *)pValue + sizeof(MODE_INFO));
    }
}
/* UpdateModeRegs */

/** 
 * - Function Name: FindFastClkField
 * - Description  : find the first fast_clock field in pLibDev->regArray[]
 * - Arguments
 *     - 
 * - Returns      :                                          
 *******************************************************************************/
void FindFastClkField( A_UINT32 devNum ) {
    LIB_DEV_INFO *pLibDev = gLibInfo.pLibDevArray[devNum];
    A_UINT32  i;

    for( i = 0; i < pLibDev->sizeRegArray; i++ ) {
        if( strncmp(pLibDev->regArray[i].fieldName, "fas", 3) == 0 ) {
            /* this is the first fast_clock field,
            * save the register offset
            */
            fastClkReg = pLibDev->regArray[i].regOffset;
/*                      printf(">>> field: %s, reg offset: 0x%08lx\n",*/
/*                             pLibDev->regArray[i].fieldName,        */
/*                             pLibDev->regArray[i].regOffset);       */
            fastClkFieldFound = 1;
            break;
        }
    }
}
/* FindFastClkField */

/** 
 * - Function Name: LoadRegs
 * - Description  : 
 * - Arguments
 *     - 
 * - Returns      :                                                                
 *******************************************************************************/
void LoadRegs( A_UINT32 devNum ) {
    LIB_DEV_INFO *pLibDev = gLibInfo.pLibDevArray[devNum];
    A_UINT32 cntr = 0;
    A_UINT32 i;

    for( i = 0; i < pLibDev->sizePciValuesArray; i++ ) {
        /* if dealing with a version 2 config file,
         * then all values are found in the base array
         */
        pciValues[i].offset = pLibDev->pciValuesArray[i].offset;

        if( (pLibDev->turbo == TURBO_ENABLE) && (ar5kInitData[pLibDev->ar5kInitIndex].cfgVersion < 2) ) 
        {
            if( pLibDev->devMap.remoteLib ) {
                pciValues[i].baseValue = pLibDev->pciValuesArray[i].turboValue;
            }
            else {
                REGW(devNum, pLibDev->pciValuesArray[i].offset, pLibDev->pciValuesArray[i].turboValue);
            }
        }
        else {
            if( pLibDev->devMap.remoteLib ) {
                pciValues[i].baseValue = pLibDev->pciValuesArray[i].baseValue;
            }
            else {
                if( isMerlin(pLibDev->swDevID) ) {
                    /* when fast clk mode doesn't apply, need to exit early */
                    /* find the fast clock values, exit before writing any.
                    * the first fast clock register occurs at the second
                    * occurrence of the reg offset in fastClkReg
                    */
                    if( fastClkReg == pLibDev->pciValuesArray[i].offset ) {
                        cntr++;
                        if( 2 == cntr ) { /* found fast clk values */
                            //printf("!fastClkApply: breaking...\n");
                            break;
                        }
                    }
                }

                /* write the values to the chip */
                REGW(devNum, pLibDev->pciValuesArray[i].offset, pLibDev->pciValuesArray[i].baseValue);
            }
        }
    } // end of for
}
/* LoadRegs */

/** 
 * - Function Name: SetChainMask
 * - Description  :
 * - Arguments
 *     - 
 * - Returns      :                                                                
 *******************************************************************************/
void SetChainMask( A_UINT32 devNum ) {
    LIB_DEV_INFO *pLibDev = gLibInfo.pLibDevArray[devNum];

    //printf("pLibDev->libCfgParams.tx_chain_mask is %d\n", pLibDev->libCfgParams.tx_chain_mask);
    //printf("pLibDev->libCfgParams.rx_chain_mask is %d\n", pLibDev->libCfgParams.rx_chain_mask);
    /* Setup Owl specific chain mask registers and swap bit before tx power/board init */
    if( (pLibDev->libCfgParams.rx_chain_mask == 0x5) && !(isOwl_2_Plus(pLibDev->macRev)) ) {
        /* 1.0 WAR */
        REGW(devNum, 0x99A4, 0x7);
        REGW(devNum, 0xA39C, 0x7);
    }
    else {
        REGW(devNum, 0x99A4, pLibDev->libCfgParams.rx_chain_mask & 0x7);
        REGW(devNum, 0xA39C, pLibDev->libCfgParams.rx_chain_mask & 0x7);
    }
    if( (pLibDev->swDevID == SW_DEVICE_ID_OWL) ) {
        /*
        * Set the self generated frame chain mask to match the tx setting
        * Note: 1 is OR'ed into tx chain mask register settings for diagnostic
        * chain masks of 0x4 and 0x2.
        */
        REGW(devNum, 0x832c, (pLibDev->libCfgParams.tx_chain_mask | 0x1) & 0x7);
    }
    else {
        REGW(devNum, 0x832c, (pLibDev->libCfgParams.tx_chain_mask) & 0x7);
    }

    // Chain 0 & 2 enabled chain hacking
    if( (pLibDev->libCfgParams.tx_chain_mask == 0x5) || ((pLibDev->libCfgParams.tx_chain_mask == 0x4) && ((pLibDev->swDevID == SW_DEVICE_ID_OWL))) || (pLibDev->libCfgParams.rx_chain_mask == 0x5) ) {
        // Set swap_alt_chain for 2 chain transmit via 0 and 2
        REGW(devNum, 0xA268, REGR(devNum, 0xA268) | 0x40);
    }
}
/* SetChainMask */

/** 
 * - Function Name: RunSpurMitigation
 * - Description  :
 * - Arguments
 *     - 
 * - Returns      :                                                                
 *******************************************************************************/
void RunSpurMitigation( A_UINT32 devNum, A_UINT32 freq ) {
    LIB_DEV_INFO *pLibDev = gLibInfo.pLibDevArray[devNum];

    // Set spur mitigation for OWL spur
    if(isOwl(pLibDev->swDevID)     &&
       !isSowl(pLibDev->macRev)    &&
       !isMerlin(pLibDev->swDevID) &&
#ifdef OWL_PB42
       !isHowlAP(devNum) &&
#endif
       !isSowl_Emulation(pLibDev->macRev) &&
       !isMerlin_Fowl_Emulation(pLibDev->macRev))
    {
        if(pLibDev->libCfgParams.ht40_enable) {
            setSpurMitigationOwl(devNum, freq);
        }
    }

    /* set spur mitigation for Merlin */
    if( isMerlin(pLibDev->swDevID) && (pLibDev->libCfgParams.spurChans[0] != 0) ) {
        setSpurMitigationMerlin(devNum, freq);
    }
}
/* RunSpurMitigation */

/** 
 * - Function Name: TurnOffChain
 * - Description  :
 * - Arguments
 *     - 
 * - Returns      :                                                                
 *******************************************************************************/
void TurnOffChain( A_UINT32 devNum ) {
    LIB_DEV_INFO *pLibDev = gLibInfo.pLibDevArray[devNum];

    /* Disable Fowl chain 0 for tx_mask of chain 1 or 2 only enabled */
    if ((pLibDev->libCfgParams.tx_chain_mask == 0x2) || (pLibDev->libCfgParams.tx_chain_mask == 0x4)) {
      A_CHAR *bank6names[] = {
              "rf_pwd2G", "rf_pwdmix2_int", "rf_pwdpa2_int", "rf_pwdmix5_int", "rf_pwdvga5_int", "rf_pwdpa5_int",
              "rf_pwd4Grx", "rf_pwd4Gtx", "rf_pwdfilt_int", "rf_pwdbbmix2_int", "rf_pwdbbmix5_int"
      };
      A_UINT32 numBk6Params = sizeof(bank6names)/sizeof(bank6names[0]), b;
      PARSE_FIELD_INFO bank6Fields[sizeof(bank6names)/sizeof(bank6names[0])];
      assert(numBk6Params == 11);

      /* Write Bank5 so following Bank6 update only changes Chain 0 */
      writeField(devNum, "rf_cs", 0);

      for (b = 0; b < numBk6Params; b++) {
          memcpy(bank6Fields[b].fieldName, bank6names[b], strlen(bank6names[b]) + 1);
          bank6Fields[b].valueString[0] = '1';
          bank6Fields[b].valueString[1] = '\0';
      }
      writeMultipleFieldsInRfBank(devNum, bank6Fields, numBk6Params, 6);

      /* Reset values in the reg file for next reset write of bank6 */
      for (b = 0; b < numBk6Params; b++) {
          bank6Fields[b].valueString[0] = '0';
          bank6Fields[b].valueString[1] = '\0';
      }
      changeMultipleFields(devNum, bank6Fields, numBk6Params);

      /* Reset Bank5 field to update all chains */
      writeField(devNum, "rf_cs", 3);

      /* Play with the XPA */
      REGW(devNum, 0xa3d8, (pLibDev->libCfgParams.tx_chain_mask << 1) | 0x01);
    }
}
/* TurnOffChain */

/** 
 * - Function Name: SetupStreamMapping
 * - Description  :
 * - Arguments
 *     - 
 * - Returns      :                                                                
 *******************************************************************************/
void SetupStreamMapping( A_UINT32 devNum ) {
    LIB_DEV_INFO *pLibDev = gLibInfo.pLibDevArray[devNum];

#ifdef _IQV
    // bit 8 is set to 0, so that direct mapping is used for multistream data. (one of spatial mapping)
    if( pLibDev->libCfgParams.ht40_enable ) {
        REGW(devNum, 0x9804, 0x2C4 | ((pLibDev->libCfgParams.extended_channel_op & EXT_CHANNEL_BIT_MASK) << 4) | ((pLibDev->libCfgParams.extended_channel_sep == 25) << 5));
    }
    else {
        REGW(devNum, 0x9804, 0x2C0);
    }
#else	
    if( pLibDev->libCfgParams.ht40_enable ) {
        REGW(devNum, 0x9804, REGR(devNum, 0x9804) | 0x2C4 | 
             ((pLibDev->libCfgParams.extended_channel_op & EXT_CHANNEL_BIT_MASK) << 4) | 
             ((pLibDev->libCfgParams.extended_channel_sep == 25) << 5));
    }
    else {
        REGW(devNum, 0x9804, REGR(devNum, 0x9804) | 0x2C0);
    }
#endif // _IQV
}
/* SetupStreamMapping */

/** 
 * - Function Name: AdjustTxThresh
 * - Description  :
 * - Arguments
 *     - 
 * - Returns      :                                                                
 *******************************************************************************/
void AdjustTxThresh( A_UINT32 devNum ) {
    LIB_DEV_INFO *pLibDev = gLibInfo.pLibDevArray[devNum];
    A_UINT32  newThreshold, newThresholdTurbo;

    getField(devNum, "mc_trig_level", &newThreshold, &newThresholdTurbo);
    if( (pLibDev->turbo == TURBO_ENABLE) && (ar5kInitData[pLibDev->ar5kInitIndex].cfgVersion < 2) ) {
        newThreshold = newThresholdTurbo * 2;
    }
    else {
        newThreshold = newThreshold * 2;
    }
    if( newThreshold > 0x3f ) {
        newThreshold = 0x3f;
    }
    changeField(devNum, "mc_trig_level", newThreshold);
    pLibDev->adjustTxThresh  = 0;
}
/* AdjustTxThresh */

/** 
 * - Function Name: CheckNoise
 * - Description  :
 * - Arguments
 *     - 
 * - Returns      :                                                                
 *******************************************************************************/
void CheckNoise( A_UINT32 devNum ) {
    LIB_DEV_INFO *pLibDev = gLibInfo.pLibDevArray[devNum];
    A_UINT32 temp1, temp2, temp3, temp4;
	A_INT32 nf_ctl = 0;
    A_INT32 nf_ext = 0;
    A_INT32  i;

    if (isOwl(pLibDev->swDevID)) {
        for (i = 0; i < (isOwl(pLibDev->swDevID) ? 3 : 2); i++) {
            temp1 = REGR(devNum, (0x9800 + (i * 0x1000) + (25<<2)));
            temp2 = (temp1 >> 19) & 0x1ff;

			if(isMerlin(pLibDev->swDevID)) { // bit location is changed in Merlin
				temp2 = (temp1 >> 20) & 0x1ff;
			}

			if ((temp2 & 0x100) != 0x0) {
                nf_ctl = 0 - ((temp2 ^ 0x1ff) + 1);
            }
            temp4 = 0;
			if (isOwl(pLibDev->swDevID)) {
				if((isSowl_Emulation(pLibDev->macRev)) || (isMerlin_Fowl_Emulation(pLibDev->macRev))) {
					printf ("channel = %d [ht40=%d] ", pLibDev->freqForResetDevice, pLibDev->libCfgParams.ht40_enable);
				}
				temp3 = REGR(devNum, (0x9800 + (i * 0x1000) + (111<<2)));
				temp4 = (temp3 >> 23) & 0x1ff;

			    if(isMerlin(pLibDev->swDevID)) { // bit location is changed in Merlin
				   temp4 = (temp3 >> 16) & 0x1ff;
				}

				if ((temp4 & 0x100) != 0x0) {
					nf_ext = 0 - ((temp4 ^ 0x1ff) + 1);
				}
			}
			if((isSowl_Emulation(pLibDev->macRev)) || (isMerlin_Fowl_Emulation(pLibDev->macRev))				) {
				printf("NF for Chain%d = ctl %d, ext %d \n", i, nf_ctl, nf_ext);
			}
        }
    }
}
/* CheckNoise */

//#define DBG
#undef DBG
/**
 * - Function Name: mini_resetDevice
 * - Description  : 
 * - Arguments
 *     -      
 * - Returns      : 
 ******************************************************************************/
MANLIB_API void mini_resetDevice(
 A_UINT32 devNum,
 A_UCHAR *mac,
 A_UCHAR *bss,
 A_UINT32 freq,
 A_UINT32 turbo,
 A_BOOL   hwRst)
{
    LIB_DEV_INFO *pLibDev = gLibInfo.pLibDevArray[devNum];
    A_UINT32 prev_freq;
    static A_UINT32 prevFreq = 0;
    static A_UINT32 currMacMode2040 = 99;

#ifndef CUSTOMER_REL
    A_UINT32 resetDevice_start, resetDevice_end;
//    printf("SNOOP::calling resetDevice:with turbo=%d:freq=%d\n", turbo, freq);
    resetDevice_start=milliTime();
#else
    static A_UINT32 rst      = 0;
#endif

    //strip off the channelMask flags, not using this any more, leave for now just
    //in case somewhere in the code we set the mask.
	pLibDev->channelMasks = turbo & 0xffff0000;
    turbo = turbo & 0xffff;
/*  printf(">>> resetDevice(%d), freq(%d)\n", rst++, freq);*/

    /* Ensure that the mode and the freq are in sync */
    /* This fix appears to break calibration at 2G, so leave it out until
     * it can be properly implemented
     */
#if (0)     
    if( freq < 4000 ) {
        pLibDev->mode = MODE_11G;
    }
    else {
        pLibDev->mode = MODE_11A;
    }
#endif    

//printf("SNOOP: freq = %d, channelMasks = %x\n", freq, pLibDev->channelMasks);

    prev_freq = pLibDev->freqForResetDevice;
    pLibDev->turbo = 0;
    pLibDev->mdkErrno = 0;

    if (checkDevNum(devNum) == FALSE) {
        mError(devNum, EINVAL, "Device Number %d:resetDevice\n", devNum);
        return;
    }
    if (gLibInfo.pLibDevArray[devNum]->devState < INIT_STATE) {
        mError(devNum, EILSEQ, "Device Number %d:resetDevice: Device should be initialized before calling reset\n", devNum);
        return;
    }
    
    pLibDev->hwDevID = (pLibDev->devMap.OScfgRead(devNum, 0) >> 16) & 0xffff;

    /* load config data, if it hasn't been loaded already */
    if( !loadCfgData(devNum, freq) ) {
        printf(" Load cfgdata\n");
        return;
    }

    /* Hardware reset */
    if( hwRst ) {
        hwReset(devNum, BUS_RESET | BB_RESET | MAC_RESET); /* takes about 219 msec */
    }

#if defined(DBG)    
    printf("time0(%d msec)\n", GetTickCount() - resetDevice_start); /* */
    temptime = GetTickCount();
#endif /* #if defined() */    

    // program the pll, check to see if we want to override it
    if( pLibDev->libCfgParams.applyPLLOverride && isOwl(pLibDev->swDevID) ) {
        REGW(devNum, 0x7014, pLibDev->libCfgParams.pllValue);
    }
    else {
        // MU:: I am not sure if updating reset freq in pLibDev at this point will cause any problem.
        // But for freq dependent xpa bias level feature to work properly, I need to have reset
        // frequency at this point.
        A_UINT32 tempFreq;
        tempFreq = pLibDev->freqForResetDevice;
        pLibDev->freqForResetDevice = freq;

        if( isMerlin(pLibDev->swDevID) ) {
            gFreqMHz = freq; /* set this prior to programming the pll (this is merlin specific)*/
        }

        pllProgram(devNum, turbo);
        pLibDev->freqForResetDevice = tempFreq;
    }

    // Put baseband into base/TURBO mode
    pLibDev->turbo = turbo;

    // Base mode
    REGW(devNum, PHY_FRAME_CONTROL, REGR(devNum, PHY_FRAME_CONTROL) & ~PHY_FC_TURBO_MODE);

#ifdef OWL_PB42
    if( isHowlAP(devNum) )
    //Setting DAC/ADC Phase Sel to 3
    {
        int rddata,wrdata;
        rddata = hwRtcRegRead(devNum,0x10);
        wrdata = ((rddata & 0xffffffcc) | (0 << 4) | 0x3);
        hwRtcRegWrite(devNum,0x10, wrdata);
    }
#endif
    /******************************************************/
    /* check if fast clk mode settings need to be applied */
    /******************************************************/
    if( isMerlin(pLibDev->swDevID) ) {
        /* the first time around, find the first fast_clock field in
        * pLibDev->regArray[] so that subsequent calls to resetDevice()
        * can use that information to determine whether to write in the 
        * fast clock fields or not. This has to be done for all frequencies.
        */
        if( !fastClkFieldFound ) {
            FindFastClkField(devNum);
        }

        /* check if fast clock applies */
        fastClkApply = FaskClkCheck(devNum, freq);
    }

    /*******************************************************/
    /* update the regArray entries that are mode dependent */
    /*******************************************************/
    UpdateModeRegs(devNum);

    //see if need to perform any tx threshold adjusting
    if( pLibDev->adjustTxThresh ) {
        AdjustTxThresh(devNum);
    }

    // Setup Values from EEPROM - only touches EEPROM the first time
    //FJC: 06/09/03 moved reading eeprom to here, since it reads the EAR
    if( pLibDev->eePromLoad ) {
        if( pLibDev->swDevID >= 0x0012 ) {
            REGW(devNum, 0x6010, REGR(devNum, 0x6010) | 0x3);
        }
        if( !setupEEPromMap(devNum) ) {
            mError(devNum, EIO, "Error: unable to load EEPROM\n");
            return;
        }
    }

    /***************************************************/
    /* Initialize chips with values from register file */
    /***************************************************/
    LoadRegs(devNum);

    /* delta_slope_coeff_exp and delta_slope_coeff_man for venice */
    if( (freq != prevFreq) || 
        (currMacMode2040 != pLibDev->libCfgParams.ht40_enable) ) {
        double fclk = 40;

        if( pLibDev->mode == MODE_11A || pLibDev->mode == MODE_11G ) {
            calculateDeltaCoeffs(devNum, fclk, freq);
        }
        /* update the current mac mode */
        currMacMode2040 = pLibDev->libCfgParams.ht40_enable;
    }

    //Disable all the mac queue clocks
    disable5211QueueClocks(devNum);

// Byteswap Tx and Rx descriptors for Big Endian systems
#if defined(ARCH_BIG_ENDIAN)
#ifdef OWL_PB42
    if( isHowlAP(devNum) ) {
        REGW(devNum, F2_CFG, F2_CFG_SWTB|F2_CFG_SWRB);
    }
    else {
#endif
        REGW(devNum, F2_CFG, F2_CFG_SWTD | F2_CFG_SWRD);
#ifdef OWL_PB42
    }
#endif
#endif /* defined(ARCH_BIG_ENDIAN) */

    /* Setup the macAddr in the chip */
    if( hwRst ) {
        writeTestAddr(devNum, mac, bss);
    }

    // Set the RX Disable to stop even packets outside of RX_FILTER (ProbeRequest)
    // Note that scripts must turn off RX Disable to receive packets inc. ACKs.
    ar5kInitData[pLibDev->ar5kInitIndex].pMacAPI->disableRx(devNum);

    // Writing to F2_BEACON will start timers. Hence it should be the last
    // register to be written.
    REGW(devNum, F2_BEACON, F2_BEACON_RESET_TSF | 0xffff);

    /* mask setup ??? */
    if( isOwl(pLibDev->swDevID) ) {
        SetChainMask(devNum);
    }

#ifdef OWL_PB42
    if( isHowlAP(devNum) ) {
        // Enable IQ Swap Specific to Howl
        int rddata;
        rddata = REGR(devNum,0x9800+(666<<2));
        int wrdata;
        wrdata = rddata | 0x1;
        REGW(devNum,0x9800+(666<<2), wrdata);
    }
#endif

#if defined(DBG)    
    printf("time0.8(%d msec)\n", GetTickCount() - temptime); /* 63 */
    temptime = GetTickCount();
#endif /* #if defined() */    

    // Setup the transmit power values for cards since 0x0[0-2]05
    pLibDev->freqForResetDevice = freq;

    if( isMerlin(pLibDev->swDevID) ) {
        /* set the power and the channel only if appropriate */
        if( (freq != prevFreq) || hwRst ){
            initializeTransmitPower(devNum, freq, 0, NULL);
#if defined(DBG)    
    printf("time0.9(%d msec)\n", GetTickCount() - temptime); /*  */
    temptime = GetTickCount();
#endif /* #if defined() */    
            setChannel(devNum, freq);
        }
    }else{
        initializeTransmitPower(devNum, freq, 0, NULL);
        setChannel(devNum, freq);
    }

#if defined(DBG)    
    printf("time1.0(%d msec)\n", GetTickCount() - temptime); /*  */
    temptime = GetTickCount();
#endif /* #if defined() */    

    RunSpurMitigation(devNum, freq);
#if defined(DBG)    
    printf("time1.1(%d msec)\n", GetTickCount() - temptime); /* 266(32) */
    temptime = GetTickCount();
#endif /* #if defined() */    

    if( isOwl(pLibDev->swDevID) && !isMerlin(pLibDev->swDevID) ) {
        if( freq < 2412 ) { /* not right ??? */
            changeField(devNum, "rf_pwd_icsyndiv", 0);
        }
        else if( freq <= 2422 ) {
            changeField(devNum, "rf_pwd_icsyndiv", 1);
        }
        else {
            changeField(devNum, "rf_pwd_icsyndiv", 2);
        }
    }

    /* BAA: write "1" to bit 22 here to avoid disappearing card problem */
    if( isMerlin(pLibDev->swDevID) ) {
        REGW(devNum, 0x4004, 0x0040073b);
    }
    else {
        REGW(devNum, 0x4004, 0x0040073f);
    }

    // For owl based devices when tx_mask is 2 and 4 we enable chain 0 on baseband but turn off chain 0 on fowl.
    if( pLibDev->swDevID == SW_DEVICE_ID_OWL ) {
        TurnOffChain(devNum);
    }

    SetupStreamMapping(devNum);
#if defined(DBG)    
    printf("time1.2(%d msec)\n", GetTickCount() - temptime); /* 0 */
    temptime = GetTickCount();
#endif /* #if defined() */    

    // activate D2
    REGW(devNum, PHY_ACTIVE, PHY_ACTIVE_EN);
    mSleep(2); /* need to wait before running cals after activating the BB */

    if( (pLibDev->swDevID == 0xf11b) && ((pLibDev->mode == MODE_11B) || (pLibDev->mode == MODE_11G))) {
        REGW(devNum, 0xd808,0x502);
        //mSleep(2);
    }
#if defined(DBG)    
    printf("time1.3(%d msec)\n", GetTickCount() - temptime); /* 16 */
    temptime = GetTickCount();
#endif /* #if defined() */    

    /* set up GPIO 9 to switch the Band Select pin on the FEM
     * to have its VLNA input line control the appropriate LNA
     */;
    if(isMerlin(pLibDev->swDevID) && pLibDev->femBandSel) {
        if (pLibDev->mode == MODE_11G) {
            ar5416SetGpio(devNum,9,1); /* Set high to control 2GHz LNA */
        }
        else {
            ar5416SetGpio(devNum,9,0); /* Set low to control 5GHz LNA */
        }
    }
#if defined(DBG)    
    printf("time1.4(%d msec)\n", GetTickCount() - temptime); /* 0 */
    temptime = GetTickCount();
#endif /* #if defined() */    

    /* run calibrations */
#ifndef CUSTOMER_REL /* for internal builds don't use the workaround since it 
                      * may cause problems in mdk or dvt 
                      */
    if((enableCal && (freq != prevFreq)) || hwRst) {
        RunCals(devNum, freq);
    }
#else /* This is a workaround for a bug that has not been fixed yet.
       * Skip the first 8 resetDevice calls as the mode and freq
       * may be out of sync and this causes the carrier leak cal
       * (and other cals) to fail and add delay time.
       */
    if((enableCal && (rst > 7) && (freq != prevFreq)) || hwRst) {
        RunCals(devNum, freq);
    }
    rst++;
#endif

#if defined(DBG)    
    printf("time2(%d msec)\n", GetTickCount() - temptime); /* 66 */
    temptime = GetTickCount();
#endif /* #if defined() */    

    pLibDev->devState = RESET_STATE;

    if( !isMerlin(pLibDev->swDevID) ) {
        CheckNoise(devNum);
    }

    if (isOwl(pLibDev->swDevID) && (pLibDev->libCfgParams.rx_chain_mask == 0x5) && !(isOwl_2_Plus(pLibDev->macRev))) {
        /* TODO 1.0 WAR */
        REGW(devNum, 0x99A4, 0x5);
    }

    /* save the current frequency before leaving */
    prevFreq = freq;

#ifndef __ATH_DJGPPDOS__
    // ART ANI routine
    tweakArtAni(devNum, prev_freq, pLibDev->freqForResetDevice);
#endif
#ifndef CUSTOMER_REL
    resetDevice_end=milliTime();
//printf("SNOOP::exit resetDevice:Time taken = %dms\n", (resetDevice_end-resetDevice_start));
#endif
}
/* mini_resetDevice */

/**************************************************************************
* resetDevice - reset the device and initialize all registers
*
*/
MANLIB_API void resetDevice(
 A_UINT32 devNum,
 A_UCHAR *mac,
 A_UCHAR *bss,
 A_UINT32 freq,
 A_UINT32 turbo)
{
    LIB_DEV_INFO *pLibDev = gLibInfo.pLibDevArray[devNum];
    A_INT32  i;
    A_UINT32  *pValue = NULL;
    A_UINT32  newThreshold, newThresholdTurbo;
    A_UINT32 temp1, temp2, temp3, temp4, temp5, temp6;
    A_UINT32 cntr = 0;
    A_UINT32 modifier;
    A_BOOL   earHere = FALSE;
    A_UINT32 prev_freq;
    A_UINT32 gpioData;
    A_INT32  timeout = 0;
	A_INT32 nf_ctl = 0, nf_ext = 0;

#ifndef CUSTOMER_REL
    A_UINT32 resetDevice_start, resetDevice_end;
//    printf("SNOOP::calling resetDevice:with turbo=%d:freq=%d\n", turbo, freq);
    resetDevice_start=milliTime();
#else
    static A_UINT32 rst = 0;
#endif
    //strip off the channelMask flags, not using this any more, leave for now just
    //in case somewhere in the code we set the mask.
	pLibDev->channelMasks = turbo & 0xffff0000;
    turbo = turbo & 0xffff;
/*printf(">>> resetDevice(%d), freq(%d)\n", rst++, freq);*/

    /* Ensure that the mode and the freq are in sync */
    /* This fix appears to break calibration at 2G, so leave it out until
     * it can be properly implemented
     */
#if (0)     
    if( freq < 4000 ) {
        pLibDev->mode = MODE_11G;
    }
    else {
        pLibDev->mode = MODE_11A;
    }
#endif    

    //look for the other alternative way of setting the 2.5Mhz channel centers
    //look for channel *10
//printf("SNOOP: freq at 1= %d\n", freq);

    if((freq >= 49000) && (freq < 60000)) {
        if((freq % 10) == 0) {
            freq = freq / 10;
//printf("SNOOP: freq at 2= %d\n", freq);
        }
        else {
            //must be a 2.5 MHz channel - check
//printf("SNOOP: freq at 3= %d\n", freq);
            if((freq % 25) != 0) {
                mError(devNum, EINVAL, "Illegal channel value %d:resetDevice\n", freq);
                return;
            }
            pLibDev->channelMasks |= QUARTER_CHANNEL_MASK;
            freq = (freq - 25) / 10;
        }
    }
    if((freq >= 23020) && (freq <= 25140)) {
        if((freq % 10) == 0) {
            freq = freq / 10;
        }
        else {
            mError(devNum, EINVAL, "Illegal channel value %d:resetDevice\n", freq);
            return;
        }
    }
//printf("SNOOP: freq = %d, channelMasks = %x\n", freq, pLibDev->channelMasks);
//printf("SNOOP: txm = %x, rxm = %x, ht40 = %d, gi = %d\n", pLibDev->libCfgParams.tx_chain_mask,
//	   pLibDev->libCfgParams.rx_chain_mask, pLibDev->libCfgParams.ht40_enable, pLibDev->libCfgParams.short_gi_enable);
    // temporarily disable eeprom load for griffin SNOOP: remove asap

    prev_freq = pLibDev->freqForResetDevice;

    pLibDev->turbo = 0;
    pLibDev->mdkErrno = 0;
    if (checkDevNum(devNum) == FALSE) {
        mError(devNum, EINVAL, "Device Number %d:resetDevice\n", devNum);
        return;
    }
    if (gLibInfo.pLibDevArray[devNum]->devState < INIT_STATE) {
        mError(devNum, EILSEQ, "Device Number %d:resetDevice: Device should be initialized before calling reset\n", devNum);
        return;
    }

    pLibDev->hwDevID = (pLibDev->devMap.OScfgRead(devNum, 0) >> 16) & 0xffff;

// Dont call the driver init code for now
#if (MDK_AP) || (CUSTOMER_REL) || (!INCLUDE_DRIVER_CODE)
    pLibDev->use_init = !DRIVER_INIT_CODE;
#endif
    if (pLibDev->use_init == DRIVER_INIT_CODE) {
#if (!CUSTOMER_REL) && (INCLUDE_DRIVER_CODE)
#ifndef NO_LIB_PRINT
       printf("resetDevice::Using Driver init code\n");
#endif

       if (m_ar5212Reset(pLibDev, mac, bss, freq, turbo) != 0) {

       }
       if (!loadCfgData(devNum, freq))
         {
                 printf(" Load cfgdata\n");
                 return;
         }
        if(!setupEEPromMap(devNum))
        {
            mError(devNum, EIO, "Error: unable to load EEPROM\n");
            return;
        }
#endif //CUSTOMER_REL
    }
    else {
    // load config data, if return value = 0 return from resetDevice
    if (!loadCfgData(devNum, freq))
    {
            printf(" Load cfgdata\n");
            return;
    }

//++JC++
	if((((pLibDev->swDevID & 0xff) == 0x0018) || isNala(pLibDev->swDevID)) && (pLibDev->mode == MODE_11A)) {
		pLibDev->mode = MODE_11G;
//              printf("Debug:: Setting 11g mode in resetDevice for Griffin\n");
    }
//++JC++
    hwReset(devNum, BUS_RESET | BB_RESET | MAC_RESET);
    // program the pll, check to see if we want to override it
    if (pLibDev->libCfgParams.applyPLLOverride) {
        if (isOwl(pLibDev->swDevID)) {
            REGW(devNum, 0x7014, pLibDev->libCfgParams.pllValue);
        } else {
            REGW(devNum, 0xa200, 0);
            REGW(devNum, 0x987c, pLibDev->libCfgParams.pllValue);
        }
        mSleep(2);
    } else {
        // MU:: I am not sure if updating reset freq in pLibDev at this point will cause any problem.
        // But for freq dependent xpa bias level feature to work properly, I need to have reset
        // frequency at this point.
        A_UINT32 tempFreq;
        tempFreq = pLibDev->freqForResetDevice;
        pLibDev->freqForResetDevice = freq;

        if( isMerlin(pLibDev->swDevID) ) {
            gFreqMHz = freq; /* set this prior to programming the pll (this is merlin specific)*/
        }

        pllProgram(devNum, turbo);
        pLibDev->freqForResetDevice = tempFreq;
    }

    // Put baseband into base/TURBO mode
    pLibDev->turbo = turbo;
    if(turbo == TURBO_ENABLE) {
            REGW(devNum, PHY_FRAME_CONTROL, REGR(devNum, PHY_FRAME_CONTROL) | PHY_FC_TURBO_MODE);

            //check the turbo bit is set
            //FJC, increased this timeout from 10 to 20, sometimes predator fails here.
            for( i = 0; i < 20; i++ ) {
                if( REGR(devNum, PHY_FRAME_CONTROL) & PHY_FC_TURBO_MODE ) {
                    break;
                }
                mSleep(1);
            }


            if( i == 10 ) {
                mError(devNum, EIO, "Device Number %d:ResetDevice: Unable to put the device into tubo mode\n", devNum);
                return;
            }

            // Reset the baseband
            hwReset(devNum, BB_RESET);
    }
    else {
        // Base mode
        REGW(devNum, PHY_FRAME_CONTROL, REGR(devNum, PHY_FRAME_CONTROL) & ~PHY_FC_TURBO_MODE);
        //put these back, in case they were written for 11g turbo
    }

    /* Handle mode switching, get a ptr to the top of the modal array */
    switch (pLibDev->mode) {
    case MODE_11A:  //11a
        if (isOwl(pLibDev->swDevID) && pLibDev->libCfgParams.ht40_enable) 
        {
            pValue = &(pLibDev->pModeArray->value11aTurbo);  //11a ht40
        } else if(pLibDev->turbo != TURBO_ENABLE) {    //11a base
            pValue = &(pLibDev->pModeArray->value11a);
        } else {
            pValue = &(pLibDev->pModeArray->value11aTurbo);
        }
        break;

    case MODE_11G: //11g
        if (isOwl(pLibDev->swDevID) && pLibDev->libCfgParams.ht40_enable) {
            pValue = &(pLibDev->pModeArray->value11b); // ht40 params live in the 11b column
        } else if ((pLibDev->turbo != TURBO_ENABLE) || (ar5kInitData[pLibDev->ar5kInitIndex].cfgVersion < 3)) { //11g base
            pValue = &(pLibDev->pModeArray->value11g);
        } else {
            pValue = &(pLibDev->pModeArray->value11gTurbo);
        }
        break;

    case MODE_11B: //11b
        pValue = &(pLibDev->pModeArray->value11b);
        break;
    } //end switch

#ifdef OWL_PB42
     if(isHowlAP(devNum))
     //Setting DAC/ADC Phase Sel to 3
     {
       int rddata,wrdata;
       rddata = hwRtcRegRead(devNum,0x10);
       wrdata = ((rddata & 0xffffffcc) | (0 << 4) | 0x3);
       hwRtcRegWrite(devNum,0x10, wrdata);
     }
#endif
        /******************************************************/
        /* check if fast clk mode settings need to be applied */
        /******************************************************/
         /* the first time around, find the first fast_clock field in
          * pLibDev->regArray[] so that subsequent calls to resetDevice()
          * can use that information to determine whether to write in the 
          * fast clock fields or not. This has to be done for all frequencies.
          */
        if( isMerlin(pLibDev->swDevID) ) {
            if( !fastClkFieldFound ) {
                for( i = 0; i < pLibDev->sizeRegArray; i++ ) {
                    if( strncmp(pLibDev->regArray[i].fieldName, "fas", 3) == 0 ) {
                        /* this is the first fast_clock field,
                         * save the register offset
                         */
                        fastClkReg = pLibDev->regArray[i].regOffset;
/*                      printf(">>> field: %s, reg offset: 0x%08lx\n",*/
/*                             pLibDev->regArray[i].fieldName,        */
/*                             pLibDev->regArray[i].regOffset);       */
                        fastClkFieldFound = 1;
                        break;
                    }
                }
            }
        }

        
        if( isMerlin(pLibDev->swDevID) ) {
            /* among 5G frequencies, check for 5MHz separation ones */
            if( freq > 4000 ) {
                /* check for fast clock condition */
                if( pLibDev->libCfgParams.fastClk5g ) {
                    fastClkApply = 1;
                    //printf("> fastClkApply: %d (ALL)\n", fastClkApply);
                }
                else if( (freq % 20) == 0 ) {
                    fastClkApply = 0;
                }
                else if( (freq % 10) == 0 ) {
                    fastClkApply = 0;
                }
                else { /* apply to 5MHz channels only */
                    fastClkApply = 1;
                }
            }
            else {
                fastClkApply = 0; /* a 2G frequency */
            }
            //printf("> fastClkApply: %d, freq: %d\n", fastClkApply, freq);
        }

    /*******************************************************/
    /* update the regArray entries that are mode dependent */
    /*******************************************************/
    for(i = 0; i < pLibDev->sizeModeArray; i++) {

            /* Deal with fast clock and Rx gain table options during the
             * updating process.  This process updates the default values of
             * the reg offsets in the pciValuesArray[]. The default reg offset 
             * values will be updated unless the new values are not applicable.
             */
            if( isMerlin(pLibDev->swDevID) ) {
                /* do not update fast clk fields when not applicable */
                if( !fastClkApply ) {
                    if( strncmp(pLibDev->pModeArray[i].fieldName,"fas", 3) == 0 ) {
                        pValue = (A_UINT32 *)((A_CHAR *)pValue + sizeof(MODE_INFO));
                        continue; /* skip updating */
                    }
                }

                /* skip original Rx gain table values if not applicable */
                if( pLibDev->libCfgParams.rxGainType != RX_GAIN_ORIG ) {
                    if( strncmp(pLibDev->pModeArray[i].fieldName,"ori", 3) == 0 ) {
                        pValue = (A_UINT32 *)((A_CHAR *)pValue + sizeof(MODE_INFO));
                        continue; /* skip updating */
                    }
                } 

                /* skip 13dB backoff Rx gain table values if not applicable */
                if( pLibDev->libCfgParams.rxGainType != RX_GAIN_13dB_BACKOFF ) {
                    if( strncmp(pLibDev->pModeArray[i].fieldName,"bac", 3) == 0 ) {
                        pValue = (A_UINT32 *)((A_CHAR *)pValue + sizeof(MODE_INFO));
                        continue; /* skip updating */
                    }
                }
            }

            /* update the pci reg array, but do not write into chip */
            updateField(devNum, 
                        &pLibDev->regArray[pLibDev->pModeArray[i].indexToMainArray],
                        *pValue, 
                        0);
        //increment value pointer by size of an array element
        pValue = (A_UINT32 *)((A_CHAR *)pValue + sizeof(MODE_INFO));
    }

    //see if need to perform any tx threshold adjusting
    if(pLibDev->adjustTxThresh) {
        getField(devNum, "mc_trig_level", &newThreshold, &newThresholdTurbo);
        if((pLibDev->turbo == TURBO_ENABLE) && (ar5kInitData[pLibDev->ar5kInitIndex].cfgVersion < 2)) {
            newThreshold = newThresholdTurbo * 2;
        }
        else {
            newThreshold = newThreshold * 2;
        }
        if (newThreshold > 0x3f) {
            newThreshold = 0x3f;
        }
        changeField(devNum, "mc_trig_level", newThreshold);
        pLibDev->adjustTxThresh  = 0;
    }
    if(((pLibDev->swDevID & 0xff) >= 0x13) && (!isFalcon(devNum))) {
        if(pLibDev->libCfgParams.enableXR) {
            changeField(devNum, "bb_enable_xr", 1);
        }
        else {
                /* field does not exist in merlin, at least, not as such */
                if( !isMerlin(pLibDev->swDevID) ) {
                    changeField(devNum, "bb_enable_xr", 0); 
                }
        }
    }

    // Setup Values from EEPROM - only touches EEPROM the first time
    //FJC: 06/09/03 moved reading eeprom to here, since it reads the EAR
    if (pLibDev->eePromLoad) {
        if(pLibDev->swDevID >= 0x0012) {
            REGW(devNum, 0x6010, REGR(devNum, 0x6010) | 0x3);
        }
        if(!setupEEPromMap(devNum)) {
            mError(devNum, EIO, "Error: unable to load EEPROM\n");
            return;
        }
        if((((pLibDev->eepData.version >> 12) & 0xF) >= 4) &&
           (((pLibDev->eepData.version >> 12) & 0xF) <= 5) &&
           (!isFalcon(devNum)))
        {
            earHere = ar5212IsEarEngaged(devNum, pLibDev->pEarHead, freq);
//          printf("In reset Device pLibDev->p16kEepHeader->majorVersion loadcfgdata ar5212IsEarEngaged\n");
        }
    }

    //do a check for EAR changes to rf registers
    if ((earHere) && (!isFalcon(devNum))){
        ar5212EarModify(devNum, pLibDev->pEarHead, EAR_LC_RF_WRITE, freq, &modifier);
    }

    /***************************************************/
    /* Initialize chips with values from register file */
    /***************************************************/
        for( i = 0; i < pLibDev->sizePciValuesArray; i++ ) {
            /* if dealing with a version 2 config file,
             * then all values are found in the base array
             */
            pciValues[i].offset = pLibDev->pciValuesArray[i].offset;

            if( (pLibDev->turbo == TURBO_ENABLE) && (ar5kInitData[pLibDev->ar5kInitIndex].cfgVersion < 2) ) 
            {
                if( pLibDev->devMap.remoteLib ) {
                    pciValues[i].baseValue = pLibDev->pciValuesArray[i].turboValue;
                }
                else {
#if defined(COBRA_AP)
                    if( pLibDev->pciValuesArray[i].offset < 0x14000 )
#endif
                        REGW(devNum, pLibDev->pciValuesArray[i].offset, pLibDev->pciValuesArray[i].turboValue);
                }
            }
            else {
                if( pLibDev->devMap.remoteLib ) {
                    pciValues[i].baseValue = pLibDev->pciValuesArray[i].baseValue;
                }
                else {
#if defined(COBRA_AP)
                if( pLibDev->pciValuesArray[i].offset < 0x14000 )
#endif
                    if( isMerlin(pLibDev->swDevID) ) {
                        /* look for the first reg offset that repeats in the 
                         * pciValuesArray[] and exit before writing any of the 
                         * values that follow.  Given the order in which the 
                         * fields are in the cfg file, the first offset that 
                         * will repeat will be the first fast clock value 
                         */
#if (0)                         
                            if( 0x9824 == pLibDev->pciValuesArray[i].offset ) {
                                printf(">>> i: %d, offset: 0x%08lx, val: 0x%08lx\n",
                                       i,
                                       pLibDev->pciValuesArray[i].offset,
                                       pLibDev->pciValuesArray[i].baseValue);
                            }
#endif                        
                        if( fastClkReg == pLibDev->pciValuesArray[i].offset ) {
                            cntr++;
                            if( 2 == cntr ) { /* found fast clk values */
                                //printf("!fastClkApply: breaking...\n");
                                break;
                            }
                        }
                    }

                    /* write the values to the chip */
                    REGW(devNum, pLibDev->pciValuesArray[i].offset, pLibDev->pciValuesArray[i].baseValue);
                }
            }
        } // end of for

    if (pLibDev->devMap.remoteLib)
        sendPciWrites(devNum, pciValues, pLibDev->sizePciValuesArray);

    /* delta_slope_coeff_exp and delta_slope_coeff_man for venice */
    if ((pLibDev->swDevID & 0x00ff) >= 0x0013) {
        double fclk,coeff, coeff_shgi;
        A_UINT32 coeff_exp_shgi, coeff_man_shgi;
        A_UINT32 coeffExp,coeffMan;
        A_UINT32 deltaSlopeCoeffMan, deltaSlopeCoeffExp;
        A_UINT32 progCoeff;

        switch (pLibDev->mode) {
            case MODE_11A:
                if(pLibDev->swDevID == 0xf013) {
                    fclk = 16.0;
                } else {
                    switch (turbo) {
                    case QUARTER_SPEED_MODE:
                        fclk = 10;
                        if(isEagle(pLibDev->swDevID)) {
                            writeField(devNum, "bb_quarter_rate_mode", 1);
                            writeField(devNum, "bb_half_rate_mode", 0);
                            writeField(devNum, "bb_window_length", 3);
                        }
                        break;
                    case HALF_SPEED_MODE:
                        fclk = 20;
                        if(isEagle(pLibDev->swDevID)) {
                            writeField(devNum, "bb_quarter_rate_mode", 0);
                            writeField(devNum, "bb_half_rate_mode", 1);
                            writeField(devNum, "bb_window_length", 3);
                        }
                        break;
                    case TURBO_ENABLE:
                        fclk = 80;
                        if(isEagle(pLibDev->swDevID)) {
                            writeField(devNum, "bb_quarter_rate_mode", 0);
                            writeField(devNum, "bb_half_rate_mode", 0);
                            writeField(devNum, "bb_window_length", 0);
                        }
                        break;
                    default :   // base
                        fclk = 40;
                        if(isEagle(pLibDev->swDevID)) {
                            writeField(devNum, "bb_quarter_rate_mode", 0);
                            writeField(devNum, "bb_half_rate_mode", 0);
                            writeField(devNum, "bb_window_length", 0);
                        }
                        break;
                    }
                }
                progCoeff = 1;
                break;
            case MODE_11G:
                if(pLibDev->swDevID == 0xf013) {
                    fclk = (16.0 * 10.0) / 11.0;
                }
                else {
                    switch (turbo) {
                    case QUARTER_SPEED_MODE:
                        fclk = 10;
                        if(isEagle(pLibDev->swDevID)) {
                            writeField(devNum, "bb_quarter_rate_mode", 1);
                            writeField(devNum, "bb_half_rate_mode", 0);
                        }
                        break;
                    case HALF_SPEED_MODE:
                        fclk = 20;
                        if(isEagle(pLibDev->swDevID)) {
                            writeField(devNum, "bb_quarter_rate_mode", 0);
                            writeField(devNum, "bb_half_rate_mode", 1);
                        }
                        break;
                    case TURBO_ENABLE:
                        fclk = 80;
                        if(isEagle(pLibDev->swDevID)) {
                            writeField(devNum, "bb_quarter_rate_mode", 0);
                            writeField(devNum, "bb_half_rate_mode", 0);
                        }
                        break;
                    default :   // base
                        fclk = 40;
                        if(isEagle(pLibDev->swDevID)) {
                            writeField(devNum, "bb_quarter_rate_mode", 0);
                            writeField(devNum, "bb_half_rate_mode", 0);
                        }
                        break;
                    }
                }
                progCoeff = 1;
                break;
            default:
                fclk = 0;
                progCoeff = 0;
                break;
        }
		if(isSowl_Emulation(pLibDev->macRev) ||
		   isMerlin_Fowl_Emulation(pLibDev->macRev)) {
			REGW(devNum, 0x10f0, ((14 << 4) | (24 << 10)));
			REGW(devNum, 0x801c, ((29 << 20) | (54 << 14) | (23 << 7) | 23));
			if(pLibDev->libCfgParams.ht40_enable == 0x0) {
				fclk = 21.82;  //HT20
			} else {
                fclk = 10.91;  //HT40
			}
			printf("FLCK is %f\n", fclk);
		}
        
        if (progCoeff) {
            coeff = (2.5 * fclk) / ((double)freq);
#ifndef _IQV
            coeffExp = 14 -  (int)(floor(log10(coeff)/log10(2)));
            coeffMan = (int)(floor((coeff*(pow(2,coeffExp))) + 0.5));
#else // to avoid compiler error at LP
            coeffExp = 14 -  (int)(floor(log10((double)coeff)/log10(2.0)));
            coeffMan = (int)(floor((coeff*(pow(2.0,(double)coeffExp))) + 0.5));
#endif
            deltaSlopeCoeffExp = coeffExp - 16;
            deltaSlopeCoeffMan = coeffMan;
            REGW(devNum, 0x9814, (REGR(devNum,0x9814) & 0x00001fff) |
                                 (deltaSlopeCoeffExp << 13) |
                                      (deltaSlopeCoeffMan << 17));
//            printf("Delta Coeff Exp = %d   Delta Coeff Man = %d\n", deltaSlopeCoeffExp, deltaSlopeCoeffMan);
            if (isOwl(pLibDev->swDevID)) {

                coeff_shgi = (0.9 * 2.5 * fclk) / freq;
#ifndef _IQV
                coeff_exp_shgi  = (A_UINT32) (14 - floor(log10(coeff_shgi)/log10(2)));
                coeff_man_shgi = (A_UINT32) floor((coeff_shgi * pow(2, coeff_exp_shgi)) + 0.5);
#else
                coeff_exp_shgi  = (A_UINT32) (14 - floor(log10((double)coeff_shgi)/log10(2.0)));
                coeff_man_shgi = (A_UINT32) floor((coeff_shgi * pow(2.0, (double)coeff_exp_shgi)) + 0.5);
#endif
                coeff_exp_shgi -= 16;
                //printf("Delta Coeff Exp Short GI = %d   Delta Coeff Man Short GI = %d\n", coeff_exp_shgi, coeff_man_shgi);
                REGW(devNum, 0x99D0, (coeff_man_shgi << 4) | coeff_exp_shgi);
            }
        }
    }
    //Disable all the mac queue clocks
    if((pLibDev->swDevID != 0x0007) && (pLibDev->swDevID != 0x0010)) {
        disable5211QueueClocks(devNum);
    }


// Byteswap Tx and Rx descriptors for Big Endian systems
#if defined(ARCH_BIG_ENDIAN)
#ifdef OWL_PB42
if(isHowlAP(devNum))
 {
   REGW(devNum, F2_CFG, F2_CFG_SWTB|F2_CFG_SWRB);
 }
else
 {
#endif
    REGW(devNum, F2_CFG, F2_CFG_SWTD | F2_CFG_SWRD);
#ifdef OWL_PB42
 }
#endif

#endif

    // Setup the macAddr in the chip
    memcpy(pLibDev->macAddr.octets, mac, WLAN_MAC_ADDR_SIZE);
#ifndef ARCH_BIG_ENDIAN
    REGW(devNum, F2_STA_ID0, pLibDev->macAddr.st.word);
    temp1 = REGR(devNum, F2_STA_ID1);
    temp2 = (temp1 & 0xffff0000) | F2_STA_ID1_AD_HOC | pLibDev->macAddr.st.half | F2_STA_ID1_DESC_ANT;
#ifdef _IQV
		// set sequence number fixed, disable the sequence number auto-increase function
		#define F2_STA_ID1_FIXED_SEQ  0x20000000
		temp2 = temp2 | F2_STA_ID1_FIXED_SEQ;
#endif	//  _IQV 		
    REGW(devNum, F2_STA_ID1, temp2);
#else
    {
        A_UINT32 addr;

        addr = swap_l(pLibDev->macAddr.st.word);
        REGW(devNum, F2_STA_ID0, addr);
        addr = (A_UINT32)swap_s(pLibDev->macAddr.st.half);
        REGW(devNum, F2_STA_ID1, (REGR(devNum, F2_STA_ID1) & 0xffff0000) | F2_STA_ID1_AD_HOC |
                addr | F2_STA_ID1_DESC_ANT);
    }
#endif

    // then our BSSID
    memcpy(pLibDev->bssAddr.octets, bss, WLAN_MAC_ADDR_SIZE);
#ifndef ARCH_BIG_ENDIAN
    REGW(devNum, F2_BSS_ID0, pLibDev->bssAddr.st.word);
    REGW(devNum, F2_BSS_ID1, pLibDev->bssAddr.st.half);
#else
    {
        A_UINT32 addr;

        addr = swap_l(pLibDev->bssAddr.st.word);
        REGW(devNum, F2_BSS_ID0, addr);
        addr = (A_UINT32)swap_s(pLibDev->bssAddr.st.half);
        REGW(devNum, F2_BSS_ID1, addr);
    }
#endif


    REGW(devNum, F2_BCR, F2_BCR_STAMODE);
    REGR(devNum, F2_BSR); // cleared on read

    // enable activity leds and clock run enable
#if defined(COBRA_AP)
    sysRegWrite(AR531XPLUS_PCI + F2_PCICFG, sysRegRead(AR531XPLUS_PCI + F2_PCICFG) | F2_PCICFG_CLKRUNEN | F2_PCICFG_LED_ACT);
#else
//  REGW(devNum, F2_PCICFG, REGR(devNum, F2_PCICFG) | F2_PCICFG_CLKRUNEN | F2_PCICFG_LED_ACT);
#endif

    // Set the RX Disable to stop even packets outside of RX_FILTER (ProbeRequest)
    // Note that scripts must turn off RX Disable to receive packets inc. ACKs.
//  REGW(devNum, F2_DIAG_SW, F2_DIAG_RX_DIS);
    ar5kInitData[pLibDev->ar5kInitIndex].pMacAPI->disableRx(devNum);

    // Writing to F2_BEACON will start timers. Hence it should be the last
    // register to be written.
    REGW(devNum, F2_BEACON, F2_BEACON_RESET_TSF | 0xffff);

    if (isOwl(pLibDev->swDevID)) {
//printf("pLibDev->libCfgParams.tx_chain_mask is %d\n", pLibDev->libCfgParams.tx_chain_mask);
//printf("pLibDev->libCfgParams.rx_chain_mask is %d\n", pLibDev->libCfgParams.rx_chain_mask);
        /* Setup Owl specific chain mask registers and swap bit before tx power/board init */
        if ((pLibDev->libCfgParams.rx_chain_mask == 0x5) && !(isOwl_2_Plus(pLibDev->macRev))) {
            /* 1.0 WAR */
            REGW(devNum, 0x99A4, 0x7);
            REGW(devNum, 0xA39C, 0x7);
        } else {
            REGW(devNum, 0x99A4, pLibDev->libCfgParams.rx_chain_mask & 0x7);
//			if(isSowl(pLibDev->macRev) && (pLibDev->libCfgParams.rx_chain_mask == 0x5)) {
//				REGW(devNum, 0xA39C, 0x7); // SOWL 1.0 WAR
//			} else {
                                REGW(devNum, 0xA39C, pLibDev->libCfgParams.rx_chain_mask & 0x7);
//                        }
        }
	if((pLibDev->swDevID == SW_DEVICE_ID_OWL)){
			/*
			* Set the self generated frame chain mask to match the tx setting
			* Note: 1 is OR'ed into tx chain mask register settings for diagnostic
			* chain masks of 0x4 and 0x2.
			*/
			REGW(devNum, 0x832c, (pLibDev->libCfgParams.tx_chain_mask | 0x1) & 0x7);
		} else {
			REGW(devNum, 0x832c, (pLibDev->libCfgParams.tx_chain_mask) & 0x7);
		}

        // Chain 0 & 2 enabled chain hacking
	if ((pLibDev->libCfgParams.tx_chain_mask == 0x5) || ((pLibDev->libCfgParams.tx_chain_mask == 0x4) && ((pLibDev->swDevID == SW_DEVICE_ID_OWL))) || (pLibDev->libCfgParams.rx_chain_mask == 0x5)) {
            // Set swap_alt_chain for 2 chain transmit via 0 and 2
            REGW(devNum, 0xA268, REGR(devNum, 0xA268) | 0x40);
        }
    }

#ifdef OWL_PB42
    if(isHowlAP(devNum))
      {
    // Enable IQ Swap Specific to Howl
       int rddata;
       rddata = REGR(devNum,0x9800+(666<<2));
       int wrdata;
       wrdata = rddata | 0x1;
       REGW(devNum,0x9800+(666<<2), wrdata);
	}
#endif

    // Setup board specific options
    if (pLibDev->eePromLoad) {
        assert(pLibDev->eepData.eepromChecked);
        if((((pLibDev->eepData.version >> 12) & 0xF) == 1) ||
           (((pLibDev->eepData.version >> 12) & 0xF) == 2))
        {
            REGW(devNum, PHY_BASE+(10<<2), (REGR(devNum, PHY_BASE+(10<<2)) & 0xFFFF00FF) |
                 (pLibDev->eepData.xlnaOn << 8));
            REGW(devNum, PHY_BASE+(13<<2), (pLibDev->eepData.xpaOff << 24) |
                 (pLibDev->eepData.xpaOff << 16) | (pLibDev->eepData.xpaOn << 8) |
                 pLibDev->eepData.xpaOn);
            REGW(devNum, PHY_BASE+(17<<2), (REGR(devNum, PHY_BASE+(17<<2)) & 0xFFFFC07F) |
                 ((pLibDev->eepData.antenna >> 1) & 0x3F80));
            REGW(devNum, PHY_BASE+(18<<2), (REGR(devNum, PHY_BASE+(18<<2)) & 0xFFFC0FFF) |
                 ((pLibDev->eepData.antenna << 10) & 0x3F000));
            REGW(devNum, PHY_BASE+(25<<2), (REGR(devNum, PHY_BASE+(25<<2)) & 0xFFF80FFF) |
                 ((pLibDev->eepData.thresh62 << 12) & 0x7F000));
            REGW(devNum, PHY_BASE+(68<<2), (REGR(devNum, PHY_BASE+(68<<2)) & 0xFFFFFFFC) |
                 (pLibDev->eepData.antenna & 0x3));
        }
    }

    // Setup the transmit power values for cards since 0x0[0-2]05
    pLibDev->freqForResetDevice = freq;
    initializeTransmitPower(devNum, freq, 0, NULL);
    setChannel(devNum, freq);

    /* set spur mitigation for COBRA and Eagle */
    if (pLibDev->mode == MODE_11G) {
        if(isCobra(pLibDev->swDevID) || isEagle(pLibDev->swDevID)) {
            printf("SNOOP: about to enter spur code\n");
            setSpurMitigation(devNum, freq);
        }
    }

	// Set spur mitigation for OWL spur
	if(isOwl(pLibDev->swDevID)     &&
       !isSowl(pLibDev->macRev)    &&
       !isMerlin(pLibDev->swDevID) &&
#ifdef OWL_PB42
	   !isHowlAP(devNum) &&
#endif
	   !isSowl_Emulation(pLibDev->macRev) &&
	   !isMerlin_Fowl_Emulation(pLibDev->macRev))
	{
		if(pLibDev->libCfgParams.ht40_enable) {
			setSpurMitigationOwl(devNum, freq);
		}
	}

        /* set spur mitigation for Merlin */
        if( isMerlin(pLibDev->swDevID) && (pLibDev->libCfgParams.spurChans[0] != 0) ) {
/*          printf(">>>>SNOOP: about to enter spur code on Merlin\n");*/
            setSpurMitigationMerlin(devNum, freq);
        }

   /* overwrite for quarter rate mode -- start */
   if ((pLibDev->swDevID & 0xff) == 0x0013) {
             /* tx power control measure time */
             /* wait after reset */
           if (!pwrParams.initialized[pLibDev->mode]) {
               pwrParams.initialized[pLibDev->mode] = TRUE;

                 //pwrParams.bb_active_to_receive[pLibDev->mode] = getFieldForMode(devNum, "bb_active_to_receive", pLibDev->mode, 0); // base mode vals
                 pwrParams.bb_active_to_receive[pLibDev->mode] = (REGR(devNum, 0x9914) & 0x3FFF);

                 pwrParams.rf_wait_I[pLibDev->mode] = getFieldForMode(devNum, "rf_wait_I", pLibDev->mode, 0);
                 pwrParams.rf_wait_S[pLibDev->mode] = getFieldForMode(devNum, "rf_wait_S", pLibDev->mode, 0);
                 pwrParams.rf_max_time[pLibDev->mode] = getFieldForMode(devNum, "rf_max_time", pLibDev->mode, 0);
           }

           temp1 = pwrParams.bb_active_to_receive[pLibDev->mode];
           temp2 = pwrParams.rf_wait_I[pLibDev->mode];
           temp3 = pwrParams.rf_wait_S[pLibDev->mode];
           temp4 = pwrParams.rf_max_time[pLibDev->mode];

           if (turbo == HALF_SPEED_MODE) {
                 temp1 = temp1 >> 1;
                 temp2 = temp2 << 1;
                 temp3 = temp3 << 1;
                 temp4 = temp4 + 1;
             } else if (turbo == QUARTER_SPEED_MODE) {
                 temp1 = temp1 >> 2;
                 temp2 = temp2 << 2;
                 temp3 = temp3 << 2;
                 temp4 = temp4 + 2;
             } else if (turbo == TURBO_ENABLE) {
                 temp1 = temp1 << 1;
                 temp2 = temp2 >> 1;
                 temp3 = temp3 >> 1;
             }

             temp1 = (temp1 > 0x3FFF) ? 0x3FFF : temp1;  // limit to max field size of bb_active_to_receive
             temp2 = (temp2 > 31) ? 31 : temp2;  // limit to max field size of rf_wait_I
             temp3 = (temp3 > 31) ? 31 : temp3;  // limit to max field size of rf_wait_S
             temp4 = (temp4 > 3)  ?  3 : temp4;  // limit to max field size of rf_max_time

             writeField(devNum, "bb_active_to_receive", temp1);
             writeField(devNum, "rf_wait_I", temp2);
             writeField(devNum, "rf_wait_S", temp3);
             writeField(devNum, "rf_max_time", temp4);
   }

    if (isOwl(pLibDev->swDevID)) {
        /* Modify bank6 bias current based on 2GHz Channel value to avoid transients - default is 2 */
#if (1) /* BAAt */
        /* merlin does not have rf regs.  Any tweaking to the new analog
         * registers during resetDevice will be done once there is specific
         * data for merlin
         */
    if( !isMerlin(pLibDev->swDevID) ) { /* BAAt */
        if (freq < 2412) {
            changeField(devNum, "rf_pwd_icsyndiv", 0);
        } else if (freq <= 2422) {
            changeField(devNum, "rf_pwd_icsyndiv", 1);
        } else {
            changeField(devNum, "rf_pwd_icsyndiv", 2);
        }
    }
#endif
    /* BAA: write "1" to bit 22 here to avoid disappearing card problem */
    if( isMerlin(pLibDev->swDevID) ) {
//        REGR(devNum, 0x4004);
        REGW(devNum, 0x4004, 0x0040073b);
    }
    else {
        REGW(devNum, 0x4004, 0x0040073f);
    }
		// For owl based devices when tx_mask is 2 and 4 we enable chain 0 on baseband but turn off chain 0 on fowl.
	if(pLibDev->swDevID == SW_DEVICE_ID_OWL) {
          /* Disable Fowl chain 0 for tx_mask of chain 1 or 2 only enabled */
          if ((pLibDev->libCfgParams.tx_chain_mask == 0x2) || (pLibDev->libCfgParams.tx_chain_mask == 0x4)) {
            A_CHAR *bank6names[] = {
                    "rf_pwd2G", "rf_pwdmix2_int", "rf_pwdpa2_int", "rf_pwdmix5_int", "rf_pwdvga5_int", "rf_pwdpa5_int",
                    "rf_pwd4Grx", "rf_pwd4Gtx", "rf_pwdfilt_int", "rf_pwdbbmix2_int", "rf_pwdbbmix5_int"
            };
            A_UINT32 numBk6Params = sizeof(bank6names)/sizeof(bank6names[0]), b;
            PARSE_FIELD_INFO bank6Fields[sizeof(bank6names)/sizeof(bank6names[0])];
            assert(numBk6Params == 11);

            /* Write Bank5 so following Bank6 update only changes Chain 0 */
            writeField(devNum, "rf_cs", 0);

            for (b = 0; b < numBk6Params; b++) {
                memcpy(bank6Fields[b].fieldName, bank6names[b], strlen(bank6names[b]) + 1);
                bank6Fields[b].valueString[0] = '1';
                bank6Fields[b].valueString[1] = '\0';
            }
            writeMultipleFieldsInRfBank(devNum, bank6Fields, numBk6Params, 6);

            /* Reset values in the reg file for next reset write of bank6 */
            for (b = 0; b < numBk6Params; b++) {
                bank6Fields[b].valueString[0] = '0';
                bank6Fields[b].valueString[1] = '\0';
            }
            changeMultipleFields(devNum, bank6Fields, numBk6Params);

            /* Reset Bank5 field to update all chains */
            writeField(devNum, "rf_cs", 3);

            /* Play with the XPA */
            REGW(devNum, 0xa3d8, (pLibDev->libCfgParams.tx_chain_mask << 1) | 0x01);
		  }
		}

#ifdef _IQV
		// bit 8 is set to 0, so that direct mapping is used for multistream data. (one of spatial mapping)
	    if (pLibDev->libCfgParams.ht40_enable) {
		    REGW(devNum, 0x9804, 0x2C4 | ((pLibDev->libCfgParams.extended_channel_op & EXT_CHANNEL_BIT_MASK) << 4) | ((pLibDev->libCfgParams.extended_channel_sep == 25) << 5));
		} else {
			REGW(devNum, 0x9804, 0x2C0);
		}
#else	
#if (1)        
            if( pLibDev->libCfgParams.ht40_enable ) {
                REGW(devNum, 0x9804, REGR(devNum, 0x9804) | 0x2C4 | 
                     ((pLibDev->libCfgParams.extended_channel_op & EXT_CHANNEL_BIT_MASK) << 4) | 
                     ((pLibDev->libCfgParams.extended_channel_sep == 25) << 5));
            }
            else {
                REGW(devNum, 0x9804, REGR(devNum, 0x9804) | 0x2C0);
            }
#else        
            if( pLibDev->libCfgParams.ht40_enable ) {
                A_UINT32 reg9804, a , b, c;
                reg9804 = REGR(devNum, 0x9804);
                a = (pLibDev->libCfgParams.extended_channel_op & EXT_CHANNEL_BIT_MASK) << 4;
                b = (pLibDev->libCfgParams.extended_channel_sep == 25) << 5;
                c = reg9804 | 0x2c4 | a | b;
                printf("reg9804(0x%x), a(0x%x), b(0x%x), c(0x%x)\n", reg9804, a, b, c);
                REGW(devNum, 0x9804, c); 
            }
            else {
                A_UINT32 reg9804, c;
                reg9804 = REGR(devNum, 0x9804);
                c = reg9804 | 0x2c0;
                printf("reg9804(0x%x), c(0x%x)\n", reg9804, c);
                REGW(devNum, 0x9804, c);
            }
#endif        
#endif // _IQV
    }

#if (0) /* BAA: causing problems in AP93 */
        if( isMerlin(pLibDev->swDevID) ) {
            Set11nMac2040(devNum);
        }
#endif        

   if (isOwl(pLibDev->swDevID) || pLibDev->swDevID == SW_DEVICE_ID_CONDOR || pLibDev->swDevID == 0x0018 || pLibDev->swDevID == 0x0019 ||
	   isCobra(pLibDev->swDevID) || (pLibDev->swDevID == SW_DEVICE_ID_DRAGON) || (isNala(pLibDev->swDevID))) {
       // add code for griffin here
   } else if(((pLibDev->swDevID & 0xff) == 0x0014)||((pLibDev->swDevID & 0xff) >= 0x0016)) { /* venice derby     if (turbo == QUARTER_SPEED_MODE)  */
         /* tx power control measure time */
         /* wait after reset */
           if (!pwrParams.initialized[pLibDev->mode]) {
               pwrParams.initialized[pLibDev->mode] = TRUE;

        //                 pwrParams.bb_active_to_receive[pLibDev->mode] = getFieldForMode(devNum, "bb_active_to_receive", pLibDev->mode, 0); // base mode vals
        //                 pwrParams.bb_tx_frame_to_tx_d_start[pLibDev->mode] = getFieldForMode(devNum, "bb_tx_frame_to_tx_d_start", pLibDev->mode, 0); // base mode vals

               // do a regread instead of getFieldForMode to capture EAR changes.
               pwrParams.bb_active_to_receive[pLibDev->mode] = (REGR(devNum, 0x9914) & 0x3FFF);
               pwrParams.bb_tx_frame_to_tx_d_start[pLibDev->mode] = (REGR(devNum, 0x9824) & 0xFF);

               pwrParams.rf_pd_period_a[pLibDev->mode] = getFieldForMode(devNum, "rf_pd_period_a", pLibDev->mode, 0); // base mode vals
               pwrParams.rf_pd_delay_a[pLibDev->mode] = getFieldForMode(devNum, "rf_pd_delay_a", pLibDev->mode, 0); // base mode vals
               pwrParams.rf_pd_period_xr[pLibDev->mode] = getFieldForMode(devNum, "rf_pd_period_xr", pLibDev->mode, 0); // base mode vals
               pwrParams.rf_pd_delay_xr[pLibDev->mode] = getFieldForMode(devNum, "rf_pd_delay_xr", pLibDev->mode, 0); // base mode vals

           }


           temp1 = pwrParams.bb_active_to_receive[pLibDev->mode];
           temp2 = pwrParams.rf_pd_period_a[pLibDev->mode];
           temp3 = pwrParams.rf_pd_delay_a[pLibDev->mode];
           temp4 = pwrParams.rf_pd_period_xr[pLibDev->mode];
           temp5 = pwrParams.rf_pd_delay_xr[pLibDev->mode];
           temp6 = pwrParams.bb_tx_frame_to_tx_d_start[pLibDev->mode];

         if (turbo == TURBO_ENABLE) {
             temp1 = temp1 << 1;
             temp2 = temp2 >> 1;
             temp3 = temp3 >> 1;
             temp4 = temp4 >> 1;
             temp5 = temp5 >> 1;
         } else if (turbo == HALF_SPEED_MODE) {
             temp1 = temp1 >> 1;
             temp2 = temp2 << 1;
             temp3 = temp3 << 1;
             temp4 = temp4 << 1;
             temp5 = temp5 << 1;
         } else if (turbo == QUARTER_SPEED_MODE) {
             temp1 = temp1 >> 2;
             temp2 = temp2 << 2;
             temp3 = temp3 << 2;
             temp4 = temp4 << 2;
             temp5 = temp5 << 2;
             temp6 -= ((temp6 > 5) ? 5 : 0);
         }

         temp1 = (temp1 > 0x3FFF) ? 0x3FFF : temp1;  // limit to max field size of bb_active_to_receive
         temp2 = (temp2 > 15) ? 15 : temp2;  // limit to max field size of rf_wait_I
         temp3 = (temp3 > 15) ? 15 : temp3;  // limit to max field size of rf_wait_S
         temp4 = (temp4 > 15) ? 15 : temp4;  // limit to max field size of rf_wait_I
         temp5 = (temp5 > 15) ? 15 : temp5;  // limit to max field size of rf_wait_S
         writeField(devNum, "bb_active_to_receive", temp1);
         writeField(devNum, "rf_pd_period_a", temp2);
         writeField(devNum, "rf_pd_delay_a", temp3);
         writeField(devNum, "rf_pd_period_xr", temp4);
         writeField(devNum, "rf_pd_delay_xr", temp5);
         writeField(devNum, "bb_tx_frame_to_tx_d_start", temp6);
    }

   /* overwrite for quarter rate mode -- end */

    // activate D2
    REGW(devNum, PHY_ACTIVE, PHY_ACTIVE_EN);
    mSleep(3);

    if( (pLibDev->swDevID == 0xf11b) && (pLibDev->mode == MODE_11B)) {
        REGW(devNum, 0xd87c,0x19);
        mSleep(4);
    }
    if( (pLibDev->swDevID == 0xf11b) && ((pLibDev->mode == MODE_11B) || (pLibDev->mode == MODE_11G))) {
        REGW(devNum, 0xd808,0x502);
        mSleep(2);
    }

    if (isGriffin_1_1(pLibDev->macRev))
    {
        REGW(devNum,0xa358, (REGR(devNum,0xa358) | 0x3));
//       REGW(devNum,0xa358, (REGR(devNum,0xa358) | 0x40000000));
    }

    /* set up GPIO 9 to switch the Band Select pin on the FEM
     * to have its VLNA input line control the appropriate LNA
     */;
    if(isMerlin(pLibDev->swDevID) && pLibDev->femBandSel) {
        if (pLibDev->mode == MODE_11G) {
            ar5416SetGpio(devNum,9,1); /* Set high to control 2GHz LNA */
        }
        else {
            ar5416SetGpio(devNum,9,0); /* Set low to control 5GHz LNA */
        }
    }

    //enableCal flag should have the bits set for CAL
    // calibrate it and poll the bit going to 0 for completion
#ifndef CUSTOMER_REL
    if (enableCal) {
#else   /* for external builds skip first 8 resets, this is work in progress */
    if (enableCal && (rst > 7)) {
#endif
		if((isMerlin(pLibDev->swDevID)) &&
		   (enableCal == (DO_NF_CAL | DO_OFSET_CAL))) 
        {
			A_INT32 num_nf_cal, kk, nf_force;
			if(freq > 4000) {
				num_nf_cal = 2;
				nf_force = -110;  // Force NF to -110dB on 5G channel
			}
			else {
				num_nf_cal = 1;
				nf_force = -110;  // Force NF to -110dB on 2G channel
			}
		    for( kk = 0; kk < num_nf_cal; kk++) {
            /********************************/
            /* enable Rx filter calibration */
            /********************************/
            /* first, clear the bb_off_pwdAdc bit */
            REGW(devNum, PHY_ADC_PARALLEL_CNTL,
                REGR(devNum, PHY_ADC_PARALLEL_CNTL) & PWD_ADC_CL_MASK);

            /* then enable the calibration */
            REGW(devNum, PHY_AGC_CONTROL,
                REGR(devNum, PHY_AGC_CONTROL) | (0x1<<16));

            /*********************************/
            /* perform DC offset calibration */
            /*********************************/
			REGW(devNum, PHY_AGC_CONTROL,
				REGR(devNum, PHY_AGC_CONTROL) | DO_OFSET_CAL);
			// noise cal takes a long time to complete in simulation
			// need not wait for the noise cal to complete

				//printf("DC Offset Calibration is Running!\n");
	        // Do remote check
		    if (pLibDev->devMap.remoteLib) {
			    timeout = 32000;
				i = pLibDev->devMap.r_calCheck(devNum, DO_OFSET_CAL, timeout);
			} else {
	            timeout = 8;
		        for (i = 0; i < timeout; i++)
			    {
				    if ((REGR(devNum, PHY_AGC_CONTROL) & DO_OFSET_CAL) == 0 ) {
					    break;
					}
					mSleep(1);
				}
			}

			if(i >= timeout) {
	            //mError(EIO, "Device Number %d:resetDevice: device failed to finish offset and/or noisefloor calibration in 10 ms\n");
#ifndef CUSTOMER_REL
	            printf("resetDevice : Device Number %d:Didn't complete DC offset cal after timeout(%d) on channel %d but keep going anyway\n", devNum, timeout, pLibDev->freqForResetDevice);
#endif
			}

            /*********************************/
            /* disable Rx filter calibration */
            /*********************************/
            /* first, set the bb_off_pwdAdc bit */
            REGW(devNum, PHY_ADC_PARALLEL_CNTL,
                REGR(devNum, PHY_ADC_PARALLEL_CNTL) | ~PWD_ADC_CL_MASK);

            /* then disable the calibration */
            REGW(devNum, PHY_AGC_CONTROL,
                REGR(devNum, PHY_AGC_CONTROL) & ~(0x1<<16));

            /***********************************/
            /* perform noise floor calibration */
            /***********************************/
                    REGW(devNum, PHY_AGC_CONTROL,
                         REGR(devNum, PHY_AGC_CONTROL) | DO_NF_CAL);
                //printf("NF Calibration is Running!\n");
                    // noise cal takes a long time to complete in simulation
                    // need not wait for the noise cal to complete

                    // Do remote check
                    if( pLibDev->devMap.remoteLib ) {
                        timeout = 32000;
                        i = pLibDev->devMap.r_calCheck(devNum, DO_NF_CAL, timeout);
                    }
                    else {
                        timeout = 8;
                        for( i = 0; i < timeout; i++ ) {
                            if( (REGR(devNum, PHY_AGC_CONTROL) & DO_NF_CAL) == 0 ) {
                                break;
                            }
                            mSleep(1);
                        }
                    }

                    if( i >= timeout ) {
                        //mError(EIO, "Device Number %d:resetDevice: device failed to finish offset and/or noisefloor calibration in 10 ms\n");
#ifndef CUSTOMER_REL
                        printf("resetDevice : Device Number %d:Didn't complete NF cal after timeout(%d) on channel %d but keep going anyway\n", devNum, timeout, pLibDev->freqForResetDevice);
#endif
                    }
                /* read/display the NF after calibration */
                if(num_nf_cal == 2) {
					if(!ReadNF(devNum)) {
						break;           
					} else {
  						if(kk == (num_nf_cal - 1)) {
							for (i = 0; i < 2; i ++) {
								temp1 = REGR(devNum, 0x9864 + (i * 0x1000));
								temp1 &= 0xFFFFFE00;
								REGW(devNum, 0x9864 + (i * 0x1000), temp1 | ((nf_force << 1) & 0x1ff));
   					 			temp1 = REGR(devNum, 0x99bc + (i * 0x1000));
								temp1 &= 0xFFFFFE00;
								REGW(devNum, 0x99bc + (i * 0x1000), temp1 | ((nf_force << 1) & 0x1ff));

							}
							temp1 = REGR(devNum, PHY_AGC_CONTROL);
							temp1 = temp1 & 0xFFFD7ffD;
							REGW(devNum, PHY_AGC_CONTROL, temp1 | (0x0 << 17) | (0x0 << 15) | DO_NF_CAL);
						    // Do remote check
							if( pLibDev->devMap.remoteLib ) {
								timeout = 32000;
								i = pLibDev->devMap.r_calCheck(devNum, DO_NF_CAL, timeout);
							}
							else {
								timeout = 8;
								for( i = 0; i < timeout; i++ ) {
									if( (REGR(devNum, PHY_AGC_CONTROL) & DO_NF_CAL) == 0 ) {
										break;
									}
									mSleep(1);
								}
							}
							if( i >= timeout ) {
								//mError(EIO, "Device Number %d:resetDevice: device failed to finish offset and/or noisefloor calibration in 10 ms\n");
#ifndef CUSTOMER_REL 
								printf("resetDevice : Device Number %d:Didn't complete NF cal after timeout(%d) on channel %d but keep going anyway\n", devNum, timeout, pLibDev->freqForResetDevice);
#endif
							}

							/* Restore maxCCAPower register parameter again so    */
							/* that we're not capped by the median we just        */
							/* loaded.  This will be initial (and max) value      */
							/* of next noise floor calibration the baseband does. */
     
							for (i = 0; i < 2; i ++) {
					 			temp1 = REGR(devNum, 0x9860 + (i * 0x1000));
								temp1 &= 0xFFFFFE00;
								REGW(devNum, 0x9864 + (i * 0x1000), temp1);
 					 			temp1 = REGR(devNum, 0x99bc + (i * 0x1000));
								temp1 &= 0xFFFFFE00;
								REGW(devNum, 0x99bc + (i * 0x1000), temp1);

							}
							temp1 = REGR(devNum, PHY_AGC_CONTROL);
							temp1 = temp1 & 0xfffD7fff;
							REGW(devNum, PHY_AGC_CONTROL, temp1 | (0x0 << 17) | (0x1 << 15));
						}
					}
				}
            }
		} else { // Non-Merlin
	        REGW(devNum, PHY_AGC_CONTROL,
		        REGR(devNum, PHY_AGC_CONTROL) | enableCal);
			// noise cal takes a long time to complete in simulation
			// need not wait for the noise cal to complete

	        // Do remote check
		    if (pLibDev->devMap.remoteLib) {
			    timeout = 32000;
				i = pLibDev->devMap.r_calCheck(devNum, enableCal, timeout);
			} else {
	            timeout = 8;
		        for (i = 0; i < timeout; i++)
			    {
				    if ((REGR(devNum, PHY_AGC_CONTROL) & (enableCal)) == 0 ) {
					    break;
				    }
					mSleep(1);
			 }
			}
			if(i >= timeout) {
	            //mError(EIO, "Device Number %d:resetDevice: device failed to finish offset and/or noisefloor calibration in 10 ms\n");
#ifndef CUSTOMER_REL
		        printf("resetDevice : Device Number %d:Didn't complete cal after timeout(%d) on channel %d but keep going anyway\n", devNum, timeout, pLibDev->freqForResetDevice);
#endif
			}
		}

#if (FOR_REMOVAL) 
//++JC++
        // For Griffin alone - To work around Carrier Leak Problem
        if(isGriffin(pLibDev->swDevID) || isEagle(pLibDev->swDevID)) {
            if (isGriffin_1_0(pLibDev->macRev)) {
                griffin_cl_cal(devNum);
            } else {
                  REGW(devNum,0xa358, (REGR(devNum,0xa358) | 0x3));
           }
        }
        // End of workaround for griffing Carrier Leak Problem
//++JC++
#endif

    }
#ifdef CUSTOMER_REL
    rst++;
#endif

    } // end of else (use_init...

    if((isGriffin(pLibDev->swDevID) || isEagle(pLibDev->swDevID)) && (!pLibDev->libCfgParams.noUnlockEeprom)) {
	  if(!isNala(pLibDev->swDevID)) {
        if(isCondor(pLibDev->swDevID)) {
        //need to remove the write protection from eeprom
            REGW(devNum, F2_GPIOCR, REGR(devNum, F2_GPIOCR) | F2_GPIOCR_3_CR_A);
            gpioData = 0x1 << 3;
            REGW(devNum, F2_GPIODO, REGR(devNum, F2_GPIODO) & ~gpioData);
        }
        //need to remove the write protection from eeprom
        REGW(devNum, F2_GPIOCR, REGR(devNum, F2_GPIOCR) | F2_GPIOCR_4_CR_A);
        gpioData = 0x1 << 4;
        REGW(devNum, F2_GPIODO, REGR(devNum, F2_GPIODO) & ~gpioData);
	  }
    }

    pLibDev->devState = RESET_STATE;

    if (isFalcon(devNum) || isOwl(pLibDev->swDevID)) {
        for (i = 0; i < (isOwl(pLibDev->swDevID) ? 3 : 2); i++) {
            temp1 = REGR(devNum, (0x9800 + (i * 0x1000) + (25<<2)));
            temp2 = (temp1 >> 19) & 0x1ff;

			if(isMerlin(pLibDev->swDevID)) { // bit location is changed in Merlin
				temp2 = (temp1 >> 20) & 0x1ff;
			}

			if ((temp2 & 0x100) != 0x0) {
                nf_ctl = 0 - ((temp2 ^ 0x1ff) + 1);
            }
            temp4 = 0;
			if (isOwl(pLibDev->swDevID)) {
				if((isSowl_Emulation(pLibDev->macRev)) || (isMerlin_Fowl_Emulation(pLibDev->macRev))) {
					printf ("channel = %d [ht40=%d] ", pLibDev->freqForResetDevice, pLibDev->libCfgParams.ht40_enable);
				}
				temp3 = REGR(devNum, (0x9800 + (i * 0x1000) + (111<<2)));
				temp4 = (temp3 >> 23) & 0x1ff;

			    if(isMerlin(pLibDev->swDevID)) { // bit location is changed in Merlin
				   temp4 = (temp3 >> 16) & 0x1ff;
				}

				if ((temp4 & 0x100) != 0x0) {
					nf_ext = 0 - ((temp4 ^ 0x1ff) + 1);
				}
			}
			if((isSowl_Emulation(pLibDev->macRev)) || (isMerlin_Fowl_Emulation(pLibDev->macRev))				) {
				printf("NF for Chain%d = ctl %d, ext %d \n", i, nf_ctl, nf_ext);
			}
        }
        //printf("\n");
    }

    if (isOwl(pLibDev->swDevID) && (pLibDev->libCfgParams.rx_chain_mask == 0x5) && !(isOwl_2_Plus(pLibDev->macRev))) {
        /* TODO 1.0 WAR */
        REGW(devNum, 0x99A4, 0x5);
    }

    // DMA size fix for Nala
     if(isNala(pLibDev->swDevID)) {
     	REGW(devNum, F2_RXCFG, F2_DMASIZE_16B);
     }


#ifndef __ATH_DJGPPDOS__
    // ART ANI routine
    tweakArtAni(devNum, prev_freq, pLibDev->freqForResetDevice);
#endif
#ifndef CUSTOMER_REL
    resetDevice_end=milliTime();
//printf("SNOOP::exit resetDevice:Time taken = %dms\n", (resetDevice_end-resetDevice_start));
#endif
}

/**************************************************************************
 * testLib -  test function  that check for software interrupt (added to
 *            create dktest functionality).  Returns TRUE if pass, FALSE
 *            otherwise
 *
 */
MANLIB_API A_BOOL testLib
(
 A_UINT32 devNum,
 A_UINT32 timeout
)
{
    ISR_EVENT   event;
    LIB_DEV_INFO *pLibDev = gLibInfo.pLibDevArray[devNum];
    A_UINT32    i;
    A_UINT32    saveRegValue;
    A_UINT32    testRegValue;
    A_UINT32    testValues[] = {0xffffffff, 0x00000000, 0xa5a5a5a5};
    A_UINT32    mask;

    if (checkDevNum(devNum) == FALSE) {
        mError(devNum, EINVAL, "Device Number %d:testLib\n", devNum);
        printf("Device Number %d:testLib\n", devNum);
        return(FALSE);
    }
    if (pLibDev->devState < RESET_STATE) {
        mError(devNum, EILSEQ, "Device Number %d:testLib: Device should be out of Reset before testing library\n", devNum);
        printf("Device Number %d:testLib: Device should be out of Reset before testing library\n", devNum);
        return(FALSE);
    }

    // interrupt test not implemented for falcon yet
    // ISR, IMR, IER are different
    if (isFalcon(devNum)) {
        return(TRUE);
    }

    if (isGriffin(pLibDev->swDevID)) {
        if (isGriffin_1_0(pLibDev->macRev) || isGriffin_1_1(pLibDev->macRev)) {
            printf("******************\n\n");
            printf(" register test disabled for griffin on mb51. \n");
            printf("    enable it when griffin is fixed\n");
            printf("******************\n\n");
            return(TRUE);
        }
    }

    //enable software interrupt
    REGW(devNum, F2_IMR, REGR(devNum, F2_IMR) | F2_IMR_SWI);
    REGW(devNum, 0xA0, REGR(devNum, 0xA0) | F2_IMR_SWI);

#ifndef SIM
    //clear ISR before enable
    REGR(devNum, F2_ISR);
#endif
	REGW(devNum, 0x20, 0);
    REGR(devNum, 0x80);
    REGW(devNum, F2_IER, F2_IER_ENABLE);

    //create the software interrupt
    REGW(devNum, F2_CR, REGR(devNum, F2_CR) | F2_CR_SWI);
    //wait for software interrupt
    for (i = 0; i < timeout; i++)
    {
        event = pLibDev->devMap.getISREvent(devNum);
        if (event.valid)
        {
            //see if it is the SW interrupt
            if (event.ISRValue & F2_ISR_SWI) {
                //This is the event we are waiting for
                break;
            }
        }
        mSleep(1);
    }

    if (i == timeout) {
        mError(devNum, 21, "Device Number %d:Error: Software interrupt not received\n", devNum);
        return(FALSE);
    }

    //perform a simple register test
    saveRegValue = REGR(devNum, F2_STA_ID0);
    for(i = 0; i < sizeof(testValues)/sizeof(A_UINT32); i++) {
        REGW(devNum, F2_STA_ID0, testValues[i]);
        testRegValue = REGR(devNum, F2_STA_ID0);
        if(testRegValue != testValues[i]) {
            mError(devNum, 71, "Device Number %d:Failed simple register test, wrote %x, read %x\n", devNum, testValues[i], testRegValue);
            REGW(devNum, F2_STA_ID0, saveRegValue);
            return(FALSE);
        }
    }
    //do a walking 1 pattern test
    mask = 0x01;
    for(i = 0; i < 32; i++) {
        REGW(devNum, F2_STA_ID0, mask);
        testRegValue = REGR(devNum, F2_STA_ID0);
        if(testRegValue != mask) {
            mError(devNum, 71, "Failed simple register test, wrote %x, read %x\n", mask, testRegValue);
            REGW(devNum, F2_STA_ID0, saveRegValue);
            return(FALSE);
        }
        mask = mask << 1;
    }

    REGW(devNum, F2_STA_ID0, saveRegValue);
    return(TRUE);
}




/**************************************************************************
 * checkRegs - Perform register tests to various domains of the AR5K
 *
 */
MANLIB_API A_UINT32 checkRegs
(
 A_UINT32 devNum
)
{
    LIB_DEV_INFO *pLibDev = gLibInfo.pLibDevArray[devNum];
    A_UINT32 failures = 0;
    A_UINT32 addr, i, loop, wrData, rdData, pattern;
    A_UINT32 regAddr[2] = {F2_STA_ID0, PHY_BASE+(8 << 2)};
    A_UINT32 regHold[2];
    A_UINT32 patternData[4] = {0x55555555, 0xaaaaaaaa, 0x66666666, 0x99999999};

    if(pLibDev->swDevID == 0x0007) {
        // Test PHY & MAC registers
        for(i = 0; i < 2; i++) {
            addr = regAddr[i];
            regHold[i] = REGR(devNum, addr);
            for (loop=0; loop < 0x10000; loop++) {

                wrData =  (loop << 16) | loop;
                REGW(devNum, addr, wrData);
                rdData = REGR(devNum, addr);
                if (rdData != wrData) {
                    failures++;
                }
            }

            for (pattern=0; pattern<4; pattern++) {

                wrData = patternData[pattern];

                REGW(devNum, addr, wrData);
                rdData = REGR(devNum, addr);

                if (wrData != rdData) {
                    failures++;
                }
            }
        }

        // Disable AGC to A2 traffic
        borrowAnalogAccess(devNum);

        // Test Radio Register
        for (loop=0; loop < 10000; loop++) {
            wrData = loop & 0x3f;

            // ------ DAC 1 Write -------
            REGW(devNum, (PHY_BASE+(0x35<<2)), (reverseBits(wrData, 6) << 16) | (reverseBits(wrData, 6) << 8) | 0x24);
            REGW(devNum, (PHY_BASE+(0x34<<2)), 0x0);
            REGW(devNum, (PHY_BASE+(0x34<<2)), 0x00120017);

            for (i=0; i<18; i++) {
                REGW(devNum, PHY_BASE+(0x20<<2), 0x00010000);
            }

            rdData = reverseBits((REGR(devNum, PHY_BASE+(256<<2)) >> 26) & 0x3f, 6);

            if (rdData != wrData) {
                    failures++;
            }

            REGW(devNum, (PHY_BASE+(0x34<<2)), 0x0);
            REGW(devNum, (PHY_BASE+(0x34<<2)), 0x00110017);

            for (i=0; i<18; i++) {
                REGW(devNum, PHY_BASE+(0x20<<2), 0x00010000);
            }

            rdData = reverseBits((REGR(devNum, PHY_BASE+(256<<2)) >> 26) & 0x3f, 6);

            if (rdData != wrData) {
                    failures++;
            }
        }
        REGW(devNum, (PHY_BASE+(0x34<<2)), 0x14);
        for(i = 0; i < 2; i++) {
            REGW(devNum, regAddr[i], regHold[i]);
        }

        returnAnalogAccess(devNum);
        gLibInfo.pLibDevArray[devNum]->devState = INIT_STATE;
    }
    else {
        mError(devNum, EIO, "Device Number %d:CheckRegs not implemented for this deviceID\n", devNum);
    }

    return failures;
}

 /**
  * - Function Name: changeChannel
  * - Description  : wrapper for fast channel change routines
  * - Arguments
  *     - devNum: device id
  *     - freq  : new channel
  * - Returns      : 
  ******************************************************************************/
MANLIB_API void changeChannel(
 A_UINT32 devNum,
 A_UINT32 freq )
{
    LIB_DEV_INFO *pLibDev = gLibInfo.pLibDevArray[devNum];

    if (checkDevNum(devNum) == FALSE) {
        mError(devNum, EINVAL, "Device Number %d:changeChannel\n", devNum);
        return;
    }
    if(isOwl(pLibDev->swDevID)) {
        ar5416ChannelChangeTx(devNum, freq);
        return;
    }
}
/* changeChannel */

#if (0) 
/**
 * - Function Name: ar5416ChannelChange_
 * - Description  : fast channel change without all the overhead of resetting. 
 *                  Tested on ar928x chipsets only.
 * - Arguments
 *     -      
 * - Returns      : 
 ******************************************************************************/
MANLIB_API A_BOOL
ar5416ChannelChange_(struct ath_hal *ah, HAL_CHANNEL *chan,
                    HAL_CHANNEL_INTERNAL *ichan, HAL_HT_MACMODE macmode) {
    u_int32_t synthDelay, qnum;
    struct ath_hal_5416 *ahp = AH5416(ah);

    ar5416GetNf(ah, ichan);

    /* TX must be stopped by now */
    for( qnum = 0; qnum < AR_NUM_QCU; qnum++ ) {
        if( ar5416NumTxPending(ah, qnum) ) {
            HALDEBUG(ah, "%s: Transmit frames pending on queue %d\n", __func__, qnum);
            HALASSERT(0);
            return(AH_FALSE);
        }
    }

    /*
     * Kill last Baseband Rx Frame - Request analog bus grant
     */
    OS_REG_WRITE(ah, AR_PHY_RFBUS_REQ, AR_PHY_RFBUS_REQ_EN);
    if( !ath_hal_wait(ah, AR_PHY_RFBUS_GRANT, AR_PHY_RFBUS_GRANT_EN,
                      AR_PHY_RFBUS_GRANT_EN) ) {
        HALDEBUG(ah, "%s: Could not kill baseband RX\n", __func__);
        return(AH_FALSE);
    }

    /* Setup 11n MAC/Phy mode registers */
    ar5416Set11nRegs(ah, chan, macmode);

    /*
     * Change the synth
     */
    if( !ahp->ah_rfHal.setChannel(ah, ichan) ) {
        HALDEBUG(ah, "%s: failed to set channel\n", __func__);
        return(AH_FALSE);
    }

    /*
     * Setup the transmit power values.
     *
     * After the public to private hal channel mapping, ichan contains the
     * valid regulatory power value.
     * ath_hal_getctl and ath_hal_getantennaallowed look up ichan from chan.
     */
    if( ar5416EepromSetTransmitPower(ah, &ahp->ah_eeprom, ichan,
                                     ath_hal_getctl(ah,chan), ath_hal_getantennaallowed(ah, chan),
                                     ichan->maxRegTxPower * 2,
                                     AH_MIN(MAX_RATE_POWER, AH_PRIVATE(ah)->ah_powerLimit)) != HAL_OK ) {
        HALDEBUG(ah, "%s: error init'ing transmit power\n", __func__);
        return(AH_FALSE);
    }

    /*
     * Wait for the frequency synth to settle (synth goes on via PHY_ACTIVE_EN).
     * Read the phy active delay register. Value is in 100ns increments.
     */
    synthDelay = OS_REG_READ(ah, AR_PHY_RX_DELAY) & AR_PHY_RX_DELAY_DELAY;
    if( IS_CHAN_CCK(chan) ) {
        synthDelay = (4 * synthDelay) / 22;
    }
    else {
        synthDelay /= 10;
    }

    if( ar5416Get11nHwPlatform(ah) == HAL_MAC_BB_EMU ) {
        OS_DELAY(synthDelay + EMU_HAINAN_BASE_ACTIVATE_DELAY);
    }
    else {
        OS_DELAY(synthDelay + BASE_ACTIVATE_DELAY);
    }

    if( !ichan->oneTimeCalsDone ) {
        /*
         * Start offset and carrier leak cals
         */
    }

    /*
     * Write spur immunity and delta slope for OFDM enabled modes (A, G, Turbo)
     */
    if( IS_CHAN_OFDM(chan)|| IS_CHAN_HT(chan) ) {
        //ar5416SetSpurMitigation(pDev, pChval);
        ar5416SetDeltaSlope(ah, ichan);
        ar5416_spur_mitigate(ah, chan);
    }

    /*
     * Start Noise Floor Cal
     */
    OS_REG_SET_BIT(ah, AR_PHY_AGC_CONTROL, AR_PHY_AGC_CONTROL_NF);

    if( !ichan->oneTimeCalsDone ) {
        /*
         * wait for end of offset and carrier leak cals
         */
        ichan->oneTimeCalsDone = AH_TRUE;
    }

    /*
     * Release the RFBus Grant
     */
    OS_REG_WRITE(ah, AR_PHY_RFBUS_REQ, 0);

    return(AH_TRUE);
}
/* ar5416ChannelChange_ */
#endif

/** 
 * - Function Name: Set11nMac2040
 * - Description  : configures the chip for 20/40 operation. The joined rx 
 *                  clear bit will force the MAC to perform a logical AND on
 *                  the rx clear of the control and extension channels when
 *                  performing transmit countdown for channel contention.  This
 *                  is necessary for cases where there are overlapping cells in
 *                  a network that share the extension channel but not the
 *                  control channel.  Without setting the joined rx clear bit a
 *                  node will transmit an HT40 frame if the control channel
 *                  alone is idle with no regard to activity on the extension
 *                  channel.  Setting the joined rx clear bit will prevent both
 *                  HT20 and HT40 frames from transmitting if there is activity
 *                  on either the control or extension channels.
 * - Arguments
 *     - 
 * - Returns      :                          
 *******************************************************************************/
#define AR_2040_MODE            (0x8318)
#define AR_2040_JOINED_RX_CLEAR (0x00000001)
void Set11nMac2040( A_UINT32 devNum ) {
    LIB_DEV_INFO *pLibDev = gLibInfo.pLibDevArray[devNum];
    if( pLibDev->libCfgParams.ht40_enable ) {
        REGW(devNum, AR_2040_MODE, (REGR(devNum, AR_2040_MODE) | AR_2040_JOINED_RX_CLEAR));
    }
    else {
        REGW(devNum, AR_2040_MODE, (REGR(devNum, AR_2040_MODE)& ~AR_2040_JOINED_RX_CLEAR));
    }
}
/* Set11nMac2040 */

/** 
 * - Function Name: ForceChannelIdle
 * - Description  : Forces the BB to disregard the transmitting medium.  This 
 *                  function ensures that the noise floor values will be
 *                  disregarded when transmitting; hence, there is no chance
 *                  that the BB will stop transmitting because it thinks the
 *                  medium is busy given outdated noise floor values. This
 *                  function can come in handy during a fast channel change
 *                  that does not perform the NF cal, for example.
 * - Arguments
 *     - devNum: device id
 *     - forceIdle: flag to set or not set the BB.
 * - Returns      :                              
 *******************************************************************************/
#define FORCE_CHAN_IDLE_HIGH (0x00200000)
void ForceChannelIdle( A_UINT32 devNum, A_UINT32 forceIdle ) {

    if(forceIdle) {
        REGW(devNum, F2_DIAG_SW, (REGR(devNum, F2_DIAG_SW) | FORCE_CHAN_IDLE_HIGH));
    }
    else {
        REGW(devNum, F2_DIAG_SW, (REGR(devNum, F2_DIAG_SW) & ~FORCE_CHAN_IDLE_HIGH));
    }
}
/* ForceChannelIdle */

/**
 * - Function Name: ar5416ChannelChangeTx
 * - Description  : Fast channel change for cases where the card is 
 *                  transmitting and only the transmitting channel needs to be
 *                  changed.  This is the case during calibration, for
 *                  instance.  This routine does not perform all the
 *                  calibrations of a regular channel change, only the DC
 *                  Offset cal since the carrier leak cal is tied to it. This
 *                  routine does not carry all the overhead of resetting the
 *                  hw.  It is intended to provide the fastest channel change
 *                  possible under the circumstances.
 * - Arguments
 *     -      
 * - Returns      : 
 ******************************************************************************/
MANLIB_API A_BOOL
ar5416ChannelChangeTx(A_UINT32 devNum, A_UINT32 freq) {
    double fclk = 40;

    /* Ensure BB won't lock up by forcing the channel to idle */
    ForceChannelIdle(devNum, 1);

    /* Request analog bus access */
    borrowAnalogAccess(devNum);

    /* Setup 11n MAC/Phy mode registers */
    SetupStreamMapping(devNum);
    //Set11nMac2040(devNum);

    /* Change the synthesizer */
    setChannel(devNum, freq);

    /* Set up the tx power values */
    initializeTransmitPower(devNum, freq, 0, NULL);

    /* Recalculate delta slope coefficients */
    calculateDeltaCoeffs(devNum, fclk, freq);

    /* Run spur mitigation */
    RunSpurMitigation( devNum, freq );

    /* perform DC offset calibration */
    RunDcOffsetCal(devNum);

    /* Release the Rf bus */
    returnAnalogAccess(devNum);

    /* Put BB back to normal */
    ForceChannelIdle(devNum, 0);

    return (TRUE);
}
/* ar5416ChannelChangeTx */


/**************************************************************************
 * setChannel - Perform the algorithm to change the channel
 *
 */
A_BOOL setChannel
(
 A_UINT32 devNum,
 A_UINT32 freq      // New channel
)
{
    LIB_DEV_INFO *pLibDev = gLibInfo.pLibDevArray[devNum];
    A_UINT32 i = pLibDev->ar5kInitIndex;

    if (ar5kInitData[i].pRfAPI->setChannel == NULL)
    {
        return(0);
    }
    return (ar5kInitData[i].pRfAPI->setChannel(devNum, freq));

}

#define ENABLE_SPUR_RSSI          1
#define ENABLE_SPUR_FILTER        1

#define USE_SPUR_FILT_IN_AGC      1

#define USE_SPUR_FILT_IN_SELFCOR_MERLIN  0
#define USE_SPUR_FILT_IN_SELFCOR_OWL  1

#define MASK_RATE_CNTL            0xff // enable vit pucture per rate, 8 bits, lsb is low rate
#define ENABLE_CHAN_MASK          1
#define ENABLE_PILOT_MASK         1

#define ENABLE_MASK_PPM_MERLIN    0    // bins move with freq offset
#define ENABLE_MASK_PPM_OWL       1    // bins move with freq offset

#define MASK_RATE_SELECT          0xff // use mask1 or mask2, one per rate
#define ENABLE_VIT_SPUR_RSSI      1

#define SPUR_RSSI_THRESH          40


/**************************************************************
 * setSpurMitigationMerlin
 * VN - Jan 15, 2007
 * Apply Spur Immunity to Merlin Boards that need it.
 * Applies only to OFDM RX operation.
 */
void setSpurMitigationMerlin(A_UINT32 devNum, A_UINT32 freq)
{
    LIB_DEV_INFO *pLibDev = gLibInfo.pLibDevArray[devNum];
	A_UINT16 i, cnt, spurFlag;
    A_UINT32 spurredChans[5];  // Channels affected by spur
    A_UINT16 totalSpurredChans = 0;
	A_UINT32 spur;
	A_UINT32 addr, newVal, oldVal, tmp_mask;
	double bin, bin_old, bb_spur_off, denom;
	A_INT32 bb_spur, spur_miti_done;
	double upper, lower, upper_old, lower_old, cur_bin;
	A_UINT32 spur_delta_phase;
	A_UINT32 spur_freq_sd;
	double norm, limit;
	A_UINT32 pilot_mask_1, pilot_mask_2, pilot_mask_3, pilot_mask_4, bp;
    	A_UINT32 chan_mask_1, chan_mask_2, chan_mask_3, chan_mask_4;
	A_INT16 cur_vit_mask;
	A_UINT16 mask_m[62], mask_p[62], mask_amt;
    	A_BOOL spur_subchannel_sd;
	// spur affected channels
#if (0)
    totalSpurredChans = 4;
    spurredChans[0] = 2400;
    spurredChans[1] = 2440;
    spurredChans[2] = 2480;
	spurredChans[2] = 2464;
#else
    memcpy(spurredChans,pLibDev->libCfgParams.spurChans, 5*sizeof(A_UINT32));
    for(i=0;i<5;i++) {
/*       printf("spurredChans[%d](%d)\n",i,spurredChans[i]); */
        if(0 != spurredChans[i]) {
            totalSpurredChans++;
        }
    }
    if(!totalSpurredChans) {
        return;
    }
#endif
	spurFlag = 1;
	//	if(spurFlag == 0)
	//		return;

/*  printf("SNOOP:: Running spur mitigation for MERLIN, channel = %d, ht40 = %d\n", freq, pLibDev->libCfgParams.ht40_enable);*/

	spur_miti_done = 0;

	for(i=0; i<totalSpurredChans; i++)
	{
        	spur = spurredChans[i];
                bb_spur = spur - freq;
//        	printf("Spur is %d\n", spur);

		// if base band spur is inside of the range +/- 18.750 apply spur mitigation
		// else no action needed
            if(pLibDev->libCfgParams.ht40_enable) {
				limit = 18.75;
			} else {
				limit = 9.38;
			}

        	if (((bb_spur > -limit) && (bb_spur < limit)) && (spur_miti_done == 0))
			{
				// Disable MRC CCK in spur channel.
				REGW(devNum, 0xa22c, REGR(devNum, 0xa22c) & ~(1 << 6));
				//printf("Reg 0xa22c is 0x%x\n", REGR(devNum, 0xa22c));

       			bin = (bb_spur/0.3125);
				bin_old = bin;
				spur_miti_done = 1;
				// logic to pick the channel (ctrl or extn) to cancel the spur
				if(pLibDev->libCfgParams.ht40_enable) {
			        norm = 80;
					if (bb_spur < 0)  {
						spur_subchannel_sd = 1;
						bb_spur_off = bb_spur + 10;
					} else {
						spur_subchannel_sd = 0;
						bb_spur_off = bb_spur - 10;
					}
				} else {
					norm = 40;
					//for static mode
					spur_subchannel_sd = 0; //don't care
					bb_spur_off = bb_spur; //center freq same as center of subchannel
				}
//			printf("norm is %d\n", norm);
//		printf("chan:%d    bin:%5.1f  bb_spur:%3d   spur:%d\n",
//		        freq, bin, bb_spur, spur);

		// modify bb_ch0_enable_spur_rssi
				addr = 0x9920;
                oldVal = REGR(devNum, addr);
                newVal = oldVal | (ENABLE_SPUR_RSSI  <<31)
                            | (ENABLE_SPUR_FILTER<<30)
                            | (ENABLE_CHAN_MASK  <<29)
                            | (ENABLE_PILOT_MASK <<28);
				REGW(devNum, addr, newVal);
//		 printf("0x%04x: 0x%08x\n", addr, newVal);

		// modify bb_spur_rssi_thresh
                addr = 0x994c;
                newVal = (MASK_RATE_CNTL       <<18)
                         | (ENABLE_MASK_PPM_MERLIN   <<17)
                         | (MASK_RATE_SELECT     <<9)
                         | (ENABLE_VIT_SPUR_RSSI <<8)
                         | (SPUR_RSSI_THRESH     <<0);
				REGW(devNum, addr, newVal);
//		 printf("0x%04x: 0x%08x\n", addr, newVal);

                // $spur_delta_phase = ($bb_spur* (10**6) * (25/2*(10**9)) * (2**21)) & 0xfffff;
                // $spur_freq_sd     = (($bb_spur_off/44) * (2**11)) & 0x3ff;

                spur_delta_phase = (int)((bb_spur/norm) * pow(2.0, 21.0)) & 0xfffff;

		// TEST ONLY
		//spur_delta_phase = 0xfffff;
				denom = 44;
				if(pLibDev->mode == MODE_11A)
				{
					denom = 40;
				}
                spur_freq_sd     = (int)(((bb_spur_off/denom) * pow(2.0, 11.0))) & 0x3ff;
//		printf("bb_spur %d bb_spur_off %f\n", bb_spur, bb_spur_off);
/*       printf("spur_freq_sd %x \tspur_delta_phase %x\n", spur_freq_sd, spur_delta_phase);*/
//          printf("%x %x\n", spur_freq_sd, spur_delta_phase);

                // modify bb_spur_delta_phase
				addr = 0x99a0;
                newVal = (USE_SPUR_FILT_IN_AGC     <<30)
                          | (USE_SPUR_FILT_IN_SELFCOR_MERLIN <<31)
                          | (spur_freq_sd          <<20)
                          | (spur_delta_phase      <<0);
/*      printf("0x%04x: 0x%08x\n", addr, newVal);*/
				REGW(devNum, addr, newVal);

		//choose to cancel between control and extension
		addr = 0x99c0;
                newVal = (spur_subchannel_sd     <<28);
/*      printf("0x%04x: 0x%08x\n", addr, newVal);*/
				REGW(devNum, addr, newVal);

		// Set pilot masks
                cur_bin = -60;
                upper = bin + 1;
                lower = bin - 1;

                pilot_mask_1 = 0;
                pilot_mask_2 = 0;
                pilot_mask_3 = 0;
                pilot_mask_4 = 0;


		//printf("dbg: pm_cur_bin: %f\n", $cur_bin);
                for (bp=0;bp<30;bp++) {
                    if ((cur_bin>lower) && (cur_bin<upper)) {
                       pilot_mask_1 = pilot_mask_1 | 0x1<<bp;
                    }
                    cur_bin++;
                }

                // printf("dbg: pm_cur_bin: %f\n", cur_bin);
                for (bp=0;bp<30;bp++) {
                    if ((cur_bin>lower) && (cur_bin<upper)) {
                       pilot_mask_2 = pilot_mask_2 | 0x1<<bp;
                    }
                    cur_bin++;
                }
                cur_bin++; // Skip bin zero

                // printf("dbg: pm_cur_bin: %f\n", cur_bin);
                for (bp=0;bp<30;bp++) {
                    if ((cur_bin>lower) && (cur_bin<upper)) {
                       pilot_mask_3 = pilot_mask_3 | 0x1<<bp;
                    }
                    cur_bin++;
                }
                // printf("dbg: pm_cur_bin: %f\n", cur_bin);
                for (bp=0;bp<30;bp++) {
                    if ((cur_bin>lower) && (cur_bin<upper)) {
                       pilot_mask_4 = pilot_mask_4 | 0x1<<bp;
                    }
                    cur_bin++;
                }

				REGW(devNum, 0x9980, pilot_mask_1);
				REGW(devNum, 0x9984, pilot_mask_2);
				REGW(devNum, 0xa3b0, pilot_mask_3);
				REGW(devNum, 0xa3b4, pilot_mask_4);

//                printf("bb_pilot_mask_1 0x%08x\n", pilot_mask_1);
//                printf("bb_pilot_mask_2 0x%08x\n", pilot_mask_2);
//                printf("bb_pilot_mask_3 0x%08x\n", pilot_mask_3);
//                printf("bb_pilot_mask_4 0x%08x\n", pilot_mask_4);

// Set channel masks
				cur_bin = -60;
                upper = bin + 1;
                lower = bin - 1;
                chan_mask_1 = 0;
                chan_mask_2 = 0;
                chan_mask_3 = 0;
                chan_mask_4 = 0;

                for (bp=0; bp<30; bp++) {
                    if ((cur_bin>lower) && (cur_bin<upper)) {
                       chan_mask_1 = chan_mask_1 | 0x1<<bp;
                    }
                    cur_bin++;
                }


                for (bp=0; bp<30; bp++) {
                    if ((cur_bin>lower) && (cur_bin<upper)) {
                       chan_mask_2 = chan_mask_2 | 0x1<<bp;
                    }
                    cur_bin++;
                }
                cur_bin++;

                for (bp=0; bp<30; bp++) {
                    if ((cur_bin>lower) && (cur_bin<upper)) {
                       chan_mask_3 = chan_mask_3 | 0x1<<bp;
                    }
                    cur_bin++;
                }

                for (bp=0; bp<30; bp++) {
                    if ((cur_bin>lower) && (cur_bin<upper)) {
                       chan_mask_4 = chan_mask_4 | 0x1<<bp;
                    }
                    cur_bin++;
                }

				REGW(devNum, 0x9998, chan_mask_1);
				REGW(devNum, 0x999c, chan_mask_2);
				REGW(devNum, 0x99d4, chan_mask_3);
				REGW(devNum, 0x99d8, chan_mask_4);

//				printf ("bb_chan_mask_1  0x%08x\n", chan_mask_1);
//                printf ("bb_chan_mask_2  0x%08x\n", chan_mask_2);
//                printf ("bb_chan_mask_3  0x%08x\n", chan_mask_3);
//                printf ("bb_chan_mask_4  0x%08x\n", chan_mask_4);

			}
		        else  if (((bb_spur > -limit) && (bb_spur < limit)) && (spur_miti_done == 1))
			{
		               bin = (bb_spur/0.3125);
			}
			else  // spur is outside +/- 18.750 
			{
				bin = 0;
			}  // end of spur range check

	// Modify viterbi mask 1
	if((bb_spur > -limit) && (bb_spur < limit))
	{
		if (spur_miti_done == 0)
		{
//			printf("first bb_spur is %d\n", bb_spur);
		// clean mask_p and mask_m arrays
			for(cnt=0; cnt<62; cnt++)
			{
				mask_m[cnt] = 0;
				mask_p[cnt] = 0;
			}
			cur_vit_mask = 61;
	                upper = bin + 1.2;
        	        lower = bin - 1.2;

                	for (cnt=0; cnt<123; cnt++) 
				{
                    if ((cur_vit_mask > lower) && (cur_vit_mask < upper))
					{
                        if ((fabs((double)cur_vit_mask-bin)) < 0.75)
						{
                           mask_amt = 1;
                        }
						else
						{
                           mask_amt = 0;
                        }
                        if (cur_vit_mask < 0)
					{
                            			mask_m[abs(cur_vit_mask)] = mask_amt;
                        			} 
						else 
						{
			                        mask_p[cur_vit_mask] = mask_amt;
                        		}
                    		}
                    		cur_vit_mask--;
                	}
		}
		else if(spur_miti_done == 1)
		{
//			printf("Second bb_spur is %d\n", bb_spur);
		// clean mask_p and mask_m arrays
			for(cnt=0; cnt<62; cnt++)
			{
				mask_m[cnt] = 0;
				mask_p[cnt] = 0;
			}
			cur_vit_mask = 61;
	        bin = bin_old;
			upper = bin + 1.2;
        	        lower = bin - 1.2;
			upper_old = bin_old + 1.2;
			lower_old = bin_old + 1.2;
//			printf("bin_old is %5.1f, bin is %5.1f\n", bin_old, bin);
                	for (cnt=0; cnt<123; cnt++) 
				{
                    		if (((cur_vit_mask > lower) && (cur_vit_mask < upper)) || ((cur_vit_mask > lower_old) && (cur_vit_mask < upper_old)))
					{
                        		if ((fabs((double)cur_vit_mask-bin)) < 0.75) 
					{
                           			mask_amt = 1;
                        			} 
						else 
						{
			                        mask_amt = 0;
                        		}
                        		if (cur_vit_mask < 0) 
					{
                            mask_m[abs(cur_vit_mask)] = mask_amt;
                        }
						else
						{
			                        mask_p[cur_vit_mask] = mask_amt;
                        		}
                    		}
                    		cur_vit_mask--;
                	}
		}
	
		addr = 0x9900;
                tmp_mask =  (mask_m[46] << 30) | (mask_m[47] << 28)
                          | (mask_m[48] << 26) | (mask_m[49] << 24)
                          | (mask_m[50] << 22) | (mask_m[51] << 20)
                          | (mask_m[52] << 18) | (mask_m[53] << 16)
                          | (mask_m[54] << 14) | (mask_m[55] << 12)
                          | (mask_m[56] << 10) | (mask_m[57] <<  8)
                          | (mask_m[58] <<  6) | (mask_m[59] <<  4)
                          | (mask_m[60] <<  2) | (mask_m[61] <<  0);
				REGW(devNum, addr, tmp_mask);
//                printf ("bb_vit_mask_m_46_61 0x%04x: 0x%08x\n", addr, tmp_mask);

                addr = 0x9904;
                tmp_mask =                       (mask_m[31] << 28)
                          | (mask_m[32] << 26) | (mask_m[33] << 24)
                          | (mask_m[34] << 22) | (mask_m[35] << 20)
                          | (mask_m[36] << 18) | (mask_m[37] << 16)
                          | (mask_m[38] << 14) | (mask_m[39] << 12)
                          | (mask_m[40] << 10) | (mask_m[41] <<  8)
                          | (mask_m[42] <<  6) | (mask_m[43] <<  4)
                          | (mask_m[44] <<  2) | (mask_m[45] <<  0);
				REGW(devNum, addr, tmp_mask);
//                printf ("bb_vit_mask_m_31_45 0x%04x: 0x%08x\n", addr, tmp_mask);

                addr = 0x9908;
                tmp_mask =  (mask_m[16] << 30) | (mask_m[16] << 28)
                          | (mask_m[18] << 26) | (mask_m[18] << 24)
                          | (mask_m[20] << 22) | (mask_m[20] << 20)
                          | (mask_m[22] << 18) | (mask_m[22] << 16)
                          | (mask_m[24] << 14) | (mask_m[24] << 12)
                          | (mask_m[25] << 10) | (mask_m[26] <<  8)
                          | (mask_m[27] <<  6) | (mask_m[28] <<  4)
                          | (mask_m[29] <<  2) | (mask_m[30] <<  0);
				REGW(devNum, addr, tmp_mask);
//               printf ("bb_vit_mask_m_16_30 0x%04x: 0x%08x\n", addr, tmp_mask);

                addr = 0x990c;
                tmp_mask =  (mask_m[ 0] << 30) | (mask_m[ 1] << 28)
                          | (mask_m[ 2] << 26) | (mask_m[ 3] << 24)
                          | (mask_m[ 4] << 22) | (mask_m[ 5] << 20)
                          | (mask_m[ 6] << 18) | (mask_m[ 7] << 16)
                          | (mask_m[ 8] << 14) | (mask_m[ 9] << 12)
                          | (mask_m[10] << 10) | (mask_m[11] <<  8)
                          | (mask_m[12] <<  6) | (mask_m[13] <<  4)
                          | (mask_m[14] <<  2) | (mask_m[15] <<  0);
				REGW(devNum, addr, tmp_mask);
//                printf ("bb_vit_mask_m_00_15 0x%04x: 0x%08x\n", addr, tmp_mask);

                addr = 0x9988;
                tmp_mask =                       (mask_p[15] << 28)
                          | (mask_p[14] << 26) | (mask_p[13] << 24)
                          | (mask_p[12] << 22) | (mask_p[11] << 20)
                          | (mask_p[10] << 18) | (mask_p[ 9] << 16)
                          | (mask_p[ 8] << 14) | (mask_p[ 7] << 12)
                          | (mask_p[ 6] << 10) | (mask_p[ 5] <<  8)
                          | (mask_p[ 4] <<  6) | (mask_p[ 3] <<  4)
                          | (mask_p[ 2] <<  2) | (mask_p[ 1] <<  0);
				REGW(devNum, addr, tmp_mask);
//                printf ("bb_vit_mask_p_15_01 0x%04x: 0x%08x\n", addr, tmp_mask);

                addr = 0x998c;
                tmp_mask =                       (mask_p[30] << 28)
                          | (mask_p[29] << 26) | (mask_p[28] << 24)
                          | (mask_p[27] << 22) | (mask_p[26] << 20)
                          | (mask_p[25] << 18) | (mask_p[24] << 16)
                          | (mask_p[23] << 14) | (mask_p[22] << 12)
                          | (mask_p[21] << 10) | (mask_p[20] <<  8)
                          | (mask_p[19] <<  6) | (mask_p[18] <<  4)
                          | (mask_p[17] <<  2) | (mask_p[16] <<  0);
				REGW(devNum, addr, tmp_mask);
//                printf ("bb_vit_mask_p_30_16 0x%04x: 0x%08x\n", addr, tmp_mask);

                addr = 0x9990;
                tmp_mask =                       (mask_p[45] << 28)
                          | (mask_p[44] << 26) | (mask_p[43] << 24)
                          | (mask_p[42] << 22) | (mask_p[41] << 20)
                          | (mask_p[40] << 18) | (mask_p[39] << 16)
                          | (mask_p[38] << 14) | (mask_p[37] << 12)
                          | (mask_p[36] << 10) | (mask_p[35] <<  8)
                          | (mask_p[34] <<  6) | (mask_p[33] <<  4)
                          | (mask_p[32] <<  2) | (mask_p[31] <<  0);
				REGW(devNum, addr, tmp_mask);
//               printf ("bb_vit_mask_p_45_31 0x%04x: 0x%08x\n", addr, tmp_mask);

                addr = 0x9994;
                tmp_mask =  (mask_p[61] << 30) | (mask_p[60] << 28)
                          | (mask_p[59] << 26) | (mask_p[58] << 24)
                          | (mask_p[57] << 22) | (mask_p[56] << 20)
                          | (mask_p[55] << 18) | (mask_p[54] << 16)
                          | (mask_p[53] << 14) | (mask_p[52] << 12)
                          | (mask_p[51] << 10) | (mask_p[50] <<  8)
                          | (mask_p[49] <<  6) | (mask_p[48] <<  4)
                          | (mask_p[47] <<  2) | (mask_p[46] <<  0);
				REGW(devNum, addr, tmp_mask);
//                printf ("bb_vit_mask_p_61_46 0x%04x: 0x%08x\n", addr, tmp_mask);

// Set viterbi mask 2
                cur_vit_mask = 61;
                upper = bin + 1.2;
                lower = bin - 1.2;

				// clean mask_p and mask_m arrays
				for(cnt=0; cnt<62; cnt++)
				{
					mask_m[cnt] = 0;
					mask_p[cnt] = 0;
				}

                for (cnt=0;cnt<123;cnt++)
				{
                    if ((cur_vit_mask > lower) && (cur_vit_mask < upper))
					{
                        if ((fabs((double)cur_vit_mask-bin)) < 0.75)
						{
                           mask_amt = 1;
                        } else
						{
                           mask_amt = 0;
                        }
                        if (cur_vit_mask < 0)
						{
                            mask_m[abs(cur_vit_mask)] = mask_amt;
                        } else
						{
                            mask_p[cur_vit_mask] = mask_amt;
                        }
                    }
                    cur_vit_mask--;
                }

                addr = 0xa3a0;
                tmp_mask =  (mask_m[46] << 30) | (mask_m[47] << 28)
                          | (mask_m[48] << 26) | (mask_m[49] << 24)
                          | (mask_m[50] << 22) | (mask_m[51] << 20)
                          | (mask_m[52] << 18) | (mask_m[53] << 16)
                          | (mask_m[54] << 14) | (mask_m[55] << 12)
                          | (mask_m[56] << 10) | (mask_m[57] <<  8)
                          | (mask_m[58] <<  6) | (mask_m[59] <<  4)
                          | (mask_m[60] <<  2) | (mask_m[61] <<  0);
				REGW(devNum, addr, tmp_mask);
//                printf ("bb_vit_mask2_m_46_61 0x%04x: 0x%08x\n", addr, tmp_mask);

                addr = 0xa3a4;
                tmp_mask =                       (mask_m[31] << 28)
                          | (mask_m[32] << 26) | (mask_m[33] << 24)
                          | (mask_m[34] << 22) | (mask_m[35] << 20)
                          | (mask_m[36] << 18) | (mask_m[37] << 16)
                          | (mask_m[38] << 14) | (mask_m[39] << 12)
                          | (mask_m[40] << 10) | (mask_m[41] <<  8)
                          | (mask_m[42] <<  6) | (mask_m[43] <<  4)
                          | (mask_m[44] <<  2) | (mask_m[45] <<  0);
				REGW(devNum, addr, tmp_mask);
//                printf ("bb_vit_mask2_m_31_45 0x%04x: 0x%08x\n", addr, tmp_mask);

                addr = 0xa3a8;
                tmp_mask =  (mask_m[16] << 30) | (mask_m[16] << 28)
                          | (mask_m[18] << 26) | (mask_m[18] << 24)
                          | (mask_m[20] << 22) | (mask_m[20] << 20)
                          | (mask_m[22] << 18) | (mask_m[22] << 16)
                          | (mask_m[24] << 14) | (mask_m[24] << 12)
                          | (mask_m[25] << 10) | (mask_m[26] <<  8)
                          | (mask_m[27] <<  6) | (mask_m[28] <<  4)
                          | (mask_m[29] <<  2) | (mask_m[30] <<  0);
				REGW(devNum, addr, tmp_mask);
//                printf ("bb_vit_mask2_m_16_30 0x%04x: 0x%08x\n", addr, tmp_mask);

                addr = 0xa3ac;
                tmp_mask =  (mask_m[ 0] << 30) | (mask_m[ 1] << 28)
                          | (mask_m[ 2] << 26) | (mask_m[ 3] << 24)
                          | (mask_m[ 4] << 22) | (mask_m[ 5] << 20)
                          | (mask_m[ 6] << 18) | (mask_m[ 7] << 16)
                          | (mask_m[ 8] << 14) | (mask_m[ 9] << 12)
                          | (mask_m[10] << 10) | (mask_m[11] <<  8)
                          | (mask_m[12] <<  6) | (mask_m[13] <<  4)
                          | (mask_m[14] <<  2) | (mask_m[15] <<  0);
				REGW(devNum, addr, tmp_mask);
//                printf ("bb_vit_mask2_m_00_15 0x%04x: 0x%08x\n", addr, tmp_mask);

                addr = 0xa3b8;
                tmp_mask =                       (mask_p[15] << 28)
                          | (mask_p[14] << 26) | (mask_p[13] << 24)
                          | (mask_p[12] << 22) | (mask_p[11] << 20)
                          | (mask_p[10] << 18) | (mask_p[ 9] << 16)
                          | (mask_p[ 8] << 14) | (mask_p[ 7] << 12)
                          | (mask_p[ 6] << 10) | (mask_p[ 5] <<  8)
                          | (mask_p[ 4] <<  6) | (mask_p[ 3] <<  4)
                          | (mask_p[ 2] <<  2) | (mask_p[ 1] <<  0);
				REGW(devNum, addr, tmp_mask);
//                printf ("bb_vit_mask2_p_15_01 0x%04x: 0x%08x\n", addr, tmp_mask);

                addr = 0xa3bc;
                tmp_mask =                       (mask_p[30] << 28)
                          | (mask_p[29] << 26) | (mask_p[28] << 24)
                          | (mask_p[27] << 22) | (mask_p[26] << 20)
                          | (mask_p[25] << 18) | (mask_p[24] << 16)
                          | (mask_p[23] << 14) | (mask_p[22] << 12)
                          | (mask_p[21] << 10) | (mask_p[20] <<  8)
                          | (mask_p[19] <<  6) | (mask_p[18] <<  4)
                          | (mask_p[17] <<  2) | (mask_p[16] <<  0);
				REGW(devNum, addr, tmp_mask);
//                printf ("bb_vit_mask2_p_30_16 0x%04x: 0x%08x\n", addr, tmp_mask);

                addr = 0xa3c0;
                tmp_mask =                       (mask_p[45] << 28)
                          | (mask_p[44] << 26) | (mask_p[43] << 24)
                          | (mask_p[42] << 22) | (mask_p[41] << 20)
                          | (mask_p[40] << 18) | (mask_p[39] << 16)
                          | (mask_p[38] << 14) | (mask_p[37] << 12)
                          | (mask_p[36] << 10) | (mask_p[35] <<  8)
                          | (mask_p[34] <<  6) | (mask_p[33] <<  4)
                          | (mask_p[32] <<  2) | (mask_p[31] <<  0);
				REGW(devNum, addr, tmp_mask);
//                printf ("bb_vit_mask2_p_45_31 0x%04x: 0x%08x\n", addr, tmp_mask);

                addr = 0xa3c4;
                tmp_mask =  (mask_p[61] << 30) | (mask_p[60] << 28)
                          | (mask_p[59] << 26) | (mask_p[58] << 24)
                          | (mask_p[57] << 22) | (mask_p[56] << 20)
                          | (mask_p[55] << 18) | (mask_p[54] << 16)
                          | (mask_p[53] << 14) | (mask_p[52] << 12)
                          | (mask_p[51] << 10) | (mask_p[50] <<  8)
                          | (mask_p[49] <<  6) | (mask_p[48] <<  4)
                          | (mask_p[47] <<  2) | (mask_p[46] <<  0);
				REGW(devNum, addr, tmp_mask);
//                printf ("bb_vit_mask2_p_61_45 0x%04x: 0x%08x\n", addr, tmp_mask);
		} //endif for viterbi depuncturing
	} // loop on spur_arr
}


/**************************************************************
 * setSpurMitigationOwl
 *
 * Apply Spur Immunity to Boards that require it.
 * Applies only to OFDM RX operation.
 */
void setSpurMitigationOwl(A_UINT32 devNum, A_UINT32 freq)
{
    LIB_DEV_INFO *pLibDev = gLibInfo.pLibDevArray[devNum];
	A_UINT16 i, j, cnt, spurFlag;
    A_UINT32 spurredChans[10];  // Channels affected by spur
    A_UINT16 totalSpurredChans;
	A_UINT16 spur_arr[2], spur_arr_size;
	A_UINT32 spur_lo, spur_hi, spur, spur_base;
	A_UINT32 addr, newVal, oldVal, tmp_mask;
	double bin, bb_spur_off, denom;
    A_INT32 bb_spur, bb_spur_lo, bb_spur_hi;
	double upper, lower, cur_bin;
	A_UINT32 spur_delta_phase;
	A_UINT32 spur_freq_sd;
	A_UINT32 pilot_mask_1, pilot_mask_2, pilot_mask_3, pilot_mask_4, bp;
    A_UINT32 chan_mask_1, chan_mask_2, chan_mask_3, chan_mask_4;
	A_INT16 cur_vit_mask;
	A_UINT16 mask_m[62], mask_p[62], mask_amt;

	// spur affected channels
    totalSpurredChans = 2;
	spurredChans[0] = 5190;
	spurredChans[1] = 5270;

	// Check if current freq is in spur affected list
	// If not return, otherwise continue
	spurFlag = 0;
	for(i = 0; i<totalSpurredChans; i++)
	{
		if(spurredChans[i] == freq)
			spurFlag = 1;
	}
	if(spurFlag == 0)
		return;

//	printf("SNOOP:: Running spur mitigation for OWL, channel = %d, ht40 = %d\n", freq, pLibDev->libCfgParams.ht40_enable);

    spur_arr_size = 1;
	spur_arr[0] = 40;
//	spur_arr[1] = 44;

	for(i=0; i<spur_arr_size; i++)
	{
		spur_base = spur_arr[i];
        spur_lo = (A_UINT32)((freq/spur_base)*spur_base);
        if ((A_UINT16)spur_lo == freq) {
            spur_hi = spur_lo;
        } else {
            spur_hi = spur_lo + spur_base;
        }

//		printf("spur_hi %d, spur_lo %d\n", spur_hi, spur_lo);
        bb_spur_lo = spur_lo - freq;
        bb_spur_hi = spur_hi - freq;

		for(j=0; j<2; j++)  // Loop 2 times for spur_lo and spur_hi
		{
			bb_spur = bb_spur_lo;
			if(j == 1) bb_spur = bb_spur_hi;
            spur = freq + bb_spur;

			// if base band spur is inside of the range +/- 18.750 apply spur mitigation
			// else no action needed

            if ((bb_spur > -18.750) && (bb_spur < 18.750))
			{
                bin = (bb_spur/0.3125);
//                printf("chan:%d    bin:%5.1f  bb_spur:%3d   spur:%d base:%d\n",
//                          freq, bin, bb_spur, spur, spur_base);

				// modify bb_ch0_enable_spur_rssi
				addr = 0xc920;
                oldVal = REGR(devNum, addr);
                newVal = oldVal | (ENABLE_SPUR_RSSI  <<31)
                            | (ENABLE_SPUR_FILTER<<30)
                            | (ENABLE_CHAN_MASK  <<29)
                            | (ENABLE_PILOT_MASK <<28);
				REGW(devNum, addr, newVal);
//                printf("0x%04x: 0x%08x\n", addr, newVal);

				// modify bb_spur_rssi_thresh
                addr = 0x994c;
                newVal = (MASK_RATE_CNTL       <<18)
                         | (ENABLE_MASK_PPM_OWL  <<17)
                         | (MASK_RATE_SELECT     <<9)
                         | (ENABLE_VIT_SPUR_RSSI <<8)
                         | (SPUR_RSSI_THRESH     <<0);
				REGW(devNum, addr, newVal);
//                printf("0x%04x: 0x%08x\n", addr, newVal);

				bb_spur_off = bb_spur - 10 ;

                // $spur_delta_phase = ($bb_spur* (10**6) * (25/2*(10**9)) * (2**21)) & 0xfffff;
                // $spur_freq_sd     = (($bb_spur_off/44) * (2**11)) & 0x3ff;

                spur_delta_phase = (int)((bb_spur * pow(10.0, 6.0) * (25.0/2* pow(10.0, -9.0)) * pow(2.0, 21.0))) & 0xfffff;

				// TEST ONLY
//				spur_delta_phase = 0xfffff;
				denom = 44;
				if(pLibDev->mode == MODE_11A)
				{
					denom = 40;
				}
                spur_freq_sd     = (int)(((bb_spur_off/denom) * pow(2.0, 11.0))) & 0x3ff;
//				printf("bb_spur %d bb_spur_off %d\n", bb_spur, bb_spur_off);
//              printf("spur_freq_sd %x \tspur_delta_phase %x\n", spur_freq_sd, spur_delta_phase);
                // printf("%x %x\n", spur_freq_sd, spur_delta_phase);

                // modify bb_spur_delta_phase
				addr = 0x99a0;
                newVal = (USE_SPUR_FILT_IN_AGC     <<30)
                          | (USE_SPUR_FILT_IN_SELFCOR_OWL <<31)
                          | (spur_freq_sd          <<20)
                          | (spur_delta_phase      <<0);
//                printf("0x%04x: 0x%08x\n", addr, newVal);
				REGW(devNum, addr, newVal);

// Set pilot masks
                cur_bin = -60;
                upper = bin + 1;
                lower = bin - 1;

                pilot_mask_1 = 0;
                pilot_mask_2 = 0;
                pilot_mask_3 = 0;
                pilot_mask_4 = 0;


//                printf("dbg: pm_cur_bin: %f\n", $cur_bin);
                for (bp=0;bp<30;bp++) {
                    if ((cur_bin>lower) && (cur_bin<upper)) {
                       pilot_mask_1 = pilot_mask_1 | 0x1<<bp;
                    }
                    cur_bin++;
                }

                // printf("dbg: pm_cur_bin: %f\n", cur_bin);
                for (bp=0;bp<30;bp++) {
                    if ((cur_bin>lower) && (cur_bin<upper)) {
                       pilot_mask_2 = pilot_mask_2 | 0x1<<bp;
                    }
                    cur_bin++;
                }
                cur_bin++; // Skip bin zero

                // printf("dbg: pm_cur_bin: %f\n", cur_bin);
                for (bp=0;bp<30;bp++) {
                    if ((cur_bin>lower) && (cur_bin<upper)) {
                       pilot_mask_3 = pilot_mask_3 | 0x1<<bp;
                    }
                    cur_bin++;
                }
                // printf("dbg: pm_cur_bin: %f\n", cur_bin);
                for (bp=0;bp<30;bp++) {
                    if ((cur_bin>lower) && (cur_bin<upper)) {
                       pilot_mask_4 = pilot_mask_4 | 0x1<<bp;
                    }
                    cur_bin++;
                }

				REGW(devNum, 0x9980, pilot_mask_1);
				REGW(devNum, 0x9984, pilot_mask_2);
				REGW(devNum, 0xa3b0, pilot_mask_3);
				REGW(devNum, 0xa3b4, pilot_mask_4);

//                printf("bb_pilot_mask_1 0x%08x\n", pilot_mask_1);
//                printf("bb_pilot_mask_2 0x%08x\n", pilot_mask_2);
//                printf("bb_pilot_mask_3 0x%08x\n", pilot_mask_3);
//                printf("bb_pilot_mask_4 0x%08x\n", pilot_mask_4);

// Set channel masks
				cur_bin = -60;
                upper = bin + 1;
                lower = bin - 1;
                chan_mask_1 = 0;
                chan_mask_2 = 0;
                chan_mask_3 = 0;
                chan_mask_4 = 0;

                for (bp=0; bp<30; bp++) {
                    if ((cur_bin>lower) && (cur_bin<upper)) {
                       chan_mask_1 = chan_mask_1 | 0x1<<bp;
                    }
                    cur_bin++;
                }


                for (bp=0; bp<30; bp++) {
                    if ((cur_bin>lower) && (cur_bin<upper)) {
                       chan_mask_2 = chan_mask_2 | 0x1<<bp;
                    }
                    cur_bin++;
                }
                cur_bin++;

                for (bp=0; bp<30; bp++) {
                    if ((cur_bin>lower) && (cur_bin<upper)) {
                       chan_mask_3 = chan_mask_3 | 0x1<<bp;
                    }
                    cur_bin++;
                }

                for (bp=0; bp<30; bp++) {
                    if ((cur_bin>lower) && (cur_bin<upper)) {
                       chan_mask_4 = chan_mask_4 | 0x1<<bp;
                    }
                    cur_bin++;
                }

				REGW(devNum, 0x9998, chan_mask_1);
				REGW(devNum, 0x999c, chan_mask_2);
				REGW(devNum, 0x99d4, chan_mask_3);
				REGW(devNum, 0x99d8, chan_mask_4);

//				printf ("bb_chan_mask_1  0x%08x\n", chan_mask_1);
//                printf ("bb_chan_mask_2  0x%08x\n", chan_mask_2);
//                printf ("bb_chan_mask_3  0x%08x\n", chan_mask_3);
//                printf ("bb_chan_mask_4  0x%08x\n", chan_mask_4);


// Modify viterbi mask 1

				// clean mask_p and mask_m arrays
				for(cnt=0; cnt<62; cnt++)
				{
					mask_m[cnt] = 0;
					mask_p[cnt] = 0;
				}

				cur_vit_mask = 61;
                upper = bin + 1.2;
                lower = bin - 1.2;

                for (cnt=0; cnt<123; cnt++)
				{
                    if ((cur_vit_mask > lower) && (cur_vit_mask < upper))
					{
                        if ((fabs((double)cur_vit_mask-bin)) < 0.75)
						{
                           mask_amt = 1;
                        }
						else
						{
                           mask_amt = 0;
                        }
                        if (cur_vit_mask < 0)
						{
                            mask_m[abs(cur_vit_mask)] = mask_amt;
                        }
						else
						{
                            mask_p[cur_vit_mask] = mask_amt;
                        }
                    }
                    cur_vit_mask--;
                }

                addr = 0x9900;
                tmp_mask =  (mask_m[46] << 30) | (mask_m[47] << 28)
                          | (mask_m[48] << 26) | (mask_m[49] << 24)
                          | (mask_m[50] << 22) | (mask_m[51] << 20)
                          | (mask_m[52] << 18) | (mask_m[53] << 16)
                          | (mask_m[54] << 14) | (mask_m[55] << 12)
                          | (mask_m[56] << 10) | (mask_m[57] <<  8)
                          | (mask_m[58] <<  6) | (mask_m[59] <<  4)
                          | (mask_m[60] <<  2) | (mask_m[61] <<  0);
				REGW(devNum, addr, tmp_mask);
//                printf ("bb_vit_mask_m_46_61 0x%04x: 0x%08x\n", addr, tmp_mask);

                addr = 0x9904;
                tmp_mask =                       (mask_m[31] << 28)
                          | (mask_m[32] << 26) | (mask_m[33] << 24)
                          | (mask_m[34] << 22) | (mask_m[35] << 20)
                          | (mask_m[36] << 18) | (mask_m[37] << 16)
                          | (mask_m[38] << 14) | (mask_m[39] << 12)
                          | (mask_m[40] << 10) | (mask_m[41] <<  8)
                          | (mask_m[42] <<  6) | (mask_m[43] <<  4)
                          | (mask_m[44] <<  2) | (mask_m[45] <<  0);
				REGW(devNum, addr, tmp_mask);
//                printf ("bb_vit_mask_m_31_45 0x%04x: 0x%08x\n", addr, tmp_mask);

                addr = 0x9908;
                tmp_mask =  (mask_m[16] << 30) | (mask_m[16] << 28)
                          | (mask_m[18] << 26) | (mask_m[18] << 24)
                          | (mask_m[20] << 22) | (mask_m[20] << 20)
                          | (mask_m[22] << 18) | (mask_m[22] << 16)
                          | (mask_m[24] << 14) | (mask_m[24] << 12)
                          | (mask_m[25] << 10) | (mask_m[26] <<  8)
                          | (mask_m[27] <<  6) | (mask_m[28] <<  4)
                          | (mask_m[29] <<  2) | (mask_m[30] <<  0);
				REGW(devNum, addr, tmp_mask);
//               printf ("bb_vit_mask_m_16_30 0x%04x: 0x%08x\n", addr, tmp_mask);

                addr = 0x990c;
                tmp_mask =  (mask_m[ 0] << 30) | (mask_m[ 1] << 28)
                          | (mask_m[ 2] << 26) | (mask_m[ 3] << 24)
                          | (mask_m[ 4] << 22) | (mask_m[ 5] << 20)
                          | (mask_m[ 6] << 18) | (mask_m[ 7] << 16)
                          | (mask_m[ 8] << 14) | (mask_m[ 9] << 12)
                          | (mask_m[10] << 10) | (mask_m[11] <<  8)
                          | (mask_m[12] <<  6) | (mask_m[13] <<  4)
                          | (mask_m[14] <<  2) | (mask_m[15] <<  0);
				REGW(devNum, addr, tmp_mask);
//                printf ("bb_vit_mask_m_00_15 0x%04x: 0x%08x\n", addr, tmp_mask);

                addr = 0x9988;
                tmp_mask =                       (mask_p[15] << 28)
                          | (mask_p[14] << 26) | (mask_p[13] << 24)
                          | (mask_p[12] << 22) | (mask_p[11] << 20)
                          | (mask_p[10] << 18) | (mask_p[ 9] << 16)
                          | (mask_p[ 8] << 14) | (mask_p[ 7] << 12)
                          | (mask_p[ 6] << 10) | (mask_p[ 5] <<  8)
                          | (mask_p[ 4] <<  6) | (mask_p[ 3] <<  4)
                          | (mask_p[ 2] <<  2) | (mask_p[ 1] <<  0);
				REGW(devNum, addr, tmp_mask);
//                printf ("bb_vit_mask_p_15_01 0x%04x: 0x%08x\n", addr, tmp_mask);

                addr = 0x998c;
                tmp_mask =                       (mask_p[30] << 28)
                          | (mask_p[29] << 26) | (mask_p[28] << 24)
                          | (mask_p[27] << 22) | (mask_p[26] << 20)
                          | (mask_p[25] << 18) | (mask_p[24] << 16)
                          | (mask_p[23] << 14) | (mask_p[22] << 12)
                          | (mask_p[21] << 10) | (mask_p[20] <<  8)
                          | (mask_p[19] <<  6) | (mask_p[18] <<  4)
                          | (mask_p[17] <<  2) | (mask_p[16] <<  0);
				REGW(devNum, addr, tmp_mask);
//                printf ("bb_vit_mask_p_30_16 0x%04x: 0x%08x\n", addr, tmp_mask);

                addr = 0x9990;
                tmp_mask =                       (mask_p[45] << 28)
                          | (mask_p[44] << 26) | (mask_p[43] << 24)
                          | (mask_p[42] << 22) | (mask_p[41] << 20)
                          | (mask_p[40] << 18) | (mask_p[39] << 16)
                          | (mask_p[38] << 14) | (mask_p[37] << 12)
                          | (mask_p[36] << 10) | (mask_p[35] <<  8)
                          | (mask_p[34] <<  6) | (mask_p[33] <<  4)
                          | (mask_p[32] <<  2) | (mask_p[31] <<  0);
				REGW(devNum, addr, tmp_mask);
//               printf ("bb_vit_mask_p_45_31 0x%04x: 0x%08x\n", addr, tmp_mask);

                addr = 0x9994;
                tmp_mask =  (mask_p[61] << 30) | (mask_p[60] << 28)
                          | (mask_p[59] << 26) | (mask_p[58] << 24)
                          | (mask_p[57] << 22) | (mask_p[56] << 20)
                          | (mask_p[55] << 18) | (mask_p[54] << 16)
                          | (mask_p[53] << 14) | (mask_p[52] << 12)
                          | (mask_p[51] << 10) | (mask_p[50] <<  8)
                          | (mask_p[49] <<  6) | (mask_p[48] <<  4)
                          | (mask_p[47] <<  2) | (mask_p[46] <<  0);
				REGW(devNum, addr, tmp_mask);
//                printf ("bb_vit_mask_p_61_46 0x%04x: 0x%08x\n", addr, tmp_mask);

// Set viterbi mask 2
                cur_vit_mask = 61;
                upper = bin + 1.2;
                lower = bin - 1.2;

				// clean mask_p and mask_m arrays
				for(cnt=0; cnt<62; cnt++)
				{
					mask_m[cnt] = 0;
					mask_p[cnt] = 0;
				}

                for (cnt=0;cnt<123;cnt++)
				{
                    if ((cur_vit_mask > lower) && (cur_vit_mask < upper))
					{
                        if ((fabs((double)cur_vit_mask-bin)) < 0.75)
						{
                           mask_amt = 1;
                        } else
						{
                           mask_amt = 0;
                        }
                        if (cur_vit_mask < 0)
						{
                            mask_m[abs(cur_vit_mask)] = mask_amt;
                        } else
						{
                            mask_p[cur_vit_mask] = mask_amt;
                        }
                    }
                    cur_vit_mask--;
                }

                addr = 0xa3a0;
                tmp_mask =  (mask_m[46] << 30) | (mask_m[47] << 28)
                          | (mask_m[48] << 26) | (mask_m[49] << 24)
                          | (mask_m[50] << 22) | (mask_m[51] << 20)
                          | (mask_m[52] << 18) | (mask_m[53] << 16)
                          | (mask_m[54] << 14) | (mask_m[55] << 12)
                          | (mask_m[56] << 10) | (mask_m[57] <<  8)
                          | (mask_m[58] <<  6) | (mask_m[59] <<  4)
                          | (mask_m[60] <<  2) | (mask_m[61] <<  0);
				REGW(devNum, addr, tmp_mask);
//                printf ("bb_vit_mask2_m_46_61 0x%04x: 0x%08x\n", addr, tmp_mask);

                addr = 0xa3a4;
                tmp_mask =                       (mask_m[31] << 28)
                          | (mask_m[32] << 26) | (mask_m[33] << 24)
                          | (mask_m[34] << 22) | (mask_m[35] << 20)
                          | (mask_m[36] << 18) | (mask_m[37] << 16)
                          | (mask_m[38] << 14) | (mask_m[39] << 12)
                          | (mask_m[40] << 10) | (mask_m[41] <<  8)
                          | (mask_m[42] <<  6) | (mask_m[43] <<  4)
                          | (mask_m[44] <<  2) | (mask_m[45] <<  0);
				REGW(devNum, addr, tmp_mask);
//                printf ("bb_vit_mask2_m_31_45 0x%04x: 0x%08x\n", addr, tmp_mask);

                addr = 0xa3a8;
                tmp_mask =  (mask_m[16] << 30) | (mask_m[16] << 28)
                          | (mask_m[18] << 26) | (mask_m[18] << 24)
                          | (mask_m[20] << 22) | (mask_m[20] << 20)
                          | (mask_m[22] << 18) | (mask_m[22] << 16)
                          | (mask_m[24] << 14) | (mask_m[24] << 12)
                          | (mask_m[25] << 10) | (mask_m[26] <<  8)
                          | (mask_m[27] <<  6) | (mask_m[28] <<  4)
                          | (mask_m[29] <<  2) | (mask_m[30] <<  0);
				REGW(devNum, addr, tmp_mask);
//                printf ("bb_vit_mask2_m_16_30 0x%04x: 0x%08x\n", addr, tmp_mask);

                addr = 0xa3ac;
                tmp_mask =  (mask_m[ 0] << 30) | (mask_m[ 1] << 28)
                          | (mask_m[ 2] << 26) | (mask_m[ 3] << 24)
                          | (mask_m[ 4] << 22) | (mask_m[ 5] << 20)
                          | (mask_m[ 6] << 18) | (mask_m[ 7] << 16)
                          | (mask_m[ 8] << 14) | (mask_m[ 9] << 12)
                          | (mask_m[10] << 10) | (mask_m[11] <<  8)
                          | (mask_m[12] <<  6) | (mask_m[13] <<  4)
                          | (mask_m[14] <<  2) | (mask_m[15] <<  0);
				REGW(devNum, addr, tmp_mask);
//                printf ("bb_vit_mask2_m_00_15 0x%04x: 0x%08x\n", addr, tmp_mask);

                addr = 0xa3b8;
                tmp_mask =                       (mask_p[15] << 28)
                          | (mask_p[14] << 26) | (mask_p[13] << 24)
                          | (mask_p[12] << 22) | (mask_p[11] << 20)
                          | (mask_p[10] << 18) | (mask_p[ 9] << 16)
                          | (mask_p[ 8] << 14) | (mask_p[ 7] << 12)
                          | (mask_p[ 6] << 10) | (mask_p[ 5] <<  8)
                          | (mask_p[ 4] <<  6) | (mask_p[ 3] <<  4)
                          | (mask_p[ 2] <<  2) | (mask_p[ 1] <<  0);
				REGW(devNum, addr, tmp_mask);
//                printf ("bb_vit_mask2_p_15_01 0x%04x: 0x%08x\n", addr, tmp_mask);

                addr = 0xa3bc;
                tmp_mask =                       (mask_p[30] << 28)
                          | (mask_p[29] << 26) | (mask_p[28] << 24)
                          | (mask_p[27] << 22) | (mask_p[26] << 20)
                          | (mask_p[25] << 18) | (mask_p[24] << 16)
                          | (mask_p[23] << 14) | (mask_p[22] << 12)
                          | (mask_p[21] << 10) | (mask_p[20] <<  8)
                          | (mask_p[19] <<  6) | (mask_p[18] <<  4)
                          | (mask_p[17] <<  2) | (mask_p[16] <<  0);
				REGW(devNum, addr, tmp_mask);
//                printf ("bb_vit_mask2_p_30_16 0x%04x: 0x%08x\n", addr, tmp_mask);

                addr = 0xa3c0;
                tmp_mask =                       (mask_p[45] << 28)
                          | (mask_p[44] << 26) | (mask_p[43] << 24)
                          | (mask_p[42] << 22) | (mask_p[41] << 20)
                          | (mask_p[40] << 18) | (mask_p[39] << 16)
                          | (mask_p[38] << 14) | (mask_p[37] << 12)
                          | (mask_p[36] << 10) | (mask_p[35] <<  8)
                          | (mask_p[34] <<  6) | (mask_p[33] <<  4)
                          | (mask_p[32] <<  2) | (mask_p[31] <<  0);
				REGW(devNum, addr, tmp_mask);
//                printf ("bb_vit_mask2_p_45_31 0x%04x: 0x%08x\n", addr, tmp_mask);

                addr = 0xa3c4;
                tmp_mask =  (mask_p[61] << 30) | (mask_p[60] << 28)
                          | (mask_p[59] << 26) | (mask_p[58] << 24)
                          | (mask_p[57] << 22) | (mask_p[56] << 20)
                          | (mask_p[55] << 18) | (mask_p[54] << 16)
                          | (mask_p[53] << 14) | (mask_p[52] << 12)
                          | (mask_p[51] << 10) | (mask_p[50] <<  8)
                          | (mask_p[49] <<  6) | (mask_p[48] <<  4)
                          | (mask_p[47] <<  2) | (mask_p[46] <<  0);
				REGW(devNum, addr, tmp_mask);
//                printf ("bb_vit_mask2_p_61_45 0x%04x: 0x%08x\n", addr, tmp_mask);

			}
			else   // spur is outside +/- 18.750
			{
				bin = 0;
			}  // end of spur range check
		}  // End of spur_lo/hi loop
	} // loop on spur_arr
}

#define SPUR_CHAN_WIDTH       87
#define BIN_WIDTH_BASE_100HZ  3125
#define BIN_WIDTH_TURBO_100HZ 6250
/**************************************************************
 * setSpurMitigation
 *
 * Apply Spur Immunity to Boards that require it.
 * Applies only to OFDM RX operation.
 */
void setSpurMitigation(A_UINT32 devNum, A_UINT32 freq)
{
    LIB_DEV_INFO *pLibDev = gLibInfo.pLibDevArray[devNum];
    A_UINT32      pilotMask[2] = {0, 0}, binMagMask[4] = {0, 0, 0 , 0};
    A_UINT32      spurChans[2];
    A_UINT16      i;
    A_UINT16      finalSpur, curChanAsSpur, binWidth = 0, spurDetectWidth, spurChan;
    A_INT32       spurDeltaPhase = 0, spurFreqSd = 0, spurOffset, binOffsetNumT16, curBinOffset;
    A_INT16       numBinOffsets;
    A_UINT16      magMapFor4[4] = {1, 2, 2, 1};
    A_UINT16      magMapFor3[3] = {1, 2, 1};
    A_UINT16      *pMagMap;

	if(isSpider(pLibDev->swDevID)) {
		spurChans[0] = 2440;
		spurChans[1] = 2464;
printf("SNOOP: setting spider spur channels\n");
	}
	else
	{
		spurChans[0] = 2420;
		spurChans[1] = 2464;
	}

    /* Only support 11g mode */
    if(pLibDev->mode != MODE_11G) {
        return;
    }

    curChanAsSpur = (A_UINT16)((freq - 2300) * 10);

    /*
     * Check if spur immunity should be performed for this channel
     * Should only be performed once per channel and then saved
     */
    finalSpur = 0;
    spurDetectWidth = SPUR_CHAN_WIDTH;
    if (pLibDev->turbo == TURBO_ENABLE) {
        spurDetectWidth *= 2;
    }

    /* Decide if any spur affects the current channel */
    for (i = 0; i < 2; i++) {
        spurChan = (A_UINT16)((spurChans[i] - 2300) * 10);
        if ((curChanAsSpur - spurDetectWidth <= spurChan) &&
            (curChanAsSpur + spurDetectWidth >= spurChan))
        {
            finalSpur = spurChan;
            break;
        }
    }

    /* Write spur immunity data */
    if (finalSpur == 0) {
        /* Disable spur mitigation */
        writeField(devNum, "bb_enable_spur_filter", 0);
        writeField(devNum, "bb_enable_chan_mask", 0);
        writeField(devNum, "bb_enable_pilot_mask", 0);
		if (!isOwl(pLibDev->swDevID)) {
            writeField(devNum, "bb_use_spur_filter_in_agc", 0);
		}
        writeField(devNum, "bb_use_spur_filter_in_selfcor", 0);
        writeField(devNum, "bb_enable_spur_rssi", 0);
        return;
    } else {
        spurOffset = finalSpur - curChanAsSpur;
        /*
         * Spur calculations:
         * spurDeltaPhase is (spurOffsetIn100KHz / chipFrequencyIn100KHz) << 21
         * spurFreqSd is (spurOffsetIn100KHz / sampleFrequencyIn100KHz) << 11
         */
        switch (pLibDev->mode) {
            case MODE_11A: /* Chip Frequency & sampleFrequency are 40 MHz */
                spurDeltaPhase = (spurOffset << 17) / 25;
                spurFreqSd = spurDeltaPhase >> 10;
                binWidth = BIN_WIDTH_BASE_100HZ;
                break;
            case MODE_11G:
                if(pLibDev->turbo != TURBO_ENABLE) {
                    /* Chip Frequency is 44MHz, sampleFrequency is 40 MHz */
                    spurFreqSd = (spurOffset << 8) / 55;
                    spurDeltaPhase = (spurOffset << 17) / 25;
                    binWidth = BIN_WIDTH_BASE_100HZ;
                }
                else {
                    /* Chip Frequency & sampleFrequency are 80 MHz */
                    spurDeltaPhase = (spurOffset << 16) / 25;
                    spurFreqSd = spurDeltaPhase >> 10;
                    binWidth = BIN_WIDTH_TURBO_100HZ;
                }
                break;
        }

        spurDeltaPhase &= 0xfffff;
        spurFreqSd &= 0x3ff;

        /* Compute Pilot Mask */
        binOffsetNumT16 = ((spurOffset * 1000) << 4) / binWidth;
        /* The spur is on a bin if it's remainder at times 16 is 0 */
        if (binOffsetNumT16 & 0xF) {
            numBinOffsets = 4;
            pMagMap = magMapFor4;
        } else {
            numBinOffsets = 3;
            pMagMap = magMapFor3;
        }
        for (i = 0; i < numBinOffsets; i++) {
            /* Get Pilot Mask values */
            curBinOffset = (binOffsetNumT16 >> 4) + i + 25;
            if ((curBinOffset >= 0) && (curBinOffset <= 32)) {
                if (curBinOffset <= 25) {
                    pilotMask[0] |= 1 << curBinOffset;
                } else if (curBinOffset >= 27){
                    pilotMask[0] |= 1 << (curBinOffset - 1);
                }
            }
            else if ((curBinOffset >= 33) && (curBinOffset <= 52)) {
                pilotMask[1] |= 1 << (curBinOffset - 33);
            }

            /* Get viterbi values */
            if ((curBinOffset >= -1) && (curBinOffset <= 14)) {
                binMagMask[0] |= pMagMap[i] << (curBinOffset + 1) * 2;
            } else if ((curBinOffset >= 15) && (curBinOffset <= 30)) {
                binMagMask[1] |= pMagMap[i] << (curBinOffset - 15) * 2;
            } else if ((curBinOffset >= 31) && (curBinOffset <= 46)) {
                binMagMask[2] |= pMagMap[i] << (curBinOffset -31) * 2;
            } else if((curBinOffset >= 47) && (curBinOffset <= 53)) {
                binMagMask[3] |= pMagMap[i] << (curBinOffset -47) * 2;
            }
        }

#if 0
        printf("||=== Spur Mitigation Debug =====\n");
        printf("|| For channel %d MHz a spur was found at freq %d KHz\n", freq, spurChan + 23000);
        printf("|| Offset found to be %d (100 KHz)\n", spurOffset);
        printf("|| spurDeltaPhase %d, spurFreqSd %d\n", spurDeltaPhase, spurFreqSd);
        printf("|| Pilot Mask 0 0X%08X 1 0X%08X\n", pilotMask[0], pilotMask[1]);
        printf("|| Viterbi Mask 0 0X%08X 1 0X%08X 2 0X%08X 3 0X%08X\n", binMagMask[0], binMagMask[1], binMagMask[2], binMagMask[3]);
        printf("||\n");
        printf("||===");

        for (i = 0; i <= 25; i++) {
            if ((pilotMask[0] >> i) & 1) {
                printf(" X ");
            } else {
                printf("   ");
            }
        }
        printf("===||\n");

        printf("||");
        for (i = 0; i <= 31; i+=2) {
            printf(" %d ", (binMagMask[0] >> i) & 0x3);
        }
        for (i = 0; i <= 23; i+=2) {
            printf(" %d ", (binMagMask[1] >> i) & 0x3);
        }
        printf("||\n");

        printf("||-27-26-25-24-23-22-21-20-19-18-17-16-15-14-13-12-11-10-9 -8 -7 -6 -5 -4 -3 -2 -1  0 ||\n");
        printf("||------------------------------------------------------------------------------------||\n");

        printf("||===");
        for (i = 26; i <= 31; i++) {
            if ((pilotMask[0] >> i) & 1) {
                printf(" X ");
            } else {
                printf("   ");
            }
        }
        for (i = 0; i <= 19; i++) {
            if ((pilotMask[1] >> i) & 1) {
                printf(" X ");
            } else {
                printf("   ");
            }
        }
        printf("===||\n");

        printf("||");
        for (i = 22; i <= 31; i+=2) {
            printf(" %d ", (binMagMask[1] >> i) & 0x3);
        }
        for (i = 0; i <= 31; i+=2) {
            printf(" %d ", (binMagMask[2] >> i) & 0x3);
        }
        for (i = 0; i <= 13; i+=2) {
            printf(" %d ", (binMagMask[3] >> i) & 0x3);
        }
        printf("||\n");

        printf("|| 0  1  2  3  4  5  6  7  8  9  10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27||\n");
        printf("||------------------------------------------------------------------------------------||\n");
#endif

        /* Write Spur Delta Phase, Spur Freq, and enable bits */
        writeField(devNum, "bb_enable_spur_filter", 1);
        writeField(devNum, "bb_enable_chan_mask", 1);
        writeField(devNum, "bb_enable_pilot_mask", 1);
		if (!isOwl(pLibDev->swDevID)) {
            writeField(devNum, "bb_use_spur_filter_in_agc", 1);
		}
//        writeField(devNum, "bb_use_spur_filter_in_selfcor", 1);
        writeField(devNum, "bb_spur_rssi_thresh", 40);
        writeField(devNum, "bb_enable_spur_rssi", 1);
        writeField(devNum, "bb_mask_rate_cntl", 255);
        writeField(devNum, "bb_mask_select", 240);
        writeField(devNum, "bb_spur_delta_phase", spurDeltaPhase);
        writeField(devNum, "bb_spur_freq_sd", spurFreqSd);
        /* Write pilot masks */
        writeField(devNum, "bb_pilot_mask_1", pilotMask[0]);
        writeField(devNum, "bb_pilot_mask_2", pilotMask[1]);
        writeField(devNum, "bb_chan_mask_1", pilotMask[0]);
        writeField(devNum, "bb_chan_mask_2", pilotMask[1]);
        /* Write magnitude masks */
        REGW(devNum, 0x9900, binMagMask[0]);
        REGW(devNum, 0x9904, binMagMask[1]);
        REGW(devNum, 0x9908, binMagMask[2]);
        REGW(devNum, 0x990c, (REGR(devNum, 0x990c) & 0xffffc000) | binMagMask[3]);
        REGW(devNum, 0x9988, binMagMask[0]);
        REGW(devNum, 0x998c, binMagMask[1]);
        REGW(devNum, 0x9990, binMagMask[2]);
        REGW(devNum, 0x9994, (REGR(devNum, 0x9994) & 0xffffc000) | binMagMask[3]);
    }
    return;
}

/**************************************************************************
* rereadProm - reset the EEPROM information
*
*/
MANLIB_API void rereadProm
(
 A_UINT32 devNum
)
{
    A_UINT16 i;
    if (checkDevNum(devNum) == FALSE) {
        mError(devNum, EINVAL, "Device Number %d:rereadProm\n", devNum);
        return;
    }
    gLibInfo.pLibDevArray[devNum]->eepData.eepromChecked = FALSE;
    for (i = 0; i < 4; i++) {
        gLibInfo.pLibDevArray[devNum]->eepromHeaderApplied[i] = FALSE;
    }

    //free up the 16K eeprom struct if needed
    freeEepStructs(devNum);
//  printf("After rereadProm \n");
}

/**************************************************************************
* eepromRead - call correct eepromRead Function
*
* RETURNS: 16 bit value from given offset (in a 32-bit value)
*/
MANLIB_API A_UINT32 eepromRead
(
 A_UINT32 devNum,
 A_UINT32 eepromOffset
)
{
    LIB_DEV_INFO *pLibDev = gLibInfo.pLibDevArray[devNum];
    A_UINT32 i = pLibDev->ar5kInitIndex;

    if (checkDevNum(devNum) == FALSE) {
        mError(devNum, EINVAL, "Device Number %d:eepromRead\n", devNum);
        return 0xdeadbeef;
    }

    if (ar5kInitData[i].pMacAPI->eepromRead == NULL)
    {
        printf(" In ar5kInitData\n");
        return(0xdeadbeef);
    }

    if (pLibDev->devMap.remoteLib)
        {
            //  printf("\n In IF eepromRead = %d \n",eepromOffset);
                return (pLibDev->devMap.r_eepromRead(devNum, eepromOffset));

        }
    else
        {
            //printf("\n In ELSE eepromRead = %d \n",eepromOffset);
                return (ar5kInitData[i].pMacAPI->eepromRead(devNum, eepromOffset));

        }
}

/**************************************************************************
* eepromWrite - Call correct eepromWrite function
*
*/
MANLIB_API void eepromWrite
(
 A_UINT32 devNum,
 A_UINT32 eepromOffset,
 A_UINT32 eepromValue
)
{
    LIB_DEV_INFO *pLibDev = gLibInfo.pLibDevArray[devNum];
    A_UINT32 i = pLibDev->ar5kInitIndex;

    if (checkDevNum(devNum) == FALSE) {
        mError(devNum, EINVAL, "Device Number %d:eepromWrite\n", devNum);
        return;
    }

    if (ar5kInitData[i].pMacAPI->eepromWrite == NULL)
    {
        return;
    }
    if (pLibDev->devMap.remoteLib)
       pLibDev->devMap.r_eepromWrite(devNum, eepromOffset, eepromValue);
    else
        ar5kInitData[i].pMacAPI->eepromWrite(devNum, eepromOffset, eepromValue);
    return;
}




/**************************************************************************
* setAntenna - Change antenna to the given antenna A or B
*
*/
MANLIB_API void setAntenna
(
 A_UINT32 devNum,
 A_UINT32 antenna
)
{
    LIB_DEV_INFO *pLibDev = gLibInfo.pLibDevArray[devNum];
    A_UINT32    antMode = 0;

    if (checkDevNum(devNum) == FALSE) {
        mError(devNum, EINVAL, "Device Number %d:setAntenna\n", devNum);
        return;
    }
    if (!ar5kInitData[pLibDev->ar5kInitIndex].pMacAPI->setupAntenna(devNum, antenna, &antMode))
    {
        return;
    }

}

/**************************************************************************
* setTransmitPower - Adjusts output power to a percentage of maximum.
*   resetDevice or changeChannel must be called before the setting takes affect.
*   powerScale is ignored if the EEPROM is not at least rev 1
*
*/
MANLIB_API void setPowerScale
(
 A_UINT32 devNum,
 A_UINT32 powerScale
)
{
    if (checkDevNum(devNum) == FALSE) {
        mError(devNum, EINVAL, "Device Number %d:setTransmitPower\n", devNum);
        return;
    }
    if((powerScale < TP_SCALE_LOWEST) || (powerScale > TP_SCALE_HIGHEST)) {
        mError(devNum, EINVAL, "Device Number %d:setPowerScale: Invalid powerScale option: %d\n", devNum, powerScale);
        return;
    }
    gLibInfo.pLibDevArray[devNum]->txPowerData.tpScale = (A_UINT16)powerScale;
    return;
}

/**************************************************************************
* setTransmitPower - Calls the correct setTransmitPower function based on
*       deviceID
*
*
*/
MANLIB_API void setTransmitPower
(
 A_UINT32 devNum,
 A_UCHAR txPowerArray[17]
)
{

    if (checkDevNum(devNum) == FALSE) {
        mError(devNum, EINVAL, "Device Number %d:setTransmitPower\n", devNum);
        return;
    }
    if (gLibInfo.pLibDevArray[devNum]->devState < RESET_STATE) {
        mError(devNum, EILSEQ, "Device Number %d:setTransmitPower: Device should be out of Reset before changing transmit power\n", devNum);
        return;
    }

    borrowAnalogAccess(devNum);

    initializeTransmitPower(devNum, 0, 1, txPowerArray);
    mSleep(1);

    returnAnalogAccess(devNum);
    mSleep(1);
}


/**************************************************************************
* setSingleTransmitPower - Set one pcdac value to be copied to all rates
*   with zero gain_delta.
*   Ignores power scaling but will be reset by resetDevice or changeChannel.
*
*/
MANLIB_API void setSingleTransmitPower
(
 A_UINT32 devNum,
 A_UCHAR pcdac
)
{
    LIB_DEV_INFO *pLibDev = gLibInfo.pLibDevArray[devNum];

    if (checkDevNum(devNum) == FALSE) {
        mError(devNum, EINVAL, "Device Number %d:setSingleTransmitPower\n", devNum);
        return;
    }
    if (gLibInfo.pLibDevArray[devNum]->devState < RESET_STATE) {
        mError(devNum, EILSEQ, "Device Number %d:setSingleTransmitPower: Device should be out of Reset before changing transmit power\n", devNum);
        return;
    }

    ar5kInitData[pLibDev->ar5kInitIndex].pRfAPI->setSinglePower(devNum, pcdac);

}


/**************************************************************************
* devSleep - Put the device to sleep for sleep power measurements
*
*/
MANLIB_API void devSleep
(
 A_UINT32 devNum
)
{

    if (checkDevNum(devNum) == FALSE) {
        mError(devNum, EINVAL, "Device Number %d:devSleep\n", devNum);
        return;
    }
    if (gLibInfo.pLibDevArray[devNum]->devState < RESET_STATE) {
        mError(devNum, EILSEQ, "Device Number %d:devSleep: Device should be out of Reset before entering sleep\n", devNum);
        return;
    }

#if defined(MDK_AP)
    mError(devNum, EINVAL, "Device Number %d:devSleep: AP cannot go to sleep \n", devNum);
#else
    REGW(devNum, F2_SFR, F2_SFR_SLEEP);
#endif
}

MANLIB_API A_UINT32 checkProm
(
 A_UINT32 devNum,
 A_UINT32 enablePrint
)
{
    LIB_DEV_INFO *pLibDev = gLibInfo.pLibDevArray[devNum];

    if (checkDevNum(devNum) == FALSE) {
        mError(devNum, EINVAL, "Device Number %d:printProm\n", devNum);
        return 1;
    }

    //  printf("After CheckDenNUm in checkprom \n");
    if (gLibInfo.pLibDevArray[devNum]->devState < RESET_STATE) {
        mError(devNum, EILSEQ, "Device Number %d:printProm: Device should be out of Reset before checking the EEPROM\n", devNum);
        return 1;
    }

    //check that the info fromt the eeprom is valid
    if(!pLibDev->eepData.infoValid) {
        mError(devNum,EILSEQ, "Device Number %d:checkProm: eeprom info is not valid\n", devNum);
                    return 1;
    }
    if(((pLibDev->eepData.version >> 12) & 0xF) == 1) {
#ifndef __ATH_DJGPPDOS__
        check5210Prom(devNum, enablePrint);
    //  printf("After Check5210Prom \n");
#endif
    }
    else if(((pLibDev->eepData.version >> 12) & 0xF) >= 3) {

    }
    else{
        mError(devNum, EIO, "Device Number %d:checkProm: Wrong PROM Version to print\n", devNum);
        return 1;
    }
    //  printf("checkProm\n");
    return 0;
}


/**************************************************************************
* setupEEPromMap - Read the EEPROM and setup the structure
*
* Returns: TRUE if the EEPROM is calibrated, else FALSE
*/
A_BOOL setupEEPromMap
(
 A_UINT32 devNum
)
{
    LIB_DEV_INFO *pLibDev = gLibInfo.pLibDevArray[devNum];
    A_UINT32     pciReg;

    // return if the eeprom has already been loaded
    if(pLibDev->eepData.eepromChecked == TRUE) {
        return pLibDev->eepData.infoValid;
    }

    if (isDragon_sd(pLibDev->swDevID)) {
#if !defined(FREEDOM_AP)
#ifdef LINUX
        ar6000EepromAttach(devNum);
#endif
#endif
    }
	else if (isOwl(pLibDev->swDevID)) {
		//about to call the eeprom read code
//printf("SNOOP: about to call the new reading code\n");
		if(!ar5416EepromAttach(devNum)) {
			return FALSE;
		}
//printf("SNOOP: called the new reading code\n");
	}
#ifdef OWL_PB42
	else if (isHowlAP(devNum)) {
		//about to call the eeprom read code
//printf("SNOOP: about to call the new reading code\n");
		if(!ar5416EepromAttach(devNum)) {
			return FALSE;
		}
//printf("SNOOP: called the new reading code\n");
	}
#endif
	else {
        // Setup the PCI Config space for correct card access on the first reset
        // Setup memory window, bus mastering, & SERR
        pciReg = pLibDev->devMap.OScfgRead(devNum, F2_PCI_CMD);
        pciReg |= (MEM_ACCESS_ENABLE | MASTER_ENABLE | SYSTEMERROR_ENABLE);
        pciReg &= ~MEM_WRITE_INVALIDATE; // Disable write & invalidate for our device
        pLibDev->devMap.OScfgWrite(devNum, F2_PCI_CMD, pciReg);

        pciReg = pLibDev->devMap.OScfgRead(devNum, F2_PCI_CACHELINESIZE);
        //changed this to not write the cacheline size, only want to write the latency timer
        pciReg = (pciReg & 0xffff00ff) | (0x40 << 8);
        pLibDev->devMap.OScfgWrite(devNum, F2_PCI_CACHELINESIZE, pciReg);

        pLibDev->eepData.version = (A_UINT16) eepromRead(devNum, (ATHEROS_EEPROM_OFFSET + 1));
        pLibDev->eepData.protect = (A_UINT16) eepromRead(devNum, (EEPROM_PROTECT_OFFSET));

        if(((pLibDev->eepData.version >> 12) & 0xF) < 2) {
#ifndef __ATH_DJGPPDOS__
            read5210eepData(devNum);
#endif
        }

        else if(((pLibDev->eepData.version >> 12) & 0xF) == 2) {
            mError(devNum, EIO, "Device Number %d:Version 2 EEPROM not supported \n", devNum);
        } else if ((((pLibDev->eepData.version >> 12) & 0xF) == 3) || (((pLibDev->eepData.version >> 12) & 0xF) >= 4)) {
            if(!readEEPData_16K(devNum)) {
                mError(devNum, EIO, "Device Number %d:Unable to read 16K eeprom info\n", devNum);
                pLibDev->eepData.eepromChecked = TRUE;
                pLibDev->eepData.infoValid = FALSE;
                return FALSE;
            }
//          printf("Middle setupEEPromMap\n");
            if ((((pLibDev->eepData.version >> 12) & 0xF) >= 4) &&
                (((pLibDev->eepData.version >> 12) & 0xF) <= 5))
            {
                //read the EAR
                if(!readEar(devNum, pLibDev->p16kEepHeader->earStartLocation)) {
                    mError(devNum, EIO, "Unable to read EAR information from EEPROM\n");
                //  printf(" In IF read Ear\n");
                    pLibDev->eepData.eepromChecked = TRUE;
                    pLibDev->eepData.infoValid = FALSE;
                    return FALSE;
                }
            }
            pLibDev->eepromHeaderChecked = TRUE;
        }


        else {
            mError(devNum, EIO, "Device Number %d:setupEEPromMap: Invalid version found: 0x%04X\n", devNum, pLibDev->eepData.version);
            pLibDev->eepData.eepromChecked = TRUE;
            pLibDev->eepData.infoValid = FALSE;
        //  printf(" In else read Ear\n");
            return FALSE;
        }
    }
    pLibDev->eepData.infoValid = TRUE;
    pLibDev->eepData.eepromChecked = TRUE;
    //  printf(" In read Ear\n");
    return TRUE;
    //  printf("End setupEEPromMap\n");
}

#ifdef HEADER_LOAD_SCHEME
A_BOOL setupEEPromHeaderMap
(
 A_UINT32 devNum
)
{
    LIB_DEV_INFO *pLibDev = gLibInfo.pLibDevArray[devNum];

    // Only check the EEPROM once
    if(pLibDev->eepromHeaderChecked == TRUE) {
        return TRUE;
    }

    allocateEepStructs(devNum);
    pLibDev->eepData.version = (A_UINT16) eepromRead(devNum, (ATHEROS_EEPROM_OFFSET + 1));
    if (((pLibDev->eepData.version >> 12) & 0xF) == 3) {
        readHeaderInfo(devNum, pLibDev->p16kEepHeader);
        pLibDev->eepromHeaderChecked = TRUE;
        return TRUE;
    }
    return FALSE;
}
#endif //HEADER_LOAD_SCHEME

void initializeTransmitPower
(
    A_UINT32 devNum,
    A_UINT32 freq,
    A_INT32  override,
    A_UCHAR  *pwrSettings
)
{
    LIB_DEV_INFO *pLibDev = gLibInfo.pLibDevArray[devNum];
    A_UINT32 i = pLibDev->ar5kInitIndex;


    if (ar5kInitData[i].pRfAPI->setPower == NULL)
    {
        return;
    }

    ar5kInitData[i].pRfAPI->setPower(devNum, freq, override, pwrSettings);
    return;
}




/**************************************************************************
* checkDevNum - Makes sure the device is initialized before using
*
* RETURNS: FALSE if device not yet initialized - else TRUE
*/
A_BOOL checkDevNum
(
 A_UINT32 devNum
)
{
    if(gLibInfo.pLibDevArray[devNum] == NULL) {
#ifndef NO_LIB_PRINT
        printf("Device Number %d:DevNum not initialized for function: ", devNum);
#endif
        return FALSE;
    }
    return TRUE;
}

/**************************************************************************
* reverseBits - Reverses bit_count bits in val
*
* RETURNS: Bit reversed value
*/
A_UINT32 reverseBits
(
 A_UINT32 val,
 int bit_count
)
{
    A_UINT32    retval = 0;
    A_UINT32    bit;
    int         i;

    for (i = 0; i < bit_count; i++)
    {
        bit = (val >> i) & 1;
        retval = (retval << 1) | bit;
    }
    return retval;
}

/**************************************************************************
* mError - output error messages
*
* This routine is the equivalent of printf.  It is used such that logging
* capabilities can be added.
*
* RETURNS: same as printf.  Number of characters printed
*/
int mError
(
    A_UINT32 devNum,
    A_UINT32 error,
    const char * format,
    ...
)
{

    LIB_DEV_INFO *pLibDev;
    va_list argList;
    int retval = 0;
#ifndef NO_LIB_PRINT
    char    buffer[256];
#endif

    /*if have logging turned on then can also write to a file if needed */
    /* get the arguement list */
    va_start(argList, format);

    /* using vprintf to perform the printing it is the same is printf, only
     * it takes a va_list or arguments
     */
#ifndef NO_LIB_PRINT
    printf("%s : ", strerror(error));
    retval = vprintf(format, argList);
    fflush(stdout);

#ifndef MDK_AP
#ifndef __ATH_DJGPPDOS__
    if (logging) {
        vsprintf(buffer, format, argList);
        fputs(buffer, logFileHandle);
        fflush(logFileHandle);
    }
#endif //__ATH_DJGPPDOS__
#endif //MDK_AP
#endif //NO_LIB_PRINT


    if(devNum != 0xdead) {
        //take a copy of the error buffer
        pLibDev = gLibInfo.pLibDevArray[devNum];
        vsprintf(pLibDev->mdkErrStr, format, argList);
        pLibDev->mdkErrno = error;
    }
    else {
        //this is a special case where initializeDevice failed,
        //before a devNum could be assigned, store these in static variables
        vsprintf(tempMDKErrStr, format, argList);
        tempMDKErrno = error;

    }
    va_end(argList);    /* cleanup arg list */

    return(retval);
}

/**************************************************************************
* getMdkErrStr - Get last error string
*
* This routine should only be called if mdkErrno is set.  It will copy
* the last error string into the callers buffer.  Callers buffer should
* be at least SIZE_ERROR_BUFFER bytes
*
* RETURNS: same as printf.  Number of characters printed
*/
MANLIB_API void getMdkErrStr
(
    A_UINT32 devNum,
    A_CHAR *pBuffer     //pointer to called allocated memory
)
{
    LIB_DEV_INFO *pLibDev;
    char *pErrString;

    if(devNum == 0xdead) {
        pErrString = tempMDKErrStr;
    }
    else {
        pLibDev = gLibInfo.pLibDevArray[devNum];
        pErrString = pLibDev->mdkErrStr;
    }
    strncpy(pBuffer, pErrString, SIZE_ERROR_BUFFER-1);
    return;
}

/**************************************************************************
* getLastErrorNo - get the last error number
*
* Returns : mdkErrno
*/
MANLIB_API A_INT32 getMdkErrNo
(
    A_UINT32 devNum
)
{
    LIB_DEV_INFO *pLibDev;
    A_INT32 returnValue;

    if(devNum == 0xdead) {
        returnValue = tempMDKErrno;
        tempMDKErrno = 0;
    }
    else {
        pLibDev = gLibInfo.pLibDevArray[devNum];
        returnValue = pLibDev->mdkErrno;
        pLibDev->mdkErrno = 0;
    }
    return returnValue;
}

#ifndef MDK_AP
/**************************************************************************
* enableLogging - enable logging of any lib messages to file
*
*
*
*/
MANLIB_API void enableLogging
(
 A_CHAR *pFilename
)
{
    logging = 1;

    logFileHandle = fopen(pFilename, "a+");

    if (logFileHandle == NULL) {
        printf( "Unable to open file for logging within library%s\n", pFilename);
        logging = 0;
    }
}

/**************************************************************************
* disableLogging - turn off logging flag
*
*
*/
MANLIB_API void disableLogging(void)
{
    logging = 0;
    fclose(logFileHandle);
    logFileHandle = NULL;
}
#endif

MANLIB_API void devlibCleanup()
{
    A_UINT32 i;

    //printf("SNOOP:::: devlibCleanup called \n");
    // cleanup all the devInfo structures
        for ( i = 0; i < LIB_MAX_DEV; i++ ) {
            if ( gLibInfo.pLibDevArray[i] ) {
                closeDevice(i);
                gLibInfo.pLibDevArray[i] = NULL;
            }
    }

}

MANLIB_API A_BOOL isFalconEmul(A_UINT32 devNum)
{
    LIB_DEV_INFO *pLibDev = gLibInfo.pLibDevArray[devNum];
    A_UINT32 swDevID;
    A_UINT32 hwDevID  = (pLibDev->devMap.OScfgRead(devNum, 0) >> 16) & 0xffff;
    A_BOOL   retVal = FALSE;
    if (pLibDev->regFileRead) {
        swDevID  = ar5kInitData[pLibDev->ar5kInitIndex].swDeviceID;
        if ( (swDevID == 0xe018) ||
            (swDevID == 0xfb40)) {
            retVal = TRUE ;
        }
    } else {
        if (hwDevID == 0xfb40)  {
            retVal = TRUE ;
        }
    }
    return(retVal);
}

MANLIB_API A_BOOL isFalcon_1_0(A_UINT32 devNum)
{
    LIB_DEV_INFO *pLibDev = gLibInfo.pLibDevArray[devNum];
    A_UINT32 swDevID;
    A_UINT32 hwDevID  = (pLibDev->devMap.OScfgRead(devNum, 0) >> 16) & 0xffff;

    if (pLibDev->regFileRead) {
        swDevID  = ar5kInitData[pLibDev->ar5kInitIndex].swDeviceID;
        if (swDevID == 0x0020) {
            return(TRUE) ;
        }
    } else {
        if ((hwDevID == 0xff18) || (hwDevID == 0x0020)){
            return(TRUE) ;
        }
    }
    return(FALSE);
}

MANLIB_API A_BOOL isFalcon(A_UINT32 devNum)
{
    return(isFalconEmul(devNum) || isFalcon_1_0(devNum));
}

MANLIB_API A_BOOL isFalconDeviceID(A_UINT32 swDevID)
{
    A_BOOL   retVal = FALSE;

    if ((swDevID == 0xe018) || (swDevID == 0xf018) || (swDevID == 0x0020)) {
        retVal = TRUE;
    }

    return(retVal);
}

MANLIB_API A_BOOL large_pci_addresses(A_UINT32 devNum)
{
    return(isFalcon(devNum));
}

MANLIB_API void setChain
(
    A_UINT32 devNum,
    A_UINT32 chain,
    A_UINT32 phase
)
{
    LIB_DEV_INFO *pLibDev = gLibInfo.pLibDevArray[devNum];

    if (!isFalcon(devNum)) {
        return;
    }

    init_beamforming_weights(devNum);

    pLibDev->libCfgParams.chainSelect = chain;
    writeField(devNum, "bb_rx_chain_mask", ((chain + 1) & pLibDev->libCfgParams.rx_chain_mask));

//printf("SNOOP: setchain : rx_chain_mask set to 0x%x\n", ((chain + 1) & pLibDev->libCfgParams.rx_chain_mask));
//  writeField(devNum, "bb_tx_chain_mask", ((chain == 2) ? 1 : (chain + 1)));
    writeField(devNum, "bb_tx_chain_mask", ((chain + 1) & pLibDev->libCfgParams.tx_chain_mask));
//printf("SNOOP: setchain : tx_chain_mask set to 0x%x\n", ((chain + 1) & pLibDev->libCfgParams.tx_chain_mask));
//  writeField(devNum,"bb_eirp_limited", 1);

    if (chain < 2) {
        writeField(devNum, "bb_use_static_txbf_cntl", 1);
        writeField(devNum, "bb_static_txbf_enable", 0);
        writeField(devNum, "bb_use_static_rx_coef_idx", 1);
        writeField(devNum, "bb_bf_weight_update", 0);
        writeField(devNum, "bb_use_static_chain_sel", 1);
        writeField(devNum, "bb_static_chain_sel", chain);
    } else {
        writeField(devNum, "bb_use_static_txbf_cntl", 0);
        writeField(devNum, "bb_static_txbf_enable", 0);
        writeField(devNum, "bb_use_static_rx_coef_idx", 0);
        writeField(devNum, "bb_bf_weight_update", 1);
        writeField(devNum, "bb_use_static_chain_sel", 0);
        writeField(devNum, "bb_static_chain_sel", 0);
//      writeField(devNum, "bb_disable_txbf_ext_switch", 1);
//      writeField(devNum, "bb_disable_txbf_blna", 1);

        writeField(devNum, "bb_cf_phase_ramp_alpha0", 0);
        writeField(devNum, "bb_cf_phase_ramp_init0", phase*511/720);
        writeField(devNum, "bb_cf_phase_ramp_bias0", 0*31/45);
        writeField(devNum, "bb_cf_phase_ramp_enable0", 1);

        writeField(devNum, "chn1_bb_cf_phase_ramp_alpha0", 0);
        writeField(devNum, "chn1_bb_cf_phase_ramp_init0", 0);
        writeField(devNum, "chn1_bb_cf_phase_ramp_bias0", 0);
        writeField(devNum, "chn1_bb_cf_phase_ramp_enable0", 1);

        writeField(devNum, "bb_cf_phase_ramp_alpha1", 0);
        writeField(devNum, "bb_cf_phase_ramp_init1", phase*511/720);
        writeField(devNum, "bb_cf_phase_ramp_bias1", 0*31/45);
        writeField(devNum, "bb_cf_phase_ramp_enable1", 1);

        writeField(devNum, "chn1_bb_cf_phase_ramp_alpha1", 0);
        writeField(devNum, "chn1_bb_cf_phase_ramp_init1", 0);
        writeField(devNum, "chn1_bb_cf_phase_ramp_bias1", 0);
        writeField(devNum, "chn1_bb_cf_phase_ramp_enable1", 1);

        enable_beamforming(devNum); // also called in resetDevice
    }

}

void enable_beamforming
(
    A_UINT32 devNum
)
{
    LIB_DEV_INFO *pLibDev = gLibInfo.pLibDevArray[devNum];
    A_UCHAR  rxStation[6] = {0x10, 0x11, 0x11, 0x11, 0x11, 0x01};   // ASSUMPTION : rxstation Addr
    A_UCHAR  txStation[6] = {0x20, 0x22, 0x22, 0x22, 0x22, 0x02};   // Golden
    A_UINT32  ii, temp1, temp2, temp3, temp4;
    A_UINT32  key_idx = 0;
    A_UINT32  key_reg_5 ;
    A_UINT32  antChain0, antChain1;

    antChain0 = (pLibDev->libCfgParams.antenna & 0x1);
    antChain1 = ((pLibDev->libCfgParams.antenna >> 16) & 0x1);

    key_reg_5 = ((0x7 << 0) | (0x0 << 3) | (0x00 << 4) | (1 << 9) |
                           (antChain0 << 10) | (antChain1 << 11) | (antChain0 << 12) | (antChain1 << 13) | ((pLibDev->libCfgParams.chainSelect & 0x1) << 14)) ;

    for (ii=0; ii<8; ii++) {
        REGW(devNum, 0x8800 + 4*ii, 0x00000000);
    }

    temp1 = rxStation[0] | (rxStation[1] << 8) | (rxStation[2] << 16) | (rxStation[3] << 24);
    temp2 = rxStation[4] | (rxStation[5] << 8);

    temp3 = txStation[0] | (txStation[1] << 8) | (txStation[2] << 16) | (txStation[3] << 24);
    temp4 = txStation[4] | (txStation[5] << 8);

    for (key_idx = 0; key_idx < 12; key_idx++) {
        REGW(devNum, (0x8800 + ((8*key_idx)<<2) + 0x00), 0x00000000);  // key[31:0]
        REGW(devNum, (0x8800 + ((8*key_idx)<<2) + 0x04), 0x00000000);  // key[47:32]
        REGW(devNum, (0x8800 + ((8*key_idx)<<2) + 0x08), 0x00000000);  // key[79:48]
        REGW(devNum, (0x8800 + ((8*key_idx)<<2) + 0x0C), 0x00000000);  // key[95:80]
        REGW(devNum, (0x8800 + ((8*key_idx)<<2) + 0x10), 0x00000000);  // key[127:96]
        REGW(devNum, (0x8800 + ((8*key_idx)<<2) + 0x14), key_reg_5);   // key type / bf info
//      if (key_idx % 2 == 0) {
//        if (0) {
//            REGW(devNum, (0x8800 + ((8*key_idx)<<2) + 0x18), ( ((temp2 & 0x1) << 31) | (temp1 >> 1)));  // dest addr [32:1]
//            REGW(devNum, (0x8800 + ((8*key_idx)<<2) + 0x1C), ( (0x1 << 15) | (temp2 >> 1)));  // key_valid, dest addr [47:33]
//        } else {
            REGW(devNum, (0x8800 + ((8*key_idx)<<2) + 0x18), ( ((temp4 & 0x1) << 31) | (temp3 >> 1)));  // dest addr [32:1]
            REGW(devNum, (0x8800 + ((8*key_idx)<<2) + 0x1C), ( (0x1 << 15) | (temp4 >> 1)));  // key_valid, dest addr [47:33]
//        }
    }

    // reg 0x8120 setup
//  if (pLibDev->libCfgParams.tx_chain_mask == 0x3) {
//    if (1) {
//      printf("SNOOP: tx_chain_mask = 0x%x, enabling bf\n", pLibDev->libCfgParams.tx_chain_mask);
        writeField(devNum, "mc_bfcoef_enable", 1);
        writeField(devNum, "mc_bfcoef_update_self_gen", 1);
//    } else {
//      printf("SNOOP: tx_chain_mask = 0x%x, disabling bf\n", pLibDev->libCfgParams.tx_chain_mask);
//        writeField(devNum, "mc_bfcoef_enable", 0);
//        writeField(devNum, "mc_bfcoef_update_self_gen", 0);
//    }
    writeField(devNum, "mc_dual_chain_ant_mode", 1);
    writeField(devNum, "mc_falcon_desc_mode", 1);
    writeField(devNum, "mc_kc_rx_ant_update", 1);
    writeField(devNum, "mc_falcon_bb_interface", 1);

    // disable bf coeff aging flush
//  REGW(devNum, 0x81C0, 0);

    // enable bf for all 64 types of frames
    ii = 0;
    while (ii < 64) {
        temp1 = REGR(devNum, 0x8500 + (ii << 2));
        // enable bf update and on tx for normal and self_gen pkts
        REGW(devNum, (0x8500 + (ii << 2)), ((0x3 << 0) | (0x3 << 2) | (1 << 4) | temp1));  // ok to enable 2x2 with aggressive 2ms weight expiration per DB
//      REGW(devNum, (0x8500 + (ii << 2)), ((0x3 << 0) | (0x1 << 2) | (1 << 4) | temp1));  // disable bf on acks
//      REGW(devNum, (0x8500 + (ii << 2)), ((0x3 << 0) | (0x2 << 2) | (1 << 4) | temp1));  // disable bf on data
        ii++;
    }

    // initialize bf coeff aging table
    for (ii = 0; ii<8; ii++) {
        REGW(devNum, (0x8180 + 4*ii), 0x0);
    }

}

void init_beamforming_weights (A_UINT32 devNum) {

    A_UINT32   ii, offset;

    offset = 0x6c;

//  printf("\n********************\nSNOOP: initializing beamforming weights\n****************\n");
//mSleep(500);

    for (ii = 0; ii < 8; ii++) {
        REGW(devNum, 0xa400 + ii*offset, 0x3c083c08);
        REGW(devNum, 0xb400 + ii*offset, 0x27392939);
        REGW(devNum, 0xa404 + ii*offset, 0x3d0a3d08);
        REGW(devNum, 0xb404 + ii*offset, 0x27382738);
        REGW(devNum, 0xa408 + ii*offset, 0x3b0c3a0d);
        REGW(devNum, 0xb408 + ii*offset, 0x233d233b);
        REGW(devNum, 0xa40c + ii*offset, 0x3a0d3a0c);
        REGW(devNum, 0xb40c + ii*offset, 0x243a223a);
        REGW(devNum, 0xa410 + ii*offset, 0x3c0e3a0d);
        REGW(devNum, 0xb410 + ii*offset, 0x25382539);
        REGW(devNum, 0xa414 + ii*offset, 0x3c0a3a0b);
        REGW(devNum, 0xb414 + ii*offset, 0x28392637);
        REGW(devNum, 0xa418 + ii*offset, 0x3b0f3b0d);
        REGW(devNum, 0xb418 + ii*offset, 0x243a2538);
        REGW(devNum, 0xa41c + ii*offset, 0x3a0e390e);
        REGW(devNum, 0xb41c + ii*offset, 0x253a233b);
        REGW(devNum, 0xa420 + ii*offset, 0x3a0d3a0c);
        REGW(devNum, 0xb420 + ii*offset, 0x24382639);
        REGW(devNum, 0xa424 + ii*offset, 0x3a0f3a0f);
        REGW(devNum, 0xb424 + ii*offset, 0x243b223a);
        REGW(devNum, 0xa428 + ii*offset, 0x3c0a3a0d);
        REGW(devNum, 0xb428 + ii*offset, 0x2a382738);
        REGW(devNum, 0xa42c + ii*offset, 0x39093b0a);
        REGW(devNum, 0xb42c + ii*offset, 0x2936283a);
        REGW(devNum, 0xa430 + ii*offset, 0x39083b08);
        REGW(devNum, 0xb430 + ii*offset, 0x2b382d36);
        REGW(devNum, 0xa434 + ii*offset, 0x3a06a323);
        REGW(devNum, 0xb434 + ii*offset, 0x2b359f1f);
        REGW(devNum, 0xa438 + ii*offset, 0x3c023b03);
        REGW(devNum, 0xb438 + ii*offset, 0x2f332d33);
        REGW(devNum, 0xa43c + ii*offset, 0x3a033b04);
        REGW(devNum, 0xb43c + ii*offset, 0x2d352d34);
        REGW(devNum, 0xa440 + ii*offset, 0x3d033c02);
        REGW(devNum, 0xb440 + ii*offset, 0x31322f33);
        REGW(devNum, 0xa444 + ii*offset, 0x3c013c02);
        REGW(devNum, 0xb444 + ii*offset, 0x2f333131);
        REGW(devNum, 0xa448 + ii*offset, 0x3c003b03);
        REGW(devNum, 0xb448 + ii*offset, 0x30312e31);
        REGW(devNum, 0xa44c + ii*offset, 0x3e013d00);
        REGW(devNum, 0xb44c + ii*offset, 0x322e2f31);
        REGW(devNum, 0xa450 + ii*offset, 0x3aff3dff);
        REGW(devNum, 0xb450 + ii*offset, 0x312e322c);
        REGW(devNum, 0xa454 + ii*offset, 0x3dfe3cff);
        REGW(devNum, 0xb454 + ii*offset, 0x332b332e);
        REGW(devNum, 0xa458 + ii*offset, 0x3e013eff);
        REGW(devNum, 0xb458 + ii*offset, 0x302e332b);
        REGW(devNum, 0xa45c + ii*offset, 0x3c023c05);
        REGW(devNum, 0xb45c + ii*offset, 0x2f2e2e31);
        REGW(devNum, 0xa460 + ii*offset, 0x3e023c01);
        REGW(devNum, 0xb460 + ii*offset, 0x312e302d);
        REGW(devNum, 0xa464 + ii*offset, 0x3e0a3d03);
        REGW(devNum, 0xb464 + ii*offset, 0x2c35302c);
        REGW(devNum, 0xa468 + ii*offset, 0x5512390e);
        REGW(devNum, 0xb468 + ii*offset, 0x5d0b2636);

    }
}

MANLIB_API A_BOOL isPredator(A_UINT32 swDeviceID)
{
    if (swDeviceID == SW_DEVICE_ID_PREDATOR) {
        return TRUE;
    } else {
        return FALSE;
    }
}

MANLIB_API A_BOOL isGriffin(A_UINT32 swDeviceID)
{
    if ((swDeviceID == 0x0018) ||
        (swDeviceID == SW_DEVICE_ID_AP51) ||
        (swDeviceID == DEVICE_ID_SPIDER1_0) ||
        (swDeviceID == DEVICE_ID_SPIDER2_0) ||
		(swDeviceID == SW_DEVICE_ID_NALA)) {
        return TRUE;
    } else {
        return FALSE;
    }
}

MANLIB_API A_BOOL isSpider(A_UINT32 swDeviceID)
{
    if ((swDeviceID == DEVICE_ID_SPIDER1_0) ||
        (swDeviceID == DEVICE_ID_SPIDER2_0)) {
printf("Snoop: Is spider returned true\n");
        return TRUE;
    } else {
        return FALSE;
    }
}

MANLIB_API A_BOOL isCobra(A_UINT32 swDeviceID)
{
    if (swDeviceID == SW_DEVICE_ID_AP51 || swDeviceID == DEVICE_ID_SPIDER1_0 ||
		swDeviceID == DEVICE_ID_SPIDER2_0) {
        return TRUE;
    } else {
        return FALSE;
    }
}
MANLIB_API A_BOOL isGriffin_1_0(A_UINT32 macRev)
{
    if (macRev == 0x74) {
        return TRUE;
    } else {
        return FALSE;
    }
}

MANLIB_API A_BOOL isGriffin_1_1(A_UINT32 macRev)
{
    if (macRev == 0x75) {
        return TRUE;
    } else {
        return FALSE;
    }
}

MANLIB_API A_BOOL isGriffin_2_0(A_UINT32 macRev)
{
    if (macRev == 0x76) {
        return TRUE;
    } else {
        return FALSE;
    }
}

MANLIB_API A_BOOL isGriffin_2_1(A_UINT32 macRev)
{
    if (macRev == 0x79 || macRev == 0xb0) {
        return TRUE;
    } else {
        return FALSE;
    }
}

MANLIB_API A_BOOL isGriffin_lite(A_UINT32 macRev)
{
    if (macRev == 0x78) {
        return TRUE;
    } else {
        return FALSE;
    }
}

MANLIB_API A_BOOL isEagle_lite(A_UINT32 macRev)
{
    if (macRev == 0xa4) {
        return TRUE;
    } else {
        return FALSE;
    }
}

MANLIB_API A_BOOL needsUartPciCfg(A_UINT32 swDeviceID)
{
    if (isGriffin(swDeviceID) && !isCobra(swDeviceID)) {
        return TRUE;
    } else {
        return FALSE;
    }
}

void memDisplay(A_UINT32 devNum, A_UINT32 address, A_UINT32 nWords) {
  A_UINT32 iIndex, memValue;
    LIB_DEV_INFO *pLibDev;

    pLibDev = gLibInfo.pLibDevArray[devNum];

  for(iIndex=0; iIndex<nWords; iIndex++) {
    pLibDev->devMap.OSmemRead(devNum, address+(iIndex*4), (A_UCHAR *)&memValue, sizeof(memValue));
    printf("Word %d (addr %x)=%x\n", iIndex, (address+ (iIndex*4)), memValue);
  }

}

#ifdef OWL_PB42
// This routine will be used to distinguish between Howl AP vs Hydra AP (PB42)
MANLIB_API A_BOOL isHowlAP(A_UINT32 devIndex)
{
   static A_BOOL flag=FALSE;
   static A_BOOL howl_ap=FALSE;
   A_UINT32 chip_id;
   if(flag){
     return howl_ap;
   }else{
     flag=1;
     chip_id=get_chip_id(devIndex);
     printf("chip id: %x\n",chip_id);
     if(((chip_id&0x000000ff)==0xb0)||((chip_id&0x000000ff)==0xb1)||((chip_id&0x000000ff)==0xb4)||((chip_id&0x000000ff)==0xb5)||((chip_id&0x000000ff)==0xb6)||((chip_id&0x000000ff)==0xb7)||((chip_id&0x000000ff)==0xb8)||((chip_id&0x000000ff)==0xb9)){
       howl_ap=TRUE;
     }
       return howl_ap;
   }
}


MANLIB_API A_BOOL isPythonAP(A_UINT32 devIndex)
{
   static A_BOOL flag=FALSE;
   static A_BOOL python_ap=FALSE;
   A_UINT32 chip_id;
   if(flag){
     return python_ap;
   }else{
     flag=1;
     chip_id=get_chip_id(devIndex);
     printf("chip id: %x\n",chip_id);
     if(((chip_id&0x000000ff)==0xc0)||((chip_id&0x000000ff)==0xc1)||((chip_id&0x000000ff)==0xc2)||((chip_id&0x000000ff)==0xc3)||((chip_id&0x000000ff)==0xc4)||((chip_id&0x000000ff)==0xc5)||((chip_id&0x000000ff)==0xc6)||((chip_id&0x000000ff)==0xc7)){
       python_ap=TRUE;
     }
       return python_ap;
   }
}

#endif

MANLIB_API A_BOOL isEagle(A_UINT32 swDeviceID)
{
    if ((swDeviceID == SW_DEVICE_ID_EAGLE) || (swDeviceID == SW_DEVICE_ID_DRAGON)) {
        return TRUE;
    } else if (isCondor(swDeviceID)) {
        return TRUE;
    } else {
        return FALSE;
    }
}

MANLIB_API A_BOOL isOwl(A_UINT32 swDeviceID)
{
    if ((swDeviceID == SW_DEVICE_ID_OWL)  ||
        (swDeviceID == SW_DEVICE_ID_SOWL) ||
        (swDeviceID == SW_DEVICE_ID_HOWL) ||
        (swDeviceID == SW_DEVICE_ID_MERLIN))
    {
        return TRUE;
    } else {
        return FALSE;
    }
}

MANLIB_API A_BOOL isMerlin(A_UINT32 swDeviceID)
{
#ifdef FJC_TEST
    return TRUE;
#endif
    if ((swDeviceID == SW_DEVICE_ID_MERLIN) || (swDeviceID == SW_DEVICE_ID_MERLIN_PCIE)){
        return TRUE;
    } else {
        return FALSE;
    }

}


//check for engineering samples of merlin
MANLIB_API A_BOOL isMerlin_Eng(A_UINT32 macRev)
{
    if(((macRev & MERLIN_MAC_REV_MASK) == MERLIN_1207) ||
        ((macRev & MERLIN_MAC_REV_MASK) == MERLIN_0108)){
        return TRUE;
    }
    return FALSE;
}

MANLIB_API A_BOOL isMerlin_0108(A_UINT32 macRev)
{
	if((macRev & MERLIN_MAC_REV_MASK) == MERLIN_0108) {
         return TRUE;
    }
    return FALSE;
}

MANLIB_API A_BOOL isMerlin_0208(A_UINT32 macRev)
{
    if((macRev & MERLIN_MAC_REV_MASK) == MERLIN_0208) {
         return TRUE;
    }
    return FALSE;
}

MANLIB_API A_BOOL isMerlinPcie(A_UINT32 macRev)
{
	A_UINT32 pcieRev;

	pcieRev = macRev & 0x000ff000;

	if( (pcieRev == 0x00080000) ||
		(pcieRev == 0x00081000) ||
		(pcieRev == 0x00084000) ||
		(pcieRev == 0x00085000)) {
		return TRUE;
	}
	return FALSE;
}

MANLIB_API A_BOOL isMerlinPowerControl(A_UINT32 swDeviceID)
{
#ifdef FJC_TEST
    return TRUE;
#endif
    return(isMerlin(swDeviceID));
}

MANLIB_API A_BOOL isNala(A_UINT32 swDeviceID)
{
	if(swDeviceID == SW_DEVICE_ID_NALA) {
		return TRUE;
	} else {
		return FALSE;
	}
}

MANLIB_API A_BOOL is11nDeviceID(A_UINT32 swDeviceID)
{
	return(isOwl(swDeviceID));
}

MANLIB_API A_BOOL isEagle_1_0(A_UINT32 macRev)
{
    if (macRev == 0xa1) {
        return TRUE;
    } else {
        return FALSE;
    }
}


MANLIB_API A_BOOL isCondor(A_UINT32 swDeviceID)
{
    if (swDeviceID == SW_DEVICE_ID_CONDOR) {
        return TRUE;
    } else {
        return FALSE;
    }
}

MANLIB_API A_BOOL isDragon(A_UINT32 devNum)
{
    LIB_DEV_INFO *pLibDev = gLibInfo.pLibDevArray[devNum];
    A_UINT32 swDevID;

    if (pLibDev->ar5kInitIndex == UNKNOWN_INIT_INDEX) {
       printf("isDragon:ERROR:unknown index %d\n", pLibDev->ar5kInitIndex);
       return FALSE;
    }

    swDevID  = ar5kInitData[pLibDev->ar5kInitIndex].swDeviceID;
    if (swDevID == SW_DEVICE_ID_DRAGON) {
        return TRUE;
    } else {
        return FALSE;
    }
}


MANLIB_API A_BOOL isEagle_d(A_UINT32 devNum)
{
    LIB_DEV_INFO *pLibDev = gLibInfo.pLibDevArray[devNum];
    A_UINT32 swDevID;

    if (pLibDev->ar5kInitIndex == UNKNOWN_INIT_INDEX) {
        printf("isEagle_d:ERROR:unknown index %d\n", pLibDev->ar5kInitIndex);
        return FALSE;
    }

    swDevID  = ar5kInitData[pLibDev->ar5kInitIndex].swDeviceID;
    if (swDevID == SW_DEVICE_ID_EAGLE) {
        return TRUE;
    } else {
        return FALSE;
    }
}


MANLIB_API A_BOOL isOwl_2_Plus(A_UINT32 macRev)
{

	// If Sowl return true (this takes care of merlin also)
    if ((macRev & 0xff) == 0xff) {
		return TRUE;
	}

	// For howl mac version number is 0x140. It is of neither old nor new version format.
    if (((macRev & 0xff) == 0x40) || ((macRev & 0xff) == 0x00)) {
		return TRUE;
	}

	if ( (((macRev & ~7) == 0xc8) || ((macRev & ~7) == 0xd8)) &&
        ((macRev & 0x7) >= 1) )
    {
        return TRUE;
    } else {
        return FALSE;
    }
}

MANLIB_API A_BOOL isSowl_Emulation(A_UINT32 macRev)
{
    if(macRev == 0x4f0ff)
    {
        return TRUE;
    } else {
        return FALSE;
    }
}

MANLIB_API A_BOOL isSowl(A_UINT32 macRev)
{
    if ((macRev & NEW_MAC_ID_VER_MASK) == SOWL_MAC_ID) {
        return TRUE;
    } else {
        return FALSE;
    }
}

MANLIB_API A_BOOL isSowl1_1(A_UINT32 macRev)
{
    if ((macRev & NEW_MAC_ID_VER_MASK) == SOWL_MAC_ID) { // first check if this is sowl
        if((macRev&0x100) == 0x100) { // check only revision number bit
            return TRUE;
        } else {
            return FALSE;
        }
    } else {
        return FALSE;
    }
}

MANLIB_API A_BOOL isMerlin_Fowl_Emulation(A_UINT32 macRev)
{
    if(macRev == 0x8f0ff)
    {
        return TRUE;
    } else {
        return FALSE;
    }
}

MANLIB_API A_BOOL isDragon_sd(A_UINT32 swDevID)
{
    if (swDevID == SW_DEVICE_ID_DRAGON) {
        return TRUE;
    } else {
        return FALSE;
    }
}

/* Grants analog access (uses newer mechanism for recent designs). */
A_BOOL borrowAnalogAccess(A_UINT32 devNum)
{
    LIB_DEV_INFO *pLibDev = gLibInfo.pLibDevArray[devNum];
    A_UINT32 mlCount;

    if (isGriffin(pLibDev->swDevID) || isEagle(pLibDev->swDevID) || isOwl(pLibDev->swDevID)) {
        REGW(devNum, PHY_RFBUS_REQ, PHY_RFBUS_REQ_REQUEST); /* Request analog bus grant */
        for (mlCount = 0; mlCount < 100; mlCount++) {
            if (REGR(devNum, PHY_RFBUS_GNT)) {
                break;
            }
            mSleep(5);
        }
        if (mlCount >= 100) {
            printf("Failed to grant analog bus access within %d ms\n", 500);
            return FALSE;
        }
    } else {
        REGW(devNum, 0x9808, REGR(devNum, 0x9808) | 0x08000000);
    }
    return TRUE;
}

void returnAnalogAccess(A_UINT32 devNum)
{
    LIB_DEV_INFO *pLibDev = gLibInfo.pLibDevArray[devNum];

    if (isGriffin(pLibDev->swDevID) || isEagle(pLibDev->swDevID) || isOwl(pLibDev->swDevID)) {
        /* Release the RFBus Grant */
        REGW(devNum, PHY_RFBUS_REQ, 0);
    } else {
        REGW(devNum, 0x9808, REGR(devNum, 0x9808) & (~0x08000000));
    }
}

// This function will check if single chain Base Band support exists in the device.
MANLIB_API A_BOOL isBB_single_chain(A_UINT32 macRev)
{
	// Starting from Sowl chipset 8 lsb of macRev is 0xff. For howl mac version number is 0x140.
    if (((macRev & 0xff) == 0xff) || ((macRev & 0xff) == 0x40)) {
		return TRUE;
	}

    return FALSE;
}

#ifdef OWL_PB42
MANLIB_API A_BOOL isFlashCalData()
{
    if(isPythonAP(0)){
    	int         to = 50000;
    	A_UINT32    eepromValue;
    	A_UINT32    status;
   	 //read the memory mapped eeprom location zero for magic number
   	eepromValue = REGR(0, (EEPROM_MEM_OFFSET));

    	//check busy bit to see if eeprom read succeeded and get valid data in read register
    	while (to > 0) {
        	status = REGR(0, EEPROM_STATUS_READ) & EEPROM_BUSY_M;
        	if (status == 0) {
            		eepromValue = REGR(0, EEPROM_STATUS_READ) & 0xffff;
            		break;
        	}
        	to--;
    	}
    	if ((eepromValue & 0x0000ffff)==0xa55a){
		return 0;   // Cal data in EEPROM of XB92 cards/ HB95 cards for TB317 boards
    	}
    	else{
        	return 1;   // AP93 cal data is in Flash
    	}
     }else{//if(isPythonAP(0)){
        return (cal_data_in_eeprom == 1) ? 0:1;
     }
}
#endif
