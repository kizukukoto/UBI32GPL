// dk_mem.c - handle memory management 

// Copyright © 2000-2001 Atheros Communications, Inc.,  All Rights Reserved.
 

// DESCRIPTION
// -----------
// Handles memory management.

// modification history
// --------------------
// 01dec01 	sharmat 	created (copied from windows client)


#include <stdio.h>
#include <stdlib.h>
#include "wlantype.h"
#include "dk_mem.h"

#if defined(VXWORKS) 
#include "hw.h"
#else
#include "common_hw.h"
#endif
#include "dk_common.h"



/**************************************************************************
* memGetIndexForBlock - find next free descriptor index for multiple block
*
* Go through bit masks to find free blocks that are contiguous
*
* RETURNS: A_OK if found one, A_ERROR otherwise
*/
// Function conflicting with function defined in mAlloc.c using suffix #2
A_STATUS memGetIndexForBlock2
(
	MDK_WLAN_DEV_INFO	*pdevInfo,	/* pointer to device info structure */
	A_UCHAR  *mapBytes,		/* pointer to the map bits to reference */
	A_UINT16 numBlocks,		/* number of blocks want to allocate */
	A_UINT16 *pIndex		/* gets updated with index */
)
{
	A_UCHAR		bitIndex;		/* the bit in the byte that is free */
	A_UINT16	byteIndex;		/* which byte contains the free bit */
	A_UINT16	blocksFound=0;  /* count of free blocks found so far */
	A_UCHAR		byteVar;
	A_UCHAR		firstBit;		/* bit index of first free block */
	A_UINT16    firstByte;		/* byte index of first free block */
	A_UINT32        NumBuffMapBytes;
	A_UINT32        NumBuffBlocks;

	NumBuffBlocks   = pdevInfo->pdkInfo->memSize / BUFF_BLOCK_SIZE;
	NumBuffMapBytes = NumBuffBlocks / 8;

	for (byteIndex = 0; byteIndex < NumBuffMapBytes; byteIndex++) {
		if (mapBytes[byteIndex] != 0xff) {
			byteVar=mapBytes[byteIndex];
			for(bitIndex = 0; bitIndex < 8; bitIndex++) {
				if((byteVar >> bitIndex) & 0x1) {
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
		return(A_NO_MEMORY);
	}

	/* calculate the block index */
	*pIndex = (8 * firstByte) + firstBit;

	/* update the number of blocks allocated */
	pdevInfo->pnumBuffs[*pIndex] = numBlocks;

	/* mark the bits as taken */
	blocksFound = 0; /* now using a blocks marked */
	for(byteIndex = firstByte; byteIndex < NumBuffMapBytes; byteIndex++) {
		for(bitIndex = firstBit; bitIndex < 8; bitIndex++) {
			mapBytes[byteIndex] |= (0x01 << bitIndex);
			blocksFound++;
			if (blocksFound == numBlocks) {
				return(A_OK);
			}
		}
		/* need to set bit index to zero, when move onto next byte */
		firstBit = 0; /* use firstBit because this is what for loop uses */
	}
	return(A_OK);
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
// Conflicting function  defined in mAlloc.c using suffix #2
void memMarkIndexesFree2
(
	MDK_WLAN_DEV_INFO	*pdevInfo,	/* pointer to device info structure */
	A_UINT16 index,				/* the index to free */
	A_UCHAR  *mapBytes			/* pointer to the map bits to reference */
)
{
	A_UCHAR			bitMask;		/* mask to clear the necessary bit */
	A_UINT16		byteIndex;
	A_UCHAR			bitIndex;
	A_UINT16		i;
	
	/* findout the bit and byte number of first bit */
	byteIndex = index >> 3;
	bitIndex = index & 0x07;

	/* clear the bits */
	for (i = 0; i < pdevInfo->pnumBuffs[index]; i++)
		{
		bitMask = ((0x01 << bitIndex) ^ 0xff);
		mapBytes[byteIndex] &= bitMask;
		if(bitMask == 0x7f)
			{
			bitIndex = 0;
			byteIndex++;
			}
		else
			{
			bitIndex++;
			}
		}
	return;
	}

