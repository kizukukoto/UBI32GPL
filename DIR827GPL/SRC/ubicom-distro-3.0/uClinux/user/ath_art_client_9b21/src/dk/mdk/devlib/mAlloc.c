/* mAlloc.c - contians memory allocaiton and free'ing functions */
/* Copyright (c) 2001 Atheros Communications, Inc., All Rights Reserved */

#ident  "ACI $Id: //depot/sw/src/dk/mdk/devlib/mAlloc.c#12 $, $Header: //depot/sw/src/dk/mdk/devlib/mAlloc.c#12 $"

/* 
Revsision history
--------------------
1.0       Created.
*/

/*
DESCRIPTION
This module handles memory management within application, for example
the allocation of descriptors from within the descriptor pool
*/

#ifdef __ATH_DJGPPDOS__
#include <unistd.h>
#ifndef EILSEQ  
    #define EILSEQ EIO
#endif	// EILSEQ

#define __int64	long long
#define HANDLE long
typedef unsigned long DWORD;
#endif	// #ifdef __ATH_DJGPPDOS__

#include <assert.h>
#include <errno.h>
#include "wlantype.h"
#include "athreg.h"
#include "manlib.h"
#include "mEeprom.h"
#include "mConfig.h"
#include "mdata.h"

A_BOOL memGetIndexForBlock(A_UINT32 devNum, A_UINT32 numBlocks, A_UINT32 *pIndex);
void memMarkIndexesFree(A_UINT32 devNum, A_UINT32 index);

/**************************************************************************
* memAlloc - allocate physically contiguous memory of size numBytes 
*
* Returns: The physical address of the memory allocated
*/
A_UINT32 memAlloc
(
 A_UINT32 devNum, 
 A_UINT32 numBytes
)
{
	LIB_DEV_INFO *pLibDev = gLibInfo.pLibDevArray[devNum];
	A_UINT32 physAddress;
    A_UINT32 offset;        /* offset from start of where descriptor is */
    A_UINT32 numBlocks;     /* num blocks need to allocate */
    A_UINT32 index;         /* index of first block to allocate */
#ifdef _DEBUG_ALL
	const A_UINT32 beefy = 0xBEEFBEEF;
//	const A_UINT32 beefy = 0x0;
	A_UINT32 i = 0;
#endif

   //printf("SNOOP::memAlloc:called\n"); 

	if(numBytes > pLibDev->devMap.DEV_MEMORY_RANGE) {
 		mError(devNum, EINVAL, "Device Number %d:memAlloc: Physical memory of size %ld out of range - returning NULL address\n", devNum, numBytes); 
        return((A_UINT32)NULL);
	}

    /* calculate how many blocks of memory are needed and round up result */
    numBlocks = A_DIV_UP(numBytes, BUFF_BLOCK_SIZE);
    
    if(memGetIndexForBlock(devNum, numBlocks, &index) != TRUE) {
		mError(devNum, ENOMEM, "Device Number %d:memAlloc: Failed to allocate physical memory of size %ld - returning NULL address\n", devNum, numBytes); 
        return((A_UINT32)NULL);
	}

    /* got an index, now calculate addresses */
    offset = index * BUFF_BLOCK_SIZE;
    physAddress = pLibDev->devMap.DEV_MEMORY_ADDRESS + offset;

#ifdef _DEBUG_ALL
	// Write allocated mem to BEEFBEEF for debug help
	for(i=0; i < (numBlocks * BUFF_BLOCK_SIZE); i+=4) {
        pLibDev->devMap.OSmemWrite(devNum, physAddress + i, (A_UCHAR *)&beefy, sizeof(beefy));
	}
#endif

    //printf("SNOOP::memAlloc::Allocated address %x for %d bytes\n", physAddress, numBytes);

    return(physAddress);    
}
/**************************************************************************
* memFree - Free the block allocated at the listed physical address
*
*/
void memFree
(
 A_UINT32 devNum,
 A_UINT32 physAddress
)
{
	LIB_DEV_INFO *pLibDev = gLibInfo.pLibDevArray[devNum];
	A_UINT32 index;

    //printf("SNOOP::memFree::devMap.DEV_MEMORY_ADDRESS=%x:range=%x:Free memory at address = %x\n", pLibDev->devMap.DEV_MEMORY_ADDRESS, pLibDev->devMap.DEV_MEMORY_RANGE, physAddress);

	if((physAddress < pLibDev->devMap.DEV_MEMORY_ADDRESS) || 
		(physAddress > pLibDev->devMap.DEV_MEMORY_ADDRESS + pLibDev->devMap.DEV_MEMORY_RANGE)) {
		mError(devNum, EINVAL, "Device Number %d:memFree: Cannot free memory outside the memory address range\n", devNum);
		return;
	}

    index = (physAddress - pLibDev->devMap.DEV_MEMORY_ADDRESS) / BUFF_BLOCK_SIZE;
	if(pLibDev->mem.pIndexBlocks[index] == 0) {
		mError(devNum, EINVAL, "Device Number %d:memFree: This phyAddress was never allocated\n", devNum);
		return;
	}

	memMarkIndexesFree(devNum, index);

	pLibDev->mem.pIndexBlocks[index] = 0;
}

/**************************************************************************
* memGetIndexForBlock - Go through bit masks to find free blocks that are 
*                       contiguous
*
* RETURNS: OK if found one, ERROR otherwise
*/
A_BOOL memGetIndexForBlock
(
 A_UINT32 devNum,
 A_UINT32 numBlocks,  /* number of blocks want to allocate */
 A_UINT32 *pIndex     /* gets updated with index */
)
{
	LIB_DEV_INFO *pLibDev = gLibInfo.pLibDevArray[devNum];

	A_UCHAR		bitIndex;		/* the bit in the byte that is free */
	A_UINT16	byteIndex;		/* which byte contains the free bit */
	A_UINT16	blocksFound=0;  /* count of free blocks found so far */
	A_UCHAR		byte;
	A_UCHAR		firstBit = 0;	/* bit index of first free block */
	A_UINT16    firstByte = 0;	/* byte index of first free block */

	for (byteIndex = 0; byteIndex < pLibDev->mem.allocMapSize; byteIndex++) {
		if (pLibDev->mem.pAllocMap[byteIndex] != 0xff) {
			byte=pLibDev->mem.pAllocMap[byteIndex];
			for(bitIndex = 0; bitIndex < 8; bitIndex++) {
				if((byte >> bitIndex) & 0x1) {
					/* index not free, need to start count again */
					blocksFound=0;
				}
				else {
					/* index is free, update count */
					blocksFound++;
					if(blocksFound==1) {
						/* update the index of the first block */
						firstBit = bitIndex;
						firstByte = byteIndex;

					}
				}

				if(blocksFound == numBlocks) {
					/* break out of first loop */
					break;
				}
			}
			if(blocksFound==numBlocks) {
				/* break out of second loop */
				break;
			}
		}
	}
		
	if (blocksFound != numBlocks) {
		/* we didn't find a free blcok */
		return(FALSE);
	}

	/* calculate the block index */
	*pIndex = (8 * firstByte) + firstBit;

	/* update the number of blocks allocated */
	pLibDev->mem.pIndexBlocks[*pIndex] = (A_UINT16)numBlocks;

	/* mark the bits as taken */
	blocksFound = 0; /* now using a blocks marked */
	for(byteIndex = firstByte; byteIndex < pLibDev->mem.allocMapSize; byteIndex++) {
		for(bitIndex = firstBit; bitIndex < 8; bitIndex++) {
			pLibDev->mem.pAllocMap[byteIndex] |= (0x01 << bitIndex);
			blocksFound++;
			if (blocksFound == numBlocks) {
				return(TRUE);
			}
		}
		/* need to set bit index to zero, when move onto next byte */
		firstBit = 0; /* use firstBit because this is what for loop uses */
	}
	return(TRUE);
}

/**************************************************************************
* memMarkIndexesFree - mark the specified indexes as free
*
* Find the byte number and bit position of index, mark bit as free, repeat 
* for all bits
* 
*
* RETURNS: N/A
*/
void memMarkIndexesFree
(
 A_UINT32 devNum,
 A_UINT32 index   /* the index to free */
)
{
	LIB_DEV_INFO *pLibDev = gLibInfo.pLibDevArray[devNum];

	A_UCHAR			bitMask;		/* mask to clear the necessary bit */
	A_UINT32		byteIndex;
	A_UCHAR			bitIndex;
	A_UINT16		i;
	
	/* findout the bit and byte number of first bit */
	byteIndex = index >> 3;  
	bitIndex = (A_UCHAR) (index & 0x07);

	/* clear the bits */
	for (i = 0; i < pLibDev->mem.pIndexBlocks[index]; i++) {
		bitMask = (A_UCHAR)((0x01 << bitIndex) ^ 0xff);
		pLibDev->mem.pAllocMap[byteIndex] &= bitMask;
		if(bitMask == 0x7f)	{
			bitIndex = 0;
			byteIndex++;
		}
		else {
			bitIndex++;
		}
	}
	return;
}

