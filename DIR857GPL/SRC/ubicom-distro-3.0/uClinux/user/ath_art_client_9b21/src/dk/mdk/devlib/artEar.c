/* artEar.c - contians functions for managing EAR in ART */
/* Copyright (c) 2003 Atheros Communications, Inc., All Rights Reserved */

#ident  "ACI $Id: //depot/sw/src/dk/mdk/devlib/artEar.c#15 $, $Header: //depot/sw/src/dk/mdk/devlib/artEar.c#15 $"


#ifdef __ATH_DJGPPDOS__
#include <unistd.h>
#ifndef EILSEQ  
    #define EILSEQ EIO
#endif	// EILSEQ

#define __int64	long long
#define HANDLE long
typedef unsigned long DWORD;
#define Sleep	delay
#endif	// #ifdef __ATH_DJGPPDOS__

#include <stdlib.h>
#include <string.h>  // For memset() call
#include "wlantype.h"
#include "athreg.h"
#include "manlib.h"
#include "mConfig.h"
#include <errno.h>

#include "artEar.h"

int EarDebugLevel = EAR_DEBUG_OFF; //EAR_DEBUG_VERBOSE; 

static A_UINT16
ar5212EarDoRH(A_UINT32 devNum, REGISTER_HEADER *pRH);

static RF_REG_INFO *
ar5212GetRfBank(A_UINT32 devNum, A_UINT16 bank);

static A_UINT16
ar5212CFlagsToEarMode(A_UINT32 devNum);

static A_BOOL
ar5212IsChannelInEarRegHead(A_UINT16 earChannel, A_UINT16 channelMhz);

static void
ar5212ModifyRfBuffer(RF_REG_INFO *pRfRegs, A_UINT32 reg32, A_UINT32 numBits,
                     A_UINT32 firstBit, A_UINT32 column);

static void
showEarAlloc(EAR_ALLOC *earAlloc);

/******************** EAR Entry Routines ********************/
/**************************************************************
 * ar5212IsEarEngaged
 *
 * Given the reset mode and channel - does the EAR exist and
 * does it have any changes for this mode and channel
 */
A_BOOL
ar5212IsEarEngaged(A_UINT32 devNum, EAR_HEADER   *pEarHead, A_UINT32 freq)
{
    int          i;
   	LIB_DEV_INFO *pLibDev = gLibInfo.pLibDevArray[devNum];

    if(!pLibDev->libCfgParams.loadEar) 
		{
//				printf("In ar5212IsEarEngaged pLibDev->libCfgParams.loadEar\n");
				return FALSE;
		}

	/* Check for EAR */
    if (!pEarHead) 
		{
			//	printf("pEarHead\n");
        return FALSE;
    }

    /* Check for EAR found some reg headers */
    if (pEarHead->numRHs == 0) 
		{
      // printf("pEarHead->numRHs\n");
				return FALSE;
    }

    /* Check register headers for match on channel and mode */
    for (i = 0; i < pEarHead->numRHs; i++)
		{
        if ((pEarHead->pRH[i].modes & ar5212CFlagsToEarMode(devNum)) &&
            ar5212IsChannelInEarRegHead(pEarHead->pRH[i].channel, (A_UINT16)freq))
        {
			//printf("ar5212IsChannelInEarRegHead\n");
            return TRUE;
        }
    }

    devNum = 0;   //quiet warnings
	//	printf("ar5212IsearEngaged\n");
	return FALSE;
}

/**************************************************************
 * ar5212EarModify
 *
 * The location sensitive call the can enable/disable calibrations
 * or check for additive register changes
 */
A_BOOL
ar5212EarModify(A_UINT32 devNum, EAR_HEADER   *pEarHead, EAR_LOC_CHECK loc, A_UINT32 freq, A_UINT32 *modifier)
{
    int          i;

    if(pEarHead == NULL) {
		mError(devNum, EINVAL, "ar5212EarModify: earHead is NULL\n");
		return FALSE;
	}
    /* Should only be called if EAR is engaged */
    if(!ar5212IsEarEngaged(devNum, pEarHead, freq)) {
		mError(devNum, EINVAL, "ar5212EarModify: ear is not engaged\n");
		return FALSE;
	}

    /* Check to see if there are any EAR modifications for the given code location */
    for (i = 0; i < pEarHead->numRHs; i++) {
        /* Check register headers for match on channel and mode */
        if (pEarHead->pRH[i].modes & ar5212CFlagsToEarMode(devNum) &&
            ar5212IsChannelInEarRegHead(pEarHead->pRH[i].channel, (A_UINT16)freq))
        {
            switch (loc) {
                /* Stage: 0 */
            case EAR_LC_RF_WRITE:
                if (pEarHead->pRH[i].stage == EAR_STAGE_ANALOG_WRITE) {
                    if (EarDebugLevel >= EAR_DEBUG_VERBOSE) {
                        printf("ar5212EarModify: Performing stage 0 ops on RH %d\n", i);
                    }
                    ar5212EarDoRH(devNum, &(pEarHead->pRH[i]));
                }
                break;
                /* Stage: 1 */
            case EAR_LC_PHY_ENABLE:
                if (pEarHead->pRH[i].stage == EAR_STAGE_PRE_PHY_ENABLE) {
                    if (EarDebugLevel >= EAR_DEBUG_VERBOSE) {
                        printf("ar5212EarModify: Performing stage 1 ops on RH %d\n", i);
                    }
                    ar5212EarDoRH(devNum, &(pEarHead->pRH[i]));
                }
                break;
                /* Stage: 2 */
            case EAR_LC_POST_RESET:
                if (pEarHead->pRH[i].stage == EAR_STAGE_POST_RESET) {
                    if (EarDebugLevel >= EAR_DEBUG_VERBOSE) {
                        printf("ar5212EarModify: Performing stage 2 ops on RH %d\n", i);
                    }
                    ar5212EarDoRH(devNum, &(pEarHead->pRH[i]));
                }
                break;
                /* Stage: 3 */
            case EAR_LC_POST_PER_CAL:
                if (pEarHead->pRH[i].stage == EAR_STAGE_POST_PER_CAL) {
                    if (EarDebugLevel >= EAR_DEBUG_VERBOSE) {
                        printf("ar5212EarModify: Performing stage 3 ops on RH %d\n", i);
                    }
                    ar5212EarDoRH(devNum, &(pEarHead->pRH[i]));
                }
                break;
                /* Stage: Any */
            case EAR_LC_PLL:
                if (IS_PLL_SET(pEarHead->pRH[i].disabler.disableField)) {
                    *modifier = pEarHead->pRH[i].disabler.pllValue;
                    if (EarDebugLevel >= EAR_DEBUG_VERBOSE) {
                        printf("ar5212EarModify: Modifying PLL value!!! to %d\n", *modifier);
                    }
                    return TRUE;
                }
                break;
                /* Stage: Any */
            case EAR_LC_RESET_OFFSET:
                if ((pEarHead->pRH[i].disabler.valid) &&
                    !((pEarHead->pRH[i].disabler.disableField >> 8) & 0x1))
                {
                    if (EarDebugLevel >= EAR_DEBUG_VERBOSE) {
                        printf("ar5212EarModify: Disabling reset code Offset cal\n");
                    }
                    return TRUE;
                }
                break;
                /* Stage: Any */
            case EAR_LC_RESET_NF:
                if ((pEarHead->pRH[i].disabler.valid) &&
                    !((pEarHead->pRH[i].disabler.disableField >> 9) & 0x1))
                {
                    if (EarDebugLevel >= EAR_DEBUG_VERBOSE) {
                        printf("ar5212EarModify: Disabling reset code Noise Floor cal\n");
                    }
                    return TRUE;
                }
                break;
                /* Stage: Any */
            case EAR_LC_RESET_IQ:
                if ((pEarHead->pRH[i].disabler.valid) &&
                    !((pEarHead->pRH[i].disabler.disableField >> 10) & 0x1))
                {
                    if (EarDebugLevel >= EAR_DEBUG_VERBOSE) {
                        printf("ar5212EarModify: Disabling reset code IQ cal\n");
                    }
                    return TRUE;
                }
                break;
                /* Stage: Any */
            case EAR_LC_PER_FIXED_GAIN:
                if ((pEarHead->pRH[i].disabler.valid) &&
                    !((pEarHead->pRH[i].disabler.disableField >> 11) & 0x1))
                {
                    if (EarDebugLevel >= EAR_DEBUG_VERBOSE) {
                        printf("ar5212EarModify: Disabling periodic fixed gain calibration\n");
                    }
                    return TRUE;
                }
                break;
                /* Stage: Any */
            case EAR_LC_PER_NF:
                if ((pEarHead->pRH[i].disabler.valid) &&
                    !((pEarHead->pRH[i].disabler.disableField >> 12) & 0x1))
                {
                    if (EarDebugLevel >= EAR_DEBUG_VERBOSE) {
                        printf("ar5212EarModify: Disabling periodic noise floor cal\n");
                    }
                    return TRUE;
                }
                break;
                /* Stage: Any */
            case EAR_LC_PER_GAIN_CIRC:
                if ((pEarHead->pRH[i].disabler.valid) &&
                    !((pEarHead->pRH[i].disabler.disableField >> 13) & 0x1))
                {
                    if (EarDebugLevel >= EAR_DEBUG_VERBOSE) {
                        printf("ar5212EarModify: Disabling gain circulation\n");
                    }
                    return TRUE;
                }
                break;
            }
        }
    }
    /* TODO: Track modified analog registers outside of stage 0 and write them with protection */

    return FALSE;
}


/******************** EAR Support Routines ********************/

/**************************************************************
 * ar5212EarDoRH
 *
 * The given register header needs to be executed by performing
 * a register write, modify, or analog bank modify
 *
 * If an ananlog bank is modified - return the bank number as a
 * bit mask.  (bit 0 is bank 0)
 */
static A_UINT16
ar5212EarDoRH(A_UINT32 devNum, REGISTER_HEADER *pRH)
{
    A_UINT16 tag, regLoc = 0, num, opCode;
    A_UINT16 bitsThisWrite, numBits, startBit, bank, column, last, i;
    A_UINT32 address, reg32, mask, regInit;
    RF_REG_INFO *pRfRegs;
    A_UINT16 rfBankMask = 0;

    switch (pRH->type) {
    case EAR_TYPE0:
        do {
            address = pRH->regs[regLoc] & ~T0_TAG_M;
            tag = (A_UINT16)(pRH->regs[regLoc] & T0_TAG_M);
            regLoc++;
            if ((tag == T0_TAG_32BIT) || (tag == T0_TAG_32BIT_LAST)) {
                /* Full word writes */
                reg32 = pRH->regs[regLoc++] << 16;
                reg32 |= pRH->regs[regLoc++];
                if (EarDebugLevel >= EAR_DEBUG_VERBOSE) {
                    printf("ar5212EarDoRH: Type 0 EAR 32-bit write to 0x%04X : 0x%08x\n", address, reg32);
                }
                REGW(devNum, address, reg32);
            } else if (tag == T0_TAG_16BIT_LOW) {
                /* Half word read/modify/write low data */
                if (EarDebugLevel >= EAR_DEBUG_VERBOSE) {
                    printf("ar5212EarDoRH: Type 0 EAR 16-bit low write to 0x%04X : 0x%04x\n",
                             address, pRH->regs[regLoc]);
                }
                REG_RMW(devNum, address, 0xFFFF, pRH->regs[regLoc++]);
            } else {
                /* Half word read/modify/write high data */
                if (EarDebugLevel >= EAR_DEBUG_VERBOSE) {
                    printf("ar5212EarDoRH: Type 0 EAR 16-bit high write to 0x%04X : 0x%04x\n",
                             address, pRH->regs[regLoc]);
                }
                REG_RMW(devNum, address, 0xFFFF0000, pRH->regs[regLoc++] << 16);
            }
        } while (tag != T0_TAG_32BIT_LAST);
        break;
    case EAR_TYPE1:
        address = pRH->regs[regLoc] & ~T1_NUM_M;
        num = (A_UINT16)(pRH->regs[regLoc] & T1_NUM_M);
        regLoc++;
        for (i = 0; i < num + 1; i++) {
            /* Sequential Full word writes */
            reg32 = pRH->regs[regLoc++] << 16;
            reg32 |= pRH->regs[regLoc++];
            if (EarDebugLevel >= EAR_DEBUG_VERBOSE) {
                printf("ar5212EarDoRH: Type 1 EAR 32-bit write to 0x%04X : 0x%08x\n", address, reg32);
            }
            REGW(devNum, address, reg32);
            address += sizeof(A_UINT32);
        }
        break;
    case EAR_TYPE2:
        do {
            last = (A_UINT16)(IS_TYPE2_LAST(pRH->regs[regLoc]));
            bank = (A_UINT16)((pRH->regs[regLoc] & T2_BANK_M) >> T2_BANK_S);
            /* Record which bank is written */
            rfBankMask |= 1 << bank;
            column = (A_UINT16)((pRH->regs[regLoc] & T2_COL_M) >> T2_COL_S);
            startBit = (A_UINT16)(pRH->regs[regLoc] & T2_START_M);
            pRfRegs = ar5212GetRfBank(devNum, bank);
            if (IS_TYPE2_EXTENDED(pRH->regs[regLoc++])) {
                num = (A_UINT16)(A_DIV_UP(pRH->regs[regLoc], 16));
                numBits = pRH->regs[regLoc];
                regLoc++;
                for (i = 0; i < num; i++) {
                    /* Modify the bank 16 bits at a time */
                    bitsThisWrite = (numBits > 16) ? (A_UINT16)16 : numBits;
                    numBits = (A_UINT16)(numBits - bitsThisWrite);
                    reg32 = pRH->regs[regLoc++];
                    if (EarDebugLevel >= EAR_DEBUG_VERBOSE) {
                        printf("ar5212EarDoRH: Type 2 EAR E analog modify bank %d, startBit %d, column %d numBits %d data %d\n",
                                 bank, startBit, column, bitsThisWrite, reg32);
                    }
                    ar5212ModifyRfBuffer(pRfRegs, reg32, bitsThisWrite, startBit, column);
                    startBit = (A_UINT16)(startBit + bitsThisWrite);
                }
            } else {
                numBits = (A_UINT16)((pRH->regs[regLoc] & T2_NUMB_M) >> T2_NUMB_S);
                reg32   = pRH->regs[regLoc++] & T2_DATA_M;
                if (EarDebugLevel >= EAR_DEBUG_VERBOSE) {
                    printf("ar5212EarDoRH: Type 2 EAR analog modify bank %d, startBit %d, column %d numBits %d data %d\n",
                             bank, startBit, column, numBits, reg32);
                }
                ar5212ModifyRfBuffer(pRfRegs, reg32, numBits, startBit, column);
            }
        } while (!last);
        break;
    case EAR_TYPE3:
        do {
            opCode = (A_UINT16)((pRH->regs[regLoc] & T3_OPCODE_M) >> T3_OPCODE_S);
            last = (A_UINT16)(IS_TYPE3_LAST(pRH->regs[regLoc]));
            startBit = (A_UINT16)((pRH->regs[regLoc] & T3_START_M) >> T3_START_S);
            numBits = (A_UINT16)(pRH->regs[regLoc] & T3_NUMB_M);
            regLoc++;
            /* Grab Address */
            address = pRH->regs[regLoc++];
            if (numBits > 16) {
                reg32 = pRH->regs[regLoc++] << 16;
                reg32 |= pRH->regs[regLoc++];
            } else {
                reg32 = pRH->regs[regLoc++];
            }
            mask = ((1 << numBits) - 1) << startBit;
            regInit = REGR(devNum, address);
            switch (opCode) {
            case T3_OC_REPLACE:
                reg32 = (regInit & ~mask) | ((reg32 << startBit) & mask);
                break;
            case T3_OC_ADD:
                reg32 = (regInit & ~mask) |
                    (((regInit & mask) + (reg32 << startBit)) & mask);
                break;
            case T3_OC_SUB:
                reg32 = (regInit & ~mask) |
                    (((regInit & mask) - (reg32 << startBit)) & mask);
                break;
            case T3_OC_MUL:
                reg32 = (regInit & ~mask) |
                    (((regInit & mask) * (reg32 << startBit)) & mask);
                break;
            case T3_OC_XOR:
                reg32 = (regInit & ~mask) |
                    (((regInit & mask) ^ (reg32 << startBit)) & mask);
                break;
            case T3_OC_OR:
                reg32 = (regInit & ~mask) |
                    (((regInit & mask) | (reg32 << startBit)) & mask);
                break;
            case T3_OC_AND:
                reg32 = (regInit & ~mask) |
                    (((regInit & mask) & (reg32 << startBit)) & mask);
                break;
            }
            REGW(devNum, address, reg32);
            if (EarDebugLevel >= EAR_DEBUG_VERBOSE) {
                printf("ar5212EarDoRH: Type 3 EAR 32-bit rmw to 0x%04X : 0x%08x\n", address, reg32);
            }
        } while (!last);
        break;
    }
    return rfBankMask;
}

/**************************************************************
 * ar5212GetRfBank
 *
 * Get the UINT16 pointer to the start of the requested RF Bank
 */
static RF_REG_INFO *
ar5212GetRfBank(A_UINT32 devNum, A_UINT16 bank)
{
   	LIB_DEV_INFO *pLibDev = gLibInfo.pLibDevArray[devNum];
	
	if((bank < 1) || (bank > 7)) {
		mError(devNum, EINVAL, "ar5212GetRfBank: Illegal bank number = %d\n", bank);
		return(NULL);
	}

	return(&(pLibDev->rfBankInfo[bank]));
}

/**************************************************************
 * ar5212CFlagsToEarMode
 *
 * Convert from pLibDev info to the EAR bitmap format
 */
static A_UINT16
ar5212CFlagsToEarMode(A_UINT32 devNum)
{
	LIB_DEV_INFO *pLibDev = gLibInfo.pLibDevArray[devNum];

    if ((pLibDev->libCfgParams.enableXR) && (pLibDev->mode == MODE_11A) && (pLibDev->turbo == TURBO_ENABLE)) {
        return EAR_5T | EAR_XR;
    }
    if ((pLibDev->libCfgParams.enableXR) && (pLibDev->mode == MODE_11G) && (pLibDev->turbo == TURBO_ENABLE)) {
        return EAR_2T | EAR_XR;
    }

    if ((pLibDev->mode == MODE_11G) && (pLibDev->turbo == TURBO_ENABLE)) {
        return EAR_2T;
    }
    if ((pLibDev->mode == MODE_11A) && (pLibDev->turbo == TURBO_ENABLE)) {
        return EAR_5T;
    }
    if ((pLibDev->libCfgParams.enableXR) && (pLibDev->mode == MODE_11A) ) {
        return EAR_11A | EAR_XR;
    }
    if ((pLibDev->libCfgParams.enableXR) && (pLibDev->mode == MODE_11G)) {
        return EAR_11G | EAR_XR;
    }

    if (pLibDev->mode == MODE_11B) {
        return EAR_11B;
    }
    if (pLibDev->mode == MODE_11G) {
        return EAR_11G;
    }


    return EAR_11A;
}

/**************************************************************
 * ar5212IsChannelInEarRegHead
 *
 * Check if EAR should be applied for current channel
 */
static A_BOOL
ar5212IsChannelInEarRegHead(A_UINT16 earChannel, A_UINT16 channelMhz)
{
    int i;
    const A_UINT16 earFlaggedChannels[14][2] = {
        {2412, 2412}, {2417, 2437}, {2442, 2457}, {2462, 2467}, {2472, 2472}, {2484, 2484}, {2300, 2407},
        {4900, 5160}, {5170, 5180}, {5190, 5250}, {5260, 5300}, {5310, 5320}, {5500, 5700}, {5725, 5825}
    };

    /* No channel specified - applies to all channels */
    if (earChannel == 0) {
        return TRUE;
    }

    /* Do we match the single channel check if its set? */
    if (IS_CM_SINGLE(earChannel)) {
        return (A_CHAR)(((earChannel & ~0x8000) == channelMhz) ? TRUE : FALSE);
    }

    /* Rest of the channel modifier bits can be set as one or more ranges - check them all */
    if ((IS_CM_SPUR(earChannel)) && (IS_SPUR_CHAN(channelMhz))) {
        return TRUE;
    }
    for (i = 0; i < 14; i++) {
        if ((earChannel >> i) & 1) {
            if ((earFlaggedChannels[0][i] <= channelMhz) && (earFlaggedChannels[1][i] >= channelMhz)) {
                return TRUE;
            }
        }
    }
    return FALSE;
}

/**************************************************************
 * ar5212ModifyRfBuffer
 *
 * Perform analog "swizzling" of parameters into their location
 */
static void
ar5212ModifyRfBuffer(RF_REG_INFO *pRfRegs, A_UINT32 reg32, A_UINT32 numBits,
                     A_UINT32 firstBit, A_UINT32 column)
{
    A_UINT32 tmp32, mask, arrayEntry, lastBit;
    A_INT32  bitPosition, bitsLeft;

//    ASSERT(column <= 3);
//    ASSERT(numBits <= 32);
//    ASSERT(firstBit + numBits <= MAX_ANALOG_START);

    tmp32 = reverseBits(reg32, numBits);
    arrayEntry = (firstBit - 1) / 8;
    bitPosition = (firstBit - 1) % 8;
    bitsLeft = numBits;
    while (bitsLeft > 0) {
        lastBit = (bitPosition + bitsLeft > 8) ? (8) : (bitPosition + bitsLeft);
        mask = (((1 << lastBit) - 1) ^ ((1 << bitPosition) - 1)) << (column * 8);
        pRfRegs->pPciValues[arrayEntry].baseValue &= ~mask;
        pRfRegs->pPciValues[arrayEntry].baseValue |= ((tmp32 << bitPosition) << (column * 8)) & mask;
        bitsLeft -= (8 - bitPosition);
        tmp32 = tmp32 >> (8 - bitPosition);
        bitPosition = 0;
        arrayEntry++;
    }
}


/************ FORWARD SOFTWARE COMPATIBILITY EAR FUNCTIONS ************/

/**************************************************************************
 * ar5212EarPreParse
 *
 * Parse the entire EAR saving away the size required for each EAR register header
 * Allocation is only performed for matching version ID's.
 */
static int
ar5212EarPreParse(A_UINT32 *in, int numLocs, EAR_ALLOC *earAlloc)
{
    A_UINT16      numFound = 0;
    int           curEarLoc = 0;
    A_UINT16      numLocsConsumed;
    A_UINT16      type, regHead, tag, last;
    A_UINT32      verId;
    A_BOOL        verMaskMatch = FALSE;

    /* Record the version Id */
    verId = in[curEarLoc++];

    while (curEarLoc < numLocs) {
        /* Parse Version/Header/End */
        if (IS_VER_MASK(in[curEarLoc])) {
            verMaskMatch = IS_EAR_VMATCH(verId, in[curEarLoc]);
            curEarLoc++;
        } else if (IS_END_HEADER(in[curEarLoc])) {
            break;
        } else {
            /* Must be a register header - find number of locations consumed */
            regHead = (A_UINT16)(in[curEarLoc++]);

            if (IS_CM_SET(regHead)) {
                /* Bump location for channel modifier */
                curEarLoc++;
            }

            if (IS_DISABLER_SET(regHead)) {
                if (IS_PLL_SET(in[curEarLoc])) {
                    /* Bump location for PLL */
                    curEarLoc++;
                }
                /* Bump location for disabler */
                curEarLoc++;
            }

            numLocsConsumed = 0;
            /* Now into the actual register writes */
            type = (A_UINT16)((regHead & RH_TYPE_M) >> RH_TYPE_S);
            switch (type) {
            case EAR_TYPE0:
                do {
                    tag = (A_UINT16)(in[curEarLoc] & T0_TAG_M);
                    if ((tag == T0_TAG_32BIT) || (tag == T0_TAG_32BIT_LAST)) {
                        /* Full word writes */
                        numLocsConsumed += 3;
                        curEarLoc += 3;
                    } else {
                        /* Half word writes */
                        numLocsConsumed += 2;
                        curEarLoc += 2;
                    }
                }
                while ((tag != T0_TAG_32BIT_LAST) && (curEarLoc < numLocs));
                break;
            case EAR_TYPE1:
                numLocsConsumed = (A_UINT16)(((in[curEarLoc] & T1_NUM_M) * sizeof(A_UINT16)) + 3);
                curEarLoc += numLocsConsumed;
                break;
            case EAR_TYPE2:
                do {
                    last = (A_UINT16)(IS_TYPE2_LAST(in[curEarLoc]));
                    if(IS_TYPE2_EXTENDED(in[curEarLoc])) {
                        numLocsConsumed = (A_UINT16)(numLocsConsumed + 2 + A_DIV_UP(in[curEarLoc+1], 16));
                        curEarLoc += 2 + A_DIV_UP(in[curEarLoc+1], 16);
                    } else {
                        numLocsConsumed += 2;
                        curEarLoc += 2;
                    }
                } while (!last && (curEarLoc < numLocs));
                break;
            case EAR_TYPE3:
                do {
                    last = (A_UINT16)(IS_TYPE3_LAST(in[curEarLoc]));
                    numLocsConsumed = (A_UINT16)(numLocsConsumed + 2 + A_DIV_UP(in[curEarLoc] & 0x1F, 16));
                    curEarLoc += 2 + A_DIV_UP(in[curEarLoc] & 0x1F, 16);
                } while (!last && (curEarLoc < numLocs));
                break;
            }
            /* Only record allocation information for matching versions */
            if (verMaskMatch) {
                if (numLocsConsumed > EAR_MAX_RH_REGS) {
                    printf("ar5212EarPreParse: A register header exceeds the max allowable size\n");
                    earAlloc->numRHs = 0;
                    return 0;
                }
                earAlloc->locsPerRH[numFound] = numLocsConsumed;
                numFound++;
            }
        }
    }

    earAlloc->numRHs = numFound;

    if (EarDebugLevel >= 1) {
        printf("Number of locations parsed in preParse: %d\n", curEarLoc);
    }
    return curEarLoc;
}

/**************************************************************************
 * ar5212EarAllocate
 *
 * Now that the Ear structure size is known, allocate the EAR header and
 * the individual register headers
 */
static A_BOOL
ar5212EarAllocate(EAR_ALLOC *earAlloc, EAR_HEADER **ppEarHead)
{
    int             sizeForRHs;
    int             sizeForRegs = 0;
    int             i;
    A_UINT16        *currentAlloc;
    REGISTER_HEADER *pRH;

    /* Return if no applicable EAR exists */
    if (earAlloc->numRHs == 0) {
        return TRUE;
    }

    /* Allocate the Ear Header */
    *ppEarHead = (EAR_HEADER *)malloc(sizeof(EAR_HEADER));
    if (*ppEarHead == NULL) {
        printf("ar5212EarAllocate: Failed to retrieve space for EAR Header\n");
        return FALSE;
    }
    memset(*ppEarHead, 0, sizeof(EAR_HEADER));

    /* Save number of register headers */
    (*ppEarHead)->numRHs = earAlloc->numRHs;

    /* Size the malloc for the Ear Register Headers */
    sizeForRHs = earAlloc->numRHs * sizeof(REGISTER_HEADER);
    for (i = 0; i < earAlloc->numRHs; i++) {
        sizeForRegs += earAlloc->locsPerRH[i] * sizeof(A_UINT16);
    }

    if (EarDebugLevel >= EAR_DEBUG_VERBOSE) {
        printf("ar5212EarAllocate: Size for RH's %d, size for reg data %d\n", sizeForRHs, sizeForRegs);
    }

    /* Malloc and assign the space to the RH's */
    (*ppEarHead)->pRH = (REGISTER_HEADER *) malloc(sizeForRegs + sizeForRHs);
    if ((*ppEarHead)->pRH == NULL) {
        printf("ar5212EarAllocate: Failed to retrieve space for EAR individual registers\n");
        return 0;
    }
    memset((*ppEarHead)->pRH, 0, sizeForRegs + sizeForRHs);
    (*ppEarHead)->earSize = sizeForRegs + sizeForRHs;

    currentAlloc = (A_UINT16 *)(((A_UINT8 *)(*ppEarHead)->pRH) + sizeForRHs);
    for (i = 0; i < earAlloc->numRHs; i++) {
        if (EarDebugLevel >= EAR_DEBUG_EXTREME) {
             printf("ar5212EarAllocate: reg data %2d memory location 0x%08X\n", i, (A_UINT32)currentAlloc);
        }
        pRH = &((*ppEarHead)->pRH[i]);
        pRH->regs = currentAlloc;
        currentAlloc += earAlloc->locsPerRH[i];
    }
    return TRUE;
}

void
ar5212EarFree(EAR_HEADER *pEarHead) 
{
	if(pEarHead == NULL) {
		//already free
		return;
	}
	if(pEarHead->pRH) {
		free(pEarHead->pRH);
		pEarHead->pRH = NULL;
	}

	free(pEarHead);
	pEarHead = NULL;
	return;
}

/**************************************************************************
 * ar5212EarCheckAndFill
 *
 * Save away EAR contents
 */
static A_BOOL
ar5212EarCheckAndFill(EAR_HEADER *earHead, A_UINT32 *in, int totalLocs, EAR_ALLOC *pEarAlloc)
{
    int             curEarLoc = 0, currentRH = 0, regLoc, i;
    A_UINT16        regHead, tag, last, currentVerMask, addr, num, numBits, startBit;
    REGISTER_HEADER *pRH, tempRH;
    A_UINT16        tempRHregs[EAR_MAX_RH_REGS], *pReg16;
    A_BOOL          verMaskUsed, verMaskMatch;

    /* Setup the temporary register header */

    /* Save the version Id */
    earHead->versionId = (A_UINT16)in[curEarLoc++];

    /* Save the first version mask */
    verMaskUsed = 0;
    verMaskMatch = IS_EAR_VMATCH(earHead->versionId, in[curEarLoc]);
    currentVerMask = (A_UINT16)in[curEarLoc++];
    if (!IS_VER_MASK(currentVerMask)) {
        printf("ar5212EarCheckAndFill: ERROR: First entry after Version ID is not a Version Mask\n");
        return FALSE;
    }

    while (curEarLoc < totalLocs) {
        /* Parse Version/Header/End */
        if (IS_VER_MASK(in[curEarLoc])) {
            if (!verMaskUsed) {
                printf("ar5212EarCheckAndFill: ERROR: Multiple version masks setup with no register headers location %d\n",
                         curEarLoc);
                return FALSE;
            }
            verMaskUsed = 0;
            verMaskMatch = IS_EAR_VMATCH(earHead->versionId, in[curEarLoc]);
            currentVerMask = (A_UINT16)in[curEarLoc++];
        } else if (IS_END_HEADER(in[curEarLoc])) {
            printf("ar5212EarCheckAndFill: ERROR: Somehow we've hit the END location but parse shouldn't let us get here. loc %d\n",
                     curEarLoc);
            return FALSE;
        } else {
            if (currentRH > earHead->numRHs) {
                printf("ar5212EarCheckAndFill: ERROR: Exceeded number of register headers found in preParse. loc %d\n",
                         curEarLoc);
                return FALSE;
            }
            memset(&tempRH, 0, sizeof(REGISTER_HEADER));
            memset(&tempRHregs, 0, sizeof(A_UINT16) * EAR_MAX_RH_REGS);
            tempRH.regs = tempRHregs;
            pRH = &tempRH;

            /* Must be a register header - save last version mask */
            pRH->versionMask = currentVerMask;
            regHead = (A_UINT16)in[curEarLoc++];
            pRH->modes = (A_UINT16)(regHead & RH_MODES_M);
            if ((pRH->modes != RH_ALL_MODES) && (pRH->modes & RH_RESERVED_MODES)) {
                printf("ar5212EarCheckAndFill: WARNING: Detected that reserved modes have been set. loc %d\n", curEarLoc);
            }
            pRH->type = (A_UINT16)((regHead & RH_TYPE_M) >> RH_TYPE_S);
            pRH->stage = (A_UINT16)((regHead & RH_STAGE_M) >> RH_STAGE_S);
            if (IS_CM_SET(regHead)) {
                pRH->channel = (A_UINT16)in[curEarLoc++];
                if (IS_CM_SINGLE(pRH->channel) && !IS_5GHZ_CHAN(pRH->channel) &&
                    !IS_2GHZ_CHAN(pRH->channel))
                {
                    printf("ar5212EarCheckAndFill: WARNING: Specified single channel %d is not within tunable range. loc %d\n",
                             pRH->channel & CM_SINGLE_CHAN_M, curEarLoc);
                }
            }

            if (IS_DISABLER_SET(regHead)) {
                pRH->disabler.valid = TRUE;
                pRH->disabler.disableField = (A_UINT16)in[curEarLoc++];
                if (IS_PLL_SET(pRH->disabler.disableField)) {
                    pRH->disabler.pllValue = (A_UINT16)in[curEarLoc++];
                    if (pRH->disabler.pllValue & RH_RESERVED_PLL_BITS) {
                        printf("ar5212EarCheckAndFill: WARNING: Detected that reserved pll bits have been set. loc %d\n", curEarLoc);
                    }
                }
            }

            /* Now into the actual register writes */
            regLoc = 0;
            switch (pRH->type) {
            case EAR_TYPE0:
                do {
                    pRH->regs[regLoc] = (A_UINT16)in[curEarLoc++];
                    tag = (A_UINT16)(pRH->regs[regLoc] & T0_TAG_M);
                    if (!IS_VALID_ADDR(pRH->regs[regLoc] & ~T0_TAG_M)) {
                        printf("ar5212EarCheckAndFill: WARNING: Detected invalid register address 0x%04X at loc %d\n",
                                 pRH->regs[regLoc] & ~T0_TAG_M, curEarLoc);
                    }
                    regLoc++;
                    if ((tag == T0_TAG_32BIT) || (tag == T0_TAG_32BIT_LAST)) {
                        /* Full word writes */
                        pRH->regs[regLoc++] = (A_UINT16)in[curEarLoc++];
                        pRH->regs[regLoc++] = (A_UINT16)in[curEarLoc++];
                    } else {
                        /* Half word writes */
                        pRH->regs[regLoc++] = (A_UINT16)in[curEarLoc++];
                    }
                } while ((tag != T0_TAG_32BIT_LAST) && (curEarLoc < totalLocs));
                break;
            case EAR_TYPE1:
                pRH->regs[regLoc] = (A_UINT16)in[curEarLoc++];
                addr = (A_UINT16)(pRH->regs[regLoc] & ~T1_NUM_M);
                num = (A_UINT16)(pRH->regs[regLoc] & T1_NUM_M);
                regLoc++;
                for (i = 0; i < num + 1; i++) {
                    /* Full word writes */
                    if (!IS_VALID_ADDR(addr + (sizeof(A_UINT32) * i))) {
                        printf("ar5212EarCheckAndFill: WARNING: Detected invalid register address 0x%04X at loc %d\n",
                                 addr + (sizeof(A_UINT32) * i), curEarLoc);
                    }
                    pRH->regs[regLoc++] = (A_UINT16)in[curEarLoc++];
                    pRH->regs[regLoc++] = (A_UINT16)in[curEarLoc++];
                }
                break;
            case EAR_TYPE2:
                do {
                    pRH->regs[regLoc] = (A_UINT16)in[curEarLoc++];
                    last = (A_UINT16)(IS_TYPE2_LAST(pRH->regs[regLoc]));
                    if (((pRH->regs[regLoc] & T2_BANK_M) >> T2_BANK_S) == 0) {
                        printf("ar5212EarCheckAndFill: WARNING: Bank 0 update found in Type2 write at loc %d\n", curEarLoc);
                    }
                    startBit = (A_UINT16)(pRH->regs[regLoc] & T2_START_M);
                    if (IS_TYPE2_EXTENDED(pRH->regs[regLoc])) {
                        regLoc++;
                        pRH->regs[regLoc] = (A_UINT16)in[curEarLoc++];
                        if (pRH->regs[regLoc] < 12) {
                            printf("ar5212EarCheckAndFill: WARNING: Type2 Extended Write used when number of bits is under 12 at loc %d\n",
                                     curEarLoc);
                        }
                        num = (A_UINT16)(A_DIV_UP(pRH->regs[regLoc], 16));
                        numBits = pRH->regs[regLoc];
                        if (startBit + numBits > MAX_ANALOG_START) {
                            printf("ar5212EarCheckAndFill: ERROR: Type2 write will exceed analog buffer limits (at loc %d)\n", curEarLoc);
                            return FALSE;
                        }
                        regLoc++;
                        for (i = 0; i < num; i++) {
                            /* Add check data exceeds num bits check? */
                            pRH->regs[regLoc++] = (A_UINT16)in[curEarLoc++];
                        }
                        if (~((1 << (numBits % 16)) - 1) & in[curEarLoc - 1]) {
                            printf("ar5212EarCheckAndFill: WARNING: Type2 extended Write data exceeds number of bits specified at loc %d\n",
                                     curEarLoc);
                        }
                    } else {
                        regLoc++;
                        pRH->regs[regLoc] = (A_UINT16)in[curEarLoc++];
                        numBits = (A_UINT16)((pRH->regs[regLoc] & T2_NUMB_M) >> T2_NUMB_S);
                        if (startBit + numBits > MAX_ANALOG_START) {
                            printf("ar5212EarCheckAndFill: ERROR: Type2 write will exceed analog buffer limits (at loc %d)\n", curEarLoc);
                            return FALSE;
                        }
                        if (~((1 << numBits) - 1) &
                            (pRH->regs[regLoc] & T2_DATA_M))
                        {
                            printf("ar5212EarCheckAndFill: WARNING: Type2 Write data exceeds number of bits specified at loc %d\n",
                                     curEarLoc);
                        }
                        regLoc++;
                    }
                } while (!last && (curEarLoc < totalLocs));
                break;
            case EAR_TYPE3:
                do {
                    pRH->regs[regLoc] = (A_UINT16)in[curEarLoc++];
                    last = (A_UINT16)(IS_TYPE3_LAST(pRH->regs[regLoc]));
                    num = (A_UINT16)(A_DIV_UP((pRH->regs[regLoc] & T3_NUMB_M), 16));
                    if (((pRH->regs[regLoc] & T3_START_M) >> T3_START_S) +
                        (pRH->regs[regLoc] & T3_NUMB_M) > 31)
                    {
                        printf("ar5212EarCheckAndFill: WARNING: Type4 StartBit plus Number of Bits > 31 at loc %d\n",
                                 curEarLoc);
                    }
                    if (((pRH->regs[regLoc] & T3_OPCODE_M) >> T3_OPCODE_S) > T3_MAX_OPCODE) {
                        printf("ar5212EarCheckAndFill: WARNING: Type4 OpCode exceeds largest selectable opcode at loc %d\n",
                                 curEarLoc);
                    }
                    if (pRH->regs[regLoc] & T3_RESERVED_BITS) {
                        printf("ar5212EarCheckAndFill: WARNING: Type4 Reserved bits used at loc %d\n",
                                 curEarLoc);
                    }
                    numBits = (A_UINT16)(pRH->regs[regLoc] & T3_NUMB_M);
                    regLoc++;
                    /* Grab Address */
                    pRH->regs[regLoc] = (A_UINT16)in[curEarLoc++];
                    if (!IS_VALID_ADDR(pRH->regs[regLoc])) {
                        printf("ar5212EarCheckAndFill: WARNING: Detected invalid register address 0x%04X at loc %d\n",
                                 pRH->regs[regLoc], curEarLoc);
                    }
                    regLoc++;
                    for (i = 0; i < num; i++) {
                        pRH->regs[regLoc++] = (A_UINT16)in[curEarLoc++];
                    }
                    if (~((1 << (numBits % 16)) - 1) & in[curEarLoc - 1])
                    {
//                        printf("ar5212EarCheckAndFill: WARNING: Type3 write data exceeds number of bits specified at loc %d\n",
//                                 curEarLoc);
                    }
                } while (!last && (curEarLoc < totalLocs));
                break;
            }
            verMaskUsed = 1;
            /* Save any register headers that match the version mask*/
            if (verMaskMatch) {
                if (regLoc != pEarAlloc->locsPerRH[currentRH]) {
                    printf("ar5212EarCheckAndFill: Ear Allocation did not match Ear Usage\n");
                    return FALSE;
                }
                /*
                 * Copy stack register header to real register header on match
                 * BCOPY the regs, save away the reg16 ptr as the struct copy will overwrite it
                 * finally restore the reg16 ptr.
                 */
                memcpy(earHead->pRH[currentRH].regs, pRH->regs, regLoc * sizeof(A_UINT16));
                pReg16 = earHead->pRH[currentRH].regs;
                earHead->pRH[currentRH] = *pRH;
                earHead->pRH[currentRH].regs = pReg16;

                currentRH++;
            }
        } /* End Register Header Parse */
    } /* End Location Reading */
    return TRUE;
}

static void
showEarAlloc(EAR_ALLOC *earAlloc)
{
    int i;

    printf("Found %d register headers\n", earAlloc->numRHs);
    for (i = 0; i < earAlloc->numRHs; i++) {
        printf("Register Header %3d requires %2d 16-bit locations\n", i + 1,
                 earAlloc->locsPerRH[i]);
    }
}


A_BOOL
readEar
(
 A_UINT32 devNum,
 A_UINT32 earStartLocation
)
{
    EAR_ALLOC earAlloc;
    int       earLocs = 0, parsedLocs;
    A_UINT32  *earBuffer = NULL;
	LIB_DEV_INFO *pLibDev = gLibInfo.pLibDevArray[devNum];

    if(!pLibDev->libCfgParams.loadEar) {
//		printf("Information: Called readEar and loadEar was not set, returning\n");
		return TRUE;
	}

    if (earStartLocation)
		{
      //if((earStartLocation < ATHEROS_EEPROM_OFFSET) ||(earStartLocation > ATHEROS_EEPROM_END)) 
			if((earStartLocation < ATHEROS_EEPROM_OFFSET) ||(earStartLocation > checkSumLengthLib)) 
			{
				mError(devNum, EINVAL, "Illegal EAR start location\n");
				return FALSE;
			}
        
			//allocation a block of memory to take the eeprom locations

      //earLocs = ATHEROS_EEPROM_END - earStartLocation;
			earLocs = checkSumLengthLib - earStartLocation;
			earBuffer = (A_UINT32 *)malloc(sizeof(A_UINT32) * earLocs);
			if(earBuffer == NULL) 
			{
				mError(devNum, ENOMEM, "readEar: Unable to allocate memory for earBuffer\n");
				return FALSE;
			}
			eepromReadBlock(devNum, earStartLocation, earLocs, earBuffer);
    }

    if (earBuffer) 
		{
				memset(&earAlloc, 0, sizeof(EAR_ALLOC));
				parsedLocs = ar5212EarPreParse(earBuffer, earLocs, &earAlloc);

//#ifdef DEBUG
        if (EarDebugLevel >= EAR_DEBUG_VERBOSE) 
				{
            showEarAlloc(&earAlloc);
        }
//#endif

        /* Do not allocate EAR if no registers exist */
        if (earAlloc.numRHs) 
				{
						if(pLibDev->pEarHead) 
						{
								//free up previous allocation first
								ar5212EarFree(pLibDev->pEarHead);
						}
            if (ar5212EarAllocate(&earAlloc, &(pLibDev->pEarHead)) == FALSE) 
						{
                printf("ar5212Attach: Could not allocate memory for EAR structures\n");
                return FALSE;
            }

            if(!ar5212EarCheckAndFill(pLibDev->pEarHead, earBuffer, parsedLocs, &earAlloc)) 
						{
                /* Ear is bad */
                printf("ar5212Attach: An unrecoverable EAR error was detected\n");
                return FALSE;
            }
#ifdef DEBUG
//            if (EarDebugLevel >= EAR_DEBUG_BASIC) {
//                printEar(pDev->pHalInfo->pEarHead, &earAlloc);
//            }
#endif
        }
    }
	free(earBuffer);
	return TRUE;
}
